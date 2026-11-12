/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NUKE_RENDER_H
#define SC64_NUKE_RENDER_H
#include "../common/gba.h"
#include "sc64_nuke_source_generated.h"

enum { SC64_NUKE_ROW_MAX = 126 };

static int
sc64_nuke_expand_row(const uint8_t **input, const uint8_t *end, uint8_t *row,
                     unsigned width)
{
    unsigned col = 0;
    while (col < width) {
        if (*input == end) return 0;
        unsigned token = *(*input)++;
        unsigned n = token & 63u;
        unsigned kind = token >> 6;
        if (!n || n > width - col || kind == 3) return 0;
        if (kind == 0) {
            if ((unsigned)(end - *input) < n) return 0;
            for (unsigned i = 0; i < n; ++i) row[col++] = *(*input)++;
        } else {
            uint8_t value = 0;
            if (kind == 1) {
                if (*input == end) return 0;
                value = *(*input)++;
            }
            for (unsigned i = 0; i < n; ++i) row[col++] = value;
        }
    }
    return 1;
}

/* Positions are projected screen centers; world/camera clipping remains
 * with the caller. Rows are decoded only when the whole sprite overlaps the
 * screen. Modifier 9 modifies the destination, exactly as for source weapons. */
static int
sc64_nuke_draw_frame(const Sc64NukeFrame *f, int cx, int cy)
{
    uint8_t row[SC64_NUKE_ROW_MAX];
    if (!f || !f->w || !f->h || f->w > sizeof(row) || f->x + f->w > f->cw ||
        f->y + f->h > f->ch || (f->modifier != 0 && f->modifier != 9) ||
        f->offset > sizeof(sc64_nuke_pixels) ||
        f->length > sizeof(sc64_nuke_pixels) - f->offset)
        return 0;
    int left = cx - f->cw / 2 + f->x, top = cy - f->ch / 2 + f->y;
    if (left >= 240 || left + f->w <= 0 || top >= 160 || top + f->h <= 0)
        return 1;
    const uint8_t *input = sc64_nuke_pixels + f->offset, *end = input + f->length;
    for (unsigned y = 0; y < f->h; ++y) {
        if (!sc64_nuke_expand_row(&input, end, row, f->w)) return 0;
        int screen_y = top + (int)y;
        if (screen_y < 0 || screen_y >= 160) continue;
        if (f->modifier == 9)
            gfx_blit_sprite_mapped(left, screen_y, f->w, 1, row,
                                   sc64_nuke_light_lut, SC64_NUKE_LIGHT_ROWS);
        else
            gfx_blit_sprite(left, screen_y, f->w, 1, row);
    }
    return input == end;
}
#endif
