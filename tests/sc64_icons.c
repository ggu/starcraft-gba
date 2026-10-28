/* Optional real-table render oracle for the private command icon atlas.
 * Copyright (C) 2026 blizzard-gba contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#define SC64_RACES 1
#include "../common/gba.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_icons.h"

static u8 framebuffer[240 * 160];

void gfx_pixel(int x, int y, u8 color)
{
    if ((unsigned)x < 240u && (unsigned)y < 160u)
        framebuffer[y * 240 + x] = color;
}

int main(void)
{
#ifdef SC64_ICON_TABLE_PRESENT
    const Sc64IconFrame *icon = &sc64_icons[0];
    unsigned expected = 0, rendered = 0;
    assert(icon->width && icon->height && icon->width <= 20 && icon->height <= 20);
    assert(icon->offset + (unsigned)icon->width * icon->height <= sizeof(sc64_icon_pixels));
    for (unsigned i = 0; i < (unsigned)icon->width * icon->height; ++i) {
        u8 color = sc64_icon_pixels[icon->offset + i];
        if (color) {
            assert(color >= 32 && color <= 255);
            ++expected;
        }
    }
    assert(expected >= 4);
    assert(sc64_icon_draw_centered(0, 120, 80, 22));
    for (unsigned i = 0; i < sizeof(framebuffer); ++i)
        rendered += framebuffer[i] != 0;
    assert(rendered == expected);
    assert(!sc64_icon_draw_centered(SC64_ICON_COUNT, 120, 80, 22));
    assert(sc64_icon_draw_centered(SC64_ICON_MOVE, 120, 80, 18));
    puts("SC64 source icon table render oracle passed");
#else
    assert(!sc64_icon_draw_centered(0, 120, 80, 22));
    puts("SC64 source icon table unavailable; procedural fallback remains active");
#endif
    return 0;
}
