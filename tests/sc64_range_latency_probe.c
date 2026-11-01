/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual game-update routing latency probe. Run both with and without
 * SC64_RANGE_WORK; output is game ticks, not a CPU performance measurement. */
#define main firing_fixture_main
#include "sc64_direct_firing_points_game.c"
#undef main

static void scenario(const char *name, int sealed, int competing, int moving)
{
    int a[2], t[2], first_move[2] = {-1,-1}, first_hit[2] = {-1,-1};
    int count = competing ? 2 : 1;
    unsigned worker_ticks = 0, worker_units = 0;
    int point_route;
    setup_combat_scene();
    memset(u, 0, sizeof(u));
    assert(spawn_build(SIDE, B_COMMAND, 100, 100, 0) >= 0);
    assert(spawn_build(ENEMY, B_COMMAND, 900, 100, 0) >= 0);
    clear_walk_grid();
    for (int row = 0; row < MAP_ROWS; ++row)
        if (sealed || row != 20) SC64_GRID_SET(path_walkable, 24 + row * MAP_COLS, 0);
    for (int i = 0; i < count; ++i) {
        int y = (40 + i * 5) * 16 + 8;
        a[i] = spawn_unit(SIDE, U_TANK, 22 * 16 + 8, y);
        t[i] = spawn_unit(ENEMY, U_HYDRALISK, 32 * 16 + 8, y);
        assert(a[i] >= 0 && t[i] >= 0);
        u[a[i]].order = ORDER_ATTACK_TARGET; u[a[i]].target = t[i];
        u[t[i]].hp = u[t[i]].maxhp = 65535;
    }
    memset(visible, 0xff, sizeof(visible));
    { int nx, ny; point_route = path_next_tile(u[a[0]].x, u[a[0]].y, u[t[0]].x, u[t[0]].y, &nx, &ny);
      assert(!sc64_direct_firing_waypoint(u[a[0]].x, u[a[0]].y, u[t[0]].x, u[t[0]].y, 0, unit_range(U_TANK), &nx, &ny)); }
    sc64_routes_reset();
    sc64_direct_firing_cache_invalidate();
#ifdef SC64_RANGE_WORK
    sc64_range_route_reset();
#endif
    for (int tick = 1; tick <= 1000; ++tick) {
        int x[2], y[2], hp[2];
        for (int i = 0; i < count; ++i) {
            x[i] = u[a[i]].x; y[i] = u[a[i]].y; hp[i] = u[t[i]].hp;
        }
        for (int i = 0; i < count; ++i) {
            u[t[i]].x = (32 - (moving && i == 0 && ((tick / 14) & 1))) * 16 + 8;
            u[t[i]].y = (40 + i * 5) * 16 + 8;
            u[t[i]].order = ORDER_IDLE;
        }
        game_update(0, 0);
        assert(screen == SCREEN_PLAY && frame == tick);
        assert(path_grid_valid && !SC64_GRID_GET(path_walkable, 24 + 40 * MAP_COLS));
#ifdef SC64_RANGE_WORK
        if (sc64_range_route_last_units) ++worker_ticks;
        worker_units += sc64_range_route_last_units;
        assert(sc64_range_route_last_units <= SC64_RANGE_WORK_BUDGET_PER_TICK);
#endif
        for (int i = 0; i < count; ++i) {
            if (first_move[i] < 0 && (x[i] != u[a[i]].x || y[i] != u[a[i]].y)) first_move[i] = tick;
            if (first_hit[i] < 0 && u[t[i]].hp < hp[i]) first_hit[i] = tick;
        }
        if (moving && tick % 14 == 0)
            u[t[0]].x = ((tick / 14) & 1 ? 31 : 32) * 16 + 8;
    }
    printf("%s point_route=%d worker_ticks=%u worker_units=%u", name, point_route, worker_ticks, worker_units);
    for (int i = 0; i < count; ++i)
        printf(" attacker%d first_move=%d first_hit=%d end=(%d,%d) target_hp=%d", i, first_move[i], first_hit[i], u[a[i]].x, u[a[i]].y, u[t[i]].hp);
    printf(" screen=%d frame=%d\n", screen, frame);
}
int main(void)
{
    scenario("reachable", 0, 0, 0);
    scenario("sealed", 1, 0, 0);
    scenario("competing_moving", 0, 1, 1);
    return 0;
}
