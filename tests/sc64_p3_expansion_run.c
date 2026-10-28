/* Normal-controller Higher Ground win attempt. Runtime reads choose inputs and
 * populate the trace; every gameplay mutation is produced by game_update(). */
#include "../starcraft/game.c"
#include <assert.h>
#include <errno.h>
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

static int side_cannon_count(void)
{
    return side_building_count(B_PHOTON_CANNON);
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

static int assault_focus_owner(void)
{
    int units1 = owner_units(ENEMY), units4 = owner_units(4);
    if (!units1) return 4;
    if (!units4) return ENEMY;
    /* Once one force is below a manageable mobile count, pressure the other
       front instead of spending the entire army on its remaining static base. */
    if (units1 <= 30 && units4 > 30) return 4;
    if (units4 <= 30 && units1 > 30) return ENEMY;
    return units1 <= units4 ? ENEMY : 4;
}

static int closest_foe_building(int x, int y)
{
    int focus = assault_focus_owner();
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == focus) {
            int d = dist2(x, y, b[i].x, b[i].y);
            if (d < best_d) { best = i; best_d = d; }
        }
    if (best >= 0) return best;
    int other = focus == ENEMY ? 4 : ENEMY;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == other) {
            int d = dist2(x, y, b[i].x, b[i].y);
            if (d < best_d) { best = i; best_d = d; }
        }
    return best;
}

static int remaining_foe_buildings(void)
{ return owner_buildings(ENEMY) + owner_buildings(4); }

static int raw_owner_unit_count(int raw_owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_owner == raw_owner) ++count;
    return count;
}

static int raw_owner_building_count(int raw_owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_owner == raw_owner) ++count;
    return count;
}

static void checkpoint(const char *kind)
{
    trace_prefix();
    fprintf(trace,
        "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%d,\"elapsed\":%u,"
        "\"screen\":%d,\"won\":%d,\"lost\":%d,\"p3_outcome\":%u,"
        "\"global_mission\":%d,\"selected_mission\":%d,\"race_mode\":%d,"
        "\"campaign_phase\":%u,\"campaign_objectives\":%u,\"completed\":%d,"
        "\"mission_unsupported_count\":%d,\"model_valid\":%d,"
        "\"raw_owner7_units\":%d,\"raw_owner0_units\":%d,\"raw_owner6_units\":%d,"
        "\"raw_owner7_buildings\":%d,\"raw_owner0_buildings\":%d,\"raw_owner6_buildings\":%d,"
        "\"p3_sequence\":%u,\"p3_fired\":%u,\"action_ticks\":%u,"
        "\"side_units\":%d,\"side_army\":%d,\"side_buildings\":%d,"
        "\"side_probes\":%d,\"side_cannons\":%d,"
        "\"force2_units\":%d,\"force2_buildings\":%d,"
        "\"force3_units\":%d,\"force3_buildings\":%d,"
        "\"minerals\":%d,\"gas\":%d,\"supply\":%d,\"supply_cap\":%d,"
        "\"weapons_level\":%d,\"armor_level\":%d,\"weapons_research\":%d,\"armor_research\":%d,"
        "\"zealots_queued\":%d,\"gateways\":%d,\"cursor_x\":%d,\"cursor_y\":%d,"
        "\"selected_units\":%d,\"selected_building\":%d,\"geyser_count\":%d}\n",
        kind, (unsigned)host_ticks, frame, (unsigned)race_campaign.elapsed_frames,
        screen, won, lost, race_p3.outcome,
        SC64_P3_GLOBAL_ID, race_selected_mission, race_mode,
        (unsigned)race_campaign.phase, (unsigned)race_campaign.objective_flags,
        (race_completed & (1 << SC64_P3_GLOBAL_ID)) != 0,
        mission_unsupported_count, sc64_p3_model_state_valid(&race_p3),
        raw_owner_unit_count(7), raw_owner_unit_count(0), raw_owner_unit_count(6),
        raw_owner_building_count(7), raw_owner_building_count(0), raw_owner_building_count(6),
        race_p3.sequence, race_p3.fired, race_p3_action_ticks,
        owner_units(SIDE), owner_army(SIDE), owner_buildings(SIDE),
        side_probe_count(), side_cannon_count(),
        owner_units(ENEMY), owner_buildings(ENEMY), owner_units(4), owner_buildings(4),
        minerals, gas, supply_used, supply_cap,
        weapons_level, armor_level, weapons_research, armor_research,
        queued_type(U_ZEALOT),
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
    step(KEY_START);   /* explicit launch without a gameplay tick */
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
    int start_nexus = nearest_side_building();
    int start_target = start_nexus >= 0 ? closest_foe_building(b[start_nexus].x, b[start_nexus].y) : -1;
    if (start_target >= 0) {
        int nx = -1, ny = -1;
        int route = path_next_tile(b[start_nexus].x, b[start_nexus].y,
                                   b[start_target].x, b[start_target].y, &nx, &ny);
        fprintf(stderr, "  initial route=%d nexus=%d,%d target=%d/%d at=%d,%d next=%d,%d\n",
                route, b[start_nexus].x, b[start_nexus].y, b[start_target].owner,
                b[start_target].type, b[start_target].x, b[start_target].y, nx, ny);
    }
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

static int candidate_blocks_path_cell(int type, int x, int y, int col, int row)
{
    int left, up, right, down;
    if (sc64_game_building_bounds_source(type, &left, &up, &right, &down)) {
        int x0 = x * 2 - left, x1 = x * 2 + right;
        int y0 = y * 2 - up, y1 = y * 2 + down;
        int cell_x0 = col * 32, cell_y0 = row * 32;
        return cell_x0 <= x1 && cell_x0 + 32 >= x0 &&
               cell_y0 <= y1 && cell_y0 + 32 >= y0;
    }
    return dist2(col * 16 + 8, row * 16 + 8, x, y) < 18 * 18;
}

static int route_survives_candidate(int sx, int sy, int gx, int gy,
                                    int type, int bx, int by)
{
    static const s8 dx[4] = {1, -1, 0, 0};
    static const s8 dy[4] = {0, 0, 1, -1};
    u8 seen[MAP_CELLS];
    u16 queue[MAP_CELLS];
    int start_col = limit(sx / 16, 0, MAP_COLS - 1);
    int start_row = limit(sy / 16, 0, MAP_ROWS - 1);
    int goal_col = limit(gx / 16, 0, MAP_COLS - 1);
    int goal_row = limit(gy / 16, 0, MAP_ROWS - 1);
    int start = start_col + start_row * MAP_COLS;
    int goal = goal_col + goal_row * MAP_COLS;
    int head = 0, tail = 0;
    if (!path_grid_valid) build_path_grid();
    if (candidate_blocks_path_cell(type, bx, by, goal_col, goal_row)) return 0;
    memset(seen, 0, sizeof(seen));
    seen[start] = 1;
    queue[tail++] = (u16)start;
    while (head < tail) {
        int cell = queue[head++];
        int col = cell % MAP_COLS, row = cell / MAP_COLS;
        if (cell == goal) return 1;
        for (int dir = 0; dir < 4; ++dir) {
            int nc = col + dx[dir], nr = row + dy[dir];
            if (nc < 0 || nr < 0 || nc >= MAP_COLS || nr >= MAP_ROWS) continue;
            int next = nc + nr * MAP_COLS;
            if (seen[next] || (next != start &&
                (!SC64_GRID_GET(path_walkable, next) ||
                 candidate_blocks_path_cell(type, bx, by, nc, nr)))) continue;
            seen[next] = 1;
            queue[tail++] = (u16)next;
        }
    }
    return 0;
}

static int path_waypoint(int sx, int sy, int gx, int gy, int *wx, int *wy)
{
    int x = sx, y = sy;
    for (int step = 0; step < 8; ++step) {
        int nx = x, ny = y;
        if (!path_next_tile(x, y, gx, gy, &nx, &ny) || (nx == x && ny == y)) break;
        x = nx; y = ny;
    }
    if (x == sx && y == sy) return 0;
    *wx = x; *wy = y;
    return 1;
}

static int candidate_preserves_routes(int type, int bx, int by)
{
    int nexus = nearest_side_building(), has_route = 0, wx, wy;
    if (nexus < 0) return 0;
    if (!path_grid_valid) build_path_grid();
    for (int owner_index = 0; owner_index < 2; ++owner_index) {
        int owner = owner_index == 0 ? ENEMY : 4;
        int target = -1, best = 0x7fffffff, wx, wy;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == owner) {
                int d = dist2(b[nexus].x, b[nexus].y, b[i].x, b[i].y);
                if (d < best) { best = d; target = i; }
            }
        if (target < 0 || !path_waypoint(b[nexus].x, b[nexus].y,
                                          b[target].x, b[target].y, &wx, &wy)) continue;
        has_route = 1;
        if (!route_survives_candidate(b[nexus].x, b[nexus].y, wx, wy, type, bx, by)) return 0;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == SIDE && b[i].type == B_GATEWAY &&
                path_waypoint(b[i].x, b[i].y, b[target].x, b[target].y, &wx, &wy) &&
                !route_survives_candidate(b[i].x, b[i].y, wx, wy, type, bx, by)) return 0;
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].order == ORDER_ATTACK_MOVE &&
            path_waypoint(u[i].x, u[i].y, u[i].tx, u[i].ty, &wx, &wy)) {
            has_route = 1;
            if (!route_survives_candidate(u[i].x, u[i].y, wx, wy, type, bx, by)) return 0;
        }
    return has_route;
}

static int find_build_site_near_nexus(int type, int *x_out, int *y_out)
{
    int nexus = nearest_side_building();
    if (nexus < 0) return 0;
    int best = 0x7fffffff, found = 0;
    for (int dy = -256; dy <= 256; dy += 16)
        for (int dx = -256; dx <= 256; dx += 16) {
            int x = b[nexus].x + dx, y = b[nexus].y + dy;
            if (x < 32 || x >= WORLD_W - 32 || y < 32 || y >= WORLD_H - 32 ||
                !race_can_site(SIDE, type, x, y) || !can_place(type, x, y)) continue;
            int d = dist2(b[nexus].x, b[nexus].y, x, y);
            /* Keep the northwest attack corridor clear. The large-bank army
               can otherwise seal its own exit with the repeated near-Nexus
               Pylon/Gateway placement policy. */
            if (d < 96 * 96 || !candidate_preserves_routes(type, x, y)) continue;
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

static void build_cannon_near_nexus(void)
{
    int site_x, site_y, worker = -1, best = 0x7fffffff;
    if (minerals < 150 || side_building(B_FORGE, 1) < 0 ||
        !find_build_site_near_nexus(B_PHOTON_CANNON, &site_x, &site_y)) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
            u[i].order != ORDER_BUILD) {
            int d = dist2(u[i].x, u[i].y, site_x, site_y);
            if (d < best) { best = d; worker = i; }
        }
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    tap(KEY_B);
    for (int row = 0; row < MENU_STOP + 2; ++row) tap(KEY_DOWN);
    tap(KEY_A); /* Contextual Probe command: NEXUS, then CANNON. */
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
    if (gas_workers >= 4) return;
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

static int expansion_started;
static int expansion_slot = -1;
static int expansion2_started;
static int expansion2_slot = -1;
static int expansion3_started;
static int expansion3_slot = -1;
static int second_site_cleared;
static int second_site_raid_started;
static const int expansion_x = 840, expansion_y = 112;
static const int expansion2_x = 216, expansion2_y = 1496;
static const int expansion3_x = 1824, expansion3_y = 240;

static int try_build_expansion_nexus(void)
{
    int site = race_can_site(SIDE, B_NEXUS, expansion_x, expansion_y) &&
               can_place(B_NEXUS, expansion_x, expansion_y);
    if (expansion_started || frame < 2400 || minerals < 400 || !site) return 0;
    int nearest = -1, best = 0x7fffffff;
    for (int i=0; i<MAX_UNITS; ++i) if (u[i].alive && u[i].owner==SIDE &&
            u[i].type==U_PROBE && u[i].order!=ORDER_BUILD) {
        int d=dist2(u[i].x,u[i].y,expansion_x,expansion_y);
        if (d<best) { best=d; nearest=i; }
    }
    if (nearest<0) return 0;
    int selected_worker = select_worker_at(u[nearest].x,u[nearest].y);
    if (selected_worker<0) return 0;
    tap(KEY_B);
    for (int i=0;i<MENU_STOP+1;++i) tap(KEY_DOWN);
    if (menu_cursor!=MENU_STOP+1) return 0;
    tap(KEY_A);
    if (mode!=MODE_BUILD || menu!=B_NEXUS) return 0;
    move_cursor(expansion_x,expansion_y);
    tap(KEY_A);
    if (u[selected_worker].order!=ORDER_BUILD || u[selected_worker].target<0 ||
        u[selected_worker].target>=MAX_BUILDINGS || b[u[selected_worker].target].type!=B_NEXUS) {
        if (mode==MODE_BUILD) tap(KEY_B);
        return 0;
    }
    expansion_started=1;
    expansion_slot=u[selected_worker].target;
    fprintf(stderr,"EXPANSION queued frame=%d probe=%d nexus=%d site=%d,%d minerals=%d\n",
            frame,selected_worker,expansion_slot,b[expansion_slot].x,b[expansion_slot].y,minerals);
    return 1;
}


static int try_build_second_nexus(void)
{
    if (!second_site_cleared) return 0;
    int site = race_can_site(SIDE, B_NEXUS, expansion2_x, expansion2_y) &&
               can_place(B_NEXUS, expansion2_x, expansion2_y);
    if (expansion2_started || frame < 16000 || minerals < 400 || !site) return 0;
    int nearest = -1, best = 0x7fffffff;
    for (int i=0; i<MAX_UNITS; ++i) if (u[i].alive && u[i].owner==SIDE &&
            u[i].type==U_PROBE && u[i].order!=ORDER_BUILD) {
        int d=dist2(u[i].x,u[i].y,expansion2_x,expansion2_y);
        if (d<best) { best=d; nearest=i; }
    }
    if (nearest<0) return 0;
    int selected_worker = select_worker_at(u[nearest].x,u[nearest].y);
    if (selected_worker<0) return 0;
    tap(KEY_B);
    for (int i=0;i<MENU_STOP+1;++i) tap(KEY_DOWN);
    if (menu_cursor!=MENU_STOP+1) return 0;
    tap(KEY_A);
    if (mode!=MODE_BUILD || menu!=B_NEXUS) return 0;
    move_cursor(expansion2_x,expansion2_y);
    tap(KEY_A);
    if (u[selected_worker].order!=ORDER_BUILD || u[selected_worker].target<0 ||
        u[selected_worker].target>=MAX_BUILDINGS || b[u[selected_worker].target].type!=B_NEXUS) {
        if (mode==MODE_BUILD) tap(KEY_B);
        return 0;
    }
    expansion2_started=1;
    expansion2_slot=u[selected_worker].target;
    fprintf(stderr,"EXPANSION2 queued frame=%d probe=%d nexus=%d site=%d,%d minerals=%d\n",
            frame,selected_worker,expansion2_slot,b[expansion2_slot].x,b[expansion2_slot].y,minerals);
    return 1;
}

static int hostiles_near(int x, int y, int radius)
{
    int n=0;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && (u[i].owner==ENEMY || u[i].owner==4) &&
            dist2(u[i].x,u[i].y,x,y)<=radius*radius) ++n;
    return n;
}

static int try_build_third_nexus(void)
{
    static int last_diagnostic;
    if (frame >= 60000 && frame-last_diagnostic >= 6000) {
        last_diagnostic=frame;
        fprintf(stderr,"EXPANSION3 check frame=%d owner4army=%d nearby=%d minerals=%d site=%d place=%d probes=%d\n",
                frame,owner_army(4),hostiles_near(expansion3_x,expansion3_y,128),minerals,
                race_can_site(SIDE,B_NEXUS,expansion3_x,expansion3_y),
                can_place(B_NEXUS,expansion3_x,expansion3_y),owner_units(SIDE));
    }
    if (!expansion_started || !expansion2_started || expansion3_started ||
        frame < 60000 || owner_army(4)>3 || hostiles_near(expansion3_x,expansion3_y,128)>0 || minerals<400 ||
        !race_can_site(SIDE,B_NEXUS,expansion3_x,expansion3_y) ||
        !can_place(B_NEXUS,expansion3_x,expansion3_y)) return 0;
    if (menu_open) tap(KEY_B);
    if (mode != MODE_NORMAL) tap(KEY_B);
    if (menu_open || mode != MODE_NORMAL) { fprintf(stderr,"EXPANSION3 normalize failed mode=%d menu=%d\n",mode,menu_open); return 0; }
    int nearest=-1,best=0x7fffffff;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive&&u[i].owner==SIDE&&
            u[i].type==U_PROBE&&u[i].order!=ORDER_BUILD) {
        int d=dist2(u[i].x,u[i].y,expansion3_x,expansion3_y);
        if(d<best){best=d;nearest=i;}
    }
    if(nearest<0)return 0;
    int worker=select_worker_at(u[nearest].x,u[nearest].y);
    if(worker<0)return 0;
    tap(KEY_B);
    for(int i=0;i<MENU_STOP+1;++i)tap(KEY_DOWN);
    if(menu_cursor!=MENU_STOP+1)return 0;
    tap(KEY_A);
    if(mode!=MODE_BUILD||menu!=B_NEXUS||menu_open)return 0;
    int target_x=snap4(cx,expansion3_x,WORLD_W);
    int target_y=snap4(cy,expansion3_y,WORLD_H);
    for(int n=0;n<4096&&(cx!=target_x||cy!=target_y);++n){
        int old_x=cx,old_y=cy;
        u16 held=KEY_R;
        if(cx<target_x)held|=KEY_RIGHT;
        if(cx>target_x)held|=KEY_LEFT;
        if(cy<target_y)held|=KEY_DOWN;
        if(cy>target_y)held|=KEY_UP;
        step(held);
        if(cx==old_x&&cy==old_y)break;
    }
    step(0);
    if(cx!=target_x||cy!=target_y){tap(KEY_B);return 0;}
    tap(KEY_A);
    if(u[worker].order!=ORDER_BUILD||u[worker].target<0||
       u[worker].target>=MAX_BUILDINGS||b[u[worker].target].type!=B_NEXUS) {
        for(int k=0;k<2&&(mode!=MODE_NORMAL||menu_open);++k)tap(KEY_B);
        return 0;
    }
    expansion3_started=1; expansion3_slot=u[worker].target;
    fprintf(stderr,"EXPANSION3 queued frame=%d probe=%d nexus=%d site=%d,%d minerals=%d\n",
            frame,worker,expansion3_slot,b[expansion3_slot].x,b[expansion3_slot].y,minerals);
    return 1;
}

static int building_near(int type, int x, int y, int radius)
{
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].owner==SIDE && b[i].type==type &&
            dist2(b[i].x,b[i].y,x,y)<=radius*radius) return i;
    return -1;
}

static int protect_expansion(void)
{
    if (!expansion_started || expansion_slot<0 || expansion_slot>=MAX_BUILDINGS ||
        !building_complete(expansion_slot)) return 0;
    if (building_near(B_PYLON,760,160,112)<0 && minerals>=100 &&
        race_can_site(SIDE,B_PYLON,760,160) && can_place(B_PYLON,760,160)) {
        int before=minerals;
        build_on_site(B_PYLON,MENU_DEPOT,760,160);
        if (minerals<before) {
            fprintf(stderr,"EXPANSION pylon frame=%d minerals=%d\n",frame,minerals);
            return 1;
        }
    }
    if (building_near(B_PYLON,760,160,112)>=0 &&
        !b[building_near(B_PYLON,760,160,112)].under_construction &&
        building_near(B_PHOTON_CANNON,840,224,144)<0 && minerals>=150 &&
        side_building(B_FORGE,1)>=0 && race_site_powered(SIDE,840,224) &&
        race_can_site(SIDE,B_PHOTON_CANNON,840,224) && can_place(B_PHOTON_CANNON,840,224)) {
        int before=minerals;
        build_on_site(B_PHOTON_CANNON,MENU_STOP+2,840,224);
        if (minerals<before) {
            fprintf(stderr,"EXPANSION cannon frame=%d minerals=%d\n",frame,minerals);
            return 1;
        }
    }
    return 0;
}

static int protect_second_expansion(void)
{
    if (!expansion2_started || expansion2_slot<0 || expansion2_slot>=MAX_BUILDINGS ||
        !building_complete(expansion2_slot)) return 0;
    if (building_near(B_PYLON,320,1480,112)<0 && minerals>=100 &&
        race_can_site(SIDE,B_PYLON,320,1480) && can_place(B_PYLON,320,1480)) {
        int before=minerals;
        build_on_site(B_PYLON,MENU_DEPOT,320,1480);
        if (minerals<before) {
            fprintf(stderr,"EXPANSION2 pylon frame=%d minerals=%d\n",frame,minerals);
            return 1;
        }
    }
    int pylon=building_near(B_PYLON,320,1480,112);
    if (pylon>=0 && !b[pylon].under_construction &&
        building_near(B_PHOTON_CANNON,400,1480,144)<0 && minerals>=150 &&
        side_building(B_FORGE,1)>=0 && race_site_powered(SIDE,400,1480) &&
        race_can_site(SIDE,B_PHOTON_CANNON,400,1480) && can_place(B_PHOTON_CANNON,400,1480)) {
        int before=minerals;
        build_on_site(B_PHOTON_CANNON,MENU_STOP+2,400,1480);
        if (minerals<before) {
            fprintf(stderr,"EXPANSION2 cannon frame=%d minerals=%d\n",frame,minerals);
            return 1;
        }
    }
    return 0;
}

static int protect_third_expansion(void)
{
    if(!expansion3_started||expansion3_slot<0||expansion3_slot>=MAX_BUILDINGS||
       !building_complete(expansion3_slot))return 0;
    if(building_near(B_PYLON,1808,128,112)<0&&minerals>=100&&
       race_can_site(SIDE,B_PYLON,1808,128)&&can_place(B_PYLON,1808,128)) {
        int before=minerals; build_on_site(B_PYLON,MENU_DEPOT,1808,128);
        if(minerals<before){fprintf(stderr,"EXPANSION3 pylon frame=%d\n",frame);return 1;}
    }
    int p=building_near(B_PYLON,1808,128,112);
    if(p>=0&&!b[p].under_construction&&building_near(B_PHOTON_CANNON,1904,224,144)<0&&
       minerals>=150&&side_building(B_FORGE,1)>=0&&race_site_powered(SIDE,1904,224)&&
       race_can_site(SIDE,B_PHOTON_CANNON,1904,224)&&can_place(B_PHOTON_CANNON,1904,224)) {
        int before=minerals;build_on_site(B_PHOTON_CANNON,MENU_STOP+2,1904,224);
        if(minerals<before){fprintf(stderr,"EXPANSION3 cannon frame=%d\n",frame);return 1;}
    }
    return 0;
}

static void build_pylons_and_gateways(void)
{
    int free_supply = race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE);
    if (side_building(B_GATEWAY, 0) < 0 && minerals >= 150) {
        build_near_nexus(B_GATEWAY, MENU_BARRACKS);
        return;
    }
    if (side_building(B_FORGE, 0) < 0 && minerals >= 150 &&
        side_building(B_GATEWAY, 1) >= 0) {
        build_near_nexus(B_FORGE, MENU_ENG_BAY);
        return;
    }
    if (side_cannon_count() < 10 && side_building(B_FORGE, 1) >= 0 && minerals >= 150) {
        build_cannon_near_nexus();
        return;
    }
    if (free_supply <= 24 && minerals >= 100 && side_building_count(B_PYLON) < 12) {
        build_near_nexus(B_PYLON, MENU_DEPOT);
        return;
    }
    if (side_building(B_CYBERNETICS_CORE, 0) < 0 && minerals >= 200 &&
        side_building(B_GATEWAY, 1) >= 0) {
        build_near_nexus(B_CYBERNETICS_CORE, MENU_ACADEMY);
        return;
    }
    if (side_building_count(B_GATEWAY) < 2 && minerals >= 150) {
        build_near_nexus(B_GATEWAY, MENU_BARRACKS);
        return;
    }
    if (side_building_count(B_ASSIMILATOR) < 3 && minerals >= 100)
        build_assimilator();
}

static void queue_probe(void)
{
    int nexus = nearest_side_building();
    if (nexus < 0 || side_probe_count() >= 40 || b[nexus].queue_count >= 4 || minerals < 50)
        return;
    move_cursor(b[nexus].x, b[nexus].y);
    tap(KEY_A); tap(KEY_A); /* selected Nexus queues its Probe normally */
}

static void research_forge_upgrade(void)
{
    int forge = side_building(B_FORGE, 1), choice = -1;
    if (forge < 0 || gas < 100 || minerals < 100) return;
    if (weapons_level < 2 && !weapons_research) choice = MENU_WEAPONS;
    else if (armor_level < 2 && !armor_research) choice = MENU_ARMOR;
    if (choice < 0) return;
    move_cursor(b[forge].x, b[forge].y);
    tap(KEY_A);
    if (selected_building != forge) return;
    tap(KEY_B);
    for (int row = 0; row < choice; ++row) tap(KEY_DOWN);
    tap(KEY_A);
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

static int nexus_threat(int nexus, int *target_out)
{
    int target = -1, best = 240 * 240;
    if (nexus < 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && (u[i].owner == ENEMY || u[i].owner == 4)) {
            int d = dist2(b[nexus].x, b[nexus].y, u[i].x, u[i].y);
            if (d < best) { best = d; target = i; }
        }
    if (target < 0) return 0;
    *target_out = target;
    return 1;
}

static void issue_sweep_order(int first_push)
{
    (void)first_push;
    int nexus = -1, frontier = -1, farthest = -1, target = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_NEXUS) { nexus = i; break; }
    if (nexus < 0) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type) &&
            u[i].type != U_OVERLORD && u[i].type != U_CHRYSALIS) {
            int d = dist2(b[nexus].x, b[nexus].y, u[i].x, u[i].y);
            if (d > farthest) { farthest = d; frontier = i; }
        }
    if (frontier < 0) return;
    target = closest_foe_building(u[frontier].x, u[frontier].y);
    if (target < 0) return;
    /* Reissue the ordinary combat-group order so freshly produced and
       previously idle combat units join the active sweep. */
    if (select_group_for_control()) attack_move_selected(b[target].x, b[target].y);
}

static void issue_home_defense(int nexus, int target)
{
    if (nexus < 0 || target < 0) return;
    /* Use the normal R combat-group selection. A local box around the Nexus
       can include mineral Probes and send them into the attack. */
    if (select_group_for_control())
        attack_move_selected(u[target].x, u[target].y);
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
            fprintf(stderr, "  army type=%d order=%d target=%d tx=%d,ty=%d at=%d,%d\n",
                    u[i].type, u[i].order, u[i].target, u[i].tx, u[i].ty, u[i].x, u[i].y);
            int next_x = -1, next_y = -1;
            int route = path_next_tile(u[i].x, u[i].y, u[i].tx, u[i].ty, &next_x, &next_y);
            fprintf(stderr,
                    "    path=%d next=%d,%d cache=%d,%d timer=%u retry=%u tileblocked=%d stepclear=%d\n",
                    route, next_x, next_y, u[i].path_x, u[i].path_y, u[i].path_timer,
                    u[i].path_retry, terrain_blocked(u[i].x / 16, u[i].y / 16),
                    route ? unit_step_clear(&u[i], next_x, next_y) : 0);
            if (u[i].type == U_ZEALOT || u[i].type == U_DRAGOON || u[i].type == U_SCOUT) {
                int acquire = unit_range(u[i].type) > 64 ? unit_range(u[i].type) + 18 : 64;
                int nearest = nearest_enemy_typed(u[i].x, u[i].y, SIDE, acquire, u[i].type
                                                 SC64_P7_SOURCE_OWNER_ARG(u[i].raw_owner));
                int closest_owner = -1, closest_type = -1, closest_d2 = 0x7fffffff;
                for (int j = 0; j < MAX_UNITS; ++j)
                    if (u[j].alive && (u[j].owner == ENEMY || u[j].owner == 4)) {
                        int d2 = dist2(u[i].x, u[i].y, u[j].x, u[j].y);
                        if (d2 < closest_d2) {
                            closest_d2 = d2; closest_owner = u[j].owner; closest_type = u[j].type;
                        }
                    }
                for (int j = 0; j < MAX_BUILDINGS; ++j)
                    if (b[j].alive && (b[j].owner == ENEMY || b[j].owner == 4)) {
                        int d2 = dist2(u[i].x, u[i].y, b[j].x, b[j].y);
                        if (d2 < closest_d2) {
                            closest_d2 = d2; closest_owner = b[j].owner; closest_type = b[j].type;
                        }
                    }
                int nflying = nearest >= 0 && nearest < MAX_UNITS &&
                              sc64_race_unit_flying(u[nearest].type);
                fprintf(stderr,
                        "    acquire=%d weapon=%d cd=%d nearest=%d nearest_d2=%d closest=%d/%d d2=%d\n",
                        acquire, (sc64_race_unit_range_vs(u[i].type, nflying) + 1) / 2,
                        u[i].cd, nearest,
                        nearest >= 0 ? (int)dist2(u[i].x, u[i].y, target_x(nearest), target_y(nearest)) : -1,
                        closest_owner, closest_type, closest_d2);
            }
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
        if (!strcmp(argv[i], "--trace") && i + 1 < argc && argv[i + 1][0]) {
            trace_path = argv[++i];
        } else if (!strcmp(argv[i], "--limit") && i + 1 < argc) {
            char *end = NULL;
            errno = 0;
            long value = strtol(argv[++i], &end, 10);
            if (errno || !end || *end || value < 1 || value > 1000000) {
                fprintf(stderr, "invalid --limit value\n");
                return 2;
            }
            run_limit = (int)value;
        } else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_p3_expansion_run [--limit TICKS] [--trace PATH]");
            return 0;
        } else {
            fprintf(stderr, "unknown or incomplete argument: %s\n", argv[i]);
            return 2;
        }
    }
    if (!trace_path) trace_path = ".local/sc64/p3-expansion-controller.json";
    mkdir(".local", 0777);
    mkdir(".local/sc64", 0777);
    trace = fopen(trace_path, "w");
    if (!trace) {
        fprintf(stderr, "cannot open trace output: %s\n", trace_path);
        return 2;
    }
    fprintf(trace, "{\"schema\":1,\"mission\":22,\"policy\":\"protoss-three-expansion-source-cleared\",\"events\":[\n");
    gba_init();
    launch_p3();
    checkpoint("start");

    int next_order = frame;
    int next_expansion = frame + 240;
    int next_expansion2 = frame + 1200;
    int next_expansion3 = frame + 30000;
    int next_site_raid = frame + 9000;
    int first_push = 1;
    int next_queue = frame + 30;
    int next_worker = frame + 60;
    int next_build = frame + 10;
    int next_gas = frame + 10;
    int next_research = frame + 120;
    int next_status = frame + 10000;
    int assault_started = 0;
    while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost) {
        if (frame >= next_expansion) {
            if (!expansion_started) try_build_expansion_nexus();
            next_expansion = frame + (expansion_started ? 6000 : 240);
        }
        if (frame >= next_expansion2) {
            if (expansion_started && !expansion2_started) try_build_second_nexus();
            next_expansion2 = frame + (expansion2_started ? 6000 : 240);
        }
        if (frame >= next_expansion3) {
            if (!expansion3_started) try_build_third_nexus();
            next_expansion3 = frame + (expansion3_started ? 6000 : 240);
        }
        if (!second_site_raid_started && frame >= next_site_raid && expansion_started &&
            owner_army(SIDE) >= 10 && side_cannon_count() >= 4) {
            issue_attack_move(320,1480);
            second_site_raid_started=1;
            fprintf(stderr,"EXPANSION2 raid frame=%d nearby=%d\n",frame,hostiles_near(216,1496,160));
        }
        if (second_site_raid_started && !second_site_cleared &&
            hostiles_near(216,1496,128)==0) {
            second_site_cleared=1;
            fprintf(stderr,"EXPANSION2 site-clear frame=%d\n",frame);
        }
        if (frame >= next_build) {
            if (!protect_expansion() && !protect_second_expansion() &&
                !protect_third_expansion()) build_pylons_and_gateways();
            next_build = frame + 300;
        }
        if (frame >= next_worker) {
            queue_probe();
            next_worker = frame + 240;
        }
        if (frame >= next_gas) {
            assign_gas_probe();
            next_gas = frame + 180;
        }
        if (frame >= next_research) {
            research_forge_upgrade();
            next_research = frame + 600;
        }
        if (frame >= next_queue && side_building(B_GATEWAY, 1) >= 0) {
            if (side_building(B_CYBERNETICS_CORE, 1) >= 0 && gas >= 50)
                queue_dragoon();
            else
                queue_zealot();
            next_queue = frame + 150;
        }
        if (frame >= next_order && second_site_raid_started && !assault_started && frame < 30000) {
            next_order=frame+900;
        } else if (frame >= next_order) {
            if (!assault_started && frame >= 30000 && owner_army(SIDE) >= 18 &&
                side_cannon_count() >= 8)
                assault_started = 1;
            if (assault_started) {
                int nexus = -1, target = -1;
                for (int i = 0; i < MAX_BUILDINGS; ++i)
                    if (b[i].alive && b[i].owner == SIDE && b[i].type == B_NEXUS) { nexus = i; break; }
                if (!first_push && nexus_threat(nexus, &target)) {
                    issue_home_defense(nexus, target);
                    next_order = frame + 1800;
                } else {
                    issue_sweep_order(first_push);
                    first_push = 0;
                    next_order = frame + 6000;
                }
            } else {
                issue_defense_order();
                next_order = frame + 900;
            }
        }
        step(0);
        if (frame >= next_status) {
            checkpoint("progress");
            next_status += 10000;
        }
    }
    int source_win = won && !lost && screen == SCREEN_RESULT &&
                     race_p3.outcome == SC64_P3_WON &&
                     race_p3.sequence == SC64_P3_SEQ_DONE_WIN &&
                     sc64_p3_model_state_valid(&race_p3) &&
                     race_campaign.phase == SC64_RACE_PHASE_COMPLETE &&
                     (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) &&
                     (race_completed & (1 << SC64_P3_GLOBAL_ID)) &&
                     mission_unsupported_count == 0 && remaining_foe_buildings() == 0;
    const char *terminal = source_win ? "won" : lost ? "lost" : won ? "invalid" : "limit";
    checkpoint(terminal);
    fprintf(trace, "]}\n");
    fclose(trace);
    printf("P3 expansion run result=%s frame=%d outcome=%u remaining_enemy_buildings=%d side_units=%d side_buildings=%d trace=%s\n",
           source_win ? "WON" : lost ? "LOST" : won ? "INVALID" : "LIMIT", frame, race_p3.outcome,
           remaining_foe_buildings(), owner_units(SIDE), owner_buildings(SIDE), trace_path);
    if (!source_win) print_blocker();
    return source_win ? 0 : 1;
}
