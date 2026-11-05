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
        if(u[i].type==U_REAVER)reaver=i;
        if(u[i].type==SC64_SHARED_EGG_TYPE)egg=i;
    }
    assert(reaver>=0 && egg>=0);
    minerals=200;gas=200;
    assert(race_scarab_enqueue(reaver,0));
    assert(race_scarab_production_tick(reaver,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(reaver,0,&parent,&counts));
    int child=parent.active_slot;assert(child>=0);
    for(int i=0;i<SC64_SCARAB_BUILD_TICKS;i++)
        assert(race_scarab_production_tick(reaver,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(reaver,0)==SC64_SCARAB_COMPLETED);
    int target=spawn_unit(ENEMY,U_ZERGLING,u[reaver].x,u[reaver].y+60);assert(target>=0);
    race_scarab_probe_parent_attack(reaver,target);
    assert(u[child].order==1 && u[child].target==target);
    /* Use the real order-1 dispatcher: it initializes motion scratch before
     * the first flight-control tick (P10 constructor has a cache marker). */
    race_scarab_control_dispatch(child);
    assert(u[child].alive && u[child].order==2);
    race_scarab_control_dispatch(child);
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    /* Exercise real frame/cache processing while motion flags are live. */
    for(int i=0;i<4;i++)game_update(0,0);
    assert(u[child].alive && u[child].order==2 && SC64_P7_ARBITER_CACHE_VALID(u[child].weapon));
    Sc64ScarabSteer world_motion;assert(race_scarab_probe_motion_load(&u[child],&world_motion));
#endif
    Unit motion=u[child];
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
#define PROFILE_PREVIOUS_X(q) ((q).path_timer & 4095u)
#else
#define PROFILE_PREVIOUS_X(q) ((q).path_timer)
#endif
    if (!(motion.order==2 && (PROFILE_PREVIOUS_X(motion)>24 || motion.path_retry>12)))
        fprintf(stderr,"flight route unavailable: alive=%d order=%d x=%d y=%d tx=%d ty=%d path=%d,%d previous=%d,%d cd=%u carry=%d timer=%u target=%d\n",
            motion.alive,motion.order,motion.x,motion.y,motion.tx,motion.ty,motion.path_x,motion.path_y,
            motion.path_timer,motion.path_retry,(unsigned)motion.cd,motion.carry,(unsigned)motion.work_timer,motion.target);
    assert(motion.order==2 && (PROFILE_PREVIOUS_X(motion)>24 || motion.path_retry>12));
#undef PROFILE_PREVIOUS_X
    assert(spawn_build(SIDE,B_GREATER_SPIRE,390,390,0)>=0);
    int muta=spawn_unit(SIDE,U_MUTALISK,350,350);assert(muta>=0);
    assert(race_guardian_morph_start(muta));
    assert(u[muta].type==U_GUARDIAN_COCOON && minerals==135 && gas==100);
    /* Identify a rejected row without mutating the candidate. */
    for(int i=0;i<MAX_UNITS;i++) {
        Unit w,sw;
        if (!sc64_guardian_wire_encode(&u[i],&w,1)) {
            fprintf(stderr,"guardian wire rejected slot %d type %d raw %d\n",i,u[i].type,u[i].raw_type);break;
        }
        if (w.alive && (w.type==U_SCARAB || w.raw_type==85) &&
            !sc64_scarab_waypoint_to_wire(&w,&w,(unsigned)(race_cols*16),(unsigned)(race_rows*16),15u,SC64_ACTOR_WAYPOINT_CODEC)) {
            fprintf(stderr,"waypoint wire rejected slot %d\n",i);break;
        }
        if (!race_scarab_save_wire(&w,&sw)) { fprintf(stderr,"flight wire rejected slot %d\n",i);break; }
        if(sw.type==SC64_SHARED_EGG_TYPE && !sw.alive)memset(&sw,0,sizeof(sw));
#ifdef SC64_Z7_DERIVED_SAVE
        Sc64DerivedUnitSave row;
        if (!sc64_derived_unit_encode_for(&row,&sw,(unsigned)(race_cols*16),(unsigned)(race_rows*16),15)) {
#else
        Sc64P7TypedUnitSave row;
        if (!sc64_p7_typed_unit_encode_for(&row,&sw,(unsigned)(race_cols*16),(unsigned)(race_rows*16),15)) {
#endif
            fprintf(stderr,"leaf wire rejected slot %d type %d raw %d hp %d maxhp %d cd %u shield %d flags %u path %d,%d timers %u,%u\n",
                i,sw.type,sw.raw_type,sw.hp,sw.maxhp,(unsigned)sw.cd,sw.shield,(unsigned)sw.flags,
                sw.path_x,sw.path_y,(unsigned)sw.path_timer,(unsigned)sw.path_retry);break;
        }
    }
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    for(int i=0;i<MAX_BUILDINGS;i++) {
        Sc64P7TypedBuildingSave row;
        if(!sc64_p7_typed_building_encode(&row,&b[i],(unsigned)(race_cols*16),(unsigned)(race_rows*16))) {
            fprintf(stderr,"building row rejected slot %d type %d hp %d maxhp %d raw %d owner %d larva %d timer %d\n",
                i,b[i].type,b[i].hp,b[i].maxhp,b[i].raw_type,b[i].owner,b[i].larva,b[i].larva_timer);break;
        }
    }
    fprintf(stderr,"pre-save battery=%d research=%d\n",race_p7_battery_live_world_valid(),race_bc_research_live_valid());
#endif
    int saved=race_save_game();
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    if(!saved) {
        const Sc64StageTagPair pair={SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION,
                                    SC64_SHARED_ACTOR_FLIGHT_SAVE_VERSION,15u};
        Sc64StageSaveAuth auth;
        if(sc64_stage_save_authenticate(&auth,&race_data,pair.outer,race_data.version,&pair,1)) {
            const Sc64StageSaveAuth *guard_auth=&auth;
            fprintf(stderr,"post-save candidate state=%d scarab=%d guardian=%d transport=%d\n",race_data.state[0],
                race_scarab_save_candidate_valid(&race_data,(unsigned)race_cols*16u,(unsigned)race_rows*16u,guard_auth),
                race_guardian_candidate_valid(&race_data,(unsigned)race_cols*16u,(unsigned)race_rows*16u,guard_auth),
                race_sk_transport_candidate_valid(&race_data));
        }
    }
#endif
    assert(saved);RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION &&
           plan.guardian_auth.inner==SC64_SHARED_ACTOR_FLIGHT_SAVE_VERSION &&
           plan.guardian_auth.features==15u);
    guardian_test_snapshot(before);
    guardian_test_retag(selected.slot,SC64_SHARED_ACTOR_FULL_TRANSPORT_SAVE_VERSION,
                         SC64_SHARED_ACTOR_FULL_SAVE_VERSION);
    GbaSaveSelection fallback;
    assert(race_worker_cargo_select(&plan,&fallback));
    assert(fallback.tag==SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION && fallback.slot!=selected.slot);
    guardian_test_restore(before);
    memset(u,0,sizeof(u));assert(race_load_game());
    assert(u[child].cd==motion.cd && u[child].weapon==motion.weapon &&
           u[child].path_timer==motion.path_timer && u[child].path_retry==motion.path_retry &&
           u[child].path_x==motion.path_x && u[child].path_y==motion.path_y);
    assert(u[egg].alive && u[egg].type==SC64_SHARED_EGG_TYPE &&
           u[muta].alive && u[muta].type==U_GUARDIAN_COCOON &&
           u[child].alive && u[child].type==U_SCARAB);
    for(int i=0;i<SC64_GUARDIAN_TICKS;i++)assert(race_guardian_morph_tick(muta));
    assert(sc64_race_raw_unit(u[muta].type)==44 && u[muta].raw_type==44);
    assert(race_save_game());memset(u,0,sizeof(u));assert(race_load_game());
    assert(sc64_race_raw_unit(u[muta].type)==44 && u[muta].raw_type==44 && u[egg].alive);
    puts("profile HOST selected journal: predecessor, paid Egg/Cocoon/Scarab and Guardian passed");
    return 0;
}
