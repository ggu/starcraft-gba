/* SPDX-License-Identifier: GPL-3.0-or-later
 * A Move ends on its point (StarCraft 64: every parity move-lab leg ends at
 * 0 px), and a mover that cannot reach the exact point still ends, through
 * sc64_move_close(), instead of circling. Synthetic Z7 actors.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_RACES) || !defined(SC64_IMPORTED_MISSION)
#error "Compile with the Z7 race profile"
#endif

enum { LIMIT = 3000 };

static void setup(void)
{
    game_init();
    race_mode = 0;
    race_human = R_ZERG;
    race_selected_mission = SC64_Z7_GLOBAL_ID;
    race_cols = SC64_Z7_MAP_COLS;
    race_rows = SC64_Z7_MAP_ROWS;
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    selected_building = -1;
    mode = MODE_NORMAL;
    menu_open = 0;
    screen = SCREEN_PLAY;
    frame = 0;
    /* Keep the mission objective live while using a synthetic actor set. */
    assert(spawn_build(ENEMY, B_HATCHERY, 1904, 1904, 0) >= 0);
    invalidate_path_grid();
}

static int route(int unit, int x, int y)
{
    int nx, ny;
    return path_next_tile(u[unit].x, u[unit].y, x, y, &nx, &ny);
}

static void order_one(int unit, int x, int y)
{
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    u[unit].selected = 1;
    assert(issue_order(ORDER_MOVE, x, y) > 0);
    u[unit].selected = 0;
}

/* Runs until every listed unit's Move ends; returns the updates taken. Each
 * unit, once within SC64_MOVE_CLOSE px of its point, must end within a
 * bounded number of updates and never leave that window: no circling. */
static int run_to_end(const int *units, int count)
{
    int inside[8] = {0};
    for (int t = 0; t < LIMIT; ++t) {
        int moving = 0;
        game_update(0, 0);
        assert(race_z7.runtime.fault == 0);
        for (int k = 0; k < count; ++k) {
            const Unit *q = &u[units[k]];
            assert(q->alive);
            if (q->order != ORDER_MOVE) continue;
            ++moving;
            int d = dist2(q->x, q->y, q->tx, q->ty);
            if (d <= SC64_MOVE_CLOSE * SC64_MOVE_CLOSE) {
                /* The window is entered once and left only by ending. */
                ++inside[k];
                assert(inside[k] <= 120);
            } else {
                assert(!inside[k]);
            }
        }
        if (!moving) return t + 1;
    }
    for (int k = 0; k < count; ++k) {
        const Unit *q = &u[units[k]];
        fprintf(stderr, "stuck unit %d at %d,%d point %d,%d order %d retry %d timer %d path %d,%d\n", units[k], q->x, q->y,
                q->tx, q->ty, q->order, q->path_retry, q->path_timer, q->path_x, q->path_y);
    }
    assert(!"a Move never ended");
    return LIMIT;
}

static void check_open_ground_ends_on_the_point(void)
{
    setup();
    int a = spawn_unit(SIDE, U_HYDRALISK, 1623, 937);
    assert(a >= 0);
    assert(route(a, 1560, 900));
    order_one(a, 1560, 900);
    int t = run_to_end(&a, 1);
    assert(u[a].x == 1560 && u[a].y == 900 && u[a].order == ORDER_IDLE);
    printf("open ground: on the point after %d updates\n", t);
}

static void check_blocked_tile_ends_beside_it(void)
{
    setup();
    int a = spawn_unit(SIDE, U_HYDRALISK, 1623, 937);
    assert(a >= 0);
    /* The nearest blocked tile with a walkable, reachable neighbour. */
    int bx = -1, by = -1, best = 0x7fffffff;
    for (int row = 1; row + 1 < MAP_ROWS; ++row)
        for (int col = 1; col + 1 < MAP_COLS; ++col) {
            if (!terrain_blocked(col, row)) continue;
            int px = col * 16 + 8, py = row * 16 + 8, d = dist2(px, py, u[a].x, u[a].y);
            if (d >= best || d > 400 * 400) continue;
            if (terrain_blocked(col - 1, row) || !route(a, px - 16, py)) continue;
            best = d; bx = px; by = py;
        }
    assert(bx >= 0);
    order_one(a, bx, by);
    int t = run_to_end(&a, 1);
    assert(u[a].order == ORDER_IDLE && dist2(u[a].x, u[a].y, bx, by) <= 24 * 24);
    printf("blocked tile at %d,%d: ended at %d,%d after %d updates\n", bx, by, u[a].x, u[a].y, t);
}

static void check_crowd_on_one_point_ends(void)
{
    setup();
    int units[4];
    for (int k = 0; k < 4; ++k) {
        units[k] = spawn_unit(SIDE, U_HYDRALISK, 1623 + (k & 1) * 14, 937 + (k >> 1) * 14);
        assert(units[k] >= 0);
    }
    /* Separate orders to one point: they cannot all stand on it. */
    for (int k = 0; k < 4; ++k) order_one(units[k], 1560, 900);
    int t = run_to_end(units, 4);
    int on_point = 0;
    for (int k = 0; k < 4; ++k) {
        assert(u[units[k]].order == ORDER_IDLE);
        assert(dist2(u[units[k]].x, u[units[k]].y, 1560, 900) <= 32 * 32);
        on_point += u[units[k]].x == 1560 && u[units[k]].y == 900;
    }
    /* At most one stands on the point; a later arrival may push the first,
     * now idle, aside. */
    assert(on_point <= 1);
    printf("crowd of 4: all ended after %d updates, %d on the point\n", t, on_point);
}

int main(void)
{
    check_open_ground_ends_on_the_point();
    check_blocked_tile_ends_beside_it();
    check_crowd_on_one_point_ends();
    puts("exact Move arrival checks passed (synthetic Z7 actors)");
    return 0;
}
