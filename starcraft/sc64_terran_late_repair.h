#ifndef SC64_TERRAN_LATE_REPAIR_H
#define SC64_TERRAN_LATE_REPAIR_H

#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_T6_RUNTIME)
#include "sc64_t6_repair_adapter.h"

/* Mechanical classifications follow pinned BWAPI d727fed UnitType.cpp's
 * Mechanical flag and OpenBW bwgame.h order_Repair (references/blizzard-gba).
 * The raw-19 hero Vulture is mechanical too; its scenario-only rule has no
 * production price, so repair uses its source unit data values below. */
static int race_late_repair_unit_type_mechanical(int type)
{
    switch (type) {
    case U_SCV: case U_VULTURE: case U_GOLIATH: case U_TANK: case U_WRAITH:
    case U_DROPSHIP: case U_SCIENCE_VESSEL: case U_BATTLECRUISER:
    case U_NORAD_II: case U_RAYNOR_VULTURE:
        return 1;
    default:
        return 0;
    }
}

static int race_late_repair_unit_raw_mechanical(int raw)
{
    switch (raw) {
    case 2: case 3: case 5: case 7: case 8: case 9: case 11: case 12:
    case 19: case 29:
        return 1;
    default:
        return 0;
    }
}

/* The ordinary Terran structure set and the source's crashed Norad object
 * are mechanical in the pinned BWAPI UnitType flags. Special beacons, doors,
 * traps, and non-Terran buildings deliberately stay out. */
static int race_late_repair_building_raw_mechanical(int raw)
{
    switch (raw) {
    case 106: case 107: case 108: case 109: case 110: case 111: case 112:
    case 113: case 114: case 115: case 116: case 117: case 118: case 120:
    case 121: case 122: case 123: case 125: case 126:
        return 1;
    default:
        return 0;
    }
}

static int race_late_repair_unit_fields_valid(int raw, int owner, int alive,
                                               int hp, int maxhp, int loaded)
{
    return race_late_repair_unit_raw_mechanical(raw) && owner == SIDE && alive &&
           !loaded && hp >= 0 && maxhp > 0 && hp < maxhp;
}

static int race_late_repair_building_fields_valid(int raw, int owner, int alive,
                                                   int complete, int airborne,
                                                   int hp, int maxhp)
{
    return race_late_repair_building_raw_mechanical(raw) && owner == SIDE && alive &&
           complete && !airborne && hp >= 0 && maxhp > 0 && hp < maxhp;
}

static int race_late_repair_worker_valid(int type, int owner, int alive,
                                          int selected, int loaded)
{
    return type == U_SCV && owner == SIDE && alive && selected && !loaded;
}

static int race_late_repair_target_unit_valid(int slot)
{
    if (!race_late_active() || slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        !race_late_repair_unit_type_mechanical(u[slot].type)) return 0;
    return race_late_repair_unit_fields_valid(u[slot].raw_type, u[slot].owner,
                 u[slot].alive, u[slot].hp, u[slot].maxhp,
                 SC64_TRANSPORT_LOADED(&u[slot]));
}

static int race_late_repair_target_building_valid(int slot)
{
    const Sc64RaceBuildingRule *rule;
    if (!race_late_active() || slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive)
        return 0;
    rule = sc64_race_building_rule(b[slot].type);
    if (!rule || rule->race != R_TER ||
        sc64_race_raw_building(b[slot].type) != b[slot].raw_type ||
        !race_late_repair_building_raw_mechanical(b[slot].raw_type)) return 0;
    int hp = b[slot].hp;
    if (hp == b[slot].maxhp && race_building_can_burn(&b[slot]) &&
        (b[slot].shield_remainder & 255)) --hp;
    return race_late_repair_building_fields_valid(b[slot].raw_type, b[slot].owner,
                  b[slot].alive, building_complete(slot), (b[slot].flags & 4) != 0,
                  hp, b[slot].maxhp);
}

/* Returns a target-independent, reference-bounded cost tuple for either
 * actor class. Source data for raw-19 Raynor Vulture is 150/0/900, despite
 * its scenario rule having no producer or training cost. */
static int race_late_repair_target_costs(int unit_target, int slot,
                                         Sc64T6RepairCosts *costs)
{
    unsigned maxhp, minerals_cost, gas_cost, build_frames;
    if (!costs || slot < 0) return 0;
    if (unit_target) {
        const Sc64RaceUnitRule *rule;
        const Sc64TerranLateActorFact *fact;
        if (slot >= MAX_UNITS || !race_late_repair_target_unit_valid(slot)) return 0;
        rule = sc64_race_unit_rule(u[slot].type);
        fact = sc64_terran_late_unit_fact(u[slot].type);
        if (!rule || rule->race != R_TER) return 0;
        maxhp = (unsigned)u[slot].maxhp;
        minerals_cost = fact ? (unsigned)fact->minerals : rule->minerals;
        gas_cost = fact ? (unsigned)fact->gas : rule->gas;
        build_frames = fact ? (unsigned)fact->build_frames : rule->build_frames;
        if (u[slot].type == U_RAYNOR_VULTURE) {
            minerals_cost = 150; gas_cost = 0; build_frames = 900;
        }
    } else {
        const Sc64RaceBuildingRule *rule;
        const Sc64T6BuildingFact *special;
        if (slot >= MAX_BUILDINGS || !race_late_repair_target_building_valid(slot)) return 0;
        rule = sc64_race_building_rule(b[slot].type);
        special = sc64_t6_building_fact((unsigned)b[slot].raw_type);
        if (!rule || rule->race != R_TER) return 0;
        maxhp = (unsigned)b[slot].maxhp;
        minerals_cost = special ? special->minerals : rule->minerals;
        gas_cost = special ? special->gas : rule->gas;
        build_frames = special ? special->build_frames : rule->build_frames;
    }
    return sc64_t6_repair_costs(maxhp, minerals_cost, gas_cost, build_frames, costs);
}

/* The ordinary target click chooses the nearest eligible friendly mechanical
 * target in the engine's existing 26-pixel interaction radius. Unit slots are
 * checked first, then building slots; strict distance comparison keeps the
 * first slot on a tie. */
static int race_late_request_repair(void)
{
    int target = -1, best_d = 26 * 26, unit_target = 0;
    int issued = 0;
    if (!race_late_active()) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!race_late_repair_target_unit_valid(i)) continue;
        int d = dist2(cx, cy, u[i].x, u[i].y);
        if (d < best_d) { target = i; best_d = d; unit_target = 1; }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!race_late_repair_target_building_valid(i)) continue;
        int d = dist2(cx, cy, b[i].x, b[i].y);
        if (d < best_d) { target = MAX_UNITS + i; best_d = d; unit_target = 0; }
    }
    if (target < 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *worker = &u[i];
        if (!race_late_repair_worker_valid(worker->type, worker->owner,
                 worker->alive, worker->selected, SC64_TRANSPORT_LOADED(worker)) ||
            (unit_target && target == i)) continue;
        worker->order = ORDER_REPAIR;
        worker->target = target;
        worker->tx = unit_target ? u[target].x : b[target - MAX_UNITS].x;
        worker->ty = unit_target ? u[target].y : b[target - MAX_UNITS].y;
        worker->carry = worker->work_timer = 0;
#ifdef SC64_WORKER_CARGO_KIND
        sc64_worker_cargo_clear(worker);
        sc64_worker_gather_pending_set(worker, 0);
#endif
        worker->path_timer = worker->path_retry = 0;
        ++issued;
    }
    return issued;
}

static void race_late_repair_stop(Unit *worker)
{
    worker->order = ORDER_IDLE;
    worker->target = -1;
    worker->carry = worker->work_timer = 0;
#ifdef SC64_WORKER_CARGO_KIND
    sc64_worker_cargo_clear(worker);
    sc64_worker_gather_pending_set(worker, 0);
#endif
}

static int __attribute__((noinline)) race_late_repair_tick_unit(int slot)
{
    Unit *worker;
    int unit_target, target_slot, tx, ty, keep, timer;
    Sc64T6RepairCosts costs;
    if (!race_late_active() || slot < 0 || slot >= MAX_UNITS || !u[slot].alive)
        return 0;
    worker = &u[slot];
    if (worker->owner != SIDE || worker->type != U_SCV || worker->target < 0) {
        race_late_repair_stop(worker);
        return 1;
    }
    unit_target = worker->target < MAX_UNITS;
    target_slot = unit_target ? worker->target : worker->target - MAX_UNITS;
    if ((unit_target && (target_slot == slot ||
                         !race_late_repair_target_unit_valid(target_slot))) ||
        (!unit_target && !race_late_repair_target_building_valid(target_slot)) ||
        worker->carry < 0 || worker->carry >= SC64_T6_REPAIR_FP_ONE ||
        worker->work_timer < 0 || worker->work_timer > 65535 ||
        !race_late_repair_target_costs(unit_target, target_slot, &costs)) {
        race_late_repair_stop(worker);
        return 1;
    }
    if (unit_target) { tx = u[target_slot].x; ty = u[target_slot].y; }
    else { tx = b[target_slot].x; ty = b[target_slot].y; }
    worker->tx = tx; worker->ty = ty;
    if (dist2(worker->x, worker->y, tx, ty) > 36 * 36) {
        move_toward(worker, tx, ty);
        return 1;
    }
    /* Four native repair updates per five 30-Hz simulation ticks, matching
     * the established T6 building-repair adapter and persisted frame clock. */
    if (frame % 5 == 0) return 1;
    int *hp = unit_target ? &u[target_slot].hp : &b[target_slot].hp;
    int maxhp = unit_target ? u[target_slot].maxhp : b[target_slot].maxhp;
    int old_hp = *hp, repair_hp = old_hp;
    int fractional_only = !unit_target && old_hp == maxhp &&
        race_building_can_burn(&b[target_slot]) &&
        (b[target_slot].shield_remainder & 255);
    /* Price and accrue the remaining fraction as one integer HP; this is an
     * approximation within the existing integer repair and payment model. */
    if (fractional_only) --repair_hp;
    timer = worker->work_timer;
    keep = sc64_t6_repair_step(&repair_hp, maxhp, &worker->carry, &timer,
                               &minerals, &gas, &costs);
    if (!fractional_only || repair_hp == maxhp) *hp = repair_hp;
    if (!unit_target && repair_hp == maxhp &&
        (fractional_only || repair_hp > old_hp) &&
        race_building_can_burn(&b[target_slot]))
        b[target_slot].shield_remainder &= ~255;
    worker->work_timer = timer;
    if (!keep) race_late_repair_stop(worker);
    return 1;
}

#endif /* SC64_TERRAN_LATE_RUNTIME && SC64_T6_RUNTIME */
#endif
