/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#ifndef SC64_SHARED_CARRIER_EXPERIMENT
#error "This fixture exercises the gated shared carrier backend"
#endif
static int children(int carrier, unsigned state)
{
    int count=0;
    for (int i=0;i<MAX_UNITS;++i)
        count += u[i].alive && u[i].type==SC64_INTERCEPTOR_TYPE &&
                 u[i].build==carrier && sc64_p7_carrier_child_state(&u[i])==state;
    return count;
}
int main(void)
{
    game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1; race_skirmish_map=0;
    race_skirmish_init(); screen=SCREEN_PLAY;
    int carrier=spawn_unit(SIDE,SC64_CARRIER_TYPE,600,600);
    assert(carrier>=0); minerals=25;
    assert(race_p7_carrier_training_queue_owner(carrier,SIDE));
#ifndef SC64_P7_PROJECTILES_ENABLED
    assert(!race_save_game());
#endif
    for (int i=0;i<430 && screen==SCREEN_PLAY;++i) game_update(0,0);
    assert(children(carrier,SC64_P7_INTERCEPTOR_INSIDE)==1);
    int prey=spawn_unit(ENEMY,U_ZERGLING,660,600); assert(prey>=0);
    int hp=u[prey].hp, launched=0;
    for (int i=0;i<400 && screen==SCREEN_PLAY && u[prey].alive;++i) {
        game_update(0,0);
        if (children(carrier,SC64_P7_INTERCEPTOR_OUTSIDE)) launched=1;
    }
    assert(launched && (!u[prey].alive || u[prey].hp<hp));
    int doomed=spawn_unit(SIDE,SC64_CARRIER_TYPE,700,700);
    int victim=spawn_unit(ENEMY,U_ZERGLING,790,700);
    assert(doomed>=0 && victim>=0 && race_p7_carrier_training_stock(doomed,1)==1);
    for (int i=0;i<80 && !children(doomed,SC64_P7_INTERCEPTOR_OUTSIDE);++i)
        game_update(0,0);
    int child=-1;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].type==SC64_INTERCEPTOR_TYPE && u[i].build==doomed &&
            sc64_p7_carrier_child_state(&u[i])==SC64_P7_INTERCEPTOR_OUTSIDE) child=i;
    assert(child>=0);
    hit_target(doomed,10000,U_MARINE,ENEMY);
    assert(!u[doomed].alive && u[child].alive && u[child].build==-1 &&
           sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_DETACHED);
    /* Each custom-game opponent uses its own wallet and actor binding. */
    for (int owner=1; owner<5; ++owner) {
        if (owner==SIDE) continue;
        int bot=spawn_unit(owner,SC64_CARRIER_TYPE,500+owner*16,500);
        assert(bot>=0); race_minerals[owner]=25;
        assert(!race_p7_carrier_training_queue_owner(bot,SIDE));
        race_ai_plan(owner);
        assert(u[bot].carry==1);
        assert(race_minerals[owner]==0);
        assert(!race_p7_carrier_training_queue_owner(bot,owner));
        race_p7_carrier_training_source_tick();
        assert(children(bot,SC64_P7_INTERCEPTOR_TRAINING)==1);
    }
    puts("shared carrier experiment: ok"); return 0;
}
