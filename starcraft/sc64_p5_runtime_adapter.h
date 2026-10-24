#ifndef SC64_P5_RUNTIME_ADAPTER_H
#define SC64_P5_RUNTIME_ADAPTER_H

/* P5 adapter for authenticated source metadata. This is a bounded authored
 * interpretation, not native CHK execution. RunAIScript rows are replaced by
 * a deterministic paid-pressure policy described below. */
#if defined(SC64_P5_RUNTIME)

#include "sc64_p5_state.h"
#include "sc64_p5_world_adapter.h"

enum {
    SC64_P5_SOURCE_CREATE_COUNT = 11,
    SC64_P5_SOURCE_UNIT_COUNT = 110,
    SC64_P5_SOURCE_BUILDING_COUNT = 69,
    SC64_P5_SOURCE_MINERAL_COUNT = 57,
    SC64_P5_SOURCE_GEYSER_COUNT = 3,
    SC64_P5_PRESSURE_ORDER_COST = 25,
    SC64_P5_PRESSURE_ORDERS_PER_SCRIPT = 2,
    SC64_P5_RUNTIME_FAULT_MAP = 0x20,
    SC64_P5_RUNTIME_FAULT_CAPACITY = 0x21,
    SC64_P5_RUNTIME_FAULT_ACTOR = 0x22
};

/* Source FORC bytes for 008/022. The race-map descriptor retains OWNR/SIDE
 * ownership and race data but not the force array, so pin this authenticated
 * selector map here and reject any descriptor that no longer matches its
 * corresponding P5 census. Slots 2/7 are Force 1, 6 Force 2, 4 Force 3, and
 * 0/3/5 Force 4. */
static const u8 race_p5_force_for_source[8] = {3, 3, 0, 3, 2, 3, 1, 0};
static const u8 race_p5_owner_modes_expected[8] = {2, 2, 3, 1, 1, 2, 1, 0};
static const u8 race_p5_owner_races_expected[8] = {2, 1, 2, 1, 1, 2, 0, 2};
static const u8 race_p5_chk_owners_expected[12] =
    {0, 0, 3, 5, 5, 0, 5, 6, 0, 0, 0, 0};
static const u8 race_p5_chk_sides_expected[12] =
    {2, 0, 2, 0, 0, 2, 1, 2, 7, 7, 7, 4};
static const u8 race_p5_chk_forces_expected[20] =
    {3, 3, 0, 3, 2, 3, 1, 0, 9, 0, 8, 0, 7, 0, 6, 0, 0, 0, 0, 0};

/* Keep raw CHK owner metadata intact, but assign active source forces distinct
 * live-owner buckets because the shared target API only sees owner IDs.
 * Force 1 (slots 2 and 7) is allied; Force 2/3/4 (6/4/3) are separate
 * hostile players. Slot 6 uses NEUTRAL as a compact fifth bucket only in P5;
 * its actual race is still installed below. Inactive slots have no actors. */
static int race_p5_live_owner_for_source(unsigned slot)
{
    switch (slot) {
    case 7: return SIDE;
    case 2: return RESCUABLE;
    case 3: return ENEMY;
    case 4: return 4;
    case 6: return NEUTRAL;
    case 0: case 1: case 5: return NEUTRAL;
    default: return -1;
    }
}

static Sc64P5PersistentState race_p5;
static Sc64P5WorldContext race_p5_world;

static int race_p5_active(void)
{ return !race_mode && race_selected_mission == SC64_P5_GLOBAL_ID; }

static int race_p5_source_owner(unsigned slot)
{
    return race_p5_live_owner_for_source(slot);
}

#ifdef SC64_P5_SHUTTLE_ENABLED
/* Produced actors need the same source provenance as their owning force.
 * Imported and rescued actors retain the authenticated original raw_owner. */
static int race_p5_product_source_owner(int owner)
{
    static const u8 source_for_owner[5] = {7, 3, 6, 2, 4};
    return owner >= SIDE && owner <= 4 ? source_for_owner[owner] : -1;
}
#endif

static int race_p5_runtime_state_valid(void);

static unsigned race_p5_remaining_creations(const Sc64P5PersistentState *state)
{
    const unsigned trigger = 3;
    const Sc64P5SourceTrigger *tr = &sc64_p5_source_triggers[trigger];
    unsigned remaining = 0;
    int started;
    if (!state) return SC64_P5_SOURCE_CREATE_COUNT;
    started = sc64_p5_trigger_mask_test(state->runtime.model.started,
                                        (u8)trigger);
    /* started with cursor zero means every action in this one-shot program
     * has completed; it is distinct from the unstarted zero-cursor state. */
    if (started && !state->runtime.model.action_cursor[trigger]) return 0;
    for (unsigned i = 0; i < tr->action_count; ++i) {
        const Sc64P5SourceAction *a = &sc64_p5_source_actions[tr->first_action + i];
        if (a->opcode == 11 && (!started || i >= state->runtime.model.action_cursor[trigger]))
            remaining += sc64_p5_source_create_count(a);
    }
    return remaining;
}

static unsigned race_p5_reserved_units(void)
{
    return race_p5_active() && race_p5_runtime_state_valid() ?
           race_p5_remaining_creations(&race_p5) :
           (race_p5_active() ? SC64_P5_SOURCE_CREATE_COUNT : 0u);
}

static const char *race_p5_objective_text(void)
{ return "Bring Tassadar and two Zealots to the extraction point."; }
static const char *race_p5_victory_text(void)
{ return "TASSADAR AND ZEALOTS EXTRACTED"; }
static const char *race_p5_failure_text(void)
{ return "THE PROTOSS FORCE WAS LOST"; }

static int race_p5_source_model_preflight(void)
{
    static const u8 enabled_trigger_player[SC64_P5_TRIGGER_COUNT] = {
        18, 7, 18, 7, 7, 18, 18, 18, 19, 20, 21
    };
    unsigned creates = 0, wraiths = 0, battlecruisers = 0, revealers = 0;
    if (!sc64_p5_source_model_valid()) return 0;
    for (unsigned i = 0; i < 12; ++i)
        if (sc64_p5_source_owners[i] != race_p5_chk_owners_expected[i] ||
            sc64_p5_source_sides[i] != race_p5_chk_sides_expected[i]) return 0;
    for (unsigned i = 0; i < 20; ++i)
        if (sc64_p5_source_forces[i] != race_p5_chk_forces_expected[i]) return 0;
    for (unsigned i = 0; i < 64; ++i) {
        if (sc64_p5_source_property_used[i]) return 0;
        for (unsigned byte = 0; byte < 20; ++byte)
            if (sc64_p5_source_properties[i][byte]) return 0;
    }
    for (unsigned t = 0; t < SC64_P5_TRIGGER_COUNT; ++t) {
        const Sc64P5SourceTrigger *tr = &sc64_p5_source_triggers[t];
        unsigned executors = 0;
        for (unsigned p = 0; p < 27; ++p) executors += tr->enabled_players[p] != 0;
        if (executors != 1 || !tr->enabled_players[enabled_trigger_player[t]] ||
            tr->initial_cursor || tr->execution_flags) return 0;
        for (unsigned i = 0; i < tr->action_count; ++i) {
            const Sc64P5SourceAction *a = &sc64_p5_source_actions[tr->first_action + i];
            if (!sc64_p5_action_duration(t, i)) return 0;
            switch (a->opcode) {
            case 1: case 2: case 4: case 7: case 8: case 9: case 10:
            case 12: case 13: case 22: case 26: case 43:
                break;
            case 11:
                if (a->group2 != 0 || a->flags != 0x10u ||
                    sc64_race_unit_from_raw(a->unit) < 0) return 0;
                ++creates;
                if (t != 3) return 0;
                if (i < 9 && a->group1 == 6 && a->unit == 8) ++wraiths;
                else if (i == 9 && a->group1 == 6 && a->unit == 12 &&
                         a->location == 3) ++battlecruisers;
                else if (i == 10 && a->group1 == 13 && a->unit == 101 &&
                         a->location == 3) ++revealers;
                else return 0;
                break;
            case 15:
                if (t != 8 || i != 0 || a->group2 != 0x63697553u ||
                    executors != 1 || !tr->enabled_players[19]) return 0;
                break;
            case 16:
                if (!((t == 9 && i == 1 && a->group2 == 0x49357250u &&
                       a->location == 16) ||
                      (t == 10 && i == 1 && a->group2 == 0x42357250u &&
                       a->location == 17)) || executors != 1 ||
                    !tr->enabled_players[t == 9 ? 20 : 21]) return 0;
                break;
            default: return 0;
            }
            if (a->opcode == 22 &&
                (t != 3 || i != 17 || a->group1 != 13 || a->unit != 101 ||
                 a->group2 || a->modifier || a->location || a->flags != 0x10u))
                return 0;
            if (a->opcode == 43 &&
                (t != 4 || i != 0 || a->group1 != 13 || a->unit != 229 ||
                 a->location != 2 || a->modifier != 4 || a->flags != 0x10u))
                return 0;
            if (a->opcode == 13 &&
                (a->group1 || (a->group2 != 0 && a->group2 != 4) ||
                 (a->modifier != 4 && a->modifier != 5))) return 0;
            if (a->opcode == 26 &&
                (a->location || a->flags || a->group2 > 65535u || a->unit > 2 ||
                 a->modifier != SC64_RESOURCE_MOD_SET)) return 0;
        }
    }
    return creates == SC64_P5_SOURCE_CREATE_COUNT && wraiths == 9 &&
           battlecruisers == 1 && revealers == 1;
}

static int race_p5_preflight(const Sc64RaceMap *map)
{
    unsigned units = 0, buildings = 0, minerals_count = 0, geysers_count = 0;
    if (!map || map->width != SC64_P5_MAP_COLS ||
        map->height != SC64_P5_MAP_ROWS || map->human_slot != 7 ||
        map->unit_count != 239 || !map->units || !map->owner_modes ||
        !map->owner_races || !race_p5_source_model_preflight()) return 0;
    if (MAX_UNITS < SC64_P5_SOURCE_UNIT_COUNT + SC64_P5_SOURCE_CREATE_COUNT ||
        MAX_BUILDINGS < SC64_P5_SOURCE_BUILDING_COUNT ||
        MAX_MINERALS < SC64_P5_SOURCE_MINERAL_COUNT ||
        MAX_RACE_GEYSERS < SC64_P5_SOURCE_GEYSER_COUNT) return 0;
    for (unsigned slot = 0; slot < 8; ++slot)
        if (map->owner_modes[slot] != race_p5_owner_modes_expected[slot] ||
            map->owner_races[slot] != race_p5_owner_races_expected[slot]) return 0;
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        int raw = src->source_type;
        if (src->x_px >= map->width * 16u ||
            src->y_px >= map->height * 16u) return 0;
        if (raw == 214) return 0; /* The P5 imported rows have no start markers. */
        if (raw >= 176 && raw <= 178) {
            if (src->owner != 11) return 0;
            ++minerals_count;
        } else if (raw == 188) {
            if (src->owner != 11) return 0;
            ++geysers_count;
        } else if (sc64_race_unit_from_raw(raw) >= 0) {
            if (src->owner >= 8) return 0;
            ++units;
        } else if (sc64_race_building_from_raw(raw) >= 0) {
            if (src->owner >= 8) return 0;
            ++buildings;
        }
        else return 0;
    }
    return units == SC64_P5_SOURCE_UNIT_COUNT &&
           buildings == SC64_P5_SOURCE_BUILDING_COUNT &&
           minerals_count == SC64_P5_SOURCE_MINERAL_COUNT &&
           geysers_count == SC64_P5_SOURCE_GEYSER_COUNT;
}

/* Called immediately after importing a fresh map, before simulation can
 * consume resources or kill actors. Resume validation intentionally does not
 * use this starting-census check. */
static int race_p5_starting_world_matches_source(const Sc64RaceMap *map)
{
    u8 seen_units[MAX_UNITS], seen_buildings[MAX_BUILDINGS];
    u8 seen_minerals[MAX_MINERALS], seen_geysers[MAX_RACE_GEYSERS];
    unsigned units = 0, buildings = 0, minerals_count = 0;
    if (!map || geyser_count < 0 || geyser_count > MAX_RACE_GEYSERS) return 0;
    memset(seen_units, 0, sizeof(seen_units));
    memset(seen_buildings, 0, sizeof(seen_buildings));
    memset(seen_minerals, 0, sizeof(seen_minerals));
    memset(seen_geysers, 0, sizeof(seen_geysers));
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        unsigned raw = src->source_type;
        int found = 0;
        if (raw >= 176 && raw <= 178) {
            if (src->owner != 11) return 0;
            for (int i = 0; i < MAX_MINERALS; ++i)
                if (!seen_minerals[i] && m[i].amount > 0 && m[i].x == src->x_px &&
                    m[i].y == src->y_px && m[i].style == (int)raw) {
                    seen_minerals[i] = 1; ++minerals_count; found = 1; break;
                }
        } else if (raw == 188) {
            if (src->owner != 11) return 0;
            for (int i = 0; i < geyser_count; ++i)
                if (!seen_geysers[i] && geysers[i].x == src->x_px &&
                    geysers[i].y == src->y_px) {
                    seen_geysers[i] = 1; found = 1; break;
                }
        } else if (sc64_race_unit_from_raw((int)raw) >= 0) {
            if (src->owner >= 8) return 0;
            int expected_owner = race_p5_live_owner_for_source(src->owner);
            for (int i = 0; i < MAX_UNITS; ++i)
                if (!seen_units[i] && u[i].alive && u[i].raw_type == (int)raw &&
                    u[i].raw_owner == src->owner && u[i].owner == expected_owner &&
                    u[i].x == src->x_px && u[i].y == src->y_px) {
                    seen_units[i] = 1; ++units; found = 1; break;
                }
        } else if (sc64_race_building_from_raw((int)raw) >= 0) {
            if (src->owner >= 8) return 0;
            int expected_owner = race_p5_live_owner_for_source(src->owner);
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (!seen_buildings[i] && b[i].alive && b[i].raw_type == (int)raw &&
                    b[i].raw_owner == src->owner && b[i].owner == expected_owner &&
                    b[i].x == src->x_px && b[i].y == src->y_px) {
                    seen_buildings[i] = 1; ++buildings; found = 1; break;
                }
        }
        if (!found) return 0;
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if ((u[i].alive != 0) != (seen_units[i] != 0)) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if ((b[i].alive != 0) != (seen_buildings[i] != 0)) return 0;
    for (int i = 0; i < MAX_MINERALS; ++i)
        if ((m[i].amount > 0) != (seen_minerals[i] != 0)) return 0;
    if (geyser_count != SC64_P5_SOURCE_GEYSER_COUNT) return 0;
    for (int i = 0; i < geyser_count; ++i)
        if (!seen_geysers[i]) return 0;
    return units == SC64_P5_SOURCE_UNIT_COUNT &&
           buildings == SC64_P5_SOURCE_BUILDING_COUNT &&
           minerals_count == SC64_P5_SOURCE_MINERAL_COUNT &&
           (unsigned)geyser_count == SC64_P5_SOURCE_GEYSER_COUNT;
}

static int race_p5_bind_world(const Sc64RaceMap *map)
{
    if (!race_p5_preflight(map)) return 0;
    memset(&race_p5_world, 0, sizeof(race_p5_world));
    race_p5_world.source_current_player = map->human_slot;
    race_p5_world.deaths_by_source = race_p5.deaths_by_source;
    for (unsigned slot = 0; slot < 8; ++slot) {
        int owner = race_p5_live_owner_for_source(slot);
        if (owner < SIDE || owner > 4) return 0;
        race_p5_world.source_slot_active[slot] = 1;
        race_p5_world.live_owner_for_source[slot] = (s8)owner;
        race_p5_world.source_force_for_slot[slot] = race_p5_force_for_source[slot];
    }
    /* The active P5 neutral bucket represents source slot 6's actual race. */
    race_player[NEUTRAL] = map->owner_races[6];
    for (unsigned slot = 8; slot < SC64_P5_SOURCE_SLOT_COUNT; ++slot)
        race_p5_world.live_owner_for_source[slot] = -1;
    return 1;
}

static int race_p5_runtime_state_valid(void)
{ return sc64_p5_persistent_state_valid(&race_p5); }

static int race_p5_runtime_init(const Sc64RaceMap *map)
{
    memset(&race_p5, 0, sizeof(race_p5));
    (void)sc64_p5_runtime_init(&race_p5.runtime);
    if (!race_p5_active()) return 1;
    if (!race_p5_bind_world(map)) {
        race_p5.runtime.fault = SC64_P5_RUNTIME_FAULT_MAP;
        return 0;
    }
    if (!race_p5_starting_world_matches_source(map)) {
        race_p5.runtime.fault = SC64_P5_RUNTIME_FAULT_ACTOR;
        return 0;
    }
    return race_p5_runtime_state_valid();
}

static int race_p5_rebind_loaded_world(const Sc64RaceMap *map)
{
    if (!race_p5_active()) return 1;
    return race_p5_runtime_state_valid() && race_p5_bind_world(map);
}

static void race_p5_increment_death(unsigned source_slot)
{
    if (source_slot < SC64_P5_SOURCE_SLOT_COUNT &&
        race_p5.deaths_by_source[source_slot] != 0xffffffffu)
        ++race_p5.deaths_by_source[source_slot];
}

/* Call before the core marks a killed actor dead. Current Player is the new
 * source owner for rescued actors; otherwise raw_owner keeps CHK provenance. */
static void race_p5_note_unit_death(const Unit *q)
{
    unsigned slot;
    if (!race_p5_active() || !q) return;
    slot = q->owner == SIDE ? race_p5_world.source_current_player :
           (q->raw_owner >= 0 ? (unsigned)q->raw_owner : SC64_P5_SOURCE_SLOT_COUNT);
    race_p5_increment_death(slot);
}

static void race_p5_note_building_death(const Building *q)
{
    unsigned slot;
    if (!race_p5_active() || !q) return;
    slot = q->owner == SIDE ? race_p5_world.source_current_player :
           (q->raw_owner >= 0 ? (unsigned)q->raw_owner : SC64_P5_SOURCE_SLOT_COUNT);
    race_p5_increment_death(slot);
}

static int race_p5_source_force(unsigned source_slot)
{
    if (source_slot >= 8 || !race_p5_world.source_slot_active[source_slot]) return -1;
    return race_p5_world.source_force_for_slot[source_slot];
}

static int race_p5_sources_hostile(unsigned a, unsigned b0)
{
    int force_a = race_p5_source_force(a), force_b = race_p5_source_force(b0);
    /* Slots without source actors are not combatants even though they share
     * the neutral live-owner bucket with the active Force 2 player. */
    if (!((a == 2 || a == 3 || a == 4 || a == 6 || a == 7) &&
          (b0 == 2 || b0 == 3 || b0 == 4 || b0 == 6 || b0 == 7))) return 0;
    if (force_a < 0 || force_b < 0 || force_a == force_b) return 0;
    return 1;
}

static int race_p5_units_hostile(const Unit *a, const Unit *b0)
{
    unsigned slot_a, slot_b;
    if (!a || !b0 || !a->alive || !b0->alive) return 0;
    slot_a = a->owner == SIDE ? race_p5_world.source_current_player :
             (a->raw_owner >= 0 ? (unsigned)a->raw_owner : 99u);
    slot_b = b0->owner == SIDE ? race_p5_world.source_current_player :
             (b0->raw_owner >= 0 ? (unsigned)b0->raw_owner : 99u);
    return race_p5_sources_hostile(slot_a, slot_b);
}

static int race_p5_owners_hostile(int owner_a, int owner_b)
{
    /* This hook runs before generic neutral-owner rejection in P5. Distinct
     * force buckets make coarse owner queries exact without actor lookups. */
    if (owner_a < SIDE || owner_b < SIDE || owner_a > 4 || owner_b > 4 ||
        owner_a == owner_b) return 0;
    if ((owner_a == SIDE || owner_a == RESCUABLE) &&
        (owner_b == SIDE || owner_b == RESCUABLE)) return 0;
    return 1;
}

/* The source marks slot 2 Rescue Passive. Retain raw_owner while changing the
 * current owner, mirroring ordinary contact-rescue behavior in this engine. */
static void race_p5_rescue_tick(void)
{
    int changed = 0;
    if (!race_p5_active() || screen != SCREEN_PLAY ||
        !race_p5_runtime_state_valid()) return;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == RESCUABLE) {
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE) {
            int dx = u[i].x - u[j].x, dy = u[i].y - u[j].y;
            if (dx * dx + dy * dy <= 32 * 32) {
                u[i].owner = SIDE; u[i].selected = 0;
                u[i].order = ORDER_IDLE; u[i].target = -1;
                u[i].tx = u[i].x; u[i].ty = u[i].y;
                changed = 1; break;
            }
        }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == RESCUABLE) {
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE) {
            int dx = b[i].x - u[j].x, dy = b[i].y - u[j].y;
            if (dx * dx + dy * dy <= 32 * 32) {
                b[i].owner = SIDE; changed = 1; break;
            }
        }
    }
    if (changed) {
        supply_used = race_owner_supply(SIDE);
        supply_cap = race_owner_supply_cap(SIDE);
        target_lists_valid = 0; sc64_unit_bucket_active = 0;
        sc64_target_index_end();
    }
}

/* Opaque native AI is not copied. On each authenticated script row, direct at
 * most two already-present idle combatants from the trigger's source force
 * toward the nearest human actor, spending 25 minerals per order. This is a
 * deterministic bounded paid-pressure policy, not native AI equivalence. */
static int race_p5_opaque_ai(void *context, const Sc64P5SourceAction *a)
{
    Sc64P5WorldContext *ctx = context;
    unsigned executor = 27, force;
    int orders = 0;
    u8 source_mask = 0;
    unsigned ai_bit;
    if (!ctx || !a || (a->opcode != 15 && a->opcode != 16) ||
        a->trigger < 8 || a->trigger > 10 || a->index != (a->trigger == 8 ? 0 : 1))
        return 0;
    if ((a->trigger == 8 && (a->opcode != 15 || a->group2 != 0x63697553u)) ||
        (a->trigger == 9 && (a->opcode != 16 || a->group2 != 0x49357250u)) ||
        (a->trigger == 10 && (a->opcode != 16 || a->group2 != 0x42357250u))) return 0;
    for (unsigned player = 0; player < 27; ++player)
        if (sc64_p5_source_triggers[a->trigger].enabled_players[player]) {
            if (executor != 27) return 0;
            executor = player;
        }
    /* TRIG enabled-player positions 19/20/21 encode Force 2/3/4. */
    if (executor != (unsigned)(a->trigger + 11u) ||
        executor < 18 || executor > 21) return 0;
    force = executor - 18u;
    for (unsigned slot = 0; slot < 8; ++slot)
        if (ctx->source_slot_active[slot] && ctx->source_force_for_slot[slot] == force)
            source_mask |= (u8)(1u << slot);
    for (int i = 0; i < MAX_UNITS &&
                    orders < SC64_P5_PRESSURE_ORDERS_PER_SCRIPT; ++i) {
        unsigned source;
        int raw, target_x = 0, target_y = 0, have_target = 0;
        if (!u[i].alive || u[i].owner != ENEMY || u[i].order != ORDER_IDLE ||
            u[i].raw_owner < 0 || u[i].raw_owner >= 8) continue;
        source = (unsigned)u[i].raw_owner;
        if (!(source_mask & (u8)(1u << source)) ||
            sc64_race_is_worker(u[i].type) || sc64_race_unit_damage(u[i].type) <= 0 ||
            race_minerals[ENEMY] < SC64_P5_PRESSURE_ORDER_COST) continue;
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE) {
            if (!have_target || dist2(u[i].x, u[i].y, u[j].x, u[j].y) <
                                dist2(u[i].x, u[i].y, target_x, target_y)) {
                target_x = u[j].x; target_y = u[j].y; have_target = 1;
            }
        }
        if (!have_target) break;
        raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        if (raw < 0) return 0;
        race_minerals[ENEMY] -= SC64_P5_PRESSURE_ORDER_COST;
        u[i].order = ORDER_ATTACK_MOVE; u[i].target = -1;
        u[i].tx = target_x; u[i].ty = target_y;
        u[i].path_timer = 0; u[i].path_retry = 0;
        ++orders;
    }
    ai_bit = a->trigger - 8u;
    race_p5.pressure_dispatched_mask |= (u8)(1u << ai_bit);
    return 1;
}

static int race_p5_world_action(void *context, const Sc64P5SourceAction *a)
{
    Sc64P5WorldContext *ctx = context;
    if (!ctx || !a) return 0;
    if (!sc64_p5_world_action(ctx, a)) return 0;
    if (a->opcode == 10) { cx = ctx->center_x; cy = ctx->center_y; }
    if (a->opcode == 12) race_campaign.objective_stage = a->trigger;
    return 1;
}

static int race_p5_campaign_tick(void)
{
    Sc64P5WorldOps ops;
    int result;
    if (!race_p5_active()) return 1;
    if (!race_p5_runtime_state_valid()) {
        race_p5.runtime.fault = SC64_P5_FAULT_STATE;
        return 0;
    }
    if (screen != SCREEN_PLAY) return 1;
    /* Rescue is a source-world update and must precede Bring/Command
     * predicates for this scheduler tick. */
    race_p5_rescue_tick();
#ifdef SC64_P5_COUNT_SNAPSHOT
    /* Conditions are all sampled before any action dispatch. A failed snapshot
     * leaves the exact existing full-scan callback active. */
    (void)sc64_p5_count_snapshot_begin(&race_p5_world);
#endif
    ops.context = &race_p5_world;
    ops.condition = sc64_p5_world_condition;
    ops.action = race_p5_world_action;
    ops.opaque_ai = race_p5_opaque_ai;
    race_p5_world.victory_requested = 0;
    race_p5_world.defeat_requested = 0;
    result = sc64_p5_adapter_tick(&race_p5.runtime, &ops,
                                  screen != SCREEN_PLAY);
#ifdef SC64_P5_COUNT_SNAPSHOT
    sc64_p5_count_snapshot_end(&race_p5_world);
#endif
    if (result < 0 || !race_p5_runtime_state_valid()) return 0;
    if (race_p5.runtime.model.outcome == SC64_P5_WON) {
        won = 1; lost = 0;
        race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
        race_completed |= 1 << race_selected_mission;
    } else if (race_p5.runtime.model.outcome == SC64_P5_LOST) {
        won = 0; lost = 1;
        race_campaign.phase = SC64_RACE_PHASE_FAILED;
    }
    return 1;
}

#endif /* SC64_P5_RUNTIME */
#endif /* SC64_P5_RUNTIME_ADAPTER_H */
