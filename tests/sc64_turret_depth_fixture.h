/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Test-only overlay for the facing bank. The wide rectangle exposes turret-only
 * crops and makes the overlapping draw orders easy to distinguish. */
#ifndef SC64_TURRET_DEPTH_FIXTURE_H
#define SC64_TURRET_DEPTH_FIXTURE_H

#define sc64_unit_facing_frames sc64_fixture_base_facing_frames
#include_next "sc64_unit_facing_generated.h"
#undef sc64_unit_facing_frames

#define SC64_DEPTH_SYNTHETIC_TURRET 1

static const Sc64SpriteFrame *sc64_unit_facing_frames(unsigned short raw)
{
    if (raw != 4) return sc64_fixture_base_facing_frames(raw);

    enum { WIDTH = 31, HEIGHT = 13, DIRECTIONS = 17 };
    static unsigned char pixels[WIDTH * HEIGHT];
    static Sc64SpriteFrame frames[DIRECTIONS];
    if (!frames[0].pixels) {
        memset(pixels, C_CYAN, sizeof(pixels));
        for (unsigned i = 0; i < DIRECTIONS; ++i)
            frames[i] = (Sc64SpriteFrame){0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT, pixels};
    }
    return frames;
}
#endif
