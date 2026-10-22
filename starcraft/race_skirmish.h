#ifndef SC64_RACE_SKIRMISH_H
#define SC64_RACE_SKIRMISH_H

#include "race_rules.h"

/* Fixed-width faction state shared by human and AI owners. The caller stores
 * one record per owner and feeds completed hatcheries/pylons and unit queues
 * into these pure transitions. All rates and timers are simulation frames. */
typedef struct {
    unsigned char race;
    unsigned char larva;
    unsigned char hatcheries;
    unsigned char powered;
    unsigned short supply_used;
    unsigned short supply_cap;
    unsigned short larva_clock;
} Sc64RaceState;

typedef struct {
    unsigned short minerals;
    unsigned short gas;
} Sc64RaceBank;

/* Shared by human and AI command paths: resources are debited only when the
 * complete order can be accepted. */
static inline int sc64_race_pay(Sc64RaceBank *bank, unsigned short minerals,
                                 unsigned short gas)
{
    if (bank->minerals < minerals || bank->gas < gas) return 0;
    bank->minerals -= minerals;
    bank->gas -= gas;
    return 1;
}

static inline int sc64_race_pay_unit(Sc64RaceBank *bank, int unit_type)
{
    const Sc64RaceUnitRule *r = sc64_race_unit_rule(unit_type);
    return r && sc64_race_pay(bank, r->minerals, r->gas);
}

static inline int sc64_race_pay_building(Sc64RaceBank *bank, int building_type)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(building_type);
    return r && sc64_race_pay(bank, r->minerals, r->gas);
}

static inline void sc64_race_state_init(Sc64RaceState *s, unsigned char race,
                                         unsigned short used, unsigned short cap,
                                         unsigned char hatcheries,
                                         unsigned char larvae)
{
    s->race = race;
    s->supply_used = used;
    s->supply_cap = cap;
    s->hatcheries = hatcheries;
    s->larva = larvae > 3 ? 3 : larvae;
    s->powered = 0;
    s->larva_clock = 0;
}

/* A completed Hatchery produces one larva about every 14.25 seconds on
 * fastest setting (342 frames); the three-larva local cap mirrors Brood War.
 * Call once per simulation frame, passing whether this owner has room. */
static inline void sc64_race_larva_tick(Sc64RaceState *s)
{
    if (s->race != R_ZERG || !s->hatcheries || s->larva >= 3) return;
    if (++s->larva_clock >= 342) {
        s->larva_clock = 0;
        ++s->larva;
    }
}

static inline int sc64_race_can_produce(const Sc64RaceState *s,
                                         int unit_type, int count)
{
    const Sc64RaceUnitRule *r = sc64_race_unit_rule(unit_type);
    unsigned int needed;
    if (!r || r->race != s->race || count < 1) return 0;
    if ((r->flags & SC_RACE_F_LARVA) && s->larva < 1) return 0;
    if (r->flags & SC_RACE_F_REQUIRES_POWER) return 0;
    /* A Zergling egg yields two units from one larva and reserves one supply
     * per unit (one doubled supply point each). */
    needed = unit_type == U_ZERGLING ? (unsigned int)count * 2u : r->supply * (unsigned int)count;
    if (s->supply_used + needed > s->supply_cap) return 0;
    return 1;
}

static inline int sc64_race_morph(Sc64RaceState *s, int unit_type, int count)
{
    const Sc64RaceUnitRule *r = sc64_race_unit_rule(unit_type);
    unsigned int needed;
    if (!sc64_race_can_produce(s, unit_type, count)) return 0;
    needed = unit_type == U_ZERGLING ? (unsigned int)count * 2u : r->supply * (unsigned int)count;
    if (r->flags & SC_RACE_F_LARVA) --s->larva;
    s->supply_used = (unsigned short)(s->supply_used + needed);
    return 1;
}

static inline void sc64_race_unit_died(Sc64RaceState *s, int unit_type, int count)
{
    const Sc64RaceUnitRule *r = sc64_race_unit_rule(unit_type);
    unsigned int lost;
    if (!r || count < 1) return;
    lost = unit_type == U_ZERGLING ? (unsigned int)count * 2u : r->supply * (unsigned int)count;
    s->supply_used = lost > s->supply_used ? 0 : (unsigned short)(s->supply_used - lost);
}

static inline void sc64_race_set_pylon_power(Sc64RaceState *s, int any_active_pylon)
{ if (s->race == R_PROTOSS) s->powered = any_active_pylon != 0; }

/* Protoss structure placement reserves no worker permanently; completion
 * returns the Probe to the caller's idle/mining pool. Zerg construction
 * consumes the Drone and therefore removes its two doubled supply points. */
static inline int sc64_race_build_consumes_worker(int building_type)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(building_type);
    return r && (r->flags & SC_RACE_F_CONSUMES_WORKER) != 0;
}

static inline int sc64_race_build_complete(Sc64RaceState *s, int building_type)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(building_type);
    if (!r || r->race != s->race) return 0;
    if (building_type == B_HATCHERY) {
        if (s->hatcheries < 255) ++s->hatcheries;
        if (s->larva < 3) ++s->larva;
    }
    if (r->supply_provided) {
        unsigned int cap = s->supply_cap + r->supply_provided;
        s->supply_cap = cap > 65535u ? 65535u : (unsigned short)cap;
    }
    return 1;
}

/* Integer shield recharge: caller invokes once per simulation frame. The
 * canonical Brood War shield rate is 7/256 HP per frame; remainder preserves
 * fractional accumulation without floating point. */
static inline int sc64_race_shield_tick(unsigned short *shields,
                                         unsigned short max_shields,
                                         unsigned short *remainder)
{
    unsigned int n;
    if (*shields >= max_shields) { *remainder = 0; return 0; }
    n = (unsigned int)*remainder + 7u;
    while (n >= 256u && *shields < max_shields) { ++*shields; n -= 256u; }
    *remainder = *shields < max_shields ? (unsigned short)n : 0;
    return 1;
}

/* The main simulation updates at 30Hz while BWAPI's frame values are 24Hz. */
static inline int sc64_race_shield_tick_30fps(unsigned short *shields,
                                               unsigned short max_shields,
                                               unsigned short *remainder)
{
    unsigned int n;
    if (*shields >= max_shields) { *remainder = 0; return 0; }
    n = (unsigned int)*remainder + 7u;
    if (n >= 320u) { ++*shields; n -= 320u; }
    *remainder = *shields < max_shields ? (unsigned short)n : 0;
    return 1;
}

#endif
