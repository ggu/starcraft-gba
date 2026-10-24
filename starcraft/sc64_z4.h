#ifndef SC64_Z4_H
#define SC64_Z4_H

#include "../common/gba.h"

/* Bounded source-driven rule model for Zerg mission 4 / Agent of the Swarm.
 * The trigger/action indices and selectors are taken from the authenticated
 * private source audit. This is a small state machine, not a CHK interpreter.
 * Adapters resolve engine-side counts, switch comparison semantics, action
 * presentation and raw Wait/CountdownTimer conversion. */
enum {
    SC64_Z4_TRIGGER_KERRIGAN_DEFEAT = 0,
    SC64_Z4_TRIGGER_STARTUP = 1,
    SC64_Z4_TRIGGER_HATCH_WARNING = 2,
    SC64_Z4_TRIGGER_HATCH = 3,
    SC64_Z4_TRIGGER_COMMAND_CENTER_WIN = 4,
    SC64_Z4_TRIGGER_FORCE_START = 5,
    SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT = 6,
    SC64_Z4_TRIGGER_COUNT = 7
};

enum {
    SC64_Z4_ACTIONS_KERRIGAN_DEFEAT = 5,
    SC64_Z4_ACTIONS_STARTUP = 14,
    SC64_Z4_ACTIONS_HATCH_WARNING = 1,
    SC64_Z4_ACTIONS_HATCH = 24,
    SC64_Z4_ACTIONS_COMMAND_CENTER_WIN = 14,
    SC64_Z4_ACTIONS_FORCE_START = 3,
    SC64_Z4_ACTIONS_CHRYSALIS_DEFEAT = 5,
    SC64_Z4_HATCH_WARNING_COUNTDOWN = 30,
    SC64_Z4_MISSION_COUNTDOWN = 600,
    SC64_Z4_ELAPSED_DEFEAT_GATE = 1
};

enum {
    SC64_Z4_UNIT_KERRIGAN = 51,
    SC64_Z4_UNIT_CHRYSALIS = 150,
    SC64_Z4_UNIT_RAYNOR_COMMAND_CENTER = 106,
    SC64_Z4_SELECTOR_CURRENT_PLAYER = 13,
    SC64_Z4_SELECTOR_FORCE_3 = 20,
    SC64_Z4_SELECTOR_FORCE_2 = 19,
    SC64_Z4_SELECTOR_ANY_UNIT = 229,
    SC64_Z4_SWITCH_CONDITION_2 = 2,
    SC64_Z4_SWITCH_CONDITION_3 = 3,
    SC64_Z4_ACTION_SWITCH_MODIFIER_4 = 4,
    SC64_Z4_ACTION_SWITCH_MODIFIER_5 = 5
};

enum {
    SC64_Z4_RUNNING = 0,
    SC64_Z4_WON = 1,
    SC64_Z4_LOST = 2
};

enum {
    SC64_Z4_EVENT_NONE = 0,
    SC64_Z4_EVENT_TRIGGER_KERRIGAN_DEFEAT = 1u << 0,
    SC64_Z4_EVENT_TRIGGER_STARTUP = 1u << 1,
    SC64_Z4_EVENT_TRIGGER_HATCH_WARNING = 1u << 2,
    SC64_Z4_EVENT_TRIGGER_HATCH = 1u << 3,
    SC64_Z4_EVENT_TRIGGER_COMMAND_CENTER_WIN = 1u << 4,
    SC64_Z4_EVENT_TRIGGER_FORCE_START = 1u << 5,
    SC64_Z4_EVENT_TRIGGER_CHRYSALIS_DEFEAT = 1u << 6,
    SC64_Z4_EVENT_WON = 1u << 7,
    SC64_Z4_EVENT_LOST = 1u << 8
};

#define SC64_Z4_ALL_TRIGGERS ((u16)((1u << SC64_Z4_TRIGGER_COUNT) - 1u))

typedef struct {
    u16 fired;                 /* One bit for each source trigger record. */
    u8 outcome;
    u8 action_cursor[SC64_Z4_TRIGGER_COUNT]; /* Current action index + 1; 0=idle/done. */
} Sc64Z4State;

/* Adapter-owned campaign state. Timers use the game's fixed 30 Hz update
 * clock; source Wait/Countdown operands are milliseconds/seconds and are
 * converted by the adapter. `switch_bits` retains source switch #0. */
typedef struct {
    Sc64Z4State model;
    u16 countdown_ticks;
    u16 action_ticks[SC64_Z4_TRIGGER_COUNT];
    u32 faults;
    u16 message_ticks;
    u8 switch_bits;
    u8 countdown_active;
    u8 ai_script_mask;
    /* Script-authored hatch effects: bit 0 means the source KillUnit removed
     * the live Chrysalis, bit 1 means the source Kerrigan creation succeeded.
     * Persisted so a save in the transition window retains the exact policy. */
    u8 hatch_effects;
} Sc64Z4RuntimeState;

_Static_assert(sizeof(Sc64Z4State) == 10, "Z4 persisted model state size");

typedef struct {
    u16 kerrigan_count;        /* Current Player / unit 51, raw Command <= 0. */
    u16 chrysalis_count;       /* Current Player / unit 150, raw Command <= 0. */
    u16 command_center_count;  /* Force 3 / unit 106, raw Command <= 0. */
    u16 force2_any_units;      /* Force 2 / raw AnyUnit 229, raw Command >= 1. */
    u16 countdown_remaining;   /* Raw CHK countdown value; conversion is adapter policy. */
    u8 elapsed_seconds;        /* Source ElapsedTime domain, not simulation frames. */
    u8 countdown_active;       /* Set only after startup action index 2 is dispatched. */
    u8 switch_condition_2;     /* Evaluated raw Switch condition comparison 2. */
    u8 switch_condition_3;     /* Evaluated raw Switch condition comparison 3. */
    /* Authored GBA policy around the source's intentional actor handoff. Only
     * suppress Chrysalis loss after the scripted kill actually removed it and
     * until the scripted Kerrigan create succeeds. Arm Kerrigan loss only
     * after that successful creation. This is not an N64 scheduler claim. */
    u8 suppress_chrysalis_loss;
    u8 kerrigan_survival_armed;
    u8 complete_actions;       /* Bit per trigger: adapter finished its current action. */
    u8 paused;
} Sc64Z4Input;

typedef struct {
    u8 trigger;
    u8 action_index;           /* Zero-based source action index. */
} Sc64Z4ActionEvent;

typedef struct {
    u16 events;
    u8 action_count;
    Sc64Z4ActionEvent actions[SC64_Z4_TRIGGER_COUNT];
} Sc64Z4Output;

static inline u8 sc64_z4_action_count(u8 trigger)
{
    static const u8 counts[SC64_Z4_TRIGGER_COUNT] = {
        SC64_Z4_ACTIONS_KERRIGAN_DEFEAT,
        SC64_Z4_ACTIONS_STARTUP,
        SC64_Z4_ACTIONS_HATCH_WARNING,
        SC64_Z4_ACTIONS_HATCH,
        SC64_Z4_ACTIONS_COMMAND_CENTER_WIN,
        SC64_Z4_ACTIONS_FORCE_START,
        SC64_Z4_ACTIONS_CHRYSALIS_DEFEAT
    };
    return trigger < SC64_Z4_TRIGGER_COUNT ? counts[trigger] : 0;
}

static inline u16 sc64_z4_trigger_event(u8 trigger)
{
    return trigger < SC64_Z4_TRIGGER_COUNT ? (u16)(1u << trigger) : 0;
}

static inline void sc64_z4_emit_action(Sc64Z4Output *out, u8 trigger, u8 index)
{
    if (!out || out->action_count >= SC64_Z4_TRIGGER_COUNT) return;
    out->actions[out->action_count].trigger = trigger;
    out->actions[out->action_count].action_index = index;
    ++out->action_count;
}

static inline u16 sc64_z4_init(Sc64Z4State *s)
{
    u8 *p;
    u32 i;
    if (!s) return SC64_Z4_EVENT_NONE;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
    return SC64_Z4_EVENT_NONE;
}

static inline int sc64_z4_state_valid(const Sc64Z4State *s)
{
    u8 i;
    if (!s || (s->fired & (u16)~SC64_Z4_ALL_TRIGGERS) || s->outcome > SC64_Z4_LOST)
        return 0;
    for (i = 0; i < SC64_Z4_TRIGGER_COUNT; ++i) {
        u8 cursor = s->action_cursor[i];
        if (cursor > sc64_z4_action_count(i)) return 0;
        if (cursor && !(s->fired & (u16)(1u << i))) return 0;
    }
    if (s->outcome == SC64_Z4_RUNNING) {
        if ((s->fired & (u16)(1u << SC64_Z4_TRIGGER_KERRIGAN_DEFEAT)) &&
            !s->action_cursor[SC64_Z4_TRIGGER_KERRIGAN_DEFEAT]) return 0;
        if ((s->fired & (u16)(1u << SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT)) &&
            !s->action_cursor[SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT]) return 0;
        if ((s->fired & (u16)(1u << SC64_Z4_TRIGGER_COMMAND_CENTER_WIN)) &&
            !s->action_cursor[SC64_Z4_TRIGGER_COMMAND_CENTER_WIN]) return 0;
    } else if (s->outcome == SC64_Z4_WON) {
        if (!(s->fired & (u16)(1u << SC64_Z4_TRIGGER_COMMAND_CENTER_WIN)) ||
            s->action_cursor[SC64_Z4_TRIGGER_COMMAND_CENTER_WIN]) return 0;
    } else {
        if (!((s->fired & (u16)(1u << SC64_Z4_TRIGGER_KERRIGAN_DEFEAT)) &&
              !s->action_cursor[SC64_Z4_TRIGGER_KERRIGAN_DEFEAT]) &&
            !((s->fired & (u16)(1u << SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT)) &&
              !s->action_cursor[SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT])) return 0;
    }
    return 1;
}

/* Start each source trigger at most once, then advance its audited action
 * sequence only when its adapter signals completion. Independent source
 * records retain independent cursors. Trigger conditions 0/6 use the source
 * raw switch comparisons passed by the adapter; this avoids guessing CHK
 * switch interpretation. The adapter must also choose its documented timing
 * policy for Wait, WAV, Transmission and CountdownTimer units. */
static inline Sc64Z4Output sc64_z4_step(Sc64Z4State *s, const Sc64Z4Input *in)
{
    Sc64Z4Output out = {0, 0, {{0, 0}}};
    u8 was_active[SC64_Z4_TRIGGER_COUNT];
    u8 terminal_loss = 0;
    u8 terminal_win = 0;
    u8 i;
    if (!sc64_z4_state_valid(s) || !in || in->paused || s->outcome != SC64_Z4_RUNNING)
        return out;
    for (i = 0; i < SC64_Z4_TRIGGER_COUNT; ++i)
        was_active[i] = s->action_cursor[i] != 0;

    if (!(s->fired & (u16)(1u << SC64_Z4_TRIGGER_STARTUP))) {
        s->fired |= (u16)(1u << SC64_Z4_TRIGGER_STARTUP);
        s->action_cursor[SC64_Z4_TRIGGER_STARTUP] = 1;
        out.events |= SC64_Z4_EVENT_TRIGGER_STARTUP;
        sc64_z4_emit_action(&out, SC64_Z4_TRIGGER_STARTUP, 0);
    }
    if (in->kerrigan_survival_armed &&
        !(s->fired & (u16)(1u << SC64_Z4_TRIGGER_KERRIGAN_DEFEAT)) &&
        in->elapsed_seconds >= SC64_Z4_ELAPSED_DEFEAT_GATE &&
        in->kerrigan_count == 0 && in->switch_condition_3) {
        s->fired |= (u16)(1u << SC64_Z4_TRIGGER_KERRIGAN_DEFEAT);
        s->action_cursor[SC64_Z4_TRIGGER_KERRIGAN_DEFEAT] = 1;
        out.events |= SC64_Z4_EVENT_TRIGGER_KERRIGAN_DEFEAT;
        sc64_z4_emit_action(&out, SC64_Z4_TRIGGER_KERRIGAN_DEFEAT, 0);
    }
    if (!(s->fired & (u16)(1u << SC64_Z4_TRIGGER_HATCH_WARNING)) && in->countdown_active &&
        in->countdown_remaining <= SC64_Z4_HATCH_WARNING_COUNTDOWN) {
        s->fired |= (u16)(1u << SC64_Z4_TRIGGER_HATCH_WARNING);
        s->action_cursor[SC64_Z4_TRIGGER_HATCH_WARNING] = 1;
        out.events |= SC64_Z4_EVENT_TRIGGER_HATCH_WARNING;
        sc64_z4_emit_action(&out, SC64_Z4_TRIGGER_HATCH_WARNING, 0);
    }
    if (!(s->fired & (u16)(1u << SC64_Z4_TRIGGER_HATCH)) && in->countdown_active &&
        in->countdown_remaining == 0) {
        s->fired |= (u16)(1u << SC64_Z4_TRIGGER_HATCH);
        s->action_cursor[SC64_Z4_TRIGGER_HATCH] = 1;
        out.events |= SC64_Z4_EVENT_TRIGGER_HATCH;
        sc64_z4_emit_action(&out, SC64_Z4_TRIGGER_HATCH, 0);
    }
    if (!(s->fired & (u16)(1u << SC64_Z4_TRIGGER_COMMAND_CENTER_WIN)) &&
        in->command_center_count == 0) {
        s->fired |= (u16)(1u << SC64_Z4_TRIGGER_COMMAND_CENTER_WIN);
        s->action_cursor[SC64_Z4_TRIGGER_COMMAND_CENTER_WIN] = 1;
        out.events |= SC64_Z4_EVENT_TRIGGER_COMMAND_CENTER_WIN;
        sc64_z4_emit_action(&out, SC64_Z4_TRIGGER_COMMAND_CENTER_WIN, 0);
    }
    if (!(s->fired & (u16)(1u << SC64_Z4_TRIGGER_FORCE_START)) && in->force2_any_units >= 1) {
        s->fired |= (u16)(1u << SC64_Z4_TRIGGER_FORCE_START);
        s->action_cursor[SC64_Z4_TRIGGER_FORCE_START] = 1;
        out.events |= SC64_Z4_EVENT_TRIGGER_FORCE_START;
        sc64_z4_emit_action(&out, SC64_Z4_TRIGGER_FORCE_START, 0);
    }
    if (!in->suppress_chrysalis_loss &&
        !(s->fired & (u16)(1u << SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT)) &&
        in->chrysalis_count == 0 && in->switch_condition_2) {
        s->fired |= (u16)(1u << SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT);
        s->action_cursor[SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT] = 1;
        out.events |= SC64_Z4_EVENT_TRIGGER_CHRYSALIS_DEFEAT;
        sc64_z4_emit_action(&out, SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT, 0);
    }

    /* Complete only actions that were already active at call entry. One
     * completion advances at most one action in that source trigger. */
    for (i = 0; i < SC64_Z4_TRIGGER_COUNT; ++i) {
        u8 cursor;
        if (!was_active[i] || !(in->complete_actions & (u8)(1u << i))) continue;
        cursor = s->action_cursor[i];
        if (!cursor) continue;
        if (cursor >= sc64_z4_action_count(i)) {
            s->action_cursor[i] = 0;
            if (i == SC64_Z4_TRIGGER_COMMAND_CENTER_WIN) {
                terminal_win = 1;
            } else if (i == SC64_Z4_TRIGGER_KERRIGAN_DEFEAT ||
                       i == SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT) {
                terminal_loss = 1;
            }
        } else {
            ++cursor;
            s->action_cursor[i] = cursor;
            /* Startup action index 2 is SetCountdownTimer; the adapter
             * observes that emitted action and starts its timer. */
            sc64_z4_emit_action(&out, i, (u8)(cursor - 1u));
        }
    }
    /* The independent trigger records establish no same-update ordering.
     * Deterministic adapter policy gives either survival defeat priority. */
    if (terminal_loss) {
        s->outcome = SC64_Z4_LOST;
        out.events |= SC64_Z4_EVENT_LOST;
    } else if (terminal_win) {
        s->outcome = SC64_Z4_WON;
        out.events |= SC64_Z4_EVENT_WON;
    }
    return out;
}

#endif /* SC64_Z4_H */
