/* Asset-free marker timing and raster checks.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define SC64_ORDER_MARKER_ART
#define SC64_ORDER_MARKER_BANK_HEADER "sc64_order_marker_synthetic.h"
#include "../starcraft/sc64_order_marker_vfx.h"
static u8 canvas[160][240];
static int calls;
void gfx_blit_sprite(int x, int y, int w, int h, const u8 *src)
{
    assert(x>=0 && x+w<=240 && y>=12 && y+h<=120 && w>0 && h>0);
    ++calls;
    for(int yy=0;yy<h;++yy) for(int xx=0;xx<w;++xx)
        canvas[y+yy][x+xx]=src[yy*w+xx];
}
static void reset(void) { memset(canvas,0,sizeof(canvas)); calls=0; }
int main(void)
{
    const unsigned expected[7]={4,4,3,2,1,0,0};
    unsigned phase;
    for(unsigned rem=1;rem<=7;++rem) {
        assert(sc64_order_marker_phase(rem,&phase));
        assert(phase==expected[rem-1]);
    }
    assert(!sc64_order_marker_phase(0,&phase));
    assert(!sc64_order_marker_phase(8,&phase));
    assert(!sc64_order_marker_phase(1,0));
    for (unsigned age=0;age<5;++age) {
        reset(); assert(sc64_order_marker_draw(20,30,age)); assert(calls==1);
        assert(canvas[25][13]==1 && canvas[28][17]==20);
    }
    reset(); assert(!sc64_order_marker_draw(20,30,5)); assert(!calls);
    /* Left/top clip retains source columns 2..4 and rows 1..3. */
    reset(); assert(sc64_order_marker_draw(5,16,0));
    assert(canvas[12][0]==8 && canvas[12][2]==10);
    assert(canvas[14][0]==18 && canvas[14][2]==20);
    assert(calls==3);
    reset(); assert(sc64_order_marker_draw(246,124,0));
    assert(canvas[119][239]==1 && calls==1);
    reset(); assert(sc64_order_marker_draw(-100,-100,0)); assert(!calls);
    puts("order marker timing and clipping: PASS");
    return 0;
}
