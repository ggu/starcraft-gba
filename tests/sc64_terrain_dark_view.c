// SPDX-License-Identifier: GPL-3.0-or-later
#include <assert.h>
#include "../common/gba.h"

static int map_cols = 64, map_rows = 64;
#define MAP_COLS map_cols
#define MAP_ROWS map_rows
#define WORLD_W (MAP_COLS * 16)
#define WORLD_H (MAP_ROWS * 16)
#define SC64_GRID_GET(grid, cell) (((grid)[(unsigned)(cell) >> 3] >> ((cell) & 7)) & 1u)
static u8 visible[64 * 64 / 8], explored[64 * 64 / 8];
#include "../starcraft/sc64_terrain_cache.h"

static void expect_seen_cell(int x, int y, u8 *fog, int cell)
{
    fog[cell >> 3] |= (u8)(1u << (cell & 7));
    assert(!sc64_terrain_fill_dark_view(x, y));
    fog[cell >> 3] = 0;
}

int main(void)
{
    enum { TOP = SC64_TERRAIN_CACHE_Y, HEIGHT = SC64_TERRAIN_CACHE_HEIGHT };
    static _Alignas(4) u8 bitmap[240 * 160];
    gba_init();
    for (int phase = 0; phase < 256; ++phase) {
        int x = 16 + phase % 16, y = 32 + phase / 16;
        int right = (x + 239) / 16, bottom = (y + HEIGHT - 1) / 16;
        int corner = bottom * MAP_COLS + right;
        for (int page = 0; page < 2; ++page) {
            gfx_begin(C_WHITE);
            gfx_set_clip_y(TOP, TOP + HEIGHT);
            expect_seen_cell(x, y, visible, corner);
            expect_seen_cell(x, y, explored, corner); /* old fog memory */
            /* An explored neighbor outside the viewport contributes no pixels. */
            explored[(corner + 1) >> 3] = (u8)(1u << ((corner + 1) & 7));
            assert(sc64_terrain_fill_dark_view(x, y));
            explored[(corner + 1) >> 3] = 0;
            gfx_set_clip_y(0, 160);
            assert(gfx_read_band(0, 160, bitmap));
            for (int row = 0; row < 160; ++row)
                for (int col = 0; col < 240; ++col)
                    assert(bitmap[row * 240 + col] == (row >= TOP && row < TOP + HEIGHT ? C_BLACK : C_WHITE));
            gfx_end();
        }
    }
    gfx_begin(C_WHITE);
    gfx_set_clip_y(TOP, TOP + HEIGHT);
    int right = WORLD_W - 240, bottom = WORLD_H - HEIGHT;
    assert(sc64_terrain_fill_dark_view(0, 0));
    assert(sc64_terrain_fill_dark_view(right, bottom));
    assert(!sc64_terrain_fill_dark_view(-1, 0));
    assert(!sc64_terrain_fill_dark_view(0, -1));
    assert(!sc64_terrain_fill_dark_view(right + 1, bottom));
    assert(!sc64_terrain_fill_dark_view(right, bottom + 1));
    expect_seen_cell(right, bottom, explored, MAP_COLS * MAP_ROWS - 1);

    /* A wholly unknown visit leaves the previously captured view reusable. */
    explored[0] = 1;
    gfx_fill_rows(TOP, HEIGHT, C_WHITE);
    assert(!sc64_terrain_cached(0, 0));
    sc64_terrain_prepare(0, 0);
    assert(sc64_terrain_store(0, 0));
    assert(sc64_terrain_cached(right, bottom));
    assert(sc64_terrain_cached(0, 0));
    assert(gfx_read_band(TOP, HEIGHT, bitmap));
    assert(bitmap[0] == C_WHITE);
    explored[0] = 0;

    map_cols = 15;
    map_rows = 7;
    assert(sc64_terrain_fill_dark_view(0, WORLD_H - HEIGHT));
    expect_seen_cell(0, WORLD_H - HEIGHT, explored, MAP_COLS * MAP_ROWS - 1);
    --map_cols;
    assert(!sc64_terrain_fill_dark_view(0, 0));
    ++map_cols;
    --map_rows;
    assert(!sc64_terrain_fill_dark_view(0, 0));
    map_rows = 7;
    gfx_set_clip_y(TOP + 1, TOP + HEIGHT);
    assert(!sc64_terrain_fill_dark_view(0, 0));
    gfx_end();
    return 0;
}
