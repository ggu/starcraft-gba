/* Building-shaped placement feedback from an existing sprite's alpha mask.
 * The outline is an authored approximation; exact N64 raster is untraced.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BUILDING_PLACEMENT_VFX_H
#define SC64_BUILDING_PLACEMENT_VFX_H
#include <stdint.h>
#include "../common/gba.h"

/* One bit per pixel of a sprite row: set where the pixel is opaque. The row
 * is read a word at a time once the pointer is aligned; every read stays
 * inside the row. Bits at and above w stay clear. */
static inline void
sc64_building_placement_row_mask(const u8 *p, int w, u32 m[4])
{
    int x = 0;
    m[0] = m[1] = m[2] = m[3] = 0;
    for (; x < w && ((uintptr_t)(p + x) & 3u); ++x)
        if (p[x]) m[x >> 5] |= 1u << (x & 31);
    for (; x + 4 <= w; x += 4) {
        u32 v;
        __builtin_memcpy(&v, __builtin_assume_aligned(p + x, 4), 4);
        if (v) {
            /* Bit 7 of each byte of t is set where that byte is non-zero;
             * the multiply gathers those four bits into one nibble. */
            u32 t = (((v & 0x7f7f7f7fu) + 0x7f7f7f7fu) | v) & 0x80808080u;
            u32 nibble = (((t >> 7) * 0x01020408u) >> 24) & 15u;
            int shift = x & 31;
            m[x >> 5] |= nibble << shift;
            if (shift > 28) m[(x >> 5) + 1] |= nibble >> (32 - shift);
        }
    }
    for (; x < w; ++x)
        if (p[x]) m[x >> 5] |= 1u << (x & 31);
}

/* A zero pixel is transparent. The four-neighbor boundary includes edges of
 * internal transparent holes. Coordinates follow draw_mission_sprite's canvas
 * anchor. Explicit battlefield clipping also protects HUDs in builds without
 * GBA_SCENE_CLIP. No allocation or retained state is needed. Return zero for
 * invalid metadata, one for a valid frame (including an offscreen frame).
 *
 * Each row becomes an opacity bitmask, so a pixel is read once. An opaque
 * pixel is on the boundary unless its left, right, upper and lower neighbors
 * are all opaque; neighbors outside the sprite count as transparent. */
static inline int
sc64_building_placement_outline(int cx, int cy, int canvas_w, int canvas_h,
                               int offset_x, int offset_y, int w, int h,
                               const u8 *pixels, u8 color)
{
    int left, right, top, bottom;
    int64_t x0, y0;
    u32 rows[3][4];
    u32 *above = rows[0], *row = rows[1], *below = rows[2];
    if (!pixels || w <= 0 || h <= 0 || w > 128 || h > 128 ||
        canvas_w <= 0 || canvas_h <= 0 || canvas_w > 128 || canvas_h > 128)
        return 0;
    x0 = (int64_t)cx - canvas_w / 2 + offset_x;
    y0 = (int64_t)cy - canvas_h / 2 + offset_y;
    if (x0 >= 240 || x0 + w <= 0 || y0 >= 120 || y0 + h <= 12)
        return 1;
    left = x0 < 0 ? (int)-x0 : 0;
    right = x0 + w > 240 ? (int)(240 - x0) : w;
    top = y0 < 12 ? (int)(12 - y0) : 0;
    bottom = y0 + h > 120 ? (int)(120 - y0) : h;
    if (top > 0) sc64_building_placement_row_mask(pixels + (top - 1) * w, w, above);
    else above[0] = above[1] = above[2] = above[3] = 0;
    sc64_building_placement_row_mask(pixels + top * w, w, row);
    for (int y = top; y < bottom; ++y) {
        u32 *spare;
        if (y + 1 < h) sc64_building_placement_row_mask(pixels + (y + 1) * w, w, below);
        else below[0] = below[1] = below[2] = below[3] = 0;
        for (int k = left >> 5; k <= (right - 1) >> 5; ++k) {
            u32 m = row[k];
            u32 l = (m << 1) | (k ? row[k - 1] >> 31 : 0);
            u32 r = (m >> 1) | (k < 3 ? row[k + 1] << 31 : 0);
            u32 edge = m & ~(l & r & above[k] & below[k]);
            int x = k << 5;
            if (x < left) { edge &= ~0u << (left - x); }
            if (right - x < 32) edge &= (1u << (right - x)) - 1u;
            while (edge) {
                if (!(edge & 0xffffu)) { edge >>= 16; x += 16; }
                if (!(edge & 0xffu)) { edge >>= 8; x += 8; }
                if (edge & 1u) gfx_pixel((int)(x0 + x), (int)(y0 + y), color);
                edge >>= 1;
                ++x;
            }
        }
        spare = above; above = row; row = below; below = spare;
    }
    return 1;
}
#endif
