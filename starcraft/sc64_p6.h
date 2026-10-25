#ifndef SC64_P6_H
#define SC64_P6_H

#include "../common/gba.h"

/* Compact one-shot scheduler for the authenticated Protoss 6 source table.
 * Numeric predicates and effects stay in the adapter callbacks; this is not
 * a general trigger interpreter. */
enum {
    SC64_P6_GLOBAL_ID = 25,
    SC64_P6_MAP_COLS = 128,
    SC64_P6_MAP_ROWS = 128,
    SC64_P6_TRIGGER_COUNT = 26,
    SC64_P6_ACTION_TOTAL = 99,
    SC64_P6_CONDITION_TOTAL = 29,
    SC64_P6_TRIGGER_DEFEAT_CURRENT_PLAYER = 1,
    SC64_P6_TRIGGER_DEFEAT_HERO_ABSENT = 8,
    SC64_P6_TRIGGER_VICTORY = 15,
    SC64_P6_RUNNING = 0,
    SC64_P6_WON = 1,
    SC64_P6_LOST = 2
};

static inline u8 sc64_p6_action_count(u8 trigger)
{
    static const u8 counts[SC64_P6_TRIGGER_COUNT] = {
        1,1,2,2,2,2,2,2,5,2,2,8,8,1,7,12,5,2,3,3,7,12,3,1,3,1
    };
    return trigger < SC64_P6_TRIGGER_COUNT ? counts[trigger] : 0;
}

static inline u8 sc64_p6_condition_count(u8 trigger)
{
    static const u8 counts[SC64_P6_TRIGGER_COUNT] = {
        1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2
    };
    return trigger < SC64_P6_TRIGGER_COUNT ? counts[trigger] : 0;
}

enum {
    SC64_P6_EVENT_NONE = 0,
    SC64_P6_EVENT_ACTION = 1u << 0,
    SC64_P6_EVENT_WON = 1u << 1,
    SC64_P6_EVENT_LOST = 1u << 2
};

typedef struct {
    u16 condition_true[SC64_P6_TRIGGER_COUNT];
    u32 action_complete;
    u8 paused;
} Sc64P6Input;

typedef struct {
    u32 started;
    u8 outcome;
    /* Zero is idle/done; otherwise the dispatched action index plus one. */
    u8 action_cursor[SC64_P6_TRIGGER_COUNT];
} Sc64P6State;

_Static_assert(sizeof(Sc64P6State) <= 32, "P6 model state size");

typedef struct { u8 trigger, action_index; } Sc64P6ActionEvent;
typedef struct {
    u8 events, action_count;
    Sc64P6ActionEvent actions[SC64_P6_TRIGGER_COUNT];
} Sc64P6Output;

static inline u32 sc64_p6_trigger_bit(unsigned trigger)
{ return trigger < SC64_P6_TRIGGER_COUNT ? (u32)1u << trigger : 0; }

static inline int sc64_p6_trigger_started(const Sc64P6State *s, unsigned trigger)
{ return s && (s->started & sc64_p6_trigger_bit(trigger)) != 0; }

static inline void sc64_p6_emit(Sc64P6Output *out, u8 trigger, u8 index)
{
    if (out->action_count >= SC64_P6_TRIGGER_COUNT) return;
    out->actions[out->action_count++] = (Sc64P6ActionEvent){trigger,index};
    out->events |= SC64_P6_EVENT_ACTION;
}

static inline void sc64_p6_init(Sc64P6State *s)
{
    if (s) {
        s->started = 0;
        s->outcome = SC64_P6_RUNNING;
        for (unsigned i = 0; i < SC64_P6_TRIGGER_COUNT; ++i)
            s->action_cursor[i] = 0;
    }
}

static inline int sc64_p6_state_valid(const Sc64P6State *s)
{
    unsigned terminal_count = 0, terminal = 0xffu;
    if (!s || s->outcome > SC64_P6_LOST ||
        (s->started & ~(((u32)1u << SC64_P6_TRIGGER_COUNT) - 1u))) return 0;
    for (unsigned i = 0; i < SC64_P6_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        if (cursor > sc64_p6_action_count((u8)i) ||
            (cursor && !sc64_p6_trigger_started(s, i))) return 0;
        if ((i == SC64_P6_TRIGGER_DEFEAT_CURRENT_PLAYER ||
             i == SC64_P6_TRIGGER_DEFEAT_HERO_ABSENT ||
             i == SC64_P6_TRIGGER_VICTORY) &&
            sc64_p6_trigger_started(s, i) && !cursor) {
            ++terminal_count;
            terminal = i;
        }
    }
    if (s->outcome == SC64_P6_RUNNING && terminal_count) return 0;
    if (s->outcome == SC64_P6_WON &&
        (terminal_count != 1 || terminal != SC64_P6_TRIGGER_VICTORY)) return 0;
    if (s->outcome == SC64_P6_LOST &&
        (terminal_count != 1 ||
         (terminal != SC64_P6_TRIGGER_DEFEAT_CURRENT_PLAYER &&
          terminal != SC64_P6_TRIGGER_DEFEAT_HERO_ABSENT))) return 0;
    return 1;
}

static inline int sc64_p6_input_valid(const Sc64P6Input *in)
{
    if (!in || in->paused > 1 ||
        (in->action_complete & ~(((u32)1u << SC64_P6_TRIGGER_COUNT) - 1u))) return 0;
    for (unsigned i = 0; i < SC64_P6_TRIGGER_COUNT; ++i) {
        u8 count = sc64_p6_condition_count((u8)i);
        u16 allowed = count ? (u16)(((u16)1u << count) - 1u) : 0;
        if (in->condition_true[i] & (u16)~allowed) return 0;
    }
    return 1;
}

static inline int sc64_p6_input_state_valid(const Sc64P6State *s,
                                             const Sc64P6Input *in)
{
    if (!sc64_p6_state_valid(s) || !sc64_p6_input_valid(in)) return 0;
    for (unsigned i = 0; i < SC64_P6_TRIGGER_COUNT; ++i)
        if ((in->action_complete & sc64_p6_trigger_bit(i)) &&
            !s->action_cursor[i]) return 0;
    return 1;
}

static inline int sc64_p6_conditions_met(const Sc64P6Input *in, u8 trigger)
{
    u8 n;
    if (!in || trigger >= SC64_P6_TRIGGER_COUNT) return 0;
    n = sc64_p6_condition_count(trigger);
    return n && in->condition_true[trigger] == (u16)(((u16)1u << n) - 1u);
}

/* Progress already-running records in source order, then start newly true
 * one-shot records. A lower-numbered terminal record wins a same-tick tie. */
static inline Sc64P6Output sc64_p6_step(Sc64P6State *s, const Sc64P6Input *in)
{
    Sc64P6Output out = {0,0,{{0,0}}};
    u8 was_active[SC64_P6_TRIGGER_COUNT];
    if (!sc64_p6_input_state_valid(s, in) || in->paused ||
        s->outcome != SC64_P6_RUNNING) return out;
    for (unsigned i = 0; i < SC64_P6_TRIGGER_COUNT; ++i)
        was_active[i] = s->action_cursor[i] != 0;
    for (u8 i = 0; i < SC64_P6_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        if (s->outcome != SC64_P6_RUNNING) break;
        if (!cursor || !(in->action_complete & sc64_p6_trigger_bit(i))) continue;
        if (cursor < sc64_p6_action_count(i)) {
            s->action_cursor[i] = (u8)(cursor + 1u);
            sc64_p6_emit(&out, i, cursor);
        } else {
            s->action_cursor[i] = 0;
            if (i == SC64_P6_TRIGGER_VICTORY) {
                s->outcome = SC64_P6_WON;
                out.events |= SC64_P6_EVENT_WON;
            } else if (i == SC64_P6_TRIGGER_DEFEAT_CURRENT_PLAYER ||
                       i == SC64_P6_TRIGGER_DEFEAT_HERO_ABSENT) {
                s->outcome = SC64_P6_LOST;
                out.events |= SC64_P6_EVENT_LOST;
            }
        }
    }
    if (s->outcome == SC64_P6_RUNNING) {
        for (u8 i = 0; i < SC64_P6_TRIGGER_COUNT; ++i) {
            if (!sc64_p6_conditions_met(in, i) || was_active[i] ||
                s->action_cursor[i] || sc64_p6_trigger_started(s, i)) continue;
            s->started |= sc64_p6_trigger_bit(i);
            s->action_cursor[i] = 1;
            sc64_p6_emit(&out, i, 0);
        }
    }
    return out;
}

#endif
