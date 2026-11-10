/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/sc64_building_placement_vfx.h"

static u8 screen[160][240];
static int calls;
void gfx_pixel(int x, int y, u8 color)
{
    assert(x >= 0 && x < 240 && y >= 12 && y < 120);
    screen[y][x] = color;
    ++calls;
}
static void clear(void) { memset(screen, 0, sizeof(screen)); calls = 0; }

/* The original per-pixel scan, kept as the oracle for the row-mask version. */
static u8 expected[160][240];
static void
reference_outline(int cx, int cy, int canvas_w, int canvas_h, int offset_x,
                  int offset_y, int w, int h, const u8 *pixels, u8 color)
{
    int64_t x0 = (int64_t)cx - canvas_w / 2 + offset_x;
    int64_t y0 = (int64_t)cy - canvas_h / 2 + offset_y;
    int left, right, top, bottom;
    if (x0 >= 240 || x0 + w <= 0 || y0 >= 120 || y0 + h <= 12) return;
    left = x0 < 0 ? (int)-x0 : 0;
    right = x0 + w > 240 ? (int)(240 - x0) : w;
    top = y0 < 12 ? (int)(12 - y0) : 0;
    bottom = y0 + h > 120 ? (int)(120 - y0) : h;
    for (int y = top; y < bottom; ++y)
        for (int x = left; x < right; ++x) {
            int p = y * w + x;
            if (!pixels[p]) continue;
            if (x == 0 || x == w - 1 || y == 0 || y == h - 1 ||
                !pixels[p - 1] || !pixels[p + 1] ||
                !pixels[p - w] || !pixels[p + w])
                expected[y0 + y][x0 + x] = color;
        }
}

static unsigned rng_state = 0x2026110au;
static unsigned rng(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state >> 8;
}

/* Random sprites of every width class and pointer alignment, at positions
 * that clip against each battlefield edge, must match the oracle exactly.
 * The sprite sits at the end of its allocation so an out-of-row read faults
 * under the address sanitizer. */
static void compare_with_reference(void)
{
    for (int trial = 0; trial < 6000; ++trial) {
        int w = 1 + (int)(rng() % 128), h = 1 + (int)(rng() % 128);
        int align = (int)(rng() % 4), density = (int)(rng() % 5);
        u8 *block = malloc((size_t)(w * h + align + 4));
        u8 *sprite;
        int cx, cy;
        assert(block);
        sprite = block + ((4 - ((uintptr_t)block & 3)) & 3) + align;
        /* Keep the sprite's last byte at the very end of the allocation. */
        {
            size_t slack = (size_t)(block + w * h + align + 4 - (sprite + w * h));
            sprite += slack; /* still inside the block */
            if (((uintptr_t)sprite & 3) != (unsigned)align) {
                /* The end-of-block position fixes the alignment instead. */
            }
        }
        for (int i = 0; i < w * h; ++i) {
            unsigned v = rng();
            sprite[i] = density == 0 ? (u8)(v & 1) :
                        density == 1 ? (u8)(v % 16 != 0) :
                        density == 2 ? (u8)(v % 16 == 0) :
                        density == 3 ? (u8)(1 + v % 255) : (u8)(v % 3 ? 7 : 0);
        }
        cx = (int)(rng() % 400) - 80;
        cy = (int)(rng() % 300) - 80;
        clear();
        memset(expected, 0, sizeof(expected));
        assert(sc64_building_placement_outline(cx, cy, w, h, 0, 0, w, h, sprite, C_GREEN));
        reference_outline(cx, cy, w, h, 0, 0, w, h, sprite, C_GREEN);
        assert(!memcmp(screen, expected, sizeof(screen)));
        free(block);
    }
}

int main(void)
{
    /* A stepped building silhouette with a window and solid interior. */
    static const u8 sprite[7 * 7] = {
        0,0,1,1,1,0,0,
        0,1,1,1,1,1,0,
        1,1,1,1,1,1,1,
        1,1,1,0,1,1,1,
        1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,
        1,1,1,1,1,1,1
    };
    clear();
    assert(sc64_building_placement_outline(100,60,12,10,2,1,7,7,sprite,C_GREEN));
    /* top-left is (96,56); corners are transparent, unlike a rectangle. */
    assert(screen[56][96] == 0 && screen[56][98] == C_GREEN);
    assert(screen[57][97] == C_GREEN && screen[57][99] == 0);
    assert(screen[59][99] == 0); /* window stays transparent */
    assert(screen[59][98] == C_GREEN && screen[58][99] == C_GREEN);
    assert(screen[60][98] == 0); /* opaque interior is not filled */
    assert(screen[62][96] == C_GREEN && screen[62][102] == C_GREEN);
    for (int y=0; y<160; ++y) for (int x=0; x<240; ++x)
        assert(!screen[y][x] || screen[y][x] == C_GREEN);

    clear();
    assert(sc64_building_placement_outline(2,12,7,7,0,0,7,7,sprite,C_RED));
    assert(calls > 0);
    for (int y=0; y<12; ++y) for(int x=0; x<240; ++x) assert(!screen[y][x]);
    clear();
    assert(sc64_building_placement_outline(239,119,7,7,0,0,7,7,sprite,C_RED));
    assert(calls > 0);
    for (int y=120; y<160; ++y) for(int x=0; x<240; ++x) assert(!screen[y][x]);
    clear();
    assert(sc64_building_placement_outline(INT_MIN,INT_MAX,7,7,INT_MIN,INT_MAX,7,7,sprite,C_GREEN));
    assert(!calls);
    assert(!sc64_building_placement_outline(0,0,7,7,0,0,0,7,sprite,C_GREEN));
    assert(!sc64_building_placement_outline(0,0,7,7,0,0,-1,7,sprite,C_GREEN));
    assert(!sc64_building_placement_outline(0,0,7,7,0,0,129,7,sprite,C_GREEN));
    assert(!sc64_building_placement_outline(0,0,129,7,0,0,7,7,sprite,C_GREEN));
    assert(!sc64_building_placement_outline(0,0,7,7,0,0,7,7,NULL,C_GREEN));
    assert(!calls);
    compare_with_reference();
    puts("building placement outline: PASS");
    return 0;
}
