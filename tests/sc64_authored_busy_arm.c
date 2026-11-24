/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored crowd. After setup the normal update and draw entry points run. */
#define game_init busy_engine_init
#include "game.c"
#undef game_init

#ifndef SC64_BUSY_CAMERA_Y
#define SC64_BUSY_CAMERA_Y 512
#endif

volatile u32 busy_initial_count;
volatile u32 busy_setup_fault;

void game_init(void)
{
    const int goal = 76;
    busy_engine_init();
    skirmish_bots = 3;
    init_state();
    build_path_grid();

    int alive = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        alive += u[i].alive != 0;
    for (int y = 360; y <= 664 && alive < goal; y += 20) {
        for (int x = 360; x <= 664 && alive < goal; x += 20) {
            if (!is_traversable(x / 16, y / 16, -1)) continue;
            int clear = 1;
            for (int i = 0; i < MAX_UNITS; ++i) {
                if (u[i].alive && dist2(x, y, u[i].x, u[i].y) < 18 * 18) {
                    clear = 0;
                    break;
                }
            }
            if (!clear) continue;
            int owner_index = alive % 4;
            int owner = owner_index ? skirmish_owner_ids[owner_index - 1] : SIDE;
            int slot = spawn_unit(owner, U_MARINE, x, y);
            if (slot < 0) {
                busy_setup_fault = 1;
                return;
            }
            u[slot].order = ORDER_ATTACK_MOVE;
            u[slot].tx = x < 512 ? 700 : 324;
            u[slot].ty = y < 512 ? 700 : 324;
            ++alive;
        }
    }
    busy_initial_count = (u32)alive;
    busy_setup_fault = alive != goal;
    cx = 512;
    cy = SC64_BUSY_CAMERA_Y;
}
