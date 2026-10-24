#ifndef SC64_Z5_CLOAK_H
#define SC64_Z5_CLOAK_H

#include "../common/gba.h"

#if defined(SC64_Z5_CLOAK_ENABLED)
#if !defined(SC64_Z5_RUNTIME)
#error "SC64_Z5_CLOAK_ENABLED requires SC64_Z5_RUNTIME"
#endif

enum {
    SC64_Z5_CLOAK_RAW_TYPE = 51,
    SC64_Z5_CLOAK_TECH_ID = 10,
    SC64_Z5_CLOAK_MAX_ENERGY_Q8 = 250 * 256,
    SC64_Z5_CLOAK_ACTIVATION_COST_Q8 = 25 * 256,
    SC64_Z5_CLOAK_SOURCE_DRAIN_RAW = 10,
    SC64_Z5_CLOAK_TICK_DRAIN_RAW = 8,
    SC64_Z5_CLOAK_SOURCE_REGEN_RAW = 8,
    SC64_Z5_CLOAK_REGEN_NUMERATOR_RAW = 32,
    SC64_Z5_CLOAK_REGEN_DENOMINATOR = 5,
    SC64_Z5_CLOAK_REGEN_PHASES = 5,
    SC64_Z5_CLOAK_ERROR = -1,
    SC64_Z5_CLOAK_UNCHANGED = 0,
    SC64_Z5_CLOAK_ADVANCED = 1,
    SC64_Z5_CLOAK_DENIED_ENERGY = 2
};

typedef struct Sc64Z5CloakState {
    u8 active;
    u8 regen_remainder;
} Sc64Z5CloakState;

_Static_assert(sizeof(Sc64Z5CloakState) == 2, "Z5 cloak state size");

static inline int sc64_z5_cloak_state_valid(const Sc64Z5CloakState *s)
{
    return s && s->active <= 1 && s->regen_remainder < SC64_Z5_CLOAK_REGEN_PHASES &&
           (!s->active || s->regen_remainder == 0);
}

/* Pinned OpenBW fp8 literals are raw Q8 units: source energy updates run at
 * 24Hz, draining 10 raw units while cloaked or regenerating 8 raw units while
 * uncloaked. At this runtime's 30Hz cadence the exact authored conversion is
 * 8*24/30 = 32/5 raw Q8 per uncloaked update and 10*24/30 = 8 raw Q8 drained
 * per cloaked update. `carry` stores energy in Q8 (256 raw units per point),
 * so the 25-point activation cost and 250-point cap are multiplied by 256.
 * Remainder persists the fractional regen exactly; subframe damage/energy
 * ordering is not claimed to match the native engine. */
static inline int sc64_z5_cloak_toggle(Sc64Z5CloakState *s, int *energy_q8)
{
    Sc64Z5CloakState next;
    int next_energy;
    if (!sc64_z5_cloak_state_valid(s) || !energy_q8 || *energy_q8 < 0 ||
        *energy_q8 > SC64_Z5_CLOAK_MAX_ENERGY_Q8)
        return SC64_Z5_CLOAK_ERROR;
    next = *s;
    next_energy = *energy_q8;
    if (next.active) {
        next.active = 0;
        next.regen_remainder = 0;
    } else {
        if (next_energy < SC64_Z5_CLOAK_ACTIVATION_COST_Q8)
            return SC64_Z5_CLOAK_DENIED_ENERGY;
        next_energy -= SC64_Z5_CLOAK_ACTIVATION_COST_Q8;
        next.active = 1;
        next.regen_remainder = 0;
    }
    *s = next;
    *energy_q8 = next_energy;
    return SC64_Z5_CLOAK_ADVANCED;
}

/* paused freezes both cloak drain and uncloak regeneration. */
static inline int sc64_z5_cloak_tick(Sc64Z5CloakState *s, int *energy_q8,
                                      int paused)
{
    Sc64Z5CloakState next;
    int next_energy;
    unsigned total, gain;
    if (!sc64_z5_cloak_state_valid(s) || !energy_q8 || *energy_q8 < 0 ||
        *energy_q8 > SC64_Z5_CLOAK_MAX_ENERGY_Q8 || paused < 0 || paused > 1)
        return SC64_Z5_CLOAK_ERROR;
    if (paused) return SC64_Z5_CLOAK_UNCHANGED;
    next = *s;
    next_energy = *energy_q8;
    if (next.active) {
        /* Native cloak turns off before draining if energy is below 10. */
        if (next_energy < SC64_Z5_CLOAK_SOURCE_DRAIN_RAW) {
            next.active = 0;
            next.regen_remainder = 0;
        } else {
            next_energy -= SC64_Z5_CLOAK_TICK_DRAIN_RAW;
        }
    } else if (next_energy < SC64_Z5_CLOAK_MAX_ENERGY_Q8) {
        total = next.regen_remainder + SC64_Z5_CLOAK_REGEN_NUMERATOR_RAW;
        gain = total / SC64_Z5_CLOAK_REGEN_DENOMINATOR;
        next.regen_remainder = (u8)(total % SC64_Z5_CLOAK_REGEN_DENOMINATOR);
        next_energy += (int)gain;
        if (next_energy >= SC64_Z5_CLOAK_MAX_ENERGY_Q8) {
            next_energy = SC64_Z5_CLOAK_MAX_ENERGY_Q8;
            next.regen_remainder = 0;
        }
    } else if (next.regen_remainder) {
        next.regen_remainder = 0;
    } else {
        return SC64_Z5_CLOAK_UNCHANGED;
    }
    *s = next;
    *energy_q8 = next_energy;
    return SC64_Z5_CLOAK_ADVANCED;
}
#endif

#endif
