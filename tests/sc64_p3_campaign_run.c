/* Normal-controller Higher Ground win attempt. Runtime reads choose inputs and
 * populate the trace; every gameplay mutation is produced by game_update(). */
#include "../starcraft/game.c"
#ifdef SC64_P34_NORMAL_HOME
#include "sc64_route_purity.h"
#include "sc64_controller_authored_pages.h"
static uint64_t route_after;
static int last_checkpoint_frame = -1;
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define RUN_LIMIT_DEFAULT 180000
#define CHECKPOINT_PERIOD 1000

static FILE *trace;
static u16 prior_keys;
#ifdef SC64_P34_NORMAL_HOME
static unsigned long host_ticks;
#else
static u32 host_ticks;
#endif
static int last_logged_keys = -1;
static int trace_first = 1;
static const char *trace_path;
static int run_limit = RUN_LIMIT_DEFAULT;
static int deployed;
static int controller_input_error;
static int nearest_side_building(void);
static void build_on_site(int type, int menu_row, int site_x, int site_y);

static void trace_prefix(void)
{
    if (!trace_first) fputs(",\n", trace);
    trace_first = 0;
}

static int owner_units(int owner)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++n;
    return n;
}

static int owner_buildings(int owner)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) ++n;
    return n;
}

static int owner_army(int owner)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && !sc64_race_is_worker(u[i].type) &&
            u[i].type != U_CHRYSALIS && u[i].type != U_OVERLORD) ++n;
    return n;
}

static int side_building(int type, int complete_only)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == type &&
            (!complete_only || !b[i].under_construction)) return i;
    return -1;
}

static int side_building_count(int type)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == type) ++n;
    return n;
}

static int side_construction_count(void)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].under_construction) ++n;
    return n;
}

static int side_active_builders(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && SC64_BUILD_WORKER_ORDER(u[i].order)) ++count;
    return count;
}

static int side_probe_count(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE) ++n;
    return n;
}

static int protected_route_x[2][512], protected_route_y[2][512];
static int protected_route_count[2];

static int nearest_building_for_owner(int owner, int x, int y)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) {
            int d = dist2(x, y, b[i].x, b[i].y);
            if (d < best_d) { best = i; best_d = d; }
        }
    return best;
}

static void collect_protected_routes(int home)
{
    int owner[2] = { ENEMY, 4 };
    for (int r = 0; r < 2; ++r) {
        protected_route_count[r] = 0;
        int target = nearest_building_for_owner(owner[r], b[home].x, b[home].y);
        if (target < 0) continue;
        int x = b[home].x, y = b[home].y;
        for (int step = 0; step < 512; ++step) {
            int nx, ny;
            protected_route_x[r][protected_route_count[r]] = x;
            protected_route_y[r][protected_route_count[r]++] = y;
            if (dist2(x, y, b[target].x, b[target].y) < 32 * 32 ||
                !path_next_tile(x, y, b[target].x, b[target].y, &nx, &ny) ||
                (nx == x && ny == y)) break;
            x = nx; y = ny;
        }
    }
}

static int site_near_protected_route(int x, int y)
{
    for (int r = 0; r < 2; ++r)
        for (int i = 0; i < protected_route_count[r]; ++i)
            if (dist2(x, y, protected_route_x[r][i], protected_route_y[r][i]) < 56 * 56)
                return 1;
    return 0;
}

static int site_on_protected_route(int x, int y)
{
    for (int r = 0; r < 2; ++r)
        for (int i = 0; i < protected_route_count[r]; ++i)
            if (dist2(x, y, protected_route_x[r][i], protected_route_y[r][i]) < 32 * 32)
                return 1;
    return 0;
}

/* An idle Probe still holding cargo cannot be sent back to minerals, so build
 * with empty ones. */
static int nearest_available_probe(int x, int y)
{
    int worker = -1, best = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            !u[i].carry && !SC64_BUILD_WORKER_ORDER(u[i].order)) {
            int d = dist2(u[i].x, u[i].y, x, y);
            int nx, ny;
            if (d < best && path_next_tile(u[i].x, u[i].y, x, y, &nx, &ny)) {
                best = d; worker = i;
            }
        }
    return worker;
}

static int queued_type(int type)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE)
        for (int j = 0; j < b[i].queue_count; ++j)
            if (b[i].queue[(b[i].queue_head + j) % 5] == type) ++n;
    return n;
}

static int closest_foe_building(int x, int y)
{
    int best = -1, best_d = 0x7fffffff;
    /* Both enemy forces share the mission. Prefer the nearest base reachable
       from the current army instead of committing every pass to one owner. */
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && (b[i].owner == ENEMY || b[i].owner == 4)) {
            int nx, ny;
            if (!path_next_tile(x, y, b[i].x, b[i].y, &nx, &ny)) continue;
            int d = dist2(x, y, b[i].x, b[i].y);
            if (d < best_d) { best = i; best_d = d; }
        }
    return best;
}

static int remaining_foe_buildings(void)
{ return owner_buildings(ENEMY) + owner_buildings(4); }

static void checkpoint(const char *kind)
{
    trace_prefix();
    fprintf(trace,
        "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%d,\"elapsed\":%u,"
        "\"screen\":%d,\"won\":%d,\"lost\":%d,\"p3_outcome\":%u,"
        "\"p3_sequence\":%u,\"p3_fired\":%u,\"p3_model_valid\":%d,"
        "\"campaign_phase\":%u,\"objective_flags\":%u,\"primary_objective\":%d,"
        "\"action_ticks\":%u,"
        "\"side_units\":%d,\"side_army\":%d,\"side_buildings\":%d,"
        "\"force2_units\":%d,\"force2_buildings\":%d,"
        "\"force3_units\":%d,\"force3_buildings\":%d,"
        "\"minerals\":%d,\"gas\":%d,\"supply\":%d,\"supply_cap\":%d,"
        "\"zealots_queued\":%d,\"gateways\":%d,\"cursor_x\":%d,\"cursor_y\":%d,"
        "\"selected_units\":%d,\"selected_building\":%d,\"geyser_count\":%d}\n",
        kind, (unsigned)host_ticks, frame, (unsigned)race_campaign.elapsed_frames,
        screen, won, lost, race_p3.outcome, race_p3.sequence, race_p3.fired,
        sc64_p3_model_state_valid(&race_p3), race_campaign.phase,
        race_campaign.objective_flags,
        (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) != 0,
        race_p3_action_ticks, owner_units(SIDE), owner_army(SIDE), owner_buildings(SIDE),
        owner_units(ENEMY), owner_buildings(ENEMY), owner_units(4), owner_buildings(4),
        minerals, gas, supply_used, supply_cap, queued_type(U_ZEALOT),
        side_building(B_GATEWAY, 0) >= 0, cx, cy, selected_count(), selected_building,
        geyser_count);
    fflush(trace);
}

static void step_raw(u16 held)
{
    if (deployed && (screen != SCREEN_PLAY || won || lost)) return;
    u16 pressed = held & (u16)~prior_keys;
#ifdef SC64_P34_NORMAL_HOME
    assert(route_purity_hash() == route_after);
#endif
    game_update(held, pressed);
#ifdef SC64_P34_NORMAL_HOME
    route_after = route_purity_hash();
#endif
    prior_keys = held;
    ++host_ticks;
    if (trace && held != (u16)last_logged_keys) {
        trace_prefix();
        fprintf(trace, "{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}\n",
                (unsigned)host_ticks, held, pressed);
        last_logged_keys = held;
        fflush(trace);
    }
    if (trace && deployed && screen == SCREEN_PLAY && frame > 0 && frame % CHECKPOINT_PERIOD == 0
#ifdef SC64_P34_NORMAL_HOME
        && frame != last_checkpoint_frame
#endif
        ) {
#ifdef SC64_P34_NORMAL_HOME
        last_checkpoint_frame = frame;
#endif
        checkpoint("periodic");
    }
}

static void step(u16 held)
{
    step_raw(held);
#ifdef SC64_P34_NORMAL_HOME
    sc64_controller_ack_pages(step_raw, held, &host_ticks, stderr);
#endif
}

static void tap(u16 key)
{
    step(key);
    step(0);
}

/* B reopens the order list on the last command, so walk to the row by input. */
static int open_menu_row(int row)
{
    tap(KEY_B);
    for (int guard = 0; guard < 2 * MENU_COUNT && menu_open && menu_cursor != row; ++guard)
        tap(menu_cursor < row ? KEY_DOWN : KEY_UP);
    return menu_open && menu_cursor == row;
}

static void ensure_normal_ui(void)
{
    for (int i = 0; i < 3 && (menu_open || mode != MODE_NORMAL); ++i) {
        if (menu_open || mode != MODE_NORMAL) tap(KEY_B);
    }
    if (menu_open || mode != MODE_NORMAL) {
        fprintf(stderr, "controller UI did not normalize at frame=%d mode=%d menu_open=%d menu=%d\n",
                frame, mode, menu_open, menu);
        controller_input_error = 1;
    }
}

static int snap4(int current, int target, int bound)
{
    int delta = target - current;
    int steps = delta >= 0 ? (delta + 2) / 4 : (delta - 2) / 4;
    int result = current + steps * 4;
    return result < 0 ? 0 : result >= bound ? bound - 1 : result;
}

static void move_cursor(int x, int y)
{
    if (menu_open) tap(KEY_B);
    x = snap4(cx, x, WORLD_W);
    y = snap4(cy, y, WORLD_H);
    int ticks = 0;
    while ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost && ticks++ < 4096) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT;
        if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        if (cy > y) held |= KEY_UP;
        step(held);
    }
    if ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost) {
        fprintf(stderr, "controller cursor stalled at frame=%d current=%d,%d target=%d,%d\n",
                frame, cx, cy, x, y);
        controller_input_error = 1;
    }
    step(0);
}

static void move_cursor_held(int x, int y, u16 held)
{
    x = snap4(cx, x, WORLD_W);
    y = snap4(cy, y, WORLD_H);
    int ticks = 0;
    while ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost && ticks++ < 4096) {
        u16 keys = held;
        if (cx < x) keys |= KEY_RIGHT;
        if (cx > x) keys |= KEY_LEFT;
        if (cy < y) keys |= KEY_DOWN;
        if (cy > y) keys |= KEY_UP;
        step(keys);
    }
    if ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost) {
        fprintf(stderr, "controller box cursor stalled at frame=%d current=%d,%d target=%d,%d\n",
                frame, cx, cy, x, y);
        controller_input_error = 1;
    }
}

static void select_box(int x, int y, int radius)
{
    move_cursor(x - radius, y - radius);
    step(KEY_L);
    move_cursor_held(x + radius, y + radius, KEY_L | KEY_R);
    step(0);
}

static int select_group_for_control(void)
{
    /* A selected producer consumes A as a queue command, so an attempted
       menu cancel never clears it. A harmless tiny box at the map corner
       clears the producer selection through the ordinary L-drag gesture. */
    if (selected_building >= 0) select_box(0, 0, 8);
    tap(KEY_R);
    return selected_count() > 0 && selected_building < 0;
}

static int attack_move_selected(int x, int y)
{
    if (selected_count() <= 0 || selected_building >= 0) return 0;
    if (!open_menu_row(MENU_ATTACK)) { ensure_normal_ui(); return 0; }
    tap(KEY_A);
    move_cursor(x, y);
    tap(KEY_A);
    return 1;
}

static void issue_attack_move(int x, int y)
{
    if (select_group_for_control()) attack_move_selected(x, y);
}

static void launch_p3(void)
{
    game_init();
#ifdef SC64_P34_NORMAL_HOME
    route_after = route_purity_hash();
#endif
    prior_keys = 0;
    tap(KEY_A);        /* HOME -> Protoss mission list */
    tap(KEY_RIGHT);    /* P3 ordinal 2, page two */
    step(KEY_A);       /* briefing */
    step(0);
#ifdef SC64_P34_NORMAL_HOME
    assert(sc64_controller_reveal_briefing(tap, SC64_P3_GLOBAL_ID, &host_ticks, stderr));
#else
    step(KEY_START);   /* explicit launch without a gameplay tick */
#endif
    deployed = 1;
    assert(screen == SCREEN_PLAY && !race_mode &&
           race_selected_mission == SC64_P3_GLOBAL_ID);
    assert(race_campaign.mission == SC64_P3_GLOBAL_ID &&
           sc64_p3_model_state_valid(&race_p3));
    assert(mission_unsupported_count == 0);
    assert(owner_units(SIDE) > 0 && owner_buildings(SIDE) > 0);
    assert(owner_units(ENEMY) > 0 && owner_buildings(ENEMY) > 0);
    assert(owner_units(4) > 0 && owner_buildings(4) > 0);
    fprintf(stderr, "P3 launch: side units/buildings=%d/%d force2=%d/%d force3=%d/%d resources=%d/%d geysers=%d\n",
            owner_units(SIDE), owner_buildings(SIDE), owner_units(ENEMY), owner_buildings(ENEMY),
            owner_units(4), owner_buildings(4), minerals, gas, geyser_count);
}

static int nearest_side_building(void)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_NEXUS) {
            int d = dist2(cx, cy, b[i].x, b[i].y);
            if (d < best_d) { best = i; best_d = d; }
        }
    return best;
}

/* Minerals left in reach of a Nexus, including one still under construction. */
static int minerals_near_nexus(void)
{
    int total = 0;
    for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0)
        for (int j = 0; j < MAX_BUILDINGS; ++j) if (b[j].alive && b[j].owner == SIDE &&
            b[j].type == B_NEXUS && dist2(m[i].x, m[i].y, b[j].x, b[j].y) < 240 * 240) {
            total += m[i].amount; break;
        }
    return total;
}

/* A placed building is paid for when its Probe arrives. */
static int spendable_minerals(void)
{
    int planned_minerals, planned_gas;
    race_pending_build_cost(SIDE, &planned_minerals, &planned_gas);
    return minerals - planned_minerals;
}

static void queue_zealot(void)
{
    for (int gateway = 0; gateway < MAX_BUILDINGS; ++gateway)
        if (b[gateway].alive && b[gateway].owner == SIDE && b[gateway].type == B_GATEWAY &&
            !b[gateway].under_construction && b[gateway].queue_count < 4 && spendable_minerals() >= 100 &&
            race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE) >= 4) {
            move_cursor(b[gateway].x, b[gateway].y);
            tap(KEY_A);
            if (train_type != U_ZEALOT) tap(KEY_R);
            tap(KEY_A);
        }
}

static void queue_dragoon(void)
{
    if (side_building(B_CYBERNETICS_CORE, 1) < 0) return;
    for (int gateway = 0; gateway < MAX_BUILDINGS; ++gateway)
        if (b[gateway].alive && b[gateway].owner == SIDE && b[gateway].type == B_GATEWAY &&
            !b[gateway].under_construction && b[gateway].queue_count < 4 && spendable_minerals() >= 125 &&
            gas >= 50 && race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE) >= 4) {
            move_cursor(b[gateway].x, b[gateway].y);
            tap(KEY_A);
            if (train_type != U_DRAGOON) tap(KEY_R);
            tap(KEY_A);
        }
}

static int find_build_site_near_nexus(int type, int *x_out, int *y_out)
{
    int nexus = nearest_side_building();
    if (nexus < 0) return 0;
    collect_protected_routes(nexus);
    int best = 0x7fffffff, found = 0;
    /* Preserve both live routes from the home base; leave a broad margin
       around each ramp and avoid fencing the mineral line. */
    for (int dy = -320; dy <= 320; dy += 16)
        for (int dx = -400; dx <= 400; dx += 16) {
            int x = b[nexus].x + dx, y = b[nexus].y + dy;
            if (x < 32 || x >= WORLD_W - 32 || y < 32 || y >= WORLD_H - 32 ||
                !race_can_site(SIDE, type, x, y) || !can_place(type, x, y)) continue;
            int near_mineral = 0;
            for (int mi = 0; mi < MAX_MINERALS; ++mi)
                if (m[mi].amount > 0 && dist2(x, y, m[mi].x, m[mi].y) < 64 * 64) {
                    near_mineral = 1; break;
                }
            if (near_mineral || site_near_protected_route(x, y)) continue;
            int d = dist2(b[nexus].x, b[nexus].y, x, y);
            /* Do not congest the starting Nexus/Pylons; leave workers an open
               approach lane so the construction order can actually arrive. */
            if (d < 80 * 80 || nearest_available_probe(x, y) < 0) continue;
            if (d < best) { best = d; *x_out = x; *y_out = y; found = 1; }
        }
    return found;
}

static int select_worker_at(int x, int y)
{
    ensure_normal_ui();
    /* Probes move while the input tape draws the box; keep enough margin for
       movement during the drag, then verify a Probe actually got selected. */
    select_box(x, y, 40);
    return find_selected_worker();
}

static void ensure_gateway(void)
{
    int site_x, site_y, worker = -1;
    if (side_building(B_GATEWAY, 0) >= 0 || minerals < 150 ||
        !find_build_site_near_nexus(B_GATEWAY, &site_x, &site_y)) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order != ORDER_BUILD) { worker = i; break; }
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    if (!open_menu_row(MENU_BARRACKS)) { ensure_normal_ui(); return; }
    tap(KEY_A); /* Protoss first building-menu entry is Gateway. */
    move_cursor(site_x, site_y);
    tap(KEY_A);
}

static void build_near_nexus(int type, int menu_row)
{
    ensure_normal_ui();
    int site_x, site_y, worker = -1, best = 0x7fffffff;
    int nexus = nearest_side_building();
    if (nexus < 0 || !find_build_site_near_nexus(type, &site_x, &site_y)) return;
    worker = nearest_available_probe(site_x, site_y);
    if (worker >= 0) best = dist2(u[worker].x, u[worker].y, site_x, site_y);
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    if (!open_menu_row(menu_row)) { ensure_normal_ui(); return; }
    tap(KEY_A);
    move_cursor(site_x, site_y);
    tap(KEY_A);
}

static void build_pylon_near_nexus(void)
{
    int nexus = nearest_side_building(), site_x = 0, site_y = 0;
    int best = 0x7fffffff;
    if (nexus < 0 || minerals < 100) return;
    collect_protected_routes(nexus);
    for (int dy = -400; dy <= 400; dy += 16)
        for (int dx = -400; dx <= 400; dx += 16) {
            int x = b[nexus].x + dx, y = b[nexus].y + dy;
            int d = dx * dx + dy * dy;
            if (d < 80 * 80 || x < 32 || x >= WORLD_W - 32 ||
                y < 32 || y >= WORLD_H - 32 || site_on_protected_route(x, y) ||
                !race_can_site(SIDE, B_PYLON, x, y) || !can_place(B_PYLON, x, y) ||
                nearest_available_probe(x, y) < 0) continue;
            int near_mineral = 0;
            for (int mi = 0; mi < MAX_MINERALS; ++mi)
                if (m[mi].amount > 0 && dist2(x, y, m[mi].x, m[mi].y) < 64 * 64) {
                    near_mineral = 1; break;
                }
            if (!near_mineral && d < best) { best = d; site_x = x; site_y = y; }
        }
    if (best != 0x7fffffff) build_on_site(B_PYLON, MENU_DEPOT, site_x, site_y);
}

static void build_on_site(int type, int menu_row, int site_x, int site_y)
{
    ensure_normal_ui();
    int worker = nearest_available_probe(site_x, site_y);
    if (!race_can_site(SIDE, type, site_x, site_y) || !can_place(type, site_x, site_y))
        return;
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    {
        int nx, ny;
        if (!path_next_tile(u[worker].x, u[worker].y, site_x, site_y, &nx, &ny)) return;
    }
    if (!open_menu_row(menu_row)) { ensure_normal_ui(); return; }
    tap(KEY_A);
    move_cursor(site_x, site_y);
    tap(KEY_A);
}

static void build_photon_cannon_near_nexus(void)
{
    ensure_normal_ui();
    int site_x = 0, site_y = 0, worker = -1, best = 0x7fffffff;
    int nexus = nearest_side_building();
    if (minerals < 150 || side_building_count(B_PHOTON_CANNON) >= 6 || nexus < 0)
        return;
    collect_protected_routes(nexus);
    /* Put static defense in a compact base ring. Keep clear of mineral patches
       and the path centerlines, while allowing the cannon footprint to sit
       beside the approach instead of requiring an entire 112px exclusion. */
    for (int dy = -224; dy <= 224; dy += 16)
        for (int dx = -240; dx <= 240; dx += 16) {
            int x = b[nexus].x + dx, y = b[nexus].y + dy;
            int d = dx * dx + dy * dy;
            if (d < 80 * 80 || d > 240 * 240 || x < 32 || x >= WORLD_W - 32 ||
                y < 32 || y >= WORLD_H - 32 || site_on_protected_route(x, y) ||
                !race_can_site(SIDE, B_PHOTON_CANNON, x, y) ||
                !can_place(B_PHOTON_CANNON, x, y) ||
                nearest_available_probe(x, y) < 0) continue;
            int near_mineral = 0;
            for (int mi = 0; mi < MAX_MINERALS; ++mi)
                if (m[mi].amount > 0 && dist2(x, y, m[mi].x, m[mi].y) < 64 * 64) {
                    near_mineral = 1; break;
                }
            if (!near_mineral && d < best) {
                best = d; site_x = x; site_y = y;
            }
        }
    if (best == 0x7fffffff) return;
    worker = nearest_available_probe(site_x, site_y);
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    if (!open_menu_row(MENU_STOP + 2) || !race_probe_cannon_command_available()) { ensure_normal_ui(); return; }
    tap(KEY_A);
    if (mode != MODE_BUILD || menu != B_PHOTON_CANNON) return;
    move_cursor(site_x, site_y);
    tap(KEY_A);
}

static int side_assimilator_near(int x, int y)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_ASSIMILATOR &&
            dist2(x, y, b[i].x, b[i].y) < 36 * 36) return i;
    return -1;
}

static void build_assimilator(void)
{
    if (minerals < 100) return;
    int nexus = nearest_side_building(), best = 0x7fffffff, pick = -1;
    if (nexus < 0) return;
    for (int i = 0; i < geyser_count; ++i)
        if (side_assimilator_near(geysers[i].x, geysers[i].y) < 0 &&
            race_can_site(SIDE, B_ASSIMILATOR, geysers[i].x, geysers[i].y) &&
            can_place(B_ASSIMILATOR, geysers[i].x, geysers[i].y) &&
            nearest_available_probe(geysers[i].x, geysers[i].y) >= 0) {
            int d = dist2(b[nexus].x, b[nexus].y, geysers[i].x, geysers[i].y);
            if (d < best) { best = d; pick = i; }
        }
    if (pick >= 0)
        build_on_site(B_ASSIMILATOR, MENU_REFINERY,
                      geysers[pick].x, geysers[pick].y);
}

static void assign_gas_probe(void)
{
    ensure_normal_ui();
    int gas_workers = 0;
    for (int j = 0; j < MAX_UNITS; ++j)
        if (u[j].alive && u[j].owner == SIDE && u[j].type == U_PROBE &&
            u[j].order == ORDER_GAS) ++gas_workers;
    /* Keep mining income available for reinforcements. When the mineral bank
       is low, move every gas worker back to a mineral patch using normal input. */
    int keep_gas = minerals < 200 ? 0 : 2;
    if (gas_workers > keep_gas) {
        int kept = 0;
        for (int j = 0; j < MAX_UNITS; ++j)
            if (u[j].alive && u[j].owner == SIDE && u[j].type == U_PROBE &&
                u[j].order == ORDER_GAS) {
                if (kept++ < keep_gas) continue;
                /* STOP idles the Probe; the mining pass then sends it to a patch. */
                if (u[j].carry) continue;
                select_box(u[j].x, u[j].y, 8);
                if (selected_building >= 0 || find_selected_worker() < 0) return;
                if (!open_menu_row(MENU_STOP)) { ensure_normal_ui(); return; }
                tap(KEY_A);
                return;
            }
    }
    if (gas_workers >= keep_gas) return;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_ASSIMILATOR &&
            !b[i].under_construction) {
            int nearest = -1, nearest_d = 0x7fffffff;
            for (int j = 0; j < MAX_UNITS; ++j)
                if (u[j].alive && u[j].owner == SIDE && u[j].type == U_PROBE &&
                    u[j].order != ORDER_GAS) {
                    int d = dist2(u[j].x, u[j].y, b[i].x, b[i].y);
                    if (d < nearest_d) { nearest = j; nearest_d = d; }
                    if (d < 24 * 24) {
                        select_box(u[j].x, u[j].y, 8);
                        move_cursor(b[i].x, b[i].y);
                        tap(KEY_A); /* normal worker context assigns this Probe to gas */
                        return;
                    }
                }
            if (nearest >= 0 && u[nearest].order != ORDER_MOVE) {
                select_box(u[nearest].x, u[nearest].y, 8);
                if (!open_menu_row(MENU_MOVE)) { ensure_normal_ui(); return; }
                tap(KEY_A);
                move_cursor(b[i].x, b[i].y);
                tap(KEY_A);
            }
        }
}

/* A Probe walking to a site reserves that building's cost until it arrives, so
 * one that cannot reach the site starves everything else. Give up after this
 * many frames. */
#define PENDING_BUILD_PATIENCE 1500
static int pending_build_since[MAX_UNITS];

static int recover_stalled_builder(void)
{
    ensure_normal_ui();
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || !SC64_PENDING_BUILD_ORDER(u[i].order)) {
            pending_build_since[i] = 0;
            continue;
        }
        if (!pending_build_since[i]) pending_build_since[i] = frame;
        if (frame - pending_build_since[i] > PENDING_BUILD_PATIENCE) {
            select_box(u[i].x, u[i].y, 8);
            if (selected_building >= 0 || find_selected_worker() < 0) return 0;
            if (!open_menu_row(MENU_STOP)) { ensure_normal_ui(); return 0; }
            tap(KEY_A);
            pending_build_since[i] = 0;
            return 1;
        }
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order == ORDER_BUILD && u[i].target >= 0 && u[i].target < MAX_BUILDINGS &&
            b[u[i].target].alive && b[u[i].target].under_construction &&
            dist2(u[i].x, u[i].y, b[u[i].target].x, b[u[i].target].y) > 36 * 36) {
            int nx, ny;
            int path = path_next_tile(u[i].x, u[i].y, b[u[i].target].x, b[u[i].target].y, &nx, &ny);
            if (path && u[i].path_retry < 8) continue;
            int wx = u[i].x, wy = u[i].y;
            select_box(wx, wy, 8);
            if (selected_count() != 1 || selected_building >= 0) return 0;
            if (!open_menu_row(MENU_STOP)) { ensure_normal_ui(); return 0; }
            tap(KEY_A); /* ordinary STOP releases the unreachable builder */
            return 1;
        }
    return 0;
}

#ifdef SC64_P34_NORMAL_HOME
/* Open a second mineral line before the starting bank is spent. */
static void build_forward_nexus(void)
{
    int home = side_building(B_NEXUS, 1), x = 0, y = 0, best = 0;
    if (home < 0 || side_building_count(B_NEXUS) != 1 || minerals < 500) return;
    for (int mslot = 0; mslot < MAX_MINERALS; ++mslot) {
        if (m[mslot].amount < 250 || !currently_visible(m[mslot].x,m[mslot].y) ||
            dist2(m[mslot].x,m[mslot].y,b[home].x,b[home].y) < 288*288) continue;
        for (int dy = -112; dy <= 112; dy += 32)
            for (int dx = -112; dx <= 112; dx += 32) {
                int sx = (m[mslot].x+dx)&~15, sy = (m[mslot].y+dy)&~15;
                int score = 0;
                if (sx < 64 || sy < 64 || sx >= WORLD_W-64 || sy >= WORLD_H-64 ||
                    !race_can_site(SIDE,B_NEXUS,sx,sy) || !can_place(B_NEXUS,sx,sy)) continue;
                for (int mi = 0; mi < MAX_MINERALS; ++mi)
                    if (m[mi].amount > 0 && dist2(sx,sy,m[mi].x,m[mi].y) < 160*160)
                        score += m[mi].amount;
                if (score <= best || nearest_available_probe(sx,sy) < 0) continue;
                best=score; x=sx; y=sy;
            }
    }
    if (best) {
        fprintf(stderr,"P3 forward Nexus frame=%d xy=%d,%d ore=%d\n",frame,x,y,best);
        build_on_site(B_NEXUS,MENU_STOP+1,x,y);
    }
}
#endif

static void build_pylons_and_gateways(void)
{
    if (side_active_builders() >= 2) return;
    int free_supply = race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE);
    int before = side_construction_count();
    /* Without nearest-depot delivery in this profile, Probes at a new Nexus would
     * still carry cargo home across the map, so stay on the home patches. */
    /* Tech/economy structures unlock more production and are otherwise
       starved indefinitely by supply depots when the army grows quickly. */
    if (side_building(B_ASSIMILATOR, 0) < 0 && minerals >= 100) {
        build_assimilator();
        if (side_construction_count() > before) return;
    }
    if (side_building(B_CYBERNETICS_CORE, 0) < 0 && minerals >= 200 &&
        side_building(B_GATEWAY, 1) >= 0) {
        build_near_nexus(B_CYBERNETICS_CORE, MENU_ACADEMY);
        if (side_construction_count() > before) return;
    }
    /* The home patches hold about 10,000 minerals and later Probes must walk
     * across the map, so spend on the army rather than spare production. */
    if (free_supply <= 8 && minerals >= 100 && side_building_count(B_PYLON) < 20) {
        build_pylon_near_nexus();
        if (side_construction_count() > before) return;
    }
    if (side_building_count(B_GATEWAY) < 4 && minerals >= 150) {
        build_near_nexus(B_GATEWAY, MENU_BARRACKS);
        if (side_construction_count() > before) return;
    }
}

static void queue_probe(void)
{
    ensure_normal_ui();
    int probe_total = side_probe_count() + queued_type(U_PROBE);
    if (probe_total >= 16 || minerals < 50) return;
    for (int nexus = 0; nexus < MAX_BUILDINGS && probe_total < 16; ++nexus)
        if (b[nexus].alive && b[nexus].owner == SIDE && b[nexus].type == B_NEXUS &&
            !b[nexus].under_construction && b[nexus].queue_count < 4 && minerals >= 50) {
            move_cursor(b[nexus].x, b[nexus].y);
            tap(KEY_A);
            if (selected_building != nexus) continue;
            tap(KEY_A); /* selected Nexus queues its Probe normally */
            ++probe_total;
        }
}

/* Idle Probes do not find work: starting Probes wait for an order and a spent
 * patch leaves its Probes standing on it. Send up to three each pass to the
 * least-worked visible patches near a Nexus. */
static void mine_idle_probes(void)
{
    for (int pass = 0; pass < 3 && screen == SCREEN_PLAY && !won && !lost; ++pass) {
        int probe = -1, patch = -1, best = 0x7fffffff;
        ensure_normal_ui();
        for (int i = 0; i < MAX_UNITS && probe < 0; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
                u[i].order == ORDER_IDLE && !u[i].carry) probe = i;
        if (probe < 0) return;
        for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0 && currently_visible(m[i].x, m[i].y)) {
            int depot = 0x7fffffff, workers = 0;
            for (int j = 0; j < MAX_BUILDINGS; ++j)
                if (b[j].alive && b[j].owner == SIDE && b[j].type == B_NEXUS && !b[j].under_construction) {
                    int d = dist2(m[i].x, m[i].y, b[j].x, b[j].y) / 256;
                    if (d < depot) depot = d;
                }
            if (depot == 0x7fffffff) continue;
            for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
                u[j].order == ORDER_MINE && u[j].target == i) ++workers;
            if (depot + workers * 256 < best) { best = depot + workers * 256; patch = i; }
        }
        if (patch < 0) return;
        select_box(u[probe].x, u[probe].y, 12);
        if (find_selected_worker() < 0 || selected_building >= 0) return;
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected && !sc64_race_is_worker(u[i].type)) return;
        /* A tap that lands on a unit standing at the patch selects it, so try
         * around the patch until the order takes. */
        for (int attempt = 0; attempt < 9 && u[probe].alive && u[probe].order != ORDER_MINE; ++attempt) {
            static const int offsets[][2] = {{0,0},{-8,0},{8,0},{0,-8},{0,8},
                                             {-12,-8},{12,-8},{-12,8},{12,8}};
            move_cursor(m[patch].x + offsets[attempt][0], m[patch].y + offsets[attempt][1]);
            tap(KEY_A);
        }
        /* Still refused: the last tap selected a Probe parked on the patch.
         * Walk it clear so the patch can be tapped next pass. */
        if (u[probe].alive && u[probe].order != ORDER_MINE && find_selected_worker() >= 0 &&
            selected_building < 0 && open_menu_row(MENU_MOVE)) {
            int home = nearest_side_building();
            tap(KEY_A);
            if (home >= 0) move_cursor(b[home].x, b[home].y + 48);
            tap(KEY_A);
            ensure_normal_ui();
            return;
        }
    }
}

static void issue_defense_order(void)
{
    ensure_normal_ui();
    int nexus = nearest_side_building(), target = -1, best = 0x7fffffff;
    if (nexus < 0) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && (u[i].owner == ENEMY || u[i].owner == 4)) {
            int d = dist2(b[nexus].x, b[nexus].y, u[i].x, u[i].y);
            if (d < best) { best = d; target = i; }
        }
    if (target < 0) return;
    int x = b[nexus].x, y = b[nexus].y;
    int dx = u[target].x - x, dy = u[target].y - y;
    if (FAST_ABS(dx) >= FAST_ABS(dy)) x += dx > 0 ? 48 : -48;
    else y += dy > 0 ? 48 : -48;
    issue_attack_move(limit(x, 0, WORLD_W - 1), limit(y, 0, WORLD_H - 1));
}

static void issue_sweep_order(void)
{
    ensure_normal_ui();
    int army = -1, best = 0x7fffffff, target = -1;
    /* Aim from the fighter nearest the cursor that can reach a foe base;
       one boxed in by the home buildings must not stall the assault. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type) &&
            u[i].type != U_OVERLORD && u[i].type != U_CHRYSALIS) {
            int d = dist2(cx, cy, u[i].x, u[i].y), reach;
            if (d >= best || (reach = closest_foe_building(u[i].x, u[i].y)) < 0) continue;
            best = d; army = i; target = reach;
        }
    if (army >= 0) issue_attack_move(b[target].x, b[target].y);
}

static void print_blocker(void)
{
    fprintf(stderr, "P3 limit: frame=%d result=%s forces units/buildings=%d/%d,%d/%d remaining=%d side units/army/buildings=%d/%d/%d resources=%d/%d supply=%d/%d waves=%u/%u\n",
            frame, won ? "WON" : lost ? "LOST" : "RUNNING",
            owner_units(ENEMY), owner_buildings(ENEMY), owner_units(4), owner_buildings(4),
            remaining_foe_buildings(), owner_units(SIDE), owner_army(SIDE), owner_buildings(SIDE),
            minerals, gas, supply_used, supply_cap, race_campaign.enemy_wave,
            race_campaign.enemy_units_sent);
    for (int t = 0; t < 64; ++t) {
        int n = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == t) ++n;
        if (n) fprintf(stderr, "  side unit type=%d count=%d\n", t, n);
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE)
            fprintf(stderr, "  probe order=%d target=%d at=%d,%d\n",
                    u[i].order, u[i].target, u[i].x, u[i].y);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type) &&
            u[i].type != U_OVERLORD && u[i].type != U_CHRYSALIS) {
            fprintf(stderr, "  army type=%d order=%d target=%d tx=%d,ty=%d at=%d,%d path=%d,%d retry=%d\n",
                    u[i].type, u[i].order, u[i].target, u[i].tx, u[i].ty, u[i].x, u[i].y,
                    u[i].path_x, u[i].path_y, u[i].path_retry);
        }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE)
            fprintf(stderr, "  side building raw=%d type=%d hp=%d/%d progress=%d queue=%d at=%d,%d\n",
                    b[i].raw_type, b[i].type, b[i].hp, b[i].maxhp, b[i].under_construction,
                    b[i].queue_count, b[i].x, b[i].y);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && (b[i].owner == ENEMY || b[i].owner == 4))
            fprintf(stderr, "  force owner=%d raw=%d type=%d hp=%d/%d at=%d,%d\n",
                    b[i].owner, b[i].raw_type, b[i].type, b[i].hp, b[i].maxhp, b[i].x, b[i].y);
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--trace") && i + 1 < argc) trace_path = argv[++i];
        else if (!strcmp(argv[i], "--limit") && i + 1 < argc) run_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_p3_campaign_run [--limit TICKS] [--trace PATH]");
            return 0;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            return 2;
        }
    }
    if (run_limit < 1 || run_limit > 1000000) return 2;
    mkdir(".local", 0777); mkdir(".local/sc64", 0777);
    if (!trace_path) trace_path = ".local/sc64/p3-controller-stock-asan-win.json";
    trace = fopen(trace_path, "w");
    assert(trace);
    fprintf(trace, "{\"schema\":1,\"mission\":22,\"policy\":\"protoss-stock-economy-cannons-assault\",\"events\":[\n");
    gba_init();
    launch_p3();
    checkpoint("start");

    int next_order = frame;
    int next_queue = frame + 30;
    int next_worker = frame + 60;
    int next_build = frame + 10;
    int next_gas = frame + 10;
    int assault_started = 0;
#ifdef SC64_P34_NORMAL_HOME
    int next_expansion = 12000;
#endif
    while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost &&
           !controller_input_error) {
        /* Failed placements can leave the ordinary command menu open, which
           freezes the native frame clock. Recover through the regular B menu
           input before deciding the next action. */
        if (menu_open || mode != MODE_NORMAL) ensure_normal_ui();
        if (frame >= next_build) {
            (void)recover_stalled_builder();
            build_pylons_and_gateways();
            next_build = frame + 240;
        }
#ifdef SC64_P34_NORMAL_HOME
        if (frame >= next_expansion && side_building_count(B_NEXUS) == 1) {
            build_forward_nexus();
            next_expansion = frame + 1200;
        }
#endif
        if (frame >= next_worker) {
            mine_idle_probes();
            queue_probe();
            next_worker = frame + 240;
        }
        if (frame >= next_gas) {
            assign_gas_probe();
            next_gas = frame + 300;
        }
        if (frame >= next_queue && side_building(B_GATEWAY, 1) >= 0) {
            if (side_building(B_CYBERNETICS_CORE, 1) >= 0 && gas >= 50)
                queue_dragoon();
            else
                queue_zealot();
#ifdef SC64_P4_STORM_ENABLED
            next_queue = frame + 240;
#else
            next_queue = frame + 360;
#endif
        }
        if (frame >= next_order) {
            /* Strike together and regroup when spent: units fed in piecemeal die
             * to the Sunken Colonies without clearing anything. The last enemy
             * units need less, and a force that cannot rebuild goes in anyway. */
            int army = owner_army(SIDE);
            int strike = owner_units(ENEMY) + owner_units(4) < army ? 16 : 32;
            if (assault_started && army < 10) assault_started = 0;
            if (!assault_started && (army >= strike ||
                                     (minerals_near_nexus() == 0 && minerals < 100 && army)))
                assault_started = 1;
            if (assault_started) {
                issue_sweep_order();
                next_order = frame + 1200;
            } else {
                issue_defense_order();
                next_order = frame + 900;
            }
        }
        step(0);
    }
    checkpoint(won ? "won" : lost ? "lost" : "limit");
    fprintf(trace, "]}\n");
    fclose(trace);
    printf("P3 controller run result=%s frame=%d outcome=%u sequence=%u campaign_phase=%u primary=%d remaining_enemy_buildings=%d side_units=%d side_buildings=%d trace=%s\n",
           won ? "WON" : lost ? "LOST" : "LIMIT", frame, race_p3.outcome,
           race_p3.sequence, race_campaign.phase,
           (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) != 0,
           remaining_foe_buildings(), owner_units(SIDE), owner_buildings(SIDE), trace_path);
    if (!won) print_blocker();
    int qualified = won && !lost && screen == SCREEN_RESULT &&
        sc64_p3_model_state_valid(&race_p3) &&
        race_p3.outcome == SC64_P3_WON &&
        race_p3.sequence == SC64_P3_SEQ_DONE_WIN &&
        race_campaign.phase == SC64_RACE_PHASE_COMPLETE &&
        (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) != 0 &&
        remaining_foe_buildings() == 0 && !controller_input_error;
    return qualified ? 0 : 1;
}
