/* Race collision buckets compared with the complete overlap scan. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

static unsigned int rng_state = 0x4c9f2a31u;

static unsigned int
next_random(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static int
reference_step_clear(const Unit *q, int nx, int ny)
{
    int i;
    if (nx < 0 || ny < 0 || nx >= WORLD_W || ny >= WORLD_H) return 0;
    if (IS_WORKER(q->type) && (q->order == ORDER_MINE || q->order == ORDER_GAS))
        return 1;
    for (i = 0; i < MAX_UNITS; ++i)
        if (&u[i] != q && u[i].alive && u[i].type != U_CHRYSALIS &&
            sc64_race_unit_flying(u[i].type) == sc64_race_unit_flying(q->type)) {
            int dx = u[i].x - nx, dy = u[i].y - ny;
            if (dx <= -6 || dx >= 6 || dy <= -6 || dy >= 6) continue;
            if (dx * dx + dy * dy < 36 &&
                dx * dx + dy * dy <=
                    (u[i].x - q->x) * (u[i].x - q->x) +
                    (u[i].y - q->y) * (u[i].y - q->y)) return 0;
        }
    return 1;
}

static void
check_candidate(const Unit *q, int nx, int ny, unsigned int *checks)
{
    assert(unit_step_clear(q, nx, ny) == reference_step_clear(q, nx, ny));
    ++*checks;
}

static int
random_type(void)
{
    static const int types[] = {U_MARINE, U_ZERGLING, U_SCV, U_WRAITH,
                                U_MUTALISK, U_CHRYSALIS, U_DROPSHIP};
    return types[next_random() % (sizeof(types) / sizeof(types[0]))];
}

static void
randomized_states(unsigned int *checks)
{
    int state, i;
    for (state = 0; state < 1000; ++state) {
        memset(u, 0, sizeof(u));
        for (i = 0; i < MAX_UNITS; ++i) {
            u[i].alive = (next_random() & 7u) != 0;
            u[i].type = random_type();
            u[i].x = (int)(next_random() % WORLD_W);
            u[i].y = (int)(next_random() % WORLD_H);
            u[i].order = (next_random() % 20u == 0) ? ORDER_MINE : ORDER_IDLE;
        }
        /* Keep at least one live moving query unit in every state. */
        u[state % MAX_UNITS].alive = 1;
        u[state % MAX_UNITS].type = U_MARINE;
        sc64_unit_buckets_begin();
        assert(sc64_unit_bucket_active);
        for (i = 0; i < MAX_UNITS; ++i) {
            Unit *q = &u[i];
            static const int steering_delta[3] = {-2, 0, 2};
            int step;
            if (!q->alive) continue;
            /* Check all neighboring steering candidates in succession. */
            for (step = 0; step < 9; ++step) {
                int nx = q->x + steering_delta[step % 3];
                int ny = q->y + steering_delta[step / 3];
                check_candidate(q, nx, ny, checks);
            }
            if ((i % 11) == 0) {
                int neighbor = (i + 17) % MAX_UNITS;
                u[neighbor].alive = 0;
                check_candidate(q, q->x + 2, q->y, checks);
            }
            if ((i % 7) == 0) {
                int nx = q->x ^ 1; /* guaranteed same 16-pixel bucket */
                if (nx < 0 || nx >= WORLD_W) nx = q->x;
                q->x = nx;
                sc64_unit_bucket_move(q);
                check_candidate(q, q->x + 2, q->y, checks);
            }
        }
        sc64_unit_buckets_end();
    }
}

static void
movement_invalidation(unsigned int *checks)
{
    Unit *q = &u[0], *neighbor = &u[1];
    memset(u, 0, sizeof(u));
    q->alive = neighbor->alive = 1;
    q->type = neighbor->type = U_MARINE;
    q->x = 100; q->y = 100;
    neighbor->x = 106; neighbor->y = 100;
    sc64_unit_buckets_begin();
    check_candidate(q, 101, 100, checks);
    /* A same-cell move changes collision results and must invalidate. */
    neighbor->x = 104;
    sc64_unit_bucket_move(neighbor);
    check_candidate(q, 101, 100, checks);
    /* Cross-cell movement must invalidate before bucket relinking too. */
    q->x = 119;
    sc64_unit_bucket_move(q);
    check_candidate(q, 117, 100, checks);
    sc64_unit_buckets_end();
    /* Inactive index uses the existing full-scan fallback. */
    check_candidate(q, 118, 100, checks);
}

static void
edge_overflow_and_passthrough(unsigned int *checks)
{
    int i;
    memset(u, 0, sizeof(u));
    u[0].alive = 1; u[0].type = U_MARINE; u[0].x = 1; u[0].y = 1;
    u[1].alive = 1; u[1].type = U_MARINE; u[1].x = 0; u[1].y = 1;
    u[2].alive = 1; u[2].type = U_WRAITH; u[2].x = 1; u[2].y = 1;
    u[3].alive = 1; u[3].type = U_CHRYSALIS; u[3].x = 1; u[3].y = 1;
    sc64_unit_buckets_begin();
    check_candidate(&u[0], 0, 0, checks);
    check_candidate(&u[0], 2, 0, checks);
    check_candidate(&u[0], -1, 0, checks);
    sc64_unit_buckets_end();

    memset(u, 0, sizeof(u));
    for (i = 0; i < 34; ++i) {
        u[i].alive = 1;
        u[i].type = U_MARINE;
        u[i].x = 100 + (i % 5) - 2;
        u[i].y = 100 + (i / 5) - 3;
    }
    u[0].x = 100; u[0].y = 100;
    sc64_unit_buckets_begin();
    check_candidate(&u[0], 101, 100, checks);
    {
        int step;
        for (step = 0; step < 8; ++step) {
            int nx = u[0].x + ((step & 1) ? 2 : -2);
            int ny = u[0].y + ((step & 2) ? 2 : -2);
            check_candidate(&u[0], nx, ny, checks);
        }
    }
    sc64_unit_buckets_end();

    memset(u, 0, sizeof(u));
    u[0].alive = u[1].alive = 1;
    u[0].type = U_SCV; u[1].type = U_MARINE;
    u[0].order = ORDER_MINE;
    u[0].x = u[1].x = 100; u[0].y = u[1].y = 100;
    sc64_unit_buckets_begin();
    check_candidate(&u[0], 100, 100, checks);
    sc64_unit_buckets_end();
}

int
main(void)
{
    unsigned int checks = 0;
    randomized_states(&checks);
    movement_invalidation(&checks);
    edge_overflow_and_passthrough(&checks);
    printf("Race collision buckets match full scan (%u checks).\n", checks);
    return 0;
}
