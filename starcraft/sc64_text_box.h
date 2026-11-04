/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Modal in-mission text boxes, as on the N64: a trigger row opens a box with a
 * head, a speaker name and text. The mission stops until the player acknowledges
 * it, then the next queued box opens. See docs/TEXT-BOXES.md.
 *
 * Nothing here is saved or read by the simulation. A box opens after the update
 * that posted it, and game_update() takes no simulation step while one is up, so
 * every box is a pure insertion into the input stream: the world on either side
 * is the one the same inputs would produce with no box. No save can happen while
 * a box is up, since the pause menu cannot open. A load or new game clears it.
 *
 * Z9 and Z10 have no EWRAM to spare, so the queue lives in the unused padding
 * after the second mode-4 page. It is halfword-only, as VRAM requires. */
#ifndef SC64_TEXT_BOX_H
#define SC64_TEXT_BOX_H

#include "gba.h"

typedef struct Sc64TextBoxRow {
    u16 string_id;     /* Source string; rows sharing one have identical text. */
    u16 first_page;    /* Index of the row's first wrapped page. */
    u8 trigger;        /* Source trigger record. */
    u8 action;         /* Source action slot. */
    u8 head;
    u8 speaker;
    u8 page_count;
} Sc64TextBoxRow;

typedef struct Sc64TextBoxMission {
    const Sc64TextBoxRow *rows;
    u16 row_count;
} Sc64TextBoxMission;

enum {
    SC64_TEXT_BOX_COUNT_WORD = 0, /* Queued rows; the front one is on screen. */
    SC64_TEXT_BOX_PAGE_WORD = 1,
    SC64_TEXT_BOX_ACK_WORD = 2,   /* Which acknowledging key is down, or zero. */
    SC64_TEXT_BOX_QUEUE_WORD = 3,
    SC64_TEXT_BOX_QUEUE_CAPACITY = 13,
    SC64_TEXT_BOX_WORDS_TOTAL = SC64_TEXT_BOX_QUEUE_WORD + SC64_TEXT_BOX_QUEUE_CAPACITY
};

#define SC64_TEXT_BOX_VRAM_BASE 0x06013FE0u
#define SC64_TEXT_BOX_VRAM_LIMIT 0x06014000u
_Static_assert(SC64_TEXT_BOX_VRAM_BASE + SC64_TEXT_BOX_WORDS_TOTAL * 2u <= SC64_TEXT_BOX_VRAM_LIMIT,
               "text-box state exceeds its VRAM padding");

#ifdef GBA_HOST
static u16 sc64_text_box_host_words[SC64_TEXT_BOX_WORDS_TOTAL];
#define SC64_TEXT_BOX_WORDS ((volatile u16 *)sc64_text_box_host_words)
#else
#define SC64_TEXT_BOX_WORDS ((volatile u16 *)SC64_TEXT_BOX_VRAM_BASE)
#endif

/* A or Start acknowledges. A is the N64's key. Start lets a replay dismiss a box
 * while its own recorded A stays held across it, leaving its key stream intact. */
#define SC64_TEXT_BOX_ACK_KEYS (KEY_A | KEY_START)

static inline unsigned sc64_text_box_count(void) { return SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_COUNT_WORD]; }
static inline unsigned sc64_text_box_page(void) { return SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_PAGE_WORD]; }
static inline unsigned sc64_text_box_front(void) { return SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_QUEUE_WORD]; }

static inline void sc64_text_box_reset(void)
{
    for (int i = 0; i < SC64_TEXT_BOX_WORDS_TOTAL; ++i) SC64_TEXT_BOX_WORDS[i] = 0;
}

/* Bounded: a full queue drops the new row rather than block a trigger. One
 * trigger pass may post a row a second time through another owner, so a row
 * already waiting is not queued twice. Returns 1 when the row was queued. */
static inline int sc64_text_box_push(unsigned row)
{
    unsigned count = sc64_text_box_count();
    for (unsigned i = 0; i < count; ++i)
        if (SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_QUEUE_WORD + i] == row) return 0;
    if (count >= SC64_TEXT_BOX_QUEUE_CAPACITY) return 0;
    SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_QUEUE_WORD + count] = (u16)row;
    SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_COUNT_WORD] = (u16)(count + 1u);
    return 1;
}

static inline void sc64_text_box_pop(void)
{
    unsigned count = sc64_text_box_count();
    if (!count) return;
    for (unsigned i = 1; i < count; ++i)
        SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_QUEUE_WORD + i - 1u] =
            SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_QUEUE_WORD + i];
    SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_COUNT_WORD] = (u16)(count - 1u);
    SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_PAGE_WORD] = 0;
}

/* One frame of box input, given the front row's page count. Pressing a key arms
 * it and releasing it advances, so a press never reaches the battlefield and the
 * frame after a box sees the same previous keys the box began with. Returns 1
 * when this frame advanced a page or closed a row. */
static inline int sc64_text_box_input(unsigned held, unsigned pressed, unsigned page_count)
{
    unsigned armed = SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_ACK_WORD];
    if (!sc64_text_box_count()) return 0;
    if (!armed) {
        armed = pressed & SC64_TEXT_BOX_ACK_KEYS;
        armed &= (unsigned)-(int)armed; /* One key decides, if both land together. */
        SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_ACK_WORD] = (u16)armed;
        return 0;
    }
    if (held & armed) return 0;
    SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_ACK_WORD] = 0;
    if (sc64_text_box_page() + 1u < page_count)
        SC64_TEXT_BOX_WORDS[SC64_TEXT_BOX_PAGE_WORD] = (u16)(sc64_text_box_page() + 1u);
    else
        sc64_text_box_pop();
    return 1;
}

#endif
