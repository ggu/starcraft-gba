#ifndef SC64_T3_H
#define SC64_T3_H

#include "../common/gba.h"

/* Bounded mission-script model for StarCraft 64 Terran 3, 008/00C. Trigger
 * records 0-9 were parsed from the verified local ROM with the pinned
 * references/sc64-maps extractor @ 09b57f787a247107d1de82b8c5eab2734667ea1d.
 * This captures the authored countdown, Force 1/2 and switch gates as events;
 * the engine owns map units, AI behavior, and presentation. */
enum {
    SC64_T3_TICKS_PER_SECOND = 30,
    SC64_T3_DURATION_SECONDS = 1800,
    SC64_T3_DURATION_TICKS = SC64_T3_TICKS_PER_SECOND * SC64_T3_DURATION_SECONDS,
    SC64_T3_FORCE_1 = 18,
    SC64_T3_FORCE_2 = 19,
    SC64_T3_UNIT_HATCHERY = 131,
    SC64_T3_SELECTOR_ANY_UNIT = 229,
    SC64_T3_SELECTOR_BUILDINGS = 231,
    SC64_T3_ALTERNATE_MIN_REMAINING_SECONDS = 120
};

enum {
    SC64_T3_RUNNING = 0,
    SC64_T3_WON = 1,
    SC64_T3_LOST = 2
};

enum {
    SC64_T3_EVENT_INTRO = 1u << 0,
    SC64_T3_EVENT_AI_START = 1u << 1,
    SC64_T3_EVENT_REMINDER_10_MIN = 1u << 2,
    SC64_T3_EVENT_REMINDER_5_MIN = 1u << 3,
    SC64_T3_EVENT_REMINDER_1_MIN = 1u << 4,
    SC64_T3_EVENT_ALTERNATE_OBJECTIVE = 1u << 5,
    SC64_T3_EVENT_FORCE2_AI_AND_RESOURCES = 1u << 6,
    SC64_T3_EVENT_VICTORY = 1u << 8,
    SC64_T3_EVENT_DEFEAT = 1u << 9
};

enum {
    SC64_T3_F_INTRO = 1u << 0,
    SC64_T3_F_AI_START = 1u << 1,
    SC64_T3_F_10_MIN = 1u << 2,
    SC64_T3_F_5_MIN = 1u << 3,
    SC64_T3_F_1_MIN = 1u << 4,
    SC64_T3_F_ALTERNATE = 1u << 5,
    SC64_T3_F_FORCE2_AI = 1u << 6,
    SC64_T3_F_RESULT = 1u << 7,
    SC64_T3_F_SWITCH_0 = 1u << 8,
    SC64_T3_F_SWITCH_1 = 1u << 9
};

typedef struct {
    u32 elapsed_ticks;
    u32 remaining_ticks;
    u16 fired;
    u8 outcome;
    u8 objective; /* 0: survive; 1: survive OR destroy the Zerg Hatchery. */
} Sc64T3State;

typedef struct {
    u16 force1_buildings_killed; /* CHK Kill >= 1, Force 1, selector Buildings. */
    u8 human_buildings;          /* CHK Command <= 0, raw Player 2 selector (human slot 1). */
    u8 force2_hatcheries;        /* CHK Command <= 0, Force 2, Hatchery id 131. */
    u8 force2_any_units;         /* CHK Command >= 1, Force 2, selector Any Unit. */
    u8 paused;
} Sc64T3Input;

static inline u16 sc64_t3_init(Sc64T3State *state)
{
    u8 *p;
    u32 i;
    if (!state) return 0;
    p = (u8 *)state;
    for (i = 0; i < (u32)sizeof(*state); ++i) p[i] = 0;
    state->remaining_ticks = SC64_T3_DURATION_TICKS;
    /* TRIG 0's SetSwitch action targets switch #0 (the action's second group
     * field) and sets it. The optional objective is a different switch. */
    state->fired = SC64_T3_F_INTRO | SC64_T3_F_SWITCH_0;
    return SC64_T3_EVENT_INTRO;
}

static inline int sc64_t3_state_valid(const Sc64T3State *s)
{
    const u16 known = SC64_T3_F_INTRO | SC64_T3_F_AI_START | SC64_T3_F_10_MIN |
                      SC64_T3_F_5_MIN | SC64_T3_F_1_MIN | SC64_T3_F_ALTERNATE |
                      SC64_T3_F_FORCE2_AI | SC64_T3_F_RESULT |
                      SC64_T3_F_SWITCH_0 | SC64_T3_F_SWITCH_1;
    if (!s || s->elapsed_ticks > SC64_T3_DURATION_TICKS ||
        s->remaining_ticks > SC64_T3_DURATION_TICKS ||
        s->elapsed_ticks + s->remaining_ticks != SC64_T3_DURATION_TICKS ||
        (s->fired & (u16)~known) || s->outcome > SC64_T3_LOST || s->objective > 1)
        return 0;
    if (!(s->fired & SC64_T3_F_INTRO) || !(s->fired & SC64_T3_F_SWITCH_0)) return 0;
    if ((s->fired & SC64_T3_F_10_MIN) && s->remaining_ticks > 600u * 30u) return 0;
    if ((s->fired & SC64_T3_F_5_MIN) && s->remaining_ticks > 300u * 30u) return 0;
    if ((s->fired & SC64_T3_F_1_MIN) && s->remaining_ticks > 60u * 30u) return 0;
    if (((s->fired & SC64_T3_F_SWITCH_1) != 0) !=
        ((s->fired & SC64_T3_F_ALTERNATE) != 0)) return 0;
    if (((s->fired & SC64_T3_F_ALTERNATE) != 0) != (s->objective == 1)) return 0;
    if ((s->fired & SC64_T3_F_RESULT) != 0 && s->outcome == SC64_T3_RUNNING) return 0;
    if (s->outcome != SC64_T3_RUNNING && !(s->fired & SC64_T3_F_RESULT)) return 0;
    if (s->outcome == SC64_T3_WON && s->remaining_ticks &&
        !(s->fired & SC64_T3_F_SWITCH_1)) return 0;
    return 1;
}

static inline u32 sc64_t3_remaining_seconds(const Sc64T3State *s)
{
    return (s->remaining_ticks + SC64_T3_TICKS_PER_SECOND - 1u) /
           SC64_T3_TICKS_PER_SECOND;
}

/* Advance exactly one unpaused simulation tick and return one-shot events.
 * A paused tick is a no-op, matching the game's fixed-step pause behavior. */
static inline u16 sc64_t3_step(Sc64T3State *s, const Sc64T3Input *in)
{
    u16 events = 0;
    if (!sc64_t3_state_valid(s) || !in || in->paused || s->outcome) return 0;

    if (s->remaining_ticks) {
        --s->remaining_ticks;
        ++s->elapsed_ticks;
    }

    /* TRIG 7: raw Player 2 (human slot 1 in this map) commands at most zero
     * Buildings. The mission-specific adapter resolves defeat first if this
     * and a victory condition become true on the same tick; the source trigger
     * records establish both conditions but do not establish this tie policy. */
    if (!in->human_buildings) {
        s->outcome = SC64_T3_LOST;
        s->fired |= SC64_T3_F_RESULT;
        return SC64_T3_EVENT_DEFEAT;
    }

    /* TRIG 5: Force 2 commands at least one Any Unit; action raw resource type
     * 2 sets both Ore and Gas to 5000, and runs the location-6 AI script. */
    if (!(s->fired & SC64_T3_F_FORCE2_AI) && in->force2_any_units >= 1) {
        s->fired |= SC64_T3_F_FORCE2_AI;
        events |= SC64_T3_EVENT_FORCE2_AI_AND_RESOURCES;
    }

    /* TRIG 8: Force 2 runs its authored AI script when countdown <= 120 sec. */
    if (!(s->fired & SC64_T3_F_AI_START) && s->remaining_ticks <= 120u * 30u) {
        s->fired |= SC64_T3_F_AI_START;
        events |= SC64_T3_EVENT_AI_START;
    }

    /* TRIG 3: Force 1 has killed at least one Building while 120+ seconds
     * remain; set switch #1 (not the survival switch #0) and reveal the
     * alternate Hatchery objective. */
    if (!(s->fired & SC64_T3_F_ALTERNATE) &&
        in->force1_buildings_killed >= 1 &&
        sc64_t3_remaining_seconds(s) >= SC64_T3_ALTERNATE_MIN_REMAINING_SECONDS) {
        s->fired |= SC64_T3_F_ALTERNATE;
        s->fired |= SC64_T3_F_SWITCH_1;
        s->objective = 1;
        events |= SC64_T3_EVENT_ALTERNATE_OBJECTIVE;
    }

    /* TRIG 1, 2, 9: countdown at most 600, 300, and 60 seconds. */
    if (!(s->fired & SC64_T3_F_10_MIN) && s->remaining_ticks <= 600u * 30u) {
        s->fired |= SC64_T3_F_10_MIN;
        events |= SC64_T3_EVENT_REMINDER_10_MIN;
    }
    if (!(s->fired & SC64_T3_F_5_MIN) && s->remaining_ticks <= 300u * 30u) {
        s->fired |= SC64_T3_F_5_MIN;
        events |= SC64_T3_EVENT_REMINDER_5_MIN;
    }
    if (!(s->fired & SC64_T3_F_1_MIN) && s->remaining_ticks <= 60u * 30u) {
        s->fired |= SC64_T3_F_1_MIN;
        events |= SC64_T3_EVENT_REMINDER_1_MIN;
    }

    /* TRIG 4: Force 2 has at most zero Hatcheries (unit 131), switch #1 is set,
     * and at least one second remains. Its authored create-unit actions are
     * not modeled here: raw unit count is zero, and source-game semantics for
     * that value have not been verified. */
    if ((s->fired & SC64_T3_F_SWITCH_1) && in->force2_hatcheries == 0 &&
        s->remaining_ticks > 0) {
        s->outcome = SC64_T3_WON;
        s->fired |= SC64_T3_F_RESULT;
        events |= SC64_T3_EVENT_VICTORY;
        return events;
    }

    /* TRIG 6 requires switch #0 (set by TRIG 0 at mission start), then wins
     * at countdown <= 0 regardless of the optional building-kill objective.
     * Its authored create-unit actions are likewise outside this rules model. */
    if (!s->remaining_ticks && (s->fired & SC64_T3_F_SWITCH_0)) {
        s->outcome = SC64_T3_WON;
        s->fired |= SC64_T3_F_RESULT;
        events |= SC64_T3_EVENT_VICTORY;
    }
    return events;
}

#endif /* SC64_T3_H */
