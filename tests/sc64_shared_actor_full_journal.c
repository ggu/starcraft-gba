/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST SRAM only; predecessor must be produced by real old code. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_scarab_unit_adapter.h"
extern void guardian_test_reset(void);
extern void guardian_test_snapshot(void *);
extern void guardian_test_restore(const void *);
extern void guardian_test_retag(int,u32,u32);
extern void guardian_test_corrupt(int);
static unsigned char sram[32768],before[32768],after[32768];
static Unit old_units[MAX_UNITS];static Building old_buildings[MAX_BUILDINGS];
int main(int argc,char **argv)
{
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fread(sram,1,sizeof(sram),f)==sizeof(sram));assert(!fclose(f));
    guardian_test_reset();game_init();guardian_test_restore(sram);
    assert(race_load_game());int p=-1,egg=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive) {
        if(u[i].type==U_REAVER && u[i].x==300)p=i;
        if(u[i].type==SC64_SHARED_EGG_TYPE)egg=i;
    }
    assert(p>=0 && egg>=0);minerals=100;
    assert(race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(p,0,&parent,&counts));int child=parent.active_slot;
    assert(child>=0);
    int spire=spawn_build(SIDE,B_GREATER_SPIRE,390,390,0);assert(spire>=0);
    int muta=spawn_unit(SIDE,U_MUTALISK,350,350);assert(muta>=0);
    minerals=200;gas=200;assert(race_guardian_morph_start(muta));
    assert(u[muta].type==U_GUARDIAN_COCOON && minerals==150 && gas==100);
    assert(race_save_game());
    RaceLoadPlan plan;GbaSaveSelection newest;
    assert(race_worker_cargo_select(&plan,&newest));
    assert(newest.tag==SC64_SHARED_ACTOR_FULL_TRANSPORT_SAVE_VERSION &&
           plan.guardian_auth.inner==SC64_SHARED_ACTOR_FULL_SAVE_VERSION &&
           plan.guardian_auth.features==(SC64_STAGE_SAVE_EGG|SC64_STAGE_SAVE_GUARDIAN|SC64_STAGE_SAVE_SCARAB));
    memset(u,0,sizeof(u));assert(race_load_game());
    assert(u[p].alive && u[p].type==U_REAVER && u[child].alive && u[child].type==U_SCARAB && u[egg].alive && u[muta].alive && u[muta].type==U_GUARDIAN_COCOON);
    assert(race_scarab_parent_read(p,0,&parent,&counts) && parent.active_slot==child && parent.queued==1);
    /* A valid checksum with a mismatched authenticated pair falls back to
     * the untouched real predecessor slot. */
    guardian_test_retag(newest.slot,SC64_SHARED_ACTOR_FULL_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    GbaSaveSelection fallback;assert(race_worker_cargo_select(&plan,&fallback));
    assert(fallback.tag==SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION && fallback.slot!=newest.slot);
    assert(race_load_game() && !u[child].alive && u[egg].alive);
    guardian_test_corrupt(fallback.slot);guardian_test_snapshot(before);
    memcpy(old_units,u,sizeof(u));memcpy(old_buildings,b,sizeof(b));
    int ore=minerals,gas_before=gas,frame_before=frame;u32 seed_before=seed;
    assert(!race_load_game());guardian_test_snapshot(after);
    assert(!memcmp(before,after,sizeof(before)) && !memcmp(old_units,u,sizeof(u)) &&
           !memcmp(old_buildings,b,sizeof(b)) && minerals==ore && gas==gas_before && frame==frame_before && seed==seed_before);
    puts("actual selected Egg+Guardian+Scarab journal, old fallback and atomic rejection passed");return 0;
}
