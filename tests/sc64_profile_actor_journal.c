/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST candidate: real predecessor file, paid actor jobs, selected journal.
 * Constructed test world; not a guest SRAM or N64 qualification. */
#define main sc64_profile_full_original_main
#include "sc64_shared_actor_full_journal.c"
#undef main
int main(int argc,char **argv)
{
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fread(sram,1,sizeof(sram),f)==sizeof(sram));assert(!fclose(f));
    guardian_test_reset();game_init();guardian_test_restore(sram);assert(race_load_game());
    int reaver=-1,egg=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive) {
        if(u[i].type==U_REAVER && u[i].x==300)reaver=i;
        if(u[i].type==SC64_SHARED_EGG_TYPE)egg=i;
    }
    assert(reaver>=0 && egg>=0);
    minerals=200;gas=200;
    assert(race_scarab_enqueue(reaver,0));
    assert(race_scarab_production_tick(reaver,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(reaver,0,&parent,&counts));
    int child=parent.active_slot;assert(child>=0);
    assert(spawn_build(SIDE,B_GREATER_SPIRE,390,390,0)>=0);
    int muta=spawn_unit(SIDE,U_MUTALISK,350,350);assert(muta>=0);
    assert(race_guardian_morph_start(muta));
    assert(u[muta].type==U_GUARDIAN_COCOON && minerals==135 && gas==100);
    assert(race_save_game());RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION &&
           plan.guardian_auth.inner==SC64_SHARED_ACTOR_FLIGHT_SAVE_VERSION &&
           plan.guardian_auth.features==15u);
    memset(u,0,sizeof(u));assert(race_load_game());
    assert(u[egg].alive && u[egg].type==SC64_SHARED_EGG_TYPE &&
           u[muta].alive && u[muta].type==U_GUARDIAN_COCOON &&
           u[child].alive && u[child].type==U_SCARAB);
    for(int i=0;i<SC64_GUARDIAN_TICKS;i++)assert(race_guardian_morph_tick(muta));
    assert(u[muta].type==U_GUARDIAN_STAGE && u[muta].raw_type==44);
    assert(race_save_game());memset(u,0,sizeof(u));assert(race_load_game());
    assert(u[muta].type==U_GUARDIAN_STAGE && u[muta].raw_type==44 && u[egg].alive);
    puts("profile HOST selected journal: predecessor, paid Egg/Cocoon/Scarab and Guardian passed");
    return 0;
}
