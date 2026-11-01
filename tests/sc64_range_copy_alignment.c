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

/* Exercise actual dequeue/claim/pump and byte-interval aliasing across map
 * geometries. The queue alone cannot reserve the scratch planes. */
static void test_lazy_plane_reservation(void)
{
    static const int shapes[][2] = {{128,128}, {127,127}, {128,100}, {97,97}, {64,64}};
    for (unsigned shape = 0; shape < sizeof(shapes) / sizeof(shapes[0]); ++shape) {
        int full, reduced, next_col, next_row;
        race_cols = shapes[shape][0]; race_rows = shapes[shape][1];
        memset(path_walkable, 1, sizeof(path_walkable)); path_grid_valid = 1;
        sc64_routes_reset();
#ifdef SC64_ROUTE_ACTIVE_STRIDE
        assert(sc64_route_configure());
        full = (int)sc64_route_active_slots;
#else
        full = SC64_ROUTE_SLOTS;
#endif
        reduced = sc64_range_route_reduced_slots();
        assert(SC64_ROUTE_ACTIVE_SLOTS == full && reduced <= full);
        for (int i = 0; i < full; ++i) {
            sc64_route_valid[i] = 1;
            sc64_route_age[i] = i + 1;
            sc64_route_put(i, 0, 2);
#ifdef SC64_ROUTE_ACTIVE_STRIDE
            assert(((unsigned)(sc64_route_field[i] - sc64_route_pool) +
                    sc64_route_active_bytes <=
                    SC64_ROUTE_POOL_BYTES - 2u * SC64_ROUTE_BYTES) == (i < reduced));
#endif
        }
        assert(sc64_range_route_next(1, 160, 160, 0, 64, 2, 2,
                                     &next_col, &next_row) == SC64_RANGE_ROUTE_PENDING);
        assert(sc64_range_route_queue_count == 1 && SC64_ROUTE_ACTIVE_SLOTS == full);
        sc64_range_route_tick();
        assert(sc64_range_route_active.valid && SC64_ROUTE_ACTIVE_SLOTS == reduced);
        for (int i = 0; i < full; ++i) {
            if (i < reduced) {
                assert(sc64_route_valid[i] == 1 && sc64_route_get(i, 0) == 2);
            } else assert(sc64_route_valid[i] == 0 && sc64_route_age[i] == 0);
        }
        /* Reconfigure with unchanged dimensions while active must not erase
         * the job, and reset must release rather than resurrect tail fields. */
#ifdef SC64_ROUTE_ACTIVE_STRIDE
        assert(sc64_route_configure() && sc64_range_route_active.valid);
#endif
        sc64_range_route_reset();
        assert(SC64_ROUTE_ACTIVE_SLOTS == full);
        for (int i = reduced; i < full; ++i) assert(!sc64_route_valid[i]);
        if (reduced > 0 && reduced < full) {
            /* Retained snapshots in safe slots survive; a tagged copy in an
             * overlapping slot cannot remain pinned after the claim. */
            sc64_range_route_snapshot_slot = reduced - 1;
            sc64_range_route_snapshot_key.valid = 1;
            sc64_route_valid[reduced - 1] = 3;
            sc64_range_route_copy_slot = full - 1;
            sc64_route_valid[full - 1] = 3;
            sc64_range_route_copy_cursor = 17;
            sc64_range_route_claim_planes();
            assert(sc64_range_route_snapshot_slot == reduced - 1 &&
                   sc64_range_route_snapshot_key.valid && sc64_route_valid[reduced - 1] == 3);
            assert(sc64_range_route_copy_slot == -1 && !sc64_range_route_copy_cursor &&
                   !sc64_route_valid[full - 1]);
            sc64_range_route_snapshot_invalidate();
            sc64_range_route_snapshot_slot = full - 1;
            sc64_range_route_snapshot_key.valid = 1;
            sc64_route_valid[full - 1] = 3;
            sc64_range_route_claim_planes();
            assert(sc64_range_route_snapshot_slot == -1 &&
                   !sc64_range_route_snapshot_key.valid && !sc64_route_valid[full - 1]);
        }
    }
    puts("PASS lazy route-plane reservation and geometry alias oracle");
}

int main(void)
{
    test_copy_alignment_and_remainders();
    test_lazy_plane_reservation();
    return 0;
}
