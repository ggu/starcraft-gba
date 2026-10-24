#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_z5_adapter.h"

int sc64_z5_reference_tick(void *state, const void *ops, int user_paused);

typedef struct {
    u32 tick;
    u32 condition_calls[SC64_Z5_TRIGGER_COUNT];
    u32 action_calls[SC64_Z5_TRIGGER_COUNT];
    u32 action_hash;
} Fixture;

static int condition(void *opaque, const Sc64Z5RuntimeState *state,
                     const Sc64Z5SourceCondition *row)
{
    Fixture *f = (Fixture *)opaque;
    assert(state && row && row->trigger < SC64_Z5_TRIGGER_COUNT);
    ++f->condition_calls[row->trigger];
    /* Deterministic changing world observations exercise starts and preserves. */
    if (row->trigger == SC64_Z5_TRIGGER_VICTORY ||
        row->trigger == SC64_Z5_TRIGGER_DEFEAT || row->trigger == 0)
        return 0;
    return ((f->tick + row->trigger * 13u + row->index * 7u) % 23u) < 17u;
}

static int action(void *opaque, const Sc64Z5SourceAction *row)
{
    Fixture *f = (Fixture *)opaque;
    assert(row && row->trigger < SC64_Z5_TRIGGER_COUNT);
    ++f->action_calls[row->trigger];
    f->action_hash = (f->action_hash * 16777619u) ^
                     ((u32)row->trigger << 8) ^ row->index ^ row->opcode;
    return 1;
}

static int world_pause(void *opaque, int paused)
{
    (void)opaque;
    (void)paused;
    return 1;
}

static Sc64Z5WorldOps make_ops(Fixture *f)
{
    Sc64Z5WorldOps ops = {f, condition, action, world_pause};
    return ops;
}

static void test_pruning_is_scheduler_equivalent(void)
{
    Sc64Z5RuntimeState optimized, reference;
    Fixture fast = {0}, full = {0};
    Sc64Z5WorldOps fast_ops = make_ops(&fast), full_ops = make_ops(&full);
    unsigned skipped = 0, preserve_rechecks = 0;
    (void)sc64_z5_runtime_init(&optimized);
    reference = optimized;

    for (u32 tick = 0; tick < 12000; ++tick) {
        int paused = (tick % 97u) == 31u;
        u32 started_lo = optimized.model.started_lo;
        u32 started_hi = optimized.model.started_hi;
        u32 fast_before[SC64_Z5_TRIGGER_COUNT];
        u32 full_before[SC64_Z5_TRIGGER_COUNT];
        memcpy(fast_before, fast.condition_calls, sizeof(fast_before));
        memcpy(full_before, full.condition_calls, sizeof(full_before));
        fast.tick = full.tick = tick;
        int a = sc64_z5_adapter_tick(&optimized, &fast_ops, paused);
        int b = sc64_z5_reference_tick(&reference, &full_ops, paused);
        assert(a == b);
        assert(memcmp(&optimized, &reference, sizeof(optimized)) == 0);
        assert(memcmp(fast.action_calls, full.action_calls,
                      sizeof(fast.action_calls)) == 0);
        assert(fast.action_hash == full.action_hash);
        assert(sc64_z5_runtime_state_valid(&optimized));

        for (u8 trigger = 0; trigger < SC64_Z5_TRIGGER_COUNT; ++trigger) {
            int was_started = sc64_z5_mask_test(started_lo, started_hi, trigger);
            u32 fast_delta = fast.condition_calls[trigger] - fast_before[trigger];
            u32 full_delta = full.condition_calls[trigger] - full_before[trigger];
            if (was_started && !sc64_z5_trigger_preserved(trigger)) {
                assert(fast_delta == 0);
                skipped += full_delta;
            }
            if (was_started && sc64_z5_trigger_preserved(trigger)) {
                assert(fast_delta == full_delta);
                preserve_rechecks += fast_delta;
            }
        }
        if (optimized.model.outcome != SC64_Z5_RUNNING) break;
    }
    assert(skipped > 1000u);
    /* Preserved records remain condition-polled after their Preserve action. */
    assert(preserve_rechecks > 0);
    assert(fast.condition_calls[0] == full.condition_calls[0]);
    assert(fast.condition_calls[SC64_Z5_TRIGGER_VICTORY] ==
           full.condition_calls[SC64_Z5_TRIGGER_VICTORY]);
}

static void test_started_metadata_is_still_checked(void)
{
    Sc64Z5RuntimeState state;
    Fixture f = {0};
    Sc64Z5WorldOps ops = make_ops(&f);
    (void)sc64_z5_runtime_init(&state);
    sc64_z5_mask_set(&state.model.started_lo, &state.model.started_hi, 4);
    state.model.action_cursor[4] = 1;
    state.action_ticks[4] = 1;
    assert(sc64_z5_runtime_state_valid(&state));
    /* The started record's predicates are skipped, but all metadata is still
     * validated before timer advancement. */
    assert(sc64_z5_adapter_tick(&state, &ops, 0) == 1);
    assert(f.condition_calls[4] == 0);
}

int main(void)
{
    test_pruning_is_scheduler_equivalent();
    test_started_metadata_is_still_checked();
    return 0;
}
