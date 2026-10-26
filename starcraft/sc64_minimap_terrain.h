/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MINIMAP_TERRAIN_H
#define SC64_MINIMAP_TERRAIN_H

#include "gba.h"

#define SC64_MINIMAP_TERRAIN_SIDE 24

/* Render the imported mission's 24x24 visibility raster. Black is transparent
 * because the caller has already painted the minimap's black backing panel. */
static void
sc64_minimap_draw_terrain(int x, int y,
                          const u8 *visible, const u8 *explored)
{
#if defined(SC64_MINIMAP_BLIT)
    u8 pixels[SC64_MINIMAP_TERRAIN_SIDE * SC64_MINIMAP_TERRAIN_SIDE];
    unsigned nonempty = 0;
    for (int cell = 0; cell < SC64_MINIMAP_TERRAIN_SIDE * SC64_MINIMAP_TERRAIN_SIDE; ++cell) {
        unsigned color = visible[cell] ? C_GRASS : explored[cell] ? C_FOG : C_BLACK;
        pixels[cell] = (u8)color;
        nonempty |= color;
    }
    if (nonempty) gfx_blit_sprite(x, y, SC64_MINIMAP_TERRAIN_SIDE,
                                  SC64_MINIMAP_TERRAIN_SIDE, pixels);
#else
    for (int row = 0; row < SC64_MINIMAP_TERRAIN_SIDE; ++row)
        for (int col = 0; col < SC64_MINIMAP_TERRAIN_SIDE; ++col) {
            int cell = row * SC64_MINIMAP_TERRAIN_SIDE + col;
            if (visible[cell] || explored[cell])
                gfx_pixel(x + col, y + row, visible[cell] ? C_GRASS : C_FOG);
        }
#endif
}

#endif
