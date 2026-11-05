/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only real journal integration; malformed records are HOST fixtures. */
#define main sc64_z9_existing_fixture_main
#include "sc64_z9_game.c"
#undef main
#ifndef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
#error "This fixture requires the isolated HOST Egg journal"
#endif
static union { RaceImpactRecord record; u8 bytes[sizeof(RaceImpactRecord)]; } saved,edited;
static Unit live_before[MAX_UNITS];
static Building buildings_before[MAX_BUILDINGS];
static void clear_records(void)
{
    fixture_clear_record_tag(SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION);
    fixture_clear_candidate_records();
}
static void start_custom(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;
    race_bot_races[0]=R_ZERG;init_state();assert(screen==SCREEN_PLAY && race_mode);
    minerals=1000;gas=1000;race_minerals[ENEMY]=1000;race_gas[ENEMY]=1000;
}
static int hall_for_owner(int owner)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(building_complete(i) && b[i].owner==owner &&
        sc64_race_is_zerg_townhall(b[i].type))return i;
    return -1;
}
static int egg_for_owner(int owner,int except)
{
    for(int i=0;i<MAX_UNITS;i++)if(i!=except && u[i].alive && u[i].owner==owner &&
        u[i].type==U_Z7_EGG)return i;
    return -1;
}
static void expect_failed_load_preserves_world(void)
{
    memcpy(live_before,u,sizeof(live_before));memcpy(buildings_before,b,sizeof(buildings_before));
    int ore=minerals,vespene=gas,mission=race_selected_mission,mode=race_mode;
    assert(!race_load_game());
    assert(!memcmp(live_before,u,sizeof(live_before)) && !memcmp(buildings_before,b,sizeof(buildings_before)));
    assert(minerals==ore && gas==vespene && race_selected_mission==mission && race_mode==mode);
}
int main(void)
{
    clear_records();start_custom();
    /* A legacy ordinary/raw36 row must load through the unchanged old tag. */
    int ordinary=spawn_unit(SIDE,U_OVERLORD,100,100);assert(ordinary>=0);
    u[ordinary].raw_type=36;u[ordinary].raw_owner=-1;
    assert(race_save_game());u32 len=0;
    assert(save_read_latest(saved.bytes,sizeof(saved.bytes),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&len));
    clear_records();edited=saved;edited.record.save.version=SAVE_VERSION;
    assert(save_write_first(edited.bytes,len,SC64_TRANSPORT_SAVE_VERSION));
    u[ordinary].raw_type=-1;assert(race_load_game());
    assert(u[ordinary].alive && u[ordinary].type==U_OVERLORD && u[ordinary].raw_type==36);

    clear_records();start_custom();int hall=hall_for_owner(SIDE);assert(hall>=0 && b[hall].raw_owner==-1);
    assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);b[hall].larva=3;
    assert(race_queue_unit(SIDE,hall,U_DRONE));int a=egg_for_owner(SIDE,-1);assert(a>=0);
    assert(race_queue_unit(SIDE,hall,U_DRONE));int z=egg_for_owner(SIDE,a);assert(z>=0 && z!=a);
    int timer=u[a].work_timer;assert(timer>10 && u[z].work_timer==timer);
    assert(race_save_game());assert(save_read_latest(saved.bytes,sizeof(saved.bytes),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&len));
    for(int i=0;i<10;i++)update_orders();assert(u[a].work_timer==timer-10);
    assert(race_load_game());assert(u[a].type==U_Z7_EGG && u[z].type==U_Z7_EGG);
    assert(u[a].work_timer==timer && u[z].work_timer==timer);
    for(int i=0;i<timer;i++)update_orders();
    assert(u[a].alive && u[z].alive && u[a].type==U_DRONE && u[z].type==U_DRONE);

    /* The outer slot is committed with a valid checksum, but its inner tag,
     * owner or aggregate future-slot budget is invalid. No live state may
     * be published. These records are created only in the HOST fixture. */
    clear_records();edited=saved;edited.record.save.version=SAVE_VERSION;
    assert(save_write_first(edited.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();assert(!race_save_game());

    clear_records();edited=saved;Unit row;
    assert(sc64_zp_unit_save_decode(&row,&edited.record.save.units[a],race_cols*16u,race_rows*16u));
    row.raw_owner=3;assert(sc64_zp_unit_save_encode(&edited.record.save.units[a],&row,race_cols*16u,race_rows*16u));
    assert(save_write_first(edited.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();assert(!race_save_game());

    clear_records();edited=saved;
    assert(sc64_zp_unit_save_decode(&row,&edited.record.save.units[a],race_cols*16u,race_rows*16u));
    row.target=U_ZERGLING;row.work_timer=1;
    for(unsigned i=0;i<MAX_UNITS;i++) {
        Unit prior;assert(sc64_zp_unit_save_decode(&prior,&edited.record.save.units[i],race_cols*16u,race_rows*16u));
        if (!prior.alive) assert(sc64_zp_unit_save_encode(&edited.record.save.units[i],&row,race_cols*16u,race_rows*16u));
    }
    assert(!race_shared_egg_custom_candidate_valid(&edited.record.save,race_cols*16u,race_rows*16u));
    assert(save_write_first(edited.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();assert(!race_save_game());
    puts("HOST custom Egg journal roundtrip and atomic rejection passed");return 0;
}
