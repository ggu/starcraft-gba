/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_SOURCE_ADAPTER_H
#define SC64_P8_SOURCE_ADAPTER_H

#include "sc64_p8_scenario_generated.h"
#include "sc64_p8_runtime.h"

enum {
    SC64_P8_EXPECTED_GLOBAL_ID = 27,
    SC64_P8_EXPECTED_MAP_COLS = 128,
    SC64_P8_EXPECTED_MAP_ROWS = 128,
    SC64_P8_EXPECTED_TRIGGER_COUNT = 8,
    SC64_P8_EXPECTED_ACTION_COUNT = 68,
    SC64_P8_EXPECTED_CONDITION_COUNT = 9,
    SC64_P8_EXPECTED_LOCATION_COUNT = 64
};

_Static_assert(SC64_P8_SOURCE_GLOBAL_ID == SC64_P8_EXPECTED_GLOBAL_ID,
               "P8 source identity mismatch");
_Static_assert(SC64_P8_SOURCE_MAP_COLS == SC64_P8_EXPECTED_MAP_COLS &&
               SC64_P8_SOURCE_MAP_ROWS == SC64_P8_EXPECTED_MAP_ROWS,
               "P8 source map dimensions mismatch");
_Static_assert(SC64_P8_SOURCE_TRIGGER_COUNT == SC64_P8_EXPECTED_TRIGGER_COUNT &&
               SC64_P8_SOURCE_ACTION_COUNT == SC64_P8_EXPECTED_ACTION_COUNT &&
               SC64_P8_SOURCE_CONDITION_COUNT == SC64_P8_EXPECTED_CONDITION_COUNT &&
               SC64_P8_SOURCE_LOCATION_COUNT == SC64_P8_EXPECTED_LOCATION_COUNT,
               "P8 source table counts mismatch");

typedef struct {
    void *context;
    int (*condition)(void *context, const Sc64P8SourceCondition *condition,
                     unsigned owner);
    int (*action)(void *context, const Sc64P8SourceAction *action,
                  unsigned owner, int resume);
} Sc64P8SourceWorldOps;

/* Validate the private compact descriptor before any runtime cursor can
 * index it. PreserveTrigger and prestarted records are intentionally rejected
 * until their repeat/save semantics have an authored implementation. */
static inline int sc64_p8_source_metadata_valid(void)
{
    if (SC64_P8_SOURCE_GLOBAL_ID != SC64_P8_EXPECTED_GLOBAL_ID ||
        SC64_P8_SOURCE_MAP_COLS != SC64_P8_EXPECTED_MAP_COLS ||
        SC64_P8_SOURCE_MAP_ROWS != SC64_P8_EXPECTED_MAP_ROWS ||
        SC64_P8_SOURCE_TRIGGER_COUNT != SC64_P8_TRIGGER_LIMIT ||
        SC64_P8_SOURCE_ACTION_COUNT != SC64_P8_EXPECTED_ACTION_COUNT ||
        SC64_P8_SOURCE_CONDITION_COUNT != SC64_P8_EXPECTED_CONDITION_COUNT ||
        SC64_P8_SOURCE_LOCATION_COUNT != SC64_P8_EXPECTED_LOCATION_COUNT) return 0;

    unsigned next_action = 0, next_condition = 0;
    for (unsigned t = 0; t < SC64_P8_TRIGGER_LIMIT; ++t) {
        const Sc64P8SourceTrigger *tr = &sc64_p8_source_triggers[t];
        if (tr->first_action != next_action ||
            tr->first_condition != next_condition ||
            tr->action_count > SC64_P8_ACTION_LIMIT ||
            tr->condition_count > SC64_P8_CONDITION_LIMIT ||
            tr->first_action + tr->action_count > SC64_P8_SOURCE_ACTION_COUNT ||
            tr->first_condition + tr->condition_count > SC64_P8_SOURCE_CONDITION_COUNT ||
            tr->execution_flags || tr->initial_cursor) return 0;
        for (unsigned p = 0; p < SC64_P8_EXECUTOR_COUNT; ++p) {
            unsigned enabled = tr->enabled_players[p];
            if (enabled > 1 ||
                (enabled && !(p < 8 || p == 17 || (p >= 18 && p <= 21))))
                return 0;
        }
        next_action += tr->action_count;
        next_condition += tr->condition_count;
    }
    if (next_action != SC64_P8_SOURCE_ACTION_COUNT ||
        next_condition != SC64_P8_SOURCE_CONDITION_COUNT) return 0;

    for (unsigned i = 0; i < SC64_P8_SOURCE_ACTION_COUNT; ++i) {
        const Sc64P8SourceAction *a = &sc64_p8_source_actions[i];
        if (a->trigger >= SC64_P8_TRIGGER_LIMIT || !a->opcode || a->opcode >= 59 ||
            a->location > SC64_P8_SOURCE_LOCATION_COUNT)
            return 0;
        const Sc64P8SourceTrigger *tr = &sc64_p8_source_triggers[a->trigger];
        if (i < tr->first_action || i >= tr->first_action + tr->action_count ||
            a->index != i - tr->first_action || a->opcode == 3u) return 0;
        if ((a->opcode == 1u || a->opcode == 2u) &&
            a->index + 1u != tr->action_count) return 0;
    }
    for (unsigned i = 0; i < SC64_P8_SOURCE_CONDITION_COUNT; ++i) {
        const Sc64P8SourceCondition *c = &sc64_p8_source_conditions[i];
        if (c->trigger >= SC64_P8_TRIGGER_LIMIT || !c->opcode || c->opcode >= 24 ||
            c->location > SC64_P8_SOURCE_LOCATION_COUNT) return 0;
        const Sc64P8SourceTrigger *tr = &sc64_p8_source_triggers[c->trigger];
        if (i < tr->first_condition || i >= tr->first_condition + tr->condition_count ||
            c->index != i - tr->first_condition) return 0;
    }
    for (unsigned i = 0; i < SC64_P8_SOURCE_LOCATION_COUNT; ++i) {
        const Sc64P8SourceLocation *loc = &sc64_p8_source_locations[i];
        if (loc->left > loc->right || loc->top > loc->bottom ||
            loc->right > SC64_P8_SOURCE_MAP_COLS * 32u ||
            loc->bottom > SC64_P8_SOURCE_MAP_ROWS * 32u) return 0;
    }
    return 1;
}

static inline int sc64_p8_source_runtime_init(Sc64P8RuntimeState *state,
                                              unsigned owner,
                                              unsigned force)
{
    return sc64_p8_source_metadata_valid() &&
           sc64_p8_runtime_init(state, owner, force);
}

static inline int sc64_p8_source_trigger_info(void *context, unsigned trigger,
                                                Sc64P8TriggerInfo *info)
{
    (void)context;
    if (!info || trigger >= SC64_P8_TRIGGER_LIMIT) return 0;
    const Sc64P8SourceTrigger *tr = &sc64_p8_source_triggers[trigger];
    info->action_count = (u8)tr->action_count;
    info->condition_count = (u8)tr->condition_count;
    info->initial_cursor = tr->initial_cursor;
    info->execution_flags = tr->execution_flags;
    for (unsigned i = 0; i < SC64_P8_EXECUTOR_COUNT; ++i)
        info->enabled_players[i] = tr->enabled_players[i];
    return 1;
}

static inline int sc64_p8_source_condition_dispatch(void *context,
                                                     unsigned trigger,
                                                     unsigned condition,
                                                     unsigned owner)
{
    Sc64P8SourceWorldOps *ctx = context;
    if (!ctx || !ctx->condition || trigger >= SC64_P8_TRIGGER_LIMIT) return -1;
    const Sc64P8SourceTrigger *tr = &sc64_p8_source_triggers[trigger];
    if (condition >= tr->condition_count) return -1;
    return ctx->condition(ctx->context,
        &sc64_p8_source_conditions[tr->first_condition + condition],
        owner);
}

static inline int sc64_p8_source_action_dispatch(void *context,
                                                  unsigned trigger,
                                                  unsigned action,
                                                  unsigned owner,
                                                  int resume)
{
    Sc64P8SourceWorldOps *ctx = context;
    if (!ctx || !ctx->action || trigger >= SC64_P8_TRIGGER_LIMIT) return -1;
    const Sc64P8SourceTrigger *tr = &sc64_p8_source_triggers[trigger];
    if (action >= tr->action_count) return -1;
    const Sc64P8SourceAction *source =
        &sc64_p8_source_actions[tr->first_action + action];
    int result = ctx->action(ctx->context, source, owner, resume);
    if (result == SC64_P8_ACTION_WON && source->opcode != 1u) return -1;
    if (result == SC64_P8_ACTION_LOST && source->opcode != 2u) return -1;
    if (source->opcode == 1u && result != SC64_P8_ACTION_WON &&
        result != SC64_P8_ACTION_PENDING) return -1;
    if (source->opcode == 2u && result != SC64_P8_ACTION_LOST &&
        result != SC64_P8_ACTION_PENDING) return -1;
    return result;
}

static inline Sc64P8WorldOps sc64_p8_source_world_ops(Sc64P8SourceWorldOps *ctx)
{
    Sc64P8WorldOps ops = {ctx, sc64_p8_source_trigger_info,
        sc64_p8_source_condition_dispatch, sc64_p8_source_action_dispatch};
    return ops;
}

#endif /* SC64_P8_SOURCE_ADAPTER_H */
