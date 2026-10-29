/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P9_STATE_H
#define SC64_P9_STATE_H

#include "../common/gba.h"

/* Persistent P9 "Shadow Hunters" mission state. It shares the Protoss
 * mission save union with P5-P8, so it holds only what the bounded source
 * program and the authored services change: four trigger runners and their
 * shared waits, the first eight switches (the only ones P9 uses), the
 * computer and rescuable wallets and the town AI schedule. World views and
 * callback bindings are rebuilt on load. */
enum {
    SC64_P9_TRIGGER_LIMIT = 12,
    SC64_P9_RUNNER_HUMAN = 0,   /* Source slot 7, force 1. */
    SC64_P9_RUNNER_RED,         /* Source slot 0, force 2. */
    SC64_P9_RUNNER_BAELROG,     /* Source slot 6, force 3. */
    SC64_P9_RUNNER_LEVIATHAN,   /* Source slot 5, force 4. */
    SC64_P9_RUNNER_COUNT,
    SC64_P9_WALLET_COUNT = 4,   /* Source slots 0, 2, 5 and 6. */
    SC64_P9_TOWN_COUNT = 3,
    SC64_P9_OUTCOME_RUNNING = 0,
    SC64_P9_OUTCOME_WON = 1,
    SC64_P9_OUTCOME_LOST = 2
};

static const u8 sc64_p9_runner_slot[SC64_P9_RUNNER_COUNT] = { 7, 0, 6, 5 };
static const u8 sc64_p9_runner_force[SC64_P9_RUNNER_COUNT] = { 1, 2, 3, 4 };
static const u8 sc64_p9_wallet_slot[SC64_P9_WALLET_COUNT] = { 0, 2, 5, 6 };

typedef struct {
    u16 started, running, done, waiting;  /* Trigger bit masks. */
    u8 cursor[SC64_P9_TRIGGER_LIMIT];
    u8 outcome;
    u8 reserved;
} Sc64P9Runner;

typedef struct {
    u8 script;   /* 0 inactive, otherwise 1 + the town script index. */
    u8 wave;
    u16 timer;
} Sc64P9TownState;

typedef struct {
    Sc64P9Runner runner[SC64_P9_RUNNER_COUNT];
    u16 wait_ms[SC64_P9_RUNNER_COUNT];
    u16 wallet_minerals[SC64_P9_WALLET_COUNT];
    u16 wallet_gas[SC64_P9_WALLET_COUNT];
    Sc64P9TownState towns[SC64_P9_TOWN_COUNT];
    u8 switches;        /* Source switches 0-7. */
    u8 waiting_mask;    /* Runner bits with an armed shared wait. */
    u8 pass_timer;      /* Updates until the next trigger pass. */
    u8 reserved;
} Sc64P9PersistentState;

_Static_assert(sizeof(Sc64P9PersistentState) <= 180,
               "P9 state must fit the existing Protoss mission save union");

static inline int sc64_p9_wallet_index(unsigned slot)
{
    for (unsigned i = 0; i < SC64_P9_WALLET_COUNT; ++i)
        if (sc64_p9_wallet_slot[i] == slot) return (int)i;
    return -1;
}

/* Structural checks; the runtime adapter checks cursors against the source. */
static inline int sc64_p9_persistent_state_valid(const Sc64P9PersistentState *s)
{
    const u16 trigger_mask = (u16)((1u << SC64_P9_TRIGGER_LIMIT) - 1u);
    if (!s || s->reserved || (s->waiting_mask & (u8)~0x0fu)) return 0;
    for (unsigned i = 0; i < SC64_P9_RUNNER_COUNT; ++i) {
        const Sc64P9Runner *r = &s->runner[i];
        if (r->reserved || r->outcome > SC64_P9_OUTCOME_LOST ||
            ((r->started | r->running | r->done | r->waiting) & (u16)~trigger_mask) ||
            r->started != (u16)(r->running | r->done) || (r->running & r->done) ||
            (r->waiting & (u16)~r->running)) return 0;
        for (unsigned t = 0; t < SC64_P9_TRIGGER_LIMIT; ++t)
            if (!(r->running & (1u << t)) && r->cursor[t]) return 0;
        if (!(s->waiting_mask & (1u << i)) && s->wait_ms[i]) return 0;
    }
    for (unsigned i = 0; i < SC64_P9_TOWN_COUNT; ++i)
        if (s->towns[i].script > SC64_P9_TOWN_COUNT ||
            (!s->towns[i].script && (s->towns[i].wave || s->towns[i].timer)))
            return 0;
    return 1;
}

#endif /* SC64_P9_STATE_H */
