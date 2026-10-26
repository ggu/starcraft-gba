/* SPDX-License-Identifier: GPL-3.0-or-later */
/* For the source death art test, after including
 * common/gba.c and starcraft/game.c: an independent reference for
 * death_vfx_blit, and a sweep that draws a real frame at both pixel parities,
 * against both side edges and through the battlefield clip rows. */
#ifndef SC64_SOURCE_BLIT_CHECK_H
#define SC64_SOURCE_BLIT_CHECK_H
#ifdef SC64_DEATH_SOURCE_ART
#ifndef SC64_SPAN_BLIT
#error "source blit checks exercise the span path"
#endif
#include <assert.h>
#include <string.h>

/* death_vfx_blit as specified: raw pixels, rows 12..119, gfx_blit_sprite. */
static void reference_source_blit(const Sc64SpriteFrame *s, int cx, int cy)
{
    int x = cx - s->canvas_w / 2 + s->offset_x;
    int y = cy - s->canvas_h / 2 + s->offset_y, h = s->h;
    const u8 *p = s->pixels;
    if (y < 12) { int skip = 12 - y; if (skip >= h) return; p += skip * s->w; h -= skip; y = 12; }
    if (y + h > 120) h = 120 - y;
    if (h > 0) gfx_blit_sprite(x, y, s->w, h, p);
}

static unsigned check_source_frame_blits(const Sc64SpriteFrame *s, const u16 *spans)
{
    static u16 background[19200], expected[19200];
    static int ready;
    const int xs[] = { 0, 1, 2, 3, 117, 240 - s->w, 239 - s->w, -1, -2, 241 - s->w };
    const int ys[] = { 40, 12, 11, 12 - s->h / 2, 120 - s->h, 121 - s->h };
    unsigned cases = 0;
    assert(s && spans);
    if (!ready) {
        unsigned rng = 99u;
        for (int i = 0; i < 19200; ++i) background[i] = (u16)((rng = rng * 1103515245u + 12345u) >> 5);
        ready = 1;
    }
    for (unsigned xi = 0; xi < sizeof(xs) / sizeof(xs[0]); ++xi)
        for (unsigned yi = 0; yi < sizeof(ys) / sizeof(ys[0]); ++yi) {
            int cx = xs[xi] + s->canvas_w / 2 - s->offset_x;
            int cy = ys[yi] + s->canvas_h / 2 - s->offset_y;
            memcpy((void *)pixels, background, sizeof(background));
            reference_source_blit(s, cx, cy);
            memcpy(expected, (const void *)pixels, sizeof(expected));
            memcpy((void *)pixels, background, sizeof(background));
            death_vfx_blit(s, spans, cx, cy);
            assert(memcmp(expected, (const void *)pixels, sizeof(expected)) == 0);
            ++cases;
        }
    return cases;
}
#endif
#endif
