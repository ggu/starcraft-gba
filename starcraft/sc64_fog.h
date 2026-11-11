/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_FOG_H
#define SC64_FOG_H

#include "../common/gba.h"

/* Cosmetic fog state. Visibility always wins if a stale explored bit remains. */
enum {
    SC64_FOG_UNKNOWN = 0,
    SC64_FOG_EXPLORED = 1,
    SC64_FOG_VISIBLE = 2,
    SC64_FOG_SIGNATURE_COUNT = 19683 /* 3^9 */
};

#ifndef SC64_FOG_GRID_GET
#ifdef SC64_GRID_GET
#define SC64_FOG_GRID_GET(grid, cell) SC64_GRID_GET(grid, cell)
#else
#define SC64_FOG_GRID_GET(grid, cell) ((grid)[cell] != 0)
#endif
#define SC64_FOG_LOCAL_GRID_GET
#endif

/* Encode the 3x3 state neighborhood in base three, row-major, with the
 * center at digit four. Cells beyond the map are unknown. */
static inline u16
sc64_fog_signature(const u8 *visible, const u8 *explored,
                   int cols, int rows, int col, int row)
{
    unsigned signature = 0, place = 1;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx, place *= 3u) {
            int x = col + dx, y = row + dy;
            unsigned state = SC64_FOG_UNKNOWN;
            if (x >= 0 && x < cols && y >= 0 && y < rows) {
                int cell = x + y * cols;
                if (SC64_FOG_GRID_GET(visible, cell)) state = SC64_FOG_VISIBLE;
                else if (SC64_FOG_GRID_GET(explored, cell)) state = SC64_FOG_EXPLORED;
            }
            signature += state * place;
        }
    }
    return (u16)signature;
}

/* Authored edge masks. For each of the eight neighbor directions (row-major
 * neighbors with the center omitted), a 256-bit map over the cell's pixels
 * (bit j of word w is pixel 32w+j) marks the pixels that neighbor affects,
 * after radial falloff and a 4x4 ordered-dither threshold. Three maps per
 * direction: pixels turned black by an unknown neighbor, pixels shaded early
 * by an unknown neighbor, and pixels shaded by an explored neighbor. A cell
 * ORs the maps of its neighbors, so composing costs a few word operations plus
 * one pass over the pixels. The tests derive the same pixels from the
 * distance bands and Bayer matrix directly. They are const ROM data (768
 * bytes) and add no BSS or IWRAM. */
/* Pixels turned black by an unknown neighbor in each direction. */
static const u32 sc64_fog_black_dir[8][8] = {
    {0x00020007u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
    {0xffffffffu, 0x8888ffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
    {0xa000c000u, 0x00004000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
    {0x00030007u, 0x000b0007u, 0x00030007u, 0x000b0007u, 0x00030007u, 0x000b0007u, 0x00030007u, 0x000b0007u},
    {0xe000f000u, 0xe000f000u, 0xe000f000u, 0xe000f000u, 0xe000f000u, 0xe000f000u, 0xe000f000u, 0xe000f000u},
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00020000u, 0x00030007u},
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xaaaa5555u, 0xffffffffu},
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x8000c000u},
};

/* Pixels shaded early by an unknown neighbor in each direction. */
static const u32 sc64_fog_early_dir[8][8] = {
    {0x000f001fu, 0x000f001fu, 0x00000007u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
    {0xffffffffu, 0xffffffffu, 0xaaaaffffu, 0x00005555u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
    {0xf800f400u, 0xe000fc00u, 0x00007000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
    {0x003f007fu, 0x003f005fu, 0x003f007fu, 0x003f005fu, 0x003f007fu, 0x003f005fu, 0x003f007fu, 0x003f005fu},
    {0xf800fc00u, 0xf800fc00u, 0xf800fc00u, 0xf800fc00u, 0xf800fc00u, 0xf800fc00u, 0xf800fc00u, 0xf800fc00u},
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00020005u, 0x000f0017u, 0x000f001fu},
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xffffddddu, 0xffffffffu, 0xffffffffu},
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xa0004000u, 0xf800f000u, 0xf800fc00u},
};

/* Pixels shaded by an explored neighbor in each direction. */
static const u32 sc64_fog_shade_dir[8][8] = {
    {0x000b0017u, 0x00020015u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
    {0xffffffffu, 0xaaaaffffu, 0x22225555u, 0x00004444u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
    {0xa000f000u, 0xa000d400u, 0x00005000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
    {0x002b001fu, 0x000f005fu, 0x002b001fu, 0x000f005fu, 0x002b001fu, 0x000f005fu, 0x002b001fu, 0x000f005fu},
    {0xe800f400u, 0xe800f400u, 0xe800f400u, 0xe800f400u, 0xe800f400u, 0xe800f400u, 0xe800f400u, 0xe800f400u},
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000004u, 0x000a0015u, 0x000b0017u},
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xaaaa5555u, 0xbbbbffffu, 0xffffffffu},
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00004000u, 0xa0005000u, 0xe000d400u},
};

/* Exact x / 3 for x < 131072 without a library division call (the ARM7 has
 * no divide instruction and Thumb code has no long multiply). */
static inline unsigned
sc64_fog_div3(unsigned x)
{
    return (x * 43691u) >> 17;
}

#ifdef SC64_SOFT_FOG_SHADE_ONLY
/* Measurement variant: no edge masks. The signature keeps only the center
 * class (neighbors read as visible), so cells shade when explored and
 * nothing else changes. */
static inline u16
sc64_fog_shade_only(unsigned center)
{
    return (u16)(center ? 19682u - 81u * (2u - center) : 0u);
}
#endif

static inline unsigned
sc64_fog_center_state(u16 signature)
{
    unsigned q;
    if (signature >= SC64_FOG_SIGNATURE_COUNT) return SC64_FOG_UNKNOWN;
    q = sc64_fog_div3(sc64_fog_div3(sc64_fog_div3(sc64_fog_div3(signature)))); /* /81 */
    return q - 3u * sc64_fog_div3(q);
}

/* Composite one 16x16 terrain cell. This helper is deliberately called only
 * for cells whose 3x3 signature is known to the renderer; an unknown center
 * is painted black without reading terrain. Its API has no world transform:
 * cell origins are multiples of 16, so the cell-local Bayer phase is
 * world-anchored regardless of camera motion or the HUD offset. */
static inline int
sc64_fog_cell_clip(int sx, int sy, int *top, int *bottom)
{
    if (sx <= -16 || sx >= 240 || sy <= -4 || sy >= 120) return 0;
    int first = sy < 12 ? 12 : sy;
    int last = sy + 16 > 120 ? 120 : sy + 16;
#ifdef GBA_SCENE_CLIP
    {
        u16 clip = gfx_get_clip_y();
        int clip_top = clip & 255u, clip_bottom = clip >> 8;
        if (first < clip_top) first = clip_top;
        if (last > clip_bottom) last = clip_bottom;
    }
#endif
    if (first >= last) return 0;
    *top = first;
    *bottom = last;
    return 1;
}

/* Opaque one-cell blit constrained to the world playfield [12,120), further
 * restricted by the optional scene clip. NULL source paints black without
 * reading terrain, for unknown cells. */
static inline void
sc64_fog_blit_cell(int sx, int sy, const u8 *source)
{
    int top, bottom;
    if (!sc64_fog_cell_clip(sx, sy, &top, &bottom)) return;
    if (!source) {
        gfx_rect(sx, top, 16, bottom - top, C_BLACK);
    } else if (top == sy && bottom == sy + 16) {
        gfx_blit16(sx, sy, source);
    } else {
        gfx_blit16_rows(sx, sy, source, top - sy, bottom - sy);
    }
}

/* Shaded copies of recently used terrain tiles. A cell whose center is
 * explored and that has no unknown neighbor is the shade of its tile all over,
 * so it is drawn with the same whole-tile blit as a visible cell from a
 * direct-mapped cache of shaded tiles: scrolling over explored ground then
 * costs about what scrolling over visible ground does, and a tile is shaded
 * once, not every time its cell is exposed. Tiles are identified by address,
 * so the source must be immutable terrain art (or a buffer that always holds
 * the same pixels); sc64_fog_tiles_invalidate() drops every entry and must run
 * when the shade map changes. */
#ifndef SC64_FOG_TILE_SLOTS
#define SC64_FOG_TILE_SLOTS 32
#endif
_Static_assert((SC64_FOG_TILE_SLOTS & (SC64_FOG_TILE_SLOTS - 1)) == 0,
               "the shaded-tile cache size must be a power of two");
static u8 sc64_fog_tile_pixels[SC64_FOG_TILE_SLOTS][256] __attribute__((aligned(4)));
static const u8 *sc64_fog_tile_tag[SC64_FOG_TILE_SLOTS];
static const u8 *sc64_fog_tile_lut_tag;

static void __attribute__((unused))
sc64_fog_tiles_invalidate(void)
{
    for (unsigned i = 0; i < SC64_FOG_TILE_SLOTS; ++i) sc64_fog_tile_tag[i] = 0;
    sc64_fog_tile_lut_tag = 0;
}

static const u8 *
sc64_fog_shaded_tile(const u8 *terrain, const u8 *lut)
{
    unsigned slot = (unsigned)((uintptr_t)terrain >> 8) & (SC64_FOG_TILE_SLOTS - 1u);
    u8 *out = sc64_fog_tile_pixels[slot];
    if (sc64_fog_tile_tag[slot] != terrain || sc64_fog_tile_lut_tag != lut) {
        const u8 *src = terrain, *end = terrain + 256;
        if (sc64_fog_tile_lut_tag != lut) {
            for (unsigned i = 0; i < SC64_FOG_TILE_SLOTS; ++i) sc64_fog_tile_tag[i] = 0;
            sc64_fog_tile_lut_tag = lut;
        }
        if (!((unsigned)(uintptr_t)terrain & 3u)) {
            const u32 *in = (const u32 *)(const void *)terrain;
            u32 *to = (u32 *)(void *)out;
            for (unsigned q = 0; q < 64; ++q) {
                u32 t = in[q];
                to[q] = (u32)lut[t & 255u] | ((u32)lut[(t >> 8) & 255u] << 8) |
                        ((u32)lut[(t >> 16) & 255u] << 16) | ((u32)lut[t >> 24] << 24);
            }
        } else {
            while (src < end) {
                u8 a0 = src[0], a1 = src[1], a2 = src[2], a3 = src[3];
                out[0] = lut[a0]; out[1] = lut[a1]; out[2] = lut[a2]; out[3] = lut[a3];
                src += 4; out += 4;
            }
        }
        sc64_fog_tile_tag[slot] = terrain;
        out = sc64_fog_tile_pixels[slot];
    }
    return out;
}

/* Bit k of the index selects byte k of the result. */
static const u32 sc64_fog_byte_mask[16] = {
    0x00000000u, 0x000000ffu, 0x0000ff00u, 0x0000ffffu,
    0x00ff0000u, 0x00ff00ffu, 0x00ffff00u, 0x00ffffffu,
    0xff000000u, 0xff0000ffu, 0xff00ff00u, 0xff00ffffu,
    0xffff0000u, 0xffff00ffu, 0xffffff00u, 0xffffffffu,
};

/* Four pixels at a time: look up the shaded value of each byte only when some
 * pixel of the group is shaded, then select by the groups's byte masks. */
static inline u32
sc64_fog_group(u32 pixels, unsigned black, unsigned shaded, const u8 *lut)
{
    if (shaded) {
        u32 lit = (u32)lut[pixels & 255u] | ((u32)lut[(pixels >> 8) & 255u] << 8) |
                  ((u32)lut[(pixels >> 16) & 255u] << 16) | ((u32)lut[pixels >> 24] << 24);
        u32 mask = sc64_fog_byte_mask[shaded];
        pixels = (lit & mask) | (pixels & ~mask);
    }
    if (black) pixels &= ~sc64_fog_byte_mask[black];
    return pixels;
}

static void __attribute__((noinline, unused))
sc64_fog_raster_compose(int sx, int sy, unsigned center,
                        unsigned unknown_neighbors, unsigned explored_neighbors,
                        const u8 terrain[256], const u8 shade_lut[256])
{
    u32 output[64];
    u32 black[8], shaded[8];
    unsigned w, d, q;
    u32 all = center == SC64_FOG_EXPLORED ? 0xffffffffu : 0u;
    for (w = 0; w < 8; ++w) { black[w] = 0; shaded[w] = all; }
    for (d = 0; d < 8; ++d) {
        if (unknown_neighbors & (1u << d))
            for (w = 0; w < 8; ++w) {
                black[w] |= sc64_fog_black_dir[d][w];
                shaded[w] |= sc64_fog_early_dir[d][w];
            }
        if (explored_neighbors & (1u << d))
            for (w = 0; w < 8; ++w) shaded[w] |= sc64_fog_shade_dir[d][w];
    }
    if (!((unsigned)(uintptr_t)terrain & 3u)) {
        const u32 *in = (const u32 *)(const void *)terrain;
        for (q = 0; q < 64; ++q) {
            unsigned shift = (q & 7u) * 4u;
            unsigned b = (black[q >> 3] >> shift) & 15u;
            unsigned s = (shaded[q >> 3] >> shift) & 15u & ~b;
            output[q] = sc64_fog_group(in[q], b, s, shade_lut);
        }
    } else {
        for (q = 0; q < 64; ++q) {
            unsigned shift = (q & 7u) * 4u;
            unsigned b = (black[q >> 3] >> shift) & 15u;
            unsigned s = (shaded[q >> 3] >> shift) & 15u & ~b;
            const u8 *p = terrain + 4u * q;
            u32 pixels = (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
            output[q] = sc64_fog_group(pixels, b, s, shade_lut);
        }
    }
    sc64_fog_blit_cell(sx, sy, (const u8 *)(const void *)output);
}

/* Split a signature into the center class and the bit sets of unknown and
 * explored neighbors (bit k is the k-th neighbor, row-major, center omitted). */
static inline unsigned
sc64_fog_decode(u16 signature, unsigned *unknown_neighbors, unsigned *explored_neighbors)
{
    unsigned center = SC64_FOG_UNKNOWN, encoded, neighbor = 0;
    unsigned unknown = 0, explored = 0;
    if (signature >= SC64_FOG_SIGNATURE_COUNT) signature = 0;
    /* The two most common neighborhoods: all visible (every digit two) and
     * all unknown. */
    if (signature == SC64_FOG_SIGNATURE_COUNT - 1) {
        *unknown_neighbors = *explored_neighbors = 0;
        return SC64_FOG_VISIBLE;
    }
    if (signature == 9841u) { /* every digit one: explored all round */
        *unknown_neighbors = 0;
        *explored_neighbors = 0xffu;
        return SC64_FOG_EXPLORED;
    }
    if (!signature) {
        *unknown_neighbors = 0xffu;
        *explored_neighbors = 0;
        return SC64_FOG_UNKNOWN;
    }
    encoded = signature;
    for (unsigned i = 0; i < 9; ++i) {
        unsigned quotient = sc64_fog_div3(encoded);
        unsigned state = encoded - 3u * quotient;
        encoded = quotient;
        if (i != 4u) {
            if (state == SC64_FOG_UNKNOWN) unknown |= 1u << neighbor;
            else if (state == SC64_FOG_EXPLORED) explored |= 1u << neighbor;
            ++neighbor;
        } else {
            center = state;
        }
    }
    *unknown_neighbors = unknown;
    *explored_neighbors = explored;
    return center;
}

static void __attribute__((noinline, unused))
sc64_fog_raster_cell(int sx, int sy, u16 signature,
                     const u8 terrain[256], const u8 shade_lut[256])
{
    unsigned center, unknown_neighbors, explored_neighbors;
    int top, bottom;

    /* Avoid inspecting even the source pointer for cells wholly outside the
     * world band, or outside a narrower scene clip. */
    if (!sc64_fog_cell_clip(sx, sy, &top, &bottom)) return;
    (void)top;
    (void)bottom;

    center = sc64_fog_decode(signature, &unknown_neighbors, &explored_neighbors);

    /* This path uses only a few scalars and does not reserve the compositing
     * scratch buffer. All-visible cells bypass the edge masks entirely. */
    if (center == SC64_FOG_VISIBLE && !unknown_neighbors && !explored_neighbors && terrain) {
        sc64_fog_blit_cell(sx, sy, terrain);
        return;
    }
    if (center == SC64_FOG_UNKNOWN || !terrain || !shade_lut) {
        sc64_fog_blit_cell(sx, sy, 0);
        return;
    }
    if (center == SC64_FOG_EXPLORED && !unknown_neighbors) {
        sc64_fog_blit_cell(sx, sy, sc64_fog_shaded_tile(terrain, shade_lut));
        return;
    }
    sc64_fog_raster_compose(sx, sy, center, unknown_neighbors,
                            explored_neighbors, terrain, shade_lut);
}

/* Opaque w x h block at (x, y), clipped to the world band and scene clip like
 * a whole cell. */
static inline void
sc64_fog_blit_rect(int x, int y, int w, int h, const u8 *packed)
{
    int top = y, bottom = y + h;
    if (x >= 240 || x + w <= 0 || top >= 120 || bottom <= 12) return;
    if (top < 12) top = 12;
    if (bottom > 120) bottom = 120;
#ifdef GBA_SCENE_CLIP
    {
        u16 clip = gfx_get_clip_y();
        int clip_top = clip & 255u, clip_bottom = clip >> 8;
        if (top < clip_top) top = clip_top;
        if (bottom > clip_bottom) bottom = clip_bottom;
    }
#endif
    if (top >= bottom) return;
    if (!packed) gfx_rect(x, top, w, bottom - top, C_BLACK);
    else gfx_blit_opaque(x, top, w, bottom - top, packed + (top - y) * w);
}

/* Compose only the pixels [rx0,rx1) x [ry0,ry1) of a cell whose signature is
 * already decoded. Each pixel is the same function of the signature as in
 * sc64_fog_raster_cell, so a rectangle drawn over restored pixels equals a
 * whole-cell draw there. Used for cells whose fog state is unchanged but
 * whose edge strip was newly exposed by a scroll: composing the whole cell
 * for a few exposed pixels is most of the cost of a scrolling frame. */
static void __attribute__((noinline, unused))
sc64_fog_compose_rect(int sx, int sy, unsigned center,
                      unsigned unknown_neighbors, unsigned explored_neighbors,
                      const u8 terrain[256], const u8 shade_lut[256],
                      int rx0, int ry0, int rx1, int ry1)
{
    int w = rx1 - rx0, h = ry1 - ry0, rx, ry;
    u8 packed[256];
    u32 black[8], shaded[8];
    unsigned d, k, first = (unsigned)ry0 >> 1, last = (unsigned)(ry1 - 1) >> 1;
    u32 all = center == SC64_FOG_EXPLORED ? 0xffffffffu : 0u;
    if (w <= 0 || h <= 0) return;
    for (k = first; k <= last; ++k) { black[k] = 0; shaded[k] = all; }
    for (d = 0; d < 8; ++d) {
        if (unknown_neighbors & (1u << d))
            for (k = first; k <= last; ++k) {
                black[k] |= sc64_fog_black_dir[d][k];
                shaded[k] |= sc64_fog_early_dir[d][k];
            }
        if (explored_neighbors & (1u << d))
            for (k = first; k <= last; ++k) shaded[k] |= sc64_fog_shade_dir[d][k];
    }
    {
        u8 *out = packed;
        unsigned full = (1u << w) - 1u;
        for (ry = ry0; ry < ry1; ++ry) {
            /* One row is 16 bits of each map: half a word. */
            unsigned shift = ((unsigned)ry & 1u) * 16u;
            unsigned b = (black[(unsigned)ry >> 1] >> shift) & 0xffffu;
            unsigned sh = ((shaded[(unsigned)ry >> 1] >> shift) & 0xffffu) & ~b;
            const u8 *in = terrain + ry * 16;
            b >>= rx0;
            sh >>= rx0;
            if (!(b | sh)) {
                const u8 *src = in + rx0, *end = in + rx1;
                while (src < end) *out++ = *src++;
            } else if (!b && (sh & full) == full) {
                const u8 *src = in + rx0, *end = in + rx1;
                while (end - src >= 4) {
                    u8 a0 = src[0], a1 = src[1], a2 = src[2], a3 = src[3];
                    out[0] = shade_lut[a0]; out[1] = shade_lut[a1];
                    out[2] = shade_lut[a2]; out[3] = shade_lut[a3];
                    src += 4; out += 4;
                }
                while (src < end) *out++ = shade_lut[*src++];
            } else {
                for (rx = rx0; rx < rx1; ++rx, b >>= 1, sh >>= 1) {
                    unsigned t = in[rx];
                    if (b & 1u) t = C_BLACK;
                    else if (sh & 1u) t = shade_lut[t];
                    *out++ = (u8)t;
                }
            }
        }
    }
    sc64_fog_blit_rect(sx + rx0, sy + ry0, w, h, packed);
}

/* Redraw the part of a cell inside the vertical strip x in [vx0,vx1) and the
 * horizontal strip y in [hy0,hy1) (screen coordinates; an empty strip has
 * vx1 <= vx0 or hy1 <= hy0). Cells that need no composing are simply blitted
 * whole: that is cheaper than selecting pixels. */
static void __attribute__((noinline, unused))
sc64_fog_raster_strips(int sx, int sy, u16 signature,
                       const u8 terrain[256], const u8 shade_lut[256],
                       int vx0, int vx1, int hy0, int hy1)
{
    unsigned center, unknown_neighbors, explored_neighbors;
    int top, bottom, a, b;
    if (!sc64_fog_cell_clip(sx, sy, &top, &bottom)) return;
    center = sc64_fog_decode(signature, &unknown_neighbors, &explored_neighbors);
    if (center == SC64_FOG_UNKNOWN || !terrain || !shade_lut) {
        sc64_fog_blit_cell(sx, sy, 0);
        return;
    }
    if (center == SC64_FOG_VISIBLE && !unknown_neighbors && !explored_neighbors) {
        sc64_fog_blit_cell(sx, sy, terrain);
        return;
    }
    if (center == SC64_FOG_EXPLORED && !unknown_neighbors) {
        sc64_fog_blit_cell(sx, sy, sc64_fog_shaded_tile(terrain, shade_lut));
        return;
    }
    /* Only the part of the cell inside the world band, scene clip and
     * screen is worth composing. */
    {
        int ry0 = top - sy, ry1 = bottom - sy;
        int cx0 = sx < 0 ? -sx : 0, cx1 = sx + 16 > 240 ? 240 - sx : 16;
        if (vx1 > vx0) {
            a = vx0 > sx ? vx0 : sx;
            b = vx1 < sx + 16 ? vx1 : sx + 16;
            if (b > a) sc64_fog_compose_rect(sx, sy, center, unknown_neighbors,
                                             explored_neighbors, terrain, shade_lut,
                                             a - sx, ry0, b - sx, ry1);
        }
        if (hy1 > hy0) {
            a = hy0 > top ? hy0 : top;
            b = hy1 < bottom ? hy1 : bottom;
            if (b > a && cx1 > cx0)
                sc64_fog_compose_rect(sx, sy, center, unknown_neighbors,
                                      explored_neighbors, terrain, shade_lut,
                                      cx0, a - sy, cx1, b - sy);
        }
    }
}

#ifdef SC64_FOG_LOCAL_GRID_GET
#undef SC64_FOG_GRID_GET
#undef SC64_FOG_LOCAL_GRID_GET
#endif

#endif
