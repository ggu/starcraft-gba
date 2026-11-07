/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent per-pixel oracle, including both untouched pixels in Mode 4. */
#include "gba.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    _Alignas(4) u8 storage[260];
    const int ys[] = {-15, 0, 7, 11, 12, 105, 119, 159};
    const int clips[][2] = {{0,160},{12,120},{20,25},{159,160}};
    gba_init();
    for (int off=0;off<4;off++) for (int x=-17;x<=241;x++)
    for (unsigned yi=0;yi<sizeof(ys)/sizeof(ys[0]);yi++)
    for (unsigned ci=0;ci<sizeof(clips)/sizeof(clips[0]);ci++) {
        u8 *tile=storage+off;
        for (int i=0;i<256;i++) tile[i]=(u8)((i*37+x*11+off)&255);
        gfx_begin(93); gfx_set_clip_y(clips[ci][0],clips[ci][1]);
        gfx_blit16(x,ys[yi],tile);
        for(int py=0;py<160;py++) for(int px=0;px<240;px++) {
            int tx=px-x,ty=py-ys[yi];
            u8 expected=(tx>=0&&tx<16&&ty>=0&&ty<16&&
                         py>=clips[ci][0]&&py<clips[ci][1])?tile[ty*16+tx]:93;
            if(gfx_test_pixel(px,py)!=expected) {
                fprintf(stderr,"mismatch off=%d x=%d y=%d clip=%d..%d pixel=%d,%d\n",
                        off,x,ys[yi],clips[ci][0],clips[ci][1],px,py);
                return 1;
            }
        }
    }
    puts("odd tile full-frame oracle passed");
    return 0;
}
