/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free whole-frame oracle for the bounded actor UI bar geometry. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../starcraft/game.c"

static void check_bar(int left, int top, int width, int value, int maximum,
                      int clip_top, int clip_bottom)
{
    enum { POISON=91, FULL=213, EMPTY=42 };
    int bar_top=top<12 ? 12 : top;
    int bar_bottom=top+2>120 ? 120 : top+2;
    int clamped=value<0 ? 0 : value>maximum ? maximum : value;
    int fill=maximum>0 ? (int)((int64_t)clamped*width/maximum) : 0;
    gfx_begin(POISON);
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(clip_top,clip_bottom);
#else
    (void)clip_top; (void)clip_bottom;
#endif
    sc64_draw_vital_bar(left,top,width,value,maximum,FULL,EMPTY);
    for(int y=0;y<160;++y) for(int x=0;x<240;++x) {
        int expected=POISON;
        int in_clip=1;
#ifdef GBA_SCENE_CLIP
        in_clip=y>=clip_top && y<clip_bottom;
#endif
        if(maximum>0 && in_clip && y>=bar_top && y<bar_bottom &&
           x>=left && x<left+width)
            expected=x<left+fill ? FULL : EMPTY;
        assert(gfx_test_pixel(x,y)==expected);
    }
}

int main(void)
{
    static const int maxima[]={-1,0,1,20,60,150,350,1000,100000};
    static const int widths[]={0,12,18};
    static const int positions[][2]={{-5,10},{8,12},{230,119},
                                     {250,140},{20,50}};
    gba_init();
    unsigned cases=0;
    for(unsigned m=0;m<sizeof(maxima)/sizeof(*maxima);++m) {
        int maximum=maxima[m];
        int values[]={-1,0,1,maximum/3,maximum-1,maximum,maximum+1};
        for(unsigned w=0;w<sizeof(widths)/sizeof(*widths);++w)
            for(unsigned p=0;p<sizeof(positions)/sizeof(*positions);++p)
                for(unsigned v=0;v<sizeof(values)/sizeof(*values);++v)
                    for(unsigned clip=0;clip<2;++clip) {
                        check_bar(positions[p][0],positions[p][1],widths[w],
                                  values[v],maximum,clip ? 50 : 0,
                                  clip ? 51 : 160);
                        ++cases;
                    }
    }
    printf("Actor vital bars match the independent whole-frame oracle: %u cases\n",cases);
    return 0;
}
