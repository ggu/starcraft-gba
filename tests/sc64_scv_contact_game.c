/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual order/movement coverage for SCV stock weapon contact reach. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

int main(void)
{
    int worker, marine, initial_hp, ticks;

    gba_init();
    game_init();
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(path_walkable, 0xff, sizeof(path_walkable));
    path_grid_valid = 1;
    target_lists_valid = 0;
    sc64_target_index_active = 0;
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = R_TER;
    race_player[ENEMY] = R_TER;
    race_mode = 1;
    race_human = R_TER;
    race_selected_mission = 0;
    weapons_level = armor_level = 0;
    SC64_GRID_CLEAR(visible);

    worker = spawn_unit(SIDE, U_SCV, 100, 100);
    marine = spawn_unit(ENEMY, U_MARINE, 120, 100);
    assert(worker >= 0 && marine >= 0);
    u[worker].raw_type = 7;   /* Terran SCV. */
    u[worker].raw_owner = 0;
    u[marine].raw_type = 0;   /* Terran Marine. */
    u[marine].raw_owner = 1;
    u[marine].hp = u[marine].maxhp = 100;
    u[marine].shield = 0;
    /* The normal direct-order issuing path requires a currently visible target. */
    for (int y = 0; y < MAP_ROWS; ++y)
        for (int x = 0; x < MAP_COLS; ++x)
            SC64_GRID_SET(visible, x + y * MAP_COLS, 1);

    initial_hp = u[marine].hp;
    assert(sc64_direct_attack_issue_target(marine) == 0);
    u[worker].selected = 1;
    assert(sc64_direct_attack_issue_target(marine) == 1);
    assert(u[worker].order == ORDER_ATTACK_TARGET && u[worker].target == marine);

    for (ticks = 0; ticks < 180 && u[marine].hp == initial_hp; ++ticks) {
        ++frame;
        update_orders();
    }

    if (u[marine].hp == initial_hp) {
        fprintf(stderr, "SCV stalled at (%d,%d), center distance %d; no hit after %d ticks\n",
                u[worker].x, u[worker].y,
                abs(u[worker].x-u[marine].x)+abs(u[worker].y-u[marine].y), ticks);
    }
    assert(u[marine].hp < initial_hp);
    puts("SCV attack order reached Marine and dealt damage");
    return 0;
}
