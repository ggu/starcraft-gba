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
static int nearest_side_building(void);

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

static int side_probe_count(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE) ++n;
    return n;
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
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && (b[i].owner == ENEMY || b[i].owner == 4)) {
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
        "\"p3_sequence\":%u,\"p3_fired\":%u,\"action_ticks\":%u,"
        "\"side_units\":%d,\"side_army\":%d,\"side_buildings\":%d,"
        "\"force2_units\":%d,\"force2_buildings\":%d,"
        "\"force3_units\":%d,\"force3_buildings\":%d,"
        "\"minerals\":%d,\"gas\":%d,\"supply\":%d,\"supply_cap\":%d,"
        "\"zealots_queued\":%d,\"gateways\":%d,\"cursor_x\":%d,\"cursor_y\":%d,"
        "\"selected_units\":%d,\"selected_building\":%d,\"geyser_count\":%d}\n",
        kind, (unsigned)host_ticks, frame, (unsigned)race_campaign.elapsed_frames,
        screen, won, lost, race_p3.outcome, race_p3.sequence, race_p3.fired,
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

static int snap4(int current, int target, int bound)
{
    int delta = target - current;
    int steps = delta >= 0 ? (delta + 2) / 4 : (delta - 2) / 4;
    int result = current + steps * 4;
    return result < 0 ? 0 : result >= bound ? bound - 1 : result;
}

static void move_cursor(int x, int y)
{
    x = snap4(cx, x, WORLD_W);
    y = snap4(cy, y, WORLD_H);
    while ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT;
        if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        if (cy > y) held |= KEY_UP;
        step(held);
    }
    step(0);
}

static void move_cursor_held(int x, int y, u16 held)
{
    x = snap4(cx, x, WORLD_W);
    y = snap4(cy, y, WORLD_H);
    while (cx != x || cy != y) {
        u16 keys = held;
        if (cx < x) keys |= KEY_RIGHT;
        if (cx > x) keys |= KEY_LEFT;
        if (cy < y) keys |= KEY_DOWN;
        if (cy > y) keys |= KEY_UP;
        step(keys);
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

static void garrison_near_nexus(void)
{
    int nexus = nearest_side_building();
    if (nexus < 0) return;
    select_box(b[nexus].x, b[nexus].y - 48, 32);
    if (selected_count() > 0)
        attack_move_selected(b[nexus].x, b[nexus].y - 72);
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
            tap(KEY_A); tap(KEY_A);
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
            tap(KEY_A); tap(KEY_R); tap(KEY_A);
        }
}

static int find_build_site_near_nexus(int type, int *x_out, int *y_out)
{
    int nexus = nearest_side_building();
    if (nexus < 0) return 0;
    int best = 0x7fffffff, found = 0;
    for (int dy = -160; dy <= 160; dy += 16)
        for (int dx = -160; dx <= 160; dx += 16) {
            int x = b[nexus].x + dx, y = b[nexus].y + dy;
            if (x < 32 || x >= WORLD_W - 32 || y < 32 || y >= WORLD_H - 32 ||
                !race_can_site(SIDE, type, x, y) || !can_place(type, x, y)) continue;
            int d = dist2(b[nexus].x, b[nexus].y, x, y);
            /* Do not congest the starting Nexus/Pylons; leave workers an open
               approach lane so the construction order can actually arrive. */
            if (d < 80 * 80) continue;
            if (d < best) { best = d; *x_out = x; *y_out = y; found = 1; }
        }
    return found;
}

static int select_worker_at(int x, int y)
{
    select_box(x, y, 8);
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
    int site_x, site_y, worker = -1, best = 0x7fffffff;
    int nexus = nearest_side_building();
    if (nexus < 0 || !find_build_site_near_nexus(type, &site_x, &site_y)) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order != ORDER_BUILD) {
            int d = dist2(u[i].x, u[i].y, site_x, site_y);
            if (d < best) { best = d; worker = i; }
        }
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    tap(KEY_B);
    for (int i = 0; i < menu_row; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    move_cursor(site_x, site_y);
    tap(KEY_A);
}

static void build_on_site(int type, int menu_row, int site_x, int site_y)
{
    int worker = -1, best = 0x7fffffff;
    if (!race_can_site(SIDE, type, site_x, site_y) || !can_place(type, site_x, site_y))
        return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order != ORDER_BUILD) {
            int d = dist2(u[i].x, u[i].y, site_x, site_y);
            if (d < best) { best = d; worker = i; }
        }
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    tap(KEY_B);
    for (int i = 0; i < menu_row; ++i) tap(KEY_DOWN);
    tap(KEY_A);
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
            can_place(B_ASSIMILATOR, geysers[i].x, geysers[i].y)) {
            int d = dist2(b[nexus].x, b[nexus].y, geysers[i].x, geysers[i].y);
            if (d < best) { best = d; pick = i; }
        }
    if (pick >= 0)
        build_on_site(B_ASSIMILATOR, MENU_REFINERY,
                      geysers[pick].x, geysers[pick].y);
}

static void assign_gas_probe(void)
{
    int gas_workers = 0;
    for (int j = 0; j < MAX_UNITS; ++j)
        if (u[j].alive && u[j].owner == SIDE && u[j].type == U_PROBE &&
            u[j].order == ORDER_GAS) ++gas_workers;
    if (gas_workers >= 2) return;
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

static void build_pylons_and_gateways(void)
{
    int free_supply = race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE);
    if (free_supply <= 8 && minerals >= 100 &&
        side_building_count(B_PYLON) < 6)
        build_near_nexus(B_PYLON, MENU_DEPOT);
    if (side_building_count(B_GATEWAY) < 3 && minerals >= 150)
        build_near_nexus(B_GATEWAY, MENU_BARRACKS);
    if (side_building(B_CYBERNETICS_CORE, 0) < 0 && minerals >= 200 &&
        side_building(B_GATEWAY, 1) >= 0)
        build_near_nexus(B_CYBERNETICS_CORE, MENU_ACADEMY);
    if (side_building(B_ASSIMILATOR, 0) < 0 && minerals >= 100)
        build_assimilator();
}

static void queue_probe(void)
{
    int nexus = nearest_side_building();
    if (nexus < 0 || side_probe_count() >= 8 || b[nexus].queue_count >= 4 || minerals < 50)
        return;
    move_cursor(b[nexus].x, b[nexus].y);
    tap(KEY_A); tap(KEY_A); /* selected Nexus queues its Probe normally */
}

static void issue_defense_order(void)
{
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
    int army = -1, best = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type) &&
            u[i].type != U_OVERLORD && u[i].type != U_CHRYSALIS) {
            int d = dist2(cx, cy, u[i].x, u[i].y);
            if (d < best) { best = d; army = i; }
        }
    if (army < 0) return;
    int target = closest_foe_building(u[army].x, u[army].y);
    if (target >= 0) {
        issue_attack_move(b[target].x, b[target].y);
        garrison_near_nexus();
    }
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
            u[i].type != U_OVERLORD && u[i].type != U_CHRYSALIS)
            fprintf(stderr, "  army type=%d order=%d target=%d tx=%d,ty=%d at=%d,%d\n",
                    u[i].type, u[i].order, u[i].target, u[i].tx, u[i].ty, u[i].x, u[i].y);
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
    if (!trace_path) trace_path = ".local/sc64/p3-controller-attempt.json";
    trace = fopen(trace_path, "w");
    assert(trace);
    fprintf(trace, "{\"schema\":1,\"mission\":22,\"policy\":\"protoss-economy-tech-defense-assault\",\"events\":[\n");
    gba_init();
    launch_p3();
    checkpoint("start");

    int next_order = frame;
    int next_queue = frame + 30;
    int next_worker = frame + 60;
    int next_build = frame + 10;
    int next_gas = frame + 10;
    int assault_started = 0;
    while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost) {
        if (frame >= next_build) {
            build_pylons_and_gateways();
            next_build = frame + 1200;
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
            next_queue = frame + 360;
        }
        if (frame >= next_order) {
            if (!assault_started && frame >= 60000 && owner_army(SIDE) >= 20)
                assault_started = 1;
            if (assault_started) {
                issue_sweep_order();
                next_order = frame + 6000;
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
    printf("P3 controller run result=%s frame=%d outcome=%u remaining_enemy_buildings=%d side_units=%d side_buildings=%d trace=%s\n",
           won ? "WON" : lost ? "LOST" : "LIMIT", frame, race_p3.outcome,
           remaining_foe_buildings(), owner_units(SIDE), owner_buildings(SIDE), trace_path);
    if (!won) print_blocker();
    return won ? 0 : 1;
}
