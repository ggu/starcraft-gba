/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "gba.h"
#include "../starcraft/game.c"

extern volatile u32 gfx_sprite_words_calls;

#include <assert.h>

static void capture_frame(u8 pixels[240 * 160])
{
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            pixels[y * 240 + x] = gfx_test_pixel(x, y);
}

static void assert_frame(const u8 expected[240 * 160])
{
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            assert(gfx_test_pixel(x, y) == expected[y * 240 + x]);
}

static void check_unit_render(Unit *unit, int explicit_raw)
{
    u8 expected[240 * 160];
    int x = unit->x - camx;
    int y = unit->y - camy + 12;
    const Sc64SpriteFrame *sprite = explicit_raw ?
        sc64_race_unit_sprite((unsigned short)unit->raw_type) :
        mission_unit_sprite(unit->type);
    assert(sprite != 0);

    gfx_begin(C_BLACK);
    gfx_set_clip_y(12, 120);
    u32 calls_before = gfx_sprite_words_calls;
    u32 raw_calls_before = explicit_raw ? sc64_unit_words_explicit_raw_calls :
                                           sc64_unit_words_derived_raw_calls;
    draw_unit(unit);
    assert(gfx_sprite_words_calls == calls_before + 1);
    assert((explicit_raw ? sc64_unit_words_explicit_raw_calls :
                           sc64_unit_words_derived_raw_calls) == raw_calls_before + 1);
    capture_frame(expected);

    gfx_begin(C_BLACK);
    gfx_set_clip_y(12, 120);
    draw_mission_sprite(sprite, x, y);
    int sprite_y = y - sprite->canvas_h / 2 + sprite->offset_y;
    if (unit->selected) {
        int sprite_x = x - sprite->canvas_w / 2 + sprite->offset_x;
        int sprite_w = sprite->w, sprite_h = sprite->h;
        gfx_frame(sprite_x - 2, sprite_y - 2, sprite_w + 4, sprite_h + 4,
                  C_HIGHLIGHT);
        draw_actor_vitals(x, sprite_y + sprite_h + 3, sprite_w + 4, unit->hp,
                          unit->maxhp, unit->shield,
                          sc64_actor_max_shields(unit->type, 0), 0, 0);
    }
    assert_frame(expected);
}

int main(void)
{
    gba_init();
    game_init();
    camx = camy = 0;
    screen = SCREEN_PLAY;

    int derived_slot = spawn_unit(SIDE, U_MUTALISK, 64, 64);
    assert(derived_slot >= 0);
    Unit *derived = &u[derived_slot];
    assert(derived->raw_type == -1);
    assert(sc64_race_raw_unit(derived->type) == 43);
    check_unit_render(derived, 0);

    int explicit_slot = spawn_unit(SIDE, U_MUTALISK, 112, 64);
    assert(explicit_slot >= 0);
    Unit *explicit = &u[explicit_slot];
    explicit->raw_type = 43;
    check_unit_render(explicit, 1);
    return 0;
}
