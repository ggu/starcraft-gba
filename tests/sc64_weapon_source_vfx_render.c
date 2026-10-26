/* SPDX-License-Identifier: GPL-3.0-or-later */
#define GBA_HOST 1
#define MAP_COLS 32
#define MAP_ROWS 32
static int camx, camy;
static int currently_visible(int x, int y);
#include "../starcraft/sc64_weapon_source_vfx_render.h"

#include <assert.h>
#include <stddef.h>

typedef struct { int x, y, w, h, mapped; const uint8_t *src; unsigned rows; } Call;
static Call calls[32];
static unsigned call_count;

static int currently_visible(int x, int y)
{
    (void)y;
    return x < 16; /* Match the engine's tile-wide visibility mask. */
}

void gfx_blit_sprite(int x, int y, int w, int h, const u8 *src)
{
    assert(call_count < sizeof(calls) / sizeof(calls[0]));
    calls[call_count++] = (Call){x, y, w, h, 0, src, 0};
}

void gfx_blit_sprite_mapped(int x, int y, int w, int h, const u8 *src,
                            const u8 *lookup, unsigned rows)
{
    assert(lookup);
    assert(call_count < sizeof(calls) / sizeof(calls[0]));
    calls[call_count++] = (Call){x, y, w, h, 1, src, rows};
}

static void reset_calls(void) { call_count = 0; }

int main(void)
{
    const uint8_t ordinary[] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t tall[32];
    const uint8_t strengths[] = {1, 2, 3, 4};
    const uint8_t dark_mask[] = {1};
    const uint8_t lookup[40 * 256] = {0};
    Sc64WeaponSourceVfxRaster r = {
        .pixels = ordinary, .image_id = 531,
        .x = 1, .y = 0, .mirror_x = 1, .width = 4, .height = 2,
        .canvas_w = 6, .canvas_h = 4, .modifier = 0
    };

    /* Crop placement uses the full source canvas, and fog clips at the
     * visibility boundary without reading beyond either source row. */
    sc64_weapon_source_vfx_draw_raster(&r, 16, 20, 0);
    assert(call_count == 2);
    assert(calls[0].x == 14 && calls[0].y == 30 && calls[0].w == 2);
    assert(calls[0].src == ordinary && calls[1].src == ordinary + 4);

    /* Mirroring keeps the canvas center fixed and reverses source order. */
    reset_calls();
    r.pixels = ordinary;
    sc64_weapon_source_vfx_draw_raster(&r, 16, 20, 1);
    assert(call_count == 4);
    assert(calls[0].x == 14 && calls[0].w == 1 && calls[0].src == ordinary + 3);
    assert(calls[1].src == ordinary + 2);

    reset_calls();
    for (unsigned i = 0; i < sizeof(tall); ++i) tall[i] = (uint8_t)(i + 1);
    r.pixels = tall; r.x = 0; r.mirror_x = 0; r.width = 4; r.height = 8;
    r.canvas_w = 4; r.canvas_h = 8;
    sc64_weapon_source_vfx_draw_raster(&r, 16, -2, 0);
    assert(call_count == 2);
    assert(calls[0].y == 12 && calls[0].src == tall + 24);
    assert(calls[1].y == 13 && calls[1].src == tall + 28);
    reset_calls();
    sc64_weapon_source_vfx_draw_raster(&r, 16, 110, 0);
    assert(call_count == 2);
    assert(calls[0].y == 118 && calls[0].src == tall);
    assert(calls[1].y == 119 && calls[1].src == tall + 4);

    /* Modifier 9 routes raw strengths through the destination LUT; an
     * unknown modifier is rejected instead of drawing with guessed colors. */
    reset_calls();
    r.pixels = strengths;
    r.width = 4; r.height = 1; r.canvas_w = 4; r.canvas_h = 1;
    r.mirror_x = 0;
    r.x = r.y = 0; r.modifier = 9; r.lookup = lookup; r.lookup_rows = 40;
    sc64_weapon_source_vfx_draw_raster(&r, 10, 10, 0);
    assert(call_count == 1 && calls[0].mapped && calls[0].rows == 40);
    assert(calls[0].src == strengths && calls[0].w == 4);
    reset_calls();
    r.modifier = 7;
    sc64_weapon_source_vfx_draw_raster(&r, 10, 10, 0);
    assert(call_count == 0);
    r.pixels = dark_mask;
    r.modifier = 10; r.lookup = lookup; r.lookup_rows = 1;
    r.width = 1; r.height = 1; r.canvas_w = 1; r.canvas_h = 1;
    sc64_weapon_source_vfx_draw_raster(&r, 10, 10, 0);
    assert(call_count == 1 && calls[0].mapped && calls[0].rows == 1);
    assert(calls[0].src == dark_mask && *calls[0].src == 1);
    return 0;
}
