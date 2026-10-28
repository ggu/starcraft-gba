/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_weapon_source_vfx_bounds.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>

typedef struct {
    int x, y, mirror_x;
    unsigned width, height, canvas_w, canvas_h;
} SyntheticFrame;
typedef struct { int x, y; } Point;

static int visible(int x, int y, int fog_pattern)
{
    if (x < 0 || x >= 240 || y < 12 || y >= 120) return 0;
    if (!fog_pattern) return 1;
    int tx = x / 16, ty = (y - 12) / 16;
    return ((tx + ty) & 1) == (fog_pattern - 1);
}

/* Independent pixel oracle: apply every crop, mirror origin, child offset,
 * and sampled path center, then ask whether any actual pixel reaches clip. */
static int pixel_oracle(const SyntheticFrame *frames, size_t frame_count,
                        const Point *offsets, size_t offset_count,
                        const Point *centers, size_t center_count,
                        int camx, int camy, int fog_pattern)
{
    for (size_t p = 0; p < center_count; ++p)
        for (size_t o = 0; o < offset_count; ++o)
            for (size_t f = 0; f < frame_count; ++f)
                for (int flip = 0; flip < 2; ++flip) {
                    const SyntheticFrame *r = &frames[f];
                    int left = centers[p].x + offsets[o].x - camx -
                               (int)r->canvas_w / 2 +
                               (flip ? r->mirror_x : r->x);
                    int top = centers[p].y + offsets[o].y - camy + 12 -
                              (int)r->canvas_h / 2 + r->y;
                    for (unsigned row = 0; row < r->height; ++row)
                        for (unsigned col = 0; col < r->width; ++col)
                            if (visible(left + (int)col, top + (int)row,
                                        fog_pattern)) return 1;
                }
    return 0;
}

static void envelope_matches_pixel_oracle(void)
{
    static const SyntheticFrame frames[] = {
        { 3, 2, 11, 9, 7, 18, 14 },
        { -4, 5, 8, 6, 9, 17, 21 },
        { 12, -3, -2, 5, 8, 32, 14 }
    };
    /* These are already converted to world pixels by C truncation. The
     * production accumulator accepts source-space values, so double them. */
    static const Point pixel_offsets[] = {{-7, 4}, {6, -9}, {0, 0}};
    static const Point source_offsets[] = {{-14, 8}, {12, -18}, {0, 0}};
    static const Point centers[] = {
        {-90, 44}, {-10, 36}, {34, 58}, {121, 94}, {270, 135},
        {62, 61}, {176, 88} /* interior path samples */
    };
    Sc64WeaponSourceVfxBoundsWork work = {0};
    for (size_t i = 0; i < sizeof(frames) / sizeof(frames[0]); ++i)
        sc64_weapon_source_vfx_bounds_include_frame(&work,
            frames[i].x, frames[i].y, frames[i].mirror_x,
            frames[i].width, frames[i].height,
            frames[i].canvas_w, frames[i].canvas_h);
    for (size_t i = 0; i < sizeof(source_offsets) / sizeof(source_offsets[0]); ++i)
        sc64_weapon_source_vfx_bounds_include_child_offset(&work,
            source_offsets[i].x, source_offsets[i].y);
    uint32_t packed = sc64_weapon_source_vfx_bounds_pack(&work);
    uint16_t radius_x, radius_y;
    assert(sc64_weapon_source_vfx_bounds_unpack(packed, &radius_x, &radius_y));
    for (int camy = -24; camy <= 96; camy += 24)
        for (int camx = -48; camx <= 96; camx += 24)
            for (int fog = 0; fog < 3; ++fog) {
                int64_t lo_x = centers[0].x, hi_x = lo_x;
                int64_t lo_y = centers[0].y, hi_y = lo_y;
                for (size_t i = 1; i < sizeof(centers) / sizeof(centers[0]); ++i) {
                    if (centers[i].x < lo_x) lo_x = centers[i].x;
                    if (centers[i].x > hi_x) hi_x = centers[i].x;
                    if (centers[i].y < lo_y) lo_y = centers[i].y;
                    if (centers[i].y > hi_y) hi_y = centers[i].y;
                }
                int miss = sc64_weapon_source_vfx_bounds_miss_clip(
                    lo_x, hi_x, lo_y, hi_y, camx, camy, radius_x, radius_y);
                int oracle = pixel_oracle(frames,
                    sizeof(frames) / sizeof(frames[0]), pixel_offsets,
                    sizeof(pixel_offsets) / sizeof(pixel_offsets[0]), centers,
                    sizeof(centers) / sizeof(centers[0]), camx, camy, fog);
                if (miss) assert(!oracle);
            }
}

static void cache_states_and_overflow_are_distinct(void)
{
    assert(SC64_WEAPON_SOURCE_VFX_BOUNDS_UNKNOWN !=
           SC64_WEAPON_SOURCE_VFX_BOUNDS_INVALID);
    assert(SC64_WEAPON_SOURCE_VFX_BOUNDS_UNKNOWN !=
           SC64_WEAPON_SOURCE_VFX_BOUNDS_VALID);
    assert(SC64_WEAPON_SOURCE_VFX_BOUNDS_INVALID !=
           SC64_WEAPON_SOURCE_VFX_BOUNDS_VALID);
    uint16_t x, y;
    assert(!sc64_weapon_source_vfx_bounds_unpack(
        SC64_WEAPON_SOURCE_VFX_BOUNDS_UNKNOWN, &x, &y));
    assert(!sc64_weapon_source_vfx_bounds_unpack(
        SC64_WEAPON_SOURCE_VFX_BOUNDS_INVALID, &x, &y));

    Sc64WeaponSourceVfxBoundsWork edge = {0};
    sc64_weapon_source_vfx_bounds_include_frame(&edge,
        INT16_MIN, 0, INT16_MAX, UINT16_MAX, 1, UINT16_MAX, 1);
    assert(edge.invalid);
    assert(sc64_weapon_source_vfx_bounds_pack(&edge) ==
           SC64_WEAPON_SOURCE_VFX_BOUNDS_INVALID);

    Sc64WeaponSourceVfxBoundsWork valid = {0};
    sc64_weapon_source_vfx_bounds_include_frame(&valid, 0, 0, 0,
        1, 1, 1, 1);
    sc64_weapon_source_vfx_bounds_include_child_offset(&valid, INT16_MIN, 0);
    uint32_t packed = sc64_weapon_source_vfx_bounds_pack(&valid);
    assert(sc64_weapon_source_vfx_bounds_unpack(packed, &x, &y));
    assert(x == 16384 && y == 0);

    /* Bounds compare extreme game coordinates in 64-bit space. */
    assert(sc64_weapon_source_vfx_bounds_miss_clip(
        INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MAX, INT_MAX, 32767, 32767));
    assert(!sc64_weapon_source_vfx_bounds_miss_clip(
        INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, 32767, 32767));
}

static void clip_edges_are_inclusive_at_last_visible_pixels(void)
{
    const uint16_t rx = 19, ry = 13;
    assert(!sc64_weapon_source_vfx_bounds_miss_clip(-40, -rx, 20, 20,
                                                    0, 0, rx, ry));
    assert(sc64_weapon_source_vfx_bounds_miss_clip(-41, -rx - 1, 20, 20,
                                                   0, 0, rx, ry));
    assert(!sc64_weapon_source_vfx_bounds_miss_clip(240 + rx - 1,
                                                    270, 20, 20,
                                                    0, 0, rx, ry));
    assert(sc64_weapon_source_vfx_bounds_miss_clip(240 + rx, 270,
                                                   20, 20, 0, 0, rx, ry));
    assert(!sc64_weapon_source_vfx_bounds_miss_clip(20, 20,
                                                    -80, -ry,
                                                    0, 0, rx, ry));
    assert(sc64_weapon_source_vfx_bounds_miss_clip(20, 20,
                                                   -81, -ry - 1,
                                                   0, 0, rx, ry));
    assert(!sc64_weapon_source_vfx_bounds_miss_clip(20, 20,
                                                    107 + ry, 150,
                                                    0, 0, rx, ry));
    assert(sc64_weapon_source_vfx_bounds_miss_clip(20, 20,
                                                   108 + ry, 150,
                                                   0, 0, rx, ry));
}

int main(void)
{
    envelope_matches_pixel_oracle();
    cache_states_and_overflow_are_distinct();
    clip_edges_are_inclusive_at_last_visible_pixels();
    return 0;
}
