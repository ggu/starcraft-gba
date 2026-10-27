/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_game_actor_facts.h"
#include "../starcraft/sc64_p7_sight_runtime.h"
#include "../starcraft/sc64_p7_sight_runtime.h" /* Include guard contract. */

#define IS_WORKER(type) sc64_race_is_worker(type)

/* Independent copy of the pre-table observer formula. Keep this expressed in
 * the observer adapter's original order rather than calling the shared helper. */
static int reference_radius(int type,int building)
{
    int source_pixels=building?sc64_zp_building_sight_source_pixels(type):
                               sc64_zp_actor_sight_source_pixels(type);
    if(source_pixels>0) return source_pixels/2;
    if(building)
        return (type==B_COMMAND||sc64_race_is_zerg_townhall(type)||
                type==B_NEXUS)?112:64;
    return IS_WORKER(type)?56:72;
}

static void fail(int type,int building,int got,int expected)
{
    fprintf(stderr,"radius mismatch type=%d building=%d got=%d expected=%d\n",
            type,building,got,expected);
    exit(1);
}
int main(void)
{
    unsigned checks=0;
    for(int building=0;building<=1;building++) {
        int count=building?SC_RB_COUNT:SC_RU_COUNT;
        for(int type=0;type<count;type++) {
            int expected=reference_radius(type,building);
            int got=sc64_p7_observer_sight_radius_runtime(type,building);
            if(got!=expected) fail(type,building,got,expected);
            ++checks;
        }
        for(int type=-8;type<count+8;type++) {
            if(type>=0&&type<count) continue;
            int expected=reference_radius(type,building);
            int got=sc64_p7_observer_sight_radius_runtime(type,building);
            if(got!=expected) fail(type,building,got,expected);
            ++checks;
        }
    }
    int low=sc64_p7_observer_sight_radius_runtime(INT_MIN,0);
    if(low!=reference_radius(INT_MIN,0)) fail(INT_MIN,0,low,reference_radius(INT_MIN,0));
    int high=sc64_p7_observer_sight_radius_runtime(INT_MAX,1);
    if(high!=reference_radius(INT_MAX,1)) fail(INT_MAX,1,high,reference_radius(INT_MAX,1));
    checks+=2;
#ifdef SC64_FACT_SIGHT_TABLES
    printf("production fact-table wrapper exact: checks=%u units=%d buildings=%d\n",
           checks,SC_RU_COUNT,SC_RB_COUNT);
#else
    printf("policy-fallback wrapper exact: checks=%u units=%d buildings=%d\n",
           checks,SC_RU_COUNT,SC_RB_COUNT);
#endif
    return 0;
}
