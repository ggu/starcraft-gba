/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Current controller ownership through the non-cached attack/detector path. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#if !defined(SC64_SOURCE_SIGHT_ALL_RACES) || defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED) || !defined(SC64_P7_RUNTIME)
#error "requires P7 shared source sight without the cloak cache"
#endif
int main(void)
{
    gba_init();game_init();race_mode=0;race_selected_mission=SC64_P7_GLOBAL_ID;
    memset(u,0,sizeof u);memset(b,0,sizeof b);SC64_GRID_CLEAR(visible);
    u[0].alive=1;u[0].type=U_P7_OBSERVER;u[0].raw_type=84;u[0].owner=ENEMY;u[0].raw_owner=0;u[0].x=u[0].y=520;
    u[1].alive=1;u[1].type=U_P7_OBSERVER;u[1].raw_type=84;u[1].owner=SIDE;u[1].raw_owner=0;u[1].x=u[1].y=520;
    int cell=32+32*MAP_COLS;SC64_GRID_SET(visible,cell,1);
    assert(sc64_p7_observer_can_attack_from(U_MARINE,0,SIDE,0));
    assert(sc64_p7_observer_can_attack_from(U_MARINE,0,SIDE,999));
    u[1].alive=0;assert(!sc64_p7_observer_can_attack_from(U_MARINE,0,SIDE,999));
    u[0].owner=SIDE;u[0].raw_owner=7;
    assert(!sc64_p7_observer_can_attack_from(U_MARINE,0,ENEMY,7));
    puts("non-cache captured controller and mismatched source sight: ok");return 0;
}
