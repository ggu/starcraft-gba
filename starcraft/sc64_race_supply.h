/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_RACE_SUPPLY_H
#define SC64_RACE_SUPPLY_H

/* Derived from the current controlling owner and the race of each live
 * actor or paid product. Origin raw-owner tags deliberately do not matter:
 * a rescued Terran Depot provides Terran supply for its current owner.
 * None of these totals changes the legacy save format. */
typedef struct {
    int used, provided, queued;
} Sc64RaceSupplyLedger;

#ifdef SC64_PAID_WAITING_UNITS
#include <stdint.h>

enum { SC64_QUEUE_WAITING_PROGRESS = INT16_MAX };

/* Only human, completed producers with ordinary supply-using train jobs
 * defer the tail's reservation. Aliased products still use their admitted
 * producer; mode and source permissions remain separate admission checks. */
static inline int race_queue_waiting_units_supported(const Building *q, int type)
{
    const Sc64RaceUnitRule *r = sc64_race_unit_rule(type);
    if (!q->alive || q->owner != SIDE || q->under_construction || !r ||
        (r->race != R_TER && r->race != R_PROTOSS) || (r->flags & SC_RACE_F_LARVA) ||
        !r->supply || !r->build_frames ||
        (r->build_frames * 5 + 3) / 4 >= SC64_QUEUE_WAITING_PROGRESS) return 0;
    if (sc64_race_producer_compatible(q->type, r->producer)) return 1;
#ifdef SC64_SHARED_TRANSPORT
    if (q->type == B_ROBOTICS_FACILITY && sc64_race_raw_unit(type) == 69 &&
        sc64_race_unit_from_raw(69) == type) return 1;
#endif
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) && \
    defined(SC64_P7_CARRIER_TRAINING_ENABLED) && \
    defined(SC64_P7_PROJECTILES_ENABLED) && (defined(SC64_P7_ACTORS) || defined(SC64_TERRAN_CARRIER_PROBE))
    if (q->type == B_STARGATE && type == U_P7_CARRIER && sc64_race_raw_unit(type) == 72)
        return 1;
#endif
    return 0;
}

static inline int race_queue_head_waiting(const Building *q)
{
    return q->queue_count > 0 && (unsigned)q->queue_head < 5u &&
           q->queue_progress == SC64_QUEUE_WAITING_PROGRESS &&
           race_queue_waiting_units_supported(q, q->queue[q->queue_head]);
}

static inline int race_queue_progress_for_hud(const Building *q)
{
    return race_queue_head_waiting(q) ? 0 : q->queue_progress;
}

static inline int race_queue_entry_reserves_supply(const Building *q, int offset, int type)
{
    if (offset == 0 && q->queue_progress != SC64_QUEUE_WAITING_PROGRESS) return 1;
    return !race_queue_waiting_units_supported(q, type);
}
#define SC64_QUEUE_DISPLAY_PROGRESS(q) race_queue_progress_for_hud(q)
#else
#define SC64_QUEUE_DISPLAY_PROGRESS(q) ((q)->queue_progress)
#endif

static inline int race_supply_building_provides(const Building *q)
{
    return !q->under_construction ||
           (race_mode && (q->type == B_LAIR || q->type == B_HIVE));
}

static inline void
race_supply_count_unit(Sc64RaceSupplyLedger *ledger, const Unit *q, int faction)
{
    const Sc64RaceUnitRule *r = sc64_race_unit_rule(q->type);
    if (r && r->race == faction) {
        /* This includes loaded actors and an Egg's paid reservation.
         * The Egg is Zerg even though its target stores the product. */
        ledger->used += unit_slot_supply(q);
        ledger->provided += sc64_race_unit_supply_provided(q->type);
    }
}

static inline void
race_supply_count_building(Sc64RaceSupplyLedger *ledger, const Building *q, int faction)
{
    if (race_supply_building_provides(q)) {
        const Sc64RaceBuildingRule *r = sc64_race_building_rule(q->type);
        if (r && r->race == faction) ledger->provided += r->supply_provided;
    }
    /* Count the product's race, not the producer's race or origin owner.
     * The current owner of the alive producer owns its pending products. */
    for (int j = 0; j < q->queue_count; ++j) {
        int type = q->queue[(q->queue_head + j) % 5];
        const Sc64RaceUnitRule *r = sc64_race_unit_rule(type);
        if (r && r->race == faction
#ifdef SC64_PAID_WAITING_UNITS
            && race_queue_entry_reserves_supply(q, j, type)
#endif
        )
            ledger->queued += r->supply * sc64_race_product_count(type);
    }
}

static inline int
race_supply_ledger_reserved(Sc64RaceSupplyLedger ledger)
{
    return ledger.used + ledger.queued;
}

static inline int race_supply_nuke_reservations(int owner, int faction)
{
#ifdef SC64_SHARED_NUKE_PROBE
    return faction == R_TER ? race_nuke_reserved_supply(owner) : 0;
#else
    (void)owner; (void)faction; return 0;
#endif
}

/* Admission and uncached resource-bar drawing need a fresh view. The normal
 * HUD invalidation and AI census reuse their already-existing actor walks. */
static Sc64RaceSupplyLedger __attribute__((noinline))
race_owner_supply_ledger(int owner, int faction)
{
    Sc64RaceSupplyLedger ledger = {0};
    if (owner < 0 || owner >= 5 || faction < R_TER || faction > R_PROTOSS)
        return ledger;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner)
            race_supply_count_unit(&ledger, &u[i], faction);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner)
            race_supply_count_building(&ledger, &b[i], faction);
    ledger.queued += race_supply_nuke_reservations(owner, faction);
    return ledger;
}
#endif
