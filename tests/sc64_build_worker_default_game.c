/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Automatic builder selection must preserve workers' active construction. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

int main(void)
{
    game_init();
    memset(u, 0, sizeof(u));
    int busy = spawn_unit(SIDE, U_SCV, 80, 80);
    int free_worker = spawn_unit(SIDE, U_SCV, 100, 80);
    assert(busy >= 0 && free_worker >= 0);
    u[busy].selected = 1;
    u[busy].order = ORDER_BUILD;
    u[busy].target = 3;
    cx = 80; cy = 80;
    assert(field_ui_build_worker() == free_worker);
    u[free_worker].selected = 1;
    assert(field_ui_build_worker() == free_worker);
    u[free_worker].alive = 0;
    assert(field_ui_build_worker() < 0);
    assert(!build_worker_available(busy, SIDE, -1));
    assert(u[busy].order == ORDER_BUILD && u[busy].target == 3);
    puts("default builder selection passed");
    return 0;
}
