/* Focused PACK3 route-field parity fixture for the optional aligned clear. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
#define GBA_HOT
#define SC64_RACES
#define SC64_ROUTE_PACK3
#ifndef ROUTE_TEST_COLS
#define ROUTE_TEST_COLS 128
#endif
#ifndef ROUTE_TEST_ROWS
#define ROUTE_TEST_ROWS 128
#endif
enum { MAP_COLS = ROUTE_TEST_COLS, MAP_ROWS = ROUTE_TEST_ROWS,
       MAP_CELLS = MAP_COLS * MAP_ROWS };
static u8 path_walkable[MAP_CELLS];
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid = 1;
static int limit(int n, int lo, int hi) { return n < lo ? lo : (n > hi ? hi : n); }
static int sc64_cell_col(unsigned cell) { return (int)(cell % MAP_COLS); }
static int sc64_cell_row(unsigned cell) { return (int)(cell / MAP_COLS); }
static void build_path_grid(void) { path_grid_valid = 1; }
#include "starcraft/sc64_routes.h"

static u8 reference[MAP_CELLS];
static u32 rng = 0x51c64a31u;
static u32 random32(void) { rng = rng * 1664525u + 1013904223u; return rng; }

static void reference_field(int goal)
{
    int head = 0, tail = 0, dx, dy;
    memset(reference, SC64_ROUTE_UNREACHABLE, sizeof(reference));
    if (path_walkable[goal]) {
        reference[goal] = 0;
        bfs_queue[tail++] = (u16)goal;
    } else {
        int gx = goal % MAP_COLS, gy = goal / MAP_COLS;
        for (dy = -1; dy <= 1; ++dy) for (dx = -1; dx <= 1; ++dx) {
            int x = gx + dx, y = gy + dy, cell;
            if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS) continue;
            cell = x + y * MAP_COLS;
            if (path_walkable[cell] && reference[cell] == SC64_ROUTE_UNREACHABLE) {
                reference[cell] = 0;
                bfs_queue[tail++] = (u16)cell;
            }
        }
    }
    while (head < tail) {
        int cell = bfs_queue[head++], x = cell % MAP_COLS, y = cell / MAP_COLS;
        int next[4] = { x > 0 ? cell - 1 : -1,
                        x + 1 < MAP_COLS ? cell + 1 : -1,
                        y > 0 ? cell - MAP_COLS : -1,
                        y + 1 < MAP_ROWS ? cell + MAP_COLS : -1 };
        int back[4] = { 1, 2, 3, 4 };
        int n;
        for (n = 0; n < 4; ++n) {
            int candidate = next[n];
            if (candidate >= 0 && path_walkable[candidate] &&
                reference[candidate] == SC64_ROUTE_UNREACHABLE) {
                reference[candidate] = (u8)back[n];
                bfs_queue[tail++] = (u16)candidate;
            }
        }
    }
}

static void test_clear_bytes_and_canaries(void)
{
    u8 storage[SC64_ROUTE_BYTES + 8] __attribute__((aligned(4)));
    unsigned offset, i;
    for (offset = 0; offset < 4; ++offset) {
        memset(storage, 0x5a, sizeof(storage));
        sc64_route_field_clear(storage + offset);
        for (i = 0; i < offset; ++i) assert(storage[i] == 0x5a);
        for (i = offset; i < offset + SC64_ROUTE_BYTES; ++i)
            assert(storage[i] == 0xff);
        for (i = offset + SC64_ROUTE_BYTES; i < sizeof(storage); ++i)
            assert(storage[i] == 0x5a);
    }
    /* Non-four-byte strides make later real slot rows unaligned. */
    if (SC64_ROUTE_BYTES & 3u) {
        u8 *row = sc64_route_field[1];
        assert(((uintptr_t)row & 3u) != 0u);
        memset(row, 0x5a, SC64_ROUTE_BYTES);
        sc64_route_field_clear(row);
        for (i = 0; i < SC64_ROUTE_BYTES; ++i) assert(row[i] == 0xff);
    }
}

static void test_route_field_parity(void)
{
    int query, nx, ny;
    for (query = 0; query < 24; ++query) {
        int goal, slot, cell;
        for (cell = 0; cell < MAP_CELLS; ++cell)
            path_walkable[cell] = (u8)((random32() % 100u) >= 27u);
        goal = (int)(random32() % MAP_CELLS);
        reference_field(goal);
        sc64_routes_reset();
        slot = sc64_route_slot(goal);
        for (cell = 0; cell < MAP_CELLS; ++cell)
            assert(sc64_route_get(slot, cell) == reference[cell]);
    }
    memset(path_walkable, 0, sizeof(path_walkable));
    path_walkable[MAP_CELLS - 1] = 1;
    reference_field(0); /* blocked goal seeds the walkable adjacent frontier */
    sc64_routes_reset();
    {
        int slot = sc64_route_slot(0), cell;
        for (cell = 0; cell < MAP_CELLS; ++cell)
            assert(sc64_route_get(slot, cell) == reference[cell]);
    }
    memset(path_walkable, 1, sizeof(path_walkable));
    sc64_routes_reset();
    assert(sc64_route_next(8, 8, (MAP_COLS - 1) * 16 + 8,
                           (MAP_ROWS - 1) * 16 + 8, &nx, &ny));
    assert((nx == 24 && ny == 8) || (nx == 8 && ny == 24));
}

int main(void)
{
#if ROUTE_TEST_COLS == 128 && ROUTE_TEST_ROWS == 128
    _Static_assert(SC64_ROUTE_BYTES == 6144, "fixture exercises 128x128 packed field");
#elif ROUTE_TEST_COLS == 128 && ROUTE_TEST_ROWS == 96
    _Static_assert(SC64_ROUTE_BYTES == 4608, "fixture exercises 128x96 packed field");
#elif ROUTE_TEST_COLS == 10 && ROUTE_TEST_ROWS == 10
    _Static_assert(SC64_ROUTE_BYTES == 38, "fixture exercises unaligned 38-byte rows");
#endif
    test_clear_bytes_and_canaries();
    test_route_field_parity();
    puts("aligned route clear preserves bytes and PACK3 fields");
    return 0;
}
