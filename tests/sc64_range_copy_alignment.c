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
#if SC64_ROUTE_GRID_CELLS_PER_BYTE == 8
#define SC64_GRID_STORAGE_BYTES ((MAP_CELLS + 7u) / 8u)
#define SC64_GRID_OPEN_BYTE 255
#define SC64_GRID_GET(grid, cell) (((grid)[(cell) >> 3] >> ((cell) & 7u)) & 1u)
#define SC64_GRID_SET(grid, cell, value) do { \
    u8 mask_ = (u8)(1u << ((cell) & 7u)); \
    if (value) (grid)[(cell) >> 3] |= mask_; \
    else (grid)[(cell) >> 3] &= (u8)~mask_; \
} while (0)
#else
#define SC64_GRID_STORAGE_BYTES MAP_CELLS
#define SC64_GRID_OPEN_BYTE 1
#define SC64_GRID_GET(grid, cell) ((grid)[cell] != 0)
#define SC64_GRID_SET(grid, cell, value) ((grid)[cell] = (u8)((value) != 0))
#endif
static u8 path_walkable[SC64_GRID_STORAGE_BYTES];
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
#include "../starcraft/sc64_routes.h"
#ifdef SC64_TEST_FAR_READ_WITHOUT_MONOTONE_MOVEMENT
#if !defined(SC64_ROUTE_POINT_FAR_READ) || defined(SC64_ROUTE_MONOTONE_ENABLED)
#error "this fixture must use far cached reads with ordinary point movement"
#endif
#endif

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
        memset(path_walkable, SC64_GRID_OPEN_BYTE, sizeof(path_walkable)); path_grid_valid = 1;
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

#ifdef SC64_ROUTE_POINT_POINTER_PROBE
static void test_retained_point_with_range_planes(void)
{
    const int goal = 64 * 128 + 40;
    race_cols = race_rows = 128;
    memset(path_walkable, SC64_GRID_OPEN_BYTE, sizeof(path_walkable));
    path_grid_valid = 1;
    sc64_routes_reset();
    assert(sc64_route_pcount == 0);
    const int reduced = sc64_range_route_reduced_slots();
    u8 *first_scratch = sc64_route_field[reduced];
    u8 *last_scratch = sc64_route_field[SC64_ROUTE_SLOTS - 1];
    for (int i = 0; i <= SC64_ROUTE_SLOTS + 2; ++i)
        assert(sc64_route_slot(goal + i) >= 0);
    assert(sc64_route_point_saved >= 0);
    assert(sc64_route_field[reduced] == first_scratch &&
           sc64_route_field[SC64_ROUTE_SLOTS - 1] == last_scratch);
    assert(sc64_range_route_gradient_storage() == first_scratch &&
           sc64_range_route_frontier_storage() == last_scratch);
    sc64_range_route_claim_planes();
    assert(!sc64_route_valid[reduced] && !sc64_route_valid[SC64_ROUTE_SLOTS - 1]);
    const int saved_goal = sc64_route_point_saved;
    const u8 *saved_plane = sc64_route_point_spare;
    sc64_range_route_active.valid = 1;
    int slot = sc64_route_slot(saved_goal);
    assert(slot >= 0 && slot < reduced && sc64_route_field[slot] == saved_plane);
    assert(sc64_route_get(slot, saved_goal) == 0);
    assert(sc64_route_field[reduced] == first_scratch &&
           sc64_route_field[SC64_ROUTE_SLOTS - 1] == last_scratch);
    puts("PASS retained point backing moves while both claimed range planes stay fixed");
}
#endif

#ifdef SC64_ROUTE_POINT_PREFIX_PROBE
static void test_point_prefix_with_range_planes(void)
{
    const int goal = 32 + 32 * 128;
    int nx, ny;
    race_cols = race_rows = 128;
    memset(path_walkable, SC64_GRID_OPEN_BYTE, sizeof(path_walkable));
    path_grid_valid = 1;
    sc64_routes_reset();
    assert(!sc64_route_pcount);
    int slot = sc64_route_slot_for(goal, goal + 1);
    assert(slot >= 0 && sc64_point_prefix_find(slot));
    u32 builds = sc64_route_builds, hits = sc64_route_hits, clock = sc64_route_clock;
#ifdef SC64_ROUTE_POINT_COLD_CERTIFICATE
    assert(sc64_point_prefix_find(slot)->count == SC64_POINT_CERTIFIED_ONLY);
    assert(sc64_range_cached_point_step(520, 552, 520, 520, &nx, &ny));
    assert(nx == 520 && ny == 536 && sc64_point_prefix_find(slot));
#endif
    assert(sc64_range_cached_point_step(1608, 1608, 520, 520, &nx, &ny));
    assert(nx == 1608 && ny == 1592);
#ifdef SC64_ROUTE_POINT_FAR_READ
    assert(sc64_point_prefix_find(slot));
    SC64_ROUTE_EXACT(slot);
#endif
    assert(!sc64_point_prefix_find(slot));
    assert(sc64_route_builds == builds && sc64_route_hits == hits && sc64_route_clock == clock);

    int first_plane = sc64_range_route_reduced_slots();
    int second = goal + 20;
    (void)sc64_point_prefix_fill(first_plane, second, second + 1);
    sc64_route_valid[first_plane] = 1;
    sc64_route_goal[first_plane] = (u16)second;
    assert(sc64_point_prefix_find(first_plane));
    sc64_range_route_claim_planes();
    assert(!sc64_point_prefix_find(first_plane));
    puts("PASS cached point reads preserve logical counters and range claims retire parked fields");
}
#endif

#ifdef SC64_ROUTE_POINT_FAR_READ
static void test_far_cached_point_proof(void)
{
    const int origin = 100 + 100 * 128, goal = 32 + 32 * 128;
    race_cols = race_rows = 128;
    sc64_routes_reset();
    int direction;
    for (int horizontal = 0; horizontal < 2; ++horizontal) {
        memset(path_walkable, SC64_GRID_OPEN_BYTE, sizeof(path_walkable));
        path_grid_valid = 1;
        for (int i = 8; i < 123; ++i) {
            const int wall = horizontal ? i + 64 * 128 : 64 + i * 128;
            if (i != 110) SC64_GRID_SET(path_walkable, wall, 0);
        }
        assert(sc64_point_far_read(origin, goal, &direction));
        (void)sc64_route_fill(0, goal);
        assert(direction == sc64_route_get(0, origin));

        /* A second equal lower-bound crossing needs the ordered flood. */
        const int other = horizontal ? 22 + 64 * 128 : 64 + 22 * 128;
        SC64_GRID_SET(path_walkable, other, 1);
        assert(!sc64_point_far_read(origin, goal, &direction));
        SC64_GRID_SET(path_walkable, goal, 0);
        assert(!sc64_point_far_read(origin, goal, &direction));
    }

    u32 random = 0xb283a1e9u;
    unsigned certificates = 0;
    for (int i = 0; i < 128; ++i) {
        for (int cell = 0; cell < 128 * 128; ++cell) {
            random ^= random << 13;
            random ^= random >> 17;
            random ^= random << 5;
            SC64_GRID_SET(path_walkable, cell, (random & 31u) > 2);
        }
        int root = 19 + (i & 15) + (18 + (i >> 2)) * 128;
        int from = 83 - (i & 15) + (81 - (i >> 2)) * 128;
        if (i & 1) { int swap = root; root = from; from = swap; }
        SC64_GRID_SET(path_walkable, root, 1);
        SC64_GRID_SET(path_walkable, from, 1);
        if (!sc64_point_far_read(from, root, &direction)) continue;
        (void)sc64_route_fill(0, root);
        assert(direction == sc64_route_get(0, from));
        ++certificates;
    }
    assert(certificates > 64);
    printf("PASS far exact route matches unique wall crossings and %u randomized full fields\n",
           certificates);
}
#endif

int main(void)
{
    test_copy_alignment_and_remainders();
#ifdef SC64_ROUTE_POINT_POINTER_PROBE
    test_retained_point_with_range_planes();
#endif
#ifdef SC64_ROUTE_POINT_PREFIX_PROBE
    test_point_prefix_with_range_planes();
#endif
#ifdef SC64_ROUTE_POINT_FAR_READ
    test_far_cached_point_proof();
#endif
#if !defined(SC64_ROUTE_POINT_POINTER_PROBE) && !defined(SC64_ROUTE_POINT_PREFIX_PROBE)
    test_lazy_plane_reservation();
#endif
    return 0;
}
