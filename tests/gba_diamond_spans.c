/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free differential test for Mode 4 diamond spans. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/gba.c"

static u16 expected[2][19200];
static unsigned cases;

static void run_case(int x, int y, int rx, int ry, u8 color,
                     int top, int bottom, int page)
{
    for (int p = 0; p < 2; ++p)
        for (int i = 0; i < 19200; ++i) {
            u16 v = (u16)((i * 173u + p * 8191u + cases * 37u) ^ (i >> 3));
            host_fb[p][i] = expected[p][i] = v;
        }
    /* Select each physical backbuffer through the public begin/flip API. */
    if (gfx_backbuffer_index() != page) {
        gfx_end();
        gfx_begin_preserve();
    }
    if (gfx_backbuffer_index() != page) abort();
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(top, bottom);
#else
    (void)top; (void)bottom;
    top = 0; bottom = 160;
#endif
    if (rx >= 0 && ry >= 0 && rx <= 480 && ry <= 320) {
        for (int sy = top; sy < bottom; ++sy) {
            int dy = sy - y;
            if (dy < 0) dy = -dy;
            if (dy > ry) continue;
            /* Test every pixel against the original geometric inequality. */
            for (int sx = 0; sx < 240; ++sx) {
                int dx = sx - x;
                if (dx < 0) dx = -dx;
                if (ry ? dx * ry <= rx * (ry - dy) : dx <= rx) {
                    u16 *w = &expected[page][sy * 120 + sx / 2];
                    if (sx & 1) *w = (u16)((*w & 0x00ffu) | ((u16)color << 8));
                    else *w = (u16)((*w & 0xff00u) | color);
                }
            }
        }
    }
    gfx_diamond(x, y, rx, ry, color);
    for (int p = 0; p < 2; ++p)
        for (int i = 0; i < 19200; ++i)
            if (host_fb[p][i] != expected[p][i]) {
                fprintf(stderr, "case %u: diamond (%d,%d) r=(%d,%d) clip=[%d,%d) page=%d mismatch page=%d x=%d y=%d got=%04x expected=%04x\n",
                        cases, x, y, rx, ry, top, bottom, page, p,
                        (i % 120) * 2, i / 120, host_fb[p][i], expected[p][i]);
                exit(1);
            }
    ++cases;
}

int main(void)
{
    static const int sizes[][2] = {
        {0,0}, {1,0}, {480,0}, {0,1}, {1,1}, {2,1}, {3,2},
        {7,3}, {11,7}, {16,8}, {31,31}, {63,32}, {239,120},
        {240,160}, {320,320}, {480,320}, {-1,1}, {1,-1}, {481,2}, {2,321}
    };
    static const int positions[][2] = {
        {0,0}, {1,1}, {119,79}, {120,80}, {239,159}, {240,160},
        {-20,80}, {260,80}, {120,-20}, {120,180}, {-500,-330}, {720,490}
    };
    static const int clips[][2] = {{0,160}, {12,120}, {40,41}, {80,80}};
    gba_init();
    gfx_begin_preserve();
    for (unsigned s = 0; s < sizeof sizes / sizeof sizes[0]; ++s)
        for (unsigned p = 0; p < sizeof positions / sizeof positions[0]; ++p)
            for (unsigned c = 0; c < sizeof clips / sizeof clips[0]; ++c)
                for (int page = 0; page < 2; ++page)
                    run_case(positions[p][0], positions[p][1], sizes[s][0], sizes[s][1],
                             (u8)(s * 29u + p * 7u), clips[c][0], clips[c][1], page);
    for (int ry = 1; ry <= 7; ++ry)
        for (int rx = 0; rx < 16; ++rx)
            for (int page = 0; page < 2; ++page)
                run_case(119 + page, 80, rx, ry, 93, 12, 120, page);
    printf("diamond differential: %u cases passed\n", cases);
    return 0;
}
