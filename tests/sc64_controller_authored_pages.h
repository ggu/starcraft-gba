/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CONTROLLER_AUTHORED_PAGES_H
#define SC64_CONTROLLER_AUTHORED_PAGES_H
/* Include after game.c. The callback applies one ordinary held-key update. */
static void sc64_controller_ack_pages(void (*step)(u16), u16 held,
                                      const unsigned long *ticks, FILE *log)
{
    for (unsigned guard = 0; sc64_text_box_count(); ++guard) {
        unsigned row = sc64_text_box_front(), page = sc64_text_box_page();
        int frozen = frame;
        const Sc64TextBoxRow *text = race_text_box_front_row();
        u16 key = (held & KEY_A) ? KEY_START : KEY_A;
        if (guard >= 48 || !text || page >= text->page_count ||
            SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_ACK_WORD]) abort();
        fprintf(log, "%lu %d %u %u %u %u %u %u\n",
                *ticks, frame, row, page, text->trigger, text->action, held, key);
        step(held | key);
        if (frame != frozen || sc64_text_box_front() != row ||
            sc64_text_box_page() != page ||
            SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_ACK_WORD] != key) abort();
        step(held);
        if (frame != frozen || SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_ACK_WORD]) abort();
    }
}

static int sc64_controller_reveal_briefing(void (*press)(u16), int mission,
                                           const unsigned long *ticks, FILE *log)
{
    unsigned pages = race_front.briefing_pages;
    if (race_front.page != SC_FRONT_BRIEFING || race_front.mission != mission ||
        !pages || pages > 32) return 0;
    for (unsigned page = 0; page < pages; ++page) {
        if (race_front.briefing_page != page) return 0;
        if (!sc64_frontend_briefing_is_revealed(&race_front)) press(KEY_A);
        if (!sc64_frontend_briefing_is_revealed(&race_front)) return 0;
        fprintf(log, "%lu briefing %u %u\n", *ticks, page, pages);
        press(KEY_A);
    }
    return 1;
}
#endif
