/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_STATE_H
#define SC64_P8_STATE_H

#include "sc64_p8_runtime.h"

/* Persistent P8 mission state. It shares the Protoss mission save union, so
 * it stores only what the bounded source program and authored services can
 * change: three trigger owners, their shared waits, the switch bank, the one
 * MoveLocation target, the two computer wallets, the town AI schedule and
 * the shared Protoss service clock. Locations, world views and callback
 * bindings are rebuilt on load. */
enum {
    SC64_P8_RUNNER_HUMAN = 0,   /* Source slot 7, force 1. */
    SC64_P8_RUNNER_DEFENDERS,   /* Source slot 0, force 2. */
    SC64_P8_RUNNER_CONCLAVE,    /* Source slot 3, force 3. */
    SC64_P8_RUNNER_COUNT,
    SC64_P8_AI_WALLET_COUNT = 2, /* Source slots 0 and 3. */
    SC64_P8_AI_TOWN_COUNT = 3,
    SC64_P8_MOVED_LOCATION = 22, /* The only MoveLocation destination. */
    SC64_P8_SWITCH_BYTES = 32,
    SC64_P8_AI_WALLET_LIMIT = 65535
};

static const u8 sc64_p8_runner_slot[SC64_P8_RUNNER_COUNT] = { 7, 0, 3 };
static const u8 sc64_p8_runner_force[SC64_P8_RUNNER_COUNT] = { 1, 2, 3 };

typedef struct {
    u8 script;   /* 0 inactive, otherwise 1 + the town script index. */
    u8 wave;     /* Attack waves released by this town. */
    u16 timer;   /* Updates until the next production or wave check. */
} Sc64P8AiTownState;

typedef struct {
    Sc64P8RuntimeState runtime[SC64_P8_RUNNER_COUNT];
    u32 wait_ms[SC64_P8_RUNNER_COUNT];
    u32 ai_minerals[SC64_P8_AI_WALLET_COUNT];
    u32 ai_gas[SC64_P8_AI_WALLET_COUNT];
    s16 moved_location[4];  /* Source-pixel rectangle of location 22. */
    Sc64P8AiTownState towns[SC64_P8_AI_TOWN_COUNT];
    u8 switches[SC64_P8_SWITCH_BYTES];
    u8 waiting_mask;        /* Runner bits with an armed shared wait. */
    u8 pause_trigger_mask[SC64_P8_RUNNER_COUNT];
    u8 source_pause_active;
    u8 moved_location_valid;
    u8 camera_release;      /* Runner bits whose CenterView wait releases. */
    u8 pass_timer;          /* Updates until the next trigger pass. */
    /* Shared Protoss service clock, laid out like P7's reserved bytes:
     * [0] is the source-update phase (0..4), [1..2] the secondary-order
     * stagger (0..300). Zero means the first source update is pending. */
    u8 service_clock[3];
} Sc64P8PersistentState;

_Static_assert(sizeof(Sc64P8PersistentState) <= 180,
               "P8 state must fit the existing Protoss mission save union");

static inline int sc64_p8_runner_for_slot(unsigned slot)
{
    for (unsigned i = 0; i < SC64_P8_RUNNER_COUNT; ++i)
        if (sc64_p8_runner_slot[i] == slot) return (int)i;
    return -1;
}

/* Structural checks only. The runtime adapter additionally validates the
 * trigger cursors against the authenticated source program. */
static inline int sc64_p8_persistent_state_valid(const Sc64P8PersistentState *s)
{
    if (!s || s->source_pause_active > 1u || s->moved_location_valid > 1u ||
        s->service_clock[0] > 4u ||
        ((unsigned)s->service_clock[1] | ((unsigned)s->service_clock[2] << 8)) > 300u ||
        (s->waiting_mask & (u8)~7u) || (s->camera_release & (u8)~s->waiting_mask))
        return 0;
    for (unsigned i = 0; i < SC64_P8_RUNNER_COUNT; ++i) {
        const Sc64P8RuntimeState *r = &s->runtime[i];
        if (r->owner != sc64_p8_runner_slot[i] ||
            r->force != sc64_p8_runner_force[i] ||
            r->outcome > SC64_P8_OUTCOME_LOST || r->fault != SC64_P8_FAULT_NONE ||
            r->started != (u8)(r->running | r->done) || (r->running & r->done) ||
            (r->waiting & (u8)~r->running) ||
            r->fault_trigger != 255u || r->fault_slot != 255u) return 0;
        if (!(s->waiting_mask & (1u << i)) && s->wait_ms[i]) return 0;
        if (s->pause_trigger_mask[i] & (u8)~r->started) return 0;
    }
    /* Only the human runner owns P8 PauseGame rows. */
    if (s->pause_trigger_mask[1] || s->pause_trigger_mask[2]) return 0;
    if (!s->source_pause_active && s->pause_trigger_mask[0]) return 0;
    for (unsigned i = 0; i < SC64_P8_AI_WALLET_COUNT; ++i)
        if (s->ai_minerals[i] > SC64_P8_AI_WALLET_LIMIT ||
            s->ai_gas[i] > SC64_P8_AI_WALLET_LIMIT) return 0;
    if (s->moved_location_valid) {
        if (s->moved_location[0] < 0 || s->moved_location[1] < 0 ||
            s->moved_location[0] > s->moved_location[2] ||
            s->moved_location[1] > s->moved_location[3] ||
            s->moved_location[2] > 4096 || s->moved_location[3] > 4096) return 0;
    } else if (s->moved_location[0] || s->moved_location[1] ||
               s->moved_location[2] || s->moved_location[3]) return 0;
    for (unsigned i = 0; i < SC64_P8_AI_TOWN_COUNT; ++i)
        if (s->towns[i].script > SC64_P8_AI_TOWN_COUNT ||
            (!s->towns[i].script && (s->towns[i].wave || s->towns[i].timer)))
            return 0;
    return 1;
}

#endif /* SC64_P8_STATE_H */
