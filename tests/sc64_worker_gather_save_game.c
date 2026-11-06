/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real journal round-trip of an in-progress authored resource job. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
int main(void)
{
    game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1;
    race_bot_races[0]=R_TER; race_skirmish_map=0; race_skirmish_init();
    int w=-1,p=-1;
    for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].owner==SIDE && IS_WORKER(u[i].type)) {w=i;break;}
    for(int i=0;i<MAX_MINERALS;i++) if(m[i].amount>0) {p=i;break;}
    assert(w>=0 && p>=0);
    u[w].order=ORDER_MINE; u[w].target=p; u[w].carry=0; u[w].work_timer=23;
    u[w].build=-1; sc64_worker_gather_pending_set(&u[w],0);
    assert(race_save_game());
    u[w].work_timer=0; u[w].order=ORDER_IDLE;
    assert(race_load_game());
    assert(u[w].alive && u[w].order==ORDER_MINE && u[w].target==p && u[w].work_timer==23);
    puts("worker pending gather journal round trip: ok");
    return 0;
}
