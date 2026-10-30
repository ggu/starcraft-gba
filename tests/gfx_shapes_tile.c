/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independent tile buffer and dirty-mask oracle for axis-aligned lines. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gba.h"
static u8 px[240*160]; static u32 dirty[20], drawn[20], art[20]; static u8 top,bottom;
static void snap(void){memcpy(px,gfx_tile_buffer_pixels(),sizeof(px));memcpy(dirty,gfx_tile_dirty,sizeof(dirty));memcpy(drawn,gfx_tile_drawn,sizeof(drawn));memcpy(art,gfx_tile_art_marks,sizeof(art));top=gfx_tile_band_top;bottom=gfx_tile_band_bottom;}
static void same(void){assert(!memcmp(px,gfx_tile_buffer_pixels(),sizeof(px)));assert(!memcmp(dirty,gfx_tile_dirty,sizeof(dirty)));assert(!memcmp(drawn,gfx_tile_drawn,sizeof(drawn)));assert(!memcmp(art,gfx_tile_art_marks,sizeof(art)));assert(top==gfx_tile_band_top&&bottom==gfx_tile_band_bottom);}
static void ref(int x0,int y0,int x1,int y1,u8 c){int dx=x1>x0?x1-x0:x0-x1,dy=y1>y0?y0-y1:y1-y0,sx=x0<x1?1:-1,sy=y0<y1?1:-1,e=dx+dy;for(;;){gfx_pixel(x0,y0,c);if(x0==x1&&y0==y1)break;int t=2*e;if(t>=dy){e+=dy;x0+=sx;}if(t<=dx){e+=dx;y0+=sy;}}}
static unsigned r=12345;static unsigned next(void){r=r*1664525u+1013904223u;return r;}
int main(void){gba_init();for(int n=0;n<1000;n++){int x=(int)(next()%400)-80,y=(int)(next()%300)-70,a=(int)(next()%400)-80,b=(int)(next()%300)-70;u8 c=(u8)next();int clip=next()%161,hi=clip+(int)(next()%(161-clip));if(n%2)a=x;else b=y;int capture=n&2;
 gfx_tile_enter();gfx_set_clip_y(clip,hi);gfx_tile_art_capture(capture);ref(x,y,a,b,c);snap();gfx_tile_leave();
 gfx_tile_enter();gfx_set_clip_y(clip,hi);gfx_tile_art_capture(capture);gfx_line(x,y,a,b,c);same();gfx_tile_leave();}
 puts("tile renderer axis oracle: 1000 full buffer/dirty/drawn/art/band cases passed");}
