/* Asset-free differential coverage for optional packed route-cache policies.
 * Build with RACES, LARGE_MAPS, PACK3 and selected route-policy gates. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;

#define GBA_HOT
#ifndef GBA_HOST
#define GBA_HOST
#endif
#if !defined(SC64_RACES) || !defined(SC64_LARGE_MAPS) || !defined(SC64_ROUTE_PACK3)
#error "This fixture requires packed 128x128 race route fields"
#endif

enum { MAP_COLS = 128, MAP_ROWS = 128, MAP_CELLS = MAP_COLS * MAP_ROWS };
static u8 path_walkable[(MAP_CELLS + 7) / 8];
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid = 1;
static int limit(int value, int lo, int hi)
{ return value < lo ? lo : value > hi ? hi : value; }
static int sc64_cell_col(unsigned cell) { return (int)cell % MAP_COLS; }
static int sc64_cell_row(unsigned cell) { return (int)cell / MAP_COLS; }
static void build_path_grid(void) { path_grid_valid = 1; }

#include "common/gba.h"
#include "starcraft/sc64_bitgrid.h"
#define SC64_GRID_GET(grid, cell) sc64_bitgrid_get((grid), MAP_CELLS, (cell))
#define SC64_GRID_SET(grid, cell, value) \
    sc64_bitgrid_set((grid), MAP_CELLS, (cell), (value))
#include "starcraft/sc64_routes.h"

static int distance[MAP_CELLS];
static u8 refdir[MAP_CELLS];
static u32 rng = 0x24ac91e7u;
static u32 random32(void) { rng = rng * 1664525u + 1013904223u; return rng; }
static void set_open(int cell, int value)
{ assert(sc64_bitgrid_set(path_walkable, MAP_CELLS, cell, value)); }
static int is_open(int cell)
{ return sc64_bitgrid_get(path_walkable, MAP_CELLS, cell); }
#ifdef SC64_ROUTE_DECAY_FREQ
static int cached_slot_for_goal(int goal)
{
    for (int i = 0; i < SC64_ROUTE_SLOTS; ++i)
        if (sc64_route_valid[i] && sc64_route_goal[i] == goal) return i;
    return -1;
}
#endif

/* Independent complete BFS with the production seed and neighbor tie order. */
static void reference_bfs(int goal)
{
    int head = 0, tail = 0;
    for (int i = 0; i < MAP_CELLS; ++i) {
        distance[i] = -1;
        refdir[i] = 15;
    }
    if (is_open(goal)) {
        distance[goal] = 0;
        bfs_queue[tail++] = (u16)goal;
    } else {
        int gx = goal % MAP_COLS, gy = goal / MAP_COLS;
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            int x = gx + dx, y = gy + dy, cell;
            if ((!dx && !dy) || x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS)
                continue;
            cell = x + y * MAP_COLS;
            if (is_open(cell) && distance[cell] < 0) {
                distance[cell] = 0;
                refdir[cell] = 0;
                bfs_queue[tail++] = (u16)cell;
            }
        }
    }
    while (head < tail) {
        int cell = bfs_queue[head++], x = cell % MAP_COLS, y = cell / MAP_COLS;
        int next[4] = { x ? cell - 1 : -1, x + 1 < MAP_COLS ? cell + 1 : -1,
                        y ? cell - MAP_COLS : -1,
                        y + 1 < MAP_ROWS ? cell + MAP_COLS : -1 };
        const int toward[4] = { 1, 2, 3, 4 };
        for (int i = 0; i < 4; ++i) {
            int n = next[i];
            if (n >= 0 && is_open(n) && distance[n] < 0) {
                distance[n] = distance[cell] + 1;
                refdir[n] = (u8)toward[i];
                bfs_queue[tail++] = (u16)n;
            }
        }
    }
}

static int route_matches_reference(int start, int goal)
{
    int sx = start % MAP_COLS, sy = start / MAP_COLS;
    int expected = -1, nx = 0, ny = 0;
    reference_bfs(goal);
    if (is_open(start)) {
        if (distance[start] >= 0) {
            if (start == goal || refdir[start] == 0) expected = goal;
            else expected = start + (refdir[start] == 1 ? 1 :
                                     refdir[start] == 2 ? -1 :
                                     refdir[start] == 3 ? MAP_COLS : -MAP_COLS);
        }
    } else {
        static const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
        int best = 0x7fffffff;
        for (int i = 0; i < 4; ++i) {
            int x = sx + dx[i], y = sy + dy[i], cell;
            if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS) continue;
            cell = x + y * MAP_COLS;
            if (distance[cell] >= 0 && distance[cell] < best) {
                best = distance[cell];
                expected = cell;
            }
        }
    }
    {
        u8 before[sizeof(path_walkable)];
        memcpy(before, path_walkable, sizeof(before));
        int ok = sc64_route_next(sx * 16 + 8, sy * 16 + 8,
                                 (goal % MAP_COLS) * 16 + 8,
                                 (goal / MAP_COLS) * 16 + 8, &nx, &ny);
        assert(memcmp(before, path_walkable, sizeof(before)) == 0);
        if (expected < 0) return !ok;
        if (!ok) return 0;
        if (expected == goal)
            return nx == (goal % MAP_COLS) * 16 + 8 &&
                   ny == (goal / MAP_COLS) * 16 + 8;
        return nx / 16 + (ny / 16) * MAP_COLS == expected;
    }
}

static void check_full_field(int goal)
{
    u8 before[sizeof(path_walkable)];
    memcpy(before, path_walkable, sizeof(before));
    int slot = sc64_route_slot(goal);
    assert(memcmp(before, path_walkable, sizeof(before)) == 0);
    assert(sc64_route_valid[slot] == 1);
    reference_bfs(goal);
    for (int cell = 0; cell < MAP_CELLS; ++cell) {
        int expected = distance[cell] < 0 ? SC64_ROUTE_UNREACHABLE :
                       cell == goal ? 0 : refdir[cell];
        assert(sc64_route_get(slot, cell) == expected);
    }
}

static void test_partial_and_packed_grid(void)
{
    const int goal = 64 + 64 * MAP_COLS;
    sc64_bitgrid_clear(path_walkable, MAP_CELLS);
    for (int y = 1; y < MAP_ROWS - 1; ++y)
        for (int x = 1; x < MAP_COLS - 1; ++x)
            if ((random32() % 100) < 68) set_open(x + y * MAP_COLS, 1);
    for (int x = 0; x < MAP_COLS; ++x) set_open(x + 64 * MAP_COLS, 1);
    for (int y = 0; y < MAP_ROWS; ++y) set_open(64 + y * MAP_COLS, 1);
    sc64_routes_reset();

    /* Repeated far and near origins compare each returned gradient with a
       complete BFS while exercising staged partial-field extension. */
    for (int i = 0; i < 48; ++i) {
        int start = (i & 1) ? (2 + 2 * MAP_COLS) : (125 + 125 * MAP_COLS);
        assert(route_matches_reference(start, goal));
    }
    assert(route_matches_reference(goal, goal));
    check_full_field(goal);

    /* A blocked origin needs all cardinal exits resolved before comparing
       gradients, including exits reached by going around the blocked cell. */
    memset(path_walkable, 0xff, sizeof(path_walkable));
    sc64_routes_reset();
    set_open(48 + 64 * MAP_COLS, 0);
    assert(route_matches_reference(48 + 64 * MAP_COLS, goal));
#ifdef SC64_ROUTE_PARTIAL_FLOOD
    {
        int slot = -1;
        for (int i = 0; i < SC64_ROUTE_SLOTS; ++i)
            if (sc64_route_valid[i] && sc64_route_goal[i] == goal) slot = i;
        assert(slot >= 0 && sc64_route_valid[slot] == 2);
    }
#endif
    check_full_field(goal);

    /* A blocked goal seeds its surrounding open cells in the original order. */
    set_open(goal, 0);
    sc64_routes_reset();
    assert(route_matches_reference(48 + 64 * MAP_COLS, goal));
    check_full_field(goal);

    /* One exit from a blocked start is disconnected. Unknown partial codes
       cannot be treated as unreachable until the flood really exhausts. */
    sc64_bitgrid_clear(path_walkable, MAP_CELLS);
    for (int x = 32; x <= 96; ++x) set_open(x + 64 * MAP_COLS, 1);
    set_open(48 + 64 * MAP_COLS, 0);
    sc64_routes_reset();
    assert(route_matches_reference(48 + 64 * MAP_COLS, goal));
    check_full_field(goal);

    /* Unreachable component, blocked goal, and all-blocked empty seed. */
    sc64_bitgrid_clear(path_walkable, MAP_CELLS);
    sc64_routes_reset();
    set_open(4 + 4 * MAP_COLS, 1);
    set_open(120 + 120 * MAP_COLS, 1);
    assert(route_matches_reference(4 + 4 * MAP_COLS, 120 + 120 * MAP_COLS));
    check_full_field(120 + 120 * MAP_COLS);
    sc64_bitgrid_clear(path_walkable, MAP_CELLS);
    sc64_routes_reset();
    assert(route_matches_reference(12 + 12 * MAP_COLS, 90 + 90 * MAP_COLS));
    check_full_field(90 + 90 * MAP_COLS);
}

#ifdef SC64_ROUTE_DECAY_FREQ
static void test_lfu_policy_edges(void)
{
    int i, goal = MAP_CELLS - 1;
    memset(path_walkable, 0xff, sizeof(path_walkable));

    /* Invalid entries beat every valid frequency. */
    sc64_routes_reset();
    for (i = 0; i < SC64_ROUTE_SLOTS; ++i) {
        sc64_route_valid[i] = 1;
        sc64_route_goal[i] = (u16)(i + 100);
        sc64_route_age[i] = sc64_route_age_pack(63u, (u32)(i + 10));
    }
    sc64_route_valid[7] = 0;
    (void)sc64_route_slot(goal);
    assert(sc64_route_goal[7] == goal);

    /* Lowest frequency wins; equal frequencies evict the oldest stamp. */
    sc64_routes_reset();
    for (i = 0; i < SC64_ROUTE_SLOTS; ++i) {
        sc64_route_valid[i] = 1;
        sc64_route_goal[i] = (u16)(i + 100);
        sc64_route_age[i] = sc64_route_age_pack(5u, (u32)(i + 100));
    }
    sc64_route_age[4] = sc64_route_age_pack(2u, 1000u);
    sc64_route_age[9] = sc64_route_age_pack(2u, 50u);
    (void)sc64_route_slot(goal);
    assert(sc64_route_goal[9] == goal);

    /* Equal frequency and stamp retain the first slot in scan order. */
    sc64_routes_reset();
    for (i = 0; i < SC64_ROUTE_SLOTS; ++i) {
        sc64_route_valid[i] = 1;
        sc64_route_goal[i] = (u16)(i + 100);
        sc64_route_age[i] = sc64_route_age_pack(5u, (u32)(i + 100));
    }
    sc64_route_age[3] = sc64_route_age_pack(1u, 50u);
    sc64_route_age[4] = sc64_route_age_pack(1u, 50u);
    (void)sc64_route_slot(goal);
    assert(sc64_route_goal[3] == goal);

    /* Decay runs before the 128th request; hits then increment to a cap. */
    sc64_routes_reset();
    for (i = 0; i < 2; ++i) {
        sc64_route_valid[i] = 1;
        sc64_route_goal[i] = (u16)i;
        sc64_route_age[i] = sc64_route_age_pack(8u, (u32)(i + 1));
    }
    sc64_route_clock = 126;
    (void)sc64_route_slot(0);
    assert(sc64_route_clock == 127);
    assert(sc64_route_age_freq(sc64_route_age[0]) == 9u);
    assert(sc64_route_age_freq(sc64_route_age[1]) == 8u);
    (void)sc64_route_slot(0);
    assert(sc64_route_clock == 128);
    assert(sc64_route_age_freq(sc64_route_age[0]) == 5u);
    assert(sc64_route_age_freq(sc64_route_age[1]) == 4u);
    sc64_route_age[0] = sc64_route_age_pack(63u, sc64_route_clock);
    (void)sc64_route_slot(0);
    assert(sc64_route_age_freq(sc64_route_age[0]) == 63u);

    sc64_routes_reset();
    assert(sc64_route_clock == 0);
    for (i = 0; i < SC64_ROUTE_SLOTS; ++i) {
        assert(!sc64_route_valid[i]);
        assert(sc64_route_age[i] == 0);
    }
}

static void test_decay_extension_and_wrap(void)
{
    int goal = 64 + 64 * MAP_COLS, nx, ny;
#ifndef SC64_ROUTE_PARTIAL_FLOOD
    int near_x = 66 * 16 + 8, near_y = 64 * 16 + 8;
#else
    int far_x = 8 * 16 + 8, far_y = 64 * 16 + 8;
#endif
    sc64_bitgrid_clear(path_walkable, MAP_CELLS);
    for (int x = 0; x < MAP_COLS; ++x) set_open(x + 64 * MAP_COLS, 1);
    sc64_routes_reset();
#ifdef SC64_ROUTE_PARTIAL_FLOOD
    /* Seed a known partial field through the origin-aware path used by the
       public route query, without forcing it complete through the legacy API. */
    int slot = sc64_route_slot_for_origin(goal, 66 + 64 * MAP_COLS, 1);
#else
    assert(sc64_route_next(near_x, near_y, goal % MAP_COLS * 16 + 8,
                           goal / MAP_COLS * 16 + 8, &nx, &ny));
    int slot = cached_slot_for_goal(goal);
#endif
    assert(slot >= 0);
    assert(sc64_route_valid[slot] ==
#ifdef SC64_ROUTE_PARTIAL_FLOOD
           2
#else
           1
#endif
    );
    sc64_route_age[slot] = sc64_route_age_pack(17u, 200u);

#ifdef SC64_ROUTE_PARTIAL_FLOOD
    assert(sc64_route_next(far_x, far_y, goal % MAP_COLS * 16 + 8,
                           goal / MAP_COLS * 16 + 8, &nx, &ny));
    slot = cached_slot_for_goal(goal);
    assert(slot >= 0);
    assert(sc64_route_age_freq(sc64_route_age[slot]) == 17u);
    assert(sc64_route_age_stamp(sc64_route_age[slot]) == sc64_route_clock);
#else
    /* LFU without partial flooding still preserves frequency on a hit. */
    assert(sc64_route_next(near_x, near_y, goal % MAP_COLS * 16 + 8,
                           goal / MAP_COLS * 16 + 8, &nx, &ny));
    assert(sc64_route_age_freq(sc64_route_age[slot]) == 18u);
#endif

#ifdef SC64_ROUTE_PARTIAL_FLOOD
    /* Force a real extension immediately before 26-bit recency rollover. */
    sc64_route_clock = SC64_ROUTE_RECENCY_MASK - 1u;
    assert(sc64_route_next(0 * 16 + 8, 64 * 16 + 8,
                           goal % MAP_COLS * 16 + 8, goal / MAP_COLS * 16 + 8,
                           &nx, &ny));
    slot = cached_slot_for_goal(goal);
    assert(slot >= 0);
    assert(sc64_route_age_freq(sc64_route_age[slot]) == 17u);
    assert(sc64_route_age_stamp(sc64_route_age[slot]) == SC64_ROUTE_RECENCY_MASK);
    assert(sc64_route_valid[slot] == 2);
    /* Next request invalidates the derived cache before starting a new epoch. */
    sc64_route_clock = SC64_ROUTE_RECENCY_MASK;
    assert(sc64_route_next(0 * 16 + 8, 64 * 16 + 8,
                           goal % MAP_COLS * 16 + 8, goal / MAP_COLS * 16 + 8,
                           &nx, &ny));
    slot = cached_slot_for_goal(goal);
    assert(slot >= 0);
    assert(sc64_route_age_freq(sc64_route_age[slot]) == 1u);
    assert(sc64_route_age_stamp(sc64_route_age[slot]) == 1u);
#endif
}
#endif

#ifdef SC64_ROUTE_NEW_CELL
/* A fresh cell can straddle bytes already holding arbitrary neighbouring
 * directions. Verify every bit phase, first/last cells and canaries. */
static void test_new_cell_writer(void)
{
    u8 packed[SC64_ROUTE_BYTES + 2];
    for(int trial=0;trial<256;trial++) {
        int cell=trial<8?trial:trial<16?MAP_CELLS-1-(trial-8):(int)(random32()%MAP_CELLS);
        int direction=1+(trial%4);
        for(unsigned i=0;i<SC64_ROUTE_BYTES;i++)sc64_route_field[0][i]=(u8)random32();
        sc64_route_put(0,cell,7);
        packed[0]=0x5a;packed[SC64_ROUTE_BYTES+1]=0xa5;
        memcpy(packed+1,sc64_route_field[0],SC64_ROUTE_BYTES);
        sc64_route_put(0,cell,direction);
        sc64_route_put_new(packed+1,cell,direction);
        assert(packed[0]==0x5a && packed[SC64_ROUTE_BYTES+1]==0xa5);
        assert(!memcmp(packed+1,sc64_route_field[0],SC64_ROUTE_BYTES));
    }
}
#endif

int main(void)
{
#ifdef SC64_ROUTE_NEW_CELL
    test_new_cell_writer();
#endif
    test_partial_and_packed_grid();
#ifdef SC64_ROUTE_DECAY_FREQ
    test_lfu_policy_edges();
    test_decay_extension_and_wrap();
#endif
    puts("128x128 packed route policies match BFS and restore the path grid");
    return 0;
}
