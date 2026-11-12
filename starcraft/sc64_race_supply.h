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
        if (r && r->race == faction)
            ledger->queued += r->supply * sc64_race_product_count(type);
    }
}

static inline int
race_supply_ledger_reserved(Sc64RaceSupplyLedger ledger)
{
    return ledger.used + ledger.queued;
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
    return ledger;
}
#endif
