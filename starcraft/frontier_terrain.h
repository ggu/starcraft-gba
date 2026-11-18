/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Coalesce adjacent opaque ground and fog cells without changing barriers. */
static void
frontier_draw_terrain(int col0, int row0, int col1, int row1)
{
    for (int row = row0; row < row1; ++row) {
        int run_x = 0, run_width = 0, run_color = -1;
        int y = 12 + row * 16 - camy;
        for (int col = col0; col <= col1; ++col) {
            int x = col * 16 - camx, color = -1, blocked = 0;
            if (col < col1 && x < 240 && x + 16 > 0 && y < 120 && y + 16 > 12) {
                int idx = col + row * MAP_COLS;
                if (SC64_GRID_GET(visible, idx)) {
                    blocked = terrain_blocked(col, row);
                    if (!blocked) color = C_GRASS;
                } else if (SC64_GRID_GET(explored, idx)) color = C_FOG;
            }
            if (run_width && color != run_color) {
                gfx_rect(run_x, y, run_width, 16, (u8)run_color);
                run_width = 0;
            }
            if (color >= 0) {
                if (!run_width) { run_x = x; run_color = color; }
                run_width += 16;
            } else if (blocked) {
                gfx_rect(x, y, 16, 16, C_STONE);
                gfx_line(x + 2, y + 11, x + 7, y + 5, C_SILVER);
                gfx_line(x + 7, y + 5, x + 13, y + 10, C_DIRT);
            }
        }
    }
}
