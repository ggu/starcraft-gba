/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MISSION_TEXT_H
#define SC64_MISSION_TEXT_H

/* Modal in-mission text boxes for Transmission and DisplayTextMessage rows.
 *
 * Mission interpreters post rows while they run. The world stops until the
 * player has read the queue: game_update() asks the gate below before any field
 * input or simulation work, so a posted box holds combat, AI, production,
 * trigger clocks and the pause menu alike. Because a queue always drains before
 * the next simulation tick and before a save can be written, a box is never in
 * a save and the save format is unchanged.
 *
 * The model holds no game state and needs no generated data, so it is tested
 * without private inputs. Storage is fixed: a queue overflow drops the newest
 * row and counts it. Interpreters run one trigger's rows at most in a tick;
 * tools/sc64_mission_text_pack.py reports the largest such run. */

#include "gba.h"

/* The fullest campaign candidates have no working RAM to spare, so this small
 * state lives in IWRAM. Its own section keeps it apart from IWRAM code. */
#ifdef GBA_HOST
#define SC64_MISSION_TEXT_STATE
#else
#define SC64_MISSION_TEXT_STATE __attribute__((section(".iwram.mission_text")))
#endif

typedef struct {
    const char *name;
    signed char portrait; /* Briefing portrait, or -1 for the HUD unit icon. */
    u8 raw_unit;
} Sc64MissionTextSpeaker;

typedef struct {
    u16 string_id, unit;
    u16 text;              /* Pool offset: rows split by \n, pages by \f. */
    u8 trigger, action, opcode, speaker;
} Sc64MissionTextRow;

enum {
    SC64_MISSION_TEXT_QUEUE = 16,
    /* Ignore A for a moment on each new page, so mashing A during a fight
     * cannot dismiss a line nobody saw. */
    SC64_MISSION_TEXT_SETTLE_FRAMES = 12,
    SC64_MISSION_TEXT_OPCODE_TRANSMISSION = 7,
    SC64_MISSION_TEXT_OPCODE_DISPLAY = 9
};

enum {
    SC64_MISSION_TEXT_IDLE = 0,
    SC64_MISSION_TEXT_SHOWING,  /* The head row is on screen. */
    SC64_MISSION_TEXT_RELEASE,  /* Queue read; wait for the dismissing A to lift. */
    SC64_MISSION_TEXT_RESUME    /* The next frame runs with stale presses masked. */
};

typedef struct {
    u16 queue[SC64_MISSION_TEXT_QUEUE];
    u16 page;          /* Pool offset of the page on screen. */
    u16 held_at_open;  /* Keys already down when the box took the input. */
    u16 posted, dropped;
    u8 head, count, state, settle;
    u8 opened;         /* held_at_open is valid for this run of boxes. */
} Sc64MissionText;

_Static_assert(sizeof(((Sc64MissionText *)0)->queue) == 32, "mission text queue size");

static inline void sc64_mission_text_reset(Sc64MissionText *t)
{
    u8 *p = (u8 *)t;
    for (unsigned i = 0; i < sizeof(*t); ++i) p[i] = 0;
}

/* A search of one mission's row range; missions hold at most a few dozen. */
static inline int sc64_mission_text_find_string(const Sc64MissionTextRow *rows, unsigned first,
                                                unsigned last, unsigned opcode, u32 string_id,
                                                unsigned unit)
{
    if (string_id > 65535u) return -1;
    for (unsigned i = first; i < last; ++i)
        if (rows[i].opcode == opcode && rows[i].string_id == string_id &&
            (opcode != SC64_MISSION_TEXT_OPCODE_TRANSMISSION || rows[i].unit == unit))
            return (int)i;
    return -1;
}

static inline int sc64_mission_text_find_source(const Sc64MissionTextRow *rows, unsigned first,
                                                unsigned last, unsigned trigger, unsigned action)
{
    for (unsigned i = first; i < last; ++i)
        if (rows[i].trigger == trigger && rows[i].action == action) return (int)i;
    return -1;
}

static inline const char *sc64_mission_text_next_page(const char *text)
{
    while (*text && *text != '\f') ++text;
    return *text ? text + 1 : (const char *)0;
}

static inline void sc64_mission_text_post(Sc64MissionText *t, const Sc64MissionTextRow *rows, int row)
{
    if (t->posted != 65535u) ++t->posted;
    if (row < 0) return;
    if (t->count >= SC64_MISSION_TEXT_QUEUE) {
        if (t->dropped != 65535u) ++t->dropped;
        return;
    }
    t->queue[(t->head + t->count) % SC64_MISSION_TEXT_QUEUE] = (u16)row;
    if (!t->count) {
        t->page = rows[row].text;
        t->settle = SC64_MISSION_TEXT_SETTLE_FRAMES;
    }
    ++t->count;
    t->state = SC64_MISSION_TEXT_SHOWING;
}

static inline int sc64_mission_text_showing(const Sc64MissionText *t)
{
    return t->state == SC64_MISSION_TEXT_SHOWING;
}

/* Any state but idle owns this frame's input. */
static inline int sc64_mission_text_gated(const Sc64MissionText *t)
{
    return t->state != SC64_MISSION_TEXT_IDLE;
}

static inline int sc64_mission_text_row(const Sc64MissionText *t)
{
    return sc64_mission_text_showing(t) ? t->queue[t->head] : -1;
}

/* Run one frame of input. Returns 1 when the frame belongs to the box, so the
 * caller must not simulate; otherwise *pressed is safe for the field.
 *
 * Keys already down when a box opened are not new presses when it closes, so
 * a held key neither repeats nor loses an action across a box. Every swallowed
 * frame leaves the held-key history intact for a replay that keeps its own. */
static inline int sc64_mission_text_input(Sc64MissionText *t, const Sc64MissionTextRow *rows,
                                          const char *pool, u16 held, u16 *pressed)
{
    switch (t->state) {
    case SC64_MISSION_TEXT_SHOWING: {
        if (!t->opened) {
            t->opened = 1;
            t->held_at_open = (u16)(held & (u16)~*pressed);
        }
        if (t->settle) {
            --t->settle;
            return 1;
        }
        if (!(*pressed & KEY_A)) return 1;
        const char *next = sc64_mission_text_next_page(pool + t->page);
        if (next) {
            t->page = (u16)(next - pool);
        } else {
            t->head = (u8)((t->head + 1u) % SC64_MISSION_TEXT_QUEUE);
            if (--t->count) {
                t->page = rows[t->queue[t->head]].text;
            } else {
                t->state = SC64_MISSION_TEXT_RELEASE;
                return 1;
            }
        }
        t->settle = SC64_MISSION_TEXT_SETTLE_FRAMES;
        return 1;
    }
    case SC64_MISSION_TEXT_RELEASE:
        if (!(held & KEY_A)) t->state = SC64_MISSION_TEXT_RESUME;
        return 1;
    case SC64_MISSION_TEXT_RESUME:
        *pressed = (u16)(*pressed & (u16)~t->held_at_open);
        t->state = SC64_MISSION_TEXT_IDLE;
        t->opened = 0;
        t->held_at_open = 0;
        return 0;
    default:
        return 0;
    }
}

#endif
