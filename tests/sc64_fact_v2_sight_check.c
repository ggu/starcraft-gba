/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_game_actor_facts.h"
#include "../starcraft/sc64_p7_sight_policy.h"

#ifndef SC64_FACT_TABLES
#error "sight radius table requires SC64_FACT_TABLES"
#endif
#ifndef SC64_FACT_SIGHT_TABLES
#error "build this oracle with SC64_FACT_SIGHT_TABLES"
#endif

/* Repeat inclusion proves every feature accessor stays inside the header guard. */
#include "../starcraft/sc64_fact_tables.h"
#include "../starcraft/sc64_fact_tables.h"

static unsigned long checks;
static void expect(int ok,const char *kind,int type,int building)
{
    ++checks;
    if(!ok) {
        fprintf(stderr,"sight radius mismatch: %s type=%d building=%d\n",kind,type,building);
        exit(1);
    }
}
static int reference(int type,int building)
{
    int pixels=building ? sc64_zp_building_sight_source_pixels(type) :
                          sc64_zp_actor_sight_source_pixels(type);
    return sc64_p7_sight_radius_policy(type,building,pixels);
}
int main(void)
{
    int t,building;
    for(building=0;building<=1;++building) {
        int count=building ? SC_RB_COUNT : SC_RU_COUNT;
        for(t=0;t<count;++t)
            expect(sc64_fact_sight_radius_runtime(t,building)==
                   reference(t,building),"enum",t,building);
        for(t=-64;t<count+64;++t) {
            if(t>=0&&t<count) continue;
            expect(sc64_fact_sight_radius_runtime(t,building)==
                   reference(t,building),"out-of-range",t,building);
        }
        expect(sc64_fact_sight_radius_runtime(INT_MIN,building)==
               reference(INT_MIN,building),"INT_MIN",INT_MIN,building);
        expect(sc64_fact_sight_radius_runtime(INT_MAX,building)==
               reference(INT_MAX,building),"INT_MAX",INT_MAX,building);
        for(t=1;t<=7;t+=2)
            expect(sc64_p7_sight_radius_policy(17,building,t)==t/2,
                   "odd-source-pixels",t,building);
        expect(sc64_p7_sight_radius_policy(-1000,building,0)==
               (building?64:72),"zero-source-fallback",-1000,building);
    }
    printf("sight-radius table exact: units=%d buildings=%d checks=%lu\n",
           (int)SC_RU_COUNT,(int)SC_RB_COUNT,checks);
    return 0;
}
