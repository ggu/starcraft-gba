/* SPDX-License-Identifier: GPL-3.0-or-later
 * Focused, synthetic Z7 input-path check for the opt-in group-slot fallback.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_RACES)
#error "Compile with the Z7 race profile"
#endif

static void tap(u16 key)
{
    game_update(key, key);
    assert(race_z7.runtime.fault == 0);
    game_update(0, 0);
    assert(race_z7.runtime.fault == 0);
}

static int route(int unit, int x, int y)
{
    int nx, ny;
    return path_next_tile(u[unit].x, u[unit].y, x, y, &nx, &ny);
}

static void setup_type(int count, int cursor_x, int cursor_y, int type)
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
    cx = cursor_x;
    cy = cursor_y;
    frame = 0;
    int first = spawn_unit(SIDE, type, 1623, 937);
    assert(first >= 0);
    if (count > 1) {
        int second = spawn_unit(SIDE, type, 1800, 1200);
        assert(second >= 0);
    }
    /* Keep the mission objective live while using a synthetic actor set. */
    assert(spawn_build(ENEMY, B_HATCHERY, 1904, 1904, 0) >= 0);
    assert(b[0].x >= 0 && b[0].x < WORLD_W && b[0].y >= 0 && b[0].y < WORLD_H);
    invalidate_path_grid();
    tap(KEY_R); /* ordinary select-combat-group input */
    assert(race_z7.runtime.fault == 0);
}

static void setup(int count, int cursor_x, int cursor_y)
{
    setup_type(count, cursor_x, cursor_y, U_HYDRALISK);
}

static void issue_attack_move(void)
{
    tap(KEY_B);       /* open field command menu */
    tap(KEY_DOWN);    /* ATTACK */
    assert(race_z7.runtime.fault == 0);
    tap(KEY_A);       /* enter attack mode */
    assert(race_z7.runtime.fault == 0);
    tap(KEY_A);       /* issue ATTACK_MOVE at the cursor */
    assert(race_z7.runtime.fault == 0);
}

static void check_group_slot_policy(void)
{
    setup(2, 1383, 774);
    assert(!route(0, 1367, 766)); /* first formation slot is statically isolated */
    assert(route(0, 1383, 774));  /* common cursor point is reachable */
    issue_attack_move();
    assert(u[0].order == ORDER_ATTACK_MOVE);
    assert(u[1].order == ORDER_ATTACK_MOVE);
 #ifdef SC64_REACHABLE_FORMATION
    assert(u[0].tx == 1383 && u[0].ty == 774);
    for (int tick = 0; tick < 4000; ++tick) {
        game_update(0, 0);
        assert(race_z7.runtime.fault == 0);
    }
    assert(!won && !lost && race_z7.runtime.fault == 0);
    assert(dist2(u[0].x, u[0].y, 1383, 774) < dist2(1623, 937, 1383, 774));
 #else
    assert(u[0].tx == 1367 && u[0].ty == 766);
 #endif
}

static void check_single_unit_is_unchanged(void)
{
    setup(1, 1383, 774);
    issue_attack_move();
    assert(u[0].order == ORDER_ATTACK_MOVE);
    assert(u[0].tx == 1383 && u[0].ty == 774);
}

static void check_flying_group_keeps_formation_slot(void)
{
    setup_type(2, 1383, 774, U_MUTALISK);
    assert(sc64_race_unit_flying(u[0].type));
    assert(!route(0, 1367, 766));
    issue_attack_move();
    assert(u[0].order == ORDER_ATTACK_MOVE);
    assert(u[0].tx == 1367 && u[0].ty == 766);
    assert(race_z7.runtime.fault == 0);
}

static void check_blocked_common_preserves_slot(void)
{
    setup(2, 0, 0);
    assert(!route(0, 8, 8));
    assert(!route(0, 0, 0));
    issue_attack_move();
    assert(u[0].order == ORDER_ATTACK_MOVE);
    /* Cursor is unreachable too, so the original clamped slot is retained. */
    assert(u[0].tx == 8 && u[0].ty == 8);
}

int main(void)
{
    check_group_slot_policy();
    check_single_unit_is_unchanged();
    check_flying_group_keeps_formation_slot();
    check_blocked_common_preserves_slot();
    puts("reachable formation input checks passed (synthetic Z7 actors)");
    return 0;
}
