/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST-only candidate, distinct Guardian roster. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_guardian_candidate_validation.h"

static void put(RaceSave *candidate,unsigned slot,const Unit *unit)
{
    Unit wire;assert(sc64_guardian_wire_encode(unit,&wire,1));
    assert(sc64_unit_save_encode(&candidate->units[slot],&wire,2048,2048));
}
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();
    int spire=spawn_build(SIDE,B_GREATER_SPIRE,200,200,0);
    int slot=spawn_unit(SIDE,U_MUTALISK,250,200);
    assert(spire>=0 && slot>=0);minerals=500;gas=500;
    assert(race_guardian_morph_start(slot));u[slot].work_timer=1;
    assert(race_guardian_morph_tick(slot));
    assert(u[slot].type==U_GUARDIAN_STAGE && u[slot].raw_type==44 && u[slot].raw_owner==-1);
    RaceSave candidate;memset(&candidate,0,sizeof(candidate));
    candidate.state[0]=1;candidate.state[1]=R_ZERG;candidate.state[4]=1;
    candidate.players[SIDE]=R_ZERG;candidate.players[ENEMY]=R_PROTOSS;candidate.players[4]=R_ZERG;
    Sc64StageTagPair pair={SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_SAVE_VERSION,SC64_STAGE_SAVE_GUARDIAN};
    candidate.version=pair.inner;Sc64StageSaveAuth auth;
    assert(sc64_stage_save_authenticate(&auth,&candidate,pair.outer,pair.inner,&pair,1));
    put(&candidate,(unsigned)slot,&u[slot]);
    race_mode=0;race_human=R_TER;race_selected_mission=SC64_RACE_MISSION_T1;
    assert(race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    Unit captured=u[slot];captured.owner=ENEMY;put(&candidate,(unsigned)slot,&captured);
    assert(race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    captured.owner=4;put(&candidate,(unsigned)slot,&captured);
    assert(!race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    captured.owner=SIDE;captured.raw_owner=3;put(&candidate,(unsigned)slot,&captured);
    assert(!race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    candidate.state[0]=0;candidate.state[2]=SC64_RACE_MISSION_T1;
    assert(!race_guardian_candidate_valid(&candidate,2048,2048,&auth));
    puts("Guardian distinct candidate provenance HOST stage passed");return 0;
}
