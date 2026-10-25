/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_DEATH_SOURCE_H
#define SC64_DEATH_SOURCE_H

#include <stdint.h>
#include "sc64_construction_animation.h"

/* A compiled source death animation is a short list of independent image
 * layers: the dying unit's own collapse frames, explosion overlays (and
 * overlays they spawn), and remnant/rubble sprites. Each layer runs its own
 * frame stream from start_su and is removed at end_su, mirroring IScript
 * 'end'. The generator caps long remnant streams; nothing here is
 * persistent or read by the simulation. */
typedef struct {
    uint16_t image_id;
    int8_t x, y;
    uint16_t start_su, end_su;
    uint8_t under;   /* drawn beneath units: remnants and collapse frames */
    uint8_t remnant; /* remnant/rubble sprite */
    const SC64ConstructionAnimDesc *animation;
} Sc64DeathSourceLayer;

typedef struct {
    const Sc64DeathSourceLayer *layers;
    uint8_t layer_count;
    uint16_t burst_su, remnant_start, total_su;
} Sc64DeathSource;

#define SC64_DEATH_SOURCE_MAX_LAYERS 8u

/* Return 1 with the layer's frame at the entry age, 0 when the layer has not
 * started, has ended, or its descriptor is malformed. */
static inline int
sc64_death_source_layer_frame(const Sc64DeathSourceLayer *layer,
                              uint32_t age_su, unsigned *frame)
{
    if (!layer || !frame || age_su < layer->start_su ||
        age_su >= layer->end_su) return 0;
    return sc64_construction_animation_frame(layer->animation,
                                             age_su - layer->start_su, frame);
}

static inline int
sc64_death_source_valid(const Sc64DeathSource *source)
{
    if (!source || !source->layers || !source->layer_count ||
        source->layer_count > SC64_DEATH_SOURCE_MAX_LAYERS || !source->total_su)
        return 0;
    for (unsigned i = 0; i < source->layer_count; ++i) {
        const Sc64DeathSourceLayer *l = &source->layers[i];
        if (!l->animation || l->start_su >= l->end_su ||
            l->end_su > source->total_su) return 0;
    }
    return 1;
}

#endif
