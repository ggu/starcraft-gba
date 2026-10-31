/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_scarab_clear_motion.h"
int main(void) {
    /* These source-frame positions are from the retained OpenBW far/east and
     * south probes, before either enters the arrival-clamping zone. */
    for (unsigned heading=64;heading<=128;heading+=64) {
        int32_t x=(heading==64?345:320)*256,y=(heading==64?320:345)*256;
        int32_t tx=(heading==64?448:320)*256,ty=(heading==64?320:448)*256;
        for(unsigned age=0;age<=3;age++) {
            assert(sc64_scarab_clear_motion(&x,&y,tx,ty,heading,age,1));
            printf("%u %u %d %d\n",heading,age,(int)(x/256),(int)(y/256));
        }
        int32_t bx=x,by=y;
        assert(!sc64_scarab_clear_motion(&x,&y,tx,ty,heading+1,4,1) && x==bx && y==by);
        assert(!sc64_scarab_clear_motion(&x,&y,tx,ty,heading,4,0) && x==bx && y==by);
        assert(!sc64_scarab_clear_motion(&x,&y,x+256,y+256,heading,4,1) && x==bx && y==by);
        assert(!sc64_scarab_clear_motion(&x,&y,tx,ty,heading,90,1) && x==bx && y==by);
    }
    return 0;
}
