/* Normal-controller P4 playthrough attempt. Controller choices use runtime
 * reads only; gameplay changes occur exclusively through game_update inputs. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define RUN_LIMIT_DEFAULT 180000
#define CHECKPOINT_PERIOD 900

static FILE *trace;
static u16 prior_keys;
static u32 host_ticks;
static int last_logged_keys = -1;
static int trace_first = 1;
static int run_limit = RUN_LIMIT_DEFAULT;
static int deployed;
static int last_order_frame = -1;
static int rescued_logged;
static int escape_group_ordered;
static int escape_cursor_ready;
static int heroes_released;
static const int P4_PARK_X = 520, P4_PARK_Y = 240;
static int controller_buildup_started;
static int controller_buildup_done;
static int controller_buildup_start_frame;
static int controller_buildup_pool_blocked_since = -1;
static int controller_probes_secured;
static int escape_waypoint_x = -1, escape_waypoint_y = -1;
static char attempt_path[160] = ".local/sc64/p4-controller-attempt-running.json";
static char final_attempt_path[160] = ".local/sc64/p4-controller-attempt.json";
static char verified_path[160] = ".local/sc64/p4-controller-win.json";

static void tap(u16 key);
static void move_cursor(int x, int y);
static int alive_units(void);
static int escort_combat_count(void);
static int cycle_select_actor(int slot);
static int source_pool_slack(void);
static int safe_escape_waypoint(int start_x, int start_y, int goal_x, int goal_y,
                                int *out_x, int *out_y);

static void trace_prefix(void)
{
    if (!trace_first) fputs(",\n", trace);
    trace_first = 0;
}

static int source_unit(int raw)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == raw) return i;
    return -1;
}

static int source_building(int raw, int owner)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw && b[i].owner == owner) return i;
    return -1;
}

static int side_building_type(int type, int complete_only)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == type &&
            (!complete_only || building_complete(i))) return i;
    return -1;
}

static int queue_zealot_at_gateway(int gateway)
{
    int core = side_building_type(B_CYBERNETICS_CORE, 1);
    int wanted = core >= 0 && building_complete(core) && gas >= 50 ? U_DRAGOON : U_ZEALOT;
    int mineral_cost = wanted == U_DRAGOON ? 125 : 100;
    int gas_cost = wanted == U_DRAGOON ? 50 : 0;
    if (gateway < 0 || !b[gateway].alive || b[gateway].owner != SIDE ||
        b[gateway].queue_count >= 5 || minerals < mineral_cost || gas < gas_cost) return 0;
    int old_count = b[gateway].queue_count;
    int old_minerals = minerals;
    move_cursor(b[gateway].x, b[gateway].y);
    tap(KEY_A);
    if (selected_building != gateway) return 0;
    for (int i = 0; i < 4 && train_type != wanted; ++i) tap(KEY_R);
    if (train_type != wanted) return 0;
    tap(KEY_A);
    int queued = b[gateway].queue_count > old_count || minerals < old_minerals;
    return queued;
}

static int side_probe_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE;
    return count;
}

static int queue_probe_at_nexus(void)
{
    int nexus = side_building_type(B_NEXUS, 1);
    if (nexus < 0 || b[nexus].queue_count >= 5 || minerals < 50 ||
        source_pool_slack() <= 0 ||
        race_owner_reserved_supply(SIDE) + 2 > race_owner_supply_cap(SIDE)) return 0;
    int before = b[nexus].queue_count;
    move_cursor(b[nexus].x, b[nexus].y);
    tap(KEY_A);
    if (selected_building != nexus) return 0;
    tap(KEY_A);
    int queued = b[nexus].queue_count > before &&
                 b[nexus].queue[(b[nexus].queue_head + b[nexus].queue_count - 1) % 5] == U_PROBE;
    return queued;
}

static int side_gas_worker_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
                 u[i].order == ORDER_GAS;
    return count;
}

static int side_queued_units(void)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE) count += b[i].queue_count;
    return count;
}

static int side_gateway_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        count += b[i].alive && b[i].owner == SIDE && b[i].type == B_GATEWAY;
    return count;
}

static int side_incomplete_pylon_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        count += b[i].alive && b[i].owner == SIDE && b[i].type == B_PYLON &&
                 b[i].under_construction;
    return count;
}

static int build_gateway_from_probe(int gateway_index)
{
    int worker = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order != ORDER_BUILD && u[i].order != ORDER_GAS) { worker = i; break; }
    if (worker < 0 || minerals < 150 || gateway_index >= 2) return 0;
    int x = -1, y = -1;
    static const int spot[][2] = {
        {672,320}, {720,320}, {672,384}, {720,384},
        {640,384}, {768,320}, {768,384}, {640,320},
        {672,256}, {720,256}, {640,432}, {768,432}
    };
    for (unsigned i = 0; i < sizeof(spot) / sizeof(spot[0]); ++i)
        if (race_can_site(SIDE, B_GATEWAY, spot[i][0], spot[i][1]) &&
            can_place(B_GATEWAY, spot[i][0], spot[i][1])) {
            x = spot[i][0]; y = spot[i][1]; break;
        }
    if (x < 0) return 0;
    if (!cycle_select_actor(worker)) return 0;
    tap(KEY_B);
    for (int i = 0; i < 2; ++i) tap(KEY_DOWN); /* Gateway is menu item two. */
    tap(KEY_A);
    if (mode != MODE_BUILD) return 0;
    move_cursor(x, y);
    tap(KEY_A);
    int target = u[worker].target;
    int gateway = target >= 0 && target < MAX_BUILDINGS && b[target].alive &&
                  b[target].owner == SIDE && b[target].type == B_GATEWAY &&
                  dist2(b[target].x, b[target].y, x, y) <= 8 * 8 ? target : -1;
    if (gateway >= 0 && u[worker].order == ORDER_BUILD) return 1;
    if (mode == MODE_BUILD) tap(KEY_B);
    if (menu_open) tap(KEY_B);
    return 0;
}

static int build_cybernetics_core_from_probe(void)
{
    int worker = -1;
    if (side_building_type(B_CYBERNETICS_CORE, 0) >= 0 || minerals < 200) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order != ORDER_BUILD && u[i].order != ORDER_GAS) { worker = i; break; }
    if (worker < 0) return 0;
    int nexus = source_building(154, SIDE);
    int x = -1, y = -1, best = 0x7fffffff;
    if (nexus >= 0) for (int yy = b[nexus].y - 160; yy <= b[nexus].y + 160; yy += 32)
        for (int xx = b[nexus].x - 192; xx <= b[nexus].x + 192; xx += 32) {
            if (xx < 32 || yy < 32 || xx >= WORLD_W - 32 || yy >= WORLD_H - 32 ||
                !race_can_site(SIDE, B_CYBERNETICS_CORE, xx, yy) ||
                !can_place(B_CYBERNETICS_CORE, xx, yy)) continue;
            int d = dist2(xx, yy, b[nexus].x, b[nexus].y);
            if (d < best) { x = xx; y = yy; best = d; }
        }
    if (worker < 0) return 0;
    if (x < 0 || !cycle_select_actor(worker)) {
        return 0;
    }
    tap(KEY_B);
    for (int i = 0; i < 5; ++i) tap(KEY_DOWN); /* Cybernetics Core is field-menu row five. */
    tap(KEY_A);
    if (mode != MODE_BUILD) return 0;
    move_cursor(x, y);
    tap(KEY_A);
    int target = u[worker].target;
    int core = target >= 0 && target < MAX_BUILDINGS && b[target].alive &&
               b[target].owner == SIDE && b[target].type == B_CYBERNETICS_CORE &&
               dist2(b[target].x, b[target].y, x, y) <= 8 * 8 ? target : -1;
    if (core >= 0 && u[worker].order == ORDER_BUILD) return 1;
    if (mode == MODE_BUILD) tap(KEY_B);
    if (menu_open) tap(KEY_B);
    return 0;
}

static int assign_probe_to_gas(void)
{
    int gas_workers = 0, assimilator = -1, worker = -1, best = 0x7fffffff;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_ASSIMILATOR &&
            building_complete(i)) { assimilator = i; break; }
    if (assimilator < 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || u[i].type != U_PROBE) continue;
        if (u[i].order == ORDER_GAS) { ++gas_workers; continue; }
        if (u[i].order == ORDER_BUILD) continue;
        int d = dist2(u[i].x, u[i].y, b[assimilator].x, b[assimilator].y);
        if (d < best) { best = d; worker = i; }
    }
    if (gas_workers >= 2 || worker < 0 || !cycle_select_actor(worker)) return 0;
    if (best <= 24 * 24) {
        move_cursor(b[assimilator].x, b[assimilator].y);
        tap(KEY_A);
        int assigned = u[worker].alive && u[worker].order == ORDER_GAS &&
                       u[worker].target == assimilator;
        return assigned;
    }
    if (u[worker].order == ORDER_MOVE &&
        dist2(u[worker].tx, u[worker].ty, b[assimilator].x, b[assimilator].y) < 24 * 24)
        return 0;
    tap(KEY_B);
    tap(KEY_A); /* normal Move order */
    if (mode != MODE_MOVE) { if (menu_open) tap(KEY_B); return 0; }
    move_cursor(b[assimilator].x, b[assimilator].y);
    tap(KEY_A);
    int moving = u[worker].alive && u[worker].order == ORDER_MOVE;
    return moving;
}

static int build_cannon_from_probe(int cannon_index)
{
    static const int spot[][2] = {
        {480,368}, {448,416}, {512,416}, {576,416}, {448,352}
    };
    int worker = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order != ORDER_BUILD && u[i].order != ORDER_GAS) { worker = i; break; }
    if (worker < 0 || minerals < 150 || cannon_index >= 3 ||
        source_building(166, SIDE) < 0) return 0;
    int x = -1, y = -1;
    for (unsigned i = 0; i < sizeof(spot) / sizeof(spot[0]); ++i)
        if (race_can_site(SIDE, B_PHOTON_CANNON, spot[i][0], spot[i][1]) &&
            can_place(B_PHOTON_CANNON, spot[i][0], spot[i][1])) {
            x = spot[i][0]; y = spot[i][1]; break;
        }
    if (x < 0 || !cycle_select_actor(worker)) return 0;
    tap(KEY_B);
    for (int i = 0; i < MENU_STOP + 2; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    if (mode != MODE_BUILD) return 0;
    move_cursor(x, y);
    tap(KEY_A);
    int target = u[worker].target;
    int cannon = target >= 0 && target < MAX_BUILDINGS && b[target].alive &&
                 b[target].owner == SIDE && b[target].type == B_PHOTON_CANNON &&
                 dist2(b[target].x, b[target].y, x, y) <= 8 * 8 ? target : -1;
    if (cannon >= 0 && u[worker].order == ORDER_BUILD) return 1;
    if (mode == MODE_BUILD) tap(KEY_B);
    if (menu_open) tap(KEY_B);
    return 0;
}

static int source_pool_slack(void)
{
    /* Reserve every still-authored scenario creation and every unit already
     * queued by the controller before asking the engine to train another. */
    return MAX_UNITS - alive_units() - (int)race_p4_reserved_units() -
           side_queued_units();
}

static int build_pylon_from_probe(void)
{
    static const int spot[][2] = {
        {544,368}, {608,368}, {672,368}, {544,432}, {736,368},
        {608,432}, {736,432}, {544,496}, {672,432}, {800,368},
        {800,432}, {608,496}, {736,496}, {800,496}, {480,432},
        {480,496}, {864,368}, {864,432}, {864,496}
    };
    int worker = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order != ORDER_BUILD && u[i].order != ORDER_GAS) {
            worker = i; break;
        }
    if (worker < 0 || minerals < 100) return 0;
    int x = -1, y = -1;
    int best = 0x7fffffff;
    for (unsigned i = 0; i < sizeof(spot) / sizeof(spot[0]); ++i)
        if (race_can_site(SIDE, B_PYLON, spot[i][0], spot[i][1]) &&
            can_place(B_PYLON, spot[i][0], spot[i][1])) {
            int next_x, next_y;
            if (!sc64_route_next(u[worker].x, u[worker].y, spot[i][0], spot[i][1],
                                 &next_x, &next_y)) continue;
            int d = dist2(u[worker].x, u[worker].y, spot[i][0], spot[i][1]);
            if (d < best) { x = spot[i][0]; y = spot[i][1]; best = d; }
        }
    if (x < 0) return 0;
    if (!cycle_select_actor(worker)) return 0;
    tap(KEY_B);
    for (int i = 0; i < 3; ++i) tap(KEY_DOWN); /* Protoss Depot slot is Pylon. */
    tap(KEY_A);
    if (mode != MODE_BUILD) return 0;
    move_cursor(x, y);
    tap(KEY_A);
    int target = u[worker].target;
    if (u[worker].order == ORDER_BUILD &&
        target >= 0 && target < MAX_BUILDINGS && b[target].alive &&
        b[target].owner == SIDE && b[target].type == B_PYLON &&
        dist2(b[target].x, b[target].y, x, y) <= 8 * 8) return 1;
    /* A rejected placement leaves the build cursor armed. Cancel through the
     * ordinary field menu so the next placement attempt starts cleanly. */
    if (mode == MODE_BUILD) tap(KEY_B);
    if (menu_open) tap(KEY_B);
    return 0;
}

static int owner_units(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].owner == owner;
    return count;
}

static int alive_units(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) count += u[i].alive != 0;
    return count;
}

static void checkpoint(const char *kind)
{
    int raw1_side = 0, raw1_rescuable = 0, raw1_buildings_side = 0,
        raw1_buildings_rescuable = 0;
    int probes = side_probe_count();
    int raynor = source_unit(SC64_P4_RAW_RAYNOR_VULTURE);
    int tassadar = source_unit(SC64_P4_RAW_TASSADAR);
    int near_enemy = -1, near_distance = 0x7fffffff;
    int hero_x = tassadar >= 0 ? u[tassadar].x : raynor >= 0 ? u[raynor].x : -1;
    int hero_y = tassadar >= 0 ? u[tassadar].y : raynor >= 0 ? u[raynor].y : -1;
    if (hero_x >= 0) for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || !owners_hostile(SIDE, u[i].owner)) continue;
        int d = dist2(hero_x, hero_y, u[i].x, u[i].y);
        if (d < near_distance) { near_distance = d; near_enemy = i; }
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].raw_owner == 1) {
        raw1_side += u[i].owner == SIDE;
        raw1_rescuable += u[i].owner == RESCUABLE;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].raw_owner == 1) {
        raw1_buildings_side += b[i].owner == SIDE;
        raw1_buildings_rescuable += b[i].owner == RESCUABLE;
    }
    int first_side = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) { first_side = i; break; }
    int l = 0, t = 0, r = 0, b = 0;
    (void)race_p4_location(SC64_P4_LOCATION_ESCAPE, &l, &t, &r, &b);
    trace_prefix();
    fprintf(trace,
        "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,"
        "\"screen\":%d,\"won\":%d,\"lost\":%d,\"outcome\":%u,"
        "\"fired\":%u,\"created\":%u,\"side_units\":%d,"
        "\"raynor_alive\":%d,\"raynor_owner\":%d,\"raynor_x\":%d,\"raynor_y\":%d,"
        "\"tassadar_alive\":%d,\"tassadar_owner\":%d,\"tassadar_x\":%d,\"tassadar_y\":%d,"
        "\"near_enemy\":[%d,%d,%d,%d,%d],"
        "\"escape\":[%d,%d,%d,%d],\"cursor\":[%d,%d],\"selected\":%d,"
        "\"reserve\":%u,\"faults\":%u,\"source_paused\":%u,\"action_cursor3\":%u,"
        "\"mode\":%d,\"menu_open\":%d,\"selected_building\":%d,\"first_side_order\":%d,"
        "\"first_side_pos\":[%d,%d],\"first_side_target\":[%d,%d],"
        "\"raw1_units\":[%d,%d],\"raw1_buildings\":[%d,%d],"
        "\"minerals\":%d,\"supply\":[%d,%d],\"probes\":%d,"
        "\"combat\":%d,\"queued\":%d,\"pool_slack\":%d,"
        "\"buildup\":[%d,%d,%d,%d]}\n",
        kind, (unsigned)host_ticks, (unsigned)frame, screen, won, lost,
        race_p4.model.outcome, race_p4.model.fired, race_p4.created_count,
        owner_units(SIDE), raynor >= 0, raynor >= 0 ? u[raynor].owner : -1,
        raynor >= 0 ? u[raynor].x : -1, raynor >= 0 ? u[raynor].y : -1,
        tassadar >= 0, tassadar >= 0 ? u[tassadar].owner : -1,
        tassadar >= 0 ? u[tassadar].x : -1, tassadar >= 0 ? u[tassadar].y : -1,
        near_enemy, near_enemy >= 0 ? u[near_enemy].owner : -1,
        near_enemy >= 0 ? u[near_enemy].raw_type : -1,
        near_enemy >= 0 ? u[near_enemy].x : -1,
        near_enemy >= 0 ? u[near_enemy].y : -1,
        l, t, r, b,
        cx, cy, selected_count(), race_p4_reserved_units(), race_p4.faults,
        race_p4.source_paused, race_p4.action_cursor[3], mode, menu_open,
        selected_building, first_side >= 0 ? u[first_side].order : -1,
        first_side >= 0 ? u[first_side].x : -1,
        first_side >= 0 ? u[first_side].y : -1,
        first_side >= 0 ? u[first_side].tx : -1,
        first_side >= 0 ? u[first_side].ty : -1,
        raw1_side, raw1_rescuable,
        raw1_buildings_side, raw1_buildings_rescuable,
        minerals, supply_used, supply_cap, probes, escort_combat_count(),
        side_queued_units(), source_pool_slack(), controller_buildup_started,
        controller_buildup_done, controller_buildup_start_frame,
        controller_probes_secured);
    fflush(trace);
}

static void step(u16 held)
{
    if (deployed && (screen != SCREEN_PLAY || won || lost)) return;
    u16 pressed = held & (u16)~prior_keys;
    game_update(held, pressed);
    prior_keys = held;
    ++host_ticks;
    if (held != (u16)last_logged_keys) {
        trace_prefix();
        fprintf(trace, "{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}\n",
                (unsigned)host_ticks, held, pressed);
        last_logged_keys = held;
        fflush(trace);
    }
    if (deployed && screen == SCREEN_PLAY && frame > 0 &&
        frame % CHECKPOINT_PERIOD == 0) checkpoint("periodic");
}

static void tap(u16 key) { step(key); step(0); }

/* P4's authored presentation pause advances campaign timers but freezes all
 * controller actions. Never send a key or move the cursor into that interval. */
static void wait_for_controller(void)
{
    while (deployed && screen == SCREEN_PLAY && !won && !lost &&
           race_p4.source_paused)
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
    wait_for_controller();
    x = snap4(cx, x, WORLD_W);
    y = snap4(cy, y, WORLD_H);
    while ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost) {
        if (race_p4.source_paused) { wait_for_controller(); continue; }
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT;
        if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        if (cy > y) held |= KEY_UP;
        step(held);
    }
    step(0);
}

static void select_box(int x, int y, int radius)
{
    move_cursor(x - radius, y - radius);
    step(KEY_L);
    int end_x = snap4(cx, x + radius, WORLD_W);
    int end_y = snap4(cy, y + radius, WORLD_H);
    while ((cx != end_x || cy != end_y) && screen == SCREEN_PLAY) {
        u16 held = KEY_L | KEY_R;
        if (cx < end_x) held |= KEY_RIGHT;
        if (cx > end_x) held |= KEY_LEFT;
        if (cy < end_y) held |= KEY_DOWN;
        if (cy > end_y) held |= KEY_UP;
        step(held);
    }
    step(0);
}

static int move_cursor_to_actor(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive) return 0;
    for (int attempt = 0; attempt < 12; ++attempt) {
        wait_for_controller();
        move_cursor(u[slot].x, u[slot].y);
        if (!u[slot].alive) return 0;
        if (dist2(cx, cy, u[slot].x, u[slot].y) <= 24 * 24) return 1;
    }
    return 0;
}

static int select_actor(int slot)
{
    if (selected_building >= 0) select_box(0, 0, 8);
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE)
        return 0;
    move_cursor(u[slot].x, u[slot].y);
    tap(KEY_A);
    return u[slot].selected && selected_building < 0;
}

/* Use the normal Protoss field-command menu and cursor input to cast Storm.
 * Adaptive enemy reads only choose a target; all gameplay changes still pass
 * through game_update. The splash-risk threshold is intentionally cautious
 * because Storm also damages allied units. */
static int try_cast_storm(void)
{
    int caster = -1, target_x = -1, target_y = -1, best_score = 0;
    if (!deployed || race_p4.source_paused || screen != SCREEN_PLAY || won || lost ||
        mode != MODE_NORMAL || menu_open) return 0;
    /* Keep the objective hero out of optional Storm targeting when a rescued
     * High Templar is available, but use Tassadar if he alone can reach a
     * useful cluster or the Templar lacks enough energy. */
    for (int pass = 0; pass < 2; ++pass) {
        int candidate = -1, candidate_score = 0, candidate_x = -1, candidate_y = -1;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE &&
                u[i].raw_type == (pass == 0 ? SC64_P4_STORM_HIGH_TEMPLAR_RAW :
                                              SC64_P4_RAW_TASSADAR) &&
                u[i].carry >= SC64_STORM_ENERGY_COST * SC64_STORM_ENERGY_ONE) {
                candidate = i; break;
            }
        if (candidate < 0) continue;
        for (int e = 0; e < MAX_UNITS; ++e) {
            if (!u[e].alive || !owners_hostile(SIDE, u[e].owner) ||
                (sc64_game_unit_actor_flags(u[e].type) &
                 (SC64_GAME_ACTOR_HIDDEN | SC64_GAME_ACTOR_INVINCIBLE))) continue;
            int dx = u[e].x - u[candidate].x, dy = u[e].y - u[candidate].y;
            if (dx * dx + dy * dy > SC64_STORM_CAST_RANGE_GBA_PX *
                                    SC64_STORM_CAST_RANGE_GBA_PX) continue;
            int enemies = 0, friends = 0, hero_threat = 0;
            for (int hero_raw_i = 0; hero_raw_i < MAX_UNITS; ++hero_raw_i)
                if (u[hero_raw_i].alive && u[hero_raw_i].owner == SIDE &&
                    (u[hero_raw_i].raw_type == SC64_P4_RAW_TASSADAR ||
                     u[hero_raw_i].raw_type == SC64_P4_RAW_RAYNOR_VULTURE) &&
                    dist2(u[e].x, u[e].y, u[hero_raw_i].x, u[hero_raw_i].y) <= 96 * 96)
                    hero_threat = 1;
            for (int j = 0; j < MAX_UNITS; ++j) {
                if (!u[j].alive || sc64_game_unit_actor_flags(u[j].type) &
                    (SC64_GAME_ACTOR_HIDDEN | SC64_GAME_ACTOR_INVINCIBLE)) continue;
                if (dist2(u[e].x, u[e].y, u[j].x, u[j].y) >
                    SC64_STORM_RADIUS_GBA_PX * SC64_STORM_RADIUS_GBA_PX) continue;
                if (owners_hostile(SIDE, u[j].owner)) ++enemies;
                else if (u[j].owner == SIDE) ++friends;
            }
            int score = enemies * 100 - friends * 120;
            /* A nearby hostile is worth spending Storm to protect an
             * objective hero even when friendly splash makes the raw score
             * slightly negative; collateral remains a substantial penalty. */
            if (hero_threat) score += 300;
            if ((enemies >= 2 || (hero_threat && enemies >= 1) ||
                 (enemies >= 1 && friends == 0)) && score > candidate_score) {
                candidate_score = score; candidate_x = u[e].x; candidate_y = u[e].y;
            }
        }
        if (candidate_x >= 0 && candidate_score >= 100 &&
            candidate_score > best_score) {
            caster = candidate; target_x = candidate_x; target_y = candidate_y;
            best_score = candidate_score;
        }
    }
    if (caster < 0) return 0;
    wait_for_controller();
    if (!select_actor(caster) || selected_count() != 1 ||
        !race_p4_storm_menu_available()) return 0;
    int effects_before = 0;
    for (int i = 0; i < SC64_STORM_MAX_ACTIVE; ++i)
        effects_before += race_p4.storm.storm[i].active != 0;
    wait_for_controller(); tap(KEY_B);
    for (int i = 0; i < 10; ++i) { wait_for_controller(); tap(KEY_DOWN); }
    wait_for_controller(); tap(KEY_A);
    if (mode != MODE_P4_STORM) return 0;
    move_cursor(target_x, target_y);
    wait_for_controller(); tap(KEY_A);
    int effects_after = 0;
    for (int i = 0; i < SC64_STORM_MAX_ACTIVE; ++i)
        effects_after += race_p4.storm.storm[i].active != 0;
    if (effects_after > effects_before) return 1;
    return 0;
}

static int select_army_box(void)
{
    int xs[MAX_UNITS], ys[MAX_UNITS], n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || u[i].type == U_TASSADAR ||
            u[i].type == U_ZP_RAYNOR_VULTURE || sc64_race_is_worker(u[i].type)) continue;
        xs[n] = u[i].x; ys[n] = u[i].y; ++n;
    }
    if (!n) return 0;
    for (int i = 1; i < n; ++i) {
        int x = xs[i], y = ys[i], j = i;
        while (j && xs[j - 1] > x) { xs[j] = xs[j - 1]; --j; }
        xs[j] = x;
    }
    for (int i = 1; i < n; ++i) {
        int y = ys[i], j = i;
        while (j && ys[j - 1] > y) { ys[j] = ys[j - 1]; --j; }
        ys[j] = y;
    }
    int cx0 = xs[n / 2], cy0 = ys[n / 2];
    int radius = 160;
    select_box(cx0, cy0, radius);
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
        if (u[i].type == U_TASSADAR || u[i].type == U_ZP_RAYNOR_VULTURE) return 0;
        ++count;
    }
    return count > 0;
}

/* A no-drag L tap cycles the game's ordinary single-unit selection. This is
 * more reliable than clicking when a rescued hero is stacked with escorts. */
static int cycle_select_actor(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE)
        return 0;
    for (int i = 0; i <= MAX_UNITS; ++i) {
        if (u[slot].selected && selected_count() == 1) return 1;
        wait_for_controller();
        tap(KEY_L);
        if (!u[slot].alive || screen != SCREEN_PLAY) return 0;
    }
    return 0;
}

static int stop_rescue_hero(int slot)
{
    if (slot < 0 || !u[slot].alive || u[slot].owner != SIDE) return 0;
    wait_for_controller();
    if (!move_cursor_to_actor(slot)) return 0;
    wait_for_controller();
    tap(KEY_A);
    if (!u[slot].selected || selected_count() != 1) return 0;
    wait_for_controller(); tap(KEY_B);
    for (int i = 0; i < MENU_STOP; ++i) { wait_for_controller(); tap(KEY_DOWN); }
    wait_for_controller(); tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_IDLE;
}

static int move_rescue_hero_to_safety(int slot, int safe_x, int safe_y)
{
    if (slot < 0 || !u[slot].alive || u[slot].owner != SIDE) return 0;
    wait_for_controller();
    if (!cycle_select_actor(slot)) return 0;
    wait_for_controller(); tap(KEY_B);
    wait_for_controller(); tap(KEY_A); /* ordinary Move, first field-menu item */
    if (mode != MODE_MOVE) return 0;
    move_cursor(safe_x, safe_y);
    wait_for_controller(); tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_MOVE;
}

static int issue_hero_attack_move(int slot, int x, int y)
{
    if (slot < 0 || !u[slot].alive || u[slot].owner != SIDE) return 0;
    wait_for_controller();
    if (!cycle_select_actor(slot)) return 0;
    wait_for_controller(); tap(KEY_B);
    wait_for_controller(); tap(KEY_DOWN);
    wait_for_controller(); tap(KEY_A);
    if (mode != MODE_ATTACK) return 0;
    move_cursor(x, y);
    wait_for_controller(); tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_ATTACK_MOVE;
}

static int issue_hero_move(int slot, int x, int y)
{
    if (slot < 0 || !u[slot].alive || u[slot].owner != SIDE) return 0;
    wait_for_controller();
    if (!cycle_select_actor(slot)) return 0;
    wait_for_controller(); tap(KEY_B);
    wait_for_controller(); tap(KEY_A); /* ordinary Move */
    if (mode != MODE_MOVE) return 0;
    move_cursor(x, y);
    wait_for_controller(); tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_MOVE;
}

static void escort_army_center(int fallback_x, int fallback_y, int *x, int *y);

static int first_side_combat_unit(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type) &&
            sc64_game_unit_selectable(u[i].type)) return i;
    return -1;
}

static int escort_combat_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type != U_TASSADAR &&
            u[i].type != U_ZP_RAYNOR_VULTURE &&
            u[i].raw_type != SC64_P4_RAW_TASSADAR &&
            u[i].raw_type != SC64_P4_STORM_HIGH_TEMPLAR_RAW &&
            !sc64_race_is_worker(u[i].type))
            ++count;
    return count;
}

static int actor_in_escape_region(int slot)
{
    int left, top, right, bottom;
    return slot >= 0 && slot < MAX_UNITS && u[slot].alive &&
        race_p4_location(SC64_P4_LOCATION_ESCAPE, &left, &top, &right, &bottom) &&
        u[slot].x >= left && u[slot].x <= right &&
        u[slot].y >= top && u[slot].y <= bottom;
}

static int issue_hero_escape_segment(int slot, int force)
{
    if (slot < 0 || !u[slot].alive || u[slot].owner != SIDE ||
        actor_in_escape_region(slot)) return 0;
    if (!force && u[slot].order == ORDER_MOVE &&
        dist2(u[slot].x, u[slot].y, u[slot].tx, u[slot].ty) > 96 * 96)
        return 0;
    int left, top, right, bottom, tx, ty;
    if (!race_p4_location(SC64_P4_LOCATION_ESCAPE, &left, &top, &right, &bottom))
        return 0;
    tx = (left + right) / 2;
    ty = (top + bottom) / 2;
    if (!safe_escape_waypoint(u[slot].x, u[slot].y, tx, ty, &tx, &ty))
        return 0;
    if (tx == u[slot].x && ty == u[slot].y) { tx = (left + right) / 2; ty = (top + bottom) / 2; }
    if (!issue_hero_move(slot, tx, ty)) return 0;
    return 1;
}

/* Plan a conservative route segment for the rescue party. The mission's
 * Zerg guards are dangerous to the two source heroes, so each segment avoids
 * cells within 64 px of a currently hostile unit. This only chooses ordinary
 * attack-move cursor targets; the game still executes all movement/combat. */
static int safe_escape_waypoint(int start_x, int start_y, int goal_x, int goal_y,
                                int *out_x, int *out_y)
{
    static u16 queue[MAP_CELLS], parent[MAP_CELLS], reverse_path[MAP_CELLS];
    static u32 cost[MAP_CELLS], penalty[MAP_CELLS];
    static u8 blocked[MAP_CELLS], queued[MAP_CELLS];
    int start_c = start_x / 16, start_r = start_y / 16;
    int goal_c = goal_x / 16, goal_r = goal_y / 16;
    if (start_c < 0 || start_r < 0 || goal_c < 0 || goal_r < 0 ||
        start_c >= MAP_COLS || goal_c >= MAP_COLS ||
        start_r >= MAP_ROWS || goal_r >= MAP_ROWS) return 0;
    for (int row = 0; row < MAP_ROWS; ++row) for (int col = 0; col < MAP_COLS; ++col) {
        int cell = col + row * MAP_COLS;
        int x = col * 16 + 8, y = row * 16 + 8;
        /* Terrain remains authoritative for long-route connectivity; the
         * transient path_walkable grid also removes structures/units, which
         * can seal a temporary corridor without making terrain impassable. */
        blocked[cell] = (u8)terrain_blocked(col, row);
        u32 risk = 0;
        for (int i = 0; i < MAX_UNITS; ++i) {
            if (u[i].alive && u[i].owner == RESCUABLE &&
                (u[i].raw_type == SC64_P4_RAW_TASSADAR ||
                 u[i].raw_type == SC64_P4_RAW_RAYNOR_VULTURE) &&
                dist2(x, y, u[i].x, u[i].y) < 64 * 64)
                blocked[cell] = 1;
            if (!u[i].alive || !owners_hostile(SIDE, u[i].owner)) continue;
            int d2 = dist2(x, y, u[i].x, u[i].y);
            if (d2 < 96 * 96) risk += 32768;
            else if (d2 < 160 * 160) risk += 8192;
            else if (d2 < 240 * 240) risk += 2048;
        }
        penalty[cell] = risk;
        cost[cell] = 0xffffffffu;
        parent[cell] = 0xffffu;
        queued[cell] = 0;
    }
    int start = start_c + start_r * MAP_COLS;
    int goal = goal_c + goal_r * MAP_COLS;
    blocked[start] = blocked[goal] = 0;
    unsigned head = 0, tail = 0, count = 0;
    queue[tail] = (u16)start; tail = (tail + 1u) % MAP_CELLS; ++count;
    queued[start] = 1;
    cost[start] = 0;
    parent[start] = (u16)start;
    static const int dc[4] = {0, 1, 0, -1};
    static const int dr[4] = {-1, 0, 1, 0};
    while (count) {
        int cell = queue[head]; head = (head + 1u) % MAP_CELLS; --count;
        queued[cell] = 0;
        int col = cell % MAP_COLS, row = cell / MAP_COLS;
        for (int d = 0; d < 4; ++d) {
            int nc = col + dc[d], nr = row + dr[d];
            if (nc < 0 || nr < 0 || nc >= MAP_COLS || nr >= MAP_ROWS) continue;
            int next = nc + nr * MAP_COLS;
            if (blocked[next]) continue;
            u32 next_cost = cost[cell] + 16u + penalty[next];
            if (next_cost >= cost[next]) continue;
            cost[next] = next_cost;
            parent[next] = (u16)cell;
            if (!queued[next]) {
                queue[tail] = (u16)next; tail = (tail + 1u) % MAP_CELLS;
                ++count; queued[next] = 1;
            }
        }
    }
    if (parent[goal] == 0xffffu) return 0;
    unsigned n = 0;
    for (int cell = goal; cell != start && n < MAP_CELLS; cell = parent[cell])
        reverse_path[n++] = (u16)cell;
    if (!n) { *out_x = goal_x; *out_y = goal_y; return 1; }
    /* Advance about 320 px at a time so movement uses short, threat-aware
     * segments instead of the engine's shorter but guard-exposed direct route. */
    unsigned pick = n > 20u ? n - 20u : 0u;
    int cell = reverse_path[pick];
    *out_x = (cell % MAP_COLS) * 16 + 8;
    *out_y = (cell / MAP_COLS) * 16 + 8;
    if (n <= 20u) { *out_x = goal_x; *out_y = goal_y; }
    return 1;
}

static void escort_army_center(int fallback_x, int fallback_y, int *x, int *y)
{
    int xs[MAX_UNITS], ys[MAX_UNITS], count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || u[i].type == U_TASSADAR ||
            u[i].type == U_ZP_RAYNOR_VULTURE || sc64_race_is_worker(u[i].type)) continue;
        xs[count] = u[i].x; ys[count] = u[i].y; ++count;
    }
    if (count) {
        for (int i = 1; i < count; ++i) {
            int xx = xs[i], yy = ys[i], j = i;
            while (j && xs[j - 1] > xx) { xs[j] = xs[j - 1]; --j; }
            xs[j] = xx;
            j = i;
            while (j && ys[j - 1] > yy) { ys[j] = ys[j - 1]; --j; }
            ys[j] = yy;
        }
    }
    *x = count ? xs[count / 2] : fallback_x;
    *y = count ? ys[count / 2] : fallback_y;
}

static int issue_group_order(int x, int y, int army_only, int attack_move)
{
    /* R is the ordinary initial combat-group shortcut. Once the two heroes
     * have been moved clear, select the escorts with an ordinary drag box so
     * later orders do not pull them back into the battle. */
    wait_for_controller();
    if (army_only) {
        /* P4's authored B+R chord selects the combat group without the two
         * objective heroes, even when they are standing among the escorts. */
        step(KEY_B | KEY_R);
        step(0);
    } else {
        /* The optional production attempt can leave a friendly building
         * selected; clear it with the normal UI before invoking R. */
        if (selected_building >= 0) select_box(0, 0, 8);
        step(KEY_R);
        step(0);
    }
    if (!selected_count()) return 0;
    wait_for_controller();
    tap(KEY_B);
    wait_for_controller();
    if (attack_move) { tap(KEY_DOWN); wait_for_controller(); }
    tap(KEY_A); /* Move is first; Attack Move is the second field choice. */
    if (mode != (attack_move ? MODE_ATTACK : MODE_MOVE) || menu_open) return 0;
    move_cursor(x, y);
    wait_for_controller();
    u8 commanded[MAX_UNITS];
    memset(commanded, 0, sizeof(commanded));
    int ncommanded = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) &&
            u[i].owner == SIDE && u[i].selected
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            && (!sc64_game_unit_actor_flags(u[i].type) ||
                (sc64_game_unit_actor_flags(u[i].type) & SC64_GAME_ACTOR_MOVABLE))
#endif
            ) {
            commanded[i] = 1;
            ++ncommanded;
        }
    tap(KEY_A);
    int applied = ncommanded > 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        /* Formation placement offsets each selected unit's destination. A
         * unit already at its assigned cell may consume the order immediately
         * and return to idle in this same update. */
        int at_issued_destination = u[i].alive && u[i].order == ORDER_IDLE &&
            dist2(u[i].x, u[i].y, u[i].tx, u[i].ty) <= 8 * 8;
        if (commanded[i] && (!u[i].alive ||
            (u[i].order != (attack_move ? ORDER_ATTACK_MOVE : ORDER_MOVE) &&
             !at_issued_destination))) {
            applied = 0;
        }
    }
    return applied;
}

static void launch_from_normal_menu(void)
{
    assert(sc64_catalog_is_playable(SC64_P4_GLOBAL_ID));
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_PROTOSS, SC64_P4_GLOBAL_ID) == 3);
    game_init(); prior_keys = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_PROTOSS);
    tap(KEY_RIGHT); tap(KEY_DOWN);
    assert(race_front.mission_page == 1 && race_front.row == 1);
    assert(sc64_catalog_playable_at(SC64_FACTION_PROTOSS, 3) == SC64_P4_GLOBAL_ID);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING &&
           race_front.mission == SC64_P4_GLOBAL_ID);
    step(KEY_A);
    deployed = 1;
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == SC64_P4_GLOBAL_ID && race_p4_active());
    assert(race_campaign.mission == SC64_P4_GLOBAL_ID);
    assert(alive_units() == 137 && source_unit(SC64_P4_RAW_TASSADAR) >= 0);
    assert(source_unit(SC64_P4_RAW_RAYNOR_VULTURE) < 0);
}

static int trace_path_exists(const char *path)
{
    FILE *existing = fopen(path, "rb");
    if (!existing) return 0;
    fclose(existing);
    return 1;
}

static void write_trace_header(void)
{
    trace = fopen(attempt_path, "wx");
    assert(trace);
    fprintf(trace, "{\"schema\":1,\"mission\":23,\"profile\":\"protoss-p4\","
                   "\"policy\":\"protoss-stock-gas-cannons-dragoon-escort-extract\","
                   "\"path_diagnostics\":false,\"events\":[\n");
}

static void promote_verified_trace(void)
{
    assert(screen == SCREEN_RESULT && won && !lost && race_selected_mission == 23 &&
           race_p4.model.outcome == SC64_P4_WON &&
           (race_completed & (1 << 23)) != 0);
    if (trace_path_exists(verified_path)) {
        fprintf(stderr, "refusing to overwrite existing P4 proof %s\n", verified_path);
        assert(0);
    }
    if (rename(attempt_path, verified_path) != 0) {
        FILE *src = fopen(attempt_path, "rb"), *dst = fopen(verified_path, "wbx");
        assert(src && dst);
        char buffer[4096];
        size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), src)) != 0)
            assert(fwrite(buffer, 1, n, dst) == n);
        assert(!ferror(src));
        fclose(src); fclose(dst);
        remove(attempt_path);
    }
}

int main(int argc, char **argv)
{
    const char *attempt_tag = NULL;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--limit") && i + 1 < argc) run_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--attempt-tag") && i + 1 < argc) attempt_tag = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_p4_campaign_run [--limit TICKS] [--attempt-tag TAG] ");
            return 0;
        } else return 2;
    }
    if (run_limit < 1 || run_limit > 1000000) return 2;
    char unique_tag[112];
    if (attempt_tag) {
        for (const char *p = attempt_tag; *p; ++p)
            if (!( (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                   (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' )) return 2;
    } else attempt_tag = "p4-auto";
    int tag_len = snprintf(unique_tag, sizeof(unique_tag), "%s-%ld",
                           attempt_tag, (long)getpid());
    if (tag_len < 0 || (size_t)tag_len >= sizeof(unique_tag) ||
        snprintf(attempt_path, sizeof(attempt_path),
                 ".local/sc64/p4-controller-%s-running.json", unique_tag) >=
            (int)sizeof(attempt_path) ||
        snprintf(final_attempt_path, sizeof(final_attempt_path),
                 ".local/sc64/p4-controller-%s.json", unique_tag) >=
            (int)sizeof(final_attempt_path) ||
        snprintf(verified_path, sizeof(verified_path),
                 ".local/sc64/p4-controller-%s-win.json", unique_tag) >=
            (int)sizeof(verified_path)) return 2;
    if (trace_path_exists(attempt_path) || trace_path_exists(final_attempt_path) ||
        trace_path_exists(verified_path)) return 2;
    mkdir(".local", 0777); mkdir(".local/sc64", 0777);
    write_trace_header();
    gba_init();
    launch_from_normal_menu();
    checkpoint("start");
    int l, t, r, bottom;
    assert(race_p4_location(SC64_P4_LOCATION_ESCAPE, &l, &t, &r, &bottom));
    int escape_x = (l + r) / 2, escape_y = (t + bottom) / 2;
    int scheduled_order = -1;
    int pylon_builds_started = 0;
    int pylon_build_attempts = 0;
    int pylon_next_build_frame = 0;
    int gateway_builds_started = 0;
    int gateway_next_build_frame = 0;
    int core_build_attempts = 0;
    int core_next_build_frame = 0;
    int cannon_builds_started = 0;
    int cannon_next_build_frame = 0;
    int next_storm_attempt = 0;
    int next_templar_follow = 0;
    int next_solo_hero_order = 0;
    int next_probe_queue_frame = 0;
    int heroes_final_advance = 0;
    while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost) {
        int tassadar = source_unit(SC64_P4_RAW_TASSADAR);
        int raynor = source_unit(SC64_P4_RAW_RAYNOR_VULTURE);
        int high_templar = source_unit(SC64_P4_STORM_HIGH_TEMPLAR_RAW);
        int high_templar_rescued = high_templar >= 0 && u[high_templar].owner == SIDE;
        int tassadar_rescued = tassadar >= 0 && u[tassadar].owner == SIDE;
        int raynor_rescued = raynor >= 0 && u[raynor].owner == SIDE;
        int rescued = tassadar_rescued && raynor_rescued;
        if (frame >= next_storm_attempt && !race_p4.source_paused) {
            (void)try_cast_storm();
            next_storm_attempt = frame + 90;
        }
        if (rescued && !rescued_logged) {
            checkpoint("both-rescued");
            rescued_logged = 1;
        }
        if (!escape_cursor_ready && tassadar >= 0 && !tassadar_rescued) {
            int nearest = 0x7fffffff;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type)) {
                    int d = dist2(u[i].x, u[i].y, u[tassadar].x, u[tassadar].y);
                    if (d < nearest) nearest = d;
                }
            if (nearest < 700 * 700) {
                /* Move the cursor to extraction while the combat group closes
                 * on Tassadar, so the first escort command has no long UI delay. */
                move_cursor(escape_x, escape_y);
                escape_cursor_ready = 1;
                /* Cursor movement advances the simulation too. Refresh the
                 * rescue predicates before choosing the next controller action. */
                tassadar = source_unit(SC64_P4_RAW_TASSADAR);
                raynor = source_unit(SC64_P4_RAW_RAYNOR_VULTURE);
                tassadar_rescued = tassadar >= 0 && u[tassadar].owner == SIDE;
                raynor_rescued = raynor >= 0 && u[raynor].owner == SIDE;
                rescued = tassadar_rescued && raynor_rescued;
                if (rescued && !rescued_logged) {
                    checkpoint("both-rescued");
                    rescued_logged = 1;
                }
            }
        }
        if (frame >= scheduled_order) {
            int issued = 0;
            int rescued_probes = 0, rescuable_probe = -1;
            for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
                u[i].raw_owner == 1 && u[i].raw_type == 64) {
                if (u[i].owner == SIDE) ++rescued_probes;
                else if (rescuable_probe < 0) rescuable_probe = i;
            }
            if (rescued_probes >= 4) controller_probes_secured = 1;
            int nexus = source_building(154, SIDE);
            int gateway = source_building(160, SIDE);
            if (!tassadar_rescued && tassadar >= 0) {
                /* Reach the source hero itself before beginning the optional
                 * base recovery: the Nexus lies close by, but a formation
                 * target on its edge does not satisfy the hero contact radius. */
                issued = issue_group_order(u[tassadar].x, u[tassadar].y, 0, 1);
            } else if (!high_templar_rescued && high_templar >= 0 &&
                       u[high_templar].owner == RESCUABLE) {
                /* Rescue the source-owned High Templar so the ordinary menu
                 * can provide researched Storm against the scripted waves. */
                issued = issue_group_order(u[high_templar].x, u[high_templar].y, 0, 1);
            } else if (nexus < 0 || gateway < 0) {
                int target = nexus < 0 ? source_building(154, 3) :
                                         source_building(160, 3);
                if (target >= 0) {
                    int army_x, army_y, tx = b[target].x,
                        ty = b[target].y;
                    /* Approach the Nexus from its north edge, inside the
                     * ordinary building rescue radius but clear of the two
                     * heroes standing south of it. */
                    if (b[target].raw_type == 154) ty -= 24;
                    escort_army_center(tx, ty, &army_x, &army_y);
                    (void)safe_escape_waypoint(army_x, army_y, tx, ty, &tx, &ty);
                    issued = issue_group_order(tx, ty, 0, 1);
                }
            } else if (rescued_probes < 4 && rescuable_probe >= 0) {
                int army_x, army_y, tx = u[rescuable_probe].x,
                    ty = u[rescuable_probe].y;
                escort_army_center(tx, ty, &army_x, &army_y);
                (void)safe_escape_waypoint(army_x, army_y, tx, ty, &tx, &ty);
                issued = issue_group_order(tx, ty, 0, 1);
            } else if (!controller_buildup_done && controller_probes_secured) {
                /* Keep the objective heroes inside the authored cannon line
                 * while the rescued base mines and builds a real escort. */
                if (!controller_buildup_started) {
                    controller_buildup_started = 1;
                    controller_buildup_start_frame = frame;
                }
                int heroes[3] = {tassadar, raynor, high_templar};
                for (int h = 0; h < 3; ++h) {
                    int slot = heroes[h];
                    if (slot < 0 || !u[slot].alive || u[slot].owner != SIDE) continue;
                    if (dist2(u[slot].x, u[slot].y, P4_PARK_X, P4_PARK_Y) > 48 * 48 &&
                        (u[slot].order != ORDER_MOVE ||
                         dist2(u[slot].tx, u[slot].ty, P4_PARK_X, P4_PARK_Y) > 24 * 24)) {
                        if (issue_hero_move(slot, P4_PARK_X, P4_PARK_Y)) {
                                    issued = 1;
                            break; /* one menu command per controller interval */
                        }
                    }
                }
                if (nexus < 0 || gateway < 0) {
                    controller_buildup_done = 1;
                }
                int combat = escort_combat_count();
                int slack = source_pool_slack();
                int elapsed = frame - controller_buildup_start_frame;
                if (side_probe_count() == 0) {
                    controller_buildup_done = 1;
                }
                if (slack <= 0 && side_queued_units() == 0) {
                    if (controller_buildup_pool_blocked_since < 0)
                        controller_buildup_pool_blocked_since = frame;
                } else controller_buildup_pool_blocked_since = -1;
                if (!controller_buildup_done && (combat >= 44 || elapsed >= 60000 ||
                    (controller_buildup_pool_blocked_since >= 0 &&
                     frame - controller_buildup_pool_blocked_since >= 2700))) {
                    controller_buildup_done = 1;
                } else if (!controller_buildup_done && side_gas_worker_count() < 2 &&
                           source_building(157, SIDE) >= 0) {
                    issued = assign_probe_to_gas();
                } else if (!controller_buildup_done && side_probe_count() < 8 &&
                           frame >= next_probe_queue_frame && minerals >= 50 &&
                           source_pool_slack() > 0 &&
                           race_owner_reserved_supply(SIDE) + 2 <=
                               race_owner_supply_cap(SIDE)) {
                    issued = queue_probe_at_nexus();
                    next_probe_queue_frame = frame + (issued ? 1800 : 600);
                } else if (!controller_buildup_done && minerals >= 100 && supply_cap < supply_used + 4 &&
                    pylon_builds_started < 12 && pylon_build_attempts < 24 &&
                           side_incomplete_pylon_count() == 0 &&
                           frame >= pylon_next_build_frame) {
                    ++pylon_build_attempts;
                    int built = build_pylon_from_probe();
                    if (built) {
                        ++pylon_builds_started;
                        pylon_next_build_frame = frame + 700;
                        issued = 1;
                    } else pylon_next_build_frame = frame + 300;
                } else if (!controller_buildup_done && cannon_builds_started < 3 &&
                           minerals >= 150 && frame >= cannon_next_build_frame &&
                           source_building(166, SIDE) >= 0) {
                    if (build_cannon_from_probe(cannon_builds_started)) {
                        ++cannon_builds_started;
                        cannon_next_build_frame = frame + 600;
                        issued = 1;
                    } else cannon_next_build_frame = frame + 1200;
                } else if (!controller_buildup_done && side_gateway_count() >= 2 &&
                           side_building_type(B_CYBERNETICS_CORE, 0) < 0 && core_build_attempts < 8 &&
                           minerals >= 200 && frame >= core_next_build_frame) {
                    ++core_build_attempts;
                    if (build_cybernetics_core_from_probe()) issued = 1;
                    core_next_build_frame = frame + (issued ? 600 : 1200);
                } else if (!controller_buildup_done && gateway_builds_started < 2 &&
                           minerals >= 150 && frame >= gateway_next_build_frame) {
                    int built = build_gateway_from_probe(gateway_builds_started);
                    if (built) {
                        ++gateway_builds_started;
                        gateway_next_build_frame = frame + 600;
                        issued = 1;
                    } else gateway_next_build_frame = frame + 2400;
                } else if (!controller_buildup_done && combat < 44 && minerals >= 100 && slack > 0 &&
                           supply_used + 4 <= supply_cap && gateway >= 0) {
                    for (int i = 0; i < MAX_BUILDINGS; ++i)
                        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_GATEWAY &&
                            b[i].queue_count < 5 && building_complete(i) &&
                            race_site_powered(SIDE, b[i].x, b[i].y)) {
                            gateway = i;
                            break;
                        }
                    issued = queue_zealot_at_gateway(gateway);
                }
            } else if (!tassadar_rescued && tassadar >= 0) {
                issued = issue_group_order(u[tassadar].x, u[tassadar].y, 0, 1);
            } else if (!raynor_rescued && raynor >= 0) {
                issued = issue_group_order(u[raynor].x, u[raynor].y, 0, 1);
            } else if (rescued && !heroes_released) {
                if (escort_combat_count() <= 1) {
                    /* Once every escort has fallen, advance only the two
                     * required heroes with ordinary Move orders. */
                    int tx = escape_x, ty = escape_y;
                    int hx = tassadar >= 0 ? u[tassadar].x : u[raynor].x;
                    int hy = tassadar >= 0 ? u[tassadar].y : u[raynor].y;
                    if (!safe_escape_waypoint(hx, hy, escape_x, escape_y, &tx, &ty)) {
                        tx = escape_x; ty = escape_y;
                    }
                    int moved_t = tassadar >= 0 &&
                        (u[tassadar].order != ORDER_MOVE ||
                         dist2(u[tassadar].tx, u[tassadar].ty, tx, ty) > 32 * 32) &&
                        issue_hero_move(tassadar, tx, ty);
                    raynor = source_unit(SC64_P4_RAW_RAYNOR_VULTURE);
                    int moved_r = raynor >= 0 &&
                        (u[raynor].order != ORDER_MOVE ||
                         dist2(u[raynor].tx, u[raynor].ty, tx, ty) > 32 * 32) &&
                        issue_hero_move(raynor, tx, ty);
                    issued = moved_t || moved_r;
                    if (issued) heroes_released = 1;
                } else {
                /* Refresh the escort's formation/order periodically so units
                 * that split during the scripted pause regroup on the route. */
                int hx = tassadar >= 0 ? u[tassadar].x : u[raynor].x;
                int hy = tassadar >= 0 ? u[tassadar].y : u[raynor].y;
                int army_x, army_y;
                escort_army_center(hx, hy, &army_x, &army_y);
                int tx = escape_x, ty = escape_y;
                (void)safe_escape_waypoint(army_x, army_y, escape_x, escape_y,
                                           &tx, &ty);
                /* Reuse the ordinary combat-group shortcut for every bulk
                 * waypoint. A drag box can include a hero stacked beside a
                 * rescued escort and was being rejected as an unsafe group. */
                int army_only = 1;
                issued = issue_group_order(tx, ty, army_only, 0);
                if (issued) escape_group_ordered = 1;
                }
            } else if (rescued && heroes_released && escort_combat_count() >= 2) {
                int army_x, army_y;
                escort_army_center(escape_x, escape_y, &army_x, &army_y);
                if (dist2(army_x, army_y, escape_x, escape_y) <= 360 * 360 &&
                    frame >= next_solo_hero_order) {
                    int force = !heroes_final_advance;
                    heroes_final_advance = 1;
                    int moved_t = issue_hero_escape_segment(tassadar, force);
                    raynor = source_unit(SC64_P4_RAW_RAYNOR_VULTURE);
                    int moved_r = issue_hero_escape_segment(raynor, force);
                    issued = moved_t || moved_r;
                    next_solo_hero_order = frame + 1200;
                } else if (dist2(army_x, army_y, escape_x, escape_y) > 200 * 200) {
                    int tx = escape_x, ty = escape_y;
                    (void)safe_escape_waypoint(army_x, army_y, escape_x, escape_y,
                                               &tx, &ty);
                    issued = issue_group_order(tx, ty, 1, 0);
                }
            } else if (rescued && heroes_released && escort_combat_count() <= 1) {
                /* If the escort is gone, preserve the source objective with
                 * direct ordinary Move orders for the two required heroes. */
                if (frame >= next_solo_hero_order) {
                    int heroes[2] = {tassadar, raynor};
                    for (int h = 0; h < 2; ++h) {
                        int slot = heroes[h];
                        if (slot < 0 || !u[slot].alive || u[slot].owner != SIDE ||
                            actor_in_escape_region(slot))
                            continue;
                        int tx = escape_x, ty = escape_y;
                        if (!safe_escape_waypoint(u[slot].x, u[slot].y,
                                                  escape_x, escape_y, &tx, &ty)) {
                            tx = escape_x; ty = escape_y;
                        }
                        if (u[slot].order != ORDER_MOVE ||
                            dist2(u[slot].tx, u[slot].ty, tx, ty) > 32 * 32)
                            (void)issue_hero_move(slot, tx, ty);
                    }
                    next_solo_hero_order = frame + 1200;
                }
            }
            if (rescued && !heroes_released && controller_buildup_done) {
                int army_x, army_y;
                escort_army_center(escape_x, escape_y, &army_x, &army_y);
                if (dist2(army_x, army_y, P4_PARK_X, P4_PARK_Y) > 256 * 256) {
                    heroes_released = 1;
                }
            }
            high_templar = source_unit(SC64_P4_STORM_HIGH_TEMPLAR_RAW);
            if (rescued && controller_buildup_done && high_templar >= 0 &&
                u[high_templar].owner == SIDE && escort_combat_count() > 0 &&
                frame >= next_templar_follow) {
                int army_x, army_y;
                escort_army_center(escape_x, escape_y, &army_x, &army_y);
                if (dist2(u[high_templar].x, u[high_templar].y,
                          army_x, army_y) > 128 * 128)
                    (void)move_rescue_hero_to_safety(high_templar, army_x, army_y);
                next_templar_follow = frame + 1800;
            }
            scheduled_order = frame + (rescued ?
                (escort_combat_count() ? 1200 : 300) : 2400);
            if (!issued) scheduled_order = frame + 300;
        }
        step(0);
    }
    checkpoint(won ? "won" : lost ? "lost" : "limit");
    fprintf(trace, "]}\n");
    fclose(trace); trace = NULL;
    if (won && screen == SCREEN_RESULT && race_p4.model.outcome == SC64_P4_WON &&
        race_selected_mission == 23 && (race_completed & (1 << 23)))
        promote_verified_trace();
    else
        assert(rename(attempt_path, final_attempt_path) == 0);
    printf("P4 controller attempt result=%s frame=%u fired=0x%x created=%u outcome=%u side=%d trace=%s\n",
           won ? "WON" : lost ? "LOST" : "LIMIT", (unsigned)frame,
           race_p4.model.fired, race_p4.created_count, race_p4.model.outcome,
           owner_units(SIDE), won ? verified_path : final_attempt_path);
    return won && screen == SCREEN_RESULT && race_p4.model.outcome == SC64_P4_WON &&
           race_selected_mission == 23 && (race_completed & (1 << 23)) ? 0 : 1;
}
