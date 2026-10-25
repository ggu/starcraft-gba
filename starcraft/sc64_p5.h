#ifndef SC64_P5_H
#define SC64_P5_H

#include "../common/gba.h"

/* Authored bounded scheduler for Protoss mission 5 (stable source ID 24).
 * It is not a CHK interpreter. The adapter resolves each source row's owner,
 * selector, location, switch, timer, unit counts and action effect. Record 0
 * loses for Current Player (13) having at most zero raw-229 units; record 1
 * loses when All Players (17) has at most zero raw-79 units. Record 4 requires
 * Current Player to Bring at least one raw-79 and at least two raw-65 units to
 * source location 2, then ends in Victory. */
enum {
    SC64_P5_GLOBAL_ID = 24,
    SC64_P5_MAP_COLS = 128,
    SC64_P5_MAP_ROWS = 128,
    SC64_P5_TRIGGER_COUNT = 11,
    SC64_P5_ACTIVE_TRIGGER_COUNT = 11,
    SC64_P5_ACTION_TOTAL = 37,
    SC64_P5_CONDITION_TOTAL = 12,

    SC64_P5_TRIGGER_DEFEAT_CURRENT_PLAYER = 0,
    SC64_P5_TRIGGER_DEFEAT_HERO_ABSENT = 1,
    SC64_P5_TRIGGER_VICTORY = 4,
    SC64_P5_TRIGGER_INITIAL_OBJECTIVES = 2,
    SC64_P5_TRIGGER_SWITCHED_FORCE_SETUP = 3,
    SC64_P5_TRIGGER_CREATE_HERO = 4,
    SC64_P5_TRIGGER_FORCE2_AI = 8,
    SC64_P5_TRIGGER_FORCE3_AI = 9,
    SC64_P5_TRIGGER_FORCE4_AI = 10,
    SC64_P5_ACTIONS_CURRENT_PLAYER_DEFEAT = 1,
    SC64_P5_ACTIONS_HERO_ABSENT_DEFEAT = 5,
    SC64_P5_ACTIONS_VICTORY = 3,

    SC64_P5_SELECTOR_CURRENT_PLAYER = 13,
    SC64_P5_SELECTOR_ALL_PLAYERS = 17,
    SC64_P5_SELECTOR_FORCE1 = 18,
    SC64_P5_SELECTOR_FORCE2 = 19,
    SC64_P5_SELECTOR_FORCE3 = 20,
    SC64_P5_SELECTOR_FORCE4 = 21,
    SC64_P5_RAW_ANY_UNIT = 229,
    SC64_P5_RAW_HERO = 79,
    SC64_P5_RAW_ZEALOT = 65,
    SC64_P5_LOCATION_VICTORY = 2,

    SC64_P5_RUNNING = 0,
    SC64_P5_WON = 1,
    SC64_P5_LOST = 2
};

static inline u8 sc64_p5_action_count(u8 trigger)
{
    static const u8 counts[SC64_P5_TRIGGER_COUNT] = {
        1, 5, 2, 18, 3, 1, 1, 1, 1, 2, 2
    };
    return trigger < SC64_P5_TRIGGER_COUNT ? counts[trigger] : 0;
}

static inline u8 sc64_p5_condition_count(u8 trigger)
{
    static const u8 counts[SC64_P5_TRIGGER_COUNT] = {
        1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1
    };
    return trigger < SC64_P5_TRIGGER_COUNT ? counts[trigger] : 0;
}

enum {
    SC64_P5_EVENT_NONE = 0,
    SC64_P5_EVENT_ACTION = 1u << 0,
    SC64_P5_EVENT_WON = 1u << 1,
    SC64_P5_EVENT_LOST = 1u << 2
};

typedef struct {
    /* One bit per condition row; a trigger is eligible only when all rows
     * listed for that record are true. */
    u8 condition_true[SC64_P5_TRIGGER_COUNT];
    u16 action_complete;       /* Bit per active trigger's current action. */
    u8 paused;
} Sc64P5Input;

typedef struct {
    u16 started;               /* One-shot source records already started. */
    u8 outcome;
    u8 action_cursor[SC64_P5_TRIGGER_COUNT]; /* Current action index + 1; 0 idle/done. */
} Sc64P5State;

_Static_assert(sizeof(Sc64P5State) == 14, "P5 model state size");

typedef struct {
    u8 trigger;
    u8 action_index;
} Sc64P5ActionEvent;

typedef struct {
    u8 events;
    u8 action_count;
    Sc64P5ActionEvent actions[SC64_P5_TRIGGER_COUNT];
} Sc64P5Output;

static inline int sc64_p5_trigger_mask_test(u16 mask, u8 trigger)
{
    return trigger < SC64_P5_TRIGGER_COUNT ? (int)((mask >> trigger) & 1u) : 0;
}

static inline u16 sc64_p5_init(Sc64P5State *s)
{
    u8 *p;
    u32 i;
    if (!s) return SC64_P5_EVENT_NONE;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
    return SC64_P5_EVENT_NONE;
}

static inline int sc64_p5_state_valid(const Sc64P5State *s)
{
    u8 i;
    u8 completed_terminal = 0;
    u8 completed_trigger = 0xffu;
    if (!s || s->outcome > SC64_P5_LOST || (s->started & (u16)~0x07ffu)) return 0;
    for (i = 0; i < SC64_P5_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        int started = sc64_p5_trigger_mask_test(s->started, i);
        if (cursor > sc64_p5_action_count(i)) return 0;
        if (cursor && !started) return 0;
        if (started && !cursor &&
            (i == SC64_P5_TRIGGER_DEFEAT_CURRENT_PLAYER ||
             i == SC64_P5_TRIGGER_DEFEAT_HERO_ABSENT || i == SC64_P5_TRIGGER_VICTORY)) {
            ++completed_terminal;
            completed_trigger = i;
        }
    }
    if (s->outcome == SC64_P5_RUNNING && completed_terminal) return 0;
    if (s->outcome == SC64_P5_WON &&
        (completed_terminal != 1 || completed_trigger != SC64_P5_TRIGGER_VICTORY)) return 0;
    if (s->outcome == SC64_P5_LOST &&
        (completed_terminal != 1 ||
         (completed_trigger != SC64_P5_TRIGGER_DEFEAT_CURRENT_PLAYER &&
          completed_trigger != SC64_P5_TRIGGER_DEFEAT_HERO_ABSENT))) return 0;
    return 1;
}

static inline int sc64_p5_input_valid(const Sc64P5Input *in)
{
    u8 i;
    if (!in || in->paused > 1 || (in->action_complete & (u16)~0x07ffu)) return 0;
    for (i = 0; i < SC64_P5_TRIGGER_COUNT; ++i) {
        u8 count = sc64_p5_condition_count(i);
        u8 allowed = count ? (u8)(((u8)1u << count) - 1u) : 0;
        if (in->condition_true[i] & (u8)~allowed) return 0;
    }
    return 1;
}

static inline int sc64_p5_input_state_valid(const Sc64P5State *s,
                                            const Sc64P5Input *in)
{
    u8 i;
    if (!sc64_p5_state_valid(s) || !sc64_p5_input_valid(in)) return 0;
    for (i = 0; i < SC64_P5_TRIGGER_COUNT; ++i)
        if (sc64_p5_trigger_mask_test(in->action_complete, i) &&
            !s->action_cursor[i]) return 0;
    return 1;
}

static inline int sc64_p5_conditions_met(const Sc64P5Input *in, u8 trigger)
{
    u8 count = sc64_p5_condition_count(trigger);
    u8 required;
    if (!in || trigger >= SC64_P5_TRIGGER_COUNT || !count) return 0;
    required = (u8)(((u8)1u << count) - 1u);
    return (in->condition_true[trigger] & required) == required;
}

static inline void sc64_p5_emit(Sc64P5Output *out, u8 trigger, u8 index)
{
    if (out->action_count >= SC64_P5_TRIGGER_COUNT) return;
    out->actions[out->action_count].trigger = trigger;
    out->actions[out->action_count].action_index = index;
    ++out->action_count;
    out->events |= SC64_P5_EVENT_ACTION;
}

/* Run independent source action programs in record order. The first completed
 * terminal program wins; simultaneous completions therefore follow source
 * order (loss records 0/1 before victory record 4). */
static inline Sc64P5Output sc64_p5_step(Sc64P5State *s, const Sc64P5Input *in)
{
    Sc64P5Output out = {0, 0, {{0, 0}}};
    u8 was_active[SC64_P5_TRIGGER_COUNT];
    u8 i;
    if (!sc64_p5_input_state_valid(s, in) || in->paused ||
        s->outcome != SC64_P5_RUNNING) return out;
    for (i = 0; i < SC64_P5_TRIGGER_COUNT; ++i)
        was_active[i] = s->action_cursor[i] != 0;
    for (i = 0; i < SC64_P5_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        if (s->outcome != SC64_P5_RUNNING) break;
        if (!cursor || !sc64_p5_trigger_mask_test(in->action_complete, i)) continue;
        if (cursor < sc64_p5_action_count(i)) {
            s->action_cursor[i] = (u8)(cursor + 1u);
            sc64_p5_emit(&out, i, cursor);
        } else {
            s->action_cursor[i] = 0;
            if (i == SC64_P5_TRIGGER_VICTORY) {
                s->outcome = SC64_P5_WON;
                out.events |= SC64_P5_EVENT_WON;
            } else if (i == SC64_P5_TRIGGER_DEFEAT_CURRENT_PLAYER ||
                       i == SC64_P5_TRIGGER_DEFEAT_HERO_ABSENT) {
                s->outcome = SC64_P5_LOST;
                out.events |= SC64_P5_EVENT_LOST;
            }
        }
    }
    if (s->outcome == SC64_P5_RUNNING) {
        for (i = 0; i < SC64_P5_TRIGGER_COUNT; ++i) {
            if (!sc64_p5_conditions_met(in, i) || was_active[i] ||
                s->action_cursor[i] || sc64_p5_trigger_mask_test(s->started, i)) continue;
            s->started |= (u16)1u << i;
            s->action_cursor[i] = 1;
            sc64_p5_emit(&out, i, 0);
        }
    }
    return out;
}

#endif
