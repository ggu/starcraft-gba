/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_RUNTIME_H
#define SC64_P8_RUNTIME_H

#include "../common/gba.h"

/* Bounded one-source-owner policy runner for compact P8 trigger records.
 * Callers provide the source table through callbacks, so active rows need not
 * be expanded into 8 x 64 fixed-size records. This is an authored pass
 * policy; the caller chooses when a pass runs, and this does not claim a
 * native P8 cadence. Runtime callbacks, mission save encoding and canonical
 * game wiring remain separate work; this model alone does not enable P8 or
 * prove it playable. */
enum {
    SC64_P8_TRIGGER_LIMIT = 8,
    SC64_P8_CONDITION_LIMIT = 16,
    SC64_P8_ACTION_LIMIT = 64,
    SC64_P8_EXECUTOR_COUNT = 27,
    SC64_P8_OUTCOME_RUNNING = 0,
    SC64_P8_OUTCOME_WON = 1,
    SC64_P8_OUTCOME_LOST = 2,
    SC64_P8_FAULT_NONE = 0,
    SC64_P8_FAULT_PROGRAM = 1,
    SC64_P8_FAULT_CONDITION = 2,
    SC64_P8_FAULT_ACTION = 3,
    SC64_P8_ACTION_PENDING = 0,
    SC64_P8_ACTION_COMPLETE = 1,
    SC64_P8_ACTION_WON = 2,
    SC64_P8_ACTION_LOST = 3,
    SC64_P8_ACTION_BLOCKED = 4
};

typedef struct {
    u8 action_count;
    u8 condition_count;
    u8 enabled_players[SC64_P8_EXECUTOR_COUNT];
    u8 initial_cursor;
    u32 execution_flags;
} Sc64P8TriggerInfo;

typedef struct {
    u8 started;
    u8 running;
    u8 done;
    u8 waiting; /* Pending async action; BLOCKED owner-wait rows are not set. */
    u8 cursor[SC64_P8_TRIGGER_LIMIT];
    u8 owner;
    u8 force;
    u8 outcome;
    u8 fault;
    u8 fault_trigger;
    u8 fault_slot;
} Sc64P8RuntimeState;

_Static_assert(sizeof(Sc64P8RuntimeState) <= 20,
               "P8 runtime state must remain compact");

typedef struct {
    void *context;
    /* Supply immutable metadata for one compact source trigger. */
    int (*trigger_info)(void *context, unsigned trigger,
                        Sc64P8TriggerInfo *info);
    /* Return 1/0, or a negative value for an unsupported condition. Must be
     * pure: a condition query cannot mutate world or runtime state. */
    int (*condition)(void *context, unsigned trigger, unsigned condition,
                     unsigned owner);
    /* Return COMPLETE/PENDING/WON/LOST/BLOCKED or a negative value when
     * unsupported. PENDING means an asynchronous effect has started and must
     * resume at the same cursor. BLOCKED means the effect was not started
     * because an owner-wide wait is active; retry it without resume next pass.
     * For P8 Wait/Transmission/CenterView, callers must own one shared timer
     * per source owner and an armed marker per action. While a timer is active,
     * a blocked timed action yields as BLOCKED; immediate actions in later
     * triggers still execute. This mirrors the native-backed Z8 action model;
     * P8 itself has no cadence/runtime trace qualification here. */
    int (*action)(void *context, unsigned trigger, unsigned action,
                  unsigned owner, int resume);
} Sc64P8WorldOps;

static inline int
sc64_p8_runtime_init(Sc64P8RuntimeState *state, unsigned owner, unsigned force)
{
    if (!state || owner >= 8 || force < 1 || force > 4) return 0;
    u8 *bytes = (u8 *)state;
    for (unsigned i = 0; i < sizeof(*state); ++i) bytes[i] = 0;
    state->owner = (u8)owner;
    state->force = (u8)force;
    state->fault_trigger = 255;
    state->fault_slot = 255;
    return 1;
}

static inline int
sc64_p8_runtime_state_valid(const Sc64P8RuntimeState *state,
                           const Sc64P8WorldOps *ops)
{
    if (!state || !ops || !ops->trigger_info || !ops->condition ||
        !ops->action || state->owner >= 8 || state->force < 1 ||
        state->force > 4 || state->outcome > SC64_P8_OUTCOME_LOST ||
        state->fault != SC64_P8_FAULT_NONE ||
        (state->running & (u8)~state->started) ||
        (state->done & (u8)~state->started) ||
        state->started != (u8)(state->running | state->done) ||
        (state->running & state->done) ||
        (state->waiting & (u8)~state->running) ||
        state->fault_trigger != 255 || state->fault_slot != 255) return 0;
    for (unsigned i = 0; i < SC64_P8_TRIGGER_LIMIT; ++i) {
        Sc64P8TriggerInfo info = {0};
        if (ops->trigger_info(ops->context, i, &info) != 1 ||
            info.action_count > SC64_P8_ACTION_LIMIT ||
            info.condition_count > SC64_P8_CONDITION_LIMIT ||
            info.initial_cursor || info.execution_flags) return 0;
        if (state->running & (u8)(1u << i)) {
            if (state->cursor[i] >= info.action_count) return 0;
        } else if (state->cursor[i]) return 0;
    }
    return 1;
}

static inline int
sc64_p8_runtime_trigger_enabled(const Sc64P8TriggerInfo *info,
                                unsigned owner, unsigned force)
{
    if (!info || owner >= 8 || force < 1 || force > 4) return 0;
    for (unsigned p = 0; p < SC64_P8_EXECUTOR_COUNT; ++p) {
        unsigned enabled = info->enabled_players[p];
        if (enabled > 1) return -1;
        if (enabled && p != owner && p != 17u &&
            !(p >= 18u && p <= 21u)) return -1;
    }
    return info->enabled_players[owner] || info->enabled_players[17] ||
           info->enabled_players[17u + force];
}

static inline int
sc64_p8_runtime_step(Sc64P8RuntimeState *state, const Sc64P8WorldOps *ops,
                     int paused)
{
    u8 pass_outcome = SC64_P8_OUTCOME_RUNNING;
    if (!state || !ops || paused < 0 || paused > 1 ||
        !sc64_p8_runtime_state_valid(state, ops)) return 0;
    if (paused || state->outcome != SC64_P8_OUTCOME_RUNNING) return 1;

    for (unsigned trigger = 0; trigger < SC64_P8_TRIGGER_LIMIT; ++trigger) {
        Sc64P8TriggerInfo info = {0};
        if (ops->trigger_info(ops->context, trigger, &info) != 1) {
            state->fault = SC64_P8_FAULT_PROGRAM;
            state->fault_trigger = (u8)trigger;
            return 0;
        }
        int enabled = sc64_p8_runtime_trigger_enabled(
            &info, state->owner, state->force);
        if (enabled < 0) {
            state->fault = SC64_P8_FAULT_PROGRAM;
            state->fault_trigger = (u8)trigger;
            return 0;
        }
        if (!enabled || (state->done & (u8)(1u << trigger))) continue;

        if (!(state->running & (u8)(1u << trigger))) {
            int matches = 1;
            for (unsigned slot = 0; slot < info.condition_count; ++slot) {
                int result = ops->condition(ops->context, trigger, slot,
                                            state->owner);
                if (result < 0) {
                    state->fault = SC64_P8_FAULT_CONDITION;
                    state->fault_trigger = (u8)trigger;
                    state->fault_slot = (u8)slot;
                    return 0;
                }
                if (result > 1) {
                    state->fault = SC64_P8_FAULT_CONDITION;
                    state->fault_trigger = (u8)trigger;
                    state->fault_slot = (u8)slot;
                    return 0;
                }
                if (!result) { matches = 0; break; }
            }
            if (!matches) continue;
            state->started |= (u8)(1u << trigger);
            if (!info.action_count) {
                state->done |= (u8)(1u << trigger);
                continue;
            }
            state->running |= (u8)(1u << trigger);
            state->cursor[trigger] = 0;
        }

        while (state->running & (u8)(1u << trigger)) {
            unsigned slot = state->cursor[trigger];
            int resume = !!(state->waiting & (u8)(1u << trigger));
            int result = ops->action(ops->context, trigger, slot,
                                     state->owner, resume);
            if (result == SC64_P8_ACTION_PENDING) {
                state->waiting |= (u8)(1u << trigger);
                break;
            }
            if (result == SC64_P8_ACTION_BLOCKED) {
                state->waiting &= (u8)~(1u << trigger);
                break;
            }
            state->waiting &= (u8)~(1u << trigger);
            if (result < SC64_P8_ACTION_COMPLETE ||
                result > SC64_P8_ACTION_LOST) {
                state->fault = SC64_P8_FAULT_ACTION;
                state->fault_trigger = (u8)trigger;
                state->fault_slot = (u8)slot;
                return 0;
            }
            if (result == SC64_P8_ACTION_WON)
                pass_outcome = SC64_P8_OUTCOME_WON;
            else if (result == SC64_P8_ACTION_LOST &&
                     pass_outcome != SC64_P8_OUTCOME_WON)
                pass_outcome = SC64_P8_OUTCOME_LOST;
            ++state->cursor[trigger];
            if (state->cursor[trigger] == info.action_count) {
                state->running &= (u8)~(1u << trigger);
                state->done |= (u8)(1u << trigger);
                state->cursor[trigger] = 0;
            }
        }
    }
    if (pass_outcome != SC64_P8_OUTCOME_RUNNING)
        state->outcome = pass_outcome;
    return 1;
}

#endif /* SC64_P8_RUNTIME_H */
