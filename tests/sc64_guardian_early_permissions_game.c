/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST only: real private source tables, constructed candidate/live states. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_guardian_candidate_validation.h"
int main(void)
{
    static const int mission[]={0,1,14,15};
    game_init(); race_mode=0;race_human=R_ZERG;
    for (unsigned i=0;i<4;++i) {
        race_selected_mission=mission[i];
        assert(race_guardian_early_source_owner(mission[i],3)==SIDE);
        assert(!race_guardian_campaign_allowed(SIDE,3,44));
        assert(!race_guardian_campaign_allowed(ENEMY,3,138));
        assert(race_guardian_campaign_allowed(SIDE,3,138)==(mission[i]==15));
    }
    assert(race_guardian_early_source_owner(15,1)==ENEMY);
    assert(race_guardian_early_source_owner(15,2)==4);
    assert(race_guardian_early_source_owner(15,0)<0);
    assert(race_guardian_early_source_owner(15,8)<0);
    RaceSave candidate;memset(&candidate,0,sizeof(candidate));
    candidate.players[SIDE]=R_ZERG;candidate.players[ENEMY]=R_PROTOSS;
    candidate.state[0]=0;candidate.state[2]=15;
    race_mode=1;race_human=R_TER;race_selected_mission=SC64_Z7_GLOBAL_ID;
    assert(race_guardian_candidate_origin(&candidate,SIDE,3,138));
    assert(race_guardian_candidate_origin(&candidate,ENEMY,3,138)); /* capture policy */
    assert(!race_guardian_candidate_origin(&candidate,SIDE,3,44));
    assert(!race_guardian_candidate_origin(&candidate,SIDE,0,138));
    candidate.state[2]=14;
    assert(!race_guardian_candidate_origin(&candidate,SIDE,3,138));
    assert(!race_guardian_candidate_origin(&candidate,SIDE,3,44));

    /* Constructed HOST world: real paid Lair/Queen operations use source
     * slots assigned by the Z7 spawn hook, then the real early PUNI tables. */
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));race_mode=0;race_human=R_ZERG;
    race_player[SIDE]=R_ZERG;race_selected_mission=SC64_Z7_GLOBAL_ID;
    race_z7_map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];minerals=1000;gas=1000;
    int hall=spawn_build(SIDE,B_HATCHERY,200,200,0);
    int pool=spawn_build(SIDE,B_SPAWNING_POOL,260,200,0);
    int drone=spawn_unit(SIDE,U_DRONE,300,200);
    assert(hall>=0 && pool>=0 && drone>=0 && b[hall].raw_owner==3 && u[drone].raw_owner==3);
    race_selected_mission=0;int ore=minerals,vespene=gas;
    assert(!race_start_lair_morph(hall));
    assert(minerals==ore && gas==vespene && b[hall].type==B_HATCHERY);
    race_selected_mission=15;
    assert(race_start_lair_morph(hall));
    assert(minerals==ore-150 && gas==vespene-100 && b[hall].type==B_LAIR);
    race_complete_building(hall);
    int x=0,y=0,found=0;
    for(int yy=120;yy<=280 && !found;yy+=8) for(int xx=120;xx<=280;xx+=8) {
        int tx=xx,ty=yy;
        if(race_build_request_valid(SIDE,drone,B_ZP_QUEENS_NEST,&tx,&ty,0)) {
            x=tx;y=ty;found=1;break;
        }
    }
    assert(found);ore=minerals;vespene=gas;
    assert(race_start_build(SIDE,drone,B_ZP_QUEENS_NEST,x,y));
    assert(minerals==ore && gas==vespene);
    u[drone].x=x;u[drone].y=y;race_pending_build_tick(drone);
    assert(minerals==ore-150 && gas==vespene-100 && !u[drone].alive);
    int nest=-1;
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive && b[i].type==B_ZP_QUEENS_NEST) nest=i;
    assert(nest>=0 && b[nest].raw_owner==3 && b[nest].under_construction);
    puts("Guardian early source PUNI and owner mapping HOST stage passed");
    return 0;
}
