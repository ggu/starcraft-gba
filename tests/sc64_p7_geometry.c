#include "../common/gba.h"
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_p7_geometry.h"

#include <assert.h>
#include <limits.h>

/* Raw mappings and pinned dimensions are exercised directly: this fixture
 * has no imported P7 scenario, world context, or sprite assets. */
static void expect_rect(unsigned raw,int x,int y,int building,
                        int l,int t,int r,int b)
{
    int actual_l,actual_t,actual_r,actual_b;
    assert(sc64_p7_actor_rect_source(raw,x,y,building,
                                     &actual_l,&actual_t,&actual_r,&actual_b));
    assert(actual_l==l && actual_t==t && actual_r==r && actual_b==b);
}

int main(void)
{
#ifndef SC64_P7_ACTORS
    return 2;
#else
    expect_rect(68,100,100,0,184,184,216,216); /* Archon source bounds */
    expect_rect(71,100,100,0,178,178,222,222); /* Arbiter */
    expect_rect(72,100,100,0,168,168,232,232); /* Carrier */
    expect_rect(84,100,100,0,184,184,216,216); /* Observer */
    expect_rect(159,100,100,1,156,184,245,229); /* Observatory */
    expect_rect(154,100,100,1,144,161,257,240); /* Nexus */
    assert(!sc64_p7_actor_rect_source(87,100,100,0,
                                     &(int){0},&(int){0},&(int){0},&(int){0}));
    assert(!sc64_p7_actor_rect_source(154,100,100,0,
                                     &(int){0},&(int){0},&(int){0},&(int){0}));
    assert(!sc64_p7_actor_rect_source(154,100,100,1,0,0,0,0));
    assert(!sc64_p7_actor_rect_source(68,INT_MAX,100,0,
                                     &(int){0},&(int){0},&(int){0},&(int){0}));
    assert(!sc64_p7_actor_rect_source(68,100,INT_MAX,0,
                                     &(int){0},&(int){0},&(int){0},&(int){0}));
    return 0;
#endif
}
