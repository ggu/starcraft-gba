/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free behavioral fixture for the complete race route-field cache. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8;
#define GBA_HOT
#define SC64_RACES
#ifdef TEST_PACK3
#define SC64_ROUTE_PACK3
#endif
#ifdef TEST_ACTIVE
#define SC64_LARGE_MAPS
#define SC64_ZP_CAMPAIGN_STORAGE
#define SC64_ROUTE_ACTIVE_STRIDE
#endif
#ifdef TEST_DECAY
#define SC64_ROUTE_DECAY_FREQ
#endif

static int test_cols = 8, test_rows = 8;
#define MAP_COLS test_cols
#define MAP_ROWS test_rows
#define MAP_CELLS 16384
#ifdef TEST_BIT_GRID
static u8 path_walkable[(MAP_CELLS + 7) / 8];
static int fixture_grid_get(int cell)
{ return (path_walkable[(unsigned)cell >> 3] >> (cell & 7)) & 1; }
static void fixture_grid_set(int cell, int value)
{
    u8 *p = &path_walkable[(unsigned)cell >> 3];
    u8 mask = (u8)(1u << (cell & 7));
    *p = value ? (u8)(*p | mask) : (u8)(*p & (u8)~mask);
}
#define SC64_GRID_GET(grid, cell) fixture_grid_get(cell)
#else
static u8 path_walkable[MAP_CELLS];
static int fixture_grid_get(int cell) { return path_walkable[cell] != 0; }
static void fixture_grid_set(int cell, int value) { path_walkable[cell] = (u8)(value != 0); }
#endif
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid = 1;
static int limit(int n, int lo, int hi) { return n < lo ? lo : (n > hi ? hi : n); }
static int sc64_cell_row(unsigned c) { return (int)c / MAP_COLS; }
static int sc64_cell_col(unsigned c) { return (int)c % MAP_COLS; }
static void build_path_grid(void) { path_grid_valid = 1; }

#include "../starcraft/sc64_routes.h"

static int ref_parent[MAP_CELLS], ref_queue[MAP_CELLS];
/* Independent L/R/U/D BFS oracle, storing the parent cell, not route codes. */
static int ref_bfs(int root, int origin)
{
    int h = 0, t = 0, i;
    for (i = 0; i < MAP_CELLS; ++i) ref_parent[i] = -2;
    ref_parent[root] = -1; ref_queue[t++] = root;
    while (h < t) {
        int c = ref_queue[h++], x = c % MAP_COLS, y = c / MAP_COLS;
        int n[4] = { x ? c - 1 : -1, x + 1 < MAP_COLS ? c + 1 : -1,
                     y ? c - MAP_COLS : -1,
                     y + 1 < MAP_ROWS ? c + MAP_COLS : -1 };
        for (i = 0; i < 4; ++i)
            if (n[i] >= 0 && fixture_grid_get(n[i]) && ref_parent[n[i]] == -2) {
                ref_parent[n[i]] = c; ref_queue[t++] = n[i];
            }
    }
    return ref_parent[origin] != -2;
}
static int ref_first(int root, int origin)
{
    int p;
    assert(ref_bfs(root, origin));
    p = ref_parent[origin];
    return p < 0 ? origin : p;
}
static int px(int cell) { return (cell % MAP_COLS) * 16 + 8; }
static int py(int cell) { return (cell / MAP_COLS) * 16 + 8; }
static void set_open(void)
{
    int c;
    for (c = 0; c < MAP_CELLS; ++c) fixture_grid_set(c, 0);
    for (c = 0; c < MAP_COLS * MAP_ROWS; ++c) fixture_grid_set(c, 1);
}
static void reset_cache(void)
{
    sc64_routes_reset();
    sc64_route_clock = 0;
    sc64_route_hits = sc64_route_builds = 0;
}
static int next_cell(int origin, int goal, int *out)
{
    int nx = -1, ny = -1;
    if (!sc64_route_next(px(origin), py(origin), px(goal), py(goal), &nx, &ny)) return 0;
    assert(nx >= 0 && ny >= 0);
    assert((nx & 15) == 8 && (ny & 15) == 8);
    *out = nx / 16 + (ny / 16) * MAP_COLS;
    return 1;
}
static void assert_oracle(int origin, int goal)
{
    int got, want = ref_first(goal, origin);
    assert(next_cell(origin, goal, &got));
    assert(got == want);
}

static void test_open_subpath(void)
{
    const int root = 2 + 2 * MAP_COLS, goal = 1 + 2 * MAP_COLS;
    const int origin = 0 + 2 * MAP_COLS;
    unsigned builds;
    int slot, got;
    set_open(); reset_cache();
    slot = sc64_route_slot(root);
    assert(slot >= 0 && sc64_route_valid[slot] == 1);
    builds = sc64_route_builds;
    assert_oracle(origin, goal);
    assert(sc64_route_builds == builds);
    assert(sc64_route_hits == 1);
    assert(sc64_route_clock == 2u); /* one request for seed build, one for reuse */
    /* A cache already rooted at the exact goal takes normal exact-hit path. */
    reset_cache(); (void)sc64_route_slot(root); (void)sc64_route_slot(goal);
    builds = sc64_route_builds;
    assert_oracle(origin, goal);
    assert(sc64_route_builds == builds);
    assert(sc64_route_hits == 1u); /* normal exact-root cache hit */
    /* The endpoint itself preserves exact caller coordinates. */
    {
        int nx = -1, ny = -1;
        assert(sc64_route_next(px(goal), py(goal), px(goal), py(goal), &nx, &ny));
        assert(nx == px(goal) && ny == py(goal));
    }
    /* A stale cached root that is now blocked must not be used for reuse. */
    reset_cache(); slot = sc64_route_slot(root); assert(slot >= 0);
    fixture_grid_set(root, 0); builds = sc64_route_builds;
    assert_oracle(origin, goal);
    assert(sc64_route_builds == builds + 1);
    /* These explicit next cells were recorded from the baseline implementation. */
    fixture_grid_set(root, 1); fixture_grid_set(goal, 0); reset_cache();
    (void)sc64_route_slot(root);
    assert(next_cell(origin, goal, &got) && got == 17);
    fixture_grid_set(goal, 1); fixture_grid_set(origin, 0); reset_cache();
    assert(next_cell(origin, goal, &got) && got == 17);
    puts("blocked_legacy=ok");
}

/* An open grid has equally short first moves right and down. Reverse BFS
 * prefers down here; a cached-root reuse must preserve that tie exactly. */
static void test_reuse_tie(void)
{
    int root, goal, origin = 0, slot, got;
    test_cols = 8; test_rows = 8;
    root = 2 + 2 * MAP_COLS; goal = 1 + 2 * MAP_COLS;
    set_open(); reset_cache();
    slot = sc64_route_slot(root);
    assert(slot >= 0);
    assert(ref_first(goal, origin) == MAP_COLS);
    assert(next_cell(origin, goal, &got) && got == MAP_COLS);
    assert(sc64_route_builds == 1 && sc64_route_hits == 1);
    assert(sc64_route_goal[slot] == root);
}

static unsigned tape = 0x5c64a129u;
static unsigned rnd(void)
{
    tape ^= tape << 13; tape ^= tape >> 17; tape ^= tape << 5; return tape;
}
static void test_random_tapes(void)
{
    unsigned opportunities = 0;
    int m;
    test_cols = 12; test_rows = 12;
#ifdef TEST_ACTIVE
    (void)sc64_route_configure();
#endif
    for (m = 0; m < 600; ++m) {
        int c, root, origin, goal = -1, slot, steps = 0;
        reset_cache();
        for (c = 0; c < MAP_CELLS; ++c) fixture_grid_set(c, 0);
        for (c = 0; c < MAP_COLS * MAP_ROWS; ++c) fixture_grid_set(c, (rnd() % 100u) < 78u);
        root = (int)(rnd() % (unsigned)(MAP_COLS * MAP_ROWS));
        origin = (int)(rnd() % (unsigned)(MAP_COLS * MAP_ROWS));
        if (root == origin || !fixture_grid_get(root) || !fixture_grid_get(origin)) continue;
        if (!ref_bfs(root, origin)) continue;
        slot = sc64_route_slot(root); if (slot < 0) continue;
        c = origin;
        while (c != root && steps++ < MAP_CELLS) {
            int x, y, d;
            d = sc64_route_get(slot, c);
            if (d < 1 || d > 4) break;
            x = c % MAP_COLS; y = c / MAP_COLS;
            if (d == 1) ++x; else if (d == 2) --x;
            else if (d == 3) ++y; else --y;
            c = x + y * MAP_COLS;
            if (c == root) break;
            if (abs(c % MAP_COLS - root % MAP_COLS) + abs(c / MAP_COLS - root / MAP_COLS) <= 4) {
                goal = c; break;
            }
        }
        if (goal < 0) continue;
        /* Confirm the candidate is exactly on the independently derived
         * shortest route, not merely any nearby walkable cell. */
        if (!ref_bfs(root, origin)) continue;
        {
            int p = origin, on_path = 0, guard = 0;
            while (p != root && guard++ < MAP_CELLS) {
                if (p == goal) { on_path = 1; break; }
                p = ref_parent[p];
                if (p < 0) break;
            }
            if (!on_path) continue;
        }
        ++opportunities;
        assert_oracle(origin, goal);
        assert(sc64_route_builds == 1);
    }
    printf("random_tape_reuse_opportunities=%u\n", opportunities);
    assert(opportunities >= 3);
}

static void test_long_route_bound(void)
{
    int x, y, origin, goal, root, got, sees_goal = 0;
    unsigned before;
    test_cols = 128; test_rows = 3;
#ifdef TEST_ACTIVE
    (void)sc64_route_configure();
#endif
    for (x = 0; x < MAP_CELLS; ++x) fixture_grid_set(x, 0);
    for (x = 0; x < 128; ++x) { fixture_grid_set(x, 1); fixture_grid_set(2 * 128 + x, 1); }
    fixture_grid_set(128 + 127, 1); /* sole connector between corridor rows */
    origin = 2 * 128; goal = 1; root = 0;
    reset_cache(); (void)sc64_route_slot(root);
    assert(ref_bfs(root, origin));
    /* Ensure oracle route origin->root crosses the candidate goal and has >128
     * steps, so bounded reuse must miss and build the fresh field. */
    y = origin; x = 0;
    while (y != root && x <= 300) {
        y = ref_parent[y]; ++x;
        if (y == goal) sees_goal = 1;
    }
    assert(y == root && x > 128 && sees_goal);
    before = sc64_route_builds;
    assert_oracle(origin, goal);
    assert(sc64_route_builds == before + 1);
    (void)got; (void)before;
}

static void test_unreachable_goal(void)
{
    int i, root, origin, goal, got;
    unsigned before;
    test_cols = 8; test_rows = 8;
    for (i = 0; i < MAP_CELLS; ++i) fixture_grid_set(i, 0);
    for (i = 0; i < MAP_ROWS; ++i) {
        int x;
        for (x = 0; x < 4; ++x) fixture_grid_set(x + i * MAP_COLS, 1);
        for (x = 5; x < MAP_COLS; ++x) fixture_grid_set(x + i * MAP_COLS, 1);
    }
    root = 3 + 3 * MAP_COLS; origin = 2 + 3 * MAP_COLS; goal = 5 + 3 * MAP_COLS;
    reset_cache(); (void)sc64_route_slot(root); before = sc64_route_builds;
    assert(!ref_bfs(goal, origin));
    assert(!next_cell(origin, goal, &got));
    assert(sc64_route_builds == before + 1);
    puts("unreachable_goal=miss");
}

#ifdef TEST_ACTIVE
static void assert_invalidated(void)
{
    int i;
    for (i = 0; i < SC64_ROUTE_ACTIVE_MAX_SLOTS; ++i) assert(sc64_route_valid[i] == 0);
}
static void test_active_resize(void)
{
    unsigned canary;
    test_cols = 8; test_rows = 8; set_open(); reset_cache();
    canary = sc64_route_active_bytes * sc64_route_active_slots;
    assert(canary < SC64_ROUTE_POOL_BYTES); sc64_route_pool[canary] = 0xa5;
    { int s = sc64_route_slot(0); assert(s >= 0 && sc64_route_valid[s]); }
    assert(sc64_route_pool[canary] == 0xa5);
    test_cols = 64; test_rows = 64;
    assert(sc64_route_configure()); assert_invalidated();
    assert(sc64_route_active_bytes == (64u * 64u * 3u + 7u) / 8u);
    assert(sc64_route_active_slots == SC64_ROUTE_ACTIVE_MAX_SLOTS);
    canary = sc64_route_active_bytes * sc64_route_active_slots;
    assert(canary < SC64_ROUTE_POOL_BYTES); sc64_route_pool[canary] = 0xa5;
    set_open(); { int s = sc64_route_slot(1); assert(s >= 0 && sc64_route_valid[s]); }
    assert(sc64_route_pool[canary] == 0xa5);
    test_cols = 8; test_rows = 8;
    assert(sc64_route_configure()); assert_invalidated();
    assert(sc64_route_active_bytes == 24u);
    /* Over-size geometry is rejected before route-grid access. */
    test_cols = 129; test_rows = 1;
    assert(!sc64_route_configure());
    assert(!sc64_route_next(8, 8, 24, 8, &(int){0}, &(int){0}));
    test_cols = 8; test_rows = 8; assert(sc64_route_configure());
    puts("active_resize=ok");
}
#endif

static void test_eviction_and_reset(void)
{
    int i, active, built = 0, requested_slot = -1;
    test_cols = 8; test_rows = 8; set_open(); reset_cache();
    active = SC64_ROUTE_ACTIVE_SLOTS;
    for (i = 0; i < active; ++i) {
        int goal = (i % 8) + ((i / 8) % 8) * MAP_COLS;
        (void)sc64_route_slot(goal);
    }
    for (i = 0; i < active; ++i) if (sc64_route_valid[i]) ++built;
    assert(built == active);
    (void)sc64_route_slot(7 + 7 * MAP_COLS);
    for (i = 0; i < active; ++i) {
        if (sc64_route_valid[i]) ++built;
        if (sc64_route_valid[i] && sc64_route_goal[i] == 7 + 7 * MAP_COLS) requested_slot = i;
    }
    assert(requested_slot >= 0 && built == 2 * active);
    reset_cache();
    for (i = 0; i < active; ++i) assert(sc64_route_valid[i] == 0);
    puts("eviction_reset=ok");
}

#ifdef TEST_DECAY
static void test_decay_boundary_and_wrap(void)
{
    int root = 2 + 2 * MAP_COLS, goal = 1 + 2 * MAP_COLS;
    int origin = 0 + 2 * MAP_COLS, other = 7 + 7 * MAP_COLS;
    int slot, oslot, got, got_slot;
    unsigned builds;
    set_open(); reset_cache();
    slot = sc64_route_slot(root); oslot = sc64_route_slot(other);
    assert(slot >= 0 && oslot >= 0 && slot != oslot);
    sc64_route_age[slot] = sc64_route_age_pack(6u, 120u);
    sc64_route_age[oslot] = sc64_route_age_pack(4u, 121u);
    sc64_route_clock = 127u; builds = sc64_route_builds;
    got_slot = sc64_route_step_slot(origin, goal);
    assert(got_slot >= 0);
    assert(got_slot == slot);
    assert(sc64_route_clock == 128u);
    assert(sc64_route_age_freq(sc64_route_age[slot]) == 4u);
    assert(sc64_route_age_freq(sc64_route_age[oslot]) == 2u);
    assert(sc64_route_age_stamp(sc64_route_age[slot]) == 128u);
    assert(sc64_route_builds == builds && sc64_route_hits == 1u);
    /* At recency mask max, helper must refuse the stale field; normal lookup
     * performs its one wrap clear/request and produces the exact fresh answer. */
    reset_cache(); slot = sc64_route_slot(root); assert(slot >= 0);
    sc64_route_clock = SC64_ROUTE_RECENCY_MASK;
    builds = sc64_route_builds;
    assert_oracle(origin, goal);
    assert(sc64_route_builds == builds + 1);
    assert(sc64_route_clock == 1u);
    assert(sc64_route_hits == 0u);
    {
        int i, valid = 0;
        for (i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i) if (sc64_route_valid[i]) {
            ++valid; assert(sc64_route_goal[i] == (u16)goal);
        }
        assert(valid == 1);
    }
    (void)got; (void)got_slot; (void)builds; (void)slot;
    puts("decay_boundary_wrap=ok");
}
#endif

int main(void)
{
    test_open_subpath();
    test_reuse_tie();
    test_random_tapes();
    test_long_route_bound();
    test_unreachable_goal();
    test_eviction_and_reset();
#ifdef TEST_ACTIVE
    test_active_resize();
#endif
#ifdef TEST_DECAY
    test_decay_boundary_and_wrap();
#endif
    puts("route_subpath_fixture=ok");
    return 0;
}
