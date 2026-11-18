/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Differential oracle for the Frontier terrain-only painter. */
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

static u8 expected[240 * 160];
static unsigned rng = 1;
static unsigned next_random(void) { rng = rng * 1664525u + 1013904223u; return rng; }

static void scalar(int col0, int row0, int col1, int row1)
{
    for (int row = row0; row < row1; ++row)
        for (int col = col0; col < col1; ++col) {
            int idx = col + row * MAP_COLS;
            if (!SC64_GRID_GET(visible, idx) && !SC64_GRID_GET(explored, idx)) continue;
            int x = col * 16 - camx, y = 12 + row * 16 - camy;
            if (x >= 240 || x + 16 <= 0 || y >= 120 || y + 16 <= 12) continue;
            if (!SC64_GRID_GET(visible, idx)) gfx_rect(x, y, 16, 16, C_FOG);
            else if (terrain_blocked(col, row)) {
                gfx_rect(x, y, 16, 16, C_STONE);
                gfx_line(x + 2, y + 11, x + 7, y + 5, C_SILVER);
                gfx_line(x + 7, y + 5, x + 13, y + 10, C_DIRT);
            } else gfx_rect(x, y, 16, 16, C_GRASS);
        }
}

int main(void)
{
    gba_init();
    for (int map = 0; map < ORIGINAL_MAP_COUNT; ++map) {
        original_skirmish_map = map;
        for (int sample = 0; sample < 256; ++sample) {
            for (int i = 0; i < MAP_COLS * MAP_ROWS; ++i) {
                unsigned state = next_random() >> 30;
                SC64_GRID_SET(visible, i, state == 2 || state == 3);
                SC64_GRID_SET(explored, i, state != 0);
            }
            /* Exercise both edges, interior positions and every pixel phase. */
            int phase = sample & 15;
            camx = sample & 16 ? MAP_COLS * 16 - 240 - phase : phase + ((sample & 32) ? 160 : 0);
            camy = sample & 64 ? MAP_ROWS * 16 - 108 - phase : phase + ((sample & 128) ? 160 : 0);
            int col0 = limit(camx / 16, 0, MAP_COLS - 1);
            int col1 = limit((camx + 240) / 16 + 1, 0, MAP_COLS);
            int row0 = limit(camy / 16, 0, MAP_ROWS - 1);
            int row1 = limit((camy + 108) / 16 + 1, 0, MAP_ROWS);
            gfx_begin(37);
            scalar(col0, row0, col1, row1);
            for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x)
                expected[y * 240 + x] = gfx_test_pixel(x, y);
            gfx_begin(37);
            frontier_draw_terrain(col0, row0, col1, row1);
            for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x)
                if (gfx_test_pixel(x, y) != expected[y * 240 + x]) {
                    fprintf(stderr, "terrain mismatch map=%d sample=%d pixel=%d,%d\n", map, sample, x, y);
                    return 1;
                }
        }
    }
    puts("Frontier terrain spans match scalar across 512 map/camera/fog cases");
    return 0;
}
