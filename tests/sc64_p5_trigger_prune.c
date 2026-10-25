/* Differential source-program scheduling, including active waits and terminal
 * ties. Conditions are read-only; action callback observations must be equal. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_p5_adapter.h"

typedef struct {
    unsigned seed, tick, queries, count;
    u16 events[SC64_P5_TRIGGER_COUNT];
} Fixture;

static int condition(void *context, const Sc64P5RuntimeState *state,
                       const Sc64P5SourceCondition *row)
{
    Fixture *f = context;
    ++f->queries;
    assert(state && row);
    /* Deterministic independent input streams also exercise simultaneous
     * trigger starts, switch mutations, active waits and completed latches. */
    unsigned gate = (f->seed * 17u + row->trigger * 103u) % 400u;
    if (row->trigger < 2) gate += 600u;
    return f->tick >= gate;
}

static int action(void *context, const Sc64P5SourceAction *row)
{
    Fixture *f = context;
    assert(f->count < SC64_P5_TRIGGER_COUNT);
    f->events[f->count++] = (u16)(row->trigger * 256u + row->index);
    return 1;
}

int main(void)
{
    unsigned full_queries = 0, pruned_queries = 0, compared = 0;
    for (unsigned seed = 0; seed < 128; ++seed) {
        Sc64P5RuntimeState full, pruned;
        Fixture a = {0}, b = {0};
        a.seed = b.seed = seed;
        Sc64P5WorldOps full_ops = {&a, condition, action, action};
        Sc64P5WorldOps pruned_ops = {&b, condition, action, action};
        (void)sc64_p5_runtime_init(&full);
        (void)sc64_p5_runtime_init(&pruned);
        for (unsigned tick = 0; tick < 1500; ++tick) {
            a.tick = b.tick = tick;
            a.count = b.count = 0;
            memset(a.events, 0, sizeof(a.events));
            memset(b.events, 0, sizeof(b.events));
            int paused = tick % 53u == 0;
            int r0 = sc64_p5_adapter_tick_policy(&full, &full_ops, paused, 0);
            int r1 = sc64_p5_adapter_tick_policy(&pruned, &pruned_ops, paused, 1);
            assert(r0 == r1 && r0 >= 0);
            assert(!memcmp(&full, &pruned, sizeof(full)));
            assert(a.count == b.count && !memcmp(a.events, b.events, sizeof(a.events)));
            ++compared;
        }
        full_queries += a.queries;
        pruned_queries += b.queries;
        full.fault = pruned.fault = SC64_P5_FAULT_STATE;
        assert(sc64_p5_adapter_tick_policy(&full, &full_ops, 0, 0) ==
               sc64_p5_adapter_tick_policy(&pruned, &pruned_ops, 0, 1));
        assert(!memcmp(&full, &pruned, sizeof(full)));
    }
    assert(pruned_queries < full_queries);
    printf("P5 one-shot query pruning: %u equal ticks, queries %u -> %u\n",
           compared, full_queries, pruned_queries);
    return 0;
}
