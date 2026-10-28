/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CHROME_H
#define SC64_CHROME_H

#include "sc64_chrome_generated.h"

enum {
    SC64_CHROME_TAB_OVERVIEW = 0,
    SC64_CHROME_TAB_UNITS,
    SC64_CHROME_TAB_BUILDINGS,
    SC64_CHROME_TAB_RESOURCES
};

static inline const u8 *sc64_chrome_menu_backdrop(int scene)
{
    switch (scene) {
    case 1: return sc64_chrome_menu_zerg;
    case 2: return sc64_chrome_menu_protoss;
    case 3: return sc64_chrome_menu_terran;
    default: return sc64_chrome_menu_home;
    }
}

static inline void sc64_chrome_draw_tab(int tab)
{
    static const char *const labels[] = {
        "Home", "Campaign", "Missions", "Custom Game"
    };
    const char *label = tab >= SC64_CHROME_TAB_OVERVIEW &&
        tab <= SC64_CHROME_TAB_RESOURCES ? labels[tab] : labels[0];
    gfx_blit_sprite(10, 5, SC64_CHROME_RAIL_WIDTH,
                    SC64_CHROME_RAIL_HEIGHT, sc64_chrome_rail);
    int width = sc64_font_width(label);
    int x = (240 - width) / 2;
    if (x < 14) x = 14;
    sc64_font_text(x, 10, 212, label, C_GOLD);
}

static inline void sc64_chrome_draw_title_plate(int x, int y, const char *label)
{
    gfx_blit_sprite(x, y, SC64_CHROME_TITLE_PLATE_WIDTH,
                    SC64_CHROME_TITLE_PLATE_HEIGHT, sc64_chrome_title_plate);
    int width = sc64_font_width(label);
    int left = x + (SC64_CHROME_TITLE_PLATE_WIDTH - width) / 2;
    if (left < x + 4) left = x + 4;
    sc64_font_text(left, y + 10, SC64_CHROME_TITLE_PLATE_WIDTH - 8, label, C_GOLD);
}

static inline void sc64_chrome_button(int x, int y, int compact, int tall,
                                      const char *label, int selected)
{
    int width = compact ? SC64_CHROME_SMALL_BUTTON_WIDTH : SC64_CHROME_BUTTON_WIDTH;
    int height = compact ? SC64_CHROME_SMALL_BUTTON_HEIGHT : tall ?
        SC64_CHROME_TALL_BUTTON_HEIGHT : SC64_CHROME_BUTTON_HEIGHT;
    const u8 *pixels;
    if (compact)
        pixels = selected ? sc64_chrome_button_small_selected : sc64_chrome_button_small_idle;
    else if (tall)
        pixels = selected ? sc64_chrome_button_tall_selected : sc64_chrome_button_tall_idle;
    else
        pixels = selected ? sc64_chrome_button_selected : sc64_chrome_button_idle;
    gfx_blit_sprite(x, y, width, height, pixels);
    int measured = sc64_font_width(label);
    if (tall && measured > width - 12) {
        char first[64], second[64];
        unsigned length = 0, split = 0;
        while (label[length] && length + 1 < sizeof(first)) {
            if (label[length] == ' ') {
                for (unsigned i = 0; i < length; ++i) first[i] = label[i];
                first[length] = 0;
                if (sc64_font_width(first) <= width - 12) split = length;
            }
            ++length;
        }
        if (split) {
            for (unsigned i = 0; i < split; ++i) first[i] = label[i];
            first[split] = 0;
            unsigned j = 0;
            for (unsigned i = split + 1; label[i] && j + 1 < sizeof(second); ++i)
                second[j++] = label[i];
            second[j] = 0;
            int first_width = sc64_font_width(first), second_width = sc64_font_width(second);
            sc64_font_text(x + (width - first_width) / 2, y + 4,
                           width - 8, first, selected ? C_GOLD : C_SILVER);
            sc64_font_text(x + (width - second_width) / 2, y + 15,
                           width - 8, second, selected ? C_GOLD : C_SILVER);
            return;
        }
    }
    int left = (width - measured) / 2;
    if (left < 4) left = 4;
    char fitted[64];
    const char *visible = label;
    if (measured > width - 8) {
        sc64_font_ellipsis(label,width-8,fitted,sizeof(fitted));
        visible = fitted;
        measured = sc64_font_width(visible);
        left = (width - measured) / 2;
        if (left < 4) left = 4;
    }
    sc64_font_text(x + left, y + (height - 10) / 2, width - left - 4,
                   visible, selected ? C_GOLD : C_SILVER);
}

static inline void sc64_chrome_draw_footer(const char *label)
{
    gfx_blit_sprite(10, 138, SC64_CHROME_FOOTER_WIDTH,
                    SC64_CHROME_FOOTER_HEIGHT, sc64_chrome_rail);
    char fitted[64];
    sc64_font_ellipsis(label,208,fitted,sizeof(fitted));
    sc64_font_text(16, 142, 208, fitted, C_DIM);
}

static inline void sc64_chrome_draw_notice(const char *title, const char *message)
{
    gfx_blit_sprite(16, 80, SC64_CHROME_NOTICE_WIDTH,
                    SC64_CHROME_NOTICE_HEIGHT, sc64_chrome_notice);
    int width = sc64_font_width(title);
    sc64_font_text((240 - width) / 2, 97, 196, title, C_GOLD);
    width = sc64_font_width(message);
    sc64_font_text((240 - width) / 2, 113, 196, message, C_SILVER);
}

/* Transparent original 006/033 rails are mapped to the current raw world
 * palette. The source panel's flat glass color is omitted, exposing the map. */
static inline void sc64_chrome_panel_world(int x, int y, int width, int height)
{
    enum { BORDER_X = 6, BORDER_Y = 5 };
    if (width <= 2 * BORDER_X + 1 || height <= 2 * BORDER_Y + 1 || x >= 240 || y >= 160 ||
        x + width <= 0 || y + height <= 0) return;
    for (int dy = 0; dy < height; ++dy) {
        int sy = dy < BORDER_Y ? dy : dy >= height - BORDER_Y ?
            SC64_CHROME_WORLD_PANEL_HEIGHT - (height - dy) :
            BORDER_Y + (dy - BORDER_Y) *
                (SC64_CHROME_WORLD_PANEL_HEIGHT - 2 * BORDER_Y - 1) /
                (height - 2 * BORDER_Y - 1);
        int py = y + dy;
        if (py < 0 || py >= 160) continue;
        for (int dx = 0; dx < width; ++dx) {
            if (dx >= BORDER_X && dx < width - BORDER_X &&
                dy >= BORDER_Y && dy < height - BORDER_Y) continue;
            int sx = dx < BORDER_X ? dx : dx >= width - BORDER_X ?
                SC64_CHROME_WORLD_PANEL_WIDTH - (width - dx) :
                BORDER_X + (dx - BORDER_X) *
                    (SC64_CHROME_WORLD_PANEL_WIDTH - 2 * BORDER_X - 1) /
                    (width - 2 * BORDER_X - 1);
            u8 strength = sc64_chrome_world_panel_pixels[
                sy * SC64_CHROME_WORLD_PANEL_WIDTH + sx];
            int px = x + dx;
            if (strength && px >= 0 && px < 240)
                gfx_pixel(px, py,
                          sc64_chrome_world_panel_colors[strength - 1]);
        }
    }
}

#endif
