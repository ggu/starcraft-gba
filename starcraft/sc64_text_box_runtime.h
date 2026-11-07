/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Mission-side text boxes: row lookup, the input frame that replaces a
 * simulation step, and the draw. See sc64_text_box.h. */
#ifndef SC64_TEXT_BOX_RUNTIME_H
#define SC64_TEXT_BOX_RUNTIME_H

#include "sc64_text_box_hook.h"

#include "sc64_text_box.h"
#include "sc64_text_box_generated.h"

#define SC64_TEXT_BOX_MISSIONS \
    (sizeof(sc64_text_box_missions) / sizeof(sc64_text_box_missions[0]))

static const Sc64TextBoxMission *race_text_box_mission(void)
{
    if (race_mode || race_selected_mission < 0 ||
        (unsigned)race_selected_mission >= SC64_TEXT_BOX_MISSIONS) return 0;
    return &sc64_text_box_missions[race_selected_mission];
}

#ifdef SC64_TEXT_BOX_HOST_LOG
#include <stdio.h>
#define RACE_TEXT_BOX_LOG(row) fprintf(stderr, "TEXTBOX mission=%d trigger=%d action=%d frame=%ld\n", \
    race_selected_mission, (row)->trigger, (row)->action, (long)frame)
#else
#define RACE_TEXT_BOX_LOG(row) ((void)0)
#endif

/* Rows are few, and a post is a rare event, so a bounded scan is enough. */
static int race_text_box_post_string(unsigned long string_id)
{
    const Sc64TextBoxMission *m = race_text_box_mission();
    if (!m) return 0;
    for (unsigned i = 0; i < m->row_count; ++i)
        if (m->rows[i].string_id == string_id) {
            RACE_TEXT_BOX_LOG(&m->rows[i]);
            return sc64_text_box_contains(i) || sc64_text_box_push(i);
        }
    return 0;
}

static int race_text_box_post_row(unsigned trigger, unsigned action)
{
    const Sc64TextBoxMission *m = race_text_box_mission();
    if (!m) return 0;
    for (unsigned i = 0; i < m->row_count; ++i)
        if (m->rows[i].trigger == trigger && m->rows[i].action == action) {
            RACE_TEXT_BOX_LOG(&m->rows[i]);
            return sc64_text_box_contains(i) || sc64_text_box_push(i);
        }
    return 0;
}

static const Sc64TextBoxRow *race_text_box_front_row(void)
{
    const Sc64TextBoxMission *m = race_text_box_mission();
    return m && sc64_text_box_count() && sc64_text_box_front() < m->row_count ?
        &m->rows[sc64_text_box_front()] : 0;
}

/* One frame with a box up. A stale row closes rather than hold the game. */
static void race_text_box_input(u16 held, u16 pressed)
{
    const Sc64TextBoxRow *row = race_text_box_front_row();
    if (!row) { sc64_text_box_reset(); return; }
    sc64_text_box_input(held, pressed, row->page_count);
}

#if defined(GBA_HOST) && !defined(SC64_TEXT_BOX_HOST_MODAL)
/* Host rule fixtures step game_update() with no one to press a key. Unless a
 * fixture asks for the modal behaviour, a box counts as read and closes at once.
 * Since a box takes no simulation step, those fixtures see the same world either
 * way, and tests/sc64_text_box_game.c holds the modal builds to that. */
static unsigned sc64_text_box_host_shown;
static void race_text_box_host_acknowledge(void)
{
    while (sc64_text_box_count()) {
        ++sc64_text_box_host_shown;
        sc64_text_box_pop();
    }
}
#else
#define race_text_box_host_acknowledge() ((void)0)
#endif

enum {
    SC64_TEXT_BOX_PANEL_X = 4, SC64_TEXT_BOX_PANEL_Y = 14,
    SC64_TEXT_BOX_PANEL_W = 232, SC64_TEXT_BOX_PANEL_H = 58,
    SC64_TEXT_BOX_HEAD_X = SC64_TEXT_BOX_PANEL_X + 5, SC64_TEXT_BOX_HEAD_Y = SC64_TEXT_BOX_PANEL_Y + 5,
    SC64_TEXT_BOX_TEXT_X = SC64_TEXT_BOX_PANEL_X + 39, SC64_TEXT_BOX_NAME_Y = SC64_TEXT_BOX_PANEL_Y + 3,
    SC64_TEXT_BOX_TEXT_Y = SC64_TEXT_BOX_PANEL_Y + 14
};
_Static_assert(SC64_TEXT_BOX_TEXT_X + SC64_TEXT_BOX_TEXT_WIDTH <= SC64_TEXT_BOX_PANEL_X + SC64_TEXT_BOX_PANEL_W - 2,
               "text-box text overruns its panel");
_Static_assert(SC64_TEXT_BOX_TEXT_Y + SC64_TEXT_BOX_TEXT_ROWS * 10 <= SC64_TEXT_BOX_PANEL_Y + SC64_TEXT_BOX_PANEL_H,
               "text-box text overruns its panel");
_Static_assert(SC64_TEXT_BOX_PANEL_Y >= 12 && SC64_TEXT_BOX_PANEL_Y + SC64_TEXT_BOX_PANEL_H <= 120,
               "the text box must stay clear of the cached HUD bands");

/* Drawn over the battlefield after the rest of the scene, like the pause panel.
 * It stays between the cached HUD bands, so they need no invalidation. */
static void race_text_box_draw(void)
{
    const Sc64TextBoxRow *row = race_text_box_front_row();
    unsigned page;
    const u8 *head;
    int y;
    if (!row) return;
    page = sc64_text_box_page();
    if (page >= row->page_count) page = row->page_count - 1u;
    gfx_rect(SC64_TEXT_BOX_PANEL_X, SC64_TEXT_BOX_PANEL_Y, SC64_TEXT_BOX_PANEL_W, SC64_TEXT_BOX_PANEL_H, C_PANEL);
    gfx_frame(SC64_TEXT_BOX_PANEL_X, SC64_TEXT_BOX_PANEL_Y, SC64_TEXT_BOX_PANEL_W, SC64_TEXT_BOX_PANEL_H, C_GOLD);
    head = sc64_text_box_heads[row->head];
    for (y = 0; y < SC64_TEXT_BOX_HEAD_HEIGHT; ++y)
        for (int x = 0; x < SC64_TEXT_BOX_HEAD_WIDTH; ++x)
            gfx_pixel(SC64_TEXT_BOX_HEAD_X + x, SC64_TEXT_BOX_HEAD_Y + y,
                      head[y * SC64_TEXT_BOX_HEAD_WIDTH + x]);
    gfx_frame(SC64_TEXT_BOX_HEAD_X - 1, SC64_TEXT_BOX_HEAD_Y - 1,
              SC64_TEXT_BOX_HEAD_WIDTH + 2, SC64_TEXT_BOX_HEAD_HEIGHT + 2, C_EDGE);
    sc64_font_text(SC64_TEXT_BOX_TEXT_X, SC64_TEXT_BOX_NAME_Y, SC64_TEXT_BOX_TEXT_WIDTH,
                   sc64_text_box_speakers[row->speaker], C_GOLD);
    sc64_font_text(SC64_TEXT_BOX_TEXT_X, SC64_TEXT_BOX_TEXT_Y, SC64_TEXT_BOX_TEXT_WIDTH,
                   sc64_text_box_pages[row->first_page + page], C_WHITE);
    /* The prompt sits under the head, where no text can reach it. */
    y = SC64_TEXT_BOX_HEAD_Y + SC64_TEXT_BOX_HEAD_HEIGHT + 4;
    if (row->page_count > 1u) {
        char pages[4];
        pages[0] = (char)('1' + page);
        pages[1] = '/';
        pages[2] = (char)('0' + row->page_count);
        pages[3] = 0;
        gfx_text(SC64_TEXT_BOX_HEAD_X, y, pages, C_DIM);
    }
    gfx_text(SC64_TEXT_BOX_HEAD_X + 22, y, "A", C_GOLD);
}

#endif
