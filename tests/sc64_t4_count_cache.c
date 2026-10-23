#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SC64_T4_RUNTIME 1
#include "../starcraft/sc64_t4_count_cache.h"

typedef struct {
    unsigned calls;
    unsigned action_calls;
    unsigned fail_count;
    int action_status;
    u32 shared_count;
    u32 owner_count[8];
    unsigned effects[16];
    unsigned effect_count;
} World;

static int
world_count(void *context, const Sc64T4Condition *condition,
            unsigned owner, u32 *count)
{
    World *world = context;
    ++world->calls;
    if (world->fail_count) return 0;
    *count = condition->player == 13 ? world->owner_count[owner] :
                                      world->shared_count;
    return 1;
}

static int
world_action(void *context, const Sc64T4Action *action, unsigned owner)
{
    World *world = context;
    (void)owner;
    ++world->action_calls;
    if (world->effect_count < sizeof(world->effects) / sizeof(world->effects[0]))
        world->effects[world->effect_count++] = action->argument;
    if (world->action_status == 1) world->shared_count += action->argument;
    return world->action_status;
}

static const Sc64T4TriggerOps world_ops = {world_count, world_action};

static void
test_owner_keying_and_tick_reset(void)
{
    World world = {0};
    Sc64T4CountCache cache;
    Sc64T4Condition direct = {0}, current = {0};
    u32 count = 0;
    direct.player = 0;
    current.player = 13;
    world.shared_count = 37;
    world.owner_count[2] = 4;
    world.owner_count[5] = 9;
    sc64_t4_count_cache_init(&cache, &world_ops, &world);

    assert(sc64_t4_count_cache_ops.count(&cache, &direct, 0, &count) == 1);
    assert(count == 37);
    assert(sc64_t4_count_cache_ops.count(&cache, &direct, 5, &count) == 1);
    assert(count == 37 && world.calls == 1); /* Direct selectors share. */

    assert(sc64_t4_count_cache_ops.count(&cache, &current, 2, &count) == 1);
    assert(count == 4);
    assert(sc64_t4_count_cache_ops.count(&cache, &current, 5, &count) == 1);
    assert(count == 9);
    assert(sc64_t4_count_cache_ops.count(&cache, &current, 2, &count) == 1);
    assert(count == 4 && world.calls == 3); /* Current Player is owner-keyed. */

    world.shared_count = 41;
    sc64_t4_count_cache_init(&cache, &world_ops, &world);
    assert(sc64_t4_count_cache_ops.count(&cache, &direct, 5, &count) == 1);
    assert(count == 41 && world.calls == 4); /* New game tick, no stale value. */
}

static void
test_failures_are_not_cached_and_output_is_preserved(void)
{
    World world = {0};
    Sc64T4CountCache cache;
    Sc64T4Condition condition = {0};
    u32 count = 0xfeedu;
    world.fail_count = 1;
    sc64_t4_count_cache_init(&cache, &world_ops, &world);
    assert(sc64_t4_count_cache_ops.count(&cache, &condition, 0, &count) == 0);
    assert(count == 0xfeedu && cache.used == 0);
    assert(sc64_t4_count_cache_ops.count(&cache, &condition, 0, &count) == 0);
    assert(world.calls == 2 && cache.used == 0);
    world.fail_count = 0;
    world.shared_count = 12;
    assert(sc64_t4_count_cache_ops.count(&cache, &condition, 0, &count) == 1);
    assert(count == 12 && cache.used == 1 && world.calls == 3);
}

static void
test_invalid_owner_and_corrupt_size_are_rejected(void)
{
    World world = {0};
    Sc64T4CountCache cache;
    Sc64T4Condition direct = {0};
    u32 count = 0x1234u;
    world.shared_count = 6;
    sc64_t4_count_cache_init(&cache, &world_ops, &world);
    assert(sc64_t4_count_cache_ops.count(&cache, &direct, 0, &count) == 1);
    assert(count == 6 && world.calls == 1);

    count = 0x1234u;
    assert(sc64_t4_count_cache_ops.count(&cache, &direct, 8, &count) == 0);
    assert(count == 0x1234u && world.calls == 1);

    cache.used = SC64_T4_COUNT_CACHE_LIMIT + 1;
    assert(sc64_t4_count_cache_ops.count(&cache, &direct, 0, &count) == 0);
    assert(count == 0x1234u && world.calls == 1);
}

static void
test_capacity_overflow_falls_back_without_eviction(void)
{
    World world = {0};
    Sc64T4CountCache cache;
    Sc64T4Condition conditions[SC64_T4_COUNT_CACHE_LIMIT + 1];
    u32 count;
    memset(conditions, 0, sizeof(conditions));
    world.shared_count = 8;
    sc64_t4_count_cache_init(&cache, &world_ops, &world);
    for (unsigned i = 0; i < SC64_T4_COUNT_CACHE_LIMIT; ++i)
        assert(sc64_t4_count_cache_ops.count(&cache, &conditions[i], 0,
                                             &count) == 1);
    assert(cache.used == SC64_T4_COUNT_CACHE_LIMIT &&
           world.calls == SC64_T4_COUNT_CACHE_LIMIT);
    assert(sc64_t4_count_cache_ops.count(&cache,
               &conditions[SC64_T4_COUNT_CACHE_LIMIT], 0, &count) == 1);
    assert(world.calls == SC64_T4_COUNT_CACHE_LIMIT + 1);
    /* A full cache still serves hits and repeats fallback queries. */
    assert(sc64_t4_count_cache_ops.count(&cache, &conditions[0], 0,
                                         &count) == 1);
    assert(world.calls == SC64_T4_COUNT_CACHE_LIMIT + 1);
    assert(sc64_t4_count_cache_ops.count(&cache,
               &conditions[SC64_T4_COUNT_CACHE_LIMIT], 0, &count) == 1);
    assert(world.calls == SC64_T4_COUNT_CACHE_LIMIT + 2);
}

static void
test_actions_invalidate_before_success_retry_and_error(void)
{
    World world = {0};
    Sc64T4CountCache cache;
    Sc64T4Condition condition = {0};
    Sc64T4Action action = {0};
    u32 count;
    world.shared_count = 10;
    action.argument = 3;
    sc64_t4_count_cache_init(&cache, &world_ops, &world);
    for (int status = 1; status >= -1; --status) {
        world.action_status = status;
        assert(sc64_t4_count_cache_ops.count(&cache, &condition, 0,
                                             &count) == 1);
        unsigned calls = world.calls;
        assert(sc64_t4_count_cache_ops.action(&cache, &action, 0) == status);
        assert(cache.used == 0 && world.action_calls == (unsigned)(2 - status));
        assert(sc64_t4_count_cache_ops.count(&cache, &condition, 0,
                                             &count) == 1);
        assert(world.calls == calls + 1 && count == world.shared_count);
    }
}

static void
condition_count(Sc64T4Condition *condition, u32 amount)
{
    memset(condition, 0, sizeof(*condition));
    condition->opcode = 2;
    condition->comparison = 0;
    condition->amount = amount;
}

static void
fixture_trigger(Sc64T4Trigger *trigger, Sc64T4Condition *conditions,
                Sc64T4Action *actions)
{
    memset(trigger, 0, sizeof(*trigger));
    trigger->conditions = conditions;
    trigger->actions = actions;
    trigger->enabled_players[0] = 1;
}

static void
test_wrapped_and_uncached_source_order_match(void)
{
    Sc64T4Condition condition_always, condition_after_action;
    Sc64T4Condition condition_rows[2][16];
    Sc64T4Action action_rows[2][64];
    Sc64T4Trigger triggers[2];
    Sc64T4Scenario program = {0};
    Sc64T4TriggerState plain_state, cached_state;
    Sc64T4TriggerSwitches plain_switches = {0}, cached_switches = {0};
    World plain = {0}, cached = {0};
    Sc64T4CountCache cache;
    condition_count(&condition_always, 0);
    condition_count(&condition_after_action, 2);
    memset(condition_rows, 0, sizeof(condition_rows));
    /* Trigger 0 repeats the same immutable direct-player record before its
     * action. Trigger 1 must observe that action's changed world count. */
    condition_rows[0][0] = condition_always;
    condition_rows[0][1] = condition_always;
    condition_rows[1][0] = condition_after_action;
    memset(action_rows, 0, sizeof(action_rows));
    action_rows[0][0].opcode = 99; action_rows[0][0].argument = 1;
    action_rows[1][0].opcode = 99; action_rows[1][0].argument = 10;
    fixture_trigger(&triggers[0], condition_rows[0], action_rows[0]);
    fixture_trigger(&triggers[1], condition_rows[1], action_rows[1]);
    program.triggers = triggers; program.trigger_count = 2;
    plain.shared_count = cached.shared_count = 1;
    plain.action_status = cached.action_status = 1;
    assert(sc64_t4_trigger_init(&plain_state, &program, 0, 1));
    assert(sc64_t4_trigger_init(&cached_state, &program, 0, 1));

    sc64_t4_count_cache_init(&cache, &world_ops, &cached);
    assert(sc64_t4_trigger_step(&plain_state, &program, &plain_switches,
                                &world_ops, &plain, 0));
    assert(sc64_t4_trigger_step(&cached_state, &program, &cached_switches,
                                &sc64_t4_count_cache_ops, &cache, 0));
    assert(memcmp(&plain_state, &cached_state, sizeof(plain_state)) == 0);
    assert(memcmp(&plain_switches, &cached_switches, sizeof(plain_switches)) == 0);
    assert(plain.shared_count == cached.shared_count);
    assert(plain.effect_count == cached.effect_count && plain.effect_count == 2);
    assert(memcmp(plain.effects, cached.effects,
                  plain.effect_count * sizeof(plain.effects[0])) == 0);
    assert(plain.effects[0] == 1 && plain.effects[1] == 10);
    /* The first trigger's two condition slots are distinct immutable rows,
     * so both are queried; its action invalidates before trigger 1 observes
     * the changed count. The wrapper must preserve that source ordering. */
    assert(plain.calls == 3 && cached.calls == 3);
}

int
main(void)
{
    test_owner_keying_and_tick_reset();
    test_failures_are_not_cached_and_output_is_preserved();
    test_invalid_owner_and_corrupt_size_are_rejected();
    test_capacity_overflow_falls_back_without_eviction();
    test_actions_invalidate_before_success_retry_and_error();
    test_wrapped_and_uncached_source_order_match();
    puts("T4 trigger count cache tests passed");
    return 0;
}
