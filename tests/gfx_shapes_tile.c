/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independent tile, framebuffer and dirty-mask oracles for lines and frames. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gba.h"
static u8 px[240*160]; static u32 dirty[20], drawn[20], art[20]; static u8 top,bottom;
static void snap(void){memcpy(px,gfx_tile_buffer_pixels(),sizeof(px));memcpy(dirty,gfx_tile_dirty,sizeof(dirty));memcpy(drawn,gfx_tile_drawn,sizeof(drawn));memcpy(art,gfx_tile_art_marks,sizeof(art));top=gfx_tile_band_top;bottom=gfx_tile_band_bottom;}
static void same(void){assert(!memcmp(px,gfx_tile_buffer_pixels(),sizeof(px)));assert(!memcmp(dirty,gfx_tile_dirty,sizeof(dirty)));assert(!memcmp(drawn,gfx_tile_drawn,sizeof(drawn)));assert(!memcmp(art,gfx_tile_art_marks,sizeof(art)));assert(top==gfx_tile_band_top&&bottom==gfx_tile_band_bottom);}
static void ref(int x0,int y0,int x1,int y1,u8 c){int dx=x1>x0?x1-x0:x0-x1,dy=y1>y0?y0-y1:y1-y0,sx=x0<x1?1:-1,sy=y0<y1?1:-1,e=dx+dy;for(;;){gfx_pixel(x0,y0,c);if(x0==x1&&y0==y1)break;int t=2*e;if(t>=dy){e+=dy;x0+=sx;}if(t<=dx){e+=dx;y0+=sy;}}}
static void ref_frame(int x, int y, int width, int height, u8 color)
{
    gfx_rect(x, y, width, 1, color);
    gfx_rect(x, y + height - 1, width, 1, color);
    gfx_rect(x, y, 1, height, color);
    gfx_rect(x + width - 1, y, 1, height, color);
}
static unsigned r=12345;static unsigned next(void){r=r*1664525u+1013904223u;return r;}
int main(void){gba_init();for(int n=0;n<1000;n++){int x=(int)(next()%400)-80,y=(int)(next()%300)-70,a=(int)(next()%400)-80,b=(int)(next()%300)-70;u8 c=(u8)next();int clip=next()%161,hi=clip+(int)(next()%(161-clip));if(n%2)a=x;else b=y;int capture=n&2;
 gfx_tile_enter();gfx_set_clip_y(clip,hi);gfx_tile_art_capture(capture);ref(x,y,a,b,c);snap();gfx_tile_leave();
 gfx_tile_enter();gfx_set_clip_y(clip,hi);gfx_tile_art_capture(capture);gfx_line(x,y,a,b,c);same();gfx_tile_leave();}

    static u8 expected[240 * 160], actual[240 * 160];
    for (int n = 0; n < 3000; ++n) {
        int x = (int)(next() % 320) - 40, y = (int)(next() % 220) - 30;
        int width = (int)(next() % 100) - 3, height = (int)(next() % 80) - 3;
        if (n % 6 == 0) width = n % 3;
        if (n % 11 == 0) height = n % 3;
        u8 color = (u8)next();
        int clip = next() % 161, high = clip + (int)(next() % (161 - clip));
        int capture = n & 2;

        gfx_tile_enter();
        gfx_set_clip_y(clip, high);
        gfx_tile_art_capture(capture);
        ref_frame(x, y, width, height, color);
        snap();
        gfx_tile_leave();
        gfx_tile_enter();
        gfx_set_clip_y(clip, high);
        gfx_tile_art_capture(capture);
        gfx_frame(x, y, width, height, color);
        same();
        gfx_tile_leave();

        gfx_set_clip_y(0, 160);
        gfx_begin(37);
        gfx_set_clip_y(clip, high);
        ref_frame(x, y, width, height, color);
        assert(gfx_read_band(0, 160, expected));
        gfx_set_clip_y(0, 160);
        gfx_begin(37);
        gfx_set_clip_y(clip, high);
        gfx_frame(x, y, width, height, color);
        assert(gfx_read_band(0, 160, actual));
        assert(!memcmp(actual, expected, sizeof(actual)));
        gfx_end();
    }

    puts("tile and framebuffer axis/frame oracles passed");
}
