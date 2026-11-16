/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional prepared-frame comparison through the actual resource wrapper. */
#include "../common/gba.h"
#include "sc64_sprites_generated.h"
#include <assert.h>
#include <stdio.h>

static void draw_mission_sprite(const Sc64SpriteFrame *sprite, int cx, int cy)
{
    gfx_blit_sprite(cx - sprite->canvas_w / 2 + sprite->offset_x,
                    cy - sprite->canvas_h / 2 + sprite->offset_y,
                    sprite->w, sprite->h, sprite->pixels);
}
#include "../starcraft/sc64_static_mineral_draw.h"

static u8 baseline[240 * 160];

static void init_page(void)
{
    gfx_begin(61);
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            gfx_pixel(x, y, (u8)(1 + (x * 13 + y * 29) % 255));
    gfx_set_clip_y(12, 120);
}

int main(void)
{
    static const Sc64SpriteFrame *const frames[] = {
        &sc64_sprite_mineral_field_1, &sc64_sprite_mineral_field_2,
        &sc64_sprite_mineral_field_3, &sc64_sprite_marine
    };
    static const int x[] = {-17, -2, 117, 222, 245};
    static const int y[] = {-5, 7, 45, 112, 136};
    unsigned cases = 0;
    for (unsigned i = 0; i < sizeof(frames) / sizeof(frames[0]); ++i) {
        assert((sc64_static_mineral_word_lookup(frames[i]) != NULL) == (i < 3));
        for (int phase = 0; phase < 4; ++phase)
        for (unsigned xi = 0; xi < sizeof(x) / sizeof(x[0]); ++xi)
        for (unsigned yi = 0; yi < sizeof(y) / sizeof(y[0]); ++yi) {
            init_page();
            draw_mission_sprite(frames[i], x[xi] + phase, y[yi]);
            for (int py = 0; py < 160; ++py)
                for (int px = 0; px < 240; ++px)
                    baseline[py * 240 + px] = gfx_test_pixel(px, py);
            init_page();
            draw_static_mineral_sprite(frames[i], x[xi] + phase, y[yi]);
            for (int py = 0; py < 160; ++py)
                for (int px = 0; px < 240; ++px)
                    assert(baseline[py * 240 + px] == gfx_test_pixel(px, py));
            ++cases;
        }
    }
    printf("%u prepared mineral wrapper full-page comparisons\n", cases);
    return 0;
}
