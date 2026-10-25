/*
 * Bounded construction-animation timeline sampler.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Original project code. Durations are caller-supplied source-update ticks;
 * this helper does not infer source stages or claim native IScript parity.
 */
#ifndef SC64_CONSTRUCTION_ANIMATION_H
#define SC64_CONSTRUCTION_ANIMATION_H

#include <stdint.h>

#define SC64_CONSTRUCTION_ANIM_MAX_STEPS 64u
#define SC64_CONSTRUCTION_HOLD_LAST 0xffu

typedef struct {
    uint8_t frame;
    uint32_t duration_source_updates;
} SC64ConstructionAnimStep;

typedef struct {
    const SC64ConstructionAnimStep *steps;
    uint8_t step_count;
    uint8_t loop_start; /* SC64_CONSTRUCTION_HOLD_LAST holds final frame. */
    uint8_t frame_count;
} SC64ConstructionAnimDesc;

/* Return 1 with a valid source frame; return 0 for fallback on any bad input. */
static inline int
sc64_construction_animation_frame(const SC64ConstructionAnimDesc *desc,
                                  uint32_t elapsed_source_updates,
                                  unsigned *frame)
{
    uint8_t i;
    uint32_t total = 0;

    if (frame == 0) return 0;
    if (desc == 0 || desc->steps == 0 || desc->step_count == 0 ||
        desc->step_count > SC64_CONSTRUCTION_ANIM_MAX_STEPS ||
        desc->frame_count == 0 ||
        (desc->loop_start != SC64_CONSTRUCTION_HOLD_LAST &&
         desc->loop_start >= desc->step_count)) return 0;

    for (i = 0; i < desc->step_count; ++i) {
        uint32_t duration = desc->steps[i].duration_source_updates;
        if (duration == 0 || desc->steps[i].frame >= desc->frame_count ||
            UINT32_MAX - total < duration) return 0;
        total += duration;
    }

    if (elapsed_source_updates >= total) {
        if (desc->loop_start == SC64_CONSTRUCTION_HOLD_LAST) {
            *frame = desc->steps[desc->step_count - 1].frame;
            return 1;
        }
        {
            uint32_t intro_duration = 0;
            uint32_t loop_duration = 0;
            for (i = 0; i < desc->loop_start; ++i)
                intro_duration += desc->steps[i].duration_source_updates;
            for (i = desc->loop_start; i < desc->step_count; ++i)
                loop_duration += desc->steps[i].duration_source_updates;
            if (loop_duration == 0) return 0;
            elapsed_source_updates = intro_duration +
                ((elapsed_source_updates - intro_duration) % loop_duration);
        }
    }

    {
        uint32_t cursor = 0;
        for (i = 0; i < desc->step_count; ++i) {
            cursor += desc->steps[i].duration_source_updates;
            if (elapsed_source_updates < cursor) {
                *frame = desc->steps[i].frame;
                return 1;
            }
        }
    }
    return 0;
}

#endif
