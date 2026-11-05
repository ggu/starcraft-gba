/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * ARM check that text boxes open and close in a shipped product's own build
 * (replaces common/main.c). It launches one mission from the ordinary menu, then
 * idles, acknowledging each box with A. Every key goes through game_update().
 * Results go to the mGBA debug log, one PX line per box. Nothing writes game
 * state. Emulator evidence only.
 */
#include "../common/gba.h"
#define SC64_TEXT_BOX_HOST_MODAL
#include "../starcraft/game.c"

#ifndef TB_MISSION
#error "Build with -DTB_MISSION=<global mission id>"
#endif
#ifndef TB_LIMIT
#define TB_LIMIT 9000u
#endif

volatile u16 demo_keys;

#define DEBUG_TEXT ((volatile char *)0x04FFF600)
/* Z9 and Z10 have no room for a line buffer: write the debug registers directly. */
static u16 line_len;
static u16 previous_held;

static void out_text(const char *s) { while (*s && line_len < 199) DEBUG_TEXT[line_len++] = *s++; }
static void out_num(long v)
{
    char tmp[21];
    int n = 0;
    if (v < 0) { out_text("-"); v = -v; }
    do { tmp[n++] = (char)('0' + (int)(v % 10)); v /= 10; } while (v && n < 20);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_field(const char *name, long v) { out_text(" "); out_text(name); out_text("="); out_num(v); }
static void out_flush(void)
{
    DEBUG_TEXT[line_len] = 0;
    *(volatile u16 *)0x04FFF700 = 0x100 | 2;
    line_len = 0;
}

static void frame_with(u16 held)
{
    demo_keys = held;
    game_update(held, held & (u16)~previous_held);
    audio_update();
    game_render();
    previous_held = held;
}
static void press(u16 key) { frame_with(key); frame_with(0); }

/* The panel's top edge on the page on screen. */
static unsigned gold_on_panel_top(void)
{
    const volatile u16 *page = (*(volatile u16 *)0x04000000 & 16u) ?
        (const volatile u16 *)0x0600A000 : (const volatile u16 *)0x06000000;
    unsigned n = 0;
    for (int x = SC64_TEXT_BOX_PANEL_X; x < SC64_TEXT_BOX_PANEL_X + SC64_TEXT_BOX_PANEL_W; ++x) {
        u16 pair = page[SC64_TEXT_BOX_PANEL_Y * 120 + x / 2];
        n += (u8)((x & 1) ? pair >> 8 : pair) == C_GOLD;
    }
    return n;
}

int main(void)
{
    int faction, ordinal;
    unsigned boxes = 0;
    gba_init();
    *(volatile u16 *)0x04FFF780 = 0xC0DE;
    game_init();
    press(KEY_A);
    faction = sc64_front_catalog_faction(race_front.faction);
    ordinal = sc64_catalog_playable_ordinal(faction, TB_MISSION);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    out_text("PX ROUTE");
    out_field("page", race_front.page); out_field("mission", race_front.mission);
    out_flush();
    press(KEY_START);
    out_text("PX START");
    out_field("screen", screen); out_field("mission", race_selected_mission);
    out_flush();
    while ((unsigned)frame < TB_LIMIT && screen != SCREEN_RESULT) {
        frame_with(0);
        while (sc64_text_box_count()) {
            const Sc64TextBoxRow *row = race_text_box_front_row();
            int frame_when_opened = frame;
            unsigned pages = row ? row->page_count : 0;
            /* One frame so both pages hold the box, then read the one on screen. */
            frame_with(0);
            frame_with(0);
            out_text("PX BOX");
            out_field("frame", frame);
            out_field("trigger", row ? row->trigger : -1);
            out_field("action", row ? row->action : -1);
            out_field("head", row ? row->head : -1);
            out_field("speaker", row ? row->speaker : -1);
            out_field("pages", pages);
            out_field("queued", sc64_text_box_count());
            out_field("gold", gold_on_panel_top());
            out_flush();
            for (unsigned p = 0; p < pages; ++p) press(KEY_A);
            if (frame != frame_when_opened) { out_text("PX MOVED"); out_flush(); }
            ++boxes;
        }
    }
    out_text("PX END");
    out_field("frame", frame); out_field("screen", screen); out_field("boxes", boxes);
    out_field("won", won); out_field("lost", lost);
    out_flush();
    out_text("PX DONE"); out_flush();
    for (;;) frame_with(0);
    return 0;
}
