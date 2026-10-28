/* SPDX-License-Identifier: GPL-3.0-or-later
 * Alignment/remainder oracle for bounded asynchronous route snapshot copies. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
#define GBA_HOT
#ifndef SC64_RACES
#define SC64_RACES
#endif
static int race_cols = 128, race_rows = 128;
#define MAP_COLS race_cols
#define MAP_ROWS race_rows
enum { MAP_CELLS = 128 * 128 };
static u8 path_walkable[MAP_CELLS];
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid;
static int frame;
static void build_path_grid(void) { path_grid_valid = 1; }
static int limit(int n, int lo, int hi)
{
    return n < lo ? lo : (n > hi ? hi : n);
}
static int sc64_cell_row(unsigned cell) { return (int)cell / MAP_COLS; }
static int sc64_cell_col(unsigned cell) { return (int)cell % MAP_COLS; }
#define SC64_GRID_GET(grid, cell) ((grid)[cell] != 0)
#define SC64_GRID_SET(grid, cell, value) ((grid)[cell] = (u8)((value) != 0))
#define SC64_GRID_STORAGE_BYTES MAP_CELLS
#include "../starcraft/sc64_routes.h"

static void test_copy_alignment_and_remainders(void)
{
    u8 source[96], actual[96], expected[96];
    unsigned cases = 0;
    for (unsigned i = 0; i < sizeof(source); ++i)
        source[i] = (u8)(i * 37u + 19u);
    for (unsigned dst_alignment = 0; dst_alignment < 4; ++dst_alignment)
        for (unsigned src_alignment = 0; src_alignment < 4; ++src_alignment)
            for (unsigned bytes = 0; bytes <= 64; ++bytes)
                for (unsigned offset = 0; offset < 8; ++offset) {
                    unsigned dst_base = (dst_alignment -
                        ((uintptr_t)actual & 3u)) & 3u;
                    unsigned src_base = (src_alignment -
                        ((uintptr_t)source & 3u)) & 3u;
                    unsigned dst_offset = dst_base + offset * 4u;
                    unsigned src_offset = src_base + offset * 4u;
                    assert(dst_offset + bytes <= sizeof(actual));
                    assert(src_offset + bytes <= sizeof(source));
                    memset(actual, 0xa5, sizeof(actual));
                    memset(expected, 0xa5, sizeof(expected));
                    memcpy(expected + dst_offset, source + src_offset, bytes);
                    sc64_range_copy_bytes(actual + dst_offset,
                        source + src_offset, bytes);
                    assert(memcmp(actual, expected, sizeof(actual)) == 0);
                    ++cases;
                }
    assert(cases == 8320u);
    puts("PASS bounded route-copy alignment/remainder oracle: 8320 cases");
}

int main(void)
{
    test_copy_alignment_and_remainders();
    return 0;
}
