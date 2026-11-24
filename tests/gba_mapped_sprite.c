/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "gba.h"

static u8 lookup[255 * 256], source[256 * 256], expected[240 * 160];

#define MAP_COLS 32
#define MAP_ROWS 32
static int camx, camy;

static int currently_visible(int x, int y)
{
    return ((x / 16 + y / 16) % 3) != 0;
}

#include "../starcraft/sc64_weapon_source_vfx_render.h"

static void poison(void)
{
    gfx_begin(0);
    for (int y=0;y<160;++y) for (int x=0;x<240;++x) {
        u8 value=(u8)(x*17+y*29);
        expected[y*240+x]=value;
        gfx_pixel(x,y,value);
    }
}

static void check(int x,int y,int width,int height,unsigned rows,
                  int top,int bottom)
{
    poison();
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(top,bottom);
#endif
    /* Independent scalar destination model, deliberately no halfword or
     * renderer helper. The lookup can output zero, which is opaque black. */
    for(int sy=0;sy<height;++sy) for(int sx=0;sx<width;++sx) {
        long dx=(long)x+sx,dy=(long)y+sy;
        unsigned strength=source[sy*width+sx];
        if(dx<0||dx>=240||dy<top||dy>=bottom||!strength||strength>rows)continue;
        unsigned offset=(unsigned)dy*240+(unsigned)dx;
        expected[offset]=lookup[(strength-1)*256+expected[offset]];
    }
    gfx_blit_sprite_mapped(x,y,width,height,source,lookup,rows);
    for(int py=0;py<160;++py) for(int px=0;px<240;++px)
        assert(gfx_test_pixel(px,py)==expected[py*240+px]);
}

static void check_fogged_effect(int left, int top, int width, int height,
                               unsigned modifier, int reverse)
{
    Sc64WeaponSourceVfxRaster raster = {
        .pixels = source, .lookup = lookup, .lookup_rows = 40,
        .width = width, .height = height, .canvas_w = width, .canvas_h = height,
        .modifier = modifier
    };
    poison();
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(19, 103);
    const int clip_top = 19, clip_bottom = 103;
#else
    const int clip_top = 12, clip_bottom = 120;
#endif
    for (int row = 0; row < height; ++row) {
        int y = top + row;
        int world_y = y + camy - 12;
        if (y < clip_top || y >= clip_bottom || world_y < 0 || world_y >= 512) continue;
        for (int column = 0; column < width; ++column) {
            int x = left + column;
            int world_x = x + camx;
            if (x < 0 || x >= 240 || world_x < 0 || world_x >= 512) continue;
            if (((world_x / 16) + (world_y / 16)) % 3 == 0) continue;
            unsigned strength = source[row * width + (reverse ? width - 1 - column : column)];
            if (!strength || strength > 40) continue;
            unsigned index = y * 240 + x;
            expected[index] = lookup[(strength - 1) * 256 + expected[index]];
        }
    }
    sc64_weapon_source_vfx_draw_raster(&raster, left + camx + width / 2,
                                      top + camy - 12 + height / 2, reverse);
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            assert(gfx_test_pixel(x, y) == expected[y * 240 + x]);
}

int main(void)
{
    static const int positions[][2]={{-256,-256},{-17,-3},{-1,11},{0,0},
        {1,12},{17,30},{238,119},{239,159},{240,160},{INT_MIN,INT_MIN},
        {INT_MAX,INT_MAX}};
    static const int dimensions[][2]={{1,1},{2,3},{11,13},{76,76},{256,256}};
#ifdef GBA_SCENE_CLIP
    static const int clips[][2]={{0,160},{12,120},{80,80}};
#else
    static const int clips[][2]={{0,160}};
#endif
    static const unsigned row_counts[]={1,40,255};
    gba_init();
    for(unsigned row=0;row<255;++row) for(unsigned old=0;old<256;++old)
        lookup[row*256+old]=(u8)((old*3)^(row*7));
    for(unsigned i=0;i<sizeof(source);++i)source[i]=(u8)(i*13+i/17);
    unsigned cases=0;
    for(unsigned p=0;p<sizeof(positions)/sizeof(*positions);++p)
        for(unsigned d=0;d<sizeof(dimensions)/sizeof(*dimensions);++d)
            for(unsigned c=0;c<sizeof(clips)/sizeof(*clips);++c)
                for(unsigned r=0;r<sizeof(row_counts)/sizeof(*row_counts);++r) {
                    check(positions[p][0],positions[p][1],dimensions[d][0],
                          dimensions[d][1],row_counts[r],clips[c][0],clips[c][1]);
                    ++cases;
                }
    poison();
    gfx_blit_sprite_mapped(0,0,1,1,NULL,lookup,1);
    gfx_blit_sprite_mapped(0,0,1,1,source,NULL,1);
    gfx_blit_sprite_mapped(0,0,1,1,source,lookup,0);
    gfx_blit_sprite_mapped(0,0,1,1,source,lookup,256);
    gfx_blit_sprite_mapped(0,0,257,1,source,lookup,1);
    gfx_blit_sprite_mapped(0,0,1,257,source,lookup,1);
    gfx_blit_sprite_mapped(0,0,0,1,source,lookup,1);
    gfx_blit_sprite_mapped(0,0,1,-1,source,lookup,1);
    for(int py=0;py<160;++py)for(int px=0;px<240;++px)
        assert(gfx_test_pixel(px,py)==expected[py*240+px]);
    printf("mapped sprite: %u scalar-oracle clip/alignment/strength cases passed\n",cases);

    for (unsigned modifier = 9; modifier <= 10; ++modifier) {
        for (int reverse = 0; reverse < 2; ++reverse) {
            camx = 5; camy = 7;
            check_fogged_effect(1, 17, 57, 39, modifier, reverse);
            check_fogged_effect(-7, 7, 256, 39, modifier, reverse);
            camx = 487; camy = 472;
            check_fogged_effect(-1, 21, 57, 46, modifier, reverse);
        }
    }
    puts("mapped weapon effects: fog, mirror, palette and clipping oracle passed");
    return 0;
}
