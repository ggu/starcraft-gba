/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NATIVE_START_SELECTOR_H
#define SC64_NATIVE_START_SELECTOR_H

#include <stdint.h>

enum { SC64_NATIVE_START_SLOTS = 8, SC64_NATIVE_START_NONE = 0xff };

typedef struct {
    uint8_t marker_by_row[SC64_NATIVE_START_SLOTS];
    uint32_t seed_after;
} Sc64NativeMeleeStarts;

enum {
    SC64_NATIVE_RACE_ZERG = 0, SC64_NATIVE_RACE_TERRAN = 1, SC64_NATIVE_RACE_PROTOSS = 2,
    SC64_NATIVE_RACE_RANDOM = 6
};

typedef struct {
    Sc64NativeMeleeStarts starts;
    uint32_t placement_seed;
    uint8_t race_by_row[SC64_NATIVE_START_SLOTS];
} Sc64NativeMeleeLaunch;

static inline unsigned sc64_native_melee_draw(uint32_t *state, unsigned count)
{
    *state = *state * UINT32_C(0x015a4e35) + 1u;
    return (count * ((*state >> 16) & 0x7fffu)) >> 15;
}

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
        unsigned nth = sc64_native_melee_draw(&seed, remaining);
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

/* Native Melee resolves Random races in ascending occupied-row order
 * before drawing starts from the same stream. Named and closed rows do
 * not draw. The separate placement entry still accepts a pre-draw seed. */
static inline int sc64_native_melee_launch(uint32_t seed, unsigned rows, unsigned markers,
                                           const uint8_t *race_by_row, Sc64NativeMeleeLaunch *out)
{
    if (!out || !race_by_row || !rows || ((rows | markers) & ~0xffu) ||
        sc64_native_start_count(rows) > sc64_native_start_count(markers)) return 0;
    Sc64NativeMeleeLaunch launch;
    for (unsigned row = 0; row < SC64_NATIVE_START_SLOTS; ++row) {
        launch.race_by_row[row] = SC64_NATIVE_START_NONE;
        if (!(rows & (1u << row))) continue;
        unsigned race = race_by_row[row];
        if (race == SC64_NATIVE_RACE_RANDOM) race = sc64_native_melee_draw(&seed, 3);
        else if (race > SC64_NATIVE_RACE_PROTOSS) return 0;
        launch.race_by_row[row] = (uint8_t)race;
    }
    launch.placement_seed = seed;
    if (!sc64_native_melee_starts(seed, rows, markers, &launch.starts)) return 0;
    *out = launch;
    return 1;
}
#endif
