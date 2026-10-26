/* Normal-controller Higher Ground win attempt. Runtime reads choose inputs and
 * populate the trace; every gameplay mutation is produced by game_update(). */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define RUN_LIMIT_DEFAULT 180000
#define CHECKPOINT_PERIOD 1000

static FILE *trace;
static u16 prior_keys;
static u32 host_ticks;
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
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order == ORDER_BUILD && u[i].target >= 0 &&
            u[i].target < MAX_BUILDINGS && b[u[i].target].alive &&
            b[u[i].target].under_construction) ++n;
    return n;
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

static int nearest_available_probe(int x, int y)
{
    int worker = -1, best = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order != ORDER_BUILD) {
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

static void step(u16 held)
{
    if (deployed && (screen != SCREEN_PLAY || won || lost)) return;
    u16 pressed = held & (u16)~prior_keys;
    game_update(held, pressed);
    prior_keys = held;
    ++host_ticks;
    if (trace && held != (u16)last_logged_keys) {
        trace_prefix();
        fprintf(trace, "{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}\n",
                (unsigned)host_ticks, held, pressed);
        last_logged_keys = held;
        fflush(trace);
    }
    if (trace && deployed && screen == SCREEN_PLAY && frame > 0 && frame % CHECKPOINT_PERIOD == 0)
        checkpoint("periodic");
}

static void tap(u16 key)
{
    step(key);
    step(0);
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
    tap(KEY_B);
    tap(KEY_DOWN); /* ATTACK MOVE */
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
    prior_keys = 0;
    tap(KEY_A);        /* HOME -> Protoss mission list */
    tap(KEY_RIGHT);    /* P3 ordinal 2, page two */
    step(KEY_A);       /* briefing */
    step(0);
    step(KEY_A);       /* launch without a gameplay tick */
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

static void queue_zealot(void)
{
    for (int gateway = 0; gateway < MAX_BUILDINGS; ++gateway)
        if (b[gateway].alive && b[gateway].owner == SIDE && b[gateway].type == B_GATEWAY &&
            !b[gateway].under_construction && b[gateway].queue_count < 4 && minerals >= 100 &&
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
            !b[gateway].under_construction && b[gateway].queue_count < 4 && minerals >= 125 &&
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

static int find_expansion_site(int *x_out, int *y_out)
{
    int home = nearest_side_building(), best = 0x7fffffff, found = 0;
    if (home < 0) return 0;
    collect_protected_routes(home);
    for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0 &&
        m[i].x < b[home].x - 256 && m[i].y > b[home].y - 320) {
        for (int dy = -96; dy <= 96; dy += 16)
            for (int dx = -96; dx <= 96; dx += 16) {
                int x = m[i].x + dx, y = m[i].y + dy;
                if (x < 32 || x >= WORLD_W - 32 || y < 32 || y >= WORLD_H - 32 ||
                    !race_can_site(SIDE, B_NEXUS, x, y) || !can_place(B_NEXUS, x, y) ||
                    site_near_protected_route(x, y))
                    continue;
                int d = dist2(b[home].x, b[home].y, x, y);
                if (d < best) { best = d; *x_out = x; *y_out = y; found = 1; }
            }
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
    tap(KEY_B);
    for (int i = 0; i < MENU_BARRACKS; ++i) tap(KEY_DOWN);
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
    tap(KEY_B);
    for (int i = 0; i < menu_row; ++i) tap(KEY_DOWN);
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
    tap(KEY_B);
    for (int i = 0; i < menu_row; ++i) tap(KEY_DOWN);
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
    tap(KEY_B);
    for (int i = 0; i < 12 && menu_open && menu_cursor != MENU_STOP + 2; ++i)
        tap(KEY_DOWN);
    if (!menu_open || menu_cursor != MENU_STOP + 2 ||
        !race_probe_cannon_command_available()) return;
    tap(KEY_A);
    if (mode != MODE_BUILD || menu != B_PHOTON_CANNON) return;
    move_cursor(site_x, site_y);
    tap(KEY_A);
}

static void build_mineral_expansion(void)
{
    int site_x, site_y, worker;
    if (side_building_count(B_NEXUS) >= 2 || minerals < 400 ||
        !find_expansion_site(&site_x, &site_y)) return;
    worker = nearest_available_probe(site_x, site_y);
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    tap(KEY_B);
    for (int i = 0; i < 12 && menu_open && menu_cursor != MENU_STOP + 1; ++i)
        tap(KEY_DOWN);
    if (!menu_open || menu_cursor != MENU_STOP + 1 ||
        !race_nexus_command_available()) return;
    tap(KEY_A);
    if (mode != MODE_BUILD || menu != B_NEXUS) return;
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
                int patch = nearest_mineral(u[j].x, u[j].y);
                if (patch < 0) return;
                select_box(u[j].x, u[j].y, 8);
                tap(KEY_B); tap(KEY_A); /* choose ordinary MOVE */
                move_cursor(m[patch].x, m[patch].y);
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
                tap(KEY_B); tap(KEY_A); /* choose normal MOVE order */
                move_cursor(b[i].x, b[i].y);
                tap(KEY_A);
            }
        }
}

static int recover_stalled_builder(void)
{
    ensure_normal_ui();
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
            tap(KEY_B);
            for (int row = 0; row < MENU_COUNT && menu_open && menu_cursor != MENU_STOP; ++row)
                tap(KEY_DOWN);
            if (!menu_open || menu_cursor != MENU_STOP) return 0;
            tap(KEY_A); /* ordinary STOP releases the unreachable builder */
            return 1;
        }
    return 0;
}

static void build_pylons_and_gateways(void)
{
    if (side_active_builders() >= 2) return;
    int free_supply = race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE);
    int before = side_construction_count();
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
    if (side_building(B_FORGE, 0) < 0 && minerals >= 150 &&
        side_building(B_GATEWAY, 1) >= 0) {
        build_near_nexus(B_FORGE, MENU_ENG_BAY);
        if (side_construction_count() > before) return;
    }
#ifdef SC64_P4_STORM_ENABLED
    /* The P4 carried roster outproduces the legacy P3 profile. Get a second
       production lane online before filling the defensive cannon roster. */
    if (side_building_count(B_GATEWAY) < 4 && minerals >= 150) {
        build_near_nexus(B_GATEWAY, MENU_BARRACKS);
        if (side_construction_count() > before) return;
    }
#endif
    if (side_building(B_FORGE, 1) >= 0 && minerals >= 150 &&
        side_building_count(B_PHOTON_CANNON) < 6) {
        build_photon_cannon_near_nexus();
        if (side_construction_count() > before) return;
    }
    if (free_supply <= 18 && minerals >= 100 && side_building_count(B_PYLON) < 20) {
        build_pylon_near_nexus();
        if (side_construction_count() > before) return;
    }
    if (side_building_count(B_GATEWAY) < 3 && minerals >= 150) {
        build_near_nexus(B_GATEWAY, MENU_BARRACKS);
        if (side_construction_count() > before) return;
    }
    if (frame >= 12000 && owner_buildings(ENEMY) == 0 && owner_units(ENEMY) == 0 &&
        side_building_count(B_NEXUS) < 2) {
        build_mineral_expansion();
        if (side_construction_count() > before) return;
    }
    if (side_building_count(B_GATEWAY) < 6 && minerals >= 150) {
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
    int army = -1, best = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type) &&
            u[i].type != U_OVERLORD && u[i].type != U_CHRYSALIS) {
            int d = dist2(cx, cy, u[i].x, u[i].y);
            if (d < best) { best = d; army = i; }
        }
    if (army < 0) return;
    int target = closest_foe_building(u[army].x, u[army].y);
    if (target >= 0) issue_attack_move(b[target].x, b[target].y);
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
    while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost &&
           !controller_input_error) {
        /* Failed placements can leave the ordinary command menu open, which
           freezes the native frame clock. Recover through the regular B menu
           input before deciding the next action. */
        if (menu_open || mode != MODE_NORMAL) ensure_normal_ui();
        if (frame >= next_build) {
            (void)recover_stalled_builder();
            build_pylons_and_gateways();
            next_build = frame + 500;
        }
        if (frame >= next_worker) {
            queue_probe();
            next_worker = frame + 600;
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
            if (owner_buildings(4) == 0 && owner_buildings(ENEMY) > 0)
                assault_started = owner_army(SIDE) >= 28;
            else if (!assault_started && frame >= 10000 && owner_army(SIDE) >= 10)
                assault_started = 1;
            if (assault_started) {
                issue_sweep_order();
                next_order = frame + 2200;
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
