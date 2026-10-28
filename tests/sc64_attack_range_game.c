/* SPDX-License-Identifier: GPL-3.0-or-later
 * Synthetic regressions in the actual imported-race movement/combat code.
 * This fixture is not campaign completion or performance evidence. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

static int attacker, victim;

#ifdef SC64_ATTACK_RANGE_ROUTES
static int cell_distance2(int x, int y, int tx, int ty)
{
    return dist2(x, y, tx, ty);
}

static void clear_grid_and_routes(void)
{
    memset(path_walkable, 0xff, sizeof(path_walkable));
    path_grid_valid = 1;
    sc64_routes_reset();
}

static void block_target_ring(int tx, int ty)
{
    int col = tx / 16, row = ty / 16;
    for (int y = row - 1; y <= row + 1; ++y)
        for (int x = col - 1; x <= col + 1; ++x)
            SC64_GRID_SET(path_walkable, x + y * MAP_COLS, 0);
    SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 1);
}

static void assert_in_weapon_band(int x, int y, int tx, int ty,
                                  int minimum, int maximum)
{
    int d2 = cell_distance2(x, y, tx, ty);
    assert(d2 >= minimum * minimum);
    assert(d2 <= maximum * maximum);
    assert(SC64_GRID_GET(path_walkable, x / 16 + (y / 16) * MAP_COLS));
}

/* The direct candidate finder must use its bounded grid/corridor path and
 * leave the reverse-region BFS counter untouched. The source is 480 pixels
 * from the target, with a closed 3x3 target ring. */
static void direct_waypoint_far_origin_and_keys(void)
{
    const int tx = 512, ty = 400, sx = 32, sy = 400;
    int nx, ny, builds, point_slot, cached_cell;

    clear_grid_and_routes();
    block_target_ring(tx, ty);
    assert(!path_next_tile(sx, sy, tx, ty, &nx, &ny));
    sc64_routes_reset();
    builds = (int)sc64_route_builds;
    assert(sc64_direct_firing_waypoint(sx, sy, tx, ty, 0, 210, &nx, &ny));
    assert_in_weapon_band(nx, ny, tx, ty, 0, 210);
    assert(direct_path_clear(sx, sy, nx, ny));
    assert(sc64_route_builds == (u32)builds);
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 5);
    assert(sc64_route_range_key_matches(tx, ty, 0, 210));
    cached_cell = sc64_route_goal[SC64_ROUTE_ACTIVE_SLOTS - 1];

    /* A second origin on the same clear side reuses the stored tile. */
    assert(sc64_direct_firing_waypoint(sx + 16, sy, tx, ty, 0, 210, &nx, &ny));
    assert(nx == (cached_cell % MAP_COLS) * 16 + 8);
    assert(ny == (cached_cell / MAP_COLS) * 16 + 8);
    assert(sc64_route_builds == (u32)builds);

    /* Exact target subtile and both band endpoints belong to the key. */
    assert(sc64_direct_firing_waypoint(sx, sy, tx + 1, ty, 0, 210, &nx, &ny));
    assert(sc64_route_range_key_matches(tx + 1, ty, 0, 210));
    assert_in_weapon_band(nx, ny, tx + 1, ty, 0, 210);
    assert(sc64_direct_firing_waypoint(sx, sy, tx + 1, ty, 50, 210, &nx, &ny));
    assert(sc64_route_range_key_matches(tx + 1, ty, 50, 210));
    assert_in_weapon_band(nx, ny, tx + 1, ty, 50, 210);

    /* Inclusive minimum and maximum edges use exact tile-center distances. */
    clear_grid_and_routes();
    {
        int band_tx = 50 * 16 + 8, band_ty = 25 * 16 + 8;
        assert(sc64_direct_firing_waypoint(band_tx, band_ty, band_tx, band_ty,
                                           16, 64, &nx, &ny));
        assert(cell_distance2(nx, ny, band_tx, band_ty) == 16 * 16);
        assert(sc64_direct_firing_waypoint(band_tx, band_ty, band_tx, band_ty,
                                           64, 64, &nx, &ny));
        assert(cell_distance2(nx, ny, band_tx, band_ty) == 64 * 64);
    }

    /* A direct waypoint lives only in the reserved slot. Point fields still
     * allocate from the other slots and do not erase the valid=5 marker. */
    clear_grid_and_routes();
    assert(sc64_direct_firing_waypoint(sx, sy, tx, ty, 0, 210, &nx, &ny));
    point_slot = sc64_route_slot(4 + 4 * MAP_COLS);
    assert(point_slot >= 0 && point_slot < SC64_ROUTE_POINT_SLOTS);
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 5);

    /* Cache validity is derived state and the existing reset invalidates it. */
    sc64_routes_reset();
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 0);
    assert(sc64_direct_firing_waypoint(sx, sy, tx, ty, 0, 210, &nx, &ny));
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 5);
}

/* A cached tile on the opposite side of a wall is rejected from the new
 * origin; candidate scanning finds a direct tile on this side instead. */
static void direct_waypoint_rechecks_and_rescans(void)
{
    const int tx = 512, ty = 400, left_x = 32, right_x = 992, y = 400;
    int nx, ny, builds, left_cell;
    clear_grid_and_routes();
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 32 + row * MAP_COLS, 0);
    assert(sc64_direct_firing_waypoint(left_x, y, tx, ty, 0, 210, &nx, &ny));
    assert(nx < tx);
    left_cell = sc64_route_goal[SC64_ROUTE_ACTIVE_SLOTS - 1];
    builds = (int)sc64_route_builds;

    assert(SC64_GRID_GET(path_walkable, 44 + 25 * MAP_COLS));
    assert(direct_path_clear(right_x, y, 44 * 16 + 8, 25 * 16 + 8));
    assert(sc64_direct_firing_waypoint(right_x, y, tx, ty, 0, 210, &nx, &ny));
    assert(nx > tx);
    assert(sc64_route_goal[SC64_ROUTE_ACTIVE_SLOTS - 1] != left_cell);
    assert(direct_path_clear(right_x, y, nx, ny));
    assert(sc64_route_builds == (u32)builds);
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 5);
}

/* All top-eight firing tiles lie beyond a wall, and their direct corridors
 * are blocked. The authored multi-source BFS remains the fallback that finds
 * the route through the wall's distant opening. */
static void direct_waypoint_miss_falls_back_to_region_bfs(void)
{
    const int tx = 512, ty = 400, sx = 32, sy = 400;
    int nx, ny, builds, cell;
    clear_grid_and_routes();
    for (int row = 0; row < MAP_ROWS; ++row)
        if (row < 5 || row > 7)
            SC64_GRID_SET(path_walkable, 12 + row * MAP_COLS, 0);

    assert(!sc64_direct_firing_waypoint(sx, sy, tx, ty, 0, 160, &nx, &ny));
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 0);
    builds = (int)sc64_route_builds;
    assert(sc64_route_range_next(sx, sy, tx, ty, 0, 160, &nx, &ny));
    assert(sc64_route_builds == (u32)builds + 1u);
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 3);
    cell = nx / 16 + (ny / 16) * MAP_COLS;
    assert(SC64_GRID_GET(path_walkable, cell));
    assert(abs(nx / 16 - sx / 16) + abs(ny / 16 - sy / 16) == 1);
}

#if defined(SC64_REACHABLE_FORMATION) && defined(SC64_IMPORTED_MISSION) && \
    !defined(SC64_ROUTE_PARTIAL_FLOOD) && !defined(SC64_ROUTE_VISIT_GRID)
static void direct_waypoint_is_not_a_component_hint(void)
{
    const int tx = 12 * 16 + 8, ty = 10 * 16 + 8;
    int nx, ny, hint_slots[SC64_ROUTE_COMPONENT_HINTS];
    int hint_roots[SC64_ROUTE_COMPONENT_HINTS], hint_next = 0;
    int starts[] = {1 + 10 * MAP_COLS}, goals[] = {3 + 10 * MAP_COLS};
    int other[] = {18 + 15 * MAP_COLS};

    memset(path_walkable, 0, sizeof(path_walkable));
    for (int col = 1; col <= 3; ++col)
        SC64_GRID_SET(path_walkable, col + 10 * MAP_COLS, 1);
    for (int col = 18; col <= 20; ++col)
        SC64_GRID_SET(path_walkable, col + 15 * MAP_COLS, 1);
    path_grid_valid = 1;
    sc64_routes_reset();
    assert(sc64_direct_firing_waypoint(1 * 16 + 8, 10 * 16 + 8,
                                       tx, ty, 0, 160, &nx, &ny));
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 5);
    for (int i = 0; i < SC64_ROUTE_COMPONENT_HINTS; ++i)
        hint_slots[i] = hint_roots[i] = -1;
    assert(sc64_route_cells_connected(starts, 1, goals, 1, other, 1,
                                      hint_slots, hint_roots, &hint_next, 0) == 1);
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 5);
}
#endif

static void direct_waypoint_fixture(void)
{
    direct_waypoint_far_origin_and_keys();
    direct_waypoint_rechecks_and_rescans();
    direct_waypoint_miss_falls_back_to_region_bfs();
#if defined(SC64_REACHABLE_FORMATION) && defined(SC64_IMPORTED_MISSION) && \
    !defined(SC64_ROUTE_PARTIAL_FLOOD) && !defined(SC64_ROUTE_VISIT_GRID)
    direct_waypoint_is_not_a_component_hint();
#endif
}
#endif

static void setup_scene(int type, int raw)
{
    game_init();
    init_state();
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(visible, 0xff, sizeof(visible));
    memset(explored, 0xff, sizeof(explored));
    memset(path_walkable, 0xff, sizeof(path_walkable));
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = R_ZERG; race_player[ENEMY] = R_ZERG;
    race_mode = 1; race_human = R_ZERG;
    path_grid_valid = 1; target_lists_valid = 0; sc64_target_index_active = 0;
    sc64_routes_reset();
    weapons_level = armor_level = 0;
    screen = SCREEN_PLAY; won = lost = 0;
    attacker = spawn_unit(SIDE, type, 400, 400);
    victim = spawn_unit(ENEMY, U_HYDRALISK, 400, 240);
    assert(attacker >= 0 && victim >= 0);
    u[attacker].raw_type = raw; u[attacker].raw_owner = 0;
    u[victim].raw_type = 38; u[victim].raw_owner = 1;
    u[attacker].hp = u[attacker].maxhp = 500;
    u[victim].hp = u[victim].maxhp = 500;
    u[victim].shield = 0;
    /* An open target cell is isolated by terrain, with reachable firing
     * positions outside the ring. This used to retry the center forever. */
    for (int row = 14; row <= 16; ++row) for (int col = 24; col <= 26; ++col)
        SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 0);
    SC64_GRID_SET(path_walkable, 25 + 15 * MAP_COLS, 1);
    u[attacker].path_retry = u[attacker].path_timer = 0;
}

static void approach_and_fire(void)
{
    int nx, ny, fired = 0;
    setup_scene(U_HYDRALISK, 38);
    assert(!direct_path_clear(400, 400, 400, 240));
    assert(!path_next_tile(400, 400, 400, 240, &nx, &ny));
#ifdef SC64_ATTACK_RANGE_ROUTES
    /* Force the first timed combat attempt to plan both the ordinary center
     * route and the direct firing waypoint from a cold derived cache. */
    sc64_routes_reset();
    u32 route_builds_before = sc64_route_builds;
#endif
    for (int tick = 0; tick < 500; ++tick) {
        int x = u[attacker].x, y = u[attacker].y, hp = u[victim].hp;
        int distance = dist2(x, y, u[victim].x, u[victim].y);
        tick_combat(&u[attacker], victim);
        assert(abs(u[attacker].x - x) <= 4 && abs(u[attacker].y - y) <= 4);
#ifdef SC64_ATTACK_RANGE_ROUTES
        if (tick == 0) {
            assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 5);
            assert(sc64_route_builds == route_builds_before + 1u);
            assert(u[attacker].path_x != u[victim].x ||
                   u[attacker].path_y != u[victim].y);
            assert_in_weapon_band(u[attacker].path_x, u[attacker].path_y,
                                  u[victim].x, u[victim].y, 0, 64);
            assert(direct_path_clear(x, y, u[attacker].path_x,
                                     u[attacker].path_y));
        }
#endif
        if (u[victim].hp != hp) {
            /* A Hydra's unupgraded 128-source-pixel reach stays 64 here. */
            assert(distance <= 64 * 64);
            fired = 1; break;
        }
        if (distance > 64 * 64) assert(u[victim].hp == 500);
    }
#ifdef SC64_ATTACK_RANGE_ROUTES
    assert(fired);
    /* Stock explosive 10 versus a medium Hydralisk takes the 75% factor. */
    assert(u[victim].hp == 493);
#else
    assert(!fired && u[attacker].x == 400 && u[attacker].y == 400);
#endif
}

static void preserve_other_movement(void)
{
    setup_scene(U_ZERGLING, 37);
    for (int tick = 0; tick < 100; ++tick) tick_combat(&u[attacker], victim);
    assert(u[victim].hp == 500);
    assert(u[attacker].x == 400 && u[attacker].y == 400);
#ifdef SC64_ATTACK_RANGE_ROUTES
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 0);
#endif

    setup_scene(U_HYDRALISK, 38);
    /* Explicit MOVE toward that same unreachable center is still a MOVE;
     * the combat fallback does not silently substitute a firing position. */
    for (int tick = 0; tick < 100; ++tick) move_toward(&u[attacker], 400, 240);
    assert(u[attacker].x == 400 && u[attacker].y == 400);
    assert(u[victim].hp == 500);
#ifdef SC64_ATTACK_RANGE_ROUTES
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 0);
#endif
}

#ifdef SC64_DIRECT_ATTACK
static void fog_pursuit_uses_last_seen_only(void)
{
    setup_scene(U_HYDRALISK, 38);
    u[attacker].selected = 1;
    assert(sc64_direct_attack_issue_target(victim) == 1);
    SC64_GRID_CLEAR(visible);
    u[victim].x = 800; u[victim].y = 600;
    for (int tick = 0; tick < 100; ++tick) sc64_direct_attack_tick_unit(attacker);
    assert(u[attacker].tx == 400 && u[attacker].ty == 240);
    assert(u[victim].hp == 500);
#ifdef SC64_ATTACK_RANGE_ROUTES
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 0);
#endif
}
#endif

int main(void)
{
    gba_init();
#ifdef SC64_ATTACK_RANGE_ROUTES
    direct_waypoint_fixture();
#endif
    approach_and_fire();
    preserve_other_movement();
#ifdef SC64_DIRECT_ATTACK
    fog_pursuit_uses_last_seen_only();
#endif
    puts("Attack-range actual movement/combat regression passed");
    return 0;
}
