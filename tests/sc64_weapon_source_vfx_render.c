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
static Call calls[1024];
static unsigned call_count;
static unsigned visibility_query_count;
/* fog_mode 0: the original x < 16 mask; 1: visible[] tile grid. */
static int fog_mode;
static unsigned char visible_tiles[MAP_ROWS][MAP_COLS];
/* Painted screen: each blit copies its source bytes, so the whole-frame and
 * per-tile paths can be compared pixel for pixel. */
static int canvas[160][240];

static int currently_visible(int x, int y)
{
    ++visibility_query_count;
    if (fog_mode) return visible_tiles[y / 16][x / 16];
    (void)y;
    return x < 16; /* Match the engine's tile-wide visibility mask. */
}

static void paint(int x, int y, int w, int h, const u8 *src)
{
    for (int yy = 0; yy < h; ++yy) for (int xx = 0; xx < w; ++xx)
        if (x + xx >= 0 && x + xx < 240 && y + yy >= 0 && y + yy < 160)
            canvas[y + yy][x + xx] = src[yy * w + xx];
}

void gfx_reverse_bytes(u8 *dst, const u8 *src, unsigned n)
{
    for (unsigned i = 0; i < n; ++i) dst[i] = src[n - 1 - i];
}

void gfx_blit_sprite(int x, int y, int w, int h, const u8 *src)
{
    assert(call_count < sizeof(calls) / sizeof(calls[0]));
    calls[call_count++] = (Call){x, y, w, h, 0, src, 0};
    paint(x, y, w, h, src);
}

void gfx_blit_sprite_mapped(int x, int y, int w, int h, const u8 *src,
                            const u8 *lookup, unsigned rows)
{
    assert(lookup);
    assert(call_count < sizeof(calls) / sizeof(calls[0]));
    calls[call_count++] = (Call){x, y, w, h, 1, src, rows};
    paint(x, y, w, h, src);
}

/* Every pixel equals its source byte exactly where its map tile is visible
 * and it lies in battlefield rows 12..119; all others stay untouched. */
static void check_fogged_raster(const Sc64WeaponSourceVfxRaster *r, int wx, int wy, int hflip)
{
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x) canvas[y][x] = -1;
    call_count = 0;
    sc64_weapon_source_vfx_draw_raster(r, wx, wy, hflip);
    int src_x = hflip ? (int)r->mirror_x : r->x;
    const uint8_t *px = hflip && r->mirror_pixels ? r->mirror_pixels : r->pixels;
    int left = wx - camx - (int)r->canvas_w / 2 + src_x;
    int top = wy - camy + 12 - (int)r->canvas_h / 2 + (int)r->y;
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x) {
        int col = x - left, row = y - top, mx = x + camx, my = y + camy - 12;
        int expected = -1;
        if (col >= 0 && col < (int)r->width && row >= 0 && row < (int)r->height &&
            y >= 12 && y < 120 && mx >= 0 && mx < MAP_COLS * 16 && my >= 0 &&
            my < MAP_ROWS * 16 && visible_tiles[my / 16][mx / 16]) {
            int source_col = hflip && !r->mirror_pixels ? (int)r->width - 1 - col : col;
            expected = px[row * r->width + source_col];
        }
        assert(canvas[y][x] == expected);
    }
}

static void fog_paths_match_the_oracle(void)
{
    static uint8_t frame[24 * 20], mirrored[24 * 20];
    for (unsigned i = 0; i < sizeof(frame); ++i) {
        frame[i] = (uint8_t)(1 + i % 251);
        mirrored[i] = (uint8_t)(1 + (i * 7) % 251);
    }
    Sc64WeaponSourceVfxRaster r = {
        .pixels = frame, .mirror_pixels = mirrored, .image_id = 533,
        .x = 3, .y = 2, .mirror_x = 5, .width = 24, .height = 20,
        .canvas_w = 32, .canvas_h = 26, .modifier = 0
    };
    static const int at[][2] = { {120, 60}, {20, 8}, {6, 40}, {500, 300}, {40, 2} };
    fog_mode = 1;
    for (unsigned a = 0; a < sizeof(at) / sizeof(at[0]); ++a)
        for (int pattern = 0; pattern < 4; ++pattern)
            for (int hflip = 0; hflip < 2; ++hflip) {
                camx = a == 3 ? 400 : 0; camy = a == 3 ? 260 : 0;
                for (int ty = 0; ty < MAP_ROWS; ++ty) for (int tx = 0; tx < MAP_COLS; ++tx)
                    visible_tiles[ty][tx] = (unsigned char)(pattern == 0 ? 1 : pattern == 1 ? (tx + ty) & 1 :
                        pattern == 2 ? !(tx == at[a][0] / 16 && ty == at[a][1] / 16) : tx > at[a][0] / 16);
                check_fogged_raster(&r, at[a][0], at[a][1], hflip);
                if (a == 0 && pattern == 0) assert(call_count == r.height); /* one call per row */
            }
    r.mirror_pixels = 0; /* reversed per-pixel branch */
    for (int pattern = 0; pattern < 2; ++pattern) {
        camx = camy = 0;
        for (int ty = 0; ty < MAP_ROWS; ++ty) for (int tx = 0; tx < MAP_COLS; ++tx)
            visible_tiles[ty][tx] = (unsigned char)(pattern ? (tx + ty) & 1 : 1);
        check_fogged_raster(&r, 120, 60, 1);
        if (!pattern) assert(call_count == r.height); /* reversed rows, one call each */
    }

    /* A hidden tile suppresses a whole tile span, not one lookup per pixel.
     * Keep the pixel oracle active so this also checks that no hidden source
     * bytes are written while the scan advances to the next tile boundary. */
    r.mirror_pixels = 0;
    camx = camy = 0;
    for (int ty = 0; ty < MAP_ROWS; ++ty)
        for (int tx = 0; tx < MAP_COLS; ++tx) visible_tiles[ty][tx] = 0;
    visibility_query_count = 0;
    check_fogged_raster(&r, 128, 64, 0);
    assert(call_count == 0);
    assert(visibility_query_count <= r.height * 3 + 1);

    /* The same tile skips must preserve mapped source strength and dark-mask
     * LUT dispatch, including mirrored rows and mixed visible/hidden tiles. */
    static const uint8_t lookup[40 * 256] = {0};
    for (int ty = 0; ty < MAP_ROWS; ++ty)
        for (int tx = 0; tx < MAP_COLS; ++tx)
            visible_tiles[ty][tx] = (unsigned char)((tx + ty) & 1);
    r.modifier = 9; r.lookup = lookup; r.lookup_rows = 40;
    visibility_query_count = 0;
    check_fogged_raster(&r, 128, 64, 1);
    assert(call_count > 0);
    for (unsigned i = 0; i < call_count; ++i) assert(calls[i].mapped);
    r.modifier = 10; r.lookup_rows = 1;
    visibility_query_count = 0;
    check_fogged_raster(&r, 128, 64, 0);
    assert(call_count > 0);
    for (unsigned i = 0; i < call_count; ++i) assert(calls[i].mapped);

    fog_mode = 0; camx = camy = 0;
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

    /* Mirroring keeps the canvas center fixed and reverses source order:
     * each row is drawn reversed, one call per visible tile run. */
    reset_calls();
    r.pixels = ordinary;
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x) canvas[y][x] = -1;
    sc64_weapon_source_vfx_draw_raster(&r, 16, 20, 1);
    assert(call_count == 2);
    assert(calls[0].x == 14 && calls[0].w == 2 && calls[1].y == 31);
    assert(canvas[30][14] == 4 && canvas[30][15] == 3 && canvas[30][16] == -1);
    assert(canvas[31][14] == 8 && canvas[31][15] == 7 && canvas[31][16] == -1);

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
    fog_paths_match_the_oracle();
    return 0;
}
