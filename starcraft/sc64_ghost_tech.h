/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GHOST_TECH_H
#define SC64_GHOST_TECH_H
#include <limits.h>
#include <stdint.h>
#include "sc64_battlecruiser_rules.h"
#include "sc64_nuke_rules.h"

/* Numeric source facts: verified USA N64 techdata.dat/upgrades.dat and
 * authenticated Covert Ops button records. See the Ghost tech audit. */
enum {
    SC64_GHOST_TECH_NONE, SC64_GHOST_TECH_CLOAK, SC64_GHOST_TECH_LOCKDOWN,
    SC64_GHOST_TECH_OCULAR, SC64_GHOST_TECH_REACTOR,
    SC64_GHOST_TECH_OWNER_COUNT = 8, SC64_GHOST_TECH_ALL = 15,
    SC64_GHOST_CLOAK_DRAIN_Q8 = 10, SC64_GHOST_NO_ENERGY_OVERRIDE = -1
};

typedef struct {
    uint16_t minerals, gas, source_ticks, icon;
    uint8_t raw, upgrade, energy;
} Sc64GhostTechFact;

static inline const Sc64GhostTechFact *sc64_ghost_tech_fact(unsigned kind)
{
    static const Sc64GhostTechFact facts[] = {
        {100, 100, 1200, 252, 10, 0, 25},
        {200, 200, 1500, 240,  1, 0, 100},
        {100, 100, 2500, 249, 20, 1, 0},
        {150, 150, 2500, 256, 21, 1, 0}
    };
    return kind >= SC64_GHOST_TECH_CLOAK && kind <= SC64_GHOST_TECH_REACTOR ?
        &facts[kind - SC64_GHOST_TECH_CLOAK] : 0;
}

/* Owner keys are supplied by the engine: native source slots in campaign
 * and compact controller owners in custom games. No save allocation here. */
typedef struct { uint32_t completed, pending; } Sc64GhostTechState;
typedef struct { uint16_t remaining; uint8_t kind, payer; } Sc64GhostTechJob;

enum {
    SC64_GHOST_JOB_ERROR = -1, SC64_GHOST_JOB_WAITING,
    SC64_GHOST_JOB_PROGRESS, SC64_GHOST_JOB_COMPLETE
};
enum {
    SC64_GHOST_ENERGY_ERROR = -1, SC64_GHOST_ENERGY_IDLE,
    SC64_GHOST_ENERGY_UPDATED, SC64_GHOST_ENERGY_EXHAUSTED
};

static inline uint32_t sc64_ghost_tech_bit(unsigned owner, unsigned kind)
{
    if (owner >= SC64_GHOST_TECH_OWNER_COUNT || !sc64_ghost_tech_fact(kind)) return 0;
    return UINT32_C(1) << (owner * 4u + kind - SC64_GHOST_TECH_CLOAK);
}

static inline int sc64_ghost_tech_state_valid(const Sc64GhostTechState *state)
{ return state && !(state->completed & state->pending); }

static inline int sc64_ghost_tech_has(const Sc64GhostTechState *state, unsigned owner, unsigned kind)
{ return sc64_ghost_tech_state_valid(state) && !!(state->completed & sc64_ghost_tech_bit(owner, kind)); }

/* Import only the source-authorized initial completion mask. Calls are
 * idempotent; a grant must not silently consume a live paid receipt. */
static inline int
sc64_ghost_tech_seed(Sc64GhostTechState *state, unsigned owner, unsigned initial)
{
    if (!sc64_ghost_tech_state_valid(state) || owner >= SC64_GHOST_TECH_OWNER_COUNT ||
        (initial & ~(unsigned)SC64_GHOST_TECH_ALL)) return 0;
    uint32_t mask = (uint32_t)initial << (owner * 4u);
    if (state->pending & mask) return 0;
    state->completed |= mask;
    return 1;
}

static inline int
sc64_ghost_tech_job_valid(const Sc64GhostTechState *state, const Sc64GhostTechJob *job)
{
    if (!sc64_ghost_tech_state_valid(state) || !job) return 0;
    if (!job->kind) return !job->payer && !job->remaining;
    const Sc64GhostTechFact *fact = sc64_ghost_tech_fact(job->kind);
    return fact && job->remaining && job->remaining <= fact->source_ticks &&
        !!(state->pending & sc64_ghost_tech_bit(job->payer, job->kind));
}

/* A live completed attached Covert Ops grants one job per producer and one
 * concurrent copy of each technology per payer. Permissions is a 4-bit
 * source policy mask for this payer, separate from the completed bits. */
static inline int
sc64_ghost_tech_begin(Sc64GhostTechState *state, Sc64GhostTechJob *job,
    unsigned owner, unsigned kind, unsigned permissions, int online, int *minerals, int *gas)
{
    const Sc64GhostTechFact *fact = sc64_ghost_tech_fact(kind);
    uint32_t bit = sc64_ghost_tech_bit(owner, kind);
    if (!bit || !sc64_ghost_tech_job_valid(state, job) || job->kind || online != 1 ||
        (permissions & ~(unsigned)SC64_GHOST_TECH_ALL) ||
        !(permissions & (1u << (kind - SC64_GHOST_TECH_CLOAK))) ||
        ((state->completed | state->pending) & bit) ||
        !minerals || !gas || minerals == gas ||
        *minerals < fact->minerals || *gas < fact->gas) return 0;
    *minerals -= fact->minerals;
    *gas -= fact->gas;
    *job = (Sc64GhostTechJob){fact->source_ticks, (uint8_t)kind, (uint8_t)owner};
    state->pending |= bit;
    return 1;
}

/* A detached producer, or one currently controlled by someone other than
 * the payer, cannot advance this receipt. Reattachment to its original
 * payer resumes it; any transfer/cancel policy belongs to the world. */
static inline int
sc64_ghost_tech_tick(Sc64GhostTechState *state, Sc64GhostTechJob *job,
                     unsigned current_owner, int online, int source_tick)
{
    if (!sc64_ghost_tech_job_valid(state, job) || (online != 0 && online != 1) ||
        (source_tick != 0 && source_tick != 1)) return SC64_GHOST_JOB_ERROR;
    if (!job->kind || !online || !source_tick || current_owner != job->payer)
        return SC64_GHOST_JOB_WAITING;
    if (--job->remaining) return SC64_GHOST_JOB_PROGRESS;
    uint32_t bit = sc64_ghost_tech_bit(job->payer, job->kind);
    state->pending &= ~bit;
    state->completed |= bit;
    *job = (Sc64GhostTechJob){0};
    return SC64_GHOST_JOB_COMPLETE;
}

/* Deliberate cancel refunds 100%; destruction refunds 75% per resource
 * in the pinned PC reference. Authenticate the payer independently from
 * current control before calling this; a captor cannot claim the refund. */
static inline int
sc64_ghost_tech_cancel(Sc64GhostTechState *state, Sc64GhostTechJob *job,
                      unsigned payer, int destroyed, int *minerals, int *gas)
{
    if (!sc64_ghost_tech_job_valid(state, job) || !job->kind || payer != job->payer ||
        (destroyed != 0 && destroyed != 1) || !minerals || !gas || minerals == gas)
        return 0;
    const Sc64GhostTechFact *fact = sc64_ghost_tech_fact(job->kind);
    int ore = destroyed ? fact->minerals * 3 / 4 : fact->minerals;
    int vespene = destroyed ? fact->gas * 3 / 4 : fact->gas;
    if (*minerals < 0 || *minerals > INT_MAX - ore || *gas < 0 || *gas > INT_MAX - vespene)
        return 0;
    *minerals += ore;
    *gas += vespene;
    state->pending &= ~sc64_ghost_tech_bit(payer, job->kind);
    *job = (Sc64GhostTechJob){0};
    return 1;
}

static inline int
sc64_ghost_sight_source_pixels(const Sc64GhostTechState *state, unsigned owner)
{
    return sc64_ghost_tech_has(state, owner, SC64_GHOST_TECH_OCULAR) ?
        SC64_NUKE_OCULAR_SOURCE_SIGHT : SC64_NUKE_SOURCE_SIGHT;
}

static inline int
sc64_ghost_energy_cap(const Sc64GhostTechState *state, unsigned owner)
{
    return sc64_battlecruiser_energy_cap(
        sc64_ghost_tech_has(state, owner, SC64_GHOST_TECH_REACTOR));
}

/* CHK UNIT can explicitly set the placed actor's energy percentage. For
 * an ordinary birth or no valid map override use one quarter of max. */
static inline int
sc64_ghost_energy_initial(const Sc64GhostTechState *state, unsigned owner, int percent)
{
    if (!sc64_ghost_tech_state_valid(state) || owner >= SC64_GHOST_TECH_OWNER_COUNT) return -1;
    int max = sc64_ghost_energy_cap(state, owner);
    if (percent == SC64_GHOST_NO_ENERGY_OVERRIDE) return max / 4;
    if (percent < 0 || percent > 100) return -1;
    return max * percent / 100;
}

/* This is only arithmetic; cloak order/detector visibility and spell
 * release are separate world services. Caller supplies the source clock. */
static inline int
sc64_ghost_energy_tick(const Sc64GhostTechState *state, unsigned owner, int *energy,
                       int active_cloak, int passive_cloak, int source_tick)
{
    if (!sc64_ghost_tech_state_valid(state) || owner >= SC64_GHOST_TECH_OWNER_COUNT)
        return SC64_GHOST_ENERGY_ERROR;
    int reactor = sc64_ghost_tech_has(state, owner, SC64_GHOST_TECH_REACTOR);
    int cap = sc64_battlecruiser_energy_cap(reactor);
    if (!energy || *energy < 0 || *energy > cap ||
        (unsigned)active_cloak > 1u || (unsigned)passive_cloak > 1u ||
        (unsigned)source_tick > 1u) return SC64_GHOST_ENERGY_ERROR;
    if (!source_tick) return SC64_GHOST_ENERGY_IDLE;
    if (active_cloak && !passive_cloak) {
        if (*energy < SC64_GHOST_CLOAK_DRAIN_Q8) return SC64_GHOST_ENERGY_EXHAUSTED;
        *energy -= SC64_GHOST_CLOAK_DRAIN_Q8;
    } else if (*energy < cap) {
        (void)sc64_battlecruiser_energy_tick(energy, reactor, source_tick);
    } else return SC64_GHOST_ENERGY_IDLE;
    return SC64_GHOST_ENERGY_UPDATED;
}

static inline int
sc64_ghost_energy_pay(const Sc64GhostTechState *state, unsigned owner, unsigned kind, int *energy)
{
    const Sc64GhostTechFact *fact = sc64_ghost_tech_fact(kind);
    return fact && !fact->upgrade && sc64_ghost_tech_has(state, owner, kind) &&
        sc64_battlecruiser_energy_pay(energy,
            sc64_ghost_tech_has(state, owner, SC64_GHOST_TECH_REACTOR), fact->energy);
}
#endif
