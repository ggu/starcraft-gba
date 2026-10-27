/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_p8_runtime.h"
#include <assert.h>

typedef struct {
    Sc64P8TriggerInfo trigger[SC64_P8_TRIGGER_LIMIT];
    int condition_value;
    int action0_first, action0_resume, action1;
    int action_result;
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
    return f && trigger == 0 && slot == 0 && owner == 0 ?
        f->condition_value : -1;
}

static int action(void *context, unsigned trigger, unsigned slot, unsigned owner,
                  int resume)
{
    Fixture *f = context;
    if (!f || trigger != 0 || owner != 0) return -1;
    if (slot == 0) {
        if (resume) ++f->action0_resume;
        else ++f->action0_first;
        return f->action_result;
    }
    if (slot == 1 && !resume) { ++f->action1; return SC64_P8_ACTION_WON; }
    return -1;
}

static Sc64P8WorldOps make_ops(Fixture *f)
{
    Sc64P8WorldOps ops = {f, trigger_info, condition, action};
    return ops;
}

static Fixture fixture(void)
{
    Fixture f = {0};
    f.trigger[0].condition_count = 1;
    f.trigger[0].action_count = 2;
    f.trigger[0].enabled_players[0] = 1;
    f.condition_value = 1;
    f.action_result = SC64_P8_ACTION_PENDING;
    return f;
}

int main(void)
{
    Fixture f = fixture();
    Sc64P8WorldOps ops = make_ops(&f);
    Sc64P8RuntimeState state;
    assert(sc64_p8_runtime_init(&state, 0, 1));

    f.condition_value = 0;
    assert(sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.started == 0 && f.action0_first == 0);

    f.condition_value = 1;
    assert(sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.started == 1 && state.running == 1 && state.waiting == 1);
    assert(state.cursor[0] == 0 && f.action0_first == 1);
    assert(sc64_p8_runtime_step(&state, &ops, 1));
    assert(f.action0_resume == 0 && state.waiting == 1);

    f.action_result = SC64_P8_ACTION_COMPLETE;
    assert(sc64_p8_runtime_step(&state, &ops, 0));
    assert(f.action0_resume == 1 && f.action1 == 1);
    assert(state.outcome == SC64_P8_OUTCOME_WON && state.waiting == 0);

    f = fixture(); ops = make_ops(&f);
    assert(sc64_p8_runtime_init(&state, 0, 1));
    f.action_result = -1; /* Unsupported source effect must fail closed. */
    assert(!sc64_p8_runtime_step(&state, &ops, 0));
    assert(state.fault == SC64_P8_FAULT_ACTION && state.fault_trigger == 0 &&
           state.fault_slot == 0 && state.outcome == SC64_P8_OUTCOME_RUNNING);

    f = fixture(); ops = make_ops(&f);
    assert(sc64_p8_runtime_init(&state, 0, 1));
    f.condition_value = -1; /* Unknown source predicate must fault closed. */
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
    return 0;
}
