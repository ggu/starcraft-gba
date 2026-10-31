/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include "../starcraft/sc64_sight_axis.h"

_Static_assert(sizeof(int)==4 && sizeof(unsigned)==4,
               "The GBA and test host use 32-bit int coordinates");

static uint32_t state=0x754ce129u;
static uint32_t random_word(void)
{
    state^=state<<13; state^=state>>17; state^=state<<5;
    return state;
}
static int signed_word(uint32_t bits)
{
    int value=(int)(bits&INT_MAX);
    return bits&0x80000000u ? -value-1 : value;
}
static uint64_t checks;
static void check(int position,int center,unsigned radius)
{
    int64_t delta=(int64_t)position-(int64_t)center;
    int expected=delta>=-(int64_t)radius && delta<=(int64_t)radius;
    assert(sc64_sight_axis_contains(position,center,radius)==expected);
    ++checks;
}
int main(void)
{
    static const int edges[]={INT_MIN,INT_MIN+1,-1000,-177,-176,-160,-1,
                              0,1,160,176,177,1000,INT_MAX-1,INT_MAX};
    static const unsigned radii[]={0,1,160,176,65535,INT_MAX,
                                   (unsigned)INT_MAX+1u,UINT_MAX};
    for(unsigned i=0;i<sizeof(edges)/sizeof(edges[0]);++i)
        for(unsigned j=0;j<sizeof(edges)/sizeof(edges[0]);++j)
            for(unsigned r=0;r<sizeof(radii)/sizeof(radii[0]);++r)
                check(edges[i],edges[j],radii[r]);
    /* Exhaustive source-pixel windows around every 8-pixel center in the
     * 2048-pixel world, including positions just beyond either map edge. */
    for(int center=8;center<2048;center+=8)
        for(int position=-256;position<=2304;++position) {
            check(position,center,160); check(position,center,176);
        }
    for(unsigned i=0;i<1000000u;++i) {
        int position=signed_word(random_word()),center=signed_word(random_word());
        check(position,center,random_word());
        check(position,center,(i&1u)?160u:176u);
    }
    printf("Sight axis matches 64-bit oracle: %" PRIu64 " comparisons\n",checks);
    return 0;
}
