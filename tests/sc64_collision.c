/* Randomized exactness check for the sparse unit collision buckets. */
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

static unsigned int rng_state = 0x6d2b79f5u;

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
    for (i = 0; i < MAX_UNITS; ++i)
        if (&u[i] != q && u[i].alive) {
            int dx = u[i].x - nx, dy = u[i].y - ny;
            if (dx >= 6 || dx <= -6 || dy >= 6 || dy <= -6) continue;
            if (dist2(nx, ny, u[i].x, u[i].y) < 36 &&
                dist2(nx, ny, u[i].x, u[i].y) <= dist2(q->x, q->y, u[i].x, u[i].y))
                return 0;
        }
    return 1;
}

static int
check_candidate(const Unit *q, int x, int y, unsigned int *checks)
{
    int expected = reference_step_clear(q, x, y);
    int actual = unit_step_clear(q, x, y);
    ++*checks;
    return actual == expected;
}

int
main(void)
{
    unsigned int checks = 0;
    int state, i;
    for (state = 0; state < 2000; ++state) {
        memset(u, 0, sizeof(u));
        for (i = 0; i < MAX_UNITS; ++i) {
            u[i].alive = 1;
            u[i].x = (int)(next_random() % WORLD_W);
            u[i].y = (int)(next_random() % WORLD_H);
        }
        sc64_unit_buckets_begin();
        if (!sc64_unit_bucket_active) return 1;
        for (i = 0; i < MAX_UNITS; ++i) {
            int nx = (int)(next_random() % WORLD_W);
            int ny = (int)(next_random() % WORLD_H);
            if (!check_candidate(&u[i], nx, ny, &checks)) return 2;
        }
        for (i = 0; i < MAX_UNITS; ++i) {
            Unit *q = &u[i];
            int nx = q->x + (int)(next_random() % 5) - 2;
            int ny = q->y + (int)(next_random() % 5) - 2;
            if (nx < 0) nx = 0;
            if (ny < 0) ny = 0;
            if (nx >= WORLD_W) nx = WORLD_W - 1;
            if (ny >= WORLD_H) ny = WORLD_H - 1;
            q->x = nx;
            q->y = ny;
            sc64_unit_bucket_move(q);
            if ((i % 7) == 0) u[(i + 19) % MAX_UNITS].alive = 0;
            nx = (int)(next_random() % WORLD_W);
            ny = (int)(next_random() % WORLD_H);
            if (!check_candidate(q, nx, ny, &checks)) return 3;
            if (!check_candidate(q, q->x + 2, q->y, &checks)) return 4;
        }
        sc64_unit_buckets_end();
    }
    printf("Collision buckets match full scan across 2000 randomized states (%u checks).\n", checks);
    return 0;
}
