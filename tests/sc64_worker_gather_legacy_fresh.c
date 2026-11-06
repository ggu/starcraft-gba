/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Separate old-writer/new-reader process compatibility for worker timers. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "game.c"
extern int sc64_test_sram_transfer(const char *,int);
int main(int argc, char **argv)
{
    assert(argc==3);
    game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1;
    race_bot_races[0]=R_TER; race_skirmish_map=0; race_skirmish_init();
    if (!strcmp(argv[1],"write")) {
        int worker=-1,patch=-1;
        for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].owner==SIDE && IS_WORKER(u[i].type)) {worker=i;break;}
        for(int i=0;i<MAX_MINERALS;i++) if(m[i].amount>0) {patch=i;break;}
        assert(worker>=0 && patch>=0);
        u[worker].order=ORDER_MINE; u[worker].target=patch; u[worker].carry=0;
        u[worker].work_timer=20; u[worker].build=-1;
        sc64_worker_gather_pending_set(&u[worker],0);
        assert(race_save_game() && sc64_test_sram_transfer(argv[2],1));
    } else {
        assert(!strcmp(argv[1],"read") && sc64_test_sram_transfer(argv[2],0));
        assert(race_load_game());
        int found=0;
        for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].owner==SIDE && IS_WORKER(u[i].type) &&
           u[i].order==ORDER_MINE && u[i].work_timer==20 && !u[i].carry) ++found;
        assert(found==1);
    }
    puts("old writer / new reader worker timer: ok");
    return 0;
}
