/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include "gba.h"
#include "sc64_minimap_terrain.h"

static u8 visible[SC64_MINIMAP_TERRAIN_SIDE * SC64_MINIMAP_TERRAIN_SIDE];
static u8 explored[SC64_MINIMAP_TERRAIN_SIDE * SC64_MINIMAP_TERRAIN_SIDE];
static u8 expected[240 * 160];

static void reference_terrain(int x, int y)
{
    for (int row = 0; row < SC64_MINIMAP_TERRAIN_SIDE; ++row)
        for (int col = 0; col < SC64_MINIMAP_TERRAIN_SIDE; ++col) {
            int cell = row * SC64_MINIMAP_TERRAIN_SIDE + col;
            if (visible[cell] || explored[cell])
                gfx_pixel(x + col, y + row, visible[cell] ? C_GRASS : C_FOG);
        }
}

static void poison_surface(void)
{
    gfx_begin(C_PURPLE);
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            gfx_pixel(x, y, (u8)(1 + ((x * 7 + y * 11) % 31)));
}

static void setup_pattern(unsigned mode)
{
    static unsigned rng = 0x2468ace1u;
    for (unsigned cell = 0; cell < sizeof(visible); ++cell) {
        switch (mode) {
        case 0: visible[cell] = 0; explored[cell] = 0; break;
        case 1: visible[cell] = 255; explored[cell] = 0; break;
        case 2:
            visible[cell] = ((cell % 24 + cell / 24) & 1u) ? 0 : 173;
            explored[cell] = ((cell % 24 + cell / 24) & 1u) ? 91 : 0;
            break;
        case 3: visible[cell] = 0; explored[cell] = (u8)(1 + cell % 255); break;
        default:
            rng = rng * 1664525u + 1013904223u; visible[cell] = (u8)(rng >> 24);
            rng = rng * 1664525u + 1013904223u; explored[cell] = (u8)(rng >> 24);
            break;
        }
    }
}

int main(void)
{
    static const int clips[4][2] = {{0, 160}, {120, 160}, {12, 120}, {130, 160}};
    static const int origins[][2] = {
        {196,129}, {197,129}, {198,129}, {199,129},
        {-4,129}, {-3,129}, {-2,129}, {-1,129},
        {220,129}, {221,129}, {222,129}, {223,129},
        {-24,129}, {240,129}, {197,-7}, {197,153},
    };
    unsigned cases = 0;
    gba_init();
    for (unsigned mode = 0; mode < 5; ++mode) {
        setup_pattern(mode);
        for (unsigned clip = 0; clip < 4; ++clip) {
            for (unsigned origin = 0; origin < sizeof(origins)/sizeof(origins[0]); ++origin) {
                int ox = origins[origin][0], oy = origins[origin][1];
                poison_surface();
                gfx_set_clip_y(clips[clip][0], clips[clip][1]);
                reference_terrain(ox, oy);
                for (int y = 0; y < 160; ++y)
                    for (int x = 0; x < 240; ++x)
                        expected[y * 240 + x] = gfx_test_pixel(x, y);

                poison_surface();
                gfx_set_clip_y(clips[clip][0], clips[clip][1]);
                sc64_minimap_draw_terrain(ox, oy, visible, explored);
                for (int y = 0; y < 160; ++y)
                    for (int x = 0; x < 240; ++x)
                        if (expected[y * 240 + x] != gfx_test_pixel(x, y)) {
                            fprintf(stderr, "terrain mismatch mode=%u clip=%u origin=%u x=%d y=%d\n",
                                    mode, clip, origin, x, y);
                            return 1;
                        }
                ++cases;
            }
        }
    }
    printf("minimap terrain helper: %u poisoned-background, phase and clip cases passed\n", cases);
    return 0;
}
