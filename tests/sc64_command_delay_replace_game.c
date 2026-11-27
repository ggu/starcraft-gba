/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Command latency checks in the Terran product, on missions launched from
 * the ordinary menu with every update through game_update().
 *
 * A player command that replaces a pending (delayed) Move keeps its order
 * when the Move falls due.
 * The Dropship takes unit slot 22: with the earlier 8-bit order hash, an
 * idle Marine's boarding order for that slot hashed like its idle order,
 * so the pending Move landed and cancelled the boarding. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_TRANSPORT_RUNTIME) || !defined(SC64_IMPORTED_MISSION)
#error "Compile this fixture with the Terran product flags"
#endif

volatile u16 demo_keys;
static unsigned held_keys;

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}

static void press(unsigned key) { input(key); input(0); }

static void launch_t1(void)
{
    game_init();
    held_keys = 0;
    press(KEY_A);
    press(KEY_A);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && race_selected_mission == SC64_RACE_MISSION_T1);
    for (int k = 0; k < 30; ++k) input(0);
}

static int player_marine(int skip)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 0 &&
            u[i].order == ORDER_IDLE && skip-- == 0) return i;
    assert(!"no idle player Marine");
    return -1;
}

static void select_only(int a, int b)
{
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = i == a || i == b;
    selected_building = -1;
}

/* The player's Move through the field menu's Move mode, as a press of A. */
static void player_move(int x, int y)
{
    menu_open = 0;
    mode = MODE_MOVE;
    cx = x; cy = y;
    press(KEY_A);
    assert(mode == MODE_NORMAL);
}

int main(void)
{
    launch_t1();
    int marine = player_marine(0), witness = player_marine(1);

    /* Put a player Dropship in slot 22 (an idle Zergling far from the
     * player stands there in T1; spawn_unit reuses a dead slot). */
    assert(u[22].alive && u[22].owner != SIDE && !u[22].selected);
    u[22].alive = 0;
    int dropship = spawn_unit(SIDE, sc64_race_unit_from_raw(11), u[marine].x + 24, u[marine].y);
    assert(dropship == 22 && race_transport_carrier_valid(dropship));
    for (int k = 0; k < 4; ++k) input(0);

    /* Control: a pending Move with no newer command lands after the delay. */
    select_only(witness, -1);
    player_move(u[witness].x + 40, u[witness].y + 30);
    assert(u[witness].order == ORDER_IDLE);
    for (int k = 0; k < SC64_COMMAND_DELAY_UPDATES + 1 && u[witness].order == ORDER_IDLE; ++k) input(0);
    assert(u[witness].order == ORDER_MOVE);

    /* The Marine's Move is pending when the player boards it on the
     * Dropship: press A on the carrier in the field. */
    int move_x = u[marine].x + 60, move_y = u[marine].y + 50;
    select_only(marine, -1);
    player_move(move_x, move_y);
    assert(u[marine].order == ORDER_IDLE);
    assert(sc64_delayed_commands[0].active || sc64_delayed_commands[1].active);
    mode = MODE_NORMAL; menu_open = 0;
    cx = u[dropship].x; cy = u[dropship].y;
    press(KEY_A);
    assert(u[marine].order == ORDER_BOARD && u[marine].target == dropship);

    /* The Move was given two updates ago and falls due within the next
     * few. Run well past that update: the boarding stands. */
    for (int k = 0; k < 3 * SC64_COMMAND_DELAY_UPDATES; ++k) {
        input(0);
        assert(u[marine].order == ORDER_BOARD || u[marine].order == ORDER_TRANSPORTED);
        assert(u[marine].target == dropship);
        assert(u[marine].tx != move_x || u[marine].ty != move_y);
    }
    assert(!sc64_delayed_commands[0].active && !sc64_delayed_commands[1].active);
    puts("command delay: boarding replaces a pending Move: ok");
    return 0;
}
