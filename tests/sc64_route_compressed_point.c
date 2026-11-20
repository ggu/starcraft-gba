/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#include "../starcraft/sc64_route_compressed_point.h"

enum { WORLD_SIDE = 128, CELLS = WORLD_SIDE * WORLD_SIDE };
_Alignas(u32) static u8 grid[CELLS / 8];
static u8 reference[CELLS];
static u16 queue[CELLS];
static u32 random_state = 190873;
static unsigned exact_count, fallback_count, disconnected_count;

static unsigned
random_below(unsigned n)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state % n;
}

static int
open_cell(unsigned position)
{
    return (grid[position / 8] >> (position % 8)) & 1;
}

static void
block_cell(unsigned position)
{
    grid[position / 8] &= (u8)~(1u << (position % 8));
}

static void
golden_field(unsigned root)
{
    memset(reference, 7, sizeof(reference));
    reference[root] = 0;
    queue[0] = (u16)root;
    unsigned tail = 1;
    for (unsigned head = 0; head < tail; ++head) {
        int cell = queue[head];
        int x = cell % WORLD_SIDE, y = cell / WORLD_SIDE;
        const int dx[4] = {-1, 1, 0, 0}, dy[4] = {0, 0, -1, 1};
        for (unsigned d = 0; d < 4; ++d) {
            int xx = x + dx[d], yy = y + dy[d];
            if ((unsigned)xx >= WORLD_SIDE || (unsigned)yy >= WORLD_SIDE) continue;
            unsigned next = (unsigned)(xx + yy * WORLD_SIDE);
            if (!open_cell(next) || reference[next] != 7) continue;
            reference[next] = (u8)(d + 1);
            queue[tail++] = (u16)next;
        }
    }
}

static int
check(unsigned origin, unsigned root)
{
    u16 guarded[2 * SC64_COMPRESSED_LIMIT + 2];
    guarded[0] = guarded[2 * SC64_COMPRESSED_LIMIT + 1] = 0x1357;
    int actual = sc64_compressed_point_direction(grid, origin, root, guarded + 1);
    assert(guarded[0] == 0x1357 && guarded[2 * SC64_COMPRESSED_LIMIT + 1] == 0x1357);
    if (actual < 0) ++fallback_count;
    else {
        assert(actual == reference[origin]);
        ++exact_count;
        disconnected_count += actual == 7;
    }
    return actual;
}

int
main(void)
{
    unsigned root = 32 + 32 * WORLD_SIDE, origin = 100 + 100 * WORLD_SIDE;
    memset(grid, 0xff, sizeof(grid));
    for (unsigned y = 8; y < 123; ++y)
        if (y != 110) block_cell(64 + y * WORLD_SIDE);
    golden_field(root);
    assert(check(origin, root) == 3);
    assert(check(root, root) == -1);
    assert(check(64 + 8 * WORLD_SIDE, root) == -1);

    for (unsigned y = 0; y < WORLD_SIDE; ++y) block_cell(64 + y * WORLD_SIDE);
    golden_field(root);
    assert(check(origin, root) == 7);

    /* Use irregular repeated bands whose boundaries cross the 32-bit word
     * seams. Most reduce to small weighted graphs. Dense boards must fall
     * back, rather than take a slow large Cartesian product. */
    const unsigned cuts[5] = {0, 31, 64, 97, 128};
    for (unsigned trial = 0; trial < 300; ++trial) {
        memset(grid, 0xff, sizeof(grid));
        for (unsigned y = 0; y < 4; ++y)
            for (unsigned x = 0; x < 4; ++x) {
                if (random_below(4)) continue;
                for (unsigned yy = cuts[y]; yy < cuts[y + 1]; ++yy)
                    for (unsigned xx = cuts[x]; xx < cuts[x + 1]; ++xx)
                        block_cell(xx + yy * WORLD_SIDE);
            }
        root = random_below(CELLS);
        if (!open_cell(root)) continue;
        golden_field(root);
        for (unsigned query = 0; query < 12; ++query) {
            origin = random_below(CELLS);
            if (origin != root && open_cell(origin)) check(origin, root);
        }
    }
    memset(grid, 0xff, sizeof(grid));
    for (unsigned i = 1; i < CELLS - 1; i += 37) block_cell(i);
    golden_field(0);
    assert(check(CELLS - 1, 0) == -1);
    assert(exact_count > 1000 && disconnected_count > 10);
    printf("Compressed point directions: %u exact, %u disconnected, %u fallbacks\n",
           exact_count, disconnected_count, fallback_count);
    return 0;
}
