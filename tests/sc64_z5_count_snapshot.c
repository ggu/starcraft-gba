#define SC64_Z5_COUNT_SNAPSHOT 1
#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z5_world_adapter.h"

static u32 rng_state = 0x61f09a2du;
static u32 test_random(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void init_ctx(Sc64Z5WorldContext *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->source_current_player = 3;
    for (unsigned i = 0; i < 8; ++i) ctx->source_slot_active[i] = 1;
}

static void check_rows(Sc64Z5WorldContext *ctx)
{
    unsigned cached = 0;
    for (unsigned i = 0; i < SC64_Z5_SOURCE_CONDITION_COUNT; ++i) {
        const Sc64Z5SourceCondition *row = &sc64_z5_source_conditions[i];
        u32 expected = 0, actual = 0;
        int expected_ok, found;
        if (row->opcode != 2 && row->opcode != 3) continue;
        expected_ok = sc64_z5_world_count_one(ctx, row->player, row->unit,
                                               row->opcode == 3 ? row->location : 0,
                                               &expected);
        found = sc64_z5_count_snapshot_lookup(ctx, row, &actual);
        if (found) {
            ++cached;
            assert(expected_ok && actual == expected);
        } else if (expected_ok) {
            Sc64Z5RuntimeState state;
            (void)sc64_z5_runtime_init(&state);
            assert(sc64_z5_world_condition(ctx, &state, row) ==
                   sc64_z5_world_compare(row->comparison, expected, row->amount));
        }
        if (found) {
            Sc64Z5RuntimeState state;
            (void)sc64_z5_runtime_init(&state);
            assert(sc64_z5_world_condition(ctx, &state, row) ==
                   sc64_z5_world_compare(row->comparison, expected, row->amount));
        }
    }
    assert(cached == 34u);
}

static void test_known_actor_world_and_invalidation(void)
{
    Sc64Z5WorldContext ctx;
    Sc64Z5RuntimeState runtime;
    init_ctx(&ctx);
    (void)sc64_z5_runtime_init(&runtime);
    race_cols = race_rows = 128;
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));

    /* Known source-mapped actors make each Bring bounds path observable. */
    for (int i = 0; i < 80; ++i) {
        int owner = i % 8;
        u[i].alive = 1;
        u[i].raw_type = (i % 4 == 0) ? 51 : (i % 4 == 1) ? 37 :
                        (i % 4 == 2) ? 0 : 101;
        u[i].type = sc64_race_unit_from_raw(u[i].raw_type);
        u[i].raw_owner = owner;
        u[i].owner = owner == 3 ? SIDE : ENEMY;
        u[i].x = 120 + (i * 73) % 3800;
        u[i].y = 140 + (i * 97) % 3700;
    }
    {
        int l, t, r, d;
        assert(sc64_z5_world_location(20, &l, &t, &r, &d));
        u[80].alive = 1;
        u[80].raw_type = 51;
        u[80].type = sc64_race_unit_from_raw(51);
        u[80].raw_owner = 3;
        u[80].owner = SIDE;
        u[80].x = (l + r) / 2;
        u[80].y = (t + d) / 2;
    }
    for (int i = 0; i < 32; ++i) {
        int owner = i % 8;
        b[i].alive = 1;
        b[i].raw_type = (i & 1) ? 150 : 106;
        b[i].type = sc64_race_building_from_raw(b[i].raw_type);
        b[i].raw_owner = owner;
        b[i].owner = owner == 3 ? SIDE : ENEMY;
        b[i].x = 180 + (i * 131) % 3600;
        b[i].y = 180 + (i * 101) % 3600;
    }
    assert(sc64_z5_count_snapshot_begin(&ctx, &runtime));
    assert(ctx.count_snapshot.active);
    check_rows(&ctx);
    {
        Sc64Z5RuntimeState state;
        Sc64Z5SourceCondition exact = sc64_z5_source_conditions[0];
        (void)sc64_z5_runtime_init(&state);
        exact.comparison = 10;
        exact.amount = 1;
        assert(sc64_z5_world_condition(&ctx, &state, &exact) == 1);
        exact.amount = 2;
        assert(sc64_z5_world_condition(&ctx, &state, &exact) == 0);
        exact.comparison = 2;
        assert(sc64_z5_world_condition(&ctx, &state, &exact) == -1);
    }

    /* A copied condition is not authenticated by pointer identity; fallback
     * observes a changed world instead of returning the phase snapshot. */
    {
        Sc64Z5SourceCondition copied = sc64_z5_source_conditions[0];
        u32 before, after = 0;
        assert(sc64_z5_count_snapshot_lookup(&ctx,
                    &sc64_z5_source_conditions[0], &before));
        copied.amount ^= 1u;
        u[80].alive = 0;
        assert(!sc64_z5_count_snapshot_lookup(&ctx, &copied, &after));
        assert(sc64_z5_world_count_one(&ctx, copied.player, copied.unit,
                    copied.location, &after));
        assert(after != before);
    }
    sc64_z5_count_snapshot_end(&ctx);
    assert(!ctx.count_snapshot.active);
    {
        u32 ignored;
        assert(!sc64_z5_count_snapshot_lookup(&ctx,
                    &sc64_z5_source_conditions[0], &ignored));
    }
}

static void test_randomized_parity_and_fallback(void)
{
    Sc64Z5WorldContext ctx;
    Sc64Z5RuntimeState runtime;
    init_ctx(&ctx);
    (void)sc64_z5_runtime_init(&runtime);
    race_cols = race_rows = 128;
    for (unsigned round = 0; round < 150; ++round) {
        memset(u, 0, sizeof(u));
        memset(b, 0, sizeof(b));
        for (int i = 0; i < MAX_UNITS; ++i) {
            u32 r = test_random();
            if ((r & 3u) == 0) continue;
            u[i].alive = 1;
            u[i].raw_type = (r >> 4) % 7u == 0 ? -1 :
                            (int[]){0, 1, 2, 7, 15, 32, 37, 51, 53, 101, 218}
                                [(r >> 10) % 11u];
            u[i].type = u[i].raw_type >= 0 ?
                        sc64_race_unit_from_raw(u[i].raw_type) :
                        (int)((r >> 17) % 80u);
            u[i].raw_owner = (int)((r >> 21) % 14u) - 1;
            u[i].owner = (int)((r >> 26) % 6u) - 1;
            u[i].x = (int)((r >> 2) % (128u * 32u));
            u[i].y = (int)((r >> 7) % (128u * 32u));
        }
        for (int i = 0; i < MAX_BUILDINGS; ++i) {
            u32 r = test_random();
            if ((r & 3u) == 0) continue;
            b[i].alive = 1;
            b[i].raw_type = (r >> 4) % 7u == 0 ? -1 :
                            (int[]){106, 109, 124, 150, 162, 195, 196, 203}
                                [(r >> 10) % 8u];
            b[i].type = b[i].raw_type >= 0 ?
                        sc64_race_building_from_raw(b[i].raw_type) :
                        (int)((r >> 17) % 80u);
            b[i].raw_owner = (int)((r >> 21) % 14u) - 1;
            b[i].owner = (int)((r >> 26) % 6u) - 1;
            b[i].x = (int)((r >> 2) % (128u * 32u));
            b[i].y = (int)((r >> 7) % (128u * 32u));
        }
        if (sc64_z5_count_snapshot_begin(&ctx, &runtime)) check_rows(&ctx);
        else {
            /* A selected actor with unknown location bounds must disable the
             * cache so each callback retains its original full-scan result. */
            assert(!ctx.count_snapshot.active);
            for (unsigned i = 0; i < SC64_Z5_SOURCE_CONDITION_COUNT; ++i) {
                const Sc64Z5SourceCondition *row = &sc64_z5_source_conditions[i];
                u32 count;
                if (row->opcode != 2 && row->opcode != 3) continue;
                (void)sc64_z5_world_count_one(&ctx, row->player, row->unit,
                        row->opcode == 3 ? row->location : 0, &count);
                assert(!sc64_z5_count_snapshot_lookup(&ctx, row, &count));
            }
        }
        sc64_z5_count_snapshot_end(&ctx);
    }
}

static void test_started_row_cache_policy(void)
{
    Sc64Z5WorldContext ctx;
    Sc64Z5RuntimeState runtime;
    u32 count;
    init_ctx(&ctx);
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    race_cols = race_rows = 128;
    (void)sc64_z5_runtime_init(&runtime);
    sc64_z5_mask_set(&runtime.model.started_lo, &runtime.model.started_hi, 4);
    runtime.model.action_cursor[4] = 1;
    runtime.action_ticks[4] = sc64_z5_action_duration(4, 0);
    assert(sc64_z5_runtime_state_valid(&runtime));
    assert(sc64_z5_count_snapshot_begin(&ctx, &runtime));
#ifdef SC64_Z5_PRUNE_STARTED
    assert(!sc64_z5_count_snapshot_lookup(&ctx,
                &sc64_z5_source_conditions[4], &count));
#else
    assert(sc64_z5_count_snapshot_lookup(&ctx,
                &sc64_z5_source_conditions[4], &count));
#endif
    sc64_z5_count_snapshot_end(&ctx);
}

int main(void)
{
    assert(sc64_z5_world_compare(0, 3, 2) == 1);
    assert(sc64_z5_world_compare(1, 3, 2) == 0);
    assert(sc64_z5_world_compare(10, 3, 3) == 1);
    assert(sc64_z5_world_compare(2, 3, 3) == -1);
    test_known_actor_world_and_invalidation();
    test_randomized_parity_and_fallback();
    test_started_row_cache_policy();
    return 0;
}
