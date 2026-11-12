/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NATIVE_START_SELECTOR_H
#define SC64_NATIVE_START_SELECTOR_H

#include <stdint.h>

enum { SC64_NATIVE_START_SLOTS = 8, SC64_NATIVE_START_NONE = 0xff };

typedef struct {
    uint8_t marker_by_row[SC64_NATIVE_START_SLOTS];
    uint32_t seed_after;
} Sc64NativeMeleeStarts;

static inline unsigned sc64_native_start_count(unsigned mask)
{
    unsigned count = 0;
    for (unsigned slot = 0; slot < SC64_NATIVE_START_SLOTS; ++slot)
        count += (mask >> slot) & 1u;
    return count;
}

/* Reconstruct the Melee assignment from an explicit, pre-draw seed.
 * The caller supplies the native marker IDs admitted for this match and
 * an occupied-row mask including the human. Rows and markers are separate.
 * A failed request does not publish any output. */
static inline int sc64_native_melee_starts(uint32_t seed, unsigned rows, unsigned markers,
                                          Sc64NativeMeleeStarts *out)
{
    if (!out || !rows || ((rows | markers) & ~0xffu)) return 0;
    unsigned remaining = sc64_native_start_count(markers);
    if (sc64_native_start_count(rows) > remaining) return 0;

    /* Native copies active records in ascending row order, then consumes
     * that list backward. Draw from the ascending list of remaining IDs. */
    for (unsigned row = SC64_NATIVE_START_SLOTS; row-- > 0;) {
        out->marker_by_row[row] = SC64_NATIVE_START_NONE;
        if (!(rows & (1u << row))) continue;
        seed = seed * UINT32_C(0x015a4e35) + 1u;
        unsigned nth = (remaining * ((seed >> 16) & 0x7fffu)) >> 15;
        for (unsigned marker = 0; marker < SC64_NATIVE_START_SLOTS; ++marker) {
            unsigned bit = 1u << marker;
            if (!(markers & bit)) continue;
            if (nth == 0) {
                out->marker_by_row[row] = (uint8_t)marker;
                markers &= ~bit;
                --remaining;
                break;
            }
            --nth;
        }
    }
    out->seed_after = seed;
    return 1;
}
#endif
