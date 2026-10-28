/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_FONT_H
#define SC64_FONT_H

#include "gba.h"

#define SC64_FONT_TEXT_LIMIT 1024u
#define SC64_FONT_WIDTH_LIMIT 32767

typedef struct Sc64FontGlyph {
    u8 width, height;
    signed char x_offset, y_offset;
    const u8 *pixels; /* Source palette index per pixel; 0 and 7 are clear. */
    u8 advance;        /* Derived GBA pen advance, not a source metric. */
} Sc64FontGlyph;

typedef struct Sc64Font {
    u8 first_char, glyph_count, max_width, max_height;
    u8 space_advance, line_height;
    const Sc64FontGlyph *glyphs;
} Sc64Font;

/* The generated header is private and optional: default builds continue to
 * use the authored gfx_text renderer. */
#ifdef SC64_RACES
#include "sc64_font_generated.h"
#define SC64_FONT_SOURCE_AVAILABLE 1
#endif

static inline const Sc64FontGlyph *
sc64_font_glyph(const Sc64Font *font, unsigned char ch)
{
    unsigned index;
    if (!font || !font->glyphs) return 0;
    if (ch == ' ') return 0; /* Layout uses a derived advance; no ink glyph. */
    if (ch >= font->first_char) {
        index = ch - font->first_char;
        if (index < font->glyph_count && font->glyphs[index].pixels)
            return &font->glyphs[index];
    }
    if (ch == '?') return 0;
    index = (unsigned char)'?' - font->first_char;
    if (index < font->glyph_count && font->glyphs[index].pixels)
        return &font->glyphs[index];
    return 0;
}

static inline unsigned
sc64_font_advance_for(const Sc64Font *font, unsigned char ch)
{
    const Sc64FontGlyph *glyph;
    if (ch == ' ')
        return font && font->space_advance ? font->space_advance : 4u;
    glyph = sc64_font_glyph(font, ch);
    return glyph ? glyph->advance : (font && font->max_width ? font->max_width : 6u);
}

static inline int
sc64_font_width_for(const Sc64Font *font, const char *text)
{
    unsigned width = 0, count = 0;
    if (!text) return 0;
    while (*text && *text != '\n' && count++ < SC64_FONT_TEXT_LIMIT) {
        unsigned advance = sc64_font_advance_for(font, (unsigned char)*text++);
        if (width > SC64_FONT_WIDTH_LIMIT - advance) return SC64_FONT_WIDTH_LIMIT;
        width += advance;
    }
    return (int)width;
}

/* Draw bounded ASCII with source bearings. Each newline begins a new line;
 * max_width applies independently to every line. No palette state is changed. */
static inline void
sc64_font_text_for(const Sc64Font *font, int x, int y, int max_width,
                   const char *text, u8 color)
{
    int origin_x = x, pen = 0, clipped_line = 0;
    unsigned count = 0, line_height = font && font->line_height ? font->line_height : 10u;
    if (!text || max_width <= 0) return;
    while (*text && count++ < SC64_FONT_TEXT_LIMIT) {
        unsigned char ch = (unsigned char)*text++;
        if (ch == '\n') {
            x = origin_x;
            pen = 0;
            clipped_line = 0;
            y += (int)line_height;
            continue;
        }
        if (clipped_line) continue;
        unsigned advance = sc64_font_advance_for(font, ch);
        if (pen > max_width - (int)advance) { clipped_line = 1; continue; }
        const Sc64FontGlyph *glyph = sc64_font_glyph(font, ch);
        if (glyph) {
            for (unsigned py = 0; py < glyph->height; ++py) {
                int draw_y = y + glyph->y_offset + (int)py;
                if (draw_y < 0 || draw_y >= 160) continue;
                for (unsigned px = 0; px < glyph->width; ++px) {
                    u8 source = glyph->pixels[py * glyph->width + px];
                    int draw_x = x + glyph->x_offset + (int)px;
                    if (source != 0 && source != 7 && draw_x >= 0 && draw_x < 240 &&
                        draw_x - origin_x < max_width)
                        gfx_pixel(draw_x, draw_y, color);
                }
            }
        }
        x += (int)advance;
        pen += (int)advance;
    }
}

/* Copy a single line, adding as many periods as fit when truncated. */
static inline unsigned
sc64_font_ellipsis_for(const Sc64Font *font, const char *text, int max_width,
                       char *out, unsigned out_size)
{
    unsigned length = 0, count = 0, total_width = 0;
    if (!out || out_size == 0) return 0;
    out[0] = 0;
    if (!text || max_width <= 0) return 0;
    while (text[length] && text[length] != '\n' && count++ < SC64_FONT_TEXT_LIMIT) {
        unsigned advance = sc64_font_advance_for(font, (unsigned char)text[length]);
        if (total_width > (unsigned)max_width ||
            advance > (unsigned)max_width - total_width)
            total_width = (unsigned)max_width + 1u;
        else
            total_width += advance;
        ++length;
    }
    if ((!text[length] || text[length] == '\n') && total_width <= (unsigned)max_width) {
        unsigned copied = length < out_size - 1 ? length : out_size - 1;
        for (unsigned i = 0; i < copied; ++i) out[i] = text[i];
        out[copied] = 0;
        return copied;
    }
    unsigned dots = 0, prefix = 0, width = 0;
    int dot_advance = (int)sc64_font_advance_for(font, '.');
    if (dot_advance < 1) dot_advance = 1;
    while (dots < 3 && (int)(dots + 1) * dot_advance <= max_width && dots + 1 < out_size)
        ++dots;
    int prefix_limit = max_width - (int)dots * dot_advance;
    while (prefix < length && prefix + 1 + dots < out_size) {
        unsigned advance = sc64_font_advance_for(font, (unsigned char)text[prefix]);
        if (width > (unsigned)prefix_limit || advance > (unsigned)prefix_limit - width) break;
        width += advance;
        ++prefix;
    }
    for (unsigned i = 0; i < prefix; ++i) out[i] = text[i];
    for (unsigned i = 0; i < dots; ++i) out[prefix + i] = '.';
    out[prefix + dots] = 0;
    return prefix + dots;
}

static inline unsigned
sc64_font_number_for(int value, char *out, unsigned out_size)
{
    char reversed[12];
    unsigned n = 0, written = 0;
    unsigned magnitude;
    int negative = value < 0;
    if (!out || !out_size) return 0;
    magnitude = negative ? 0u - (unsigned)value : (unsigned)value;
    do {
        reversed[n++] = (char)('0' + magnitude % 10u);
        magnitude /= 10u;
    } while (magnitude && n < sizeof(reversed));
    if (negative && written + 1 < out_size) out[written++] = '-';
    while (n && written + 1 < out_size) out[written++] = reversed[--n];
    out[written] = 0;
    return written;
}

/* Default small-font call surface used by the unified frontend. */
static inline int sc64_font_width(const char *text)
{
#ifdef SC64_FONT_SOURCE_AVAILABLE
    return sc64_font_width_for(&sc64_font_small, text);
#else
    int width = 0;
    if (text) while (*text && *text != '\n' && width < SC64_FONT_WIDTH_LIMIT - 6) { ++text; width += 6; }
    return width;
#endif
}

static inline void sc64_font_text(int x, int y, int max_width,
                                  const char *text, u8 color)
{
#ifdef SC64_FONT_SOURCE_AVAILABLE
    sc64_font_text_for(&sc64_font_small, x, y, max_width, text, color);
#else
    char bounded[SC64_FONT_TEXT_LIMIT + 1];
    unsigned count = 0;
    int origin_x = x;
    if (!text || max_width <= 0) return;
    while (*text && count++ < SC64_FONT_TEXT_LIMIT) {
        unsigned n = 0;
        int line_width = 0;
        while (*text && *text != '\n' && n < SC64_FONT_TEXT_LIMIT) {
            if (line_width + 6 > max_width) {
                while (*text && *text != '\n') ++text;
                break;
            }
            bounded[n++] = *text++;
            line_width += 6;
        }
        bounded[n] = 0;
        gfx_text(origin_x, y, bounded, color);
        if (*text == '\n') {
            ++text;
            ++count;
            y += 8;
        }
        if (count >= SC64_FONT_TEXT_LIMIT) break;
        if (!*text) break;
    }
#endif
}

static inline unsigned sc64_font_ellipsis(const char *text, int max_width,
                                         char *out, unsigned out_size)
{
#ifdef SC64_FONT_SOURCE_AVAILABLE
    return sc64_font_ellipsis_for(&sc64_font_small, text, max_width, out, out_size);
#else
    unsigned n = 0, max_chars = max_width > 0 ? (unsigned)max_width / 6u : 0;
    if (!out || !out_size) return 0;
    while (text && text[n] && text[n] != '\n' && n < max_chars && n + 1 < out_size) ++n;
    if (!text) n = 0;
    for (unsigned i = 0; i < n; ++i) out[i] = text[i];
    if (text && text[n] && text[n] != '\n') {
        while (n && n + 4 >= out_size) --n;
        if (n + 3 < out_size) { out[n++]='.'; out[n++]='.'; out[n++]='.'; }
    }
    out[n] = 0;
    return n;
#endif
}

static inline void sc64_font_num(int x, int y, int max_width, int value, u8 color)
{
    char text[13];
    unsigned size = sc64_font_number_for(value, text, sizeof(text));
    (void)size;
    sc64_font_text(x, y, max_width, text, color);
}

#endif
