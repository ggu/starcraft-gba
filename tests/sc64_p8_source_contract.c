/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional local contract check. The generated descriptor remains ignored. */
#include "../starcraft/sc64_p8_source_adapter.h"
#include <assert.h>

static int condition_true(void *context,
                          const Sc64P8SourceCondition *condition,
                          unsigned owner)
{
    (void)context;
    (void)condition;
    (void)owner;
    return 1;
}

static int unsupported_action(void *context,
                              const Sc64P8SourceAction *action,
                              unsigned owner, int resume)
{
    (void)context;
    (void)action;
    (void)owner;
    (void)resume;
    return -1;
}

static int selected_action_result(void *context,
                                  const Sc64P8SourceAction *action,
                                  unsigned owner, int resume)
{
    (void)action;
    (void)owner;
    (void)resume;
    return *(const int *)context;
}

int main(void)
{
    assert(sc64_p8_source_metadata_valid());
    unsigned actions = 0, conditions = 0;
    for (unsigned i = 0; i < SC64_P8_SOURCE_TRIGGER_COUNT; ++i) {
        actions += sc64_p8_source_triggers[i].action_count;
        conditions += sc64_p8_source_triggers[i].condition_count;
    }
    assert(actions == SC64_P8_EXPECTED_ACTION_COUNT);
    assert(conditions == SC64_P8_EXPECTED_CONDITION_COUNT);

    int action_result = SC64_P8_ACTION_COMPLETE;
    Sc64P8SourceWorldOps checked_world = {&action_result, condition_true,
                                           selected_action_result};
    unsigned victory_trigger = SC64_P8_TRIGGER_LIMIT;
    unsigned victory_slot = 0;
    unsigned defeat_trigger = SC64_P8_TRIGGER_LIMIT;
    unsigned defeat_slot = 0;
    for (unsigned t = 0; t < SC64_P8_SOURCE_TRIGGER_COUNT; ++t) {
        const Sc64P8SourceTrigger *tr = &sc64_p8_source_triggers[t];
        for (unsigned i = 0; i < tr->action_count; ++i) {
            unsigned opcode = sc64_p8_source_actions[tr->first_action + i].opcode;
            if (opcode == 1u) { victory_trigger = t; victory_slot = i; }
            if (opcode == 2u) { defeat_trigger = t; defeat_slot = i; }
        }
    }
    assert(victory_trigger < SC64_P8_TRIGGER_LIMIT &&
           defeat_trigger < SC64_P8_TRIGGER_LIMIT);
    assert(sc64_p8_source_action_dispatch(&checked_world, victory_trigger,
             victory_slot, 0, 0) == -1); /* Complete cannot skip outcome. */
    action_result = SC64_P8_ACTION_WON;
    assert(sc64_p8_source_action_dispatch(&checked_world, victory_trigger,
             victory_slot, 0, 0) == SC64_P8_ACTION_WON);
    assert(sc64_p8_source_action_dispatch(&checked_world, defeat_trigger,
             defeat_slot, 0, 0) == -1); /* Wrong terminal kind fails closed. */
    action_result = SC64_P8_ACTION_LOST;
    assert(sc64_p8_source_action_dispatch(&checked_world, defeat_trigger,
             defeat_slot, 0, 0) == SC64_P8_ACTION_LOST);

    Sc64P8SourceWorldOps world = {0, condition_true, unsupported_action};
    Sc64P8WorldOps ops = sc64_p8_source_world_ops(&world);
    Sc64P8RuntimeState state;
    assert(sc64_p8_source_runtime_init(&state, 0, 1));
    /* This exercises the actual authenticated descriptor through its typed
     * view; the first unimplemented world action must fault instead of skip. */
    assert(!sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.fault == SC64_P8_FAULT_ACTION);
    assert(state.fault_trigger < SC64_P8_TRIGGER_LIMIT);
    return 0;
}
