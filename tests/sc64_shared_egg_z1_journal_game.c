/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST constructed Z1 state; no guest save injection. */
#define SC64_SHARED_EGG_NO_MAIN
#include "sc64_shared_egg_journal_game.c"
#undef SC64_SHARED_EGG_NO_MAIN
#ifndef SC64_SHARED_EGG_Z1_HOST_STAGE
#error "Z1 Egg fixture requires the isolated stage"
#endif
int main(void)
{
    clear_records();game_init();held_keys=0;
    int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_RACE_MISSION_Z1);
    assert(ordinal>=0);press(KEY_A);
    for(int page=0;page<ordinal/2;page++)press(KEY_RIGHT);
    if(ordinal&1)press(KEY_DOWN);
    press(KEY_A);press(KEY_START);
    assert(screen==SCREEN_PLAY && !race_mode && race_selected_mission==SC64_RACE_MISSION_Z1);
    int hall=hall_for_owner(SIDE);assert(hall>=0);
    const Sc64RaceMap *map=&sc64_race_maps[SC64_RACE_MISSION_Z1];
    assert(map->human_slot<8 && b[hall].raw_owner==map->human_slot);
    minerals=1000;gas=1000;b[hall].larva=2;
    assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);
    int before=minerals, source=b[hall].raw_owner;
    b[hall].raw_owner=99;
    assert(!race_queue_unit(SIDE,hall,U_DRONE));
    assert(minerals==before && b[hall].larva==2);
    b[hall].raw_owner=source;
    assert(race_queue_unit(SIDE,hall,U_DRONE));
    int egg=egg_for_owner(SIDE,-1);assert(egg>=0 && minerals==before-50);
    int timer=u[egg].work_timer;assert(timer>0 && u[egg].raw_owner==map->human_slot);
    assert(race_save_game());
    u32 length=0;
    assert(save_read_latest(saved.bytes,sizeof(saved.bytes),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&length));
    assert(race_shared_egg_candidate_owner(SIDE,source,&race_data));
    assert(!race_shared_egg_candidate_owner(SIDE,-1,&race_data));
    assert(!race_shared_egg_candidate_owner(SIDE,8,&race_data));
    assert(!race_shared_egg_candidate_owner(ENEMY,source,&race_data));
    race_mode=1;race_player[SIDE]=R_TER;
    assert(race_load_game() && !race_mode && race_selected_mission==SC64_RACE_MISSION_Z1);
    assert(u[egg].alive && u[egg].type==U_Z7_EGG && u[egg].work_timer==timer && u[egg].raw_owner==map->human_slot);
    for(int i=0;i<timer;i++)race_z7_egg_tick_unit(egg);
    assert(u[egg].alive && u[egg].type==U_DRONE && u[egg].raw_owner==map->human_slot);
    /* A checksum-valid malformed candidate cannot mutate the live world. */
    edited=saved;Unit forged;
    assert(sc64_zp_unit_save_decode(&forged,&edited.record.save.units[egg],race_cols*16u,race_rows*16u));
    forged.raw_owner=-1;
    assert(sc64_zp_unit_save_encode(&edited.record.save.units[egg],&forged,race_cols*16u,race_rows*16u));
    clear_records();assert(save_write_first(edited.bytes,length,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();
    puts("Z1 paid Egg HOST journal and birth passed");return 0;
}
