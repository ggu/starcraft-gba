#ifndef SC64_Z5_CONSUME_H
#define SC64_Z5_CONSUME_H

#include "../common/gba.h"

#if defined(SC64_Z5_CONSUME_ENABLED)
#if !defined(SC64_Z5_RUNTIME)
#error "SC64_Z5_CONSUME_ENABLED requires SC64_Z5_RUNTIME"
#endif

enum {
    SC64_Z5_CONSUME_HERO_RAW_TYPE = 51,
    SC64_Z5_CONSUME_TECH_ID = 16,
    SC64_Z5_CONSUME_RANGE_SOURCE_PIXELS = 16,
    SC64_Z5_CONSUME_ENERGY_COST = 0,
    SC64_Z5_CONSUME_ENERGY_GAIN = 50,
    SC64_Z5_CONSUME_MAX_ENERGY_Q8 = 250 * 256,
    SC64_Z5_CONSUME_ERROR = -1,
    SC64_Z5_CONSUME_DENIED = 0,
    SC64_Z5_CONSUME_APPLIED = 1
};

/* Adapter-resolved facts from the live engine. No raw source map tables are
 * embedded here; race/building/type flags must be resolved by the caller. */
typedef struct Sc64Z5ConsumeActor {
    int raw_type;
    int owner;
    int hp;
    int energy_q8;
    u8 alive;
    u8 completed;
    u8 hallucinated;
    u8 disabled;
    u8 building;
    u8 terran_race;
    u8 larva;
    u8 invincible;
    u8 stasis;
} Sc64Z5ConsumeActor;

static inline int sc64_z5_consume_actor_valid(const Sc64Z5ConsumeActor *a)
{
    return a && a->raw_type >= 0 && a->raw_type <= 65535 &&
           a->owner >= 0 && a->owner < 12 && a->hp >= 0 &&
           a->energy_q8 >= 0 && a->energy_q8 <= SC64_Z5_CONSUME_MAX_ENERGY_Q8 &&
           a->alive <= 1 && a->completed <= 1 && a->hallucinated <= 1 &&
           a->disabled <= 1 && a->building <= 1 && a->terran_race <= 1 &&
           a->larva <= 1 && a->invincible <= 1 && a->stasis <= 1;
}

/* The pinned native engine uses sprite-bounding-box distance and adds the
 * caster's movement range to the Consume weapon's 16 source-pixel range.
 * The adapter supplies that already-computed integer distance and movement
 * allowance so this model does not duplicate engine geometry. */
static inline int sc64_z5_consume_in_range(int bbox_distance,
                                            int caster_movement_range)
{
    return bbox_distance >= 0 && caster_movement_range >= 0 &&
           (bbox_distance <= SC64_Z5_CONSUME_RANGE_SOURCE_PIXELS ||
            bbox_distance - SC64_Z5_CONSUME_RANGE_SOURCE_PIXELS <= caster_movement_range);
}

/* Return ERROR for malformed input, DENIED for native eligibility/range
 * failure, and APPLIED after atomically killing the target. Consume costs no
 * energy; a non-hallucinated target grants 50, capped at the hero's 250. */
static inline int sc64_z5_consume_cast(Sc64Z5ConsumeActor *caster,
                                        Sc64Z5ConsumeActor *target,
                                        int tech_available,
                                        int bbox_distance,
                                        int caster_movement_range)
{
    Sc64Z5ConsumeActor next_target;
    int next_energy;
    if (!sc64_z5_consume_actor_valid(caster) ||
        !sc64_z5_consume_actor_valid(target) ||
        tech_available < 0 || tech_available > 1 || bbox_distance < 0 ||
        caster_movement_range < 0)
        return SC64_Z5_CONSUME_ERROR;
    if (!tech_available || caster->raw_type != SC64_Z5_CONSUME_HERO_RAW_TYPE ||
        !caster->alive || caster->hp <= 0 || !caster->completed || caster->hallucinated ||
        caster->disabled || !target->alive || target->hp <= 0 ||
        target->owner != caster->owner || target->building ||
        target->terran_race || target->larva || target->invincible ||
        target->stasis || !sc64_z5_consume_in_range(bbox_distance,
                                                     caster_movement_range))
        return SC64_Z5_CONSUME_DENIED;

    next_target = *target;
    next_target.alive = 0;
    next_energy = caster->energy_q8;
    if (!target->hallucinated) {
        next_energy += SC64_Z5_CONSUME_ENERGY_GAIN * 256;
        if (next_energy > SC64_Z5_CONSUME_MAX_ENERGY_Q8)
            next_energy = SC64_Z5_CONSUME_MAX_ENERGY_Q8;
    }
    *target = next_target;
    caster->energy_q8 = next_energy;
    return SC64_Z5_CONSUME_APPLIED;
}

#endif /* SC64_Z5_CONSUME_ENABLED */
#endif
