/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include "gba.h"
#define MAP_COLS 64
#define MAP_ROWS 64
static u8 visible[MAP_COLS * MAP_ROWS], explored[MAP_COLS * MAP_ROWS];
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#include "frontier_minimap.h"

static u8 expected[240 * 160];
static unsigned rng = 1;

static void reference(int x, int y)
{
    for (int row = 0; row < 24; ++row)
        for (int col = 0; col < 24; ++col) {
            int cell = ((2 * row + 1) * 64 / 48) * 64 + (2 * col + 1) * 64 / 48;
            if (visible[cell]) gfx_pixel(x + col, y + row, C_GRASS);
            else if (explored[cell]) gfx_pixel(x + col, y + row, C_FOG);
        }
}

int main(void)
{
    static const int origins[][2] = {{197,129}, {196,129}, {-3,129}, {223,129}, {197,-7}, {197,153}};
    gba_init();
    for (int i = 0; i < 24; ++i)
        if (frontier_minimap_sample[i] != (2 * i + 1) * 64 / 48) return 1;
    for (int sample = 0; sample < 64; ++sample) {
        for (int cell = 0; cell < 64 * 64; ++cell) {
            rng = rng * 1664525u + 1013904223u;
            visible[cell] = (u8)((rng >> 30) == 0);
            explored[cell] = (u8)((rng >> 28) & 1);
        }
        for (unsigned o = 0; o < sizeof(origins) / sizeof(origins[0]); ++o) {
            int x = origins[o][0], y = origins[o][1];
            gfx_begin(C_PURPLE);
            gfx_rect(x, y, 24, 24, C_BLACK);
            reference(x, y);
            for (int py = 0; py < 160; ++py) for (int px = 0; px < 240; ++px)
                expected[py * 240 + px] = gfx_test_pixel(px, py);
            gfx_begin(C_PURPLE);
            gfx_rect(x, y, 24, 24, C_BLACK);
            frontier_minimap_draw_terrain(x, y);
            for (int py = 0; py < 160; ++py) for (int px = 0; px < 240; ++px)
                if (expected[py * 240 + px] != gfx_test_pixel(px, py)) {
                    fprintf(stderr, "minimap mismatch sample=%d origin=%u pixel=%d,%d\n", sample, o, px, py);
                    return 1;
                }
        }
    }
    puts("Frontier minimap samples and raster match scalar output");
    return 0;
}
