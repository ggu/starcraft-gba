/* Private source command icons for the imported race-engine HUD.
 * Copyright (C) 2026 blizzard-gba contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef STARCRAFT_SC64_ICONS_H
#define STARCRAFT_SC64_ICONS_H

#if defined(SC64_RACES) && defined(__has_include)
#if __has_include("sc64_icons_generated.h")
#include "sc64_icons_generated.h"
#define SC64_ICON_TABLE_PRESENT 1
#endif
#endif

#ifdef SC64_ICON_TABLE_PRESENT
/* The source importer fits each icon in a 20-pixel square. Larger valid
 * tables can still use the scalar path below. */
enum { SC64_ICON_COORD_LIMIT = 20 };

/* Produce floor(i * source / drawn) * stride with one division per scaled
 * axis. The remainder accumulates the fractional part of the source step. */
static void
sc64_icon_sample_offsets(u16 *out, unsigned source, unsigned drawn, unsigned stride)
{
    unsigned offset = 0, error = 0;
    unsigned step = source == drawn ? 1u : source / drawn;
    unsigned remainder = source - step * drawn;
    for (unsigned i = 0; i < drawn; ++i) {
        out[i] = (u16)offset;
        offset += step * stride;
        error += remainder;
        if (error >= drawn) {
            error -= drawn;
            offset += stride;
        }
    }
}
#endif

/* Draw an icon centered in a square UI slot, scaling down only when needed.
 * Returns zero for an absent or empty source icon so callers keep their
 * existing procedural glyph as a fallback. */
static int
sc64_icon_draw_centered(unsigned icon_id, int center_x, int center_y, int size)
{
#ifdef SC64_ICON_TABLE_PRESENT
    const Sc64IconFrame *icon;
    unsigned x, y, opaque = 0;
    unsigned draw_w, draw_h;
    int left, top;
    if (icon_id >= SC64_ICON_COUNT || size <= 0) return 0;
    icon = &sc64_icons[icon_id];
    if (!icon->width || !icon->height || icon->offset > sizeof(sc64_icon_pixels)) return 0;
    if ((unsigned)icon->width * icon->height > sizeof(sc64_icon_pixels) - icon->offset) return 0;
    draw_w = icon->width;
    draw_h = icon->height;
    if (draw_w > (unsigned)size || draw_h > (unsigned)size) {
        if (draw_w >= draw_h) {
            draw_h = (draw_h * (unsigned)size + draw_w / 2u) / draw_w;
            draw_w = (unsigned)size;
        } else {
            draw_w = (draw_w * (unsigned)size + draw_h / 2u) / draw_h;
            draw_h = (unsigned)size;
        }
        if (!draw_w) draw_w = 1;
        if (!draw_h) draw_h = 1;
    }
    left = center_x - (int)draw_w / 2;
    top = center_y - (int)draw_h / 2;
    if (draw_w <= SC64_ICON_COORD_LIMIT && draw_h <= SC64_ICON_COORD_LIMIT) {
        u16 columns[SC64_ICON_COORD_LIMIT], rows[SC64_ICON_COORD_LIMIT];
        const u8 *src = sc64_icon_pixels + icon->offset;
        sc64_icon_sample_offsets(columns, icon->width, draw_w, 1);
        sc64_icon_sample_offsets(rows, icon->height, draw_h, icon->width);
        for (y = 0; y < draw_h; ++y) {
            const u8 *row = src + rows[y];
            for (x = 0; x < draw_w; ++x) {
                if (row[columns[x]] && ++opaque == 4u) goto draw_sampled;
            }
        }
        return 0;
draw_sampled:
        for (y = 0; y < draw_h; ++y) {
            const u8 *row = src + rows[y];
            for (x = 0; x < draw_w; ++x) {
                u8 color = row[columns[x]];
                if (color) gfx_pixel(left + (int)x, top + (int)y, color);
            }
        }
        return 1;
    }
    for (y = 0; y < draw_h; ++y) {
        unsigned sy = y * icon->height / draw_h;
        for (x = 0; x < draw_w; ++x) {
            unsigned sx = x * icon->width / draw_w;
            if (sc64_icon_pixels[icon->offset + sy * icon->width + sx]) ++opaque;
        }
    }
    if (opaque < 4u) return 0;
    for (y = 0; y < draw_h; ++y) {
        unsigned sy = y * icon->height / draw_h;
        for (x = 0; x < draw_w; ++x) {
            unsigned sx = x * icon->width / draw_w;
            u8 color = sc64_icon_pixels[icon->offset + sy * icon->width + sx];
            if (color) {
                gfx_pixel(left + (int)x, top + (int)y, color);
            }
        }
    }
    return 1;
#else
    (void)icon_id;
    (void)center_x;
    (void)center_y;
    (void)size;
    return 0;
#endif
}

#endif /* STARCRAFT_SC64_ICONS_H */
