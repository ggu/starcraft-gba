/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Deliberately malformed live coordinates exercise the actual executor in a
 * noncompact Unit build under integer-undefined-behavior sanitization. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "../starcraft/game.c"
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#error "fixture requires the noncompact Unit layout"
#endif
#ifndef SC64_SHARED_DEFILER_APPROACH_PROBE
#error "fixture requires the disabled approach probe"
#endif
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    int c=spawn_unit(SIDE,U_SHARED_DEFILER,160,160);assert(c>=0);
    const int bad[2]={INT_MAX,INT_MIN};
    for(int axis=0;axis<2;axis++)for(int k=0;k<2;k++) {
        u[c].x=160;u[c].y=160;
        if(axis)u[c].y=bad[k];else u[c].x=bad[k];
        SC64_DEFILER_ACTOR(c).target_x=300;SC64_DEFILER_ACTOR(c).target_y=300;
        SC64_DEFILER_ACTOR(c).intent=1;SC64_DEFILER_ACTOR(c).phase=0;
        if(!sc64_battlecruiser_source_tick_due())++frame;
        sc64_defiler_executor_unit_tick();
        assert(!SC64_DEFILER_ACTOR(c).intent && !SC64_DEFILER_ACTOR(c).phase &&
               !sc64_shared_projectiles.active);
    }
    u[c].x=160;u[c].y=160;SC64_DEFILER_ACTOR(c).target_x=-1;
    SC64_DEFILER_ACTOR(c).intent=1;SC64_DEFILER_ACTOR(c).phase=0;
    sc64_defiler_executor_unit_tick();assert(!SC64_DEFILER_ACTOR(c).intent);
    puts("noncompact malformed Defiler coordinates fail before arithmetic");return 0;
}
