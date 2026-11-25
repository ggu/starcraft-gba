/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gba.h"

#ifndef MAX_MINERALS
#define MAX_MINERALS 144
#endif
#define SC64_MINERAL_MEMORY_BYTES ((MAX_MINERALS + 7u) / 8u)
#define MAX_RACE_GEYSERS 12
#define SC64_RACE_MAP_COUNT 30
#define SC64_IMPORTED_SKIRMISH
#define SC64_SKIRMISH_IMPORTED_COUNT 2
#include "../starcraft/sc64_mineral_memory_wire.h"

/* Independent wire specification: slot range, state word, first bit.  The
 * saved GBA representation is little-endian, so compare the physical bytes,
 * not a second invocation of the codec or its decoding logic. */
static void expected_wire_bytes(u8 *out, const int legacy[32], const u8 *memory)
{
    static const struct { unsigned first, count, word, bit; } segments[] = {
        {0, 31, 0, 1}, {31, 14, 1, 2}, {45, 26, 2, 6},
        {71, 30, 4, 2}, {101, 28, 16, 4}, {129, 24, 20, 8},
    };
    memcpy(out, legacy, sizeof(int) * 32u);
    out[1u * sizeof(int) + 2u] = 0x4d;
    out[1u * sizeof(int) + 3u] = 0x4d;
    for (unsigned i=0; i<sizeof(segments)/sizeof(segments[0]); ++i) {
        for (unsigned j=0; j<segments[i].count; ++j) {
            unsigned slot=segments[i].first+j, bit=segments[i].bit+j;
            if (slot < MAX_MINERALS && (memory[slot>>3] & (1u << (slot&7))))
                out[segments[i].word*sizeof(int) + bit/8u] |= (u8)(1u << (bit&7u));
        }
    }
}

int main(void)
{
    const int legacy[32] = {
        [0] = 1, [1] = 2, [2] = 29, [3] = 31, [4] = 3,
        [7] = 172454, [16] = 12, [20] = 2, [31] = -1,
    };
    int state[32];
    u8 expected[SC64_MINERAL_MEMORY_BYTES], actual[SC64_MINERAL_MEMORY_BYTES];
    u8 golden[sizeof(state)];

    memcpy(state, legacy, sizeof(state));
    assert(sc64_mineral_memory_wire(state, actual, 0) == 0);
    assert(memcmp(state, legacy, sizeof(state)) == 0);
    for (unsigned slot = 0; slot < MAX_MINERALS + 2u; ++slot) {
        memcpy(state, legacy, sizeof(state));
        memset(expected, 0, sizeof(expected));
        if (slot < MAX_MINERALS)
            expected[slot >> 3] = (u8)(1u << (slot & 7u));
        else if (slot == MAX_MINERALS)
            for (unsigned i = 0; i < MAX_MINERALS; ++i)
                expected[i >> 3] |= (u8)(1u << (i & 7u));

        expected_wire_bytes(golden, legacy, expected);
        assert(sc64_mineral_memory_wire(state, expected, 1) == 1);
        assert(memcmp(state, golden, sizeof(state)) == 0);
        assert(state[3] == legacy[3] && state[7] == legacy[7] && state[31] == legacy[31]);
        assert(sc64_mineral_memory_wire(state, actual, 0) == 1);
        assert(memcmp(expected, actual, sizeof(expected)) == 0);
        assert(memcmp(state, legacy, sizeof(state)) == 0);
    }

    assert(sc64_mineral_memory_wire(state, expected, 1) == 1);
    state[20] |= 1 << 30;
    assert(sc64_mineral_memory_wire(state, actual, 0) < 0);
    memcpy(state, legacy, sizeof(state));
    state[1] |= 1 << 27;
    assert(sc64_mineral_memory_wire(state, actual, 0) < 0);
    memcpy(state, legacy, sizeof(state));
    state[16] = 16;
    assert(sc64_mineral_memory_wire(state, expected, 1) < 0);

    printf("Mineral memory wire: %u bits, legacy fields and malformed records passed\n",
           (unsigned)MAX_MINERALS);
    return 0;
}
