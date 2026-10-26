/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CONSTRUCTION_SOURCE_H
#define SC64_CONSTRUCTION_SOURCE_H

#include "sc64_construction_animation.h"

typedef struct {
    signed char selector;
    uint32_t start_source_updates;
} SC64ConstructionGasOverlayRequest;

/* Values from the pinned OpenBW ImageTypes enum: 430-434 active, 435-439
 * depleted Vespene geyser smoke variants, selected by resource count. */
#define SC64_CONSTRUCTION_GAS_SMOKE_IMAGE_BASE 430u
#define SC64_CONSTRUCTION_GAS_SMOKE_DEPLETED_IMAGE_BASE 435u

typedef struct {
    unsigned short image_id;
    signed char x_offset, y_offset;
    uint32_t start_source_updates;
    unsigned char under;
    /* followmaingraphic: use the parent image's frame and horizontal flip. */
    unsigned char follows_main_graphic;
    /* Dynamic opcode-56 requests; active/depleted image bases are above. */
    unsigned char gas_overlay_request_count;
    const SC64ConstructionGasOverlayRequest *gas_overlay_requests;
    const SC64ConstructionAnimDesc *animation;
} SC64ConstructionSourceLayer;

typedef struct {
    unsigned short image_id;
    const SC64ConstructionAnimDesc *animation;
    const SC64ConstructionSourceLayer *layers;
    unsigned char layer_count;
} SC64ConstructionSourceStage;

/* Source strengths for destination-dependent IScript modifier 9 images. */
typedef struct {
    unsigned short x, y, w, h, canvas_w, canvas_h;
    const unsigned char *strengths;
} SC64ConstructionModifierFrame;

/* This visual-only adaptation has no persistent order/IScript state. Terran
 * stages follow progress at 20/40/60 percent instead of the native HP/order
 * transitions. Zerg follows the 25/50 percent morph thresholds. Protoss runs
 * OpenBW's WarpAnchor Init image program while its source build timer remains. */
static inline int
sc64_construction_source_phase(int race, int progress, int duration,
                               unsigned *stage, uint32_t *elapsed)
{
    unsigned result = 0;
    uint32_t start = 0, ticks;
    if (!stage || !elapsed || progress < 0 || duration <= 0 ||
        progress >= duration || (race != 1 && race != 2 && race != 3)) return 0;
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
    } else if (race == 2) {
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
