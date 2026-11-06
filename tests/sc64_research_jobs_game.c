/* SPDX-License-Identifier: GPL-3.0-or-later
 * Constructed host integration: actual Building ABI and journal level type.
 * The stage is not connected to menus, live update or save/load. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/game.c"
#ifndef SC64_EXPERIMENTAL_RESEARCH_JOBS
#error "requires host research job integration stage"
#endif

int main(void)
{
    Building q, saved;
    Sc64ProtossResearchLevels levels;
    int ore=1000, gas=1000;
    memset(&q,0,sizeof q); memset(&levels,0,sizeof levels);
    q.alive=1; q.owner=0; q.type=B_FORGE;
    saved=q;
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_SHIELDS,&levels,&ore,&ore,
        3,1,1,1,0,0,0,0,0));
    assert(!memcmp(&q,&saved,sizeof q) && ore==1000);
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_GROUND_WEAPONS,&levels,&ore,&gas,
        3,1,0,1,0,1,0,0,0)); /* unpowered */
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_GROUND_WEAPONS,&levels,&ore,&gas,
        3,1,1,0,0,1,0,0,0)); /* missing exact prerequisite */
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_GROUND_WEAPONS,&levels,&ore,&gas,
        3,1,1,1,0,1,0,1,0)); /* old generic weapon job still pending */
    assert(!memcmp(&q,&saved,sizeof q) && ore==1000 && gas==1000);
    assert(sc64_pr_job_begin_custom(&q,0,SC64_PR_GROUND_WEAPONS,&levels,&ore,&gas,
        3,1,1,1,0,1,0,0,0));
    assert(ore==850 && gas==850 && q.queue[1]==2 && q.queue_progress==5600);
    assert(sc64_pr_job_valid_custom(&q,0,&levels,1,0,0,0));
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,0,0,0)==0 && q.queue_progress==5600);
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,1,0,1)==-1 && q.queue_progress==5600);
    q.owner=1;
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,0,0,1)==-1 && q.queue_progress==5600);
    q.owner=0; q.queue_progress=1;
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,0,0,1)==1);
    assert(levels.level[0][SC64_PR_GROUND_WEAPONS]==2 && !q.level && !q.queue_progress);
    assert(sc64_pr_effective_level(SC64_PR_AIR_WEAPONS,0,1,0)==1);
    assert(sc64_pr_effective_level(SC64_PR_SHIELDS,0,1,2)==0);
    q.type=B_CYBERNETICS_CORE; saved=q;
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_SHIELDS,&levels,&ore,&gas,
        3,1,1,1,0,1,0,0,0)); /* shields require Forge */
    assert(!memcmp(&q,&saved,sizeof q));
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_AIR_WEAPONS,&levels,&ore,&gas,
        1,1,1,1,0,1,0,0,0)); /* already at legacy floor / cap */
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_AIR_WEAPONS,&levels,&ore,&gas,
        3,1,1,1,1,1,0,0,0)); /* duplicate */
    assert(!memcmp(&q,&saved,sizeof q));
    q.owner=1; saved=q;
    assert(!sc64_pr_job_begin_custom(&q,1,SC64_PR_AIR_WEAPONS,&levels,&ore,&gas,
        3,1,1,1,0,1,0,0,0)); /* human legacy floor must not affect an AI */
    assert(!memcmp(&q,&saved,sizeof q));
    /* Put an actual staged active job in the live array, then exercise
     * both public wrappers and direct race adapter entry points. */
    assert(sc64_pr_job_begin_custom(&q,1,SC64_PR_AIR_WEAPONS,&levels,&ore,&gas,
        3,1,1,1,0,0,0,0,0));
    b[0]=q;
    assert(b[0].level && b[0].queue_progress);
    static u8 previous_sram[32768];
    memset(host_save,0xa5,sizeof host_save);
    memcpy(previous_sram,host_save,sizeof host_save);
    assert(!save_game() && !race_save_game());
    assert(!memcmp(previous_sram,host_save,sizeof host_save));
    assert(!load_game() && !race_load_game());
    assert(!memcmp(previous_sram,host_save,sizeof host_save));

    race_mode=1; race_bots=1; race_player[1]=R_PROTOSS;
    memset(&sc64_experimental_research_levels,0,sizeof sc64_experimental_research_levels);
    race_minerals[1]=ore; race_gas[1]=gas;
    assert(!sc64_pr_stage_transfer_allowed(0,SIDE));
    assert(sc64_pr_stage_cancel_slot(0,0));
    assert(race_minerals[1]==ore+100 && race_gas[1]==gas+100);
    assert(!b[0].level && !b[0].queue_progress);
    assert(!sc64_pr_stage_cancel_slot(0,0));
    assert(race_minerals[1]==ore+100 && race_gas[1]==gas+100);

    assert(sc64_pr_job_begin_custom(&b[0],1,SC64_PR_AIR_WEAPONS,
        &sc64_experimental_research_levels,&race_minerals[1],&race_gas[1],
        3,1,1,1,0,0,0,0,0));
    int before_ore=race_minerals[1], before_gas=race_gas[1];
    b[0].owner=SIDE; saved=b[0];
    destroy_building(0,SIDE); /* wrong payer: preserve the producer and both banks */
    assert(!memcmp(&b[0],&saved,sizeof saved));
    assert(race_minerals[1]==before_ore && race_gas[1]==before_gas);
    b[0].owner=1; race_minerals[1]=65535; saved=b[0];
    destroy_building(0,SIDE); /* one overflowing resource blocks the whole transaction */
    assert(!memcmp(&b[0],&saved,sizeof saved));
    assert(race_minerals[1]==65535 && race_gas[1]==before_gas);
    race_minerals[1]=before_ore;
    destroy_building(0,SIDE);
    assert(!b[0].alive && !b[0].level && !b[0].queue_progress);
    assert(race_minerals[1]==before_ore+75 && race_gas[1]==before_gas+75);
    destroy_building(0,SIDE);
    assert(race_minerals[1]==before_ore+75 && race_gas[1]==before_gas+75);
    /* Human wallet uses the full signed-int range, unlike compact AI banks. */
    memset(&q,0,sizeof q); q.alive=1; q.owner=SIDE; q.type=B_FORGE;
    race_human=R_PROTOSS; race_player[SIDE]=R_PROTOSS; minerals=gas=1000;
    assert(sc64_pr_job_begin_custom(&q,SIDE,SC64_PR_SHIELDS,
        &sc64_experimental_research_levels,&minerals,&gas,3,1,1,1,0,0,0,0,0));
    b[1]=q; minerals=INT_MAX; int human_gas=gas; saved=b[1];
    assert(!sc64_pr_stage_cancel_slot(1,0));
    assert(!memcmp(&b[1],&saved,sizeof saved) && minerals==INT_MAX && gas==human_gas);
    minerals=70000; assert(sc64_pr_stage_cancel_slot(1,0));
    assert(minerals==70200 && gas==human_gas+200);
    b[0].level=1; b[0].queue_progress=1; b[0].queue[0]=1; b[0].queue[1]=1;
    saved=b[0];
    assert(spawn_build(SIDE,B_PYLON,100,100,0)==-1);
    assert(!memcmp(&b[0],&saved,sizeof saved));
    puts("research job staged admission, lifecycle and save refusal: ok");
    return 0;
}
