#ifndef SC64_T6_REPAIR_WORLD_H
#define SC64_T6_REPAIR_WORLD_H

#ifdef SC64_T6_RUNTIME
#include "sc64_t6_repair_adapter.h"

/* The first repair integration covers completed mechanical Terran
 * structures. Mobile mechanical actors need their own command support. */
static int
race_t6_repair_building_valid(int slot)
{
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive ||
        b[slot].owner != SIDE || !building_complete(slot)) return 0;
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(b[slot].type);
    if (!rule || rule->race != R_TER ||
        (rule->worker == 255 && b[slot].raw_type != 126)) return 0;
    return sc64_t6_repair_target_valid((unsigned)b[slot].raw_type, 1, 1, 1,
                                      b[slot].hp, b[slot].maxhp, 1, 1);
}

static int
race_t6_request_repair(void)
{
    int best = -1, best_distance = 26 * 26, issued = 0;
    if (!race_t6_active()) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!race_t6_repair_building_valid(i)) continue;
        int d = dist2(cx, cy, b[i].x, b[i].y);
        if (d < best_distance) { best = i; best_distance = d; }
    }
    if (best < 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].selected &&
            u[i].type == U_SCV && !SC64_TRANSPORT_LOADED(&u[i])) {
            u[i].order = ORDER_REPAIR;
            u[i].target = MAX_UNITS + best;
            u[i].tx = b[best].x; u[i].ty = b[best].y;
            u[i].carry = u[i].work_timer = 0;
#ifdef SC64_WORKER_CARGO_KIND
            sc64_worker_cargo_clear(&u[i]);
            sc64_worker_gather_pending_set(&u[i], 0);
#endif
            u[i].path_timer = u[i].path_retry = 0;
            ++issued;
        }
    if (issued) selected_building = -1;
    return issued;
}

static void
race_t6_repair_stop(Unit *worker)
{
    worker->order = ORDER_IDLE; worker->target = -1;
    worker->carry = worker->work_timer = 0;
#ifdef SC64_WORKER_CARGO_KIND
    sc64_worker_cargo_clear(worker);
    sc64_worker_gather_pending_set(worker, 0);
#endif
}

static int __attribute__((noinline))
race_t6_repair_tick_unit(int slot)
{
    Unit *worker = &u[slot];
    int target = worker->target - MAX_UNITS;
    if (!race_t6_active() || worker->owner != SIDE || worker->type != U_SCV ||
        !race_t6_repair_building_valid(target)) {
        race_t6_repair_stop(worker);
        return 1;
    }
    Building *structure = &b[target];
    if (dist2(worker->x, worker->y, structure->x, structure->y) > 36 * 36) {
        move_toward(worker, structure->x, structure->y);
        return 1;
    }
    /* Four native 24 Hz repair steps in five 30 Hz GBA simulation ticks.
     * Frame is save-persistent, so the cadence resumes without extra state. */
    if (frame % 5 == 0) return 1;
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(structure->type);
    const Sc64T6BuildingFact *special = sc64_t6_building_fact((unsigned)structure->raw_type);
    Sc64T6RepairCosts costs;
    if (!sc64_t6_repair_costs((unsigned)structure->maxhp,
                             special ? special->minerals : rule->minerals,
                             special ? special->gas : rule->gas,
                             special ? special->build_frames : rule->build_frames,
                             &costs)) {
        race_t6_repair_stop(worker);
        return 1;
    }
    int timer = worker->work_timer;
    int keep = sc64_t6_repair_step(&structure->hp, structure->maxhp,
                                  &worker->carry, &timer, &minerals, &gas, &costs);
    worker->work_timer = timer;
    if (!keep) race_t6_repair_stop(worker);
    return 1;
}
#endif
#endif
