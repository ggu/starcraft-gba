/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independent 64-bit oracle for the 24 Hz source / 30 Hz GBA clock mapping. */
#include "../starcraft/sc64_weapon_source_vfx.h"
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static uint64_t checked;
static uint32_t random_state=UINT32_C(0x91e10da5);

static void check(uint32_t n)
{
    /* Preserve the original wider mathematical expressions as the oracle. */
    uint32_t expected_tick=(uint32_t)(((uint64_t)n*4u)/5u);
    uint64_t exact_inverse=((uint64_t)n*5u+3u)/4u;
    uint32_t expected_age=exact_inverse>UINT32_MAX?UINT32_MAX:(uint32_t)exact_inverse;
    uint32_t tick=sc64_weapon_source_vfx_source_tick(n);
    uint32_t age=sc64_weapon_source_vfx_first_gba_age(n);
    if(tick!=expected_tick || age!=expected_age || sc64_weapon_source_vfx_gba_duration(n)!=expected_age) {
        fprintf(stderr,"n=%" PRIu32 " source=%" PRIu32 "/%" PRIu32 " inverse=%" PRIu32 "/%" PRIu32 "\n",
                n,tick,expected_tick,age,expected_age);
        assert(0);
    }
    if(exact_inverse<=UINT32_MAX) {
        assert(sc64_weapon_source_vfx_source_tick(age)>=n);
        if(age) assert(sc64_weapon_source_vfx_source_tick(age-1)<n);
    }
    ++checked;
}

static void neighborhood(uint32_t center)
{
    for(int64_t delta=-1024;delta<=1024;++delta) {
        int64_t value=(int64_t)center+delta;
        if(value>=0 && (uint64_t)value<=UINT32_MAX)check((uint32_t)value);
    }
}

int main(void)
{
    for(uint32_t n=0;n<=(UINT32_C(1)<<20);++n)check(n);
    /* Saturation transition and multiplication boundaries, including every
     * nearby modulo-4 and modulo-5 residue. */
    static const uint32_t centers[]={0,1,4,5,UINT32_MAX/4,UINT32_MAX/5,
        (uint32_t)(((uint64_t)UINT32_MAX*4u)/5u),UINT32_MAX/2,UINT32_MAX-1,UINT32_MAX};
    for(unsigned i=0;i<sizeof(centers)/sizeof(centers[0]);++i)neighborhood(centers[i]);
    for(uint32_t i=0;i<1000000u;++i) {
        random_state^=random_state<<13;
        random_state^=random_state>>17;
        random_state^=random_state<<5;
        check(random_state);
    }
    printf("source clock: checked %" PRIu64 " values against 64-bit reference; inverse minimality holds when representable\n",checked);
    return 0;
}
