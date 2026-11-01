/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Authored AI backend for the Z9 and Z10 RunAIScriptAtLocation scripts.
 *
 * As for Z8 (sc64_z8_ai.h), the native script programs are not decoded and
 * this is a bounded engine policy, not a bytecode equivalent. Each mission
 * names up to three Protoss towns by source slot. A town script starts its
 * town: it queues normally paid Gateway, Stargate and Robotics products from
 * the town's own source wallet and sends periodic attack-move waves of idle
 * combat units at the human. Scripts mapped to no town (Z9's `VluA`) are
 * accepted with no effect; their native meaning is not established. Every
 * order is an ordinary engine order.
 */
#ifndef SC64_ZERG_LATE_AI_H
#define SC64_ZERG_LATE_AI_H

#include "sc64_zerg_late_state.h"

enum {
    SC64_ZL_AI_PRODUCE_UPDATES = 240,
    SC64_ZL_AI_FIRST_WAVE_UPDATES = 4800,
    SC64_ZL_AI_WAVE_UPDATES = 3000,
    SC64_ZL_AI_HOME_GUARD = 4,
    SC64_ZL_AI_WAVE_BASE = 6,
    SC64_ZL_AI_WAVE_MAX = 16,
    /* Keep pool room for trigger-created actors and the human. */
    SC64_ZL_AI_RESERVE_UNITS = 24,
    SC64_ZL_AI_NO_TOWN = 0xff
};

typedef struct {
    u32 fourcc;     /* little-endian CHK group2 */
    u8 owner;       /* source slot that runs it */
    u8 town;        /* index into the mission's town list, or NO_TOWN */
} Sc64ZlAiScript;

typedef struct {
    u8 started;                         /* bit t: town t is running */
    u8 wave[SC64_ZL_AI_TOWNS];
    u8 produce_index[SC64_ZL_AI_TOWNS];
    u8 reserved;
    u16 wave_timer[SC64_ZL_AI_TOWNS];
    u16 produce_timer[SC64_ZL_AI_TOWNS];
    u8 pad[4];
} Sc64ZlAiState;

_Static_assert(sizeof(Sc64ZlAiState) == SC64_ZL_SAVE_AI_BYTES,
               "late Zerg AI state fills its save bytes");

static inline int
sc64_zl_ai_state_valid(const Sc64ZlAiState *ai)
{
    if (!ai || (ai->started & ~7u) || ai->reserved ||
        ai->pad[0] || ai->pad[1] || ai->pad[2] || ai->pad[3]) return 0;
    for (unsigned t = 0; t < SC64_ZL_AI_TOWNS; ++t) {
        if (!(ai->started & (1u << t)) &&
            (ai->wave[t] || ai->produce_index[t] || ai->wave_timer[t] ||
             ai->produce_timer[t])) return 0;
        if (ai->wave_timer[t] > SC64_ZL_AI_FIRST_WAVE_UPDATES ||
            ai->produce_timer[t] > SC64_ZL_AI_PRODUCE_UPDATES) return 0;
    }
    return 1;
}

static inline void
sc64_zl_ai_start_town(Sc64ZlAiState *ai, unsigned town)
{
    if (town >= SC64_ZL_AI_TOWNS || (ai->started & (1u << town))) return;
    ai->started |= (u8)(1u << town);
    ai->wave_timer[town] = SC64_ZL_AI_FIRST_WAVE_UPDATES;
    ai->produce_timer[town] = SC64_ZL_AI_PRODUCE_UPDATES;
}

#endif /* SC64_ZERG_LATE_AI_H */
