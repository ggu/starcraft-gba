#ifndef SC64_P7_H
#define SC64_P7_H

#include "../common/gba.h"

/* Bounded scheduler for the authenticated Protoss 7 trigger rows.  The
 * scenario adapter resolves every condition and effect; this is not a CHK
 * interpreter.  Source records contain no PreserveTrigger action or
 * repeat/execution flags, so started records are one-shot. */
enum {
    SC64_P7_GLOBAL_ID = 26,
    SC64_P7_MAP_COLS = 128,
    SC64_P7_MAP_ROWS = 128,
    SC64_P7_TRIGGER_COUNT = 10,
    SC64_P7_ACTION_TOTAL = 44,
    SC64_P7_CONDITION_TOTAL = 10,
    SC64_P7_TRIGGER_DEFEAT_INITIAL = 0,
    SC64_P7_TRIGGER_DEFEAT_HERO_A = 1,
    SC64_P7_TRIGGER_DEFEAT_HERO_B = 2,
    SC64_P7_TRIGGER_DEFEAT_HERO_C = 3,
    SC64_P7_TRIGGER_OBJECTIVE_SETUP = 4,
    SC64_P7_TRIGGER_VICTORY = 5,
    SC64_P7_TRIGGER_AI_FORCE3 = 6,
    SC64_P7_TRIGGER_AI_FORCE2 = 7,
    SC64_P7_TRIGGER_SET_GATE_FORCE2 = 8,
    SC64_P7_TRIGGER_SET_GATE_FORCE3 = 9,
    SC64_P7_SWITCH_VICTORY_GATE = 0,
    SC64_P7_RUNNING = 0,
    SC64_P7_WON = 1,
    SC64_P7_LOST = 2
};

static inline u8 sc64_p7_action_count(u8 trigger)
{
    static const u8 counts[SC64_P7_TRIGGER_COUNT] = {
        1,5,5,5,4,17,3,2,1,1
    };
    return trigger < SC64_P7_TRIGGER_COUNT ? counts[trigger] : 0;
}

static inline u8 sc64_p7_condition_count(u8 trigger)
{
    return trigger < SC64_P7_TRIGGER_COUNT ? 1 : 0;
}

enum {
    SC64_P7_EVENT_NONE = 0,
    SC64_P7_EVENT_ACTION = 1u << 0,
    SC64_P7_EVENT_WON = 1u << 1,
    SC64_P7_EVENT_LOST = 1u << 2
};

typedef struct {
    /* Bit 0 is the result of the trigger's single condition row. */
    u8 condition_true[SC64_P7_TRIGGER_COUNT];
    u16 action_complete;
    u8 paused;
} Sc64P7Input;

typedef struct {
    u16 started;
    u8 outcome;
    /* Zero is idle/done; otherwise current emitted action index plus one. */
    u8 action_cursor[SC64_P7_TRIGGER_COUNT];
} Sc64P7State;

_Static_assert(sizeof(Sc64P7State) == 14, "P7 scheduler state size");

typedef struct { u8 trigger, action_index; } Sc64P7ActionEvent;
typedef struct {
    u8 events, action_count;
    Sc64P7ActionEvent actions[SC64_P7_TRIGGER_COUNT];
} Sc64P7Output;

static inline u16 sc64_p7_trigger_bit(unsigned trigger)
{ return trigger < SC64_P7_TRIGGER_COUNT ? (u16)1u << trigger : 0; }

static inline int sc64_p7_trigger_started(const Sc64P7State *s, unsigned trigger)
{ return s && (s->started & sc64_p7_trigger_bit(trigger)) != 0; }

static inline int sc64_p7_terminal_kind(unsigned trigger)
{
    if (trigger <= SC64_P7_TRIGGER_DEFEAT_HERO_C) return SC64_P7_LOST;
    if (trigger == SC64_P7_TRIGGER_VICTORY) return SC64_P7_WON;
    return SC64_P7_RUNNING;
}

static inline void sc64_p7_init(Sc64P7State *s)
{
    if (!s) return;
    s->started = 0;
    s->outcome = SC64_P7_RUNNING;
    for (unsigned i = 0; i < SC64_P7_TRIGGER_COUNT; ++i) s->action_cursor[i] = 0;
}

static inline int sc64_p7_state_valid(const Sc64P7State *s)
{
    unsigned completed = 0, completed_result = SC64_P7_RUNNING;
    if (!s || s->outcome > SC64_P7_LOST || (s->started & (u16)~0x03ffu)) return 0;
    for (unsigned i = 0; i < SC64_P7_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        if (cursor > sc64_p7_action_count((u8)i) ||
            (cursor && !sc64_p7_trigger_started(s, i))) return 0;
        if (sc64_p7_terminal_kind(i) != SC64_P7_RUNNING &&
            sc64_p7_trigger_started(s, i) && !cursor) {
            ++completed;
            completed_result = (unsigned)sc64_p7_terminal_kind(i);
        }
    }
    if (s->outcome == SC64_P7_RUNNING) return completed == 0;
    return completed == 1 && completed_result == s->outcome;
}

static inline int sc64_p7_input_valid(const Sc64P7Input *in)
{
    if (!in || in->paused > 1 || (in->action_complete & (u16)~0x03ffu)) return 0;
    for (unsigned i = 0; i < SC64_P7_TRIGGER_COUNT; ++i)
        if (in->condition_true[i] > 1) return 0;
    return 1;
}

static inline int sc64_p7_input_state_valid(const Sc64P7State *s,
                                             const Sc64P7Input *in)
{
    if (!sc64_p7_state_valid(s) || !sc64_p7_input_valid(in)) return 0;
    for (unsigned i = 0; i < SC64_P7_TRIGGER_COUNT; ++i)
        if ((in->action_complete & sc64_p7_trigger_bit(i)) &&
            !s->action_cursor[i]) return 0;
    return 1;
}

static inline void sc64_p7_emit(Sc64P7Output *out, u8 trigger, u8 action)
{
    if (out->action_count >= SC64_P7_TRIGGER_COUNT) return;
    out->actions[out->action_count++] = (Sc64P7ActionEvent){trigger,action};
    out->events |= SC64_P7_EVENT_ACTION;
}

/* Advance active records in source order, then start true one-shot records.
 * The first terminal record completed in source order determines outcome. */
static inline Sc64P7Output sc64_p7_step(Sc64P7State *s, const Sc64P7Input *in)
{
    Sc64P7Output out = {0,0,{{0,0}}};
    u8 was_active[SC64_P7_TRIGGER_COUNT];
    if (!sc64_p7_input_state_valid(s,in) || in->paused ||
        s->outcome != SC64_P7_RUNNING) return out;
    for (unsigned i = 0; i < SC64_P7_TRIGGER_COUNT; ++i)
        was_active[i] = s->action_cursor[i] != 0;

    for (u8 i = 0; i < SC64_P7_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        if (s->outcome != SC64_P7_RUNNING) break;
        if (!cursor || !(in->action_complete & sc64_p7_trigger_bit(i))) continue;
        if (cursor < sc64_p7_action_count(i)) {
            s->action_cursor[i] = (u8)(cursor + 1u);
            sc64_p7_emit(&out,i,cursor);
        } else {
            s->action_cursor[i] = 0;
            s->started |= sc64_p7_trigger_bit(i);
            if (sc64_p7_terminal_kind(i) == SC64_P7_WON) {
                s->outcome = SC64_P7_WON;
                out.events |= SC64_P7_EVENT_WON;
            } else if (sc64_p7_terminal_kind(i) == SC64_P7_LOST) {
                s->outcome = SC64_P7_LOST;
                out.events |= SC64_P7_EVENT_LOST;
            }
        }
    }
    if (s->outcome == SC64_P7_RUNNING) {
        for (u8 i = 0; i < SC64_P7_TRIGGER_COUNT; ++i) {
            u16 bit = sc64_p7_trigger_bit(i);
            if (!in->condition_true[i] || was_active[i] ||
                s->action_cursor[i] || (s->started & bit)) continue;
            s->started |= bit;
            s->action_cursor[i] = 1;
            sc64_p7_emit(&out,i,0);
        }
    }
    return out;
}

#endif
