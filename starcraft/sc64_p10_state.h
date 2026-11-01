/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P10_STATE_H
#define SC64_P10_STATE_H

#include "../common/gba.h"

/* Persistent P10 "Eye of the Storm" mission state. It shares the Protoss
 * mission save union with P5-P9, so it holds only what the bounded source
 * program and the authored services change: three trigger runners and their
 * shared waits, the first sixteen switches (P10 uses 0 and 8), the two
 * computer wallets, the five town AI schedules, the one location the source
 * moves, and the shared Protoss service clock. World views and callback
 * bindings are rebuilt on load. */
enum {
    SC64_P10_TRIGGER_LIMIT = 10,
    SC64_P10_ACTION_LIMIT = 32,   /* Longest trigger: 31 start-up actions. */
    SC64_P10_RUNNER_HUMAN = 0,    /* Source slot 1, force 1. */
    SC64_P10_RUNNER_OVERMIND,     /* Source slot 3, force 2. */
    SC64_P10_RUNNER_BROOD,        /* Source slot 0, force 3. */
    SC64_P10_RUNNER_COUNT,
    SC64_P10_WALLET_COUNT = 2,    /* Source slots 0 and 3. */
    SC64_P10_TOWN_COUNT = 5,
    SC64_P10_MOVED_LOCATION = 32, /* MoveLocation's only destination. */
    SC64_P10_OUTCOME_RUNNING = 0,
    SC64_P10_OUTCOME_WON = 1,
    SC64_P10_OUTCOME_LOST = 2
};

static const u8 sc64_p10_runner_slot[SC64_P10_RUNNER_COUNT] = { 1, 3, 0 };
static const u8 sc64_p10_runner_force[SC64_P10_RUNNER_COUNT] = { 1, 2, 3 };
static const u8 sc64_p10_wallet_slot[SC64_P10_WALLET_COUNT] = { 0, 3 };

typedef struct {
    u16 started, running, done, waiting;  /* Trigger bit masks. */
    u8 cursor[SC64_P10_TRIGGER_LIMIT];
    u8 outcome;
    u8 reserved;
} Sc64P10Runner;

typedef struct {
    u8 script;   /* 0 inactive, otherwise 1 + the town script index. */
    u8 wave;
    u16 timer;
} Sc64P10TownState;

typedef struct {
    Sc64P10Runner runner[SC64_P10_RUNNER_COUNT];
    u16 wait_ms[SC64_P10_RUNNER_COUNT];
    u16 wallet_minerals[SC64_P10_WALLET_COUNT];
    u16 wallet_gas[SC64_P10_WALLET_COUNT];
    Sc64P10TownState towns[SC64_P10_TOWN_COUNT];
    s16 moved_location[4];  /* Source rectangle of location 32 once moved. */
    u16 switches;           /* Source switches 0-15. */
    u8 waiting_mask;        /* Runner bits with an armed shared wait. */
    u8 pass_timer;          /* Updates until the next trigger pass. */
    u8 moved_location_valid;
    /* Shared Protoss service clock, laid out like P7's reserved bytes:
     * [0] is the source-update phase (0..4), [1..2] the secondary-order
     * stagger (0..300). Zero means the first source update is pending. */
    u8 service_clock[3];
} Sc64P10PersistentState;

_Static_assert(sizeof(Sc64P10PersistentState) <= 180,
               "P10 state must fit the existing Protoss mission save union");

static inline int sc64_p10_wallet_index(unsigned slot)
{
    for (unsigned i = 0; i < SC64_P10_WALLET_COUNT; ++i)
        if (sc64_p10_wallet_slot[i] == slot) return (int)i;
    return -1;
}

/* Structural checks; the runtime adapter checks cursors against the source. */
static inline int sc64_p10_persistent_state_valid(const Sc64P10PersistentState *s)
{
    const u16 trigger_mask = (u16)((1u << SC64_P10_TRIGGER_LIMIT) - 1u);
    if (!s || s->moved_location_valid > 1u ||
        (s->waiting_mask & (u8)~((1u << SC64_P10_RUNNER_COUNT) - 1u)) ||
        s->service_clock[0] > 4u ||
        ((unsigned)s->service_clock[1] | ((unsigned)s->service_clock[2] << 8)) > 300u)
        return 0;
    if (s->moved_location_valid) {
        if (s->moved_location[0] < 0 || s->moved_location[1] < 0 ||
            s->moved_location[2] < s->moved_location[0] ||
            s->moved_location[3] < s->moved_location[1] ||
            s->moved_location[2] > 4096 || s->moved_location[3] > 4096) return 0;
    } else if (s->moved_location[0] || s->moved_location[1] ||
               s->moved_location[2] || s->moved_location[3]) return 0;
    for (unsigned i = 0; i < SC64_P10_RUNNER_COUNT; ++i) {
        const Sc64P10Runner *r = &s->runner[i];
        if (r->reserved || r->outcome > SC64_P10_OUTCOME_LOST ||
            ((r->started | r->running | r->done | r->waiting) & (u16)~trigger_mask) ||
            r->started != (u16)(r->running | r->done) || (r->running & r->done) ||
            (r->waiting & (u16)~r->running)) return 0;
        for (unsigned t = 0; t < SC64_P10_TRIGGER_LIMIT; ++t)
            if ((!(r->running & (1u << t)) && r->cursor[t]) ||
                r->cursor[t] >= SC64_P10_ACTION_LIMIT) return 0;
        if (!(s->waiting_mask & (1u << i)) && s->wait_ms[i]) return 0;
    }
    for (unsigned i = 0; i < SC64_P10_TOWN_COUNT; ++i)
        if (s->towns[i].script > SC64_P10_TOWN_COUNT ||
            (!s->towns[i].script && (s->towns[i].wave || s->towns[i].timer)))
            return 0;
    return 1;
}

#endif /* SC64_P10_STATE_H */
