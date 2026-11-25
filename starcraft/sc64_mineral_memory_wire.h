/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MINERAL_MEMORY_WIRE_H
#define SC64_MINERAL_MEMORY_WIRE_H

/* Marker and 144 memory bits share only the otherwise invalid high bits of
 * bounded RaceSave state words. The record size and envelope stay unchanged. */
#include "sc64_mineral_memory_marker.h"
#define SC64_MINERAL_MEMORY_WIRE_BITS 144u
_Static_assert(MAX_MINERALS <= 144, "mineral memory wire capacity");
_Static_assert(SC64_RACE_MAP_COUNT <= 64, "mineral memory mission bits");
_Static_assert(MAX_RACE_GEYSERS <= 15, "mineral memory geyser bits");
#ifdef SC64_IMPORTED_SKIRMISH
_Static_assert(SC64_SKIRMISH_IMPORTED_COUNT <= 255, "mineral memory map bits");
#endif

static int sc64_mineral_memory_wire(int state[32], u8 *memory, int encode)
{
    static const u8 index[6] = {0, 1, 2, 4, 16, 20};
    static const u8 low[6] = {1, 2, 6, 2, 4, 8};
    u32 words[6]; unsigned bit = 0;
    if (!state || !memory) return -1;
    for (unsigned i=0; i<6; ++i) memcpy(&words[i], &state[index[i]], sizeof(u32));
    if (!encode) {
        if ((words[1] & 0xffff0000u) == 0) return 0;
        if ((words[1] & 0xffff0000u) != SC64_MINERAL_MEMORY_MAGIC) return -1;
        memset(memory, 0, SC64_MINERAL_MEMORY_BYTES);
    } else {
        for (unsigned i=0; i<6; ++i) {
            if (words[i] >> low[i]) return -1;
        }
    }
    for (unsigned i=0; i<6; ++i) {
        u32 mask = encode ? 0u : ((u32)1u << low[i])-1u;
        for (unsigned b=low[i]; b<32; ++b) {
            if (i==1 && b>=16) continue;
            unsigned value;
            if (encode) {
                value = bit < MAX_MINERALS ? (memory[bit>>3] >> (bit&7)) & 1u : 0u;
                if (value) mask |= (u32)1u << b;
            } else {
                value = (words[i] >> b) & 1u;
                if (bit < MAX_MINERALS) memory[bit>>3] |= (u8)(value << (bit&7));
                else if (value) return -1;
            }
            ++bit;
        }
        words[i] = encode ? (words[i] & (((u32)1u << low[i])-1u)) | mask : words[i] & mask;
    }
    if (bit < SC64_MINERAL_MEMORY_WIRE_BITS) return -1;
    if (encode) words[1] |= SC64_MINERAL_MEMORY_MAGIC;
    for (unsigned i=0; i<6; ++i) memcpy(&state[index[i]], &words[i], sizeof(u32));
    return 1;
}
#endif
