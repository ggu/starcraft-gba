/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only Z8 queued Egg and authenticated journal continuation. */
#define main sc64_z8_existing_fixture_main
#include "sc64_z8_game.c"
#undef main
int main(void)
{
    fixture_clear_candidate_records();
    game_init();held_keys=0;
    int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z8_GLOBAL_ID);
    assert(ordinal>=0);press(KEY_A);
    for(int page=0;page<ordinal/2;page++)press(KEY_RIGHT);
    if(ordinal&1)press(KEY_DOWN);
    press(KEY_A);press(KEY_START);
    assert(screen==SCREEN_PLAY && race_z8_active());
    int hall=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(building_complete(i) && b[i].owner==SIDE && sc64_race_is_zerg_townhall(b[i].type)){hall=i;break;}
    assert(hall>=0);minerals=1000;gas=1000;b[hall].larva=3;
    assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);
    assert(race_queue_unit(SIDE,hall,U_DRONE));
    int egg=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].owner==SIDE && u[i].type==U_Z7_EGG){egg=i;break;}
    assert(egg>=0);int owner=u[egg].raw_owner,timer=u[egg].work_timer;
    assert(owner==b[hall].raw_owner && timer>0);assert(race_save_game());
    race_selected_mission=SC64_Z7_GLOBAL_ID;race_mode=1;race_player[SIDE]=R_TER;
    assert(race_load_game());assert(!race_mode && race_selected_mission==SC64_Z8_GLOBAL_ID);
    assert(u[egg].alive && u[egg].type==U_Z7_EGG && u[egg].raw_owner==owner && u[egg].work_timer==timer);
    puts("Z8 Egg journal continuation passed");return 0;
}
