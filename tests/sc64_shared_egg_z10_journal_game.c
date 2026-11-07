/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real Z10 descriptor and journal; fixture setup is HOST-only. */
#define SC64_SHARED_EGG_NO_MAIN
#include "sc64_shared_egg_journal_game.c"
#undef SC64_SHARED_EGG_NO_MAIN
int main(void)
{
    clear_records();game_init();held_keys=0;
    int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z10_GLOBAL_ID);
    assert(ordinal==9);press(KEY_A);
    for(int page=0;page<ordinal/2;page++) press(KEY_RIGHT);
    if(ordinal&1)press(KEY_DOWN);
    press(KEY_A);press(KEY_START);
    assert(screen==SCREEN_PLAY && race_zl_active() && race_selected_mission==SC64_Z10_GLOBAL_ID);
    int hall=hall_for_owner(SIDE);assert(hall>=0);
    minerals=1000;gas=1000;b[hall].larva=3;
    assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);
    assert(race_queue_unit(SIDE,hall,U_DRONE));
    int egg=egg_for_owner(SIDE,-1);assert(egg>=0);
    int owner=u[egg].raw_owner,timer=u[egg].work_timer;
    assert(owner==b[hall].raw_owner && race_save_game());
    u32 len=0;assert(save_read_latest(saved.bytes,sizeof(saved.bytes),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&len));
    race_selected_mission=SC64_Z7_GLOBAL_ID;race_mode=1;race_player[SIDE]=R_TER;
    assert(race_load_game() && !race_mode && race_selected_mission==SC64_Z10_GLOBAL_ID);
    assert(u[egg].alive && u[egg].type==U_Z7_EGG && u[egg].raw_owner==owner && u[egg].work_timer==timer);
    edited=saved;Unit bad;
    assert(sc64_zp_unit_save_decode(&bad,&edited.record.save.units[egg],race_cols*16u,race_rows*16u));
    bad.raw_owner=-1;
    assert(sc64_zp_unit_save_encode(&edited.record.save.units[egg],&bad,race_cols*16u,race_rows*16u));
    clear_records();assert(save_write_first(edited.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();
    puts("Z10 Egg descriptor, journal and owner atomicity passed");return 0;
}
