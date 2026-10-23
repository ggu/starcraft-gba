/* Synthetic regression tests for the late Terran trigger scheduler. This is
 * deliberately not a fixture containing any imported mission operands. */
#include <assert.h>
#include <string.h>
#define SC64_T4_RUNTIME 1
#define SC64_TERRAN_LATE_RUNTIME 1
#include "../starcraft/sc64_terran_late_runtime.h"

typedef struct {
    u32 count_value;
    unsigned count_calls, action_calls, applied_actions;
    unsigned retry_once;
    int count_result, action_result;
    unsigned action_owner;
    unsigned action_opcode;
} TestWorld;

static int
test_count(void *context, const Sc64T4Condition *condition,
           unsigned owner, u32 *count)
{
    TestWorld *world = context;
    assert(world && condition && count && owner < 8);
    ++world->count_calls;
    if (world->count_result != 0) return world->count_result;
    *count = world->count_value;
    return 1;
}

static int
test_action(void *context, const Sc64T4Action *action, unsigned owner)
{
    TestWorld *world = context;
    assert(world && action && owner < 8);
    ++world->action_calls;
    world->action_owner = owner;
    world->action_opcode = action->opcode;
    if (world->action_result) return world->action_result;
    if (world->retry_once) {
        --world->retry_once;
        return 0;
    }
    ++world->applied_actions;
    return 1;
}

static const Sc64TerranLateRuntimeOps ops = {test_count, test_action};

typedef struct {
    Sc64T4Trigger triggers[12];
    Sc64T4Condition conditions[12][16];
    Sc64T4Action actions[12][64];
    Sc64T4Scenario program;
    Sc64T4SourceSetup setup;
} Fixture;

static void
fixture_init(Fixture *f, unsigned n)
{
    memset(f, 0, sizeof(*f));
    assert(n >= 1 && n <= 12);
    f->program.triggers = f->triggers;
    f->program.trigger_count = (u16)n;
    for (unsigned i = 0; i < n; ++i) {
        f->triggers[i].conditions = f->conditions[i];
        f->triggers[i].actions = f->actions[i];
        f->triggers[i].enabled_players[0] = 1;
    }
    /* Distinct, valid one-based force assignments for all source slots. */
    for (unsigned i = 0; i < 8; ++i) f->setup.force_by_slot[i] = (u8)(i & 3);
}

static void
test_native_comparators_and_callback_boundary(void)
{
    Fixture f;
    Sc64TerranLateRuntimeState state;
    TestWorld world = {.count_value = 9};
    fixture_init(&f, 1);
    f.conditions[0][0].opcode = 2;
    f.conditions[0][0].comparison = 10;
    f.conditions[0][0].amount = 9;
    f.actions[0][0].opcode = 7;
    f.actions[0][1].opcode = 1;
    assert(sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
    assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                         &ops, &world, 0));
    assert(world.count_calls == 1 && world.action_calls == 1);
    assert(world.action_opcode == 7 && world.action_owner == 0);
    assert(state.player[0].outcome == SC64_TERRAN_LATE_WON);

    /* Comparison value 2 belongs to a different native schema and must not
     * silently become equality in this late-map runner. */
    f.conditions[0][0].comparison = 2;
    assert(!sc64_terran_late_program_valid(&f.program));
    assert(!sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
    f.conditions[0][0].comparison = 10;
    world.count_result = -1;
    assert(sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
    assert(!sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                          &ops, &world, 0));
    assert(state.player[0].fault == SC64_TERRAN_LATE_CONDITION_FAULT);
}

static void
test_shared_switch_force_expansion_and_time(void)
{
    Fixture f;
    Sc64TerranLateRuntimeState state;
    TestWorld world = {0};
    fixture_init(&f, 2);
    f.triggers[0].enabled_players[0] = 0;
    f.triggers[0].enabled_players[18] = 1; /* Force 1. */
    f.actions[0][0].opcode = 13;
    f.actions[0][0].argument = 255;
    f.actions[0][0].modifier = 4;
    f.actions[0][1].opcode = 0;
    f.conditions[1][0].opcode = 11;
    f.conditions[1][0].comparison = 2;
    f.conditions[1][0].resource = 255;
    f.actions[1][0].opcode = 12;
    f.triggers[1].enabled_players[0] = 1;
    assert(sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
    for (unsigned i = 0; i < 30; ++i)
        assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                             &ops, &world, 0));
    assert(state.switches.bits[31] & 0x80);
    assert(state.player[0].flags[1] == SC64_T4_TR_DONE);

    /* ElapsedTime uses seconds and native comparator 10 for exact equality. */
    memset(f.conditions[1], 0, sizeof(f.conditions[1]));
    f.conditions[1][0].opcode = 12;
    f.conditions[1][0].comparison = 10;
    f.conditions[1][0].amount = 0;
    assert(sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
    assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                         &ops, &world, 0));
    assert(state.player[0].flags[1] == SC64_T4_TR_DONE);
}

static void
test_switch_bank_is_shared_across_source_players(void)
{
    Fixture f;
    Sc64TerranLateRuntimeState state;
    TestWorld world = {0};
    fixture_init(&f, 2);
    f.triggers[0].enabled_players[0] = 0;
    f.triggers[0].enabled_players[4] = 1;
    f.actions[0][0].opcode = 13;
    f.actions[0][0].argument = 254;
    f.actions[0][0].modifier = 4;
    f.triggers[1].enabled_players[4] = 0;
    f.conditions[1][0].opcode = 11;
    f.conditions[1][0].comparison = 2;
    f.conditions[1][0].resource = 254;
    f.actions[1][0].opcode = 12;
    assert(sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
    assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                         &ops, &world, 0));
    assert(state.switches.bits[31] & 0x40);
    assert(world.action_calls == 0);
    for (unsigned tick = 0; tick < 30; ++tick)
        assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                             &ops, &world, 0));
    assert(world.action_calls == 1 && world.action_owner == 0);
}

static void
test_wait_pause_retry_and_preserve(void)
{
    Fixture f;
    Sc64TerranLateRuntimeState state, before;
    TestWorld world = {.retry_once = 1};
    fixture_init(&f, 1);
    f.conditions[0][0].opcode = 22; /* Always. */
    f.actions[0][0].opcode = 3;      /* Preserve. */
    f.actions[0][1].opcode = 4;
    f.actions[0][1].time = 1000;
    f.actions[0][2].opcode = 7;
    f.actions[0][3].opcode = 2;
    assert(sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
    assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                         &ops, &world, 0));
    assert(state.player[0].wait_ticks == 30);
    assert(state.player[0].flags[0] == (SC64_T4_TR_RUNNING|SC64_T4_TR_PRESERVE|
                                        SC64_T4_TR_WAIT));
    before = state;
    assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                         &ops, &world, 1));
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    for (unsigned i = 0; i < 29; ++i)
        assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                             &ops, &world, 0));
    assert(state.player[0].wait_ticks == 1 && world.action_calls == 0);
    assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                         &ops, &world, 0));
    /* Wait resumes at the action after it, without condition re-evaluation. */
    assert(world.action_calls == 1 && world.applied_actions == 0);
    assert(state.player[0].outcome == 0);
    for (unsigned i = 0; i < 29; ++i)
        assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                             &ops, &world, 0));
    assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                         &ops, &world, 0));
    assert(world.action_calls == 2 && world.applied_actions == 1);
    assert(state.player[0].outcome == SC64_TERRAN_LATE_LOST);
    assert(state.player[0].flags[0] == 0); /* Preserve clears the instance. */
}

static void
test_disabled_entries_and_unsupported_faults(void)
{
    Fixture f;
    Sc64TerranLateRuntimeState state;
    TestWorld world = {0};
    fixture_init(&f, 1);
    f.conditions[0][0].opcode = 250;
    f.conditions[0][0].flags = 2;
    f.actions[0][0].opcode = 250;
    f.actions[0][0].flags = 2;
    f.actions[0][1].opcode = 1;
    assert(sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
    assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                         &ops, &world, 0));
    assert(state.player[0].outcome == SC64_TERRAN_LATE_WON);
    assert(world.count_calls == 0 && world.action_calls == 0);

    f.conditions[0][0].flags = 0;
    assert(!sc64_terran_late_program_valid(&f.program));

    memset(f.conditions[0], 0, sizeof(f.conditions[0]));
    f.conditions[0][0].opcode = 22;
    f.actions[0][0].flags = 0;
    assert(!sc64_terran_late_program_valid(&f.program));

    memset(f.actions[0], 0, sizeof(f.actions[0]));
    f.actions[0][0].opcode = 26;
    world.action_result = -1;
    assert(sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
    assert(!sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                          &ops, &world, 0));
    assert(state.player[0].fault == SC64_TERRAN_LATE_ACTION_FAULT);
}

static void
test_callback_retry_is_atomic_and_program_bound_is_audited(void)
{
    Fixture f;
    Sc64TerranLateRuntimeState state;
    TestWorld world = {.retry_once = 1};
    fixture_init(&f, 12);
    f.conditions[0][0].opcode = 22;
    f.actions[0][0].opcode = 26;
    assert(sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
    assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                         &ops, &world, 0));
    assert(world.action_calls == 1 && world.applied_actions == 0);
    assert(state.player[0].cursor[0] == 0 &&
           (state.player[0].flags[0] & SC64_T4_TR_RUNNING));
    for (unsigned i = 0; i < 30; ++i)
        assert(sc64_terran_late_runtime_step(&state, &f.program, &f.setup,
                                             &ops, &world, 0));
    assert(world.action_calls == 2 && world.applied_actions == 1);
    assert(state.player[0].flags[0] == SC64_T4_TR_DONE);

    f.program.trigger_count = 13;
    assert(!sc64_terran_late_program_valid(&f.program));
    assert(!sc64_terran_late_runtime_init(&state, &f.program, &f.setup));
}

static void
test_checked_and_prevalidated_paths_match(void)
{
    Fixture f;
    Sc64TerranLateRuntimeState checked, fast;
    TestWorld checked_world = {0}, fast_world = {0};
    fixture_init(&f, 3);
    f.actions[0][0].opcode = 13;
    f.actions[0][0].argument = 127;
    f.actions[0][0].modifier = 4;
    f.conditions[1][0].opcode = 11;
    f.conditions[1][0].comparison = 2;
    f.conditions[1][0].resource = 127;
    f.actions[1][0].opcode = 26;
    f.conditions[2][0].opcode = 2;
    f.conditions[2][0].comparison = 0;
    f.conditions[2][0].amount = 0;
    f.actions[2][0].opcode = 4;
    f.actions[2][0].time = 0;
    f.actions[2][1].opcode = 7;
    assert(sc64_terran_late_runtime_init(&checked, &f.program, &f.setup));
    fast = checked;
    for (unsigned tick = 0; tick < 70; ++tick) {
        checked_world.count_value = fast_world.count_value = tick / 3;
        assert(sc64_terran_late_runtime_step(
            &checked, &f.program, &f.setup, &ops, &checked_world, 0));
        assert(sc64_terran_late_runtime_step_prevalidated(
            &fast, &f.program, &f.setup, &ops, &fast_world, 0));
        assert(memcmp(&checked, &fast, sizeof(checked)) == 0);
        assert(memcmp(&checked_world, &fast_world, sizeof(checked_world)) == 0);
    }

    /* The fast entry still refuses a corrupt scalar before indexing any
     * mission table. */
    fast.player[0].active_wait = 250;
    assert(!sc64_terran_late_runtime_step_prevalidated(
        &fast, &f.program, &f.setup, &ops, &fast_world, 0));
}

static void
test_full_and_prevalidated_validation_boundaries(void)
{
    Fixture f;
    Sc64TerranLateRuntimeState full, fast, before;
    TestWorld full_world = {0}, fast_world = {0};
    fixture_init(&f, 1);
    f.conditions[0][0].opcode = 22;
    f.actions[0][0].opcode = 1;
    assert(sc64_terran_late_runtime_init(&full, &f.program, &f.setup));
    fast = full;

    /* The checked entry fully validates immutable program data every call. */
    f.conditions[0][0].opcode = 250;
    before = full;
    assert(!sc64_terran_late_runtime_step(&full, &f.program, &f.setup,
                                          &ops, &full_world, 0));
    assert(memcmp(&full, &before, sizeof(full)) == 0);
    f.conditions[0][0].opcode = 22;

    /* Both entries retain per-frame scalar validation. */
    full.player[0].cooldown = SC64_TERRAN_LATE_PASS_TICKS;
    fast.player[0].cooldown = SC64_TERRAN_LATE_PASS_TICKS;
    before = full;
    assert(!sc64_terran_late_runtime_step(&full, &f.program, &f.setup,
                                          &ops, &full_world, 0));
    assert(memcmp(&full, &before, sizeof(full)) == 0);
    assert(!sc64_terran_late_runtime_step_prevalidated(
        &fast, &f.program, &f.setup, &ops, &fast_world, 0));

    /* Prevalidated mode checks mutable slots before a ready pass uses them.
     * A not-ready tick defers that slot check; the next ready tick rejects it
     * before evaluating a condition or dispatching an action. */
    assert(sc64_terran_late_runtime_init(&fast, &f.program, &f.setup));
    fast.player[0].flags[0] = 0x80;
    fast.player[0].cooldown = 1;
    assert(sc64_terran_late_runtime_step_prevalidated(
        &fast, &f.program, &f.setup, &ops, &fast_world, 0));
    assert(fast.player[0].cooldown == 0);
    unsigned count_calls = fast_world.count_calls;
    unsigned action_calls = fast_world.action_calls;
    assert(!sc64_terran_late_runtime_step_prevalidated(
        &fast, &f.program, &f.setup, &ops, &fast_world, 0));
    assert(fast_world.count_calls == count_calls &&
           fast_world.action_calls == action_calls);

    /* Full validation rejects the same bad slot before changing tick state. */
    assert(sc64_terran_late_runtime_init(&full, &f.program, &f.setup));
    full.player[0].flags[0] = 0x80;
    before = full;
    assert(!sc64_terran_late_runtime_step(&full, &f.program, &f.setup,
                                          &ops, &full_world, 0));
    assert(memcmp(&full, &before, sizeof(full)) == 0);
}

int
main(void)
{
    test_native_comparators_and_callback_boundary();
    test_shared_switch_force_expansion_and_time();
    test_switch_bank_is_shared_across_source_players();
    test_wait_pause_retry_and_preserve();
    test_disabled_entries_and_unsupported_faults();
    test_callback_retry_is_atomic_and_program_bound_is_audited();
    test_checked_and_prevalidated_paths_match();
    test_full_and_prevalidated_validation_boundaries();
    return 0;
}
