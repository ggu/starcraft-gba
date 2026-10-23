#define GBA_HOST 1
#define SC64_T4_RUNTIME 1
#define SC64_TERRAN_LATE_RUNTIME 1
#include "../common/gba.h"
#include "../starcraft/sc64_terran_late_count_cache.h"

#include <assert.h>

typedef struct {
    u32 world_value;
    unsigned count_calls;
    unsigned action_calls;
} FakeWorld;

static int fake_count(void *context, const Sc64T4Condition *condition,
                      unsigned owner, u32 *count)
{
    FakeWorld *world = context;
    ++world->count_calls;
    *count = world->world_value +
        ((condition->player == 13 || condition->player == 26) ? owner : 0);
    return 1;
}

static int fake_action(void *context, const Sc64T4Action *action,
                       unsigned owner)
{
    FakeWorld *world = context;
    (void)action;
    (void)owner;
    ++world->action_calls;
    ++world->world_value;
    return 1;
}

static const Sc64TerranLateRuntimeOps fake_ops = { fake_count, fake_action };

int main(void)
{
    FakeWorld world = { 10, 0, 0 };
    Sc64TerranLateCountCache cache;
    Sc64T4Condition shared = {0}, current_player = {0}, other_force = {0};
    Sc64T4Condition conditions[SC64_TERRAN_LATE_COUNT_CACHE_LIMIT + 1] = {{0}};
    Sc64T4Action action = {0};
    u32 count;

    sc64_terran_late_count_cache_init(&cache, &fake_ops, &world);
    shared.player = 0;
    assert(sc64_terran_late_count_cache_count(&cache, &shared, 0, &count) == 1);
    assert(count == 10 && world.count_calls == 1);
    assert(sc64_terran_late_count_cache_count(&cache, &shared, 7, &count) == 1);
    assert(count == 10 && world.count_calls == 1);

    /* A delegated action mutates the fake world and must invalidate a hit. */
    assert(sc64_terran_late_count_cache_action(&cache, &action, 0) == 1);
    assert(world.action_calls == 1);
    assert(sc64_terran_late_count_cache_count(&cache, &shared, 0, &count) == 1);
    assert(count == 11 && world.count_calls == 2);

    current_player.player = 13;
    assert(sc64_terran_late_count_cache_count(&cache, &current_player, 2, &count) == 1);
    assert(count == 13);
    assert(sc64_terran_late_count_cache_count(&cache, &current_player, 3, &count) == 1);
    assert(count == 14 && world.count_calls == 4);
    other_force.player = 26;
    assert(sc64_terran_late_count_cache_count(&cache, &other_force, 2, &count) == 1);
    assert(count == 13);
    assert(sc64_terran_late_count_cache_count(&cache, &other_force, 3, &count) == 1);
    assert(count == 14 && world.count_calls == 6);

    /* Full-cache misses still query the live world and remain uncached. */
    sc64_terran_late_count_cache_init(&cache, &fake_ops, &world);
    for (unsigned i = 0; i < SC64_TERRAN_LATE_COUNT_CACHE_LIMIT; ++i) {
        conditions[i].player = 0;
        conditions[i].amount = i;
        assert(sc64_terran_late_count_cache_count(&cache, &conditions[i],
                                                  0, &count) == 1);
    }
    assert(cache.used == SC64_TERRAN_LATE_COUNT_CACHE_LIMIT);
    unsigned calls_at_full = world.count_calls;
    assert(sc64_terran_late_count_cache_count(
               &cache, &conditions[SC64_TERRAN_LATE_COUNT_CACHE_LIMIT],
               0, &count) == 1);
    assert(world.count_calls == calls_at_full + 1);

    return 0;
}
