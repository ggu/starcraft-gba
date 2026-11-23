/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MINERAL_FIFO_RANK_H
#define SC64_MINERAL_FIFO_RANK_H

#include "sc64_mineral_fifo.h"

/* Begin with zeroed candidate storage. Until finish succeeds, next holds
 * ranks and tail holds patch counts. Keep these bytes out of the live queue. */
static inline int
sc64_mineral_fifo_stage_rank(Sc64MineralFifo *candidate, unsigned slot,
                            unsigned patch, unsigned rank, unsigned remaining,
                            unsigned max_ticks)
{
    if (!candidate || slot >= MAX_UNITS || patch >= MAX_MINERALS ||
        !rank || rank > MAX_UNITS || remaining > max_ticks ||
        (rank != 1 && remaining) || candidate->patch[slot] ||
        candidate->tail[patch] >= MAX_UNITS) return 0;
    candidate->patch[slot] = (uint8_t)(patch + 1u);
    candidate->next[slot] = (uint8_t)rank;
    ++candidate->tail[patch];
    return 1;
}

static inline int
sc64_mineral_fifo_finish_ranks(Sc64MineralFifo *candidate)
{
    if (!candidate) return 0;
    uint8_t ordered[MAX_UNITS] = {0};
    unsigned total = 0;
    for (unsigned patch = 0; patch < MAX_MINERALS; ++patch) {
        unsigned count = candidate->tail[patch];
        if (count > MAX_UNITS - total) return 0;
        candidate->head[patch] = (uint8_t)total;
        total += count;
    }

    /* Scatter ranks into a contiguous span per patch. With one entry per
     * rank and the count matched, every position in each span is occupied. */
    unsigned assigned = 0;
    for (unsigned slot = 0; slot < MAX_UNITS; ++slot) {
        unsigned patch = (unsigned)candidate->patch[slot] - 1u;
        if (!candidate->patch[slot]) continue;
        unsigned rank = candidate->next[slot];
        if (patch >= MAX_MINERALS || !rank || rank > candidate->tail[patch])
            return 0;
        unsigned at = candidate->head[patch] + rank - 1u;
        if (ordered[at]) return 0;
        ordered[at] = (uint8_t)(slot + 1u);
        ++assigned;
    }
    if (assigned != total) return 0;

    for (unsigned patch = 0; patch < MAX_MINERALS; ++patch) {
        unsigned start = candidate->head[patch], count = candidate->tail[patch];
        candidate->head[patch] = count ? ordered[start] : 0;
        if (!count) continue;
        unsigned end = start + count - 1u;
        candidate->tail[patch] = ordered[end];
        for (unsigned at = start; at < end; ++at)
            candidate->next[ordered[at] - 1u] = ordered[at + 1u];
        candidate->next[ordered[end] - 1u] = 0;
    }
    return 1;
}

#endif
