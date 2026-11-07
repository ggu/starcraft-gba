/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "gba.h"
#define SC64_GUARDIAN_COCOON_RENDER_STAGE 1
#include "../starcraft/sc64_guardian_cocoon_render.h"
static unsigned char timeline[751],lut[256];
static const unsigned char primary[4]={3,3,3,3},shadow[4]={1,1,1,1};
static int visible_left(int x,int y) { (void)y;return x<10; }
int main(void)
{
    const Sc64CocoonFrame frame={0,0,2,2,2,2,primary,shadow};
    Sc64CocoonBank bank={&frame,1,timeline,1,751,lut,0,1};
    gba_init();for(int i=0;i<256;i++)lut[i]=(unsigned char)(i+1);
    for(int page=0;page<2;page++) {
        gfx_begin(20);gfx_set_clip_y(12,120);
        assert(sc64_cocoon_draw(&bank,0,750,10,20,10,20,0));
        assert(gfx_test_pixel(9,19)==3 && gfx_test_pixel(9,20)==3);
        assert(gfx_test_pixel(9,21)==21 && gfx_test_pixel(11,21)==20);
        gfx_end();
    }
    gfx_begin(20);gfx_set_clip_y(12,120);
    assert(sc64_cocoon_draw(&bank,0,1,10,20,10,20,visible_left));
    assert(gfx_test_pixel(9,19)==3 && gfx_test_pixel(10,19)==20);
    assert(gfx_test_pixel(9,21)==21 && gfx_test_pixel(10,21)==20);
    gfx_begin(20);gfx_set_clip_y(20,21);
    assert(sc64_cocoon_draw(&bank,0,0,10,20,10,20,0));
    assert(gfx_test_pixel(9,19)==20 && gfx_test_pixel(9,20)==3 && gfx_test_pixel(9,21)==20);
    bank.shadow_lut=0;assert(!sc64_cocoon_draw(&bank,0,0,10,20,10,20,0));
    bank.shadow_lut=lut;assert(!sc64_cocoon_draw(&bank,0,751,10,20,10,20,0));
    timeline[0]=1;assert(!sc64_cocoon_draw(&bank,0,750,10,20,10,20,0));
    puts("Cocoon authored bank layering and clip passed");return 0;
}
