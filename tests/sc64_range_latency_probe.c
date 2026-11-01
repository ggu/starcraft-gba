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
      assert(!direct_path_clear(u[a[0]].x, u[a[0]].y, u[t[0]].x, u[t[0]].y)); }
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
            if (!competing && first_hit[i] < 0) {
                assert(u[a[i]].alive && u[t[i]].alive);
                assert(u[a[i]].order == ORDER_ATTACK_TARGET && u[a[i]].target == t[i]);
            }
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
static void contention_scene(void);
int main(void)
{
    scenario("reachable", 0, 0, 0);
    scenario("sealed", 1, 0, 0);
    contention_scene();
    return 0;
}

/* Two separate stationary building targets and two player-issued orders.
 * This scene is separate from the original scripted moving-target probe. */
static void contention_scene(void)
{
    int attackers[2], targets[2], buildings[2], first_move[2] = {-1,-1};
    int first_hit[2] = {-1,-1}, first_drop[2] = {-1,-1};
    int start_hp[2], active_mask = 0, queued_mask = 0, simultaneous = 0;
    unsigned worker_ticks = 0, worker_units = 0;
    setup_combat_scene();
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    assert(spawn_build(SIDE, B_COMMAND, 100, 100, 0) >= 0);
    for (int i = 0; i < 2; ++i) {
        int y = (i ? 44 : 20) * 16 + 8;
        buildings[i] = spawn_build(ENEMY, B_COMMAND, 40 * 16 + 8, y, 0);
        attackers[i] = spawn_unit(SIDE, U_TANK, 20 * 16 + 8, y);
        /* A stationary worker provides sight without an idle combat order. */
        assert(spawn_unit(SIDE, U_DRONE, 43 * 16 + 8, y) >= 0);
        assert(buildings[i] >= 0 && attackers[i] >= 0);
        targets[i] = MAX_UNITS + buildings[i];
        u[attackers[i]].hp = u[attackers[i]].maxhp = 65535;
        b[buildings[i]].hp = b[buildings[i]].maxhp = 65535;
        start_hp[i] = b[buildings[i]].hp;
    }
    clear_walk_grid();
    for (int row = 0; row < MAP_ROWS; ++row)
        if (row != 10 && row != 54) SC64_GRID_SET(path_walkable, 30 + row * MAP_COLS, 0);
    memset(visible, 0xff, sizeof(visible));
    for (int i = 0; i < 2; ++i) {
        assert(currently_visible(target_x(targets[i]), target_y(targets[i])));
        assert(!direct_path_clear(u[attackers[i]].x, u[attackers[i]].y,
            target_x(targets[i]), target_y(targets[i])));
        u[attackers[i]].selected = 1;
        assert(sc64_direct_attack_issue_target(targets[i]) == 1);
        u[attackers[i]].selected = 0;
    }
    sc64_routes_reset(); sc64_direct_firing_cache_invalidate();
#ifdef SC64_RANGE_WORK
    sc64_range_route_reset();
#endif
    for (int tick = 1; tick <= 1000; ++tick) {
        int x[2], y[2];
        for (int i = 0; i < 2; ++i) { x[i] = u[attackers[i]].x; y[i] = u[attackers[i]].y; }
        game_update(0, 0);
        assert(screen == SCREEN_PLAY && frame == tick);
        assert(path_grid_valid && !SC64_GRID_GET(path_walkable, 30 + 20 * MAP_COLS));
        for (int i = 0; i < 2; ++i) {
            assert(u[attackers[i]].alive && b[buildings[i]].alive);
            if (first_drop[i] < 0 && (u[attackers[i]].order != ORDER_ATTACK_TARGET ||
                                     u[attackers[i]].target != targets[i])) first_drop[i] = tick;
            if (first_move[i] < 0 && (x[i] != u[attackers[i]].x || y[i] != u[attackers[i]].y)) first_move[i] = tick;
            if (first_hit[i] < 0 && b[buildings[i]].hp < start_hp[i]) first_hit[i] = tick;
        }
#ifdef SC64_RANGE_WORK
        int present = 0;
        if (sc64_range_route_last_units) ++worker_ticks;
        worker_units += sc64_range_route_last_units;
        assert(sc64_range_route_last_units <= SC64_RANGE_WORK_BUDGET_PER_TICK);
        for (int i = 0; i < 2; ++i) {
            if (sc64_range_route_active.valid && sc64_range_route_active.target == targets[i])
                active_mask |= 1 << i, present |= 1 << i;
            for (int j = 0; j < SC64_RANGE_ROUTE_QUEUE_CAPACITY; ++j)
                if (sc64_range_route_pending[j].valid && sc64_range_route_pending[j].target == targets[i])
                    queued_mask |= 1 << i, present |= 1 << i;
        }
        if (present == 3) ++simultaneous;
#endif
    }
    printf("contention active_mask=%d queued_mask=%d simultaneous=%d worker_ticks=%u worker_units=%u", active_mask, queued_mask, simultaneous, worker_ticks, worker_units);
    for (int i = 0; i < 2; ++i)
        printf(" attacker%d first_move=%d first_hit=%d first_drop=%d target_hp=%d visible=%d", i, first_move[i], first_hit[i], first_drop[i], b[buildings[i]].hp, currently_visible(target_x(targets[i]), target_y(targets[i])));
    puts("");
    fflush(stdout);
    for (int i = 0; i < 2; ++i) assert(first_drop[i] == -1);
    /* Fast corridor witnesses may avoid queuing entirely; both attacks must
     * still complete while each original order stays valid. */
    assert(first_move[0] > 0 && first_move[1] > 0);
    assert(first_hit[0] > 0 && first_hit[1] > 0);
}
