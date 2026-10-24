#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_z5_adapter.h"

typedef struct {
    u8 target_trigger;
    u16 seen_conditions[SC64_Z5_TRIGGER_COUNT];
    u16 seen_actions[SC64_Z5_TRIGGER_COUNT];
    u8 last_action_index[SC64_Z5_TRIGGER_COUNT];
    u16 exact_action_count;
    u16 pause_calls;
    u8 last_world_pause;
    int fail_condition;
    int fail_action;
    int check_switches;
    int fail_pause;
} Fixture;

static int fixture_condition(void *context, const Sc64Z5RuntimeState *state,
                             const Sc64Z5SourceCondition *condition)
{
    Fixture *f = (Fixture *)context;
    assert(state && condition);
    assert(condition->trigger < SC64_Z5_TRIGGER_COUNT);
    assert(condition->index < sc64_z5_condition_count(condition->trigger));
    ++f->seen_conditions[condition->trigger];
    if (f->fail_condition) return -1;
    if (f->check_switches && condition->opcode == 11) {
        if (condition->trigger == SC64_Z5_TRIGGER_VICTORY)
            assert(condition->resource == SC64_Z5_SWITCH_VICTORY_GATE &&
                   condition->comparison == 2);
        int set = sc64_z5_switch_is_set(state, condition->resource);
        if (condition->comparison == 2) return set;
        if (condition->comparison == 3) return !set;
        return -1;
    }
    /* The fixture activates one record at a time; it does not assert a
     * campaign result or emulate source predicate truth. */
    return condition->trigger == f->target_trigger;
}

static int fixture_action(void *context, const Sc64Z5SourceAction *action)
{
    Fixture *f = (Fixture *)context;
    const Sc64Z5SourceAction *expected;
    assert(action && action->trigger < SC64_Z5_TRIGGER_COUNT);
    assert(sc64_z5_source_action(action->trigger, action->index, &expected));
    assert(expected == action);
    ++f->seen_actions[action->trigger];
    f->last_action_index[action->trigger] = action->index;
    ++f->exact_action_count;
    if (f->fail_action) return 0;
    return 1;
}

static int fixture_world_pause(void *context, int paused)
{
    Fixture *f = (Fixture *)context;
    ++f->pause_calls;
    f->last_world_pause = (u8)paused;
    return f->fail_pause ? 0 : 1;
}

static Sc64Z5WorldOps ops_for(Fixture *f)
{
    Sc64Z5WorldOps ops = {f, fixture_condition, fixture_action,
                          fixture_world_pause};
    return ops;
}

static void run_record_from_state(Sc64Z5RuntimeState *state, u8 trigger,
                                  Fixture *fixture)
{
    Sc64Z5WorldOps ops = ops_for(fixture);
    unsigned steps = 0;
    fixture->target_trigger = trigger;
    while (fixture->seen_actions[trigger] < sc64_z5_action_count(trigger)) {
        int result = sc64_z5_adapter_tick(state, &ops, 0);
        assert(result == 1);
        assert(sc64_z5_runtime_state_valid(state));
        assert(++steps < 10000);
    }
    assert(fixture->seen_actions[trigger] == sc64_z5_action_count(trigger));
}

static void run_record(Sc64Z5RuntimeState *state, u8 trigger, Fixture *fixture)
{
    (void)sc64_z5_runtime_init(state);
    run_record_from_state(state, trigger, fixture);
}

static void test_metadata_and_durations(void)
{
    assert(SC64_Z5_SOURCE_TRIGGER_COUNT == 40);
    assert(SC64_Z5_SOURCE_ACTION_COUNT == 155);
    assert(SC64_Z5_SOURCE_CONDITION_COUNT == 44);
    assert(sc64_z5_ceil_ticks(0) == 1);
    assert(sc64_z5_ceil_ticks(403) == 13);
    assert(sc64_z5_ceil_ticks(500) == 15);
    assert(sc64_z5_ceil_ticks(1000) == 30);
    assert(sc64_z5_ceil_ticks(0xffffffffu) == 36000);
    assert(sc64_z5_action_duration(0, 3) == 30);
    assert(sc64_z5_action_duration(0, 2) == 13);
    assert(sc64_z5_action_duration(1, 3) == 90);
    assert(sc64_z5_action_duration(0, 40) == 1);
    assert(sc64_z5_action_duration(37, 0) == 0);
    for (u8 t = 0; t < SC64_Z5_TRIGGER_COUNT; ++t) {
        assert(sc64_z5_source_triggers[t].action_count == sc64_z5_action_count(t));
        assert(sc64_z5_source_triggers[t].condition_count == sc64_z5_condition_count(t));
    }
    for (u16 i = 0; i < SC64_Z5_SOURCE_ACTION_COUNT; ++i) {
        const Sc64Z5SourceAction *a = &sc64_z5_source_actions[i];
        assert(a->opcode != 0 && a->trigger < SC64_Z5_TRIGGER_COUNT);
        assert(sc64_z5_action_duration(a->trigger, a->index) != 0);
    }
    {
        u8 preserved[SC64_Z5_TRIGGER_COUNT] = {0};
        for (u16 i = 0; i < SC64_Z5_SOURCE_ACTION_COUNT; ++i) {
            const Sc64Z5SourceAction *a = &sc64_z5_source_actions[i];
            if (a->opcode == 3) preserved[a->trigger] = 1;
        }
        assert(preserved[7] && preserved[8] && preserved[34]);
        for (u8 t = 0; t < SC64_Z5_TRIGGER_COUNT; ++t)
            if (t != 7 && t != 8 && t != 34) assert(!preserved[t]);
    }
    assert(sc64_z5_source_triggers[28].enabled_players[6] == 1);
    assert(sc64_z5_source_triggers[30].enabled_players[6] == 1);
    assert(sc64_z5_source_triggers[0].enabled_players[18] == 1);
}

static void test_dispatch_all_155_private_action_rows(void)
{
    Sc64Z5RuntimeState state;
    Fixture fixture;
    unsigned total = 0;
    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    assert(sc64_z5_runtime_state_valid(&state));
    for (u8 trigger = 0; trigger < SC64_Z5_TRIGGER_COUNT; ++trigger) {
        if (!sc64_z5_action_count(trigger)) continue;
        run_record(&state, trigger, &fixture);
        if (trigger == 0) assert(sc64_z5_switch_is_set(&state, 110));
        total += fixture.seen_actions[trigger];
    }
    assert(total == SC64_Z5_SOURCE_ACTION_COUNT);
    assert(fixture.exact_action_count == SC64_Z5_SOURCE_ACTION_COUNT);
    assert(fixture.pause_calls == 1 && fixture.last_world_pause == 1);
    assert(sc64_z5_runtime_state_valid(&state));
}

static void test_switch_bits_and_world_pause_clocking(void)
{
    Sc64Z5RuntimeState state;
    Sc64Z5WorldOps ops;
    Fixture fixture;
    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    ops = ops_for(&fixture);
    fixture.target_trigger = 0;
    /* Start trigger 0, then verify user pause freezes action timer and callbacks. */
    assert(sc64_z5_adapter_tick(&state, &ops, 0) == 1);
    {
        Sc64Z5RuntimeState snapshot = state;
        u16 condition_calls = fixture.seen_conditions[0];
        u16 action_calls = fixture.seen_actions[0];
        assert(sc64_z5_adapter_tick(&state, &ops, 1) == 0);
        assert(memcmp(&snapshot, &state, sizeof(state)) == 0);
        assert(fixture.seen_conditions[0] == condition_calls);
        assert(fixture.seen_actions[0] == action_calls);
    }
    /* Source PauseGame freezes the external world, but the sequence timers
     * continue and the authored record reaches its final switch action. */
    while (fixture.seen_actions[0] < sc64_z5_action_count(0))
        assert(sc64_z5_adapter_tick(&state, &ops, 0) == 1);
    assert(fixture.pause_calls == 1 && fixture.last_world_pause == 1);
    assert(sc64_z5_switch_is_set(&state, 110));

    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    ops = ops_for(&fixture);
    fixture.target_trigger = 3;
    run_record(&state, 3, &fixture);
    assert(sc64_z5_switch_is_set(&state, 1));

    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    state.switch_bits[0] = 1u << 1;
    ops = ops_for(&fixture);
    fixture.target_trigger = 10;
    run_record_from_state(&state, 10, &fixture);
    assert(!sc64_z5_switch_is_set(&state, 1));

    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    ops = ops_for(&fixture);
    fixture.target_trigger = 26;
    run_record(&state, 26, &fixture);
    assert(sc64_z5_switch_is_set(&state, 4));
}

static void test_user_pause_and_save_resume_mid_wait(void)
{
    Sc64Z5RuntimeState state, resumed;
    Sc64Z5WorldOps ops;
    Fixture fixture;
    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    ops = ops_for(&fixture);
    fixture.target_trigger = 0;
    while (fixture.last_action_index[0] < 3 || !fixture.seen_actions[0])
        assert(sc64_z5_adapter_tick(&state, &ops, 0) == 1);
    assert(fixture.last_action_index[0] == 3);
    assert(state.action_ticks[0] == 30);
    resumed = state;
    {
        Sc64Z5RuntimeState before = resumed;
        assert(sc64_z5_adapter_tick(&resumed, &ops, 1) == 0);
        assert(memcmp(&before, &resumed, sizeof(resumed)) == 0);
    }
    assert(sc64_z5_adapter_tick(&resumed, &ops, 0) == 1);
    assert(resumed.action_ticks[0] == 29);
    assert(sc64_z5_runtime_state_valid(&resumed));
}

static void test_failure_and_state_corruption_fail_closed(void)
{
    Sc64Z5RuntimeState state;
    Sc64Z5WorldOps ops;
    Fixture fixture;
    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    ops = ops_for(&fixture);
    fixture.fail_condition = 1;
    assert(sc64_z5_adapter_tick(&state, &ops, 0) == -SC64_Z5_FAULT_CONDITION);
    assert(state.fault == SC64_Z5_FAULT_CONDITION);
    assert(!sc64_z5_runtime_state_valid(&state));

    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    ops = ops_for(&fixture);
    fixture.target_trigger = 4;
    fixture.fail_action = 1;
    assert(sc64_z5_adapter_tick(&state, &ops, 0) == -SC64_Z5_FAULT_ACTION);
    assert(state.fault == SC64_Z5_FAULT_ACTION);

    (void)sc64_z5_runtime_init(&state);
    state.model.started_lo = 1u;
    state.model.action_cursor[0] = 1;
    state.action_ticks[0] = 1;
    assert(sc64_z5_runtime_state_valid(&state));
    state.action_ticks[0] = 2; /* Action zero has a one-tick limit. */
    assert(!sc64_z5_runtime_state_valid(&state));
    assert(sc64_z5_adapter_tick(&state, &ops, 0) == -SC64_Z5_FAULT_STATE);
    assert(state.fault == SC64_Z5_FAULT_STATE);

    (void)sc64_z5_runtime_init(&state);
    state.switch_bits[7] = 1;
    assert(!sc64_z5_runtime_state_valid(&state));

    (void)sc64_z5_runtime_init(&state);
    state.source_paused = 1;
    assert(!sc64_z5_runtime_state_valid(&state)); /* Pause was never reached. */
    (void)sc64_z5_runtime_init(&state);
    state.model.started_lo = 1u;
    state.model.action_cursor[0] = 1;
    state.action_ticks[0] = 1;
    state.source_paused = 1;
    assert(!sc64_z5_runtime_state_valid(&state)); /* Cursor precedes PauseGame. */

    (void)sc64_z5_runtime_init(&state);
    assert(sc64_z5_adapter_tick(&state, NULL, 0) == -SC64_Z5_FAULT_ACTION);
    assert(state.fault == SC64_Z5_FAULT_ACTION);
}

static void test_condition_error_and_failed_pause_callback(void)
{
    Sc64Z5RuntimeState state;
    Sc64Z5WorldOps ops;
    Fixture fixture;
    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    ops = ops_for(&fixture);
    fixture.target_trigger = 0;
    fixture.fail_action = 0;
    /* First start and execute through the source pause request. */
    while (fixture.seen_actions[0] <= 6)
        assert(sc64_z5_adapter_tick(&state, &ops, 0) == 1);
    assert(state.source_paused);
    /* A condition resolver error fails closed and poisons save state. */
    fixture.fail_condition = 1;
    assert(sc64_z5_adapter_tick(&state, &ops, 0) == -SC64_Z5_FAULT_CONDITION);
    assert(!sc64_z5_runtime_state_valid(&state));

    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    ops = ops_for(&fixture);
    fixture.target_trigger = 0;
    fixture.fail_pause = 1;
    while (fixture.seen_actions[0] < 7) {
        int result = sc64_z5_adapter_tick(&state, &ops, 0);
        if (fixture.seen_actions[0] < 7) assert(result == 1);
        else assert(result == -SC64_Z5_FAULT_PAUSE);
    }
    assert(state.fault == SC64_Z5_FAULT_PAUSE);
    assert(!sc64_z5_runtime_state_valid(&state));
}

static void test_switch_predicate_and_source_order_handoff(void)
{
    Sc64Z5RuntimeState state;
    Sc64Z5WorldOps ops;
    Fixture fixture;
    memset(&fixture, 0, sizeof(fixture));
    (void)sc64_z5_runtime_init(&state);
    ops = ops_for(&fixture);
    fixture.target_trigger = SC64_Z5_TRIGGER_VICTORY;
    fixture.check_switches = 1;
    /* The exact switch condition is false until authored trigger 0 sets it. */
    assert(sc64_z5_adapter_tick(&state, &ops, 0) == 1);
    assert(fixture.seen_actions[SC64_Z5_TRIGGER_VICTORY] == 0);
    fixture.target_trigger = 0;
    run_record_from_state(&state, 0, &fixture);
    assert(sc64_z5_switch_is_set(&state, SC64_Z5_SWITCH_VICTORY_GATE));
    fixture.target_trigger = SC64_Z5_TRIGGER_VICTORY;
    while (fixture.seen_actions[SC64_Z5_TRIGGER_VICTORY] <
           sc64_z5_action_count(SC64_Z5_TRIGGER_VICTORY))
        assert(sc64_z5_adapter_tick(&state, &ops, 0) == 1);
    assert(state.model.outcome == SC64_Z5_RUNNING);
    assert(sc64_z5_adapter_tick(&state, &ops, 0) == 1);
    assert(state.model.outcome == SC64_Z5_WON);
    assert(sc64_z5_adapter_tick(&state, &ops, 0) == 0);
    assert(fixture.seen_conditions[SC64_Z5_TRIGGER_VICTORY] > 0);
}

int main(void)
{
    test_metadata_and_durations();
    test_dispatch_all_155_private_action_rows();
    test_switch_bits_and_world_pause_clocking();
    test_user_pause_and_save_resume_mid_wait();
    test_failure_and_state_corruption_fail_closed();
    test_condition_error_and_failed_pause_callback();
    test_switch_predicate_and_source_order_handoff();
    return 0;
}
