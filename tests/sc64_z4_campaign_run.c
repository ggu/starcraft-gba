/* Normal-controller Z4 campaign attempt. All game changes are produced by
 * ordinary game_update input. Runtime reads only guide choices and checkpoints. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define RUN_LIMIT_DEFAULT 150000
#define CHECKPOINT_PERIOD 900

static FILE *trace;
static u16 prior_keys;
static u32 host_ticks;
static int last_logged_keys = -1;
static int trace_first = 1;
static const char *trace_path;
static int run_limit = RUN_LIMIT_DEFAULT;
static int controller_input_error;
static int controller_limit_reached;
static int deployed;
static int last_order_frame = -1;
static int last_production_frame = -1;
static int last_tech_frame = -1;
static int last_stage = -1;

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

static int total_units(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) n += !!u[i].alive;
    return n;
}

static int total_buildings(void)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) n += !!b[i].alive;
    return n;
}

static int side_workers(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && sc64_race_is_worker(u[i].type)) ++n;
    return n;
}

static int side_army(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type) &&
            u[i].type != U_OVERLORD && u[i].type != U_CHRYSALIS &&
            sc64_game_unit_selectable(u[i].type)) ++n;
    return n;
}

static int side_unit_type_count(int type)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == type) ++n;
    return n;
}

static void mutalisk_metrics(int *count, int *hp, int *x, int *y)
{
    int n = 0, total_hp = 0, sum_x = 0, sum_y = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_MUTALISK) {
            ++n; total_hp += u[i].hp; sum_x += u[i].x; sum_y += u[i].y;
        }
    if (count) *count = n;
    if (hp) *hp = total_hp;
    if (x) *x = n ? sum_x / n : -1;
    if (y) *y = n ? sum_y / n : -1;
}

static int side_building_type(int type, int complete_only)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == type &&
            (!complete_only || building_complete(i))) return i;
    return -1;
}

static int side_gas_workers(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && sc64_race_is_worker(u[i].type) &&
            u[i].order == ORDER_GAS) ++n;
    return n;
}

static int side_raw_unit(int raw, int raw_owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == raw && u[i].raw_owner == raw_owner &&
            u[i].owner == SIDE) return i;
    return -1;
}

static int force3_command_centers(void)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == 4 && b[i].raw_type == 106 &&
            b[i].raw_owner == 2) ++n;
    return n;
}

static int closest_force3_command_center(int x, int y)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == 4 && b[i].raw_type == 106 &&
            b[i].raw_owner == 2) {
            int d = dist2(x, y, b[i].x, b[i].y);
            if (d < best_d) { best = i; best_d = d; }
        }
    return best;
}

static int chrysalis_position(int *x, int *y)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].raw_type == 150 &&
            b[i].raw_owner == 3) {
            *x = b[i].x; *y = b[i].y; return i;
        }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 150 &&
            u[i].raw_owner == 3) {
            *x = u[i].x; *y = u[i].y; return i;
        }
    return -1;
}

static void checkpoint(const char *kind)
{
    int chrysalis_x = -1, chrysalis_y = -1;
    int chrysalis_alive = chrysalis_position(&chrysalis_x, &chrysalis_y) >= 0;
    int kerrigan = side_raw_unit(51, 3);
    int muta_count, muta_hp, muta_x, muta_y;
    mutalisk_metrics(&muta_count, &muta_hp, &muta_x, &muta_y);
    trace_prefix();
    fprintf(trace,
        "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%d,\"elapsed_frames\":%u,"
        "\"screen\":%d,\"won\":%d,\"lost\":%d,\"outcome\":%u,\"fired\":%u,"
        "\"countdown_active\":%u,\"countdown_ticks\":%u,\"hatch_effects\":%u,"
        "\"side_units\":%d,\"side_workers\":%d,\"side_army\":%d,\"side_buildings\":%d,"
        "\"side_mutalisks\":%d,\"mutalisk_hp\":%d,\"mutalisk_x\":%d,\"mutalisk_y\":%d,"
        "\"side_spire_complete\":%d,\"side_lair_complete\":%d,\"gas_workers\":%d,"
        "\"force2_units\":%d,\"force2_buildings\":%d,\"force3_units\":%d,\"force3_buildings\":%d,"
        "\"force3_command_centers\":%d,\"minerals\":%d,\"gas\":%d,\"supply\":%d,\"supply_cap\":%d,"
        "\"chrysalis_alive\":%d,\"chrysalis_x\":%d,\"chrysalis_y\":%d,\"kerrigan_alive\":%d,"
        "\"cursor_x\":%d,\"cursor_y\":%d,\"selected_units\":%d,\"selected_building\":%d,"
        "\"unsupported\":%d}\n",
        kind, (unsigned)host_ticks, frame, (unsigned)race_campaign.elapsed_frames,
        screen, won, lost, race_z4.model.outcome, race_z4.model.fired,
        race_z4.countdown_active, race_z4.countdown_ticks, race_z4.hatch_effects,
        owner_units(SIDE), side_workers(), side_army(), owner_buildings(SIDE),
        muta_count, muta_hp, muta_x, muta_y, side_building_type(B_SPIRE, 1) >= 0,
        side_building_type(B_LAIR, 1) >= 0, side_gas_workers(),
        owner_units(ENEMY), owner_buildings(ENEMY), owner_units(4), owner_buildings(4),
        force3_command_centers(), minerals, gas, supply_used, supply_cap,
        chrysalis_alive, chrysalis_x, chrysalis_y, kerrigan >= 0,
        cx, cy, selected_count(), selected_building, mission_unsupported_count);
    fflush(trace);
}

static void step(u16 held)
{
    if (deployed && (screen != SCREEN_PLAY || won || lost)) return;
    /* The UI helpers below may run several updates without advancing the
     * campaign clock (for example, while a command menu is open). Keep both
     * clocks bounded so a missed menu action cannot spin forever. */
    if (deployed && (frame >= run_limit || host_ticks >= (u32)run_limit + 10000u)) {
        controller_limit_reached = 1;
        return;
    }
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
    if (trace && deployed && screen == SCREEN_PLAY && frame > 0 &&
        frame % CHECKPOINT_PERIOD == 0) checkpoint("periodic");
}

static void tap(u16 key)
{
    step(key);
    step(0);
}

/* Field controls reopen the order list on the last command. These
 * sequences were written for a list that opens on Move, so a fresh
 * open starts there (no extra input, so the run's timing is unchanged). */
static void tap_b(void)
{
    int fresh = !menu_open && mode == MODE_NORMAL;
    tap(KEY_B);
    if (fresh && menu_open) menu_cursor = MENU_MOVE;
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
    int stalled = 0;
    while ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost &&
           !controller_input_error && !controller_limit_reached) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT;
        if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        if (cy > y) held |= KEY_UP;
        int old_x = cx, old_y = cy;
        step(held);
        if (cx == old_x && cy == old_y) {
            if (!controller_limit_reached && ++stalled >= 120) {
                fprintf(stderr, "Z4 cursor unreachable: from=%d,%d target=%d,%d frame=%d menu=%d mode=%d\n",
                        old_x, old_y, x, y, frame, menu_open, mode);
                controller_input_error = 1;
            }
        } else stalled = 0;
    }
    if (!controller_input_error && !controller_limit_reached) step(0);
    if ((cx != x || cy != y) && !won && !lost && !controller_limit_reached)
        controller_input_error = 1;
}

static void move_cursor_held(int x, int y, u16 held)
{
    x = snap4(cx, x, WORLD_W);
    y = snap4(cy, y, WORLD_H);
    int stalled = 0;
    while ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost &&
           !controller_input_error && !controller_limit_reached) {
        u16 keys = held;
        if (cx < x) keys |= KEY_RIGHT;
        if (cx > x) keys |= KEY_LEFT;
        if (cy < y) keys |= KEY_DOWN;
        if (cy > y) keys |= KEY_UP;
        int old_x = cx, old_y = cy;
        step(keys);
        if (cx == old_x && cy == old_y) {
            if (!controller_limit_reached && ++stalled >= 120) {
                fprintf(stderr, "Z4 box cursor unreachable: from=%d,%d target=%d,%d frame=%d menu=%d mode=%d\n",
                        old_x, old_y, x, y, frame, menu_open, mode);
                controller_input_error = 1;
            }
        } else stalled = 0;
    }
    if ((cx != x || cy != y) && !won && !lost && !controller_limit_reached)
        controller_input_error = 1;
}

static void select_box(int x, int y, int radius)
{
    move_cursor(x - radius, y - radius);
    step(KEY_L);
    move_cursor_held(x + radius, y + radius, KEY_L | KEY_R);
    step(0);
}

static void select_army(void)
{
    /* R selects the ordinary combat group. A corner box first clears a
     * selected producer so R cannot be interpreted as train-type cycling. */
    if (selected_building >= 0) select_box(0, 0, 8);
    tap(KEY_R);
}

static void issue_attack_move(int x, int y)
{
    select_army();
    if (!selected_count()) return;
    tap_b();
    tap(KEY_DOWN); /* ATTACK */
    tap(KEY_A);
    move_cursor(x, y);
    tap(KEY_A);
}

static void launch_from_normal_menu(void)
{
    assert(sc64_catalog_is_playable(15));
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_ZERG, 15) == 3);
    game_init();
    prior_keys = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_ZERG);
    tap(KEY_RIGHT);
    tap(KEY_DOWN);
    assert(race_front.mission_page == 1 && race_front.row == 1);
    assert(sc64_catalog_playable_at(SC64_FACTION_ZERG, 3) == 15);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 15);
    step(KEY_START);
    deployed = 1;
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == 15 && race_campaign.mission == 15);
    assert(race_human == R_ZERG && race_z4_active());
    assert(mission_unsupported_count == 0);
    assert(total_units() == 73 && total_buildings() == 67);
    assert(owner_units(SIDE) > 0 && owner_buildings(SIDE) > 0);
    assert(owner_units(ENEMY) > 0 && owner_buildings(4) > 0);
    assert(force3_command_centers() > 0);
    int cx_pos, cy_pos;
    assert(chrysalis_position(&cx_pos, &cy_pos) >= 0);
    fprintf(stderr, "Z4 launch: side=%d workers=%d army=%d buildings=%d force2=%d/%d force3=%d/%d CC3=%d resources=%d/%d\n",
            owner_units(SIDE), side_workers(), side_army(), owner_buildings(SIDE),
            owner_units(ENEMY), owner_buildings(ENEMY), owner_units(4), owner_buildings(4),
            force3_command_centers(), minerals, gas);
}

static int side_hatchery(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && !b[i].under_construction &&
            sc64_race_is_zerg_townhall(b[i].type)) return i;
    return -1;
}

static int queued_count(int type)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE)
            for (int j = 0; j < b[i].queue_count; ++j)
                if (b[i].queue[(b[i].queue_head + j) % 5] == type) ++n;
    return n;
}

static int train_type_step_count(int current, int wanted)
{
    static const int types[5] = {U_DRONE, U_OVERLORD, U_ZERGLING, U_HYDRALISK, U_MUTALISK};
    int from = -1, to = -1;
    for (int i = 0; i < 5; ++i) {
        if (types[i] == current) from = i;
        if (types[i] == wanted) to = i;
    }
    return from < 0 || to < 0 ? -1 : (to - from + 5) % 5;
}

static int queue_unit_type(int wanted)
{
    int before = queued_count(wanted);
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive || b[i].owner != SIDE || b[i].under_construction ||
            !sc64_race_is_zerg_townhall(b[i].type) || b[i].queue_count >= 4) continue;
        int cycle = train_type_step_count(train_type, wanted);
        if (cycle < 0) return 0;
        move_cursor(b[i].x, b[i].y);
        tap(KEY_A); /* Select the actual Hatchery/Lair/Hive. */
        if (selected_building != i) continue;
        for (int n = 0; n < cycle; ++n) tap(KEY_R);
        tap(KEY_A); /* Queue the selected unit through the normal producer. */
        if (queued_count(wanted) > before) return 1;
    }
    return 0;
}

static void maintain_economy(void)
{
    int free_supply = race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE);
    /* The Lair command requires an empty Hatchery queue. Stop feeding the only
     * producer once the Pool is complete so its existing queue can drain. */
    if (side_building_type(B_SPAWNING_POOL, 1) >= 0 &&
        side_building_type(B_LAIR, 0) < 0) return;
    if (free_supply <= 2 && minerals >= 100) {
        (void)queue_unit_type(U_OVERLORD);
        return;
    }
    if (side_building_type(B_SPIRE, 1) >= 0 && side_unit_type_count(U_MUTALISK) < 20 &&
        minerals >= 100 && gas >= 100) {
        (void)queue_unit_type(U_MUTALISK);
        return;
    }
    if (side_building_type(B_SPIRE, 1) >= 0 && side_unit_type_count(U_MUTALISK) < 20)
        return;
    if (minerals >= 50 && side_workers() < 18) {
        (void)queue_unit_type(U_DRONE);
        return;
    }
    if (minerals >= 50) (void)queue_unit_type(U_ZERGLING);
}

static int find_build_site_near(int type, int nearx, int neary,
                                int *site_x, int *site_y)
{
    int best = 0x7fffffff, found = 0;
    for (int dy = -176; dy <= 176; dy += 16)
        for (int dx = -176; dx <= 176; dx += 16) {
            int x = nearx + dx, y = neary + dy;
            int d = dist2(nearx, neary, x, y);
            if (d < 80 * 80 || d > 176 * 176 ||
                x < 32 || x >= WORLD_W - 32 || y < 32 || y >= WORLD_H - 32 ||
                d >= best || !race_can_site(SIDE, type, x, y) || !can_place(type, x, y))
                continue;
            best = d; *site_x = x; *site_y = y; found = 1;
        }
    return found;
}

static int select_worker_slot(int worker)
{
    if (worker < 0 || worker >= MAX_UNITS || !u[worker].alive || u[worker].owner != SIDE ||
        !sc64_race_is_worker(u[worker].type)) return 0;
    /* Walk the ordinary cursor toward the live worker position. Drones can
     * keep moving while a fixed-coordinate cursor trip is in progress, so
     * refresh the target each update and click once inside the select radius. */
    int steps = 0;
    while (u[worker].alive && dist2(cx, cy, u[worker].x, u[worker].y) > 20 * 20 &&
           screen == SCREEN_PLAY && !won && !lost && !controller_input_error &&
           !controller_limit_reached && steps++ < 120) {
        u16 held = KEY_R;
        if (cx < u[worker].x) held |= KEY_RIGHT;
        if (cx > u[worker].x) held |= KEY_LEFT;
        if (cy < u[worker].y) held |= KEY_DOWN;
        if (cy > u[worker].y) held |= KEY_UP;
        step(held);
    }
    if (!u[worker].alive) return 0;
    if (dist2(cx, cy, u[worker].x, u[worker].y) > 20 * 20) return 0;
    tap(KEY_A);
    return selected_building < 0 && find_selected_worker() == worker;
}

static int choose_menu_row(int row)
{
    if (row < 0 || row >= field_menu_count()) return 0;
    tap_b();
    for (int i = 0; i < row; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    return 1;
}

static int start_zerg_build(int type, int menu_row)
{
    int site_x = -1, site_y = -1, worker = -1, best = 0x7fffffff;
    int anchor = side_hatchery();
    if (anchor < 0 || minerals < building_cost(type) || gas < building_gas_cost(type) ||
        !find_build_site_near(type, b[anchor].x, b[anchor].y, &site_x, &site_y)) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && sc64_race_is_worker(u[i].type) &&
            u[i].order != ORDER_BUILD && u[i].order != ORDER_GAS) {
            int d = dist2(u[i].x, u[i].y, site_x, site_y);
            if (d < best) { best = d; worker = i; }
        }
    if (worker < 0 || !select_worker_slot(worker)) return 0;
    int before = owner_buildings(SIDE);
    if (!choose_menu_row(menu_row)) return 0;
    if (mode != MODE_BUILD || menu != type) return 0;
    move_cursor(site_x, site_y);
    tap(KEY_A);
    return owner_buildings(SIDE) > before;
}

static int morph_hatchery_to_lair(void)
{
    if (minerals < building_cost(B_LAIR) || gas < building_gas_cost(B_LAIR)) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_HATCHERY &&
            building_complete(i) && b[i].queue_count == 0) {
            move_cursor(b[i].x, b[i].y);
            tap(KEY_A);
            if (selected_building != i) continue;
            if (!choose_menu_row(MENU_ZP_LAIR)) continue;
            return b[i].type == B_LAIR && b[i].under_construction;
        }
    return 0;
}

static void assign_gas_workers(void)
{
    int refinery = side_building_type(B_EXTRACTOR, 1);
    if (refinery < 0) return;
    for (int i = 0; i < MAX_UNITS && side_gas_workers() < 3; ++i) {
        if (u[i].alive && u[i].owner == SIDE && sc64_race_is_worker(u[i].type) &&
            u[i].order != ORDER_BUILD && u[i].order != ORDER_GAS && select_worker_slot(i)) {
            move_cursor(b[refinery].x, b[refinery].y);
            tap(KEY_A);
        }
    }
}

static void advance_air_tech(void)
{
    if (side_gas_workers() < 3) assign_gas_workers();
    if (side_building_type(B_SPAWNING_POOL, 0) < 0) {
        if (minerals >= building_cost(B_SPAWNING_POOL))
            (void)start_zerg_build(B_SPAWNING_POOL, MENU_BARRACKS);
        return;
    }
    if (side_building_type(B_SPAWNING_POOL, 1) < 0) return;
    if (side_building_type(B_LAIR, 0) < 0) {
        (void)morph_hatchery_to_lair();
        return;
    }
    if (side_building_type(B_LAIR, 1) < 0) return;
    if (side_building_type(B_SPIRE, 0) < 0 &&
        minerals >= building_cost(B_SPIRE) && gas >= building_gas_cost(B_SPIRE))
        (void)start_zerg_build(B_SPIRE, MENU_ZP_SPIRE);
}

static void issue_objective_order(void)
{
    int target_x, target_y;
    if (!(race_z4.hatch_effects & 2u)) {
        if (chrysalis_position(&target_x, &target_y) < 0) return;
        /* Defend at the objective location until the authored Kerrigan handoff
         * succeeds; only then send the combat group after Force 3 Command Centers. */
        issue_attack_move(target_x, target_y);
        return;
    }
    /* Keep newly trained fliers home until a useful strike group has formed;
     * the earlier one-at-a-time attack orders let isolated Mutalisks die. */
    if (side_unit_type_count(U_MUTALISK) < 20) return;
    int cc = closest_force3_command_center(cx, cy);
    if (cc < 0) return;
    issue_attack_move(b[cc].x, b[cc].y);
}

static void print_blocker(void)
{
    fprintf(stderr, "Z4 controller stop: frame=%d result=%s outcome=%u fired=0x%x hatch=0x%x countdown=%u side=%d workers=%d army=%d CC3=%d enemy/force3=%d/%d,%d/%d faults=%u unsupported=%d\n",
            frame, won ? "WON" : lost ? "LOST" : "LIMIT", race_z4.model.outcome,
            race_z4.model.fired, race_z4.hatch_effects, race_z4.countdown_ticks,
            owner_units(SIDE), side_workers(), side_army(), force3_command_centers(),
            owner_units(ENEMY), owner_buildings(ENEMY), owner_units(4),
            owner_buildings(4), (unsigned)race_z4.faults, mission_unsupported_count);
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type) &&
            u[i].type != U_OVERLORD && u[i].type != U_CHRYSALIS) {
            fprintf(stderr, "route reachability from %d,%d (coarse 64px cells):\n", u[i].x, u[i].y);
            for (int y = 32; y < WORLD_H; y += 64) {
                int lo = -1, hi = -1, count = 0;
                for (int x = 32; x < WORLD_W; x += 64) {
                    int nx, ny;
                    if (is_traversable(x / 16, y / 16, -1) &&
                        sc64_route_next(u[i].x, u[i].y, x, y, &nx, &ny)) {
                        if (lo < 0) lo = x;
                        hi = x;
                        ++count;
                    }
                }
                if (count) fprintf(stderr, "  y=%d x=%d..%d count=%d\n", y, lo, hi, count);
            }
            break;
        }
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) {
            int nx = -1, ny = -1;
            int has_path = path_next_tile(u[i].x, u[i].y, u[i].tx, u[i].ty, &nx, &ny);
            fprintf(stderr, "  side unit raw=%d type=%d hp=%d/%d order=%d at=%d,%d target=%d tx=%d,ty=%d path=%d,%d timer=%d retry=%d\n",
                    u[i].raw_type, u[i].type, u[i].hp, u[i].maxhp, u[i].order,
                    u[i].x, u[i].y, u[i].target, u[i].tx, u[i].ty, u[i].path_x, u[i].path_y, u[i].path_timer, u[i].path_retry);
            fprintf(stderr, "    route=%d next=%d,%d direct=%d\n", has_path, nx, ny,
                    direct_path_clear(u[i].x, u[i].y, u[i].tx, u[i].ty));
        }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE)
            fprintf(stderr, "  side building raw=%d type=%d hp=%d/%d construction=%d queue=%d at=%d,%d\n",
                    b[i].raw_type, b[i].type, b[i].hp, b[i].maxhp,
                    b[i].under_construction, b[i].queue_count, b[i].x, b[i].y);
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--trace") && i + 1 < argc) trace_path = argv[++i];
        else if (!strcmp(argv[i], "--limit") && i + 1 < argc) run_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_z4_campaign_run [--limit TICKS] [--trace PATH]");
            return 0;
        } else return 2;
    }
    if (run_limit < 1 || run_limit > 1000000) return 2;
    mkdir(".local", 0777);
    mkdir(".local/sc64", 0777);
    if (!trace_path) trace_path = ".local/sc64/z4-controller-air-attempt-150000.json";
    trace = fopen(trace_path, "w");
    assert(trace);
    fprintf(trace, "{\"schema\":1,\"mission\":15,\"profile\":\"zerg-z4\","
                   "\"policy\":\"normal-controller-build-spire-produce-mutalisks-attack-force3\",\"events\":[\n");
    gba_init();
    launch_from_normal_menu();
    checkpoint("start");

    issue_objective_order();
    last_order_frame = frame;
    last_production_frame = frame;
    last_tech_frame = frame;
    while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost && !controller_input_error && !controller_limit_reached) {
        if (frame >= last_tech_frame + 300) {
            advance_air_tech();
            last_tech_frame = frame;
        }
        if (frame >= last_production_frame + 600) {
            maintain_economy();
            last_production_frame = frame;
        }
        if (frame >= last_order_frame + 900) {
            int stage = !!(race_z4.hatch_effects & 2u);
            if (stage != last_stage) {
                checkpoint(stage ? "posthatch" : "prehatch");
                last_stage = stage;
            }
            issue_objective_order();
            last_order_frame = frame;
        }
        step(0);
    }
    checkpoint(won ? "won" : lost ? "lost" : controller_input_error ? "input-error" : "limit");
    fprintf(trace, "]}\n");
    fclose(trace);
    printf("Z4 controller attempt result=%s frame=%d host_tick=%u outcome=%u hatch=0x%x remaining_force3_CC=%d side_units=%d side_buildings=%d trace=%s\n",
           won ? "WON" : lost ? "LOST" : controller_input_error ? "INPUT_ERROR" : "LIMIT", frame, (unsigned)host_ticks,
           race_z4.model.outcome, race_z4.hatch_effects, force3_command_centers(),
           owner_units(SIDE), owner_buildings(SIDE), trace_path);
    if (!won) print_blocker();
    return won ? 0 : 1;
}
