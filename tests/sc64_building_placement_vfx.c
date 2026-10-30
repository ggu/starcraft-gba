/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
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
    puts("building placement outline: PASS");
    return 0;
}
