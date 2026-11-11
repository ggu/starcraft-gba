/* SPDX-License-Identifier: GPL-3.0-or-later
 * Constructed Z6 HOST state; permissions are synthetic unless explicitly
 * compiled against the verified private PUNI bank. No campaign-win claim. */
#define SC64_SHARED_EGG_NO_MAIN
#include "sc64_shared_egg_journal_game.c"
#undef SC64_SHARED_EGG_NO_MAIN
#ifndef SC64_SHARED_EGG_Z6_HOST_STAGE
#error "Z6 Egg fixture requires the isolated stage"
#endif
#ifdef SC64_Z6_ACTUAL_PERMISSIONS
#define Z6_DENIED_PRODUCT U_QUEEN
#define Z6_PERMISSION_LABEL "verified actual"
#else
#define Z6_DENIED_PRODUCT U_DRONE
#define Z6_PERMISSION_LABEL "synthetic"
#endif
int fixture_export_host_save(const char *path);
int fixture_import_host_save(const char *path);
int fixture_host_save_equals(const char *path);
static int zerglings(void)
{
    int n=0;for(int i=0;i<MAX_UNITS;++i)
        n+=u[i].alive && u[i].owner==SIDE && u[i].type==U_ZERGLING;
    return n;
}
static void start_z6(void)
{
    clear_records();game_init();held_keys=0;
    int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z6_GLOBAL_ID);
    assert(ordinal>=0);press(KEY_A);
    for(int page=0;page<ordinal/2;page++)press(KEY_RIGHT);
    if(ordinal&1)press(KEY_DOWN);
    press(KEY_A);press(KEY_START);
    assert(screen==SCREEN_PLAY && !race_mode && race_selected_mission==SC64_Z6_GLOBAL_ID);
}
#ifndef SC64_Z6_EGG_NO_MAIN
int main(int argc,char **argv)
{
    assert(argc==1 || (argc==3 && (!strcmp(argv[1],"write") || !strcmp(argv[1],"read") || !strcmp(argv[1],"legacy"))));
    if(argc==3 && !strcmp(argv[1],"legacy")) {
        game_init();assert(fixture_import_host_save(argv[2]));
        RaceLoadPlan plan;GbaSaveSelection older,winner;
        assert(race_worker_cargo_select(&plan,&older));
        assert(older.tag==SC64_TRANSPORT_SAVE_VERSION && race_load_game());
        assert(!race_mode && race_selected_mission==SC64_Z6_GLOBAL_ID);
        int hall=hall_for_owner(SIDE);assert(hall>=0);
        minerals=1000;gas=1000;b[hall].larva=2;
        assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);
        assert(spawn_build(SIDE,B_SPAWNING_POOL,b[hall].x+80,b[hall].y+80,0)>=0);
        assert(race_queue_unit(SIDE,hall,U_ZERGLING));int egg=egg_for_owner(SIDE,-1);assert(egg>=0);
        assert(race_save_game());u32 length=0;
        assert(save_read_latest(saved.bytes,sizeof(saved.bytes),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&length));
        edited=saved;Unit denied;
        assert(sc64_zp_unit_save_decode(&denied,&edited.record.save.units[egg],race_cols*16u,race_rows*16u));
        denied.raw_owner=-1;
        assert(sc64_zp_unit_save_encode(&edited.record.save.units[egg],&denied,race_cols*16u,race_rows*16u));
        const u32 known[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION};
        assert(save_write_selected(edited.bytes,length,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&older,known,3));
        assert(race_worker_cargo_select(&plan,&winner) && winner.slot==older.slot);
        assert(race_load_game() && !race_mode && race_selected_mission==SC64_Z6_GLOBAL_ID);
        assert(egg_for_owner(SIDE,-1)<0);
        puts("Z6 genuine old-tag save restored after forged newer Egg");return 0;
    }
    if(argc==3 && !strcmp(argv[1],"read")) {
        game_init();assert(fixture_import_host_save(argv[2]));
        assert(race_load_game() && !race_mode && race_selected_mission==SC64_Z6_GLOBAL_ID);
        int egg=egg_for_owner(SIDE,-1),hall=hall_for_owner(SIDE);
        const Sc64RaceMap *map=&sc64_race_maps[SC64_Z6_GLOBAL_ID];
        assert(egg>=0 && hall>=0 && u[egg].target==U_ZERGLING &&
               u[egg].raw_owner==map->human_slot && minerals==950 && gas==1000 && b[hall].larva==1);
        int before=zerglings(),timer=u[egg].work_timer; assert(timer>0);
        for(int i=0;i<timer;++i)race_z7_egg_tick_unit(egg);
        assert(zerglings()==before+2 && fixture_host_save_equals(argv[2]));
        puts("Z6 " Z6_PERMISSION_LABEL "-permission fresh-process paired birth passed");return 0;
    }
    start_z6();
    int hall=hall_for_owner(SIDE);
    if(hall<0) {hall=spawn_build(SIDE,B_HATCHERY,200,200,0);assert(hall>=0);b[hall].raw_owner=3;}
    const Sc64RaceMap *map=&sc64_race_maps[SC64_Z6_GLOBAL_ID];
    assert(map->human_slot==3 && b[hall].raw_owner==map->human_slot);
    assert(sc64_race_raw_unit(U_ZERGLING)==37 && sc64_race_raw_unit(U_DRONE)==41);
    minerals=1000;gas=1000;b[hall].larva=2;
    assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);
    assert(spawn_build(SIDE,B_SPAWNING_POOL,b[hall].x+80,b[hall].y+80,0)>=0);
    assert(race_owner_has_prereq(SIDE,SC_PR_SPAWNING_POOL));
    int before=minerals,source=b[hall].raw_owner;
    assert(!sc64_mid_zerg_permission_available(SC64_Z6_GLOBAL_ID,source,46));
#ifndef SC64_Z6_ACTUAL_PERMISSIONS
    assert(!sc64_mid_zerg_permission_available(SC64_Z6_GLOBAL_ID,source,41));
    assert(!race_queue_unit(SIDE,hall,U_DRONE));
#endif
    assert(minerals==before && b[hall].larva==2);
    b[hall].raw_owner=99;assert(!race_queue_unit(SIDE,hall,U_ZERGLING));
    assert(minerals==before && b[hall].larva==2);b[hall].raw_owner=source;
    assert(race_queue_unit(SIDE,hall,U_ZERGLING));
    int egg=egg_for_owner(SIDE,-1);assert(egg>=0 && minerals==before-50);
    assert(u[egg].target==U_ZERGLING && u[egg].raw_owner==source && u[egg].work_timer>0);
    assert(race_save_game());u32 length=0;
    assert(save_read_latest(saved.bytes,sizeof(saved.bytes),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&length));
    if(argc==3) {assert(fixture_export_host_save(argv[2]));puts("Z6 " Z6_PERMISSION_LABEL "-permission producer saved");return 0;}
    assert(race_shared_egg_candidate_owner(SIDE,source,&race_data));
    assert(!race_shared_egg_candidate_owner(SIDE,-1,&race_data));
    assert(!race_shared_egg_candidate_owner(ENEMY,source,&race_data));
    assert(race_load_game());
    int count=zerglings(),timer=u[egg].work_timer;
    for(int i=0;i<timer;++i)race_z7_egg_tick_unit(egg);
    assert(zerglings()==count+2);
#ifndef SC64_Z6_ACTUAL_PERMISSIONS
    edited=saved;Unit denied;
    assert(sc64_zp_unit_save_decode(&denied,&edited.record.save.units[egg],race_cols*16u,race_rows*16u));
    denied.target=U_DRONE;denied.work_timer=sc64_z7_egg_duration(U_DRONE);
    assert(sc64_shared_egg_wire_valid(&denied,1,race_shared_egg_candidate_owner,&edited.record.save));
    assert(sc64_zp_unit_save_encode(&edited.record.save.units[egg],&denied,race_cols*16u,race_rows*16u));
    clear_records();assert(save_write_first(edited.bytes,length,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();
#endif
    edited=saved;Unit forged;
    assert(sc64_zp_unit_save_decode(&forged,&edited.record.save.units[egg],race_cols*16u,race_rows*16u));
    forged.raw_owner=-1;
    assert(sc64_zp_unit_save_encode(&edited.record.save.units[egg],&forged,race_cols*16u,race_rows*16u));
    clear_records();assert(save_write_first(edited.bytes,length,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();
    puts("Z6 " Z6_PERMISSION_LABEL "-permission paid paired Egg journal passed");return 0;
}
#endif
