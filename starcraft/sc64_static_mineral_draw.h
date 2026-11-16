/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STATIC_MINERAL_DRAW_H
#define SC64_STATIC_MINERAL_DRAW_H

#if defined(SC64_STATIC_MINERAL_WORDS) && !defined(SC64_TILE_RENDERER)
#ifndef SC64_UNIT_WORD_SPRITES
#error "static mineral words require the existing word sprite compositor"
#endif
#include "sc64_static_mineral_words_generated.h"

static void
draw_static_mineral_sprite(const Sc64SpriteFrame *sprite, int cx, int cy)
{
    const GbaSpriteWordFrame *words = sc64_static_mineral_word_lookup(sprite);
    if (words) {
        gfx_blit_sprite_words(cx - sprite->canvas_w / 2 + sprite->offset_x,
                              cy - sprite->canvas_h / 2 + sprite->offset_y,
                              words);
    } else {
        draw_mission_sprite(sprite, cx, cy);
    }
}
#else
#define draw_static_mineral_sprite draw_mission_sprite
#endif

#endif
