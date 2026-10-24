/* Differential check for the opt-in P5 type-first world-count path. */
#define SC64_P5_TYPE_FIRST_COUNTS 1
#define SC64_P5_SHUTTLE_ENABLED 1
#define SC64_TRANSPORT_LOADED(q) ((q)->loaded)
#define main sc64_p5_world_fixture_main
#include "sc64_p5_world.c"
#undef main

static unsigned random_state = 0x64a5c21du;
static unsigned next_random(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}

static int bounded(int n) { return (int)(next_random() % (unsigned)n); }

static void random_world(Sc64P5WorldContext *ctx, u32 deaths[12])
{
    init_context(ctx, deaths);
    for (unsigned i = 0; i < 12; ++i)
        ctx->live_owner_for_source[i] = bounded(7) - 2;
    for (int i = 0; i < MAX_UNITS; ++i) {
        memset(&u[i], 0, sizeof(u[i]));
        u[i].alive = bounded(4) != 0;
        u[i].raw_type = bounded(5) == 0 ? -1 : bounded(235) - 3;
        u[i].type = bounded(235) - 3;
        u[i].raw_owner = bounded(16) - 2;
        u[i].owner = bounded(8) - 2;
        u[i].x = bounded(4096); u[i].y = bounded(4096);
        u[i].loaded = bounded(3) == 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        memset(&b[i], 0, sizeof(b[i]));
        b[i].alive = bounded(4) != 0;
        b[i].raw_type = bounded(5) == 0 ? -1 : bounded(235) - 3;
        b[i].type = bounded(235) - 3;
        b[i].raw_owner = bounded(16) - 2;
        b[i].owner = bounded(8) - 2;
        b[i].x = bounded(4096); b[i].y = bounded(4096);
    }
}

int main(void)
{
    Sc64P5WorldContext ctx;
    u32 deaths[12];
    const unsigned selectors[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
        14, 15, 16, 17, 18, 19, 20, 21, 22
    };
    const unsigned wanted[] = { 0, 8, 37, 65, 79, 101, 218, 227, 228, 229, 230 };
    const unsigned locations[] = { 0, 1, 2, 9, 17, 32, 64, 65 };

    /* Run the older P5 fixture too, so its unrelated world/action assertions
     * remain part of this direct test binary. */
    assert(sc64_p5_world_fixture_main() == 0);

    /* Explicit source-sensitive cases complement the randomized parity run. */
    init_context(&ctx, deaths);
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    {
        int left, top, right, bottom;
        u32 full, fast;
        assert(sc64_p5_world_location(1, &left, &top, &right, &bottom));
        add_unit(0, 37, 2, SIDE, (left + right) / 2, (top + bottom) / 2);
        u[0].loaded = 1;
        assert(sc64_p5_world_count_full(&ctx, 13, 37, 0, &full) && full == 1);
        assert(sc64_p5_world_count_type_first(&ctx, 13, 37, 0, &fast) && fast == 1);
        assert(sc64_p5_world_count_full(&ctx, 13, 37, 1, &full) && full == 0);
        assert(sc64_p5_world_count_type_first(&ctx, 13, 37, 1, &fast) && fast == 0);
        /* Current Player follows SIDE after rescue; direct slot still uses
         * raw provenance, for both implementations. */
        assert(sc64_p5_world_count_full(&ctx, 2, 37, 0, &full) && full == 1);
        assert(sc64_p5_world_count_type_first(&ctx, 2, 37, 0, &fast) && fast == 1);
    }
    assert(sc64_p5_world_compare(10, 4, 4) == 1);
    assert(sc64_p5_world_compare(2, 4, 4) == -1);

    for (unsigned round = 0; round < 300; ++round) {
        random_world(&ctx, deaths);
        for (unsigned si = 0; si < sizeof(selectors) / sizeof(selectors[0]); ++si)
            for (unsigned wi = 0; wi < sizeof(wanted) / sizeof(wanted[0]); ++wi)
                for (unsigned li = 0; li < sizeof(locations) / sizeof(locations[0]); ++li) {
                    u32 full = 0xa5a55a5au, fast = full;
                    int full_ok = sc64_p5_world_count_full(
                        &ctx, selectors[si], wanted[wi], locations[li], &full);
                    int fast_ok = sc64_p5_world_count_type_first(
                        &ctx, selectors[si], wanted[wi], locations[li], &fast);
                    assert(full_ok == fast_ok);
                    assert(full == fast); /* Includes untouched output on errors. */
                }
    }
    return 0;
}
