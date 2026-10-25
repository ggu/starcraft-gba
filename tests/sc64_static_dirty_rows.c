/* Asset-free checks for the bounded dirty-world-row to screen-band planner. */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "../starcraft/sc64_static_dirty_rows.h"

static void
test_merges_contiguous_rows_and_keeps_gaps(void)
{
    uint8_t dirty[4 * 6] = {0};
    Sc64StaticDirtyRowsPlan plan;

    dirty[1 * 4 + 0] = 1;
    dirty[2 * 4 + 3] = 1;
    dirty[4 * 4 + 2] = 1;
    assert(sc64_static_dirty_rows_plan(dirty, 4, 6, 5, 84,
                                       12, 120, &plan));
    assert(plan.count == 2);
    assert(plan.bands[0].top == 24 && plan.bands[0].bottom == 56);
    assert(plan.bands[1].top == 72 && plan.bands[1].bottom == 88);
}

static void
test_camera_fraction_clips_cache_edges(void)
{
    uint8_t dirty[17 * 8] = {0};
    Sc64StaticDirtyRowsPlan plan;

    dirty[0] = 1;
    dirty[7 * 17 + 16] = 1;
    assert(sc64_static_dirty_rows_plan(dirty, 17, 8, 0, 15,
                                       12, 120, &plan));
    assert(plan.count == 2);
    assert(plan.bands[0].top == 12 && plan.bands[0].bottom == 13);
    assert(plan.bands[1].top == 109 && plan.bands[1].bottom == 120);

    /* A caller that does not own the complete cached map band can only
     * receive the part of each dirty row inside its clip. */
    memset(dirty, 0, sizeof(dirty));
    dirty[1 * 17] = 1;
    dirty[2 * 17 + 16] = 1;
    assert(sc64_static_dirty_rows_plan(dirty, 17, 8, 0, 15,
                                       20, 40, &plan));
    assert(plan.count == 1);
    assert(plan.bands[0].top == 20 && plan.bands[0].bottom == 40);
}

static void
test_empty_and_invalid_views_fail_closed(void)
{
    uint8_t dirty[17 * 9] = {0};
    Sc64StaticDirtyRowsPlan plan;

    assert(!sc64_static_dirty_rows_plan(dirty, 17, 9, 0, 0,
                                        12, 120, &plan));
    dirty[17 * 8] = 1;
    assert(!sc64_static_dirty_rows_plan(dirty, 17, 9, 0, 0,
                                        120, 12, &plan));
    assert(!sc64_static_dirty_rows_plan(dirty, 18, 9, 0, 0,
                                        12, 120, &plan));
    assert(!sc64_static_dirty_rows_plan(dirty, 17, 10, 0, 0,
                                        12, 120, &plan));
    assert(!sc64_static_dirty_rows_plan(dirty, 17, 9, -1, 0,
                                        12, 120, &plan));
    assert(!sc64_static_dirty_rows_plan(dirty, 17, 9, 0, -1,
                                        12, 120, &plan));
    assert(!sc64_static_dirty_rows_plan(dirty, 17, 9, INT_MAX, INT_MAX,
                                        12, 120, &plan));
    assert(!sc64_static_dirty_rows_plan(dirty, 17, 9, 0, INT_MAX,
                                        12, 120, &plan));
    assert(!sc64_static_dirty_rows_plan(NULL, 17, 9, 0, 0,
                                        12, 120, &plan));
    assert(plan.count == 0);
}

static void
test_all_row_masks_and_camera_phases(void)
{
    uint8_t dirty[17 * 9];
    Sc64StaticDirtyRowsPlan plan;

    for (int phase = 0; phase < 16; ++phase)
        for (unsigned mask = 0; mask < (1u << 9); ++mask) {
            int camy = 16 * 100 + phase;
            unsigned expected = 0;
            memset(dirty, 0, sizeof(dirty));
            for (unsigned row = 0; row < 9; ++row)
                dirty[row * 17] = (uint8_t)((mask >> row) & 1u);

            for (unsigned row = 0; row < 9;) {
                if (!(mask & (1u << row))) {
                    ++row;
                    continue;
                }
                unsigned first = row;
                while (row < 9 && (mask & (1u << row))) ++row;
                int top = 12 - phase + (int)first * 16;
                int bottom = 12 - phase + (int)row * 16;
                if (top < 12) top = 12;
                if (bottom > 120) bottom = 120;
                if (top < bottom) ++expected;
            }
            assert(sc64_static_dirty_rows_plan(dirty, 17, 9, camy / 16,
                                               camy, 12, 120, &plan) ==
                   (expected != 0));
            assert(plan.count == expected);
            for (unsigned band = 0; band < plan.count; ++band) {
                assert(plan.bands[band].top >= 12);
                assert(plan.bands[band].bottom <= 120);
                assert(plan.bands[band].top < plan.bands[band].bottom);
                if (band)
                    assert(plan.bands[band - 1].bottom < plan.bands[band].top);
            }
        }
}

int
main(void)
{
    test_merges_contiguous_rows_and_keeps_gaps();
    test_camera_fraction_clips_cache_edges();
    test_empty_and_invalid_views_fail_closed();
    test_all_row_masks_and_camera_phases();
    return 0;
}
