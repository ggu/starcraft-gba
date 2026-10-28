/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_TIMED_SERVICE_H
#define SC64_P8_TIMED_SERVICE_H

#include "sc64_p8_world_adapter.h"

enum {
    SC64_P8_TIMED_ELIGIBLE = SC64_Z8_TIMED_ELIGIBLE,
    SC64_P8_TIMED_START = SC64_Z8_TIMED_START,
    SC64_P8_TIMED_FINISH = SC64_Z8_TIMED_FINISH,
    SC64_P8_OWNER_COUNT = SC64_Z8_OWNER_LIMIT
};

typedef struct {
    u32 wait_ms[SC64_P8_OWNER_COUNT];
    void *callback_context;
    void *pause_context;
    void *pause_end_context;
    int (*presentation)(void *context, int phase,
                        const Sc64P8SourceAction *action,
                        unsigned trigger_owner, const Sc64Z8Rect *rect,
                        u32 *duration_ms);
    int (*pause_backend)(void *context, const Sc64P8SourceAction *action,
                         unsigned trigger_owner, int paused);
    int (*pause_end_backend)(void *context, unsigned trigger,
                             unsigned trigger_owner);
    u8 waiting_mask;
    u8 pause_trigger_mask[SC64_P8_OWNER_COUNT];
    u8 frame_ms;
    u8 source_pause_active;
    u8 reserved;
} Sc64P8TimedService;

_Static_assert(sizeof(Sc64P8TimedService) <= 104,
               "P8 timed service state must remain bounded");

static inline int
sc64_p8_timed_service_init(Sc64P8TimedService *state, unsigned frame_ms)
{
    if (!state || !sc64_z8_frame_ms_valid(frame_ms)) return 0;
    u8 *bytes = (u8 *)state;
    for (unsigned i = 0; i < sizeof(*state); ++i) bytes[i] = 0;
    state->frame_ms = (u8)frame_ms;
    return 1;
}

static inline int
sc64_p8_timed_service_valid(const Sc64P8TimedService *state)
{
    if (!state || !sc64_z8_frame_ms_valid(state->frame_ms) ||
        state->reserved || state->source_pause_active > 1u) return 0;
    for (unsigned owner = 0; owner < SC64_P8_OWNER_COUNT; ++owner)
        if (!(state->waiting_mask & (u8)(1u << owner)) &&
            state->wait_ms[owner]) return 0;
    return 1;
}

static inline int
sc64_p8_timed_service_set_frame_ms(Sc64P8TimedService *state,
                                  unsigned frame_ms)
{
    if (!sc64_p8_timed_service_valid(state) ||
        !sc64_z8_frame_ms_valid(frame_ms)) return 0;
    state->frame_ms = (u8)frame_ms;
    return 1;
}

/* Native PauseGame sets a source-pause marker. The game is paused, but the
 * native trigger scheduler still advances its clock and evaluates passes
 * while that marker is set. A user pause without the source marker freezes
 * both. This helper reports the native trigger/timer pause condition. */
static inline int
sc64_p8_timed_service_clock_paused(const Sc64P8TimedService *state,
                                   int game_paused)
{
    if (!sc64_p8_timed_service_valid(state) ||
        (game_paused != 0 && game_paused != 1)) return -1;
    return game_paused && !state->source_pause_active;
}

/* Call once per selected game frame before the caller's P8 trigger pass.
 * P8's pass cadence remains caller-owned and is not native-qualified. */
static inline int
sc64_p8_timed_service_tick(Sc64P8TimedService *state, int game_paused)
{
    int clock_paused = sc64_p8_timed_service_clock_paused(state, game_paused);
    if (clock_paused < 0) return 0;
    if (clock_paused) return 1;
    for (unsigned owner = 0; owner < SC64_P8_OWNER_COUNT; ++owner) {
        u8 bit = (u8)(1u << owner);
        if (!(state->waiting_mask & bit) ||
            state->wait_ms[owner] == SC64_Z8_WAIT_INDEFINITE) continue;
        if (state->wait_ms[owner] > state->frame_ms)
            state->wait_ms[owner] -= state->frame_ms;
        else {
            state->wait_ms[owner] = 0;
            state->waiting_mask &= (u8)~bit;
        }
    }
    return 1;
}

/* Match the native source-pause instance bookkeeping. The callback applies
 * the actual engine pause side effect and must fail without mutation. A
 * repeated PauseGame while source-pause is already active is a native no-op;
 * UnpauseGame only acts while the source marker is active. */
static inline int
sc64_p8_timed_service_pause_action(void *opaque,
                                   const Sc64P8SourceAction *action,
                                   unsigned owner, int paused)
{
    Sc64P8TimedService *state = (Sc64P8TimedService *)opaque;
    if (!sc64_p8_timed_service_valid(state) || !action ||
        action->trigger >= SC64_P8_TRIGGER_LIMIT ||
        owner >= SC64_P8_OWNER_COUNT || (paused != 0 && paused != 1) ||
        action->opcode != (paused ? 5u : 6u)) return -1;
    if (paused && state->source_pause_active) return SC64_P8_ACTION_COMPLETE;
    if (!paused && !state->source_pause_active) return SC64_P8_ACTION_COMPLETE;
    if (!state->pause_backend || !state->pause_end_backend ||
        state->pause_backend(state->pause_context, action, owner, paused) != 1)
        return -1;
    u8 bit = (u8)(1u << action->trigger);
    if (paused) {
        state->source_pause_active = 1;
        state->pause_trigger_mask[owner] |= bit;
    } else {
        state->source_pause_active = 0;
        state->pause_trigger_mask[owner] &= (u8)~bit;
    }
    return SC64_P8_ACTION_COMPLETE;
}

/* Native D108 dispatches UnpauseGame through the action-handler table when a
 * PauseGame-marked trigger reaches its action-list tail. The callback applies
 * that engine side effect. */
static inline int
sc64_p8_timed_service_trigger_complete(void *opaque, unsigned trigger,
                                       unsigned owner)
{
    Sc64P8TimedService *state = (Sc64P8TimedService *)opaque;
    if (!sc64_p8_timed_service_valid(state) ||
        trigger >= SC64_P8_TRIGGER_LIMIT || owner >= SC64_P8_OWNER_COUNT)
        return -1;
    u8 bit = (u8)(1u << trigger);
    if (!(state->pause_trigger_mask[owner] & bit)) return 1;
    if (!state->pause_end_backend ||
        state->pause_end_backend(state->pause_end_context, trigger, owner) != 1)
        return -1;
    state->pause_trigger_mask[owner] &= (u8)~bit;
    state->source_pause_active = 0;
    return 1;
}

static inline int
sc64_p8_timed_service_release_wait(Sc64P8TimedService *state, unsigned owner)
{
    if (!sc64_p8_timed_service_valid(state) || owner >= SC64_P8_OWNER_COUNT ||
        !(state->waiting_mask & (u8)(1u << owner))) return 0;
    state->waiting_mask &= (u8)~(1u << owner);
    state->wait_ms[owner] = 0;
    return 1;
}

/* Shared per-owner timers plus the runtime's per-trigger `resume` bit provide
 * the native Wait/Transmission/CenterView contract. A presentation callback
 * supplies native-dependent eligibility, start and finish effects. */
static inline int
sc64_p8_timed_service_action(void *opaque,
                             const Sc64P8SourceAction *action,
                             unsigned owner, const Sc64Z8Rect *rect,
                             int resume)
{
    Sc64P8TimedService *state = (Sc64P8TimedService *)opaque;
    u8 owner_bit;
    if (!sc64_p8_timed_service_valid(state) || !action ||
        owner >= SC64_P8_OWNER_COUNT || (resume != 0 && resume != 1))
        return -1;
    owner_bit = (u8)(1u << owner);
    if (action->opcode == 4u) {
        if (state->waiting_mask & owner_bit)
            return resume ? SC64_P8_ACTION_PENDING : SC64_P8_ACTION_BLOCKED;
        if (resume) return SC64_P8_ACTION_COMPLETE;
        state->wait_ms[owner] = action->time;
        state->waiting_mask |= owner_bit;
        return SC64_P8_ACTION_PENDING;
    }
    if (action->opcode != 7u && action->opcode != 10u) return -1;
    if (!action->location) return resume ? -1 : SC64_P8_ACTION_COMPLETE;
    if (!state->presentation || !rect) return -1;

    u32 duration = 0;
    int eligible = state->presentation(state->callback_context,
        SC64_P8_TIMED_ELIGIBLE, action, owner, rect, &duration);
    if (eligible == 0) return resume ? -1 : SC64_P8_ACTION_COMPLETE;
    if (eligible != 1) return -1;
    if (state->waiting_mask & owner_bit)
        return resume ? SC64_P8_ACTION_PENDING : SC64_P8_ACTION_BLOCKED;
    if (resume) {
        if (state->presentation(state->callback_context,
                SC64_P8_TIMED_FINISH, action, owner, rect, &duration) != 1)
            return -1;
        return SC64_P8_ACTION_COMPLETE;
    }
    if (action->opcode == 10u) duration = SC64_Z8_WAIT_INDEFINITE;
    if (state->presentation(state->callback_context, SC64_P8_TIMED_START,
            action, owner, rect, &duration) != 1) return -1;
    if (action->opcode == 10u) duration = SC64_Z8_WAIT_INDEFINITE;
    state->wait_ms[owner] = duration;
    state->waiting_mask |= owner_bit;
    return SC64_P8_ACTION_PENDING;
}

/* Bind to the P8 world adapter without replacing its actor/AI context. */
static inline void
sc64_p8_timed_service_bind_world(Sc64P8WorldContext *world,
                                 Sc64P8TimedService *state)
{
    if (!world || !state) return;
    world->async_action = sc64_p8_timed_service_action;
    world->async_context = state;
    if (world->pause_game != sc64_p8_timed_service_pause_action) {
        state->pause_backend = world->pause_game;
        state->pause_context = world->pause_context ? world->pause_context :
                               world->context;
    }
    if (world->pause_trigger_end != sc64_p8_timed_service_trigger_complete) {
        state->pause_end_backend = world->pause_trigger_end;
        state->pause_end_context = world->pause_end_context ?
                                  world->pause_end_context : world->context;
    }
    world->pause_game = sc64_p8_timed_service_pause_action;
    world->pause_context = state;
    world->pause_trigger_end = sc64_p8_timed_service_trigger_complete;
    world->pause_end_context = state;
}

#endif /* SC64_P8_TIMED_SERVICE_H */
