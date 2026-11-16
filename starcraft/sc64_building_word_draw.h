/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BUILDING_WORD_DRAW_H
#define SC64_BUILDING_WORD_DRAW_H

#ifdef SC64_HAS_SPRITES
#if defined(SC64_BUILDING_WORDS_PROBE) && !defined(SC64_TILE_RENDERER)
#ifndef SC64_UNIT_WORD_SPRITES
#error "SC64_BUILDING_WORDS_PROBE requires the packed sprite compositor"
#endif
#include "sc64_building_words_generated.h"
static void
draw_completed_building_sprite(const Sc64SpriteFrame *sprite, int x, int y)
{
    const GbaSpriteWordFrame *words = sc64_building_word_lookup(sprite);
    if (words) {
        gfx_blit_sprite_words(x - sprite->canvas_w / 2 + sprite->offset_x,
                              y - sprite->canvas_h / 2 + sprite->offset_y,
                              words);
    } else draw_mission_sprite(sprite, x, y);
}
#else
#define draw_completed_building_sprite draw_mission_sprite
#endif
#endif

#endif
