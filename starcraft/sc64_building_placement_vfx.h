/* Building-shaped placement feedback from an existing sprite's alpha mask.
 * The outline is an authored approximation; exact N64 raster is untraced.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BUILDING_PLACEMENT_VFX_H
#define SC64_BUILDING_PLACEMENT_VFX_H
#include <stdint.h>
#include "../common/gba.h"

/* A zero pixel is transparent. The four-neighbor boundary includes edges of
 * internal transparent holes. Coordinates follow draw_mission_sprite's canvas
 * anchor. Explicit battlefield clipping also protects HUDs in builds without
 * GBA_SCENE_CLIP. No allocation or retained state is needed. Return zero for
 * invalid metadata, one for a valid frame (including an offscreen frame). */
static inline int
sc64_building_placement_outline(int cx, int cy, int canvas_w, int canvas_h,
                               int offset_x, int offset_y, int w, int h,
                               const u8 *pixels, u8 color)
{
    int left, right, top, bottom;
    int64_t x0, y0;
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
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            int p = y * w + x;
            if (!pixels[p]) continue;
            if (x == 0 || x == w - 1 || y == 0 || y == h - 1 ||
                !pixels[p - 1] || !pixels[p + 1] ||
                !pixels[p - w] || !pixels[p + w])
                gfx_pixel((int)(x0 + x), (int)(y0 + y), color);
        }
    }
    return 1;
}
#endif
