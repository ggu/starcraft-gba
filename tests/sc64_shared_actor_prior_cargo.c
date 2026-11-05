/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual old-code writer and current reader in independent HOST processes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef SC64_PRIOR_WRITER
#include SC64_PRIOR_GAME
#else
#include "../starcraft/game.c"
#endif
extern void guardian_test_reset(void);
extern void guardian_test_snapshot(void *);
extern void guardian_test_restore(const void *);
static unsigned char sram[32768], before[32768];
#ifndef SC64_PRIOR_WRITER
static RaceImpactRecord original;
#endif
int main(int argc,char **argv)
{
    assert(argc==2);guardian_test_reset();game_init();
#ifdef SC64_PRIOR_WRITER
    race_mode=1;race_human=R_TER;race_bots=1;race_skirmish_map=0;race_bot_races[0]=R_ZERG;init_state();
    int worker=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].owner==SIDE && u[i].type==U_SCV){worker=i;break;}
    assert(worker>=0);u[worker].order=ORDER_GAS;u[worker].carry=8;u[worker].flags=0;
    assert(race_save_game());guardian_test_snapshot(sram);FILE *f=fopen(argv[1],"wb");assert(f);
    assert(fwrite(sram,1,sizeof(sram),f)==sizeof(sram));assert(!fclose(f));
    printf("genuine predecessor worker slot %d tag %08x\n",worker,(unsigned)SC64_TRANSPORT_SAVE_VERSION);
#else
    FILE *f=fopen(argv[1],"rb");assert(f);assert(fread(sram,1,sizeof(sram),f)==sizeof(sram));assert(!fclose(f));
    guardian_test_restore(sram);guardian_test_snapshot(before);u32 len=0;
    assert(save_read_latest(&original,sizeof(original),SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,&len));
    RaceLoadPlan plan;GbaSaveSelection selected;assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION && plan.legacy_worker_cargo &&
           plan.guardian_auth.inner==SC64_WORKER_CARGO_LEGACY_SAVE_VERSION && plan.guardian_auth.features==0);
    /* Dynamic transport/shot tails are outside the authenticated fixed base. */
    assert(len>=sizeof(RaceSave)-SC64_BC_SAVE_EXTRA_BYTES);
    assert(!memcmp(&original.save,&race_data,sizeof(RaceSave)-SC64_BC_SAVE_EXTRA_BYTES));
    assert(race_load_game());int worker=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].owner==SIDE && u[i].type==U_SCV && u[i].carry==8){worker=i;break;}
    assert(worker>=0 && sc64_worker_cargo_is_gas(&u[worker]));
    guardian_test_snapshot(sram);assert(!memcmp(sram,before,sizeof(sram)));
    puts("old cargo selection fixed-base immutable and live gas normalized");
#endif
    return 0;
}
