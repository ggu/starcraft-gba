/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MISSION_TEXT_QUALIFY_H
#define SC64_MISSION_TEXT_QUALIFY_H

/* Qualification builds only (-DSC64_MISSION_TEXT_QUALIFY): put every host
 * controller in front of the real modal box. Controllers are written against
 * simulation ticks, so this layer takes the box's frames between their calls and
 * reads each row with ordinary A presses through the real game_update(). A
 * controller then sees the same world as in a pass-through run, unless the box
 * disturbs it, which is what this build exists to find. */
#ifndef GBA_HOST
#error "Mission text qualification is a host build"
#endif
#include <stdio.h>
#include <stdlib.h>

static u16 sc64_mt_q_previous;
static u32 sc64_mt_q_frames, sc64_mt_q_presses;
static u8 sc64_mt_q_seen[SC64_MISSION_TEXT_ROW_COUNT / 8u + 1u];
static u8 sc64_mt_q_started;

static void sc64_mt_q_report(void)
{
    unsigned total = 0;
    fprintf(stderr, "MT QUALIFY frames=%u presses=%u dropped=%u", sc64_mt_q_frames,
            sc64_mt_q_presses, race_mission_text.dropped);
    for (int mission = 0; mission < SC64_MISSION_TEXT_MISSION_COUNT; ++mission) {
        unsigned seen = 0, first = sc64_mission_text_first[mission], last = sc64_mission_text_first[mission + 1];
        for (unsigned i = first; i < last; ++i) seen += (sc64_mt_q_seen[i >> 3] >> (i & 7u)) & 1u;
        if (seen) fprintf(stderr, " m%d=%u/%u", mission, seen, last - first);
        total += seen;
    }
    fprintf(stderr, " rows=%u\n", total);
}

static __attribute__((unused)) void sc64_mt_q_update(u16 held, u16 pressed)
{
    if (!sc64_mt_q_started) {
        sc64_mt_q_started = 1;
        sc64_mission_text_interactive = 1;
        atexit(sc64_mt_q_report);
    }
    if (screen == SCREEN_PLAY && sc64_mission_text_gated(&race_mission_text)) {
        const u16 base = (u16)(sc64_mt_q_previous & (u16)~KEY_A);
        while (screen == SCREEN_PLAY && sc64_mission_text_gated(&race_mission_text) &&
               race_mission_text.state != SC64_MISSION_TEXT_RESUME) {
            int row = sc64_mission_text_row(&race_mission_text);
            if (row >= 0) sc64_mt_q_seen[(unsigned)row >> 3] |= (u8)(1u << ((unsigned)row & 7u));
            if (row >= 0 && !race_mission_text.settle) {
                game_update((u16)(base | KEY_A), KEY_A);
                ++sc64_mt_q_presses;
                ++sc64_mt_q_frames;
            }
            game_update(base, 0);
            ++sc64_mt_q_frames;
        }
    }
    sc64_mt_q_previous = held;
    game_update(held, pressed);
}
#define game_update sc64_mt_q_update

#endif
