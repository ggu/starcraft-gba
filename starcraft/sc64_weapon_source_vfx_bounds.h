/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_WEAPON_SOURCE_VFX_BOUNDS_H
#define SC64_WEAPON_SOURCE_VFX_BOUNDS_H

#include <stdint.h>

/* A frame's crop and its optional mirror crop, expressed as the farthest
 * drawable pixel from the image center, plus source-space child offsets. */
typedef struct {
    uint32_t frame_x, frame_y;
    uint32_t child_x, child_y;
    uint8_t invalid;
} Sc64WeaponSourceVfxBoundsWork;

/* Cache encoding: zero is unknown, 01 is invalid/unbounded, 10 is valid.
 * Valid radii occupy two 15-bit fields, so the entire cache is 4 BSS bytes. */
#define SC64_WEAPON_SOURCE_VFX_BOUNDS_UNKNOWN UINT32_C(0)
#define SC64_WEAPON_SOURCE_VFX_BOUNDS_INVALID UINT32_C(0x40000000)
#define SC64_WEAPON_SOURCE_VFX_BOUNDS_VALID   UINT32_C(0x80000000)
#define SC64_WEAPON_SOURCE_VFX_BOUNDS_RADIUS_MAX UINT32_C(0x7fff)

static inline void
sc64_weapon_source_vfx_bounds_include_frame(
    Sc64WeaponSourceVfxBoundsWork *work,
    int32_t x, int32_t y, int32_t mirror_x,
    uint32_t width, uint32_t height,
    uint32_t canvas_w, uint32_t canvas_h)
{
    if (!work || work->invalid || !width || !height) return;
    int64_t half_w = (int64_t)(canvas_w / 2u);
    int64_t half_h = (int64_t)(canvas_h / 2u);
    int64_t left = -half_w + x;
    int64_t mirror_left = -half_w + mirror_x;
    int64_t right = left + width - 1u;
    int64_t mirror_right = mirror_left + width - 1u;
    int64_t top = -half_h + y;
    int64_t bottom = top + height - 1u;
    int64_t bounds[] = {left, mirror_left, right, mirror_right, top, bottom};
    for (unsigned i = 0; i < sizeof(bounds) / sizeof(bounds[0]); ++i) {
        int64_t magnitude = bounds[i] < 0 ? -bounds[i] : bounds[i];
        if (magnitude > SC64_WEAPON_SOURCE_VFX_BOUNDS_RADIUS_MAX) {
            work->invalid = 1;
            return;
        }
        uint32_t radius = (uint32_t)magnitude;
        if (i < 4u) {
            if (radius > work->frame_x) work->frame_x = radius;
        } else if (radius > work->frame_y) work->frame_y = radius;
    }
}

/* IScript LO and IMGOL coordinates are source pixels at 2x GBA scale. C's
 * signed division truncates toward zero, matching the renderer exactly. */
static inline void
sc64_weapon_source_vfx_bounds_include_child_offset(
    Sc64WeaponSourceVfxBoundsWork *work, int32_t source_x, int32_t source_y)
{
    if (!work || work->invalid) return;
    int64_t x = source_x / 2;
    int64_t y = source_y / 2;
    if (x < 0) x = -x;
    if (y < 0) y = -y;
    if (x > SC64_WEAPON_SOURCE_VFX_BOUNDS_RADIUS_MAX ||
        y > SC64_WEAPON_SOURCE_VFX_BOUNDS_RADIUS_MAX) {
        work->invalid = 1;
        return;
    }
    if ((uint32_t)x > work->child_x) work->child_x = (uint32_t)x;
    if ((uint32_t)y > work->child_y) work->child_y = (uint32_t)y;
}

static inline uint32_t
sc64_weapon_source_vfx_bounds_pack(const Sc64WeaponSourceVfxBoundsWork *work)
{
    if (!work || work->invalid ||
        work->frame_x + work->child_x > SC64_WEAPON_SOURCE_VFX_BOUNDS_RADIUS_MAX ||
        work->frame_y + work->child_y > SC64_WEAPON_SOURCE_VFX_BOUNDS_RADIUS_MAX)
        return SC64_WEAPON_SOURCE_VFX_BOUNDS_INVALID;
    return SC64_WEAPON_SOURCE_VFX_BOUNDS_VALID |
        (work->frame_y + work->child_y) << 15 |
        (work->frame_x + work->child_x);
}

static inline int
sc64_weapon_source_vfx_bounds_unpack(uint32_t packed,
                                     uint16_t *radius_x, uint16_t *radius_y)
{
    if ((packed & UINT32_C(0xc0000000)) !=
        SC64_WEAPON_SOURCE_VFX_BOUNDS_VALID || !radius_x || !radius_y)
        return 0;
    *radius_x = (uint16_t)(packed & SC64_WEAPON_SOURCE_VFX_BOUNDS_RADIUS_MAX);
    *radius_y = (uint16_t)((packed >> 15) & SC64_WEAPON_SOURCE_VFX_BOUNDS_RADIUS_MAX);
    return 1;
}

/* Inclusive pixel bounds against the exact VFX scene clip. 64-bit math keeps
 * extreme coordinates conservative instead of overflowing into a false cull. */
static inline int
sc64_weapon_source_vfx_bounds_miss_clip(int64_t lo_x, int64_t hi_x,
                                        int64_t lo_y, int64_t hi_y,
                                        int64_t camx, int64_t camy,
                                        uint16_t radius_x, uint16_t radius_y)
{
    lo_x -= camx;
    hi_x -= camx;
    lo_y = lo_y - camy + 12;
    hi_y = hi_y - camy + 12;
    return hi_x + radius_x < 0 || lo_x - radius_x >= 240 ||
           hi_y + radius_y < 12 || lo_y - radius_y >= 120;
}

#endif
