/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_p8_runtime.h"
#include <assert.h>

typedef struct {
    Sc64P8TriggerInfo trigger[SC64_P8_TRIGGER_LIMIT];
    int condition_value[SC64_P8_TRIGGER_LIMIT][SC64_P8_CONDITION_LIMIT];
    int action_result[SC64_P8_TRIGGER_LIMIT][SC64_P8_ACTION_LIMIT];
    int first_calls[SC64_P8_TRIGGER_LIMIT][SC64_P8_ACTION_LIMIT];
    int resume_calls[SC64_P8_TRIGGER_LIMIT][SC64_P8_ACTION_LIMIT];
    unsigned call_trigger[64], call_slot[64];
    int call_resume[64];
    unsigned call_count;
    unsigned expected_owner;
} Fixture;

static int trigger_info(void *context, unsigned trigger, Sc64P8TriggerInfo *info)
{
    Fixture *f = context;
    if (!f || !info || trigger >= SC64_P8_TRIGGER_LIMIT) return 0;
    *info = f->trigger[trigger];
    return 1;
}

static int condition(void *context, unsigned trigger, unsigned slot, unsigned owner)
{
    Fixture *f = context;
    if (!f || trigger >= SC64_P8_TRIGGER_LIMIT ||
        slot >= SC64_P8_CONDITION_LIMIT || owner != f->expected_owner) return -1;
    return f->condition_value[trigger][slot];
}

static int action(void *context, unsigned trigger, unsigned slot, unsigned owner,
                  int resume)
{
    Fixture *f = context;
    if (!f || trigger >= SC64_P8_TRIGGER_LIMIT ||
        slot >= SC64_P8_ACTION_LIMIT || owner != f->expected_owner ||
        f->call_count >= 64) return -1;
    f->call_trigger[f->call_count] = trigger;
    f->call_slot[f->call_count] = slot;
    f->call_resume[f->call_count++] = resume;
    if (resume) ++f->resume_calls[trigger][slot];
    else ++f->first_calls[trigger][slot];
    return f->action_result[trigger][slot];
}

static Sc64P8WorldOps make_ops(Fixture *f)
{
    Sc64P8WorldOps ops = {f, trigger_info, condition, action};
    return ops;
}

static Fixture fixture(void)
{
    Fixture f = {0};
    f.expected_owner = 0;
    f.trigger[0].condition_count = 1;
    f.trigger[0].action_count = 2;
    f.trigger[0].enabled_players[0] = 1;
    f.condition_value[0][0] = 1;
    f.action_result[0][0] = SC64_P8_ACTION_PENDING;
    f.action_result[0][1] = SC64_P8_ACTION_WON;
    return f;
}

int main(void)
{
    Fixture f = fixture();
    Sc64P8WorldOps ops = make_ops(&f);
    Sc64P8RuntimeState state;
    assert(sc64_p8_runtime_init(&state, 0, 1));

    f.condition_value[0][0] = 0;
    assert(sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.started == 0 && f.call_count == 0);

    f.condition_value[0][0] = 1;
    assert(sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.started == 1 && state.running == 1 && state.waiting == 1);
    assert(state.cursor[0] == 0 && f.first_calls[0][0] == 1);
    assert(sc64_p8_runtime_step(&state, &ops, 1));
    assert(f.resume_calls[0][0] == 0 && state.waiting == 1);

    f.action_result[0][0] = SC64_P8_ACTION_COMPLETE;
    assert(sc64_p8_runtime_step(&state, &ops, 0));
    assert(f.resume_calls[0][0] == 1 && f.first_calls[0][1] == 1);
    assert(state.outcome == SC64_P8_OUTCOME_WON && state.waiting == 0);

    f = fixture(); ops = make_ops(&f);
    assert(sc64_p8_runtime_init(&state, 0, 1));
    f.action_result[0][0] = -1; /* Unsupported source effect must fail closed. */
    assert(!sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.fault == SC64_P8_FAULT_ACTION && state.fault_trigger == 0 &&
           state.fault_slot == 0 && state.outcome == SC64_P8_OUTCOME_RUNNING);

    f = fixture(); ops = make_ops(&f);
    assert(sc64_p8_runtime_init(&state, 0, 1));
    f.condition_value[0][0] = -1; /* Unknown source predicate must fault closed. */
    assert(!sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.fault == SC64_P8_FAULT_CONDITION &&
           state.fault_trigger == 0 && state.fault_slot == 0 &&
           state.started == 0);

    f = fixture(); ops = make_ops(&f);
    assert(sc64_p8_runtime_init(&state, 0, 1));
    f.trigger[0].execution_flags = 1;
    assert(!sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.fault == SC64_P8_FAULT_NONE); /* Invalid program: no mutation. */

    f = fixture(); ops = make_ops(&f);
    assert(sc64_p8_runtime_init(&state, 0, 1));
    f.trigger[0].enabled_players[22] = 1;
    assert(!sc64_p8_runtime_step(&state, &ops, 0)); /* Unsupported selector fails closed. */
    assert(state.fault == SC64_P8_FAULT_PROGRAM && state.fault_trigger == 0);

    /* Native Force1/2/3 enabled-player selectors map to 18/19/20. */
    for (unsigned force = 1; force <= 3; ++force) {
        f = (Fixture){0}; f.expected_owner = 0;
        f.trigger[0].action_count = 1;
        f.trigger[0].enabled_players[17u + force] = 1;
        f.action_result[0][0] = SC64_P8_ACTION_COMPLETE;
        ops = make_ops(&f);
        assert(sc64_p8_runtime_init(&state, 0, force));
        assert(sc64_p8_runtime_step(&state, &ops, 0));
        assert(state.done == 1 && f.call_count == 1 &&
               f.call_trigger[0] == 0 && f.call_slot[0] == 0);
        unsigned wrong_force = force == 3 ? 1 : force + 1;
        f = (Fixture){0}; f.expected_owner = 0;
        f.trigger[0].action_count = 1;
        f.trigger[0].enabled_players[17u + wrong_force] = 1;
        f.action_result[0][0] = SC64_P8_ACTION_COMPLETE;
        ops = make_ops(&f);
        assert(sc64_p8_runtime_init(&state, 0, force));
        assert(sc64_p8_runtime_step(&state, &ops, 0));
        assert(state.started == 0 && f.call_count == 0);
    }

    /* A native-style owner wait stalls only timed actions that have not yet
     * been armed. Later triggers and their immediate actions still run. */
    f = (Fixture){0}; f.expected_owner = 0;
    f.trigger[0].action_count = 1; f.trigger[0].enabled_players[0] = 1;
    f.trigger[1].action_count = 2; f.trigger[1].enabled_players[18] = 1;
    f.action_result[0][0] = SC64_P8_ACTION_PENDING;
    f.action_result[1][0] = SC64_P8_ACTION_COMPLETE; /* free_run */
    f.action_result[1][1] = SC64_P8_ACTION_BLOCKED;  /* another timed op */
    ops = make_ops(&f);
    assert(sc64_p8_runtime_init(&state, 0, 1));
    assert(sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.started == 3 && state.running == 3 && state.waiting == 1 &&
           state.cursor[0] == 0 && state.cursor[1] == 1 &&
           f.call_count == 3 && f.call_trigger[0] == 0 &&
           f.call_trigger[1] == 1 && f.call_slot[1] == 0 &&
           f.call_trigger[2] == 1 && f.call_slot[2] == 1);
    f.action_result[0][0] = SC64_P8_ACTION_COMPLETE;
    f.action_result[1][1] = SC64_P8_ACTION_COMPLETE;
    assert(sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.done == 3 && state.running == 0 && state.waiting == 0 &&
           f.call_count == 5 && f.call_trigger[3] == 0 &&
           f.call_resume[3] == 1 && f.call_trigger[4] == 1 &&
           f.call_slot[4] == 1 && f.call_resume[4] == 0);

    /* Terminal actions are collected for the whole source-order pass: a
     * later Victory overrides an earlier Defeat, even after another trigger
     * yielded on a timed action. */
    f = (Fixture){0}; f.expected_owner = 0;
    f.trigger[0].action_count = 1; f.trigger[0].enabled_players[0] = 1;
    f.trigger[1].action_count = 1; f.trigger[1].enabled_players[18] = 1;
    f.trigger[2].action_count = 1; f.trigger[2].enabled_players[18] = 1;
    f.action_result[0][0] = SC64_P8_ACTION_PENDING;
    f.action_result[1][0] = SC64_P8_ACTION_LOST;
    f.action_result[2][0] = SC64_P8_ACTION_WON;
    ops = make_ops(&f);
    assert(sc64_p8_runtime_init(&state, 0, 1));
    assert(sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.outcome == SC64_P8_OUTCOME_WON && state.started == 7 &&
           state.running == 1 && state.done == 6 && state.waiting == 1 &&
           f.call_count == 3 && f.call_trigger[0] == 0 &&
           f.call_trigger[1] == 1 && f.call_trigger[2] == 2);
    return 0;
}
