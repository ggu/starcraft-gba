#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_p5_adapter.h"

typedef struct {
    u8 target;
    u8 action_count[SC64_P5_TRIGGER_COUNT];
    u8 ai_count[SC64_P5_TRIGGER_COUNT];
    u8 condition_count[SC64_P5_TRIGGER_COUNT];
    u16 exact_rows;
    u8 fail_action;
    u8 fail_ai;
    u8 fail_condition;
} Fixture;

static int fixture_condition(void *context, const Sc64P5RuntimeState *state,
                             const Sc64P5SourceCondition *condition)
{
    Fixture *f = (Fixture *)context;
    assert(state && condition && condition->trigger < SC64_P5_TRIGGER_COUNT);
    ++f->condition_count[condition->trigger];
    if (f->fail_condition) return -1;
    if (condition->opcode == 11) {
        int is_set;
        assert(condition->trigger == 3 || condition->trigger == 8);
        assert(condition->resource == 0 || condition->resource == 4);
        is_set = sc64_p5_switch_is_set(state, condition->resource);
        if (f->target == condition->trigger) return 1;
        if (condition->comparison == 2) return is_set;
        if (condition->comparison == 3) return !is_set;
        return -1;
    }
    return condition->trigger == f->target;
}

static int fixture_action(void *context, const Sc64P5SourceAction *action)
{
    Fixture *f = (Fixture *)context;
    const Sc64P5SourceAction *expected;
    assert(action && sc64_p5_source_action(action->trigger, action->index, &expected));
    assert(action == expected);
    assert(action->opcode != 15 && action->opcode != 16);
    ++f->action_count[action->trigger];
    ++f->exact_rows;
    return f->fail_action ? 0 : 1;
}

static int fixture_opaque_ai(void *context, const Sc64P5SourceAction *action)
{
    Fixture *f = (Fixture *)context;
    const Sc64P5SourceAction *expected;
    assert(action && sc64_p5_source_action(action->trigger, action->index, &expected));
    assert(action == expected && (action->opcode == 15 || action->opcode == 16));
    ++f->ai_count[action->trigger];
    ++f->exact_rows;
    return f->fail_ai ? 0 : 1;
}

static Sc64P5WorldOps ops_for(Fixture *f)
{
    Sc64P5WorldOps ops = {f, fixture_condition, fixture_action, fixture_opaque_ai};
    return ops;
}

static void run_one_record(u8 trigger, Fixture *f)
{
    Sc64P5RuntimeState state;
    Sc64P5WorldOps ops = ops_for(f);
    unsigned ticks = 0;
    (void)sc64_p5_runtime_init(&state);
    f->target = trigger;
    while ((unsigned)f->action_count[trigger] + f->ai_count[trigger] <
           sc64_p5_action_count(trigger)) {
        int result = sc64_p5_adapter_tick(&state, &ops, 0);
        assert(result == 1);
        assert(sc64_p5_runtime_state_valid(&state));
        assert(++ticks < 5000);
    }
}

static void test_staged_metadata_and_dispatch(void)
{
    Fixture f;
    memset(&f, 0, sizeof(f));
    assert(SC64_P5_SOURCE_GLOBAL_ID == 24);
    assert(SC64_P5_SOURCE_TRIGGER_COUNT == 11);
    assert(SC64_P5_SOURCE_ACTION_COUNT == 37);
    assert(SC64_P5_SOURCE_CONDITION_COUNT == 12);
    assert(sc64_p5_ceil_ticks(0) == 1);
    assert(sc64_p5_ceil_ticks(403) == 13);
    assert(sc64_p5_ceil_ticks(500) == 15);
    assert(sc64_p5_ceil_ticks(0xffffffffu) == 36000);
    assert(sc64_p5_action_duration(1, 0) == 30);
    assert(sc64_p5_action_duration(1, 1) == 13);
    assert(sc64_p5_action_duration(4, 1) == 15);
    assert(sc64_p5_action_duration(4, 2) == 1);
    assert(sc64_p5_action_duration(10, 1) == 1); /* opaque AI row */

    for (u8 t = 0; t < SC64_P5_TRIGGER_COUNT; ++t) {
        assert(sc64_p5_source_triggers[t].action_count == sc64_p5_action_count(t));
        assert(sc64_p5_source_triggers[t].condition_count == sc64_p5_condition_count(t));
    }
    for (u16 i = 0; i < SC64_P5_SOURCE_ACTION_COUNT; ++i) {
        const Sc64P5SourceAction *a = &sc64_p5_source_actions[i];
        assert(sc64_p5_action_duration(a->trigger, a->index) != 0);
    }

    for (u8 t = 0; t < SC64_P5_TRIGGER_COUNT; ++t) run_one_record(t, &f);
    assert(f.exact_rows == SC64_P5_SOURCE_ACTION_COUNT);
    assert(f.ai_count[8] == 1 && f.ai_count[9] == 1 && f.ai_count[10] == 1);
    assert(f.action_count[8] == 0 && f.action_count[9] == 1 && f.action_count[10] == 1);
}

static void test_switch_state_and_pause_resume(void)
{
    Sc64P5RuntimeState s, saved;
    Sc64P5WorldOps ops;
    Fixture f;
    memset(&f, 0, sizeof(f));
    (void)sc64_p5_runtime_init(&s);
    f.target = 5;
    ops = ops_for(&f);
    assert(sc64_p5_adapter_tick(&s, &ops, 0) == 1);
    assert(sc64_p5_switch_is_set(&s, 0));
    /* Trigger 3 waits on source switch 0. It may start only after trigger 5. */
    f.target = 3;
    assert(sc64_p5_adapter_tick(&s, &ops, 0) == 1);
    assert(f.action_count[3] == 1);
    while (f.action_count[3] < 17) assert(sc64_p5_adapter_tick(&s, &ops, 0) == 1);
    assert(sc64_p5_switch_is_set(&s, 4));
    assert(sc64_p5_runtime_state_valid(&s));

    saved = s;
    {
        Sc64P5RuntimeState before = saved;
        unsigned conditions = f.condition_count[3];
        assert(sc64_p5_adapter_tick(&saved, &ops, 1) == 0);
        assert(memcmp(&before, &saved, sizeof(saved)) == 0);
        assert(f.condition_count[3] == conditions);
    }
    /* The large source wait remains pending across a byte-exact save copy. */
    assert(saved.action_ticks[3] == sc64_p5_ceil_ticks(3000));
    assert(sc64_p5_adapter_tick(&saved, &ops, 0) == 1);
    assert(saved.action_ticks[3] + 1 == sc64_p5_ceil_ticks(3000));
    assert(sc64_p5_runtime_state_valid(&saved));

    (void)sc64_p5_runtime_init(&s);
    s.switch_bits[0] = 1u << 1;
    assert(!sc64_p5_runtime_state_valid(&s));
}

static void test_fail_closed_callbacks_and_invalid_inputs(void)
{
    Sc64P5RuntimeState s;
    Sc64P5WorldOps ops;
    Fixture f;
    memset(&f, 0, sizeof(f));
    (void)sc64_p5_runtime_init(&s);
    ops = ops_for(&f);
    f.target = 8;
    f.fail_ai = 1;
    assert(sc64_p5_adapter_tick(&s, &ops, 0) == -SC64_P5_FAULT_AI);
    assert(!sc64_p5_runtime_state_valid(&s));

    memset(&f, 0, sizeof(f));
    (void)sc64_p5_runtime_init(&s);
    ops = ops_for(&f);
    ops.opaque_ai = NULL;
    f.target = 8;
    assert(sc64_p5_adapter_tick(&s, &ops, 0) == -SC64_P5_FAULT_AI);

    memset(&f, 0, sizeof(f));
    (void)sc64_p5_runtime_init(&s);
    ops = ops_for(&f);
    f.target = 2;
    f.fail_action = 1;
    assert(sc64_p5_adapter_tick(&s, &ops, 0) == -SC64_P5_FAULT_ACTION);

    memset(&f, 0, sizeof(f));
    (void)sc64_p5_runtime_init(&s);
    ops = ops_for(&f);
    f.fail_condition = 1;
    assert(sc64_p5_adapter_tick(&s, &ops, 0) == -SC64_P5_FAULT_CONDITION);

    (void)sc64_p5_runtime_init(&s);
    s.model.started = 1u << 0;
    s.model.outcome = SC64_P5_WON;
    assert(!sc64_p5_state_valid(&s.model));
    assert(sc64_p5_adapter_tick(&s, &ops, 0) == -SC64_P5_FAULT_STATE);
}

int main(void)
{
    test_staged_metadata_and_dispatch();
    test_switch_state_and_pause_resume();
    test_fail_closed_callbacks_and_invalid_inputs();
    return 0;
}
