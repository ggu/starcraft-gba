/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NUKE_GHOST_ART_H
#define SC64_NUKE_GHOST_ART_H
#include "../common/gba.h"
#include "sc64_nuke_ghost_source_generated.h"

/* Ghost SpecialState1 visits bases 0, 17 and 34 at source ticks 0, 1 and 2.
 * The last pose remains until the order exits tracking. Only the clockwise
 * half-turn is stored in the source GRP; the renderer flips the other side. */
static inline const Sc64SpriteFrame *
sc64_nuke_ghost_paint_sprite(unsigned heading, unsigned source_tick)
{
    if (heading >= 32) return 0;
    unsigned pose = source_tick < 2 ? source_tick : 2;
    return &sc64_nuke_ghost_paint_frames[pose][heading <= 16 ? heading : 32 - heading];
}

static inline int
sc64_nuke_ghost_paint_draw(unsigned heading, unsigned source_tick, int x, int y)
{
    const Sc64SpriteFrame *s = sc64_nuke_ghost_paint_sprite(heading, source_tick);
    if (!s) return 0;
    int top = y - s->canvas_h / 2 + s->offset_y;
    if (heading <= 16)
        gfx_blit_sprite(x - s->canvas_w / 2 + s->offset_x, top, s->w, s->h, s->pixels);
    else {
        int left = x + s->canvas_w / 2 - s->offset_x - s->w;
#ifdef SC64_UNIT_FACING
        gfx_blit_sprite_hflip(left, top, s->w, s->h, s->pixels);
#else
        /* Small probe builds without the normal facing blitter keep the
         * same projected origin and clip using ordinary one-row blits. */
        u8 row[32]; /* The importer pins the source canvas width to 32. */
        for (unsigned y0 = 0; y0 < s->h; ++y0) {
            gfx_reverse_bytes(row, s->pixels + y0 * s->w, s->w);
            gfx_blit_sprite(left, top + (int)y0, s->w, 1, row);
        }
#endif
    }
    return 1;
}
#endif
