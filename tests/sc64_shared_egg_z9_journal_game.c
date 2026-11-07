/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only actual Z9 Egg journal and malformed owner regression. */
#define SC64_SHARED_EGG_NO_MAIN
#include "sc64_shared_egg_journal_game.c"
#undef SC64_SHARED_EGG_NO_MAIN
int main(void)
{
    clear_records();launch_z9_from_normal_menu();
    int hall=hall_for_owner(SIDE);assert(hall>=0);
    minerals=1000;gas=1000;b[hall].larva=3;
    assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);
    assert(race_queue_unit(SIDE,hall,U_DRONE));
    int egg=egg_for_owner(SIDE,-1);assert(egg>=0);
    int source_owner=u[egg].raw_owner,timer=u[egg].work_timer;
    assert(source_owner==b[hall].raw_owner && timer>0);
    assert(race_save_game());u32 len=0;
    assert(save_read_latest(saved.bytes,sizeof(saved.bytes),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&len));
    race_selected_mission=SC64_Z7_GLOBAL_ID;race_mode=1;race_player[SIDE]=R_TER;
    assert(race_load_game());
    assert(!race_mode && race_selected_mission==SC64_Z9_GLOBAL_ID);
    assert(u[egg].alive && u[egg].type==U_Z7_EGG && u[egg].raw_owner==source_owner && u[egg].work_timer==timer);
    /* A genuinely paid paired Egg survives a real journal roundtrip and
     * completes both bodies; the extra body consumes its reserved slot. */
    assert(spawn_build(SIDE,B_SPAWNING_POOL,120,120,0)>=0);
    assert(spawn_unit(SIDE,U_OVERLORD,95,95)>=0);b[hall].larva=2;
    assert(race_queue_unit(SIDE,hall,U_ZERGLING));
    int pair=-1,before=0;
    for(int i=0;i<MAX_UNITS;i++) {
        if(u[i].alive && u[i].owner==SIDE && u[i].type==U_ZERGLING) before++;
        if(u[i].alive && u[i].type==U_Z7_EGG && u[i].target==U_ZERGLING) pair=i;
    }
    assert(pair>=0);int pair_timer=u[pair].work_timer;assert(race_save_game());
    assert(race_load_game() && u[pair].work_timer==pair_timer);
    for(int i=1;i<pair_timer;i++) race_z7_egg_tick_unit(pair);
    assert(u[pair].type==U_Z7_EGG && u[pair].work_timer==1);
    /* HOST-only saturation: a paid pair must wait without consuming its
     * timer or losing its second body until a slot becomes free. */
    int artificial=-1;
    for(int i=0;i<MAX_UNITS;i++) if(!u[i].alive) {
        u[i].alive=1;u[i].owner=ENEMY;u[i].type=U_DRONE;artificial=i;
    }
    assert(artificial>=0 && find_unit_slot()<0);
    race_z7_egg_tick_unit(pair);
    assert(u[pair].type==U_Z7_EGG && u[pair].work_timer==1);
    u[artificial].alive=0;race_z7_egg_tick_unit(pair);
    int after=0;for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive && u[i].owner==SIDE && u[i].type==U_ZERGLING) after++;
    assert(after==before+2);
    edited=saved;Unit bad;
    assert(sc64_zp_unit_save_decode(&bad,&edited.record.save.units[egg],race_cols*16u,race_rows*16u));
    bad.raw_owner=-1;
    assert(sc64_zp_unit_save_encode(&edited.record.save.units[egg],&bad,race_cols*16u,race_rows*16u));
    clear_records();assert(save_write_first(edited.bytes,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION));
    expect_failed_load_preserves_world();
    puts("Z9 Egg journal and owner atomicity passed");return 0;
}
