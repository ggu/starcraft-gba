/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_native_start_selector.h"
#include "../common/gba_elapsed_clock.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_elapsed_clock(void)
{
    GbaElapsedClock clock = {0};
    assert(gba_elapsed_milliseconds_at(&clock) == 0);
    gba_elapsed_observe(&clock, (1u << 24) - 1u);
    assert(gba_elapsed_milliseconds_at(&clock) == 999);
    gba_elapsed_observe(&clock, 1u << 24);
    assert(gba_elapsed_milliseconds_at(&clock) == 1000);
    gba_elapsed_observe(&clock, UINT32_MAX);
    assert(gba_elapsed_milliseconds_at(&clock) == 255999);
    gba_elapsed_observe(&clock, 0);
    assert(gba_elapsed_milliseconds_at(&clock) == 256000);
    gba_elapsed_observe(&clock, 1u << 23);
    assert(gba_elapsed_milliseconds_at(&clock) == 256500);
    for (unsigned i = 0; i < 128; ++i) {
        gba_elapsed_observe(&clock, UINT32_MAX);
        gba_elapsed_observe(&clock, 0);
    }
    assert(gba_elapsed_milliseconds_at(&clock) == 129u * 256000u);
}

static void test_random_launch(void)
{
    const uint8_t x = SC64_NATIVE_START_NONE;
    const uint8_t r = SC64_NATIVE_RACE_RANDOM;
    const uint8_t z = SC64_NATIVE_RACE_ZERG;
    const uint8_t t = SC64_NATIVE_RACE_TERRAN;
    const uint8_t p = SC64_NATIVE_RACE_PROTOSS;
    const struct {
        unsigned rows;
        uint8_t choices[SC64_NATIVE_START_SLOTS];
        uint8_t races[4], starts[4];
        uint32_t placement_seed;
    } cases[] = {
        /* Independent native actor witness: Human Random resolved Zerg. */
        {15, {r,z,t,p}, {z,z,t,p}, {2,3,0,1}, 0xa39d7707u},
        /* Independent CPU Random witnesses: middle only and dense. */
        {5, {t,x,r,x}, {t,x,z,x}, {0,x,1,x}, 0xa39d7707u},
        {15, {t,r,r,p}, {t,z,t,p}, {1,2,3,0}, 0xb953c674u},
        /* Joint native witness checks the human draw order among ascending CPU rows. */
        {15, {r,r,r,p}, {z,t,z,p}, {2,0,1,3}, 0x1d976e05u},
        /* Additional gap and no-draw cases from the stock instructions. */
        {11, {t,r,x,r}, {t,z,x,t}, {2,3,x,0}, 0xb953c674u},
        {5, {t,r,z,r}, {t,x,z,x}, {2,x,1,x}, 0xf3aeu},
    };
    Sc64NativeMeleeLaunch out;
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        assert(sc64_native_melee_launch(0xf3ae, cases[i].rows, 15, cases[i].choices, &out));
        assert(out.placement_seed == cases[i].placement_seed);
        assert(!memcmp(out.race_by_row, cases[i].races, 4));
        assert(!memcmp(out.starts.marker_by_row, cases[i].starts, 4));
        for (unsigned j = 4; j < SC64_NATIVE_START_SLOTS; ++j)
            assert(out.race_by_row[j] == x && out.starts.marker_by_row[j] == x);
    }
    const uint8_t bad[SC64_NATIVE_START_SLOTS] = {t,r,4};
    Sc64NativeMeleeLaunch unchanged = out;
    assert(!sc64_native_melee_launch(0xf3ae, 7, 15, bad, &out));
    assert(!sc64_native_melee_launch(0xf3ae, 3, 1, bad, &out));
    assert(!sc64_native_melee_launch(0xf3ae, 0, 15, bad, &out));
    assert(out.placement_seed == unchanged.placement_seed &&
           out.starts.seed_after == unchanged.starts.seed_after &&
           !memcmp(out.race_by_row, unchanged.race_by_row, sizeof(out.race_by_row)) &&
           !memcmp(out.starts.marker_by_row, unchanged.starts.marker_by_row, sizeof(out.starts.marker_by_row)));
}

int main(void)
{
    test_elapsed_clock();
    test_random_launch();
    const struct {
        uint32_t seed;
        unsigned occupied;
        uint8_t marker[4];
    } native[] = {
        {0xf3ae, 5, {2, 255, 1, 255}}, /* middle only */
        {0xfc9c, 5, {2, 255, 1, 255}}, /* middle only, delayed */
        {0xf3ae, 3, {2, 1, 255, 255}}, /* first only */
        {0xf3ae, 9, {2, 255, 255, 1}}, /* third only */
        {0xf3ae, 15, {3, 0, 2, 1}}, /* three computers */
        {0xf3ae, 11, {0, 2, 255, 1}}, /* middle closed */
        {0xf3ae, 7, {0, 2, 1, 255}}, /* third closed */
        {0xf6e0, 15, {2, 1, 0, 3}}, /* three computers, alternate seed */
        {0xf618, 5, {1, 255, 3, 255}}, /* middle only, alternate seed */
        {0xa39d7707u, 15, {2, 3, 0, 1}}, /* Random human resolved Zerg: after one advance */
    };
    Sc64NativeMeleeStarts out;
    for (unsigned i = 0; i < sizeof(native) / sizeof(native[0]); ++i) {
        assert(sc64_native_melee_starts(native[i].seed, native[i].occupied, 15, &out));
        assert(!memcmp(out.marker_by_row, native[i].marker, sizeof(native[i].marker)));
        for (unsigned j = 4; j < SC64_NATIVE_START_SLOTS; ++j)
            assert(out.marker_by_row[j] == SC64_NATIVE_START_NONE);
    }
    assert(sc64_native_melee_starts(0, 1, 128, &out) &&
           out.marker_by_row[0] == 7 && out.seed_after == 1);
    Sc64NativeMeleeStarts unchanged = out;
    assert(!sc64_native_melee_starts(0, 0, 1, &out));
    assert(!sc64_native_melee_starts(0, 3, 1, &out));
    assert(!sc64_native_melee_starts(0, 256, 255, &out));
    assert(!sc64_native_melee_starts(0, 1, 257, &out));
    assert(!sc64_native_melee_starts(0, 1, 1, 0));
    assert(out.seed_after == unchanged.seed_after &&
           !memcmp(out.marker_by_row, unchanged.marker_by_row, sizeof(out.marker_by_row)));

    /* Non-contiguous map markers and rows may never create a duplicate
     * placement, assign a closed row or invent a source marker. */
    for (unsigned rows = 1; rows <= 255; ++rows)
        for (unsigned markers = 1; markers <= 255; ++markers) {
            int valid = sc64_native_start_count(rows) <= sc64_native_start_count(markers);
            assert(sc64_native_melee_starts(0x95d00bc1u, rows, markers, &out) == valid);
            if (!valid) continue;
            unsigned seen = 0;
            for (unsigned j = 0; j < SC64_NATIVE_START_SLOTS; ++j) {
                unsigned marker = out.marker_by_row[j];
                if (rows & (1u << j)) {
                    assert(marker < SC64_NATIVE_START_SLOTS);
                    unsigned bit = 1u << marker;
                    assert((markers & bit) && !(seen & bit));
                    seen |= bit;
                } else {
                    assert(marker == SC64_NATIVE_START_NONE);
                }
            }
        }
    puts("Native Melee start selector: qualified layouts and bounded slot masks passed");
    return 0;
}
