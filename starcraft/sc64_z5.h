#ifndef SC64_Z5_H
#define SC64_Z5_H

#include "../common/gba.h"

/* Authored rule scheduler for Zerg mission 5 (stable source ID 16).
 * This is deliberately not a CHK interpreter. The scenario adapter must
 * resolve each source condition record against its owner/selector, unit,
 * location, comparison and switch semantics, then set one true bit per
 * satisfied condition row. In particular, record 1 checks source switch 110
 * as set before its six-action victory sequence; record 2 checks Force 1
 * (selector 18) for raw unit 51 with count at most zero. Normal active records
 * are enabled for trigger slot 18; records 28 and 30 use slot 6. The adapter
 * also owns source action effects and conversion of Wait/CountdownTimer units
 * to GBA ticks. */
enum {
    SC64_Z5_GLOBAL_ID = 16,
    SC64_Z5_MAP_COLS = 128,
    SC64_Z5_MAP_ROWS = 128,
    SC64_Z5_TRIGGER_COUNT = 40,
    SC64_Z5_ACTIVE_TRIGGER_COUNT = 38,
    SC64_Z5_ACTION_TOTAL = 155,
    SC64_Z5_CONDITION_TOTAL = 44,

    SC64_Z5_TRIGGER_VICTORY = 1,
    SC64_Z5_TRIGGER_DEFEAT = 2,
    SC64_Z5_TRIGGER_OBJECTIVE_SETUP = 3,
    SC64_Z5_TRIGGER_PRESERVE_0 = 7,
    SC64_Z5_TRIGGER_PRESERVE_1 = 8,
    SC64_Z5_TRIGGER_PRESERVE_2 = 34,
    SC64_Z5_ACTIONS_DEFEAT = 5,
    SC64_Z5_ACTIONS_VICTORY = 6,
    SC64_Z5_SELECTOR_CURRENT_PLAYER = 13,
    SC64_Z5_SELECTOR_ALL_PLAYERS = 17,
    SC64_Z5_SELECTOR_FORCE1 = 18,
    SC64_Z5_SELECTOR_FORCE2 = 19,
    SC64_Z5_RAW_KERRIGAN = 51,
    SC64_Z5_SWITCH_VICTORY_GATE = 110,

    SC64_Z5_RUNNING = 0,
    SC64_Z5_WON = 1,
    SC64_Z5_LOST = 2
};

/* Per-source action and condition row counts. Empty records 37 and 39 remain
 * represented and can never be started. These are counts only, not copied
 * source condition/action payloads. */
static inline u8 sc64_z5_action_count(u8 trigger)
{
    static const u8 counts[SC64_Z5_TRIGGER_COUNT] = {
        41, 6, 5, 2, 7, 11, 7, 5, 5, 1, 5, 1, 1, 1, 1, 1, 1, 2, 6, 3,
        1, 1, 1, 1, 1, 1, 7, 1, 1, 1, 1, 2, 2, 2, 5, 8, 5, 0, 2, 0
    };
    return trigger < SC64_Z5_TRIGGER_COUNT ? counts[trigger] : 0;
}

static inline u8 sc64_z5_condition_count(u8 trigger)
{
    static const u8 counts[SC64_Z5_TRIGGER_COUNT] = {
        1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 1, 1, 2, 2, 0, 1, 0
    };
    return trigger < SC64_Z5_TRIGGER_COUNT ? counts[trigger] : 0;
}

static inline int sc64_z5_trigger_preserved(u8 trigger)
{
    return trigger == SC64_Z5_TRIGGER_PRESERVE_0 ||
           trigger == SC64_Z5_TRIGGER_PRESERVE_1 ||
           trigger == SC64_Z5_TRIGGER_PRESERVE_2;
}

enum {
    SC64_Z5_EVENT_NONE = 0,
    SC64_Z5_EVENT_ACTION = 1u << 0,
    SC64_Z5_EVENT_WON = 1u << 1,
    SC64_Z5_EVENT_LOST = 1u << 2
};

typedef struct {
    /* Bit i means source condition row i evaluated true for the matching
     * trigger record. The model requires every authored condition row to be
     * true; adapters may not collapse a multi-condition trigger to one flag. */
    u16 condition_true[SC64_Z5_TRIGGER_COUNT];
    /* A set bit means the currently emitted action for that trigger finished. */
    u32 action_complete_lo;
    u32 action_complete_hi;
    u8 paused;
} Sc64Z5Input;

typedef struct {
    u32 started_lo;            /* One-shot source triggers that have started. */
    u32 started_hi;
    u8 outcome;
    /* Zero means idle/done; otherwise this is the current action index + 1. */
    u8 action_cursor[SC64_Z5_TRIGGER_COUNT];
} Sc64Z5State;

_Static_assert(sizeof(Sc64Z5State) == 52, "Z5 model state size");

typedef struct {
    u8 trigger;
    u8 action_index;           /* Zero based source action index. */
} Sc64Z5ActionEvent;

typedef struct {
    u8 events;
    u8 action_count;
    Sc64Z5ActionEvent actions[SC64_Z5_TRIGGER_COUNT];
} Sc64Z5Output;

static inline int sc64_z5_mask_test(u32 lo, u32 hi, u8 trigger)
{
    return trigger < 32 ? (int)((lo >> trigger) & 1u) :
           trigger < SC64_Z5_TRIGGER_COUNT ? (int)((hi >> (trigger - 32)) & 1u) : 0;
}

static inline void sc64_z5_mask_set(u32 *lo, u32 *hi, u8 trigger)
{
    if (trigger < 32) *lo |= (u32)1u << trigger;
    else if (trigger < SC64_Z5_TRIGGER_COUNT) *hi |= (u32)1u << (trigger - 32);
}

static inline u16 sc64_z5_init(Sc64Z5State *s)
{
    u8 *p;
    u32 i;
    if (!s) return SC64_Z5_EVENT_NONE;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
    return SC64_Z5_EVENT_NONE;
}

static inline int sc64_z5_state_valid(const Sc64Z5State *s)
{
    u8 i;
    if (!s || s->outcome > SC64_Z5_LOST ||
        (s->started_hi & (u32)~0xffu)) return 0;
    if ((s->started_hi & ((u32)1u << (37 - 32))) ||
        (s->started_hi & ((u32)1u << (39 - 32)))) return 0;
    for (i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        if (cursor > sc64_z5_action_count(i)) return 0;
        if (cursor && !sc64_z5_mask_test(s->started_lo, s->started_hi, i)) return 0;
        if (cursor && sc64_z5_action_count(i) == 0) return 0;
    }
    {
        u8 completed_terminal = 0;
        u8 completed_trigger = 0xffu;
        for (i = 0; i < 2; ++i) {
            u8 trigger = i == 0 ? SC64_Z5_TRIGGER_VICTORY : SC64_Z5_TRIGGER_DEFEAT;
        int started = sc64_z5_mask_test(s->started_lo, s->started_hi, trigger);
            if (started && !s->action_cursor[trigger]) {
                ++completed_terminal;
                completed_trigger = trigger;
            }
        }
        if (s->outcome == SC64_Z5_RUNNING && completed_terminal) return 0;
        if (s->outcome == SC64_Z5_WON &&
            (completed_terminal != 1 || completed_trigger != SC64_Z5_TRIGGER_VICTORY)) return 0;
        if (s->outcome == SC64_Z5_LOST &&
            (completed_terminal != 1 || completed_trigger != SC64_Z5_TRIGGER_DEFEAT)) return 0;
        if (s->outcome == SC64_Z5_WON &&
            !sc64_z5_mask_test(s->started_lo, s->started_hi, SC64_Z5_TRIGGER_VICTORY))
            return 0;
        if (s->outcome == SC64_Z5_LOST &&
            !sc64_z5_mask_test(s->started_lo, s->started_hi, SC64_Z5_TRIGGER_DEFEAT))
            return 0;
    }
    return 1;
}

static inline int sc64_z5_input_valid(const Sc64Z5Input *in)
{
    u8 i;
    if (!in || in->paused > 1 || (in->action_complete_hi & (u32)~0x5fu)) return 0;
    for (i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i) {
        u8 count = sc64_z5_condition_count(i);
        u16 allowed = count ? (u16)(((u16)1u << count) - 1u) : 0;
        if (in->condition_true[i] & (u16)~allowed) return 0;
    }
    return 1;
}

static inline int sc64_z5_input_state_valid(const Sc64Z5State *s,
                                            const Sc64Z5Input *in)
{
    u8 i;
    if (!sc64_z5_state_valid(s) || !sc64_z5_input_valid(in)) return 0;
    for (i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i) {
        if (sc64_z5_mask_test(in->action_complete_lo, in->action_complete_hi, i) &&
            !s->action_cursor[i]) return 0;
    }
    return 1;
}

static inline int sc64_z5_conditions_met(const Sc64Z5Input *in, u8 trigger)
{
    u8 count = sc64_z5_condition_count(trigger);
    u16 required;
    if (!in || !count || trigger >= SC64_Z5_TRIGGER_COUNT) return 0;
    required = (u16)(((u16)1u << count) - 1u);
    return (in->condition_true[trigger] & required) == required;
}

static inline void sc64_z5_emit(Sc64Z5Output *out, u8 trigger, u8 index)
{
    if (out->action_count >= SC64_Z5_TRIGGER_COUNT) return;
    out->actions[out->action_count].trigger = trigger;
    out->actions[out->action_count].action_index = index;
    ++out->action_count;
    out->events |= SC64_Z5_EVENT_ACTION;
}

/* Advance already active action programs, then start newly eligible records
 * in source order. A record emits at most one action per update. The source
 * win program (record 1) precedes its loss program (record 2). If terminal
 * actions complete in the same update, the lower source record wins the tie;
 * otherwise the first completed terminal action determines the outcome. */
static inline Sc64Z5Output sc64_z5_step(Sc64Z5State *s, const Sc64Z5Input *in)
{
    Sc64Z5Output out = {0, 0, {{0, 0}}};
    u8 was_active[SC64_Z5_TRIGGER_COUNT];
    u8 i;
    if (!sc64_z5_input_state_valid(s, in) || in->paused ||
        s->outcome != SC64_Z5_RUNNING)
        return out;
    for (i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i)
        was_active[i] = s->action_cursor[i] != 0;
    for (i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        if (s->outcome != SC64_Z5_RUNNING) break;
        if (!cursor || !sc64_z5_mask_test(in->action_complete_lo,
                                          in->action_complete_hi, i)) continue;
        if (cursor < sc64_z5_action_count(i)) {
            s->action_cursor[i] = (u8)(cursor + 1u);
            sc64_z5_emit(&out, i, cursor);
        } else {
            s->action_cursor[i] = 0;
            if (i == SC64_Z5_TRIGGER_DEFEAT) {
                s->outcome = SC64_Z5_LOST;
                out.events |= SC64_Z5_EVENT_LOST;
            } else if (i == SC64_Z5_TRIGGER_VICTORY) {
                s->outcome = SC64_Z5_WON;
                out.events |= SC64_Z5_EVENT_WON;
            }
        }
    }

    if (s->outcome == SC64_Z5_RUNNING) {
        for (i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i) {
            u8 ready = (u8)sc64_z5_conditions_met(in, i);
            if (!ready || was_active[i] || s->action_cursor[i] ||
                !sc64_z5_action_count(i)) continue;
            if (!sc64_z5_trigger_preserved(i) &&
                sc64_z5_mask_test(s->started_lo, s->started_hi, i)) continue;
            sc64_z5_mask_set(&s->started_lo, &s->started_hi, i);
            s->action_cursor[i] = 1;
            sc64_z5_emit(&out, i, 0);
        }
    }
    return out;
}

#endif
