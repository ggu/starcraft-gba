/* SPDX-License-Identifier: GPL-3.0-or-later
 * Constructed host integration: actual Building ABI and journal level type.
 * The stage is not connected to menus, live update or save/load. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
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
    assert(sc64_pr_job_valid_custom(&q,0,&levels,1,0));
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,0)==0 && q.queue_progress==5600);
    q.owner=1;
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,1)==-1 && q.queue_progress==5600);
    q.owner=0; q.queue_progress=1;
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,1)==1);
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
    puts("research job staged actual Building admission and tick: ok");
    return 0;
}
