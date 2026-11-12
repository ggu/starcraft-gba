/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_native_start_selector.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
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
