/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only release/flight save probe with a genuine old custom Egg record. */
#define main prior_full_fixture_main
#include "sc64_shared_actor_full_journal.c"
#undef main
int main(int argc,char **argv)
{
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fread(sram,1,sizeof(sram),f)==sizeof(sram));assert(!fclose(f));
    guardian_test_reset();game_init();guardian_test_restore(sram);
    if (!race_load_game()) {
        u32 len=0;RaceLoadPlan diagnostic;
        int read=save_read_latest(&race_impact_record,sizeof(race_impact_record),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&len);
        int prep=read && race_worker_cargo_prepare_selected(&race_impact_record,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&diagnostic);
        fprintf(stderr,"old candidate read=%d prepared=%d length=%u\n",read,prep,(unsigned)len);
        if(read && diagnostic.guardian_auth.candidate) {
            const Sc64StageSaveAuth *guard_auth=&diagnostic.guardian_auth;
            fprintf(stderr,"old candidate guardian=%d scarab=%d auth=%u\n",
                race_guardian_candidate_valid(&race_data,1024,1024,guard_auth),
                race_scarab_save_candidate_valid(&race_data,1024,1024,guard_auth),guard_auth->features);
        }
        return 1;
    }
    int p=-1,egg=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive) {
        if(u[i].type==U_REAVER && u[i].x==300)p=i;
        if(u[i].type==SC64_SHARED_EGG_TYPE)egg=i;
    }
    assert(p>=0 && egg>=0);minerals=100;assert(race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(p,0,&parent,&counts));int child=parent.active_slot;
    for(int i=0;i<SC64_SCARAB_BUILD_TICKS;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED);
    int target=spawn_unit(ENEMY,U_ZERGLING,u[p].x+10,u[p].y);assert(target>=0);
    race_scarab_probe_parent_attack(p,target);assert(u[child].order==1 && u[child].target==target);
    assert(race_save_game());RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION &&
           plan.guardian_auth.inner==SC64_SHARED_ACTOR_FLIGHT_SAVE_VERSION &&
           plan.guardian_auth.features==15u);
    guardian_test_snapshot(before);memset(u,0,sizeof(u));assert(race_load_game());
    int owner;unsigned phase;assert(race_scarab_child_lookup(child,&owner,&phase) && owner==p && phase==SC64_SCARAB_CHILD_OUTSIDE && u[child].order==1);
    assert(race_scarab_flight_control_tick(child,0,0)==SC64_SCARAB_CONTROL_CONTINUE);
    Sc64ScarabChildFields fields;assert(race_scarab_child_fields_read(u[child].carry,u[child].work_timer,&fields));
    assert(fields.life==90 && fields.no_collision==7 && u[child].order==2);
    assert(race_save_game());memset(u,0,sizeof(u));assert(race_load_game());
    assert(race_scarab_child_fields_read(u[child].carry,u[child].work_timer,&fields));
    assert(fields.life==90 && fields.no_collision==7 && u[child].order==2);
    /* Drive the real FSM to its arrived phase with a controlled arrival signal. */
    assert(race_scarab_flight_control_tick(child,1,0)==SC64_SCARAB_CONTROL_ARRIVED);
    assert(u[child].order==4 && u[child].target==target);
    assert(race_save_game());memset(u,0,sizeof(u));assert(race_load_game());
    assert(u[child].order==4 && u[child].target==target);
    /* A dead slot can be reused before this child dispatches again. */
    int tx=u[child].tx,ty=u[child].ty;
    u[target].alive=0;
    assert(spawn_unit(ENEMY,U_ZERGLING,700,700)==target);
    assert(u[child].target==-1 && u[child].order==4 && u[child].tx==tx && u[child].ty==ty);
    assert(race_save_game());memset(u,0,sizeof(u));assert(race_load_game());
    assert(u[child].target==-1 && u[child].order==4 && u[child].tx==tx && u[child].ty==ty);
    guardian_test_restore(before);assert(race_load_game());
    /* Retagging a flight row as feature7 must not reinterpret the row. */
    guardian_test_retag(selected.slot,SC64_SHARED_ACTOR_FULL_TRANSPORT_SAVE_VERSION,SC64_SHARED_ACTOR_FULL_SAVE_VERSION);
    GbaSaveSelection fallback;assert(race_worker_cargo_select(&plan,&fallback));
    assert(fallback.tag==SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION && fallback.slot!=selected.slot);
    puts("actual HOST fresh and initialized Scarab flight saves, old-tag denial passed");return 0;
}
