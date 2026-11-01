/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdint.h>
typedef int16_t s16; typedef uint32_t u32;
#include "../starcraft/sc64_battlecruiser_yamato_launch_rules.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    for(unsigned h=0;h<256;++h) for(unsigned sample=0;sample<256;++sample) {
        Sc64BcLaunchResolution r;
        assert(sc64_bc_yamato_resolve_launch(h,0,4095,1,1,1,sample,&r));
        assert(r.threshold==0 && r.missed==(sample==0) && r.heading==h);
        assert(r.x>=-30 && r.x<=30 && r.y>=4065 && r.y<=4125);
        assert(sc64_bc_yamato_resolve_launch(h,200,200,0,1,0,sample,&r));
        assert(r.threshold==119 && r.missed==(sample<=119));
        assert(sc64_bc_yamato_resolve_launch(h,200,200,0,0,1,sample,&r));
        assert(r.threshold==255 && r.missed);
    }
    Sc64BcLaunchResolution out={3,4,5,6,7},before=out;
    assert(!sc64_bc_yamato_resolve_launch(256,0,0,0,0,0,0,&out));
    assert(out.x==before.x && out.y==before.y && out.heading==before.heading);
    puts("Yamato launch miss thresholds and 256 heading offsets pass");
}
