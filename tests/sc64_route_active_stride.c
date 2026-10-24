/* Asset-free active-stride cache checks against an independent reverse BFS. */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
#define GBA_HOT
#define SC64_RACES

static int race_cols = 64, race_rows = 64;
#define MAP_COLS race_cols
#define MAP_ROWS race_rows
enum { MAP_CELLS = 128 * 128 };

static u8 path_walkable[MAP_CELLS];
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid = 1;
static unsigned grid_reads;
static int limit(int n, int lo, int hi)
{
    return n < lo ? lo : (n > hi ? hi : n);
}
static int sc64_cell_row(unsigned cell) { return (int)cell / MAP_COLS; }
static int sc64_cell_col(unsigned cell) { return (int)cell % MAP_COLS; }
static void build_path_grid(void) { path_grid_valid = 1; }
#define SC64_GRID_GET(grid, cell) (++grid_reads, (grid)[cell] != 0)
#define SC64_GRID_SET(grid, cell, value) ((grid)[cell] = (u8)((value) != 0))

#include "../starcraft/sc64_routes.h"

static int distance[MAP_CELLS];
static u8 reference_direction[MAP_CELLS];
static u32 rng = 0x935ac321u;
static u32 random32(void) { rng = rng * 1664525u + 1013904223u; return rng; }

static unsigned active_cells(void)
{
    return (unsigned)MAP_COLS * (unsigned)MAP_ROWS;
}

static void reference_bfs(int goal)
{
    int head = 0, tail = 0, cells = (int)active_cells();
    for (int i = 0; i < cells; ++i) {
        distance[i] = -1;
        reference_direction[i] = SC64_ROUTE_UNREACHABLE;
    }
    if (path_walkable[goal]) {
        distance[goal] = 0;
        reference_direction[goal] = 0;
        bfs_queue[tail++] = (u16)goal;
    } else {
        int gx = goal % MAP_COLS, gy = goal / MAP_COLS;
        /* Match blocked-goal seed order: row-major eight-neighborhood. */
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            int x = gx + dx, y = gy + dy, cell;
            if ((!dx && !dy) || x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS)
                continue;
            cell = x + y * MAP_COLS;
            if (path_walkable[cell] && distance[cell] < 0) {
                distance[cell] = 0;
                reference_direction[cell] = 0;
                bfs_queue[tail++] = (u16)cell;
            }
        }
    }
    while (head < tail) {
        int cell = bfs_queue[head++], x = cell % MAP_COLS, y = cell / MAP_COLS;
        int neighbor[4] = { x ? cell - 1 : -1,
                            x + 1 < MAP_COLS ? cell + 1 : -1,
                            y ? cell - MAP_COLS : -1,
                            y + 1 < MAP_ROWS ? cell + MAP_COLS : -1 };
        for (int direction = 1; direction <= 4; ++direction) {
            int next = neighbor[direction - 1];
            if (next < 0 || !path_walkable[next] || distance[next] >= 0) continue;
            distance[next] = distance[cell] + 1;
            reference_direction[next] = (u8)direction;
            bfs_queue[tail++] = (u16)next;
        }
    }
}

static int expected_next(int sx, int sy, int gx, int gy, int *next_cell, int *is_goal)
{
    int start = sx + sy * MAP_COLS, goal = gx + gy * MAP_COLS;
    reference_bfs(goal);
    *is_goal = 0;
    if (path_walkable[start]) {
        if (distance[start] < 0) return 0;
        if (start == goal || reference_direction[start] == 0) {
            *next_cell = goal;
            *is_goal = 1;
            return 1;
        }
        switch (reference_direction[start]) {
        case 1: *next_cell = start + 1; break;
        case 2: *next_cell = start - 1; break;
        case 3: *next_cell = start + MAP_COLS; break;
        default: *next_cell = start - MAP_COLS; break;
        }
        return 1;
    }
    {
        static const int dx[4] = { 1, -1, 0, 0 };
        static const int dy[4] = { 0, 0, 1, -1 };
        int best = INT_MAX, found = 0;
        for (int i = 0; i < 4; ++i) {
            int x = sx + dx[i], y = sy + dy[i], cell, d;
            if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS) continue;
            cell = x + y * MAP_COLS;
            if (distance[cell] < 0) continue;
            d = distance[cell];
            if (d < best) { best = d; *next_cell = cell; found = 1; }
        }
        return found;
    }
}

static void assert_query_matches_bfs(int sx, int sy, int gx, int gy)
{
    int expected = -1, is_goal, nx = -1234, ny = -2345;
    u8 before[MAP_CELLS];
    int found = expected_next(sx, sy, gx, gy, &expected, &is_goal);
    memcpy(before, path_walkable, sizeof(before));
    int actual = sc64_route_next(sx * 16 + 8, sy * 16 + 8,
                                 gx * 16 + 8, gy * 16 + 8, &nx, &ny);
    assert(actual == found);
    assert(memcmp(before, path_walkable, sizeof(before)) == 0);
    if (!found) return;
    if (is_goal) assert(nx == gx * 16 + 8 && ny == gy * 16 + 8);
    else assert(nx / 16 + (ny / 16) * MAP_COLS == expected);
}

static void assert_complete_field(int gx, int gy)
{
    int goal = gx + gy * MAP_COLS;
    reference_bfs(goal);
    /* The no-origin wrapper forces completion even with partial flood enabled. */
    int slot = sc64_route_slot(goal);
    assert(slot >= 0 && sc64_route_valid[slot] == 1);
    for (int cell = 0; cell < (int)active_cells(); ++cell) {
        int expected = distance[cell] < 0 ? SC64_ROUTE_UNREACHABLE :
                       cell == goal ? 0 : reference_direction[cell];
        assert(sc64_route_get(slot, cell) == expected);
    }
}

static void set_shape(int cols, int rows, unsigned expected_bytes,
                      unsigned expected_slots)
{
    race_cols = cols;
    race_rows = rows;
    sc64_routes_reset();
    assert(sc64_route_active_bytes == expected_bytes);
    assert(sc64_route_active_slots == expected_slots);
    for (unsigned i = expected_slots; i < SC64_ROUTE_ACTIVE_MAX_SLOTS; ++i)
        assert(sc64_route_field[i] == 0);
    for (int i = 0; i < SC64_ROUTE_SLOTS; ++i) assert(sc64_route_valid[i] == 0);
}

static void assert_pool_canaries(void)
{
    unsigned cells = active_cells();
    memset(sc64_route_pool, 0xa5, sizeof(sc64_route_pool));
    for (unsigned slot = 0; slot < sc64_route_active_slots; ++slot) {
#ifdef SC64_ROUTE_WORD_CLEAR
        sc64_route_field_clear(sc64_route_field[slot]);
#else
        memset(sc64_route_field[slot], 0xff, sc64_route_active_bytes);
#endif
    }
    for (unsigned slot = 0; slot < sc64_route_active_slots; ++slot) {
        for (unsigned byte = 0; byte < sc64_route_active_bytes; ++byte)
            assert(sc64_route_field[slot][byte] == 0xff);
        sc64_route_put((int)slot, (int)cells - 1, (int)((slot + 1) & 7u));
    }
    for (unsigned slot = 0; slot < sc64_route_active_slots; ++slot) {
        assert(sc64_route_get((int)slot, (int)cells - 1) == (int)((slot + 1) & 7u));
        if (cells > 1) assert(sc64_route_get((int)slot, 0) == 7);
    }
    for (unsigned byte = sc64_route_active_bytes * sc64_route_active_slots;
         byte < sizeof(sc64_route_pool); ++byte)
        assert(sc64_route_pool[byte] == 0xa5);
}

static void assert_random_queries(unsigned count)
{
    unsigned cells = active_cells();
    for (unsigned i = 0; i < cells; ++i)
        path_walkable[i] = (u8)((random32() % 100u) < 76u);
    sc64_routes_reset();
    for (unsigned query = 0; query < count; ++query) {
        int sx = (int)(random32() % (unsigned)MAP_COLS);
        int sy = (int)(random32() % (unsigned)MAP_ROWS);
        int gx = (int)(random32() % (unsigned)MAP_COLS);
        int gy = (int)(random32() % (unsigned)MAP_ROWS);
        assert_query_matches_bfs(sx, sy, gx, gy);
    }
    assert_complete_field((MAP_COLS * 3) / 4, MAP_ROWS / 3);
}

int main(void)
{
    assert(sc64_route_configure());
    assert(sc64_route_active_bytes == 1536 && sc64_route_active_slots == 18);
    assert_random_queries(240);
    assert_pool_canaries();
    /* Open-grid ties exercise the exact root-BFS direction, including L-fast. */
    memset(path_walkable, 1, sizeof(path_walkable));
    sc64_routes_reset();
    for (int sy = 0; sy < MAP_ROWS; sy += 7)
        for (int sx = 0; sx < MAP_COLS; sx += 9)
            assert_query_matches_bfs(sx, sy, MAP_COLS - 1 - sx, MAP_ROWS - 1 - sy);

    set_shape(128, 96, 4608, 18);
    assert_random_queries(360);
    assert_pool_canaries();

    set_shape(128, 128, 6144, 14);
    assert_random_queries(360);
    assert_pool_canaries();

    set_shape(127, 127, 6049, 14);
    assert_pool_canaries();
    set_shape(64, 64, 1536, 18);
    assert_random_queries(180);
    assert_pool_canaries();

    {
        int nx = -7, ny = -9;
        static const int bad[][2] = {
            { 0, 64 }, { -1, 64 }, { 64, -1 }, { 128, 129 },
            { INT_MAX, INT_MAX }, { 64, INT_MIN }, { 129, 1 }
        };
        for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
            race_cols = bad[i][0];
            race_rows = bad[i][1];
            grid_reads = 0;
            assert(!sc64_route_next(8, 8, 24, 24, &nx, &ny));
            assert(nx == -7 && ny == -9);
            assert(grid_reads == 0);
            assert(sc64_route_active_slots == 0 && sc64_route_active_bytes == 0);
            for (int slot = 0; slot < SC64_ROUTE_SLOTS; ++slot)
                assert(sc64_route_field[slot] == 0 && sc64_route_valid[slot] == 0);
        }
    }
    puts("active-stride fields match tie-ordered BFS across shape changes and guard cases");
    return 0;
}
