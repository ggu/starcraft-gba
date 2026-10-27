/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_p7_sight_policy.h"

static unsigned long checks;
static int expected(int type, int building, int pixels)
{
    if (pixels>0) return pixels/2;
    if (building)
        return (type==B_COMMAND || sc64_race_is_zerg_townhall(type) ||
                type==B_NEXUS) ? 112 : 64;
    return sc64_race_is_worker(type) ? 56 : 72;
}
static void verify(int type, int building, int pixels)
{
    int got=sc64_p7_sight_radius_policy(type,building,pixels);
    int want=expected(type,building,pixels);
    ++checks;
    if (got!=want) {
        fprintf(stderr,"sight policy mismatch type=%d building=%d px=%d got=%d want=%d\n",
                type,building,pixels,got,want);
        exit(1);
    }
}
int main(void)
{
    unsigned state=0x93d765a1u;
    unsigned i;
    for (i=0;i<10000u;++i) {
        int type,building,pixels;
        state=state*1664525u+1013904223u;
        type=(int)(state%((unsigned)SC_RU_COUNT+256u))-128;
        building=(int)((state>>10)&1u);
        state=state*1664525u+1013904223u;
        pixels=(int)(state%260u)-2;
        verify(type,building,pixels);
    }
    verify(0,0,1); verify(0,0,3); verify(0,1,5);
    printf("P7 sight pure policy exact: %lu inputs; deterministic type/building/pixel matrix\n",
           checks);
    return 0;
}
