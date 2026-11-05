/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only real journal integration; malformed records are HOST fixtures. */
#define main sc64_z9_existing_fixture_main
#include "sc64_z9_game.c"
#undef main
#ifndef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
#error "This fixture requires the isolated HOST Egg journal"
#endif
void fixture_set_host_sequence(int slot,u32 sequence);
static union { RaceImpactRecord record; u8 bytes[sizeof(RaceImpactRecord)]; } saved,edited;
static Unit live_before[MAX_UNITS];
static Building buildings_before[MAX_BUILDINGS];
static u8 explored_before[sizeof(explored)],visible_before[sizeof(visible)];
static u8 minerals_before[sizeof(m)],geysers_before[sizeof(geysers)];
static u8 impact_before[sizeof(race_impact_rows)],front_before[sizeof(race_front)];
static Sc64SkTransportTech transport_before;
static int players_before[5],ore_before[5],gas_before[5],bot_races_before[3];
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
    memcpy(explored_before,explored,sizeof(explored));memcpy(visible_before,visible,sizeof(visible));
    memcpy(minerals_before,m,sizeof(m));memcpy(geysers_before,geysers,sizeof(geysers));
    memcpy(impact_before,race_impact_rows,sizeof(race_impact_rows));
    memcpy(front_before,&race_front,sizeof(race_front));transport_before=race_skirmish_transport_tech;
    memcpy(players_before,race_player,sizeof(race_player));memcpy(ore_before,race_minerals,sizeof(race_minerals));
    memcpy(gas_before,race_gas,sizeof(race_gas));memcpy(bot_races_before,race_bot_races,sizeof(race_bot_races));
    u32 digest=world_digest(),seed_before=seed;int impacts_before=race_impact_count;
    int values[]={minerals,gas,supply_used,supply_cap,race_selected_mission,race_mode,race_human,
        race_bots,race_completed,race_skirmish_map,frame,screen,mode,menu,menu_open,menu_cursor,
        help,won,lost,result_timer,selected_building,geyser_count,(int)race_front_transition,race_front_load_error};
    assert(!race_load_game());
    assert(!memcmp(live_before,u,sizeof(live_before)) && !memcmp(buildings_before,b,sizeof(buildings_before)));
    assert(!memcmp(explored_before,explored,sizeof(explored)) && !memcmp(visible_before,visible,sizeof(visible)));
    assert(!memcmp(minerals_before,m,sizeof(m)) && !memcmp(geysers_before,geysers,sizeof(geysers)));
    assert(!memcmp(impact_before,race_impact_rows,sizeof(race_impact_rows)) && impacts_before==race_impact_count);
    assert(!memcmp(front_before,&race_front,sizeof(race_front)) && !memcmp(&transport_before,&race_skirmish_transport_tech,sizeof(transport_before)));
    assert(!memcmp(players_before,race_player,sizeof(race_player)) && !memcmp(ore_before,race_minerals,sizeof(race_minerals)));
    assert(!memcmp(gas_before,race_gas,sizeof(race_gas)) && !memcmp(bot_races_before,race_bot_races,sizeof(race_bot_races)));
    int after[]={minerals,gas,supply_used,supply_cap,race_selected_mission,race_mode,race_human,
        race_bots,race_completed,race_skirmish_map,frame,screen,mode,menu,menu_open,menu_cursor,
        help,won,lost,result_timer,selected_building,geyser_count,(int)race_front_transition,race_front_load_error};
    assert(!memcmp(values,after,sizeof(values)) && seed_before==seed && digest==world_digest());
}
int main(void)
{
    clear_records();start_custom();
    /* A legacy ordinary/raw36 row must load through the unchanged old tag. */
    int old_hall=hall_for_owner(SIDE);assert(old_hall>=0);
    b[old_hall].raw_owner=3; /* HOST fixture representing a valid old hall. */
    int ordinary=spawn_unit(SIDE,U_OVERLORD,100,100);assert(ordinary>=0);
    u[ordinary].raw_type=36;u[ordinary].raw_owner=-1;
    assert(race_save_game());u32 len=0;
    assert(save_read_latest(saved.bytes,sizeof(saved.bytes),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&len));
    clear_records();edited=saved;edited.record.save.version=SAVE_VERSION;
    assert(save_write_first(edited.bytes,len,SC64_TRANSPORT_SAVE_VERSION));
    u[ordinary].raw_type=-1;assert(race_load_game());
    assert(u[ordinary].alive && u[ordinary].type==U_OVERLORD && u[ordinary].raw_type==36);
    assert(b[old_hall].alive && b[old_hall].raw_owner==3);
    b[old_hall].larva=3;assert(race_queue_unit(SIDE,old_hall,U_DRONE));
    int from_old=egg_for_owner(SIDE,-1);assert(from_old>=0 && u[from_old].raw_owner==-1);
    assert(b[old_hall].raw_owner==3);assert(race_save_game());assert(race_load_game());
    assert(b[old_hall].raw_owner==3 && u[from_old].alive && u[from_old].raw_owner==-1);

    clear_records();start_custom();int hall=hall_for_owner(SIDE);assert(hall>=0 && b[hall].raw_owner==-1);
    assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);b[hall].larva=3;
    assert(race_queue_unit(SIDE,hall,U_DRONE));int a=egg_for_owner(SIDE,-1);assert(a>=0);
    assert(race_queue_unit(SIDE,hall,U_DRONE));int z=egg_for_owner(SIDE,a);assert(z>=0 && z!=a);
    int timer=u[a].work_timer;assert(timer>10 && u[z].work_timer==timer);
    assert(race_save_game());assert(save_read_latest(saved.bytes,sizeof(saved.bytes),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&len));
    for(int i=0;i<10;i++)update_orders();assert(u[a].work_timer==timer-10);
    /* Loading must bind the candidate's race/owner state, not these live values. */
    race_human=R_TER;race_player[SIDE]=R_TER;race_player[ENEMY]=R_PROTOSS;
    race_bot_races[0]=R_PROTOSS;race_bots=2;race_selected_mission=SC64_Z7_GLOBAL_ID;
    assert(race_load_game());assert(u[a].type==U_Z7_EGG && u[z].type==U_Z7_EGG);
    assert(race_mode && race_human==R_ZERG && race_bots==1 && race_player[SIDE]==R_ZERG && race_player[ENEMY]==R_ZERG);
    assert(u[a].work_timer==timer && u[z].work_timer==timer);
    for(int i=0;i<timer;i++)update_orders();
    assert(u[a].alive && u[z].alive && u[a].type==U_DRONE && u[z].type==U_DRONE);

    /* A bad newer candidate must fall back to the valid older record and
     * retain only that candidate's freshly prepared authentication context. */
    RaceLoadPlan plan;GbaSaveSelection older,winner;
    assert(race_worker_cargo_select(&plan,&older));
    assert(sc64_stage_save_auth_matches(&plan.egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG));
    edited=saved;Unit bad_owner;
    assert(sc64_zp_unit_save_decode(&bad_owner,&edited.record.save.units[a],race_cols*16u,race_rows*16u));
    bad_owner.raw_owner=3;assert(sc64_zp_unit_save_encode(&edited.record.save.units[a],&bad_owner,race_cols*16u,race_rows*16u));
    const u32 known[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION};
    assert(save_write_selected(edited.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&older,known,3));
    memset(&plan,0xa5,sizeof(plan));assert(race_worker_cargo_select(&plan,&winner));
    assert(winner.slot==older.slot && sc64_stage_save_auth_matches(&plan.egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG));
    assert(race_load_game() && u[a].alive && u[a].type==U_Z7_EGG && u[a].work_timer==timer);

    /* Both semantically valid half-range records must reject atomically and
     * clear the local plan. A sole valid candidate must be re-prepared. */
    clear_records();assert(save_write_first(saved.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    assert(race_worker_cargo_select(&plan,&older));
    assert(save_write_selected(saved.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&older,known,3));
    fixture_set_host_sequence(1-older.slot,older.sequence+0x80000000u);
    memset(&plan,0xa5,sizeof(plan));assert(!race_worker_cargo_select(&plan,&winner));
    static const RaceLoadPlan empty_plan={0};assert(!memcmp(&plan,&empty_plan,sizeof(plan)));
    expect_failed_load_preserves_world();
    clear_records();assert(save_write_first(saved.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    assert(race_worker_cargo_select(&plan,&older));
    assert(save_write_selected(edited.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&older,known,3));
    fixture_set_host_sequence(1-older.slot,older.sequence+0x80000000u);
    memset(&plan,0xa5,sizeof(plan));assert(race_worker_cargo_select(&plan,&winner));
    assert(winner.slot==older.slot && sc64_stage_save_auth_matches(&plan.egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG));
    assert(race_load_game() && u[a].alive && u[a].type==U_Z7_EGG && u[a].work_timer==timer);

    /* The outer slot is committed with a valid checksum, but its inner tag,
     * owner or aggregate future-slot budget is invalid. No live state may
     * be published. These records are created only in the HOST fixture. */
    clear_records();edited=saved;edited.record.save.version=SAVE_VERSION;
    assert(save_write_first(edited.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();assert(!race_save_game());

    clear_records();edited=saved;edited.record.save.state[1]=R_TER;
    assert(save_write_first(edited.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();assert(!race_save_game());

    clear_records();edited=saved;Unit row;
    assert(sc64_zp_unit_save_decode(&row,&edited.record.save.units[a],race_cols*16u,race_rows*16u));
    row.raw_owner=3;assert(sc64_zp_unit_save_encode(&edited.record.save.units[a],&row,race_cols*16u,race_rows*16u));
    assert(save_write_first(edited.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();assert(!race_save_game());

    RaceLoadPlan stale;memset(&stale,0xa5,sizeof(stale));
    assert(!race_worker_cargo_prepare_selected(0,0,0,&stale));
    static const RaceLoadPlan zero_plan={0};assert(!memcmp(&stale,&zero_plan,sizeof(stale)));

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
