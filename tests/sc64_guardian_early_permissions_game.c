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
    puts("Guardian early source PUNI and owner mapping HOST stage passed");
    return 0;
}
