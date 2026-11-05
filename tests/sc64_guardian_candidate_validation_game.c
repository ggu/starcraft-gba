/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed detached HOST candidates: no SRAM or guest state is seeded. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_guardian_candidate_validation.h"

static void bind_auth(Sc64StageSaveAuth *auth,RaceSave *candidate,int guardian)
{
    Sc64StageTagPair pair=guardian ?
        (Sc64StageTagPair){SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_SAVE_VERSION,SC64_STAGE_SAVE_GUARDIAN} :
        (Sc64StageTagPair){SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION,0};
    candidate->version=pair.inner;
    assert(sc64_stage_save_authenticate(auth,candidate,pair.outer,pair.inner,&pair,1));
}
static void put_unit(RaceSave *candidate,unsigned slot,const Unit *unit)
{
    Unit wire;
    assert(sc64_guardian_wire_encode(unit,&wire,1));
    assert(sc64_zp_unit_save_encode(&candidate->units[slot],&wire,2048,2048));
}
static void put_building(RaceSave *candidate,unsigned slot,const Building *building)
{ assert(sc64_building_save_encode(&candidate->buildings[slot],building,2048,2048)); }

int main(void)
{
    game_init(); memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    race_mode=0; race_human=R_ZERG; race_player[SIDE]=R_ZERG; race_player[ENEMY]=R_ZERG;
    race_selected_mission=SC64_Z7_GLOBAL_ID; race_z7_map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    minerals=1000; gas=1000;
    assert(race_guardian_source_map_valid(race_z7_map));
    assert(race_guardian_source_row_valid(205,0x2080)); /* real P6-style source flags */
    assert(!race_guardian_source_row_valid(133,0x2000));
    assert(!race_guardian_source_row_valid(137,0x2000));
    assert(!race_guardian_source_row_valid(59,0));
    Sc64ImportedUnit src={0}; Sc64RaceMap map=*race_z7_map;
    map.units=&src; map.unit_count=1; src.source_type=133; src.flags=0x2000;
    assert(!race_guardian_source_map_valid(&map));
    src.source_type=205; assert(race_guardian_source_map_valid(&map));
    src.source_type=59; src.flags=0; assert(!race_guardian_source_map_valid(&map));

    int lair=spawn_build(SIDE,B_LAIR,200,200,0);
    int nest=spawn_build(SIDE,B_ZP_QUEENS_NEST,280,200,0);
    int spire=spawn_build(SIDE,B_GREATER_SPIRE,360,200,0);
    int muta=spawn_unit(SIDE,U_MUTALISK,440,200);
    assert(lair>=0 && nest>=0 && spire>=0 && muta>=0);
    b[lair].larva=1;b[lair].larva_timer=123;b[lair].shield_remainder=17;
    assert(race_guardian_campaign_morph(lair,B_HIVE));
    assert(race_guardian_morph_start(muta));
    RaceSave candidate; memset(&candidate,0,sizeof(candidate));
    candidate.state[0]=0; candidate.state[1]=R_ZERG; candidate.state[2]=SC64_Z7_GLOBAL_ID;
    for(int i=0;i<5;++i) candidate.players[i]=R_ZERG;
    Sc64StageSaveAuth auth; bind_auth(&auth,&candidate,1);
    put_unit(&candidate,(unsigned)muta,&u[muta]);
    put_building(&candidate,(unsigned)lair,&b[lair]);
    RaceSave snapshot=candidate;
    /* A fresh HOME load may be inspecting an entirely different live world. */
    race_mode=1;race_human=R_PROTOSS;race_selected_mission=SC64_RACE_MISSION_Z4;
    assert(race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    assert(!memcmp(&candidate,&snapshot,sizeof(candidate)));
    Sc64StageSaveAuth wrong=auth; wrong.candidate=&snapshot;
    assert(!race_guardian_candidate_valid(&candidate,2048,2048,&wrong));
    candidate.state[2]=SC64_RACE_MAP_COUNT;
    memset(&candidate.units[muta],0,sizeof(candidate.units[muta]));
    memset(&candidate.buildings[lair],0,sizeof(candidate.buildings[lair]));
    assert(!race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    candidate=snapshot;
    assert(!memcmp(&candidate,&snapshot,sizeof(candidate)));
    Building bad=b[lair];bad.raw_owner=7;put_building(&candidate,(unsigned)lair,&bad);
    assert(!race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    candidate=snapshot;
    bad=b[lair];bad.hp=0;put_building(&candidate,(unsigned)lair,&bad);
    assert(!race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    candidate=snapshot;
    bad=b[lair];bad.alive=0;put_building(&candidate,(unsigned)lair,&bad);
    assert(!race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    candidate=snapshot;
    Unit wire;
    assert(sc64_guardian_wire_encode(&u[muta],&wire,1));wire.target=-1;
    assert(sc64_zp_unit_save_encode(&candidate.units[muta],&wire,2048,2048));
    assert(!race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    candidate=snapshot;
    /* The identical target marker under an old candidate is never paid. */
    memset(&candidate.units[muta],0,sizeof(candidate.units[muta]));
    bind_auth(&auth,&candidate,0);
    assert(!race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    /* But a marked unrelated source type remains plain source data. */
    bad=b[lair];bad.type=B_PYLON;bad.raw_type=156;bad.maxhp=sc64_race_building_max_hp(B_PYLON);
    bad.hp=bad.maxhp;bad.under_construction=0;bad.flags=0x2080;
    put_building(&candidate,(unsigned)lair,&bad);
    assert(race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    puts("Guardian detached candidate validation HOST stage passed");return 0;
}
