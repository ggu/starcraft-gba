/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MISSION_TEXT_DRAW_H
#define SC64_MISSION_TEXT_DRAW_H

/* The box itself: portrait, speaker and one page of wrapped text across the top
 * of the map. It is drawn over the frozen scene like any other field panel, and
 * reads generated rows only. */
enum {
    MISSION_BOX_X = 2, MISSION_BOX_Y = 14, MISSION_BOX_W = 236, MISSION_BOX_H = 65,
    MISSION_BOX_TEXT_X = 48, MISSION_BOX_TEXT_W = 186, MISSION_BOX_PAGE_BYTES = 255
};

/* Briefing portraits index the briefing palette, and the world palette is live
 * during a mission, so the bank maps them to its nearest colours. */
static void
mission_box_portrait(int portrait, int x, int y)
{
    u8 line[SC64_BRIEFING_PORTRAIT_WIDTH];
    const u8 *source = sc64_briefing_portraits[portrait];
    for (int row = 0; row < SC64_BRIEFING_PORTRAIT_HEIGHT; ++row) {
        for (int col = 0; col < SC64_BRIEFING_PORTRAIT_WIDTH; ++col) {
            u8 pixel = *source++;
            line[col] = pixel >= 32 ? sc64_mission_text_portrait_lut[pixel - 32] : 0;
        }
        gfx_blit_sprite(x, y + row, SC64_BRIEFING_PORTRAIT_WIDTH, 1, line);
    }
}

static void
race_mission_text_draw(void)
{
    int row_index = sc64_mission_text_row(&race_mission_text);
    if (row_index < 0) return;
    const Sc64MissionTextRow *row = &sc64_mission_text_rows[row_index];
    const Sc64MissionTextSpeaker *speaker = &sc64_mission_text_speakers[row->speaker];
    gfx_rect(MISSION_BOX_X, MISSION_BOX_Y, MISSION_BOX_W, MISSION_BOX_H, C_PANEL);
    gfx_frame(MISSION_BOX_X, MISSION_BOX_Y, MISSION_BOX_W, MISSION_BOX_H, C_GOLD);
    gfx_frame(MISSION_BOX_X + 2, MISSION_BOX_Y + 2, 42, 40, C_EDGE);
    if (speaker->portrait >= 0) {
        mission_box_portrait(speaker->portrait, MISSION_BOX_X + 3, MISSION_BOX_Y + 3);
    }
#ifdef SC64_HUD_ICONS
    else {
        int icon = card_portrait(speaker->raw_unit);
        if (icon >= 0) card_icon_draw(icon, MISSION_BOX_X + 13, MISSION_BOX_Y + 12, card_green);
    }
#endif
    sc64_font_text(MISSION_BOX_TEXT_X, MISSION_BOX_Y + 2, MISSION_BOX_TEXT_W, speaker->name, C_GOLD);
    char page[MISSION_BOX_PAGE_BYTES + 1];
    const char *text = sc64_mission_text_pool + race_mission_text.page;
    unsigned length = 0;
    while (length < MISSION_BOX_PAGE_BYTES && text[length] && text[length] != '\f') {
        page[length] = text[length];
        ++length;
    }
    page[length] = 0;
    sc64_font_text(MISSION_BOX_TEXT_X, MISSION_BOX_Y + 13, MISSION_BOX_TEXT_W, page, C_WHITE);
    if (!race_mission_text.settle)
        gfx_text(MISSION_BOX_X + MISSION_BOX_W - 18, MISSION_BOX_Y + 3, "A>", C_CYAN);
}

#endif
