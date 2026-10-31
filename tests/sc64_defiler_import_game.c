/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic authored record fed through the actual campaign loader. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#ifndef SC64_DEFILER_IMPORT_FIXTURE
#error "requires isolated campaign import fixture"
#endif
int main(void)
{
    game_init();race_mode=0;race_selected_mission=SC64_RACE_MISSION_P1;
    Sc64RaceMap map=sc64_race_maps[SC64_RACE_MISSION_P1];
    assert(map.width && map.height);
    Sc64ImportedUnit record={320,320,52,map.human_slot,100,0,0};
    map.units=&record;map.unit_count=1;sc64_defiler_import_fixture_map=&map;
    init_state();
    int slot=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].raw_type==52) {assert(slot<0);slot=i;}
    assert(slot>=0 && u[slot].type==U_SHARED_UNCLEAN_ONE && u[slot].owner==SIDE);
    assert(u[slot].raw_owner==map.human_slot && u[slot].hp==250 && u[slot].armor==3);
    assert(sc64_defiler_stage[slot].live && sc64_defiler_stage[slot].energy_q8==16000 &&
           sc64_defiler_stage[slot].heading==104 && !race_save_game());
    sc64_defiler_import_fixture_map=0;
    puts("synthetic raw52 authored record through actual campaign loader passed");return 0;
}
