/* Public route-dispatch regression checks for the monotone witness. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
#define GBA_HOT
#define SC64_RACES
#define SC64_ROUTE_PACK3
#define SC64_ROUTE_MONOTONE_PROBE

#if defined(TEST_ACTIVE)
#define SC64_LARGE_MAPS
#define SC64_ZP_CAMPAIGN_STORAGE
#define SC64_ROUTE_ACTIVE_STRIDE
#define SC64_ROUTE_VISIT_GRID
#define SC64_ROUTE_FLOOD_SPLIT
#define SC64_ROUTE_DECAY_FREQ
#define SC64_ROUTE_L_FAST_PATH
static int race_cols = 64, race_rows = 64;
#define MAP_COLS race_cols
#define MAP_ROWS race_rows
#define MAP_CELLS (128 * 128)
#else
#if defined(TEST_PARTIAL)
#define SC64_ROUTE_PARTIAL_FLOOD
#define SC64_ROUTE_VISIT_GRID
#define SC64_ROUTE_FLOOD_SPLIT
#define SC64_ROUTE_DECAY_FREQ
#endif
enum { MAP_COLS = 32, MAP_ROWS = 24, MAP_CELLS = MAP_COLS * MAP_ROWS };
#endif

#define SC64_ROUTE_MONOTONE_DIAGNOSTICS
#define GBA_HOST
static u32 frame;
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid = 1;
static int limit(int value, int low, int high)
{
    return value < low ? low : value > high ? high : value;
}

#if defined(TEST_ACTIVE)
static u8 path_walkable[(MAP_CELLS + 7) / 8];
#define SC64_GRID_GET(grid, cell) (((grid)[(cell) >> 3] >> ((cell) & 7u)) & 1u)
#define SC64_GRID_SET(grid, cell, value) do { \
    u8 mask_ = (u8)(1u << ((cell) & 7u)); \
    if (value) (grid)[(cell) >> 3] |= mask_; \
    else (grid)[(cell) >> 3] &= (u8)~mask_; \
} while (0)
#else
static u8 path_walkable[MAP_CELLS];
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_GRID_SET(grid, cell, value) ((grid)[(cell)] = (u8)((value) != 0))
#endif

static int sc64_cell_row(unsigned cell) { return (int)(cell / (unsigned)MAP_COLS); }
static int sc64_cell_col(unsigned cell) { return (int)(cell % (unsigned)MAP_COLS); }
static void build_path_grid(void) { path_grid_valid = 1; }

#include "starcraft/sc64_routes.h"

#ifndef SC64_ROUTE_MONOTONE_DIAG_ENABLED
static struct {
    unsigned attempts, hits, fails, budget, nodes, candidates, memo_checks, memo_clear_bytes;
} sc64_route_probe_diag;
#endif

static void grid_set(int col, int row, int value)
{
    unsigned cell = (unsigned)col + (unsigned)row * (unsigned)MAP_COLS;
    SC64_GRID_SET(path_walkable, cell, value);
}

static int grid_get(int col, int row)
{
    return SC64_GRID_GET(path_walkable,
        (unsigned)col + (unsigned)row * (unsigned)MAP_COLS);
}

static void grid_fill(int value)
{
#if defined(TEST_ACTIVE)
    memset(path_walkable, value ? 0xff : 0, sizeof(path_walkable));
    /* Ignore padding and cells outside the current active map shape. */
    for (int row = 0; row < MAP_ROWS; ++row)
        for (int col = 0; col < MAP_COLS; ++col)
            grid_set(col, row, value);
#else
    memset(path_walkable, value ? 1 : 0, sizeof(path_walkable));
#endif
    path_grid_valid = 1;
}

static unsigned rng = 0x64c0ffeeu;
static unsigned random_u32(void)
{
    rng = rng * 1664525u + 1013904223u;
    return rng;
}

/* Independent reference: the same public contract, implemented as FIFO BFS. */
static int reference_step(int sx, int sy, int gx, int gy, int *next_cell)
{
    int start = sx + sy * MAP_COLS;
    int goal = gx + gy * MAP_COLS;
    int head = 0, tail = 0;
    int previous[MAP_CELLS], distance[MAP_CELLS];
    static const int dx[4] = {-1, 1, 0, 0};
    static const int dy[4] = {0, 0, -1, 1};
    for (int i = 0; i < MAP_CELLS; ++i) previous[i] = distance[i] = -1;
    if (grid_get(gx, gy)) {
        previous[goal] = goal;
        distance[goal] = 0;
        bfs_queue[tail++] = (u16)goal;
    } else {
        /* Existing route fields seed the walkable eight-neighborhood in
         * dy/dx order and return the requested pixel endpoint from a seed. */
        for (int dy0 = -1; dy0 <= 1; ++dy0)
            for (int dx0 = -1; dx0 <= 1; ++dx0) {
                int x = gx + dx0, y = gy + dy0, cell;
                if ((!dx0 && !dy0) || x < 0 || y < 0 ||
                    x >= MAP_COLS || y >= MAP_ROWS) continue;
                cell = x + y * MAP_COLS;
                if (grid_get(x, y) && previous[cell] < 0) {
                    previous[cell] = goal;
                    distance[cell] = 0;
                    bfs_queue[tail++] = (u16)cell;
                }
            }
    }
    while (head < tail) {
        int cell = bfs_queue[head++];
        int col = cell % MAP_COLS, row = cell / MAP_COLS;
        for (int d = 0; d < 4; ++d) {
            int x = col + dx[d], y = row + dy[d], n;
            if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS ||
                !grid_get(x, y)) continue;
            n = x + y * MAP_COLS;
            if (previous[n] >= 0) continue;
            previous[n] = cell;
            distance[n] = distance[cell] + 1;
            bfs_queue[tail++] = (u16)n;
        }
    }
    if (grid_get(sx, sy)) {
        if (previous[start] < 0) return 0;
        *next_cell = previous[start];
    } else {
        static const int bx[4] = {1, -1, 0, 0};
        static const int by[4] = {0, 0, 1, -1};
        int best_distance = 0x7fffffff, best = -1;
        for (int i = 0; i < 4; ++i) {
            int x = sx + bx[i], y = sy + by[i];
            if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS ||
                distance[x + y * MAP_COLS] < 0) continue;
            if (distance[x + y * MAP_COLS] < best_distance) {
                best_distance = distance[x + y * MAP_COLS];
                best = x + y * MAP_COLS;
            }
        }
        if (best < 0) return 0;
        *next_cell = best;
    }
    return 1;
}

static void reset_world(void)
{
    grid_fill(0);
    sc64_routes_reset();
    memset(&sc64_route_probe_diag, 0, sizeof(sc64_route_probe_diag));
    frame += 17;
}

static void check_dispatch(int sx, int sy, int tx, int ty)
{
    int sc = limit(sx / 16, 0, MAP_COLS - 1);
    int sr = limit(sy / 16, 0, MAP_ROWS - 1);
    int gc = limit(tx / 16, 0, MAP_COLS - 1);
    int gr = limit(ty / 16, 0, MAP_ROWS - 1);
    int expected = -1, nx = -777, ny = -777;
    int ok = reference_step(sc, sr, gc, gr, &expected);
    int got;
#ifdef SC64_ROUTE_VISIT_GRID
    u8 before[sizeof(path_walkable)];
    memcpy(before, path_walkable, sizeof(before));
#endif
    got = sc64_route_next(sx, sy, tx, ty, &nx, &ny);
#ifdef SC64_ROUTE_VISIT_GRID
    if (memcmp(before, path_walkable, sizeof(before)) != 0) {
        for (unsigned i = 0; i < sizeof(before); ++i)
            if (before[i] != path_walkable[i]) {
                fprintf(stderr, "visit-grid restore mismatch byte=%u old=%u new=%u\n",
                        i, before[i], path_walkable[i]);
                abort();
            }
    }
#endif
    assert(got == ok);
    if (!ok) return;
    if (expected == sc + sr * MAP_COLS ||
        (expected == gc + gr * MAP_COLS && !grid_get(gc, gr))) {
        assert(nx == tx && ny == ty);
    } else {
        assert(nx / 16 + (ny / 16) * MAP_COLS == expected);
    }
}

static void test_dispatch_oracle(void)
{
    reset_world();
    grid_fill(1);
    for (int y = 3; y < MAP_ROWS - 3; ++y) grid_set(MAP_COLS / 2, y, 0);
    for (int x = MAP_COLS / 2 - 3; x <= MAP_COLS / 2 + 3; ++x)
        grid_set(x, MAP_ROWS / 2, 1);
    check_dispatch(4 * 16 + 8, MAP_ROWS / 2 * 16 + 8,
                   (MAP_COLS - 5) * 16 + 8, MAP_ROWS / 2 * 16 + 8);
    check_dispatch(4 * 16 + 8, 4 * 16 + 8,
                   (MAP_COLS - 5) * 16 + 8, (MAP_ROWS - 5) * 16 + 8);
    check_dispatch(4 * 16 + 8, 4 * 16 + 8,
                   (MAP_COLS - 5) * 16 + 8, (MAP_ROWS - 5) * 16 + 8);
    check_dispatch(-100, -100, MAP_COLS * 16 + 500, MAP_ROWS * 16 + 500);

    /* The monotone witness must be exercised through sc64_route_next, and
     * must not allocate a full route field when it has an exact witness. */
#ifndef TEST_ACTIVE
    reset_world();
    grid_fill(1);
    u32 before = sc64_route_builds;
    check_dispatch(2 * 16 + 8, 2 * 16 + 8, 18 * 16 + 8, 13 * 16 + 8);
#if defined(SC64_ROUTE_MONOTONE_ENABLED) && !defined(SC64_ROUTE_L_FAST_PATH)
    assert(sc64_route_probe_diag.attempts == 1);
    assert(sc64_route_probe_diag.hits == 1);
    assert(sc64_route_builds == before);
#endif
#endif
}

static void test_random_worlds(void)
{
#if defined(TEST_ACTIVE)
    const unsigned worlds = 32;
#else
    const unsigned worlds = 160;
#endif
    for (unsigned n = 0; n < worlds; ++n) {
        grid_fill(0);
        for (int row = 0; row < MAP_ROWS; ++row)
            for (int col = 0; col < MAP_COLS; ++col)
                if ((random_u32() & 7u) < 6u) grid_set(col, row, 1);
        int sx = (int)(random_u32() % (unsigned)MAP_COLS);
        int sy = (int)(random_u32() % (unsigned)MAP_ROWS);
        int gx = (int)(random_u32() % (unsigned)MAP_COLS);
        int gy = (int)(random_u32() % (unsigned)MAP_ROWS);
        grid_set(sx, sy, 1);
        sc64_routes_reset();
        check_dispatch(sx * 16 + 8, sy * 16 + 8,
                       gx * 16 + 8, gy * 16 + 8);
    }
}

static void test_cache_precedence(void)
{
#ifdef TEST_PARTIAL
    reset_world();
    grid_fill(1);
    check_dispatch(4 * 16 + 8, 4 * 16 + 8,
                   20 * 16 + 8, 15 * 16 + 8);
#else
    reset_world();
    grid_fill(1);
    /* Force the first request through full BFS. The wall blocks every
     * Manhattan-monotone path, then the second request selects a destination
     * within four steps on the cached field's actual suffix. */
    for (int y = 1; y < MAP_ROWS - 1; ++y) grid_set(12, y, 0);
    grid_set(12, 2, 1);
    int sx = 3, sy = 12, gx = 24, gy = 12;
    check_dispatch(sx * 16 + 8, sy * 16 + 8, gx * 16 + 8, gy * 16 + 8);
    int origin = sx + sy * MAP_COLS, root = gx + gy * MAP_COLS;
    int slot = -1, cur = origin, detour_goal = -1;
    for (int i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i)
        if (sc64_route_valid[i] == 1 && sc64_route_goal[i] == root) slot = i;
    assert(slot >= 0);
    for (int steps = 0; cur != root && steps < MAP_CELLS; ++steps) {
        int d = sc64_route_get(slot, cur), col = cur % MAP_COLS, row = cur / MAP_COLS;
        if (d == 1) ++col;
        else if (d == 2) --col;
        else if (d == 3) ++row;
        else if (d == 4) --row;
        else break;
        cur = col + row * MAP_COLS;
        int manhattan = abs(col - gx) + abs(row - gy);
        if (cur != root && manhattan <= 4) detour_goal = cur;
    }
    assert(detour_goal >= 0 && detour_goal != root);
    u32 builds = sc64_route_builds;
    unsigned attempts = sc64_route_probe_diag.attempts;
    u32 clock_before = sc64_route_clock;
    u32 age_before = sc64_route_age[slot];
    check_dispatch(sx * 16 + 8, sy * 16 + 8,
                   (detour_goal % MAP_COLS) * 16 + 8,
                   (detour_goal / MAP_COLS) * 16 + 8);
    assert(sc64_route_builds == builds);
    assert(sc64_route_clock == clock_before + 1u);
#ifdef SC64_ROUTE_DECAY_FREQ
    assert(sc64_route_age_stamp(sc64_route_age[slot]) ==
           (sc64_route_clock & SC64_ROUTE_RECENCY_MASK));
    assert(sc64_route_age_freq(sc64_route_age[slot]) ==
           (sc64_route_age_freq(age_before) < 63u ?
                sc64_route_age_freq(age_before) + 1u : 63u));
#else
    assert(sc64_route_age[slot] == sc64_route_clock);
#endif
#ifdef SC64_ROUTE_MONOTONE_ENABLED
    assert(sc64_route_probe_diag.attempts == attempts);
#endif
#ifdef SC64_ROUTE_DECAY_FREQ
    assert(sc64_route_clock > 0);
    assert(sc64_route_age_freq(sc64_route_age[slot]) <= 63u);
#endif

    /* An exact cached goal also wins without another monotone attempt/build. */
    builds = sc64_route_builds;
    attempts = sc64_route_probe_diag.attempts;
    check_dispatch(sx * 16 + 8, sy * 16 + 8, gx * 16 + 8, gy * 16 + 8);
    assert(sc64_route_builds == builds);
#ifdef SC64_ROUTE_MONOTONE_ENABLED
    assert(sc64_route_probe_diag.attempts == attempts);
#endif

    reset_world();
    grid_fill(1);
    grid_set(10, 10, 0);
    grid_set(11, 10, 0);
    /* A blocked origin bypasses monotone search and preserves legacy
     * cardinal-exit selection. */
#ifdef SC64_ROUTE_MONOTONE_ENABLED
    unsigned before = sc64_route_probe_diag.attempts;
#endif
    check_dispatch(10 * 16 + 8, 10 * 16 + 8,
                   18 * 16 + 8, 10 * 16 + 8);
#ifdef SC64_ROUTE_MONOTONE_ENABLED
    assert(sc64_route_probe_diag.attempts == before);
#endif
#endif
}

#ifndef TEST_PARTIAL
static void test_budget_and_canaries(void)
{
    struct {
        u16 before[4];
        u16 scratch[MAP_CELLS];
        u16 after[4];
    } guarded;
    sc64_route_probe_stats stats;
    int predecessor = -12345, status;
    memset(&guarded, 0xa5, sizeof(guarded));
    reset_world();
    grid_fill(1);
    int origin = 7 + 6 * MAP_COLS, goal = 2 + 2 * MAP_COLS;
    status = sc64_route_monotone_probe(path_walkable, MAP_COLS, MAP_ROWS,
        MAP_COLS * MAP_ROWS, guarded.scratch, sizeof(guarded.scratch),
        origin, goal, 0, &predecessor, &stats);
    assert(status == SC64_ROUTE_PROBE_BUDGET && predecessor == -12345);
    for (int i = 0; i < 4; ++i) assert(guarded.before[i] == 0xa5a5);
    for (int i = 0; i < 4; ++i) assert(guarded.after[i] == 0xa5a5);
    predecessor = -12345;
    status = sc64_route_monotone_probe(path_walkable, MAP_COLS, MAP_ROWS,
        MAP_COLS * MAP_ROWS, guarded.scratch, sizeof(guarded.scratch),
        origin, goal, 1, &predecessor, &stats);
    assert(status == SC64_ROUTE_PROBE_BUDGET);
    predecessor = -12345;
    status = sc64_route_monotone_probe(path_walkable, MAP_COLS, MAP_ROWS,
        MAP_COLS * MAP_ROWS, guarded.scratch, sizeof(guarded.scratch),
        origin, goal, 511, &predecessor, &stats);
    assert(status == 1 && predecessor >= 0);
    predecessor = -12345;
    status = sc64_route_monotone_probe(path_walkable, MAP_COLS, MAP_ROWS,
        MAP_COLS * MAP_ROWS, guarded.scratch, sizeof(guarded.scratch),
        origin, goal, 512, &predecessor, &stats);
    assert(status == 1 && predecessor >= 0);
    for (int i = 0; i < 4; ++i) assert(guarded.before[i] == 0xa5a5);
    for (int i = 0; i < 4; ++i) assert(guarded.after[i] == 0xa5a5);
    assert(sc64_route_monotone_probe(path_walkable, MAP_COLS, MAP_ROWS,
        MAP_COLS * MAP_ROWS, guarded.scratch, sizeof(guarded.scratch),
        origin, goal, 512, &predecessor, NULL) == SC64_ROUTE_PROBE_UNSUPPORTED);
}
#endif

static void test_active_geometry(void)
{
#if defined(TEST_ACTIVE)
    const int shapes[][2] = {{64, 64}, {96, 128}, {128, 128}};
    for (unsigned s = 0; s < sizeof(shapes) / sizeof(shapes[0]); ++s) {
        race_cols = shapes[s][0]; race_rows = shapes[s][1];
        reset_world();
        grid_fill(1);
        grid_set(race_cols / 2, race_rows / 2, 0);
        check_dispatch(2 * 16 + 8, 2 * 16 + 8,
            (race_cols - 3) * 16 + 8, (race_rows - 3) * 16 + 8);
        assert(sc64_route_active_slots > 0);
        for (unsigned i = 0; i < sc64_route_active_slots; ++i)
            assert(sc64_route_field[i] >= sc64_route_pool &&
                   sc64_route_field[i] + sc64_route_active_bytes <=
                       sc64_route_pool + SC64_ROUTE_POOL_BYTES);
    }
    race_cols = 129; race_rows = 128;
    assert(sc64_route_next(8, 8, 24, 24, &(int){0}, &(int){0}) == 0);
    assert(sc64_route_active_slots == 0);
    race_cols = 64; race_rows = 64;
    sc64_routes_reset();
#endif
}

int main(void)
{
#ifdef TEST_PARTIAL
#ifdef SC64_ROUTE_MONOTONE_ENABLED
#error "partial route fields must not enable the monotone witness"
#endif
#endif
    test_dispatch_oracle();
    test_random_worlds();
    test_cache_precedence();
#ifndef TEST_PARTIAL
    test_budget_and_canaries();
#endif
    test_active_geometry();
    printf("route monotone dispatch OK active=%d builds=%u hits=%u\n",
           SC64_ROUTE_ACTIVE_SLOTS, (unsigned)sc64_route_builds,
           (unsigned)sc64_route_hits);
    return 0;
}
