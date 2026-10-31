/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Authored AI backend for the three Z8 RunAIScriptAtLocation scripts.
 *
 * The native `Zer8`, `Ze8T` and `MvTe` programs are only statically decoded
 * (docs/research/z8-native-ai-policy-programs.md); their interpreter
 * semantics are not established. This is a bounded engine policy, not a
 * bytecode equivalent:
 *   - Zer8 (source slot 2) and Ze8T (slot 1) start a town. A started town
 *     queues normally paid Gateway, Stargate and Robotics products from its
 *     own source wallet and sends periodic attack-move waves of its idle
 *     combat units at the human. Zer8's source program repeats ten attack
 *     groups and Ze8T's four, so Zer8 waves come more often.
 *   - MvTe orders the trigger owner's idle hero Dark Templars (raw 74) to
 *     the script location, matching its `move_dt` command name.
 * Every order is an ordinary engine order; nothing changes HP, position or
 * ownership.
 */
#ifndef SC64_Z8_AI_H
#define SC64_Z8_AI_H

enum {
    SC64_Z8_AI_TOWNS = 2,
    SC64_Z8_AI_PRODUCE_UPDATES = 240,
    SC64_Z8_AI_FIRST_WAVE_UPDATES = 4800,
    SC64_Z8_AI_ZER8_WAVE_UPDATES = 2400,
    SC64_Z8_AI_ZE8T_WAVE_UPDATES = 3600,
    SC64_Z8_AI_HOME_GUARD = 4,
    SC64_Z8_AI_WAVE_BASE = 6,
    SC64_Z8_AI_WAVE_MAX = 16,
    /* Keep pool room for trigger-created Dark Templars and the human. */
    SC64_Z8_AI_RESERVE_UNITS = 24,
    SC64_Z8_RAW_HERO_DARK_TEMPLAR = 74
};

typedef struct {
    u8 started;                         /* bit t: town t is running */
    u8 wave[SC64_Z8_AI_TOWNS];
    u8 produce_index[SC64_Z8_AI_TOWNS];
    u8 reserved[3];
    u16 wave_timer[SC64_Z8_AI_TOWNS];
    u16 produce_timer[SC64_Z8_AI_TOWNS];
    u8 pad[4];
} Sc64Z8AiState;

_Static_assert(sizeof(Sc64Z8AiState) == SC64_Z8_SAVE_AI_BYTES,
               "Z8 AI state fills its save bytes");

/* Town t is source slot t + 1. */
static inline int sc64_z8_ai_town_slot(unsigned town) { return (int)town + 1; }

static inline int
sc64_z8_ai_state_valid(const Sc64Z8AiState *ai)
{
    if (!ai || (ai->started & ~3u) || ai->reserved[0] || ai->reserved[1] ||
        ai->reserved[2] || ai->pad[0] || ai->pad[1] || ai->pad[2] || ai->pad[3])
        return 0;
    for (unsigned t = 0; t < SC64_Z8_AI_TOWNS; ++t) {
        if (!(ai->started & (1u << t)) &&
            (ai->wave[t] || ai->produce_index[t] || ai->wave_timer[t] ||
             ai->produce_timer[t])) return 0;
        if (ai->wave_timer[t] > SC64_Z8_AI_FIRST_WAVE_UPDATES ||
            ai->produce_timer[t] > SC64_Z8_AI_PRODUCE_UPDATES) return 0;
    }
    return 1;
}

static inline void
sc64_z8_ai_start_town(Sc64Z8AiState *ai, unsigned town)
{
    if (town >= SC64_Z8_AI_TOWNS || (ai->started & (1u << town))) return;
    ai->started |= (u8)(1u << town);
    ai->wave_timer[town] = SC64_Z8_AI_FIRST_WAVE_UPDATES;
    ai->produce_timer[town] = SC64_Z8_AI_PRODUCE_UPDATES;
}

#endif /* SC64_Z8_AI_H */
