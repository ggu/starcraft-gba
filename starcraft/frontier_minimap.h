/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Center samples for the fixed 64-by-64 Frontier map and 24-pixel minimap. */
_Static_assert(MAP_COLS == 64 && MAP_ROWS == 64, "Frontier minimap requires a 64-by-64 map");
static const u8 frontier_minimap_sample[24] = {
    1, 4, 6, 9, 12, 14, 17, 20, 22, 25, 28, 30,
    33, 36, 38, 41, 44, 46, 49, 52, 54, 57, 60, 62
};

static void
frontier_minimap_draw_terrain(int x, int y)
{
    static u8 raster[24 * 24];
    for (int row = 0; row < 24; ++row) {
        int base = frontier_minimap_sample[row] * MAP_COLS;
        for (int col = 0; col < 24; ++col) {
            int cell = base + frontier_minimap_sample[col];
            raster[row * 24 + col] = SC64_GRID_GET(visible, cell) ? C_GRASS :
                SC64_GRID_GET(explored, cell) ? C_FOG : C_BLACK;
        }
    }
    gfx_blit_opaque(x, y, 24, 24, raster);
}
