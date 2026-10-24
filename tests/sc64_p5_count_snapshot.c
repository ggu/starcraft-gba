#define SC64_P5_COUNT_SNAPSHOT 1
#define SC64_P5_SHUTTLE_ENABLED 1
#define main sc64_p5_world_baseline_main
#include "sc64_p5_world.c"
#undef main

static unsigned test_rng = 0x8a31d4c7u;
static unsigned next_rng(void)
{
    test_rng ^= test_rng << 13;
    test_rng ^= test_rng >> 17;
    test_rng ^= test_rng << 5;
    return test_rng;
}

static void check_snapshot_matches_full(Sc64P5WorldContext *ctx)
{
    static const unsigned rows[6] = {0, 1, 4, 5, 7, 8};
    assert(sc64_p5_count_snapshot_begin(ctx));
    for (unsigned i = 0; i < 6; ++i) {
        const Sc64P5SourceCondition *row = &sc64_p5_source_conditions[rows[i]];
        u32 expected = 0, actual = 0;
        unsigned location = row->opcode == 3 ? row->location : 0;
        assert(sc64_p5_count_snapshot_lookup(ctx, row, &actual));
        assert(sc64_p5_world_count_full(ctx, row->player, row->unit, location,
                                        &expected));
        assert(actual == expected);
        /* Conditions can query a row more than once during the phase. */
        assert(sc64_p5_count_snapshot_lookup(ctx, row, &actual));
        assert(actual == expected);
    }
}

int main(void)
{
    Sc64P5WorldContext ctx;
    u32 deaths[12];
    static const unsigned rows[6] = {0, 1, 4, 5, 7, 8};

    assert(sc64_p5_world_baseline_main() == 0);
    init_context(&ctx, deaths);

    /* Deterministic selector/cargo edge: global Command includes the loaded
     * source hero, while its Bring row excludes that passenger. */
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    add_unit(0, 79, 2, SIDE, 1740, 1720);
    u[0].loaded = 1;
    assert(sc64_p5_count_snapshot_begin(&ctx));
    {
        u32 value;
        assert(sc64_p5_count_snapshot_lookup(&ctx,
            &sc64_p5_source_conditions[0], &value) && value == 1);
        assert(sc64_p5_count_snapshot_lookup(&ctx,
            &sc64_p5_source_conditions[1], &value) && value == 1);
        assert(sc64_p5_count_snapshot_lookup(&ctx,
            &sc64_p5_source_conditions[4], &value) && value == 0);
    }
    sc64_p5_count_snapshot_end(&ctx);
    u[0].loaded = 0;
    assert(sc64_p5_count_snapshot_begin(&ctx));
    {
        u32 value;
        assert(sc64_p5_count_snapshot_lookup(&ctx,
            &sc64_p5_source_conditions[4], &value) && value == 1);
    }
    sc64_p5_count_snapshot_end(&ctx);

    /* Randomized live worlds cover source/live owners, malformed raw IDs,
     * buildings, locations, and loaded units. The oracle is the retained
     * full scan; snapshot construction must produce its six exact answers. */
    for (unsigned round = 0; round < 180; ++round) {
        memset(u, 0, sizeof(u));
        memset(b, 0, sizeof(b));
        for (int i = 0; i < MAX_UNITS; ++i) {
            unsigned r = next_rng();
            if ((r & 3u) == 0) continue;
            u[i].alive = 1;
            u[i].raw_type = (r >> 4) % 5u == 0 ? -1 : (int)((r >> 8) % 232u);
            u[i].type = (int)((r >> 13) % 90u);
            u[i].raw_owner = (int)((r >> 20) % 14u) - 1;
            u[i].owner = (int)((r >> 25) % 6u) - 1;
            u[i].x = (int)((r >> 2) % (128u * 32u));
            u[i].y = (int)((r >> 6) % (128u * 32u));
            u[i].loaded = (int)((r >> 29) & 1u);
        }
        for (int i = 0; i < MAX_BUILDINGS; ++i) {
            unsigned r = next_rng();
            if ((r & 3u) == 0) continue;
            b[i].alive = 1;
            b[i].raw_type = (r >> 4) % 5u == 0 ? -1 : (int)((r >> 8) % 232u);
            b[i].type = (int)((r >> 13) % 90u);
            b[i].raw_owner = (int)((r >> 20) % 14u) - 1;
            b[i].owner = (int)((r >> 25) % 6u) - 1;
            b[i].x = (int)((r >> 2) % (128u * 32u));
            b[i].y = (int)((r >> 6) % (128u * 32u));
        }
        /* Rescue happens before snapshot creation. Current Player must follow
         * the live SIDE owner while raw-owner selectors keep their provenance. */
        if (u[0].alive && u[0].raw_owner == 2) u[0].owner = SIDE;
        check_snapshot_matches_full(&ctx);

        /* Exact source rows use the snapshot; a copied row is deliberately a
         * cache miss and must observe current world state through full scan. */
        {
            Sc64P5SourceCondition copied = sc64_p5_source_conditions[4];
            u32 before = 0, after = 0;
            assert(sc64_p5_count_snapshot_lookup(&ctx,
                &sc64_p5_source_conditions[4], &before));
            copied.amount = 0;
            if (u[0].alive) u[0].alive = 0;
            assert(!sc64_p5_count_snapshot_lookup(&ctx, &copied, &after));
            assert(sc64_p5_world_count(&ctx, copied.player, copied.unit,
                copied.location, &after));
            assert(sc64_p5_world_count_full(&ctx, copied.player, copied.unit,
                copied.location, &before));
            assert(after == before);
        }
        sc64_p5_count_snapshot_end(&ctx);
        assert(!ctx.count_snapshot.active);
        /* Rebuild after a world change, and verify the new phase sees it. */
        memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
        add_unit(0, 79, 2, SIDE, 1740, 1720);
        assert(sc64_p5_count_snapshot_begin(&ctx));
        {
            u32 value;
            assert(sc64_p5_count_snapshot_lookup(&ctx,
                &sc64_p5_source_conditions[4], &value) && value == 1);
            u[0].alive = 0;
            sc64_p5_count_snapshot_end(&ctx);
            assert(sc64_p5_world_count_full(&ctx, 13, 79, 2, &value) && value == 0);
        }
        for (unsigned i = 0; i < 6; ++i) {
            u32 count = 0;
            assert(!sc64_p5_count_snapshot_lookup(&ctx,
                &sc64_p5_source_conditions[rows[i]], &count));
            assert(sc64_p5_world_count_full(&ctx,
                sc64_p5_source_conditions[rows[i]].player,
                sc64_p5_source_conditions[rows[i]].unit,
                sc64_p5_source_conditions[rows[i]].opcode == 3 ?
                    sc64_p5_source_conditions[rows[i]].location : 0, &count));
        }
    }

    /* Invalid source geometry disables the cache; the ordinary callback then
     * retains the full scan's error result. */
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    race_cols = 1;
    assert(!sc64_p5_count_snapshot_begin(&ctx));
    assert(!ctx.count_snapshot.active);
    {
        Sc64P5RuntimeState state;
        (void)sc64_p5_runtime_init(&state);
        assert(sc64_p5_world_condition(&ctx, &state,
               &sc64_p5_source_conditions[4]) == -1);
        assert(sc64_p5_world_condition(&ctx, &state,
               &sc64_p5_source_conditions[0]) == 1);
    }
    race_cols = 128;
    sc64_p5_count_snapshot_end(&ctx);
    return 0;
}
