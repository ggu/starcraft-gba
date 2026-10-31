/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BATTLECRUISER_RULES_H
#define SC64_BATTLECRUISER_RULES_H

/* Capability-neutral prerequisite predicate. Callers must derive each input
 * from live, owned buildings and bind the tower to this exact Starport.
 * This predicate does not enable production or implement the unit service. */
typedef struct {
    int starport_alive, starport_complete, starport_grounded;
    int starport_owner, requested_owner;
    int attached_tower_alive, attached_tower_complete, attached_tower_owner;
    int physics_lab_complete_owned;
} Sc64BattlecruiserProduction;

static inline int sc64_battlecruiser_production_ready(
    const Sc64BattlecruiserProduction *p)
{
    return p && p->requested_owner >= 0 && p->starport_alive &&
        p->starport_complete && p->starport_grounded &&
        p->starport_owner == p->requested_owner &&
        p->attached_tower_alive && p->attached_tower_complete &&
        p->attached_tower_owner == p->requested_owner &&
        p->physics_lab_complete_owned;
}
/* Energy uses the source Q8 representation. OpenBW adds 8 Q8 per source
 * frame. The caller supplies the existing mode-correct saved source clock.
 * Its 64000 maximum fits the existing signed-17-bit carry codecs. */
enum { SC64_BC_ENERGY_SCALE = 256, SC64_BC_ENERGY_BASE_MAX = 200,
       SC64_BC_ENERGY_REACTOR_MAX = 250 };

static inline int sc64_battlecruiser_energy_cap(int reactor)
{
    return (reactor ? SC64_BC_ENERGY_REACTOR_MAX : SC64_BC_ENERGY_BASE_MAX)
        * SC64_BC_ENERGY_SCALE;
}

/* Newly created casters start at one quarter of their current maximum. */
static inline int sc64_battlecruiser_energy_initial(int reactor)
{ return sc64_battlecruiser_energy_cap(reactor) / 4; }

static inline int sc64_battlecruiser_energy_tick(int *energy, int reactor, int source_tick)
{
    int cap = sc64_battlecruiser_energy_cap(reactor);
    if (!energy || *energy < 0 || *energy > cap ||
        (source_tick != 0 && source_tick != 1)) return 0;
    if (source_tick && *energy < cap) {
        *energy += 8;
        if (*energy > cap) *energy = cap;
    }
    return 1;
}

/* Call only at the confirmed ability payment point. A failed command leaves
 * energy unchanged; target and research eligibility belong to the caller. */
static inline int sc64_battlecruiser_energy_pay(int *energy, int reactor,
                                                unsigned cost_points)
{
    int cap = sc64_battlecruiser_energy_cap(reactor);
    if (!energy || *energy < 0 || *energy > cap || cost_points > 250u ||
        *energy < (int)(cost_points * SC64_BC_ENERGY_SCALE)) return 0;
    *energy -= (int)(cost_points * SC64_BC_ENERGY_SCALE);
    return 1;
}
/* A serializable two-bit technology mask for each native source player.
 * The containing save generation owns storage and migration; this helper
 * does not assign any existing save bytes or reinterpret legacy state. */
enum { SC64_BC_TECH_OWNERS = 8, SC64_BC_TECH_YAMATO = 1,
       SC64_BC_TECH_REACTOR = 2 };

static inline int sc64_battlecruiser_tech_has(unsigned bits, unsigned owner,
                                              unsigned tech)
{
    if (bits > 65535u || owner >= SC64_BC_TECH_OWNERS ||
        (tech != SC64_BC_TECH_YAMATO && tech != SC64_BC_TECH_REACTOR))
        return 0;
    return !!(bits & (tech << (owner * 2)));
}

static inline int sc64_battlecruiser_tech_complete(unsigned *bits,
                                                    unsigned owner,
                                                    unsigned tech)
{
    if (!bits || *bits > 65535u || owner >= SC64_BC_TECH_OWNERS ||
        (tech != SC64_BC_TECH_YAMATO && tech != SC64_BC_TECH_REACTOR))
        return 0;
    *bits |= tech << (owner * 2);
    return 1;
}
/* Lab-local research state may be encoded by a future save adapter in
 * building fields, but these values do not reserve or mutate those fields. */
typedef struct { unsigned kind, remaining; } Sc64BattlecruiserResearch;

static inline unsigned sc64_battlecruiser_research_cost(unsigned kind)
{ return kind == SC64_BC_TECH_YAMATO ? 100u : kind == SC64_BC_TECH_REACTOR ? 150u : 0u; }
static inline unsigned sc64_battlecruiser_research_duration(unsigned kind)
{ return kind == SC64_BC_TECH_YAMATO ? 1800u : kind == SC64_BC_TECH_REACTOR ? 2500u : 0u; }
static inline int sc64_battlecruiser_research_valid(const Sc64BattlecruiserResearch *r)
{
    return r && ((!r->kind && !r->remaining) ||
        (sc64_battlecruiser_research_duration(r->kind) &&
         r->remaining <= sc64_battlecruiser_research_duration(r->kind)));
}
/* busy_same_owner_tech reflects all other labs; the adapter must scan them. */
static inline int sc64_battlecruiser_research_start(Sc64BattlecruiserResearch *r,
    unsigned kind, unsigned owner, unsigned techbits, int eligible_lab,
    int busy_same_owner_tech, int *minerals, int *gas)
{
    unsigned cost = sc64_battlecruiser_research_cost(kind);
    if (!sc64_battlecruiser_research_valid(r) || r->kind || !cost ||
        owner >= SC64_BC_TECH_OWNERS || techbits > 65535u || !eligible_lab ||
        busy_same_owner_tech || sc64_battlecruiser_tech_has(techbits,owner,kind) ||
        !minerals || !gas || *minerals < (int)cost || *gas < (int)cost) return 0;
    *minerals -= (int)cost; *gas -= (int)cost;
    r->kind = kind; r->remaining = sc64_battlecruiser_research_duration(kind);
    return 1;
}
static inline int sc64_battlecruiser_research_tick(Sc64BattlecruiserResearch *r,
    unsigned owner, unsigned *techbits, int source_tick)
{
    if (!sc64_battlecruiser_research_valid(r) || !r->kind || !techbits ||
        *techbits > 65535u || owner >= SC64_BC_TECH_OWNERS ||
        (source_tick != 0 && source_tick != 1)) return 0;
    if (source_tick) {
        if (r->remaining) --r->remaining;
        else {
            unsigned kind = r->kind; r->kind = 0;
            (void)sc64_battlecruiser_tech_complete(techbits,owner,kind);
        }
    }
    return 1;
}
/* Caller supplies validated bank space: fail atomically rather than wrap. */
static inline int sc64_battlecruiser_research_cancel(Sc64BattlecruiserResearch *r,
    int destroyed, int *minerals, int *gas)
{
    unsigned refund;
    if (!sc64_battlecruiser_research_valid(r) || !r->kind ||
        (destroyed != 0 && destroyed != 1) || !minerals || !gas ||
        *minerals < 0 || *gas < 0) return 0;
    refund = sc64_battlecruiser_research_cost(r->kind);
    if (destroyed) refund = refund * 3u / 4u;
    if (*minerals > 65535 - (int)refund || *gas > 65535 - (int)refund) return 0;
    *minerals += (int)refund; *gas += (int)refund;
    r->kind = r->remaining = 0;
    return 1;
}
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
/* Two-record GBA adaptation. Authoritative shots are not visual tracers and
 * may never be evicted. The caller reserves before accepting an order. */
enum { SC64_BC_SHOT_LIMIT=2, SC64_BC_SHOT_FREE=0, SC64_BC_SHOT_APPROACH=1,
       SC64_BC_SHOT_CHARGING=2, SC64_BC_SHOT_FLIGHT=3,
       SC64_BC_SHOT_NO_CASTER=65535, SC64_BC_SHOT_NO_TARGET=65535,
       SC64_BC_YAMATO_RELEASE_SOURCE_TICK=44 };
typedef struct {
    int x_q8,y_q8;
    unsigned short caster,target,age;
    unsigned char state,phase;
    /* Reserved for source flingy motion; no lossy reconstruction on load. */
    unsigned motion;
} Sc64BcYamatoShot;
_Static_assert(sizeof(Sc64BcYamatoShot)==20,"BC shot row budget");
static inline int sc64_bc_shot_reserve(Sc64BcYamatoShot *rows,
                                       unsigned caster,unsigned target)
{
    if (!rows || caster>=65535u || target>=65535u) return -1;
    for (int i=0;i<SC64_BC_SHOT_LIMIT;++i)
        if (rows[i].state && rows[i].state!=SC64_BC_SHOT_FLIGHT && rows[i].caster==caster) return -1;
    for (int i=0;i<SC64_BC_SHOT_LIMIT;++i) if (!rows[i].state) {
        rows[i]=(Sc64BcYamatoShot){.caster=(unsigned short)caster,
                                  .target=(unsigned short)target,
                                  .state=SC64_BC_SHOT_APPROACH};
        return i;
    }
    return -1;
}
static inline void sc64_bc_shot_clear(Sc64BcYamatoShot *q)
{ if(q) *q=(Sc64BcYamatoShot){0}; }
/* Canceling or killing the caster cannot erase a projectile already fired. */
static inline void sc64_bc_shot_caster_gone(Sc64BcYamatoShot *rows,unsigned caster)
{
    if (!rows) return;
    for (int i=0;i<SC64_BC_SHOT_LIMIT;++i) if (rows[i].state && rows[i].caster==caster) {
        if (rows[i].state==SC64_BC_SHOT_FLIGHT) rows[i].caster=SC64_BC_SHOT_NO_CASTER;
        else sc64_bc_shot_clear(&rows[i]);
    }
}
/* A target removal clears the live reference. Launched missiles continue
 * toward their fixed endpoint but must never damage a recycled slot. */
static inline void sc64_bc_shot_target_gone(Sc64BcYamatoShot *rows,unsigned target)
{
    if (!rows) return;
    for (int i=0;i<SC64_BC_SHOT_LIMIT;++i)
        if (rows[i].state && rows[i].target==target) {
            if (rows[i].state==SC64_BC_SHOT_FLIGHT || rows[i].state==SC64_BC_SHOT_CHARGING)
                rows[i].target=SC64_BC_SHOT_NO_TARGET;
            else sc64_bc_shot_clear(&rows[i]);
        }
}
#endif
#endif
