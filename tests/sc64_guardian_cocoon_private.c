/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "gba.h"
#define SC64_GUARDIAN_COCOON_RENDER_STAGE 1
#include "../starcraft/sc64_guardian_cocoon_render.h"
#include "sc64_guardian_cocoon_bank_generated.h"
int main(void)
{
    gba_init();int bg=-1;
    for (int i=1;i<256;i++) if(cocoon_shadow_lut[i]!=i) {bg=i;break;}
    assert(bg>=0);gfx_begin((u8)bg);gfx_set_clip_y(12,120);
    assert(sc64_cocoon_draw(&cocoon_bank,7,750,100,40,100,40,0));
    int body=0,underlay=0;
    for(int y=12;y<120;y++)for(int x=50;x<150;x++)
        if(gfx_test_pixel(x,y)!=bg) {if(y<64)body++;else underlay++;}
    assert(body>0 && underlay>0);
    for(int t=0;t<=750;t++) for(unsigned slot=0;slot<8;slot++)
        assert(sc64_cocoon_resolve(&cocoon_bank,slot,t));
    printf("private Cocoon bank body=%d underlay=%d timeline=%u bytes\n",body,underlay,8u*751u);
    return 0;
}
