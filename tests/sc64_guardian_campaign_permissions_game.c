/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST world, using the real profile's PUNI and owner map. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

int main(void)
{
    game_init(); memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    race_mode=0; race_human=R_ZERG; race_player[SIDE]=R_ZERG; race_player[ENEMY]=R_ZERG;
    race_selected_mission=SC64_Z7_GLOBAL_ID; race_z7_map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    minerals=500; gas=500; race_minerals[ENEMY]=500; race_gas[ENEMY]=500;
    assert(!race_guardian_campaign_puni(sc64_z5_source_puni,3,44));
    assert(race_guardian_campaign_puni(sc64_z6_source_puni,3,44));
    assert(race_guardian_campaign_puni(sc64_z7_source_puni,3,44));
    assert(!race_guardian_campaign_allowed(SIDE,-1,44));
    assert(!race_guardian_campaign_allowed(SIDE,4,44));
    assert(race_guardian_campaign_allowed(SIDE,3,44));
    assert(race_guardian_campaign_allowed(ENEMY,4,44));
    int spire=spawn_build(SIDE,B_GREATER_SPIRE,200,200,0);
    int muta=spawn_unit(SIDE,U_MUTALISK,250,200);
    assert(spire>=0 && muta>=0 && u[muta].raw_owner==3);
    u[muta].hp=60; u[muta].raw_owner=4;
    assert(!race_guardian_morph_start(muta));
    assert(minerals==500 && gas==500 && u[muta].type==U_MUTALISK && u[muta].hp==60);
    u[muta].raw_owner=3; race_selected_mission=SC64_RACE_MISSION_Z4;
    assert(!race_guardian_morph_start(muta)); /* No authenticated early table. */
    assert(minerals==500 && gas==500 && u[muta].type==U_MUTALISK);
    race_selected_mission=SC64_Z7_GLOBAL_ID;
    selected_building=-1; u[muta].selected=1;
    assert(!strcmp(race_menu_label(MENU_STOP+1),"MORPH GUARDIAN"));
    choose_command(MENU_STOP+1);
    assert(minerals==450 && gas==400 && u[muta].raw_owner==3 && u[muta].hp==100);
    assert(!strcmp(race_menu_label(MENU_STOP+1),"CANCEL MORPH"));
    u[muta].hp=80; choose_command(MENU_STOP+1);
    assert(u[muta].type==U_MUTALISK);
    assert(minerals==500 && gas==500 && u[muta].raw_owner==3 && u[muta].hp==48);
    int enemy_spire=spawn_build(ENEMY,B_GREATER_SPIRE,350,200,0);
    int enemy_muta=spawn_unit(ENEMY,U_MUTALISK,400,200);
    assert(enemy_spire>=0 && enemy_muta>=0 && u[enemy_muta].raw_owner==4);
    assert(race_guardian_morph_start(enemy_muta));
    assert(race_minerals[ENEMY]==450 && race_gas[ENEMY]==400 && minerals==500 && gas==500);
    assert(race_guardian_morph_cancel(enemy_muta));
    assert(race_minerals[ENEMY]==500 && race_gas[ENEMY]==500);

    /* A source-like preplaced Cocoon has no paid job. Current source import
     * has no raw-59 mapping; future combined codecs must independently
     * authenticate every paid row and never infer payment from raw 59. */
    assert(sc64_race_unit_from_raw(59)<0);
    int preplaced=spawn_unit(SIDE,U_GUARDIAN_COCOON,450,200);
    assert(preplaced>=0); u[preplaced].raw_type=59;
    int before_ore=minerals,before_gas=gas;
    assert(!race_guardian_morph_cancel(preplaced));
    assert(minerals==before_ore && gas==before_gas);
    u[preplaced].alive=0;

    /* Real paid Egg -> Mutalisk -> Guardian command: no source-owner field
     * is synthesized by this fixture. */
    int hall=spawn_build(SIDE,B_HATCHERY,100,200,0);
    assert(hall>=0 && spawn_unit(SIDE,U_OVERLORD,100,100)>=0);
    b[hall].larva=1;
    assert(race_queue_unit(SIDE,hall,U_MUTALISK));
    int egg=-1;
    for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive && u[i].type==U_Z7_EGG && u[i].target==U_MUTALISK) egg=i;
    assert(egg>=0 && u[egg].raw_owner==3);
    while(u[egg].alive && u[egg].type==U_Z7_EGG) race_z7_egg_tick_unit(egg);
    assert(u[egg].alive && u[egg].type==U_MUTALISK && u[egg].raw_type==43 && u[egg].raw_owner==3);
    before_ore=minerals; before_gas=gas;
    assert(race_guardian_morph_start(egg));
    assert(minerals==before_ore-50 && gas==before_gas-100 && u[egg].raw_owner==3);
    /* Current-owner refund is an engine policy, not a measured N64 rule. */
    u[egg].owner=ENEMY; int enemy_ore=race_minerals[ENEMY],enemy_gas=race_gas[ENEMY];
    assert(race_guardian_morph_cancel(egg));
    assert(race_minerals[ENEMY]==enemy_ore+50 && race_gas[ENEMY]==enemy_gas+100);
    assert(minerals==before_ore-50 && gas==before_gas-100 && u[egg].raw_owner==3);
    puts("Guardian campaign source PUNI HOST stage passed");
    return 0;
}
