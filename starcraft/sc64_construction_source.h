/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CONSTRUCTION_SOURCE_H
#define SC64_CONSTRUCTION_SOURCE_H

#include "sc64_construction_animation.h"

typedef struct {
    unsigned short image_id;
    const SC64ConstructionAnimDesc *animation;
} SC64ConstructionSourceStage;

/* This visual-only adaptation has no persistent order/IScript state. Terran
 * stages follow progress at 20/40/60 percent instead of the native HP/order
 * transitions. Zerg follows the 25/50 percent morph thresholds. Completion
 * signals, shadows and worker animation remain outside this profile. */
static inline int
sc64_construction_source_phase(int race, int progress, int duration,
                               unsigned *stage, uint32_t *elapsed)
{
    unsigned result = 0;
    uint32_t start = 0, ticks;
    if (!stage || !elapsed || progress < 0 || duration <= 0 ||
        progress >= duration || (race != 1 && race != 2)) return 0;
    if (race == 1) {
        for (unsigned i = 1; i <= 3; ++i) {
            /* floor(duration*i/5)+1 implements a strict threshold without
             * overflowing even for an INT_MAX duration. */
            uint32_t at = (uint32_t)(duration / 5) * i +
                          (uint32_t)(duration % 5) * i / 5u + 1u;
            if ((uint32_t)progress < at) break;
            result = i;
            start = at;
        }
    } else {
        uint32_t quarter = (uint32_t)duration / 4u + 1u;
        uint32_t half = (uint32_t)duration / 2u + 1u;
        if ((uint32_t)progress >= half) { result = 2; start = half; }
        else if ((uint32_t)progress >= quarter) { result = 1; start = quarter; }
    }
    ticks = (uint32_t)progress - start;
    /* The GBA simulation updates at 30 Hz; source visual updates use 24 Hz.
     * This is a local stage clock, independent of rendering and wall time. */
    *elapsed = (ticks / 5u) * 4u + (ticks % 5u) * 4u / 5u;
    *stage = result;
    return 1;
}

#endif
