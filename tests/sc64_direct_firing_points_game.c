/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual-engine tests for the bounded direct firing-corridor fallback. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int attacker, victim;
static void setup_combat_scene(void);

static void clear_walk_grid(void)
{
    memset(path_walkable, 0xff, sizeof(path_walkable));
    path_grid_valid = 1;
#ifdef SC64_DIRECT_FIRING_POINTS
    sc64_direct_firing_cache_invalidate();
#endif
}

static void block_center_ring(int tx, int ty)
{
    int col = tx / 16, row = ty / 16;
    for (int y = row - 1; y <= row + 1; ++y)
        for (int x = col - 1; x <= col + 1; ++x)
            SC64_GRID_SET(path_walkable, x + y * MAP_COLS, 0);
    SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 1);
}

#ifdef SC64_DIRECT_FIRING_POINTS
static void assert_band(int x, int y, int tx, int ty, int minimum, int maximum)
{
    int d = dist2(x, y, tx, ty);
    assert(d >= minimum * minimum && d <= maximum * maximum);
    assert(x >= 8 && y >= 8 && x < MAP_COLS * 16 && y < MAP_ROWS * 16);
    assert(SC64_GRID_GET(path_walkable, x / 16 + (y / 16) * MAP_COLS));
}

static void test_bounded_search_key_and_origin_revalidation(void)
{
    const int tx = 512, ty = 400, sx = 32, sy = 400;
    int nx, ny, oldx;
    clear_walk_grid();
    block_center_ring(tx, ty);
    assert(!path_next_tile(sx, sy, tx, ty, &nx, &ny));
    assert(sc64_direct_firing_waypoint(sx, sy, tx, ty, 0, 210, &nx, &ny));
    assert_band(nx, ny, tx, ty, 0, 210);
    assert(direct_path_clear(sx, sy, nx, ny));
    assert(sc64_direct_firing_cache.valid);
    oldx = sc64_direct_firing_cache.x;
    assert(sc64_direct_firing_waypoint(sx + 16, sy, tx, ty, 0, 210, &nx, &ny));
    assert(nx == oldx && direct_path_clear(sx + 16, sy, nx, ny));

    /* All key terms are exact and min/max use inclusive squared distance. */
    assert(sc64_direct_firing_waypoint(sx, sy, tx + 1, ty, 0, 210, &nx, &ny));
    assert(sc64_direct_firing_cache.tx == tx + 1);
    assert(sc64_direct_firing_waypoint(sx, sy, tx + 1, ty, 50, 210, &nx, &ny));
    assert(sc64_direct_firing_cache.minimum == 50 &&
           sc64_direct_firing_cache.maximum == 210);
    assert_band(nx, ny, tx + 1, ty, 50, 210);
    assert(sc64_direct_firing_waypoint(sx, sy, tx + 1, ty, 50, 200, &nx, &ny));
    assert(sc64_direct_firing_cache.maximum == 200);
    assert_band(nx, ny, tx + 1, ty, 50, 200);
    clear_walk_grid();
    {
        const int bx = 50 * 16 + 8, by = 25 * 16 + 8;
        assert(sc64_direct_firing_waypoint(bx, by, bx, by, 16, 64, &nx, &ny));
        assert(dist2(nx, ny, bx, by) == 16 * 16);
        assert(sc64_direct_firing_waypoint(bx, by, bx, by, 64, 64, &nx, &ny));
        assert(dist2(nx, ny, bx, by) == 64 * 64);
    }

    /* A cached waypoint across a wall is rejected from this origin and the
     * bounded candidate scan finds a reachable point on the other side. */
    clear_walk_grid();
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 32 + row * MAP_COLS, 0);
    assert(sc64_direct_firing_waypoint(32, ty, tx, ty, 0, 210, &nx, &ny));
    assert(nx < tx);
    assert(sc64_direct_firing_waypoint(992, ty, tx, ty, 0, 210, &nx, &ny));
    assert(nx > tx && direct_path_clear(992, ty, nx, ny));
    assert_band(nx, ny, tx, ty, 0, 210);

    invalidate_path_grid();
    assert(!sc64_direct_firing_cache.valid);
    assert(sizeof(sc64_direct_firing_cache) == 14);
    assert(!sc64_direct_firing_waypoint(sx, sy, tx, ty, 0, 257, &nx, &ny));
}

static void test_one_corner_paths_cache_final_firing_tile(void)
{
    const int sx = 400, sy = 400, tx = 520, ty = 400;
    int nx, ny, final_x, final_y;

    /* A local vertical wall blocks every direct final-point corridor. Its
     * opening is north of the actor; a second wall leaves the target center
     * disconnected while the in-band firing tile stays on the actor's side. */
    clear_walk_grid();
    block_center_ring(tx, ty);
    for (int row = 0; row < MAP_ROWS; ++row)
        if (row != 22) SC64_GRID_SET(path_walkable, 26 + row * MAP_COLS, 0);
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 30 + row * MAP_COLS, 0);
    assert(!direct_path_clear(sx, sy, tx, ty));
    assert(!path_next_tile(sx, sy, tx, ty, &nx, &ny));
    assert(!direct_path_clear(sx, sy, 472, 360));
    assert(sc64_direct_firing_waypoint(sx, sy, tx, ty, 0, 64, &nx, &ny));
    final_x = sc64_direct_firing_cache.x;
    final_y = sc64_direct_firing_cache.y;
    assert_band(final_x, final_y, tx, ty, 0, 64);
    assert(nx == sx && ny != sy); /* vertical-first elbow, not final tile */
    assert(dist2(nx, ny, tx, ty) > 64 * 64);
    assert(direct_path_clear(sx, sy, nx, ny));
    assert(direct_path_clear(nx, ny, final_x, final_y));
    assert(!direct_path_clear(sx, sy, final_x, final_y));
    assert(sc64_direct_firing_waypoint(sx + 8, sy, tx, ty, 0, 64, &nx, &ny));
    assert(sc64_direct_firing_cache.x == final_x &&
           sc64_direct_firing_cache.y == final_y);
    assert(direct_path_clear(sx + 8, sy, nx, ny));
    assert(direct_path_clear(nx, ny, final_x, final_y));

    /* A horizontal local wall requires the horizontal-first elbow. A second
     * wall keeps the target center unreachable, like the vertical case. */
    clear_walk_grid();
    block_center_ring(520, 520);
    for (int col = 0; col < MAP_COLS; ++col)
        if (col != 30) SC64_GRID_SET(path_walkable, col + 26 * MAP_COLS, 0);
    for (int col = 0; col < MAP_COLS; ++col)
        SC64_GRID_SET(path_walkable, col + 30 * MAP_COLS, 0);
    assert(!path_next_tile(sx, sy, 520, 520, &nx, &ny));
    assert(sc64_direct_firing_waypoint(sx, sy, 520, 520, 0, 64, &nx, &ny));
    final_x = sc64_direct_firing_cache.x;
    final_y = sc64_direct_firing_cache.y;
    assert_band(final_x, final_y, 520, 520, 0, 64);
    assert(nx != sx && ny == sy); /* horizontal-first elbow, not final tile */
    assert(dist2(nx, ny, 520, 520) > 64 * 64);
    assert(direct_path_clear(sx, sy, nx, ny));
    assert(direct_path_clear(nx, ny, final_x, final_y));
    assert(!direct_path_clear(sx, sy, final_x, final_y));
    assert(sc64_direct_firing_waypoint(sx + 8, sy, 520, 520, 0, 64, &nx, &ny));
    assert(sc64_direct_firing_cache.x == final_x &&
           sc64_direct_firing_cache.y == final_y);
    assert(direct_path_clear(sx + 8, sy, nx, ny));
    assert(direct_path_clear(nx, ny, final_x, final_y));
}

static void test_obstructed_candidates_keep_baseline_retry(void)
{
    int nx, ny;
    setup_combat_scene();
    u[attacker].x = 32; u[attacker].y = 400;
    u[victim].x = 512; u[victim].y = 400;
    clear_walk_grid();
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 12 + row * MAP_COLS, 0);
    assert(!path_next_tile(u[attacker].x, u[attacker].y,
                           u[victim].x, u[victim].y, &nx, &ny));
    assert(!sc64_direct_firing_waypoint(u[attacker].x, u[attacker].y,
                                        u[victim].x, u[victim].y,
                                        0, 64, &nx, &ny));
    assert(!sc64_direct_firing_cache.valid);
    tick_combat(&u[attacker], victim);
    assert(u[attacker].path_retry == 12 && u[attacker].path_timer == 0);
}

static void test_save_load_invalidates_cache(void)
{
    int nx, ny;
    game_init();
    race_mode = 1; race_human = R_ZERG; race_bots = 1; race_skirmish_map = 0;
    race_skirmish_init();
    clear_walk_grid();
    assert(sc64_direct_firing_waypoint(200, 200, 300, 200, 0, 64, &nx, &ny));
    assert(race_save_game());
    assert(sc64_direct_firing_cache.valid);
    assert(race_load_game());
    assert(!sc64_direct_firing_cache.valid);
}
#endif

static void setup_combat_scene_type(int type)
{
    game_init();
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = race_player[ENEMY] = R_ZERG;
    race_mode = 1;
    race_human = R_ZERG;
    screen = SCREEN_PLAY;
    attacker = spawn_unit(SIDE, type, 400, 400);
    victim = spawn_unit(ENEMY, U_HYDRALISK, 400, 240);
    assert(attacker >= 0 && victim >= 0);
    u[attacker].raw_type = type == U_ZERGLING ? 37 : 38; u[attacker].raw_owner = 0;
    u[victim].raw_type = 38; u[victim].raw_owner = 1;
    u[attacker].hp = u[attacker].maxhp = 500;
    u[victim].hp = u[victim].maxhp = 500;
    clear_walk_grid();
    block_center_ring(400, 240);
}

static void setup_combat_scene(void)
{
    setup_combat_scene_type(U_HYDRALISK);
}

static void test_combat_hook_or_baseline(void)
{
    int nx, ny, fired = 0;
    setup_combat_scene();
    assert(!direct_path_clear(400, 400, 400, 240));
    assert(!path_next_tile(400, 400, 400, 240, &nx, &ny));
    tick_combat(&u[attacker], victim);
#ifdef SC64_DIRECT_FIRING_POINTS
    assert(sc64_direct_firing_cache.valid);
    assert(u[attacker].path_retry == 0 && u[attacker].path_timer == 24);
    assert(u[attacker].path_x != u[victim].x || u[attacker].path_y != u[victim].y);
    assert_band(u[attacker].path_x, u[attacker].path_y,
                u[victim].x, u[victim].y, 0, 64);
    assert(direct_path_clear(400, 400, u[attacker].path_x, u[attacker].path_y));
#else
    assert(u[attacker].path_retry == 12 && u[attacker].path_timer == 0);
#endif
    for (int tick = 0; tick < 500; ++tick) {
        int x = u[attacker].x, y = u[attacker].y;
        int hp = u[victim].hp;
        int distance = dist2(x, y, u[victim].x, u[victim].y);
        tick_combat(&u[attacker], victim);
        assert(abs(u[attacker].x - x) <= 4 && abs(u[attacker].y - y) <= 4);
        if (u[victim].hp != hp) {
            assert(distance <= 64 * 64);
            fired = 1; break;
        }
        if (distance > 64 * 64) assert(u[victim].hp == hp);
    }
#ifdef SC64_DIRECT_FIRING_POINTS
    assert(fired);
    assert(u[victim].hp == 493);
#else
    assert(!fired && u[attacker].x == 400 && u[attacker].y == 400);
#endif
}

static void test_one_corner_combat_moves_and_only_fires_in_range(void)
{
#ifdef SC64_DIRECT_FIRING_POINTS
    int fired = 0;
    int weapon_range = (sc64_race_unit_range_vs(U_HYDRALISK, 0) + 1) / 2;
    setup_combat_scene();
    u[attacker].x = 400; u[attacker].y = 400;
    u[victim].x = 520; u[victim].y = 400;
    clear_walk_grid();
    block_center_ring(u[victim].x, u[victim].y);
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 30 + row * MAP_COLS, 0);
    for (int row = 0; row < MAP_ROWS; ++row)
        if (row != 22) SC64_GRID_SET(path_walkable, 26 + row * MAP_COLS, 0);
    {
        int nx, ny;
        assert(!path_next_tile(u[attacker].x, u[attacker].y,
                               u[victim].x, u[victim].y, &nx, &ny));
        tick_combat(&u[attacker], victim);
        assert(sc64_direct_firing_cache.valid);
        assert(u[attacker].path_x == u[attacker].x &&
               u[attacker].path_y != u[attacker].y);
        assert(dist2(sc64_direct_firing_cache.x,
                     sc64_direct_firing_cache.y,
                     u[victim].x, u[victim].y) <= weapon_range * weapon_range);
    }
    for (int tick = 0; tick < 500; ++tick) {
        int x = u[attacker].x, y = u[attacker].y;
        int hp = u[victim].hp;
        int distance = dist2(x, y, u[victim].x, u[victim].y);
        tick_combat(&u[attacker], victim);
        assert(abs(u[attacker].x - x) <= 4 && abs(u[attacker].y - y) <= 4);
        if (u[victim].hp != hp) {
            assert(distance <= weapon_range * weapon_range);
            fired = 1;
            break;
        }
        if (distance > weapon_range * weapon_range) assert(u[victim].hp == hp);
    }
    assert(fired && u[victim].hp < 500);
#endif
}

static void test_melee_and_explicit_move_are_unchanged(void)
{
    setup_combat_scene_type(U_ZERGLING);
    for (int tick = 0; tick < 100; ++tick) tick_combat(&u[attacker], victim);
    assert(u[attacker].x == 400 && u[attacker].y == 400 && u[victim].hp == 500);
#ifdef SC64_DIRECT_FIRING_POINTS
    assert(!sc64_direct_firing_cache.valid);
#endif
    setup_combat_scene();
    for (int tick = 0; tick < 100; ++tick) move_toward(&u[attacker], 400, 240);
    assert(u[attacker].x == 400 && u[attacker].y == 400 && u[victim].hp == 500);
#ifdef SC64_DIRECT_FIRING_POINTS
    assert(!sc64_direct_firing_cache.valid);
#endif
}

int main(void)
{
    gba_init();
#ifdef SC64_DIRECT_FIRING_POINTS
    test_bounded_search_key_and_origin_revalidation();
    test_one_corner_paths_cache_final_firing_tile();
    test_obstructed_candidates_keep_baseline_retry();
#endif
    test_combat_hook_or_baseline();
    test_one_corner_combat_moves_and_only_fires_in_range();
    test_melee_and_explicit_move_are_unchanged();
#ifdef SC64_DIRECT_FIRING_POINTS
    setup_combat_scene();
    {
        int nx, ny;
        assert(sc64_direct_firing_waypoint(400, 400, 400, 240, 0, 64,
                                           &nx, &ny));
        u[attacker].path_x = nx; u[attacker].path_y = ny;
    }
    test_save_load_invalidates_cache();
#endif
    puts("Direct firing corridors actual-engine fixture passed");
    return 0;
}
