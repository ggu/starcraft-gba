/* SPDX-License-Identifier: GPL-3.0-or-later
 * Compare the optional axis paths to the original pixel-by-pixel line and
 * four-rectangle frame on complete poisoned framebuffers. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.h"
static u8 actual[240*160] __attribute__((aligned(4)));
static u8 expected[240*160] __attribute__((aligned(4)));
static unsigned rng=0x193e72a5u;
static unsigned next(void){rng=rng*1664525u+1013904223u;return rng;}
static int absref(int n){return n<0?-n:n;}
static void oldline(int x0,int y0,int x1,int y1,u8 c){int dx=absref(x1-x0),sx=x0<x1?1:-1,dy=-absref(y1-y0),sy=y0<y1?1:-1,err=dx+dy;for(;;){gfx_pixel(x0,y0,c);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}}
static void oldframe(int x,int y,int w,int h,u8 c){gfx_rect(x,y,w,1,c);gfx_rect(x,y+h-1,w,1,c);gfx_rect(x,y,1,h,c);gfx_rect(x+w-1,y,1,h,c);}
static void bg(void){gfx_set_clip_y(0,160);gfx_begin(0);for(int y=0;y<160;y+=3)for(int x=(y%7);x<240;x+=7)gfx_pixel(x,y,(u8)((x+y)%255+1));}
int main(void){gba_init(); for(int n=0;n<2500;n++){
 int x0=(int)(next()%440)-100,y0=(int)(next()%320)-80,x1=(int)(next()%440)-100,y1=(int)(next()%320)-80;
 int x=(int)(next()%320)-40,y=(int)(next()%220)-30,w=(int)(next()%100)+1,h=(int)(next()%80)+1;
 int top=(int)(next()%161),bottom=(int)(next()%161);if(top>bottom){int t=top;top=bottom;bottom=t;}u8 c=(u8)next();
 switch(n%4){case 0:x1=x0;break;case 1:y1=y0;break;case 2:x1=x0;y1=y0;break;}
 bg();gfx_set_clip_y(top,bottom);oldline(x0,y0,x1,y1,c);oldframe(x,y,w,h,c);assert(gfx_read_band(0,160,expected));
 bg();gfx_set_clip_y(top,bottom);gfx_line(x0,y0,x1,y1,c);gfx_frame(x,y,w,h,c);assert(gfx_read_band(0,160,actual));assert(!memcmp(actual,expected,sizeof(actual)));
 }
 /* Extreme axis endpoints are clipped before subtraction. */
 bg();gfx_line(INT_MIN,20,INT_MAX,20,7);for(int x=0;x<240;x++)assert(gfx_test_pixel(x,20)==7);
 bg();gfx_line(19,INT_MAX,19,INT_MIN,8);for(int y=0;y<160;y++)assert(gfx_test_pixel(19,y)==8);
 puts("shape framebuffer oracle: 2500 line/frame, clip, color and direction cases passed");return 0;}
