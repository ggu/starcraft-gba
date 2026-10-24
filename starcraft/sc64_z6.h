#ifndef SC64_Z6_H
#define SC64_Z6_H

#include "../common/gba.h"

/* Authored scheduler for Zerg mission 6, The Dark Templar. It consumes one
 * true bit per source condition through an adapter and emits numeric action
 * cursors; it is not a CHK interpreter or a claim of mission completeness. */
enum {
    SC64_Z6_GLOBAL_ID = 17,
    SC64_Z6_MAP_COLS = 128,
    SC64_Z6_MAP_ROWS = 128,
    SC64_Z6_TRIGGER_COUNT = 9,
    SC64_Z6_ACTION_TOTAL = 48,
    SC64_Z6_CONDITION_TOTAL = 13,
    SC64_Z6_TRIGGER_DEFEAT_0 = 0,
    SC64_Z6_TRIGGER_VICTORY = 5,
    SC64_Z6_TRIGGER_DEFEAT_1 = 6,
    SC64_Z6_RUNNING = 0,
    SC64_Z6_WON = 1,
    SC64_Z6_LOST = 2
};

static inline u8 sc64_z6_action_count(u8 trigger)
{
    static const u8 counts[SC64_Z6_TRIGGER_COUNT] = {1,2,1,4,2,13,5,18,2};
    return trigger < SC64_Z6_TRIGGER_COUNT ? counts[trigger] : 0;
}

static inline u8 sc64_z6_condition_count(u8 trigger)
{
    static const u8 counts[SC64_Z6_TRIGGER_COUNT] = {1,1,2,1,3,2,1,1,1};
    return trigger < SC64_Z6_TRIGGER_COUNT ? counts[trigger] : 0;
}

enum {
    SC64_Z6_EVENT_NONE = 0,
    SC64_Z6_EVENT_ACTION = 1u << 0,
    SC64_Z6_EVENT_WON = 1u << 1,
    SC64_Z6_EVENT_LOST = 1u << 2
};

typedef struct {
    u16 condition_true[SC64_Z6_TRIGGER_COUNT];
    u32 action_complete;
    u8 paused;
} Sc64Z6Input;

typedef struct {
    u16 started;
    u8 outcome;
    /* Zero is idle/done. Otherwise this is the emitted action index + 1. */
    u8 action_cursor[SC64_Z6_TRIGGER_COUNT];
} Sc64Z6State;

_Static_assert(sizeof(Sc64Z6State) == 12, "Z6 model state size");

typedef struct {
    u8 trigger;
    u8 action_index;
} Sc64Z6ActionEvent;

typedef struct {
    u8 events;
    u8 action_count;
    Sc64Z6ActionEvent actions[SC64_Z6_TRIGGER_COUNT];
} Sc64Z6Output;

static inline u16 sc64_z6_init(Sc64Z6State *s)
{
    u8 *p;
    u32 i;
    if (!s) return SC64_Z6_EVENT_NONE;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
    return SC64_Z6_EVENT_NONE;
}

static inline int sc64_z6_state_valid(const Sc64Z6State *s)
{
    u8 i, completed_terminal = 0, completed = 0xffu;
    if (!s || s->outcome > SC64_Z6_LOST || (s->started & (u16)~0x1ffu))
        return 0;
    for (i = 0; i < SC64_Z6_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        if (cursor > sc64_z6_action_count(i)) return 0;
        if (cursor && !(s->started & ((u16)1u << i))) return 0;
    }
    for (i = 0; i < SC64_Z6_TRIGGER_COUNT; ++i) {
        if (i != SC64_Z6_TRIGGER_DEFEAT_0 && i != SC64_Z6_TRIGGER_VICTORY &&
            i != SC64_Z6_TRIGGER_DEFEAT_1) continue;
        if ((s->started & ((u16)1u << i)) && !s->action_cursor[i]) {
            ++completed_terminal;
            completed = i;
        }
    }
    if (s->outcome == SC64_Z6_RUNNING && completed_terminal) return 0;
    if (s->outcome == SC64_Z6_WON &&
        (completed_terminal != 1 || completed != SC64_Z6_TRIGGER_VICTORY)) return 0;
    if (s->outcome == SC64_Z6_LOST &&
        (completed_terminal != 1 ||
         (completed != SC64_Z6_TRIGGER_DEFEAT_0 &&
          completed != SC64_Z6_TRIGGER_DEFEAT_1))) return 0;
    return 1;
}

static inline int sc64_z6_input_valid(const Sc64Z6Input *in)
{
    u8 i;
    if (!in || in->paused > 1 || (in->action_complete & (u32)~0x1ffu)) return 0;
    for (i = 0; i < SC64_Z6_TRIGGER_COUNT; ++i) {
        u8 count = sc64_z6_condition_count(i);
        u16 allowed = count ? (u16)(((u16)1u << count) - 1u) : 0;
        if (in->condition_true[i] & (u16)~allowed) return 0;
    }
    return 1;
}

static inline int sc64_z6_input_state_valid(const Sc64Z6State *s,
                                             const Sc64Z6Input *in)
{
    u8 i;
    if (!sc64_z6_state_valid(s) || !sc64_z6_input_valid(in)) return 0;
    for (i = 0; i < SC64_Z6_TRIGGER_COUNT; ++i)
        if ((in->action_complete & ((u32)1u << i)) && !s->action_cursor[i])
            return 0;
    return 1;
}

static inline int sc64_z6_conditions_met(const Sc64Z6Input *in, u8 trigger)
{
    u8 count;
    u16 required;
    if (!in || trigger >= SC64_Z6_TRIGGER_COUNT) return 0;
    count = sc64_z6_condition_count(trigger);
    if (!count) return 0;
    required = (u16)(((u16)1u << count) - 1u);
    return (in->condition_true[trigger] & required) == required;
}

static inline void sc64_z6_emit(Sc64Z6Output *out, u8 trigger, u8 index)
{
    if (out->action_count >= SC64_Z6_TRIGGER_COUNT) return;
    out->actions[out->action_count++] = (Sc64Z6ActionEvent){trigger,index};
    out->events |= SC64_Z6_EVENT_ACTION;
}

/* Active programs advance in source order, followed by newly-ready records.
 * Records 0 and 6 dispatch defeat; record 5 dispatches victory. If terminal
 * programs finish together, the lowest source record wins the tie. */
static inline Sc64Z6Output sc64_z6_step(Sc64Z6State *s, const Sc64Z6Input *in)
{
    Sc64Z6Output out = {0,0,{{0,0}}};
    u8 was_active[SC64_Z6_TRIGGER_COUNT];
    u8 i;
    if (!sc64_z6_input_state_valid(s, in) || in->paused ||
        s->outcome != SC64_Z6_RUNNING) return out;
    for (i = 0; i < SC64_Z6_TRIGGER_COUNT; ++i)
        was_active[i] = s->action_cursor[i] != 0;

    for (i = 0; i < SC64_Z6_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        if (s->outcome != SC64_Z6_RUNNING) break;
        if (!cursor || !(in->action_complete & ((u32)1u << i))) continue;
        if (cursor < sc64_z6_action_count(i)) {
            s->action_cursor[i] = (u8)(cursor + 1u);
            sc64_z6_emit(&out, i, cursor);
        } else {
            s->action_cursor[i] = 0;
            if (i == SC64_Z6_TRIGGER_VICTORY) {
                s->outcome = SC64_Z6_WON;
                out.events |= SC64_Z6_EVENT_WON;
            } else if (i == SC64_Z6_TRIGGER_DEFEAT_0 ||
                       i == SC64_Z6_TRIGGER_DEFEAT_1) {
                s->outcome = SC64_Z6_LOST;
                out.events |= SC64_Z6_EVENT_LOST;
            }
        }
    }

    if (s->outcome == SC64_Z6_RUNNING) {
        for (i = 0; i < SC64_Z6_TRIGGER_COUNT; ++i) {
            if (!sc64_z6_conditions_met(in, i) || was_active[i] ||
                s->action_cursor[i] || (s->started & ((u16)1u << i))) continue;
            s->started |= (u16)1u << i;
            s->action_cursor[i] = 1;
            sc64_z6_emit(&out, i, 0);
        }
    }
    return out;
}

#endif
