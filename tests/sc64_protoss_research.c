/* Fact, compact levels and job helpers; not a runtime or journal test. */
#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_protoss_research.h"
int main(void)
{
    Sc64ProtossResearchLevels levels={0}, candidate, before;
    Sc64ProtossResearchQuote q; uint8_t wire[SC64_PR_LEVEL_WIRE_BYTES];
    uint16_t remaining=0; int ore=1000,gas=1000;
    assert(sc64_pr_quote(SC64_PR_GROUND_WEAPONS,0,&q));
    assert(q.mineral==100 && q.gas==100 && q.ticks==5000 && q.max_level==3);
    assert(sc64_pr_quote(SC64_PR_GROUND_ARMOR,2,&q) && q.mineral==250 && q.ticks==6200);
    assert(sc64_pr_quote(SC64_PR_SHIELDS,1,&q) && q.mineral==300 && q.gas==300 && q.ticks==5600);
    assert(sc64_pr_quote(SC64_PR_SINGULARITY,0,&q) && q.ticks==3125);
    assert(!sc64_pr_quote(SC64_PR_SINGULARITY,1,&q));
    assert(!sc64_pr_begin(SC64_PR_GROUND_WEAPONS,0,0,&ore,&gas,&remaining));
    assert(ore==1000 && gas==1000 && !remaining);
    assert(sc64_pr_begin(SC64_PR_GROUND_WEAPONS,0,1,&ore,&gas,&remaining));
    assert(ore==900 && gas==900 && remaining==5000);
    assert(!sc64_pr_begin(SC64_PR_GROUND_WEAPONS,0,1,&ore,&gas,&remaining));
    assert(!sc64_pr_tick(SC64_PR_GROUND_WEAPONS,&levels.level[0][2],&remaining,0));
    assert(remaining==5000);
    for (int i=0;i<4999;++i) assert(!sc64_pr_tick(2,&levels.level[0][2],&remaining,1));
    assert(sc64_pr_tick(2,&levels.level[0][2],&remaining,1));
    assert(levels.level[0][2]==1 && levels.level[1][2]==0);
    assert(sc64_pr_begin(2,1,1,&ore,&gas,&remaining));
    assert(ore==750 && gas==750 && remaining==5600);
    levels.level[4][5]=1; levels.level[3][4]=3;
    assert(sc64_pr_levels_encode(wire,&levels,SC64_PR_CUSTOM_OWNERS)); memset(&candidate,0,sizeof candidate);
    assert(sc64_pr_levels_decode(&candidate,wire,SC64_PR_CUSTOM_OWNERS) && !memcmp(&levels,&candidate,sizeof levels));
    before=candidate; wire[7]|=0x80; /* nonzero unused custom owner */
    assert(!sc64_pr_levels_decode(&candidate,wire,SC64_PR_CUSTOM_OWNERS) && !memcmp(&candidate,&before,sizeof before));
    wire[7]&=0x0f; wire[7]=(uint8_t)((wire[7]&0xf3u)|0x08u); /* owner4 singularity = 2 */
    assert(!sc64_pr_levels_decode(&candidate,wire,SC64_PR_CUSTOM_OWNERS) && !memcmp(&candidate,&before,sizeof before));
    memset(&levels,0,sizeof levels); levels.level[0][2]=1; levels.level[4][2]=3; levels.level[7][5]=1;
    assert(sc64_pr_levels_encode(wire,&levels,SC64_PR_CAMPAIGN_OWNERS));
    assert(sc64_pr_levels_decode(&candidate,wire,SC64_PR_CAMPAIGN_OWNERS));
    assert(candidate.level[0][2]==1 && candidate.level[4][2]==3 && candidate.level[7][5]==1);
    before=candidate;
    assert(!sc64_pr_levels_decode(&candidate,wire,SC64_PR_CUSTOM_OWNERS) && !memcmp(&candidate,&before,sizeof before));
    return 0;
}
