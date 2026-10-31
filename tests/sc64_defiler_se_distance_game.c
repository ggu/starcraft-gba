/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main previous_defiler_test
#include "sc64_defiler_executor_game.c"
#undef main
#include <stdlib.h>
#ifndef SC64_SHARED_DEFILER_SE_APPROACH_PROBE
#error "requires the disabled southeast approach probe"
#endif
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    int distance=atoi(argv[1]);if(distance<280 || distance>448 || (distance&1))return 2;
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
#ifdef SC64_SHARED_DEFILER_SE_HERO_PROBE
    int c=spawn_unit(SIDE,U_SHARED_UNCLEAN_ONE,160,160);
#else
    int c=spawn_unit(SIDE,U_SHARED_DEFILER,160,160);
#endif
    assert(c>=0);
    test_phase=0;click_cast(c,160+distance/2,160+distance/2);
    int tick=1;
    while(tick<=120) {
        Sc64DefilerStageActor *a=&sc64_defiler_stage[c];
        printf("t=%d x=%d y=%d heading=%u phase=%u blocked=%u qx=%d qy=%d\n",tick,
               sc64_interceptor_integer_part(a->x_q8),sc64_interceptor_integer_part(a->y_q8),
               a->heading,a->phase,a->blocked,(int)a->x_q8,(int)a->y_q8);
        game_update(0,0);if(sc64_battlecruiser_source_tick_due())++tick;
    }
    assert(!sc64_defiler_stage[c].blocked);return 0;
}
