/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sc64_gas_service_queue.h"

static Sc64GasServiceQueue gas_service;
static u8 gas_service_finished[(MAX_BUILDINGS + 7) / 8];

static void sc64_gas_service_reset(void)
{
    memset(&gas_service, 0, sizeof gas_service);
    memset(gas_service_finished, 0, sizeof gas_service_finished);
}

static int sc64_gas_service_eligible(unsigned slot, unsigned site)
{
    const Unit *worker = &u[slot];
    if (!worker->alive || !IS_WORKER(worker->type) || worker->order != ORDER_GAS ||
        worker->target != (int)site || sc64_worker_cargo_matches_order(worker) ||
        !building_complete(site) || !ui_gas_building(b[site].type) ||
        b[site].owner != worker->owner) return 0;
#ifdef SC64_NEAREST_RESOURCE_DEPOT
    if (worker->build == -2 && !sc64_worker_gather_pending(worker)) return 0;
#endif
#ifdef SC64_P6_COMBAT_ENABLED
    if (race_p6_combat_unit_burrowed(slot)) return 0;
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_unit_burrowed(slot)) return 0;
#endif
    return dist2(worker->x, worker->y, b[site].x, b[site].y) <= 24 * 24;
}

/* Old journals store each remaining countdown but no cross-worker arrival
 * order. Finish the most advanced trips first, keeping every remaining tick.
 * Iterating slots in ascending order breaks ties in the legacy evidence. */
static void sc64_gas_service_migrate_legacy(void)
{
    sc64_gas_service_reset();
    for (unsigned slot = 0; slot < MAX_UNITS; ++slot) {
        Unit *worker = &u[slot];
        unsigned site = (unsigned)worker->target;
        if (worker->work_timer <= 0 || site >= MAX_BUILDINGS ||
            !sc64_gas_service_eligible(slot, site)) continue;
        unsigned prev = 0, id = gas_service.head[site];
        while (id && u[id - 1].work_timer <= worker->work_timer) {
            prev = id;
            id = gas_service.next[id - 1];
        }
        gas_service.next[slot] = (u8)id;
        gas_service.site[slot] = (u8)(site + 1);
        if (prev) gas_service.next[prev - 1] = (u8)(slot + 1);
        else gas_service.head[site] = (u8)(slot + 1);
        if (!id) gas_service.tail[site] = (u8)(slot + 1);
    }
}

static void __attribute__((noinline)) sc64_gas_service_begin(void)
{
    memset(gas_service_finished, 0, sizeof gas_service_finished);
    for (unsigned site = 0; site < MAX_BUILDINGS; ++site) {
        for (unsigned id = gas_service.head[site]; id;) {
            unsigned slot = id - 1;
            id = gas_service.next[slot];
            if (sc64_gas_service_eligible(slot, site)) continue;
            if (u[slot].order == ORDER_GAS) u[slot].work_timer = 0;
            sc64_gas_service_remove(&gas_service, slot);
        }
    }
}

static int __attribute__((noinline)) sc64_gas_service_ready(Unit *worker)
{
    unsigned slot = (unsigned)(worker - u);
    unsigned site = (unsigned)worker->target;
    if (site >= MAX_BUILDINGS || !sc64_gas_service_eligible(slot, site)) return 0;
    if (!sc64_gas_service_join(&gas_service, site, slot)) return 0;
    if (gas_service.head[site] != slot + 1) return 0;
    u8 mask = (u8)(1u << (site % 8));
    if (gas_service_finished[site / 8] & mask) return 0;
    if (!worker->work_timer) worker->work_timer = 60;
    if (--worker->work_timer) return 0;
    gas_service_finished[site / 8] |= mask;
    sc64_gas_service_remove(&gas_service, slot);
    return 1;
}
