/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/sc64_font.h"

static unsigned char canvas[160][240];
static unsigned text_calls;

void gfx_pixel(int x, int y, u8 color)
{
    if (x >= 0 && x < 240 && y >= 0 && y < 160) canvas[y][x] = color;
}

void gfx_text(int x, int y, const char *text, u8 color)
{
    (void)x; (void)y; (void)text; (void)color;
    ++text_calls;
}

int main(void)
{
    static const u8 bang_pixels[] = {1,7,0, 7,1,0};
    static const u8 dot_pixels[] = {1};
    static const Sc64FontGlyph glyphs[64] = {
        ['!' - 32] = {3,2,-1,0,bang_pixels,3},
        ['.' - 32] = {1,1,0,0,dot_pixels,2},
        ['A' - 32] = {3,1,0,0,dot_pixels,4},
        ['?' - 32] = {2,1,0,0,dot_pixels,3}
    };
    static const Sc64Font font = {32,64,8,9,4,10,glyphs};
    char text[20];

    assert(sc64_font_width_for(&font, "A A") == 12);
    assert(sc64_font_width_for(&font, "AB\nA") == 7);
    assert(sc64_font_advance_for(&font, ' ') == 4);
    assert(sc64_font_glyph(&font, ' ') == 0);
    assert(sc64_font_advance_for(&font, 'X') == 3); /* '?' fallback */

    sc64_font_text_for(&font, 2, 3, 6, "!A", 9);
    assert(canvas[3][1] == 9 && canvas[3][2] == 0 && canvas[3][3] == 0);
    assert(canvas[4][1] == 0 && canvas[4][2] == 9 && canvas[4][3] == 0);
    /* The second glyph would exceed six pixels, so it is clipped as a whole. */
    assert(canvas[3][5] == 0);

    memset(canvas, 0, sizeof(canvas));
    sc64_font_text_for(&font, 0, 0, 6, "!\n!", 5);
    assert(canvas[0][0] == 0); /* Negative bearing stays outside the display. */
    assert(canvas[11][0] == 5);

    assert(sc64_font_ellipsis_for(&font, "ABCD", 12, text, sizeof(text)) == 4);
    assert(strcmp(text, "A...") == 0);
    assert(sc64_font_ellipsis_for(&font, "ABCD", 5, text, sizeof(text)) == 2);
    assert(strcmp(text, "..") == 0);
    assert(sc64_font_ellipsis_for(&font, "ok", 12, text, sizeof(text)) == 2);
    assert(strcmp(text, "ok") == 0);
    assert(sc64_font_number_for(-2147483647 - 1, text, sizeof(text)) == 11);
    assert(strcmp(text, "-2147483648") == 0);
    assert(sc64_font_number_for(0, text, sizeof(text)) == 1 && strcmp(text, "0") == 0);

#ifndef SC64_FONT_SOURCE_AVAILABLE
    assert(sc64_font_width("abc") == 18);
    sc64_font_text(0, 0, 40, "fallback", 1);
    assert(text_calls == 1);
#endif
    puts("StarCraft 64 source font model: ok");
    return 0;
}
