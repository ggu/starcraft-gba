/* Asset-free tests of the reused trigger core and T6 typed callback bridge.
 * This invented program is not a source scenario or a mission-world proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_t6_runtime.h"

#if !defined(SC64_T6_RUNTIME) || !defined(SC64_T4_RUNTIME)
#error "Compile with SC64_T6_RUNTIME and SC64_T4_RUNTIME"
#endif

typedef struct {
    u32 deaths;
    unsigned count_calls, creates, attempts, retries;
    unsigned resource_calls, ai_calls, presentation_calls;
    int fail_create;
} World;

static int count(void *opaque, const Sc64T4Condition *condition,
                 unsigned owner, u32 *result)
{
    World *world = opaque;
    assert(world && owner == 0 && condition->opcode == 15);
    assert(condition->player == 1 && condition->unit == 231);
    ++world->count_calls;
    *result = world->deaths;
    return 1;
}

static int create(void *opaque, const Sc64T4Action *action, unsigned owner)
{
    World *world = opaque;
    assert(world && owner == 0 && action->opcode == 11);
    assert(action->player == 4 && action->unit == 37 && action->argument == 1);
    ++world->attempts;
    if (world->fail_create) return -1;
    if (world->retries) { --world->retries; return 0; }
    ++world->creates;
    return 1;
}

static int resources(void *opaque, const Sc64T4Action *action, unsigned owner)
{
    World *world = opaque;
    assert(world && owner == 0 && action->opcode == 26);
    ++world->resource_calls;
    return 1;
}

static int ai(void *opaque, const Sc64T4Action *action, unsigned owner)
{
    World *world = opaque;
    assert(world && owner == 0 && action->opcode == 15);
    ++world->ai_calls;
    return 1;
}

static int presentation(void *opaque, const Sc64T4Action *action, unsigned owner)
{
    World *world = opaque;
    assert(world && owner == 0 && action->opcode == 10);
    ++world->presentation_calls;
    return 1;
}

static const Sc64T6RuntimeOps ops = {count, create, resources, ai, presentation};
static const Sc64T4TriggerOps bridge = {
    sc64_t6_dispatch_count, sc64_t6_dispatch_action
};

static void typed_deaths_and_terminal_priority(void)
{
    Sc64T4Condition conditions[16] = {{
        .opcode = 15, .player = 1, .unit = 231, .comparison = 2, .amount = 1
    }};
    Sc64T4Action actions[64] = {{.opcode = 1}, {.opcode = 2}};
    Sc64T4Trigger trigger = {.conditions = conditions, .actions = actions,
                             .enabled_players = {[0] = 1}};
    Sc64T4Scenario program = {.triggers = &trigger, .trigger_count = 1};
    Sc64T4TriggerState state;
    Sc64T4TriggerSwitches switches = {{0}};
    World world = {0};
    Sc64T6Dispatch dispatch = {&ops, &world};
    assert(sc64_t4_trigger_init(&state, &program, 0, 1));
    assert(sc64_t4_trigger_step(&state, &program, &switches, &bridge, &dispatch, 0));
    assert(world.count_calls == 1 && !state.outcome);
    Sc64T4TriggerState snapshot = state;
    assert(sc64_t4_trigger_step(&state, &program, &switches, &bridge, &dispatch, 1));
    assert(memcmp(&state, &snapshot, sizeof(state)) == 0);
    assert(world.count_calls == 1);
    world.deaths = 1;
    for (unsigned tick = 0; tick < 30; ++tick)
        assert(sc64_t4_trigger_step(&state, &program, &switches,
                                   &bridge, &dispatch, 0));
    assert(state.outcome == SC64_T4_TR_LOST); /* Same-pass defeat dominates. */
    assert(world.count_calls == 2 && state.flags[0] == SC64_T4_TR_DONE);
}

static void wait_snapshot_and_atomic_retry(void)
{
    Sc64T4Condition conditions[16] = {{0}};
    Sc64T4Action actions[64] = {
        {.opcode = 4, .time = 500},
        {.opcode = 11, .location = 2, .player = 4, .argument = 1,
         .unit = 37, .flags = 24}
    };
    Sc64T4Trigger trigger = {.conditions = conditions, .actions = actions,
                             .enabled_players = {[0] = 1}};
    Sc64T4Scenario program = {.triggers = &trigger, .trigger_count = 1};
    Sc64T4TriggerState state, restored;
    Sc64T4TriggerSwitches switches = {{0}}, restored_switches;
    World world = {.retries = 1}, restored_world;
    Sc64T6Dispatch dispatch = {&ops, &world};
    assert(sc64_t4_trigger_init(&state, &program, 0, 1));
    for (unsigned tick = 0; tick < 6; ++tick)
        assert(sc64_t4_trigger_step(&state, &program, &switches,
                                   &bridge, &dispatch, 0));
    assert(state.wait_ticks == 10 && !world.attempts);
    restored = state; restored_switches = switches; restored_world = world;
    Sc64T6Dispatch restored_dispatch = {&ops, &restored_world};
    assert(sc64_t4_trigger_state_valid(&restored, &program));
    for (unsigned tick = 0; tick < 80; ++tick) {
        assert(sc64_t4_trigger_step(&state, &program, &switches,
                                   &bridge, &dispatch, 0));
        assert(sc64_t4_trigger_step_prevalidated(&restored, &program,
            &restored_switches, &bridge, &restored_dispatch, 0));
        assert(memcmp(&state, &restored, sizeof(state)) == 0);
        assert(memcmp(&world, &restored_world, sizeof(world)) == 0);
    }
    assert(world.attempts == 2 && world.creates == 1);
    assert(state.flags[0] == SC64_T4_TR_DONE);
    assert(!state.wait_ticks && state.active_wait == 255);
}

static void typed_dispatch_and_failure(void)
{
    World world = {0};
    Sc64T6Dispatch dispatch = {&ops, &world};
    Sc64T4Action action = {.opcode = 26, .player = 0, .argument = 75,
                          .unit = 0, .modifier = 7};
    assert(sc64_t6_dispatch_action(&dispatch, &action, 0) == 1);
    assert(world.resource_calls == 1);
    action = (Sc64T4Action){.opcode = 15, .argument = 1};
    assert(sc64_t6_dispatch_action(&dispatch, &action, 0) == 1);
    assert(world.ai_calls == 1);
    action = (Sc64T4Action){.opcode = 10, .location = 1};
    assert(sc64_t6_dispatch_action(&dispatch, &action, 0) == 1);
    assert(world.presentation_calls == 1);
    action.opcode = 99;
    assert(sc64_t6_dispatch_action(&dispatch, &action, 0) == -1);
    assert(world.presentation_calls == 1);

    Sc64T4Condition conditions[16] = {{0}};
    Sc64T4Action actions[64] = {{.opcode = 11, .location = 2,
        .player = 4, .argument = 1, .unit = 37, .flags = 24}};
    Sc64T4Trigger trigger = {.conditions = conditions, .actions = actions,
                             .enabled_players = {[0] = 1}};
    Sc64T4Scenario program = {.triggers = &trigger, .trigger_count = 1};
    Sc64T4TriggerState state;
    Sc64T4TriggerSwitches switches = {{0}};
    world.fail_create = 1;
    assert(sc64_t4_trigger_init(&state, &program, 0, 1));
    assert(!sc64_t4_trigger_step(&state, &program, &switches,
                               &bridge, &dispatch, 0));
    assert(state.fault == SC64_T4_TR_BAD_ACTION && state.cursor[0] == 0);
    assert(!world.creates && world.attempts == 1);
    Sc64T6RuntimeOps missing = ops;
    missing.create = 0;
    dispatch.ops = &missing;
    assert(sc64_t4_trigger_init(&state, &program, 0, 1));
    assert(!sc64_t4_trigger_step(&state, &program, &switches,
                               &bridge, &dispatch, 0));
    assert(state.fault == SC64_T4_TR_BAD_ACTION && world.attempts == 1);
}

static void operand_bounds(void)
{
    Sc64T4Action action = {.opcode = 11, .location = 2, .player = 4,
                          .argument = 1, .unit = 37, .flags = 24};
    assert(sc64_t6_runtime_action_valid(&action));
    const u32 bad_properties[] = {0, 31, 0xffffffffu};
    for (unsigned i = 0; i < sizeof(bad_properties) / sizeof(bad_properties[0]); ++i) {
        action.argument = bad_properties[i];
        assert(!sc64_t6_runtime_action_valid(&action));
    }
    action.argument = 30;
    assert(sc64_t6_runtime_action_valid(&action));
    action.flags |= 2;
    assert(!sc64_t6_runtime_action_valid(&action));
    action.flags = 24; action.trailing[2] = 1;
    assert(!sc64_t6_runtime_action_valid(&action));

    Sc64T4Condition condition = {.opcode = 15, .player = 1, .unit = 231,
                                .amount = 1, .comparison = 0, .flags = 16};
    assert(sc64_t6_runtime_condition_valid(&condition));
    condition.player = 8;
    assert(!sc64_t6_runtime_condition_valid(&condition));
    condition.player = 1; condition.resource = 255;
    assert(!sc64_t6_runtime_condition_valid(&condition));
    assert(!sc64_t6_runtime_action_valid(0));
    assert(!sc64_t6_runtime_condition_valid(0));
}

int main(void)
{
    typed_deaths_and_terminal_priority();
    wait_snapshot_and_atomic_retry();
    typed_dispatch_and_failure();
    operand_bounds();
    puts("T6 typed trigger bridge: Deaths, waits, retries, snapshots and faults pass");
    return 0;
}
