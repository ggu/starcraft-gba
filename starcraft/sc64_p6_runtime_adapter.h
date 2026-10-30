#ifndef SC64_P6_RUNTIME_ADAPTER_H
#define SC64_P6_RUNTIME_ADAPTER_H

#if defined(SC64_P6_RUNTIME)

#include "sc64_p6_state.h"
#include "sc64_p6_world_adapter.h"
#ifdef SC64_P6_COUNT_SNAPSHOT
#include "sc64_p6_count_snapshot.h"
#endif

/* Save-hook contract for the shared adapter:
 *   Sc64P6PersistentState race_p6;
 * The root save adapter should copy this value only while P6 is active and
 * require all-zero bytes while another campaign is active. Validate this
 * complete object before committing any decoded world state. THG2-to-building
 * slot indices remain transient and are rebound by exact source identity. */
enum {
    SC64_P6_SOURCE_PLACEMENT_COUNT = 204,
    SC64_P6_SOURCE_UNIT_ROWS = 204,
    SC64_P6_SOURCE_UNIT_CAPACITY = 240,
    SC64_P6_SOURCE_BUILDING_CAPACITY = 160,
    SC64_P6_RUNTIME_FAULT_MAP = 0x30,
    SC64_P6_RUNTIME_FAULT_BINDING = 0x31,
    SC64_P6_RUNTIME_FAULT_ACTOR = 0x32
};

typedef struct {
    u8 enabled;
    u8 building_slot;
} Sc64P6DoodadBinding;

static Sc64P6PersistentState race_p6;
static Sc64P6WorldContext race_p6_world;
static Sc64P6DoodadBinding race_p6_doodad_bindings[SC64_P6_DOODAD_SOURCE_COUNT];
static u8 race_p6_doodad_bindings_valid;
static int race_p6_runtime_policy_ready(const Sc64RaceMap *map);

static int race_p6_active(void)
{ return !race_mode && race_selected_mission == SC64_P6_GLOBAL_ID; }

static int race_p6_live_owner_for_source(unsigned slot)
{
    switch (slot) {
    case 7: return SIDE;
    case 1: case 2: case 5: return RESCUABLE;
    case 3: return ENEMY;
    case 6: return 4;
    case 0: case 4: return NEUTRAL;
    default: return -1;
    }
}

static int race_p6_source_owner(unsigned slot)
{
    /* Map import asks this before runtime context binding. Keep this as the
     * authenticated descriptor slot mapping, not the zeroed transient table. */
    return race_p6_live_owner_for_source(slot);
}

static int race_p6_owners_hostile(int a, int b0)
{
    if (a < SIDE || a > 4 || b0 < SIDE || b0 > 4 || a == b0) return 0;
    if ((a == SIDE || a == RESCUABLE) &&
        (b0 == SIDE || b0 == RESCUABLE)) return 0;
    if (a == NEUTRAL || b0 == NEUTRAL) return 0;
    return 1;
}

static void race_p6_increment_death(unsigned source)
{
    if (source < SC64_P6_SOURCE_SLOT_COUNT &&
        race_p6.deaths_by_source[source] < SC64_P6_MAX_SOURCE_DEATHS)
        ++race_p6.deaths_by_source[source];
}

static void race_p6_note_unit_death(const Unit *q)
{
    unsigned source;
    if (!race_p6_active() || !q) return;
    source = q->owner == SIDE ? race_p6_world.source_current_player :
             q->raw_owner >= 0 ? (unsigned)q->raw_owner : SC64_P6_SOURCE_SLOT_COUNT;
    race_p6_increment_death(source);
}

static void race_p6_note_building_death(const Building *q)
{
    unsigned source;
    if (!race_p6_active() || !q) return;
    source = q->owner == SIDE ? race_p6_world.source_current_player :
             q->raw_owner >= 0 ? (unsigned)q->raw_owner : SC64_P6_SOURCE_SLOT_COUNT;
    race_p6_increment_death(source);
}

static int race_p6_runtime_state_valid(void)
{ return sc64_p6_persistent_state_valid(&race_p6); }

static int race_p6_actor_implemented_capabilities(int raw)
{
#if defined(SC64_P6_COMBAT_ENABLED)
    return race_p6_combat_implemented_capabilities(raw);
#else
    (void)raw;
    return 0;
#endif
}

static int race_p6_resolve_actor(void *context, unsigned raw,
                                  Sc64P6WorldActorBinding *binding)
{
    int type;
    (void)context;
    if (!binding || raw >= 228u) return 0;
    type = sc64_race_unit_from_raw((int)raw);
    if (type < 0) return 0;
    binding->internal_type = type;
    binding->implemented_capabilities =
        (unsigned)race_p6_actor_implemented_capabilities((int)raw);
    return 1;
}

static int race_p6_is_men_raw(void *context, unsigned raw)
{
    (void)context;
    return raw < 228u && sc64_race_unit_from_raw((int)raw) >= 0;
}

static int race_p6_doodad_row_enabled(unsigned row)
{
    if (row >= SC64_P6_DOODAD_SOURCE_COUNT ||
        sc64_p6_doodads[row].sprite_only) return 0;
    return (race_p6.doodad_enabled[row >> 3] >> (row & 7u)) & 1u;
}

static int race_p6_doodad_enabled_slot(int slot)
{
    if (slot < 0 || slot >= MAX_BUILDINGS) return 1;
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i)
        if (!sc64_p6_doodads[i].sprite_only &&
            race_p6_doodad_bindings[i].building_slot == (u8)slot)
            return race_p6_doodad_row_enabled(i);
    /* A source door without a rebound row must stay safely closed. */
    return 1;
}

static int race_p6_doodad_slot_bound(int slot)
{
    if (slot < 0 || slot >= MAX_BUILDINGS) return 0;
    for (unsigned row = 0; row < SC64_P6_DOODAD_SOURCE_COUNT; ++row)
        if (!sc64_p6_doodads[row].sprite_only &&
            race_p6_doodad_bindings[row].building_slot == (unsigned)slot)
            return 1;
    return 0;
}

static int race_p6_building_collidable(int slot)
{
    int raw;
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive) return 0;
    raw = b[slot].raw_type >= 0 ? b[slot].raw_type : sc64_race_raw_building(b[slot].type);
    /* The pinned PC pathfinder excludes door actors from dynamic collision.
     * P6's un-scripted upper-level door props follow that policy. Its six
     * authored pit gates retain our explicit state-dependent tile blockers;
     * exact native N64 tile/door-order behavior remains unverified. */
    if (raw == 205 || raw == 206) return 0;
    if (raw >= SC64_P6_RAW_DOOR_FIRST && raw <= SC64_P6_RAW_DOOR_LAST)
        return race_p6_doodad_enabled_slot(slot);
    {
        unsigned flags = sc64_game_building_actor_flags(b[slot].type);
        return !flags || (flags & SC64_GAME_ACTOR_COLLIDABLE) != 0;
    }
}

static int race_p6_prop_state_valid(const Building *q, int type, int raw)
{
    const Sc64ZpBuildingFact *fact = sc64_zp_building_fact(type);
    int door = raw >= SC64_P6_RAW_DOOR_FIRST && raw <= SC64_P6_RAW_DOOR_LAST;
    if (!q || !fact || fact->raw != raw || q->type != type ||
        q->maxhp != fact->hp || q->hp > q->maxhp) return 0;
    /* Engine HP is integer source points. Combat retains dead trap identities
     * with nonpositive HP; invincible doors must remain alive. */
    return q->alive ? q->hp > 0 : !door && q->hp <= 0;
}

static int race_p6_source_row_valid(const Sc64RaceMap *map)
{
    unsigned units = 0, buildings = 0, minerals_count = 0, geysers_count = 0;
    if (!map || map->width != SC64_P6_MAP_COLS ||
        map->height != SC64_P6_MAP_ROWS || map->human_slot != 7 ||
        map->unit_count != SC64_P6_SOURCE_UNIT_ROWS || !map->units ||
        !map->owner_modes || !map->owner_races) return 0;
    for (unsigned i = 0; i < 8; ++i) {
        static const u8 owner_modes[8] = {2,3,3,1,2,3,1,0};
        static const u8 owner_races[8] = {0,0,2,1,1,2,1,2};
        if (map->owner_modes[i] != owner_modes[i] ||
            map->owner_races[i] != owner_races[i]) return 0;
    }
    for (unsigned i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *src = &map->units[i];
        unsigned raw = src->source_type;
        if (src->x_px >= map->width * 16u || src->y_px >= map->height * 16u)
            return 0;
        if (raw == SC64_RAW_START_LOCATION) continue;
        if (raw >= 176 && raw <= 178) {
            if (src->owner != SC64_OWNER_NEUTRAL) return 0;
            ++minerals_count;
        } else if (raw == 188) {
            if (src->owner != SC64_OWNER_NEUTRAL) return 0;
            ++geysers_count;
        } else if (sc64_race_unit_from_raw((int)raw) >= 0) {
            if (src->owner >= 8) return 0;
            ++units;
        } else if (sc64_race_building_from_raw((int)raw) >= 0) {
            if (src->owner >= 8) return 0;
            ++buildings;
        } else return 0;
    }
    return units + buildings + minerals_count + geysers_count ==
               SC64_P6_SOURCE_PLACEMENT_COUNT &&
           units + SC64_P6_CREATE_ACTIONS <= MAX_UNITS &&
           buildings + SC64_P6_DOODAD_UNIT_PROP_COUNT <= MAX_BUILDINGS &&
           minerals_count <= MAX_MINERALS && geysers_count <= MAX_RACE_GEYSERS;
}

static int race_p6_preflight(const Sc64RaceMap *map)
{
    unsigned unit_props = 0, sprites = 0, suicide_force_units = 0;
    if (!sc64_p6_runtime_metadata_valid() || !race_p6_source_row_valid(map) ||
        SC64_P6_DOODAD_SOURCE_COUNT != 37u ||
        SC64_P6_DOODAD_UNIT_PROP_COUNT != 27u ||
        SC64_P6_DOODAD_SPRITE_COUNT != 10u) return 0;
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i) {
        const Sc64P6Doodad *d = &sc64_p6_doodads[i];
        if (d->index != i || d->source_unused || d->sprite_only > 1 ||
            d->x >= map->width * 32u || d->y >= map->height * 32u) return 0;
        if (d->sprite_only) {
            ++sprites;
            if (d->raw_owner >= 8) return 0;
        } else {
            ++unit_props;
            if (d->raw_owner >= 8 ||
                sc64_race_building_from_raw((int)d->raw_type) < 0) return 0;
        }
    }
    for (unsigned i = 0; i < map->unit_count; ++i)
        if (map->units[i].owner == 3 &&
            sc64_race_unit_from_raw(map->units[i].source_type) >= 0)
            ++suicide_force_units;
    if (unit_props != SC64_P6_DOODAD_UNIT_PROP_COUNT ||
        sprites != SC64_P6_DOODAD_SPRITE_COUNT || !suicide_force_units) return 0;
    /* No supported runtime currently claims the authenticated opaque `Suic`
     * script or all P6-specific actor effects. Do not expose this mission
     * until the actual combat integration supplies those capabilities. */
    if (!race_p6_runtime_policy_ready(map)) return 0;
    return 1;
}

static int race_p6_runtime_policy_ready(const Sc64RaceMap *map)
{
    /* This symbol is intentionally weak in meaning: a combat helper may
     * provide the source actor handlers, but native source AI remains an
     * explicit authored replacement in race_p6_run_ai. */
    if (!map || !map->units) return 0;
    /* Imported starting heroes need the same actor support as later creates. */
    for (unsigned i = 0; i < map->unit_count; ++i) {
        unsigned raw = map->units[i].source_type;
        const Sc64P6ActorPolicyRequirement *r =
            sc64_p6_actor_policy_requirement((int)raw);
        if (r && (race_p6_actor_implemented_capabilities((int)raw) &
                  r->required_capabilities) != r->required_capabilities) return 0;
    }
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i) {
        unsigned raw = sc64_p6_doodads[i].raw_type;
        const Sc64P6ActorPolicyRequirement *r = sc64_p6_actor_policy_requirement((int)raw);
        if (r && (race_p6_actor_implemented_capabilities((int)raw) &
                  r->required_capabilities) != r->required_capabilities) return 0;
    }
    for (unsigned t = 0; t < SC64_P6_TRIGGER_COUNT; ++t) {
        const Sc64P6SourceTrigger *tr = &sc64_p6_source_triggers[t];
        for (unsigned i = 0; i < tr->action_count; ++i) {
            const Sc64P6SourceAction *a = &sc64_p6_source_actions[tr->first_action + i];
            const Sc64P6ActorPolicyRequirement *r =
                sc64_p6_actor_policy_requirement((int)a->unit);
            if (a->opcode == SC64_P6_ACTION_CREATE_UNIT && r &&
                (race_p6_actor_implemented_capabilities((int)a->unit) &
                 r->required_capabilities) != r->required_capabilities) return 0;
        }
    }
    /* The bounded Suic replacement is defined below and is checked against
     * the live imported Force 2 roster when the runtime is initialized. */
    return 1;
}

static int race_p6_source_doodad_set(void *context,
                                     const Sc64P6SourceAction *a)
{
    u8 selected[SC64_P6_DOODAD_SOURCE_COUNT] = {0};
    u8 next_enabled[SC64_P6_DOODAD_SOURCE_COUNT] = {0};
    int left, top, right, bottom;
    (void)context;
    if (!a || a->opcode != SC64_P6_ACTION_SET_DOODAD ||
        !sc64_p6_selector_valid(a->group1) ||
        (a->modifier != 0 && a->modifier != 4 &&
         a->modifier != 5 && a->modifier != 6) ||
        !sc64_p6_world_location(&race_p6_world, a->location,
                                &left, &top, &right, &bottom)) return 0;
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i) {
        const Sc64P6Doodad *d = &sc64_p6_doodads[i];
        int enabled;
        if (d->sprite_only || d->raw_type != a->unit ||
            d->x / 2u < (unsigned)left || d->x / 2u >= (unsigned)right ||
            d->y / 2u < (unsigned)top || d->y / 2u >= (unsigned)bottom)
            continue;
        if (sc64_p6_world_owner_matches(&race_p6_world, a->group1,
                d->raw_owner, race_p6_live_owner_for_source(d->raw_owner)) != 1)
            continue;
        switch (a->modifier) {
        case 0: case 6: enabled = !race_p6_doodad_row_enabled(i); break;
        case 4: enabled = 1; break;
        case 5: enabled = 0; break;
        default: return 0;
        }
        if (race_p6_doodad_bindings[i].building_slot >= MAX_BUILDINGS) return 0;
        selected[i] = 1; next_enabled[i] = (u8)enabled;
    }
    /* The source gate sequence deliberately names both door orientations
     * in each region, even when only one exists there. An empty selection
     * completes the action without changing the world. */
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i) if (selected[i]) {
        unsigned byte = i >> 3, bit = 1u << (i & 7u);
        int before = race_p6_doodad_row_enabled(i);
        if (next_enabled[i]) race_p6.doodad_enabled[byte] |= (u8)bit;
        else race_p6.doodad_enabled[byte] &= (u8)~bit;
        race_p6_doodad_bindings[i].enabled = next_enabled[i];
        if (before != next_enabled[i]) {
            invalidate_path_grid();
            target_lists_valid = 0;
            sc64_unit_bucket_active = 0;
            sc64_target_index_end();
        }
    }
    /* Dispatch has advanced its action cursor but has not installed the
     * action's timer yet. Whole-runtime validation belongs after dispatch,
     * when that scheduler transaction is complete. The writes above affect
     * only validated, unit-backed doodad rows. */
    return 1;
}

static int race_p6_source_ai(void *context, const Sc64P6SourceAction *a)
{
    int x = 0, y = 0, n = 0, actors = 0;
    (void)context;
    if (!race_p6_active() || !a || a->trigger != 13 || a->index != 0 ||
        a->opcode != SC64_P6_ACTION_RUN_AI || a->group2 != 0x63697553u ||
        a->group1 || a->unit || a->location || a->flags) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
        x += u[i].x; y += u[i].y; ++n;
    }
    if (!n) return 0;
    x /= n; y /= n;
    /* Source trigger 13 executes for selector 19 (Force 2). FORC assigns
     * source slot 3 to that force; source slot 6 belongs to Force 4.
     * Authored bounded replacement for opaque RunAIScript("Suic"): issue
     * one ordinary AttackMove to the current human army centroid. It retains
     * the source slot 3 identities and uses the game order system. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_owner == 3 && u[i].owner == ENEMY) {
            u[i].order = ORDER_ATTACK_MOVE;
            u[i].target = -1;
            u[i].tx = x; u[i].ty = y;
            u[i].path_timer = 0; u[i].path_retry = 0;
            ++actors;
        }
    return actors != 0;
}

static int race_p6_world_actor_create(void *context,
                                       const Sc64P6WorldCreateRequest *request)
{
    int slot;
    (void)context;
    if (!race_p6_active() || !request || request->raw_type >= 228u ||
        request->count != 1 || request->live_owner < SIDE || request->live_owner > 4)
        return 0;
    /* UPRP order 117 is deliberately fail-closed until a verified handler
     * publishes a capability and applies the effect below. */
    slot = spawn_unit(request->live_owner, request->internal_type,
                      request->x, request->y);
    if (slot < 0 || slot >= MAX_UNITS) return 0;
    u[slot].raw_type = (s16)request->raw_type;
    u[slot].raw_owner = (s8)request->source_owner;
#if defined(SC64_P6_COMBAT_ENABLED)
    if (request->has_properties &&
        !race_p6_apply_source_properties(slot, &request->properties)) {
        if (request->live_owner == SIDE && supply_used >= unit_supply(request->internal_type))
            supply_used -= unit_supply(request->internal_type);
        u[slot].alive = 0;
        return 0;
    }
#else
    if (request->has_properties) {
        if (request->live_owner == SIDE && supply_used >= unit_supply(request->internal_type))
            supply_used -= unit_supply(request->internal_type);
        u[slot].alive = 0;
        return 0;
    }
#endif
    return 1;
}

static int race_p6_actors_removed(void *context)
{
    (void)context;
    /* Like door actions, removal executes between cursor/timer writes. */
    target_lists_valid = 0;
    sc64_unit_bucket_active = 0;
    sc64_target_index_end();
    return 1;
}

static int race_p6_world_context_init(const Sc64RaceMap *map)
{
    if (!map || map->human_slot != 7) return 0;
    memset(&race_p6_world, 0, sizeof(race_p6_world));
    race_p6_world.source_current_player = 7;
    race_p6_world.map_cols = map->width;
    race_p6_world.map_rows = map->height;
    race_p6_world.deaths_by_source = race_p6.deaths_by_source;
    race_p6_world.resolve_actor = race_p6_resolve_actor;
    race_p6_world.create_actor = race_p6_world_actor_create;
    race_p6_world.is_men_raw = race_p6_is_men_raw;
    race_p6_world.run_ai = race_p6_source_ai;
    race_p6_world.set_doodad_state = race_p6_source_doodad_set;
    race_p6_world.actors_removed = race_p6_actors_removed;
    for (unsigned slot = 0; slot < 8; ++slot) {
        int owner = race_p6_live_owner_for_source(slot);
        if (owner < SIDE || owner > 4) return 0;
        race_p6_world.source_slot_active[slot] = 1;
        race_p6_world.live_owner_for_source[slot] = (s8)owner;
        race_p6_world.source_force_for_slot[slot] =
            (u8)sc64_p6_source_forces[slot];
    }
    race_player[SIDE] = map->owner_races[7];
    race_player[RESCUABLE] = map->owner_races[2];
    race_player[ENEMY] = map->owner_races[3];
    race_player[4] = map->owner_races[6];
    /* Inactive source slots 0/4 share the passive bucket and own no actors.
     * Give this otherwise unused race identity a canonical save value. */
    race_player[NEUTRAL] = R_TER;
    for (unsigned slot = 8; slot < SC64_P6_SOURCE_SLOT_COUNT; ++slot)
        race_p6_world.live_owner_for_source[slot] = -1;
    return 1;
}

static int race_p6_bind_doodads(const Sc64RaceMap *map, int spawn)
{
    u8 seen[SC64_P6_DOODAD_SOURCE_COUNT] = {0};
    if (!race_p6_active() || !map || !sc64_p6_persistent_state_valid(&race_p6))
        return 0;
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i) {
        const Sc64P6Doodad *src = &sc64_p6_doodads[i];
        int type, owner, slot = -1;
        if (src->sprite_only) {
            race_p6_doodad_bindings[i].building_slot = 0xffu;
            race_p6_doodad_bindings[i].enabled = 0;
            continue;
        }
        if (src->raw_owner >= 8 || src->x >= map->width * 32u ||
            src->y >= map->height * 32u || src->index != i) return 0;
        type = sc64_race_building_from_raw((int)src->raw_type);
        owner = race_p6_live_owner_for_source(src->raw_owner);
        if (type < 0 || owner < 0 || src->raw_type == 214u) return 0;
        if (spawn) {
            slot = spawn_build(owner, type, src->x / 2, src->y / 2, 0);
            if (slot < 0 || slot >= MAX_BUILDINGS) return 0;
            b[slot].raw_type = (s16)src->raw_type;
            b[slot].raw_owner = (s8)src->raw_owner;
            b[slot].flags = src->flags;
        } else {
            for (int j = 0; j < MAX_BUILDINGS; ++j)
                if (b[j].raw_type == (int)src->raw_type &&
                    b[j].raw_owner == (int)src->raw_owner &&
                    b[j].x == (int)(src->x / 2u) && b[j].y == (int)(src->y / 2u)) {
                    if (slot >= 0) return 0;
                    slot = j;
                }
            if (slot < 0 || b[slot].owner != owner || b[slot].flags != src->flags ||
                !race_p6_prop_state_valid(&b[slot], type, (int)src->raw_type)) return 0;
        }
        if ((unsigned)slot >= MAX_BUILDINGS || seen[i]) return 0;
        seen[i] = 1;
        race_p6_doodad_bindings[i].building_slot = (u8)slot;
        race_p6_doodad_bindings[i].enabled = (u8)race_p6_doodad_row_enabled(i);
    }
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i)
        if (!sc64_p6_doodads[i].sprite_only && !seen[i]) return 0;
    race_p6_doodad_bindings_valid = 1;
    return 1;
}

static int race_p6_doodad_bindings_still_valid(void)
{
    if (!race_p6_doodad_bindings_valid) return 0;
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i) {
        const Sc64P6Doodad *src = &sc64_p6_doodads[i];
        unsigned slot = race_p6_doodad_bindings[i].building_slot;
        if (src->sprite_only) {
            if (slot != 0xffu) return 0;
            continue;
        }
        if (slot >= MAX_BUILDINGS || b[slot].raw_type != (int)src->raw_type ||
            b[slot].raw_owner != (int)src->raw_owner ||
            b[slot].x != (int)(src->x / 2u) || b[slot].y != (int)(src->y / 2u) ||
            b[slot].owner != race_p6_live_owner_for_source(src->raw_owner) ||
            b[slot].flags != src->flags ||
            !race_p6_prop_state_valid(&b[slot],
                 sc64_race_building_from_raw((int)src->raw_type),
                 (int)src->raw_type)) return 0;
    }
    return 1;
}

static int race_p6_starting_world_matches_source(const Sc64RaceMap *map)
{
    u8 seen_units[MAX_UNITS] = {0};
    u8 seen_buildings[MAX_BUILDINGS] = {0};
    u8 seen_minerals[MAX_MINERALS] = {0};
    u8 seen_geysers[MAX_RACE_GEYSERS] = {0};
    unsigned units = 0, buildings = 0, minerals_count = 0;
    if (!map || geyser_count < 0 || geyser_count > MAX_RACE_GEYSERS) return 0;
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        unsigned raw = src->source_type;
        int found = 0;
        if (raw == SC64_RAW_START_LOCATION) continue;
        if (raw >= 176 && raw <= 178) {
            for (int i = 0; i < MAX_MINERALS; ++i)
                if (!seen_minerals[i] && m[i].amount > 0 &&
                    m[i].x == src->x_px && m[i].y == src->y_px &&
                    m[i].style == (int)raw) {
                    seen_minerals[i] = 1; ++minerals_count; found = 1; break;
                }
        } else if (raw == 188) {
            for (int i = 0; i < geyser_count; ++i)
                if (!seen_geysers[i] && geysers[i].x == src->x_px &&
                    geysers[i].y == src->y_px) {
                    seen_geysers[i] = 1; found = 1; break;
                }
        } else if (sc64_race_unit_from_raw((int)raw) >= 0) {
            int owner = race_p6_live_owner_for_source(src->owner);
            for (int i = 0; i < MAX_UNITS; ++i)
                if (!seen_units[i] && u[i].alive && u[i].raw_type == (int)raw &&
                    u[i].raw_owner == src->owner && u[i].owner == owner &&
                    u[i].x == src->x_px && u[i].y == src->y_px) {
                    seen_units[i] = 1; ++units; found = 1; break;
                }
        } else if (sc64_race_building_from_raw((int)raw) >= 0) {
            int owner = race_p6_live_owner_for_source(src->owner);
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (!seen_buildings[i] && b[i].alive && b[i].raw_type == (int)raw &&
                    b[i].raw_owner == src->owner && b[i].owner == owner &&
                    b[i].x == src->x_px && b[i].y == src->y_px) {
                    seen_buildings[i] = 1; ++buildings; found = 1; break;
                }
        }
        if (!found) return 0;
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && !seen_units[i]) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && !seen_buildings[i]) {
        int is_prop = 0;
        for (unsigned j = 0; j < SC64_P6_DOODAD_SOURCE_COUNT; ++j)
            if (!sc64_p6_doodads[j].sprite_only &&
                race_p6_doodad_bindings[j].building_slot == (u8)i &&
                b[i].raw_type == (int)sc64_p6_doodads[j].raw_type &&
                b[i].raw_owner == (int)sc64_p6_doodads[j].raw_owner) {
                is_prop = 1; break;
            }
        if (!is_prop) return 0;
    }
    for (int i = 0; i < MAX_MINERALS; ++i)
        if ((m[i].amount > 0) != (seen_minerals[i] != 0)) return 0;
    if ((unsigned)geyser_count > MAX_RACE_GEYSERS) return 0;
    for (int i = 0; i < geyser_count; ++i)
        if (!seen_geysers[i]) return 0;
    return units + buildings + minerals_count == SC64_P6_SOURCE_PLACEMENT_COUNT -
           (unsigned)geyser_count && (unsigned)geyser_count <= MAX_RACE_GEYSERS;
}

static int race_p6_runtime_init(const Sc64RaceMap *map)
{
    memset(&race_p6, 0, sizeof(race_p6));
    sc64_p6_runtime_init(&race_p6.runtime);
    memset(race_p6_doodad_bindings, 0xff, sizeof(race_p6_doodad_bindings));
    race_p6_doodad_bindings_valid = 0;
    if (!race_p6_active()) return 1;
    if (!race_p6_preflight(map) || !race_p6_world_context_init(map) ||
        !race_p6_starting_world_matches_source(map)) {
        race_p6.runtime.fault = SC64_P6_RUNTIME_FAULT_MAP;
        return 0;
    }
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i)
        if (!sc64_p6_doodads[i].sprite_only && sc64_p6_doodads[i].initial_enabled)
            race_p6.doodad_enabled[i >> 3] |= (u8)(1u << (i & 7u));
    if (!race_p6_bind_doodads(map, 1) ||
        !race_p6_starting_world_matches_source(map)) {
        race_p6.runtime.fault = SC64_P6_RUNTIME_FAULT_BINDING;
        return 0;
    }
    return sc64_p6_persistent_state_valid(&race_p6);
}

static int race_p6_rebind_loaded_world(const Sc64RaceMap *map)
{
    if (!race_p6_active()) return 1;
    if (!race_p6_preflight(map) || !sc64_p6_persistent_state_valid(&race_p6) ||
        !race_p6_world_context_init(map) || !race_p6_bind_doodads(map, 0)) return 0;
    return 1;
}

static void race_p6_rescue_tick(void)
{
    int changed = 0;
    if (!race_p6_active() || screen != SCREEN_PLAY || !race_p6_runtime_state_valid()) return;
#ifdef SC64_P6_RESCUE_GROUPED
    /* Rescue candidates are already partitioned by live owner in the unit
     * array. A compact frame-local list avoids rescanning every slot for each
     * candidate; appending at the exact outer-loop point preserves the
     * baseline's one-way chaining semantics. */
    _Static_assert(MAX_UNITS <= 256, "P6 rescue slots must fit in u8");
    u8 side_slots[MAX_UNITS];
    unsigned side_count = 0;
    for (unsigned j = 0; j < MAX_UNITS; ++j)
        if (u[j].alive && u[j].owner == SIDE) side_slots[side_count++] = (u8)j;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == RESCUABLE) {
        for (unsigned k = 0; k < side_count; ++k) {
            int j = side_slots[k];
            int dx = u[i].x-u[j].x, dy = u[i].y-u[j].y;
            if (dx*dx+dy*dy <= 32*32) {
                #ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
                if (!race_bc_before_unit_transfer(&u[i],SIDE,-1)) break;
                #endif
                u[i].owner=SIDE; u[i].selected=0; u[i].target=-1;
                u[i].order=ORDER_IDLE; u[i].tx=u[i].x; u[i].ty=u[i].y;
                side_slots[side_count++] = (u8)i;
                changed=1; break;
            }
        }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == RESCUABLE) {
            for (unsigned k = 0; k < side_count; ++k) {
                int j = side_slots[k];
                int dx=b[i].x-u[j].x, dy=b[i].y-u[j].y;
                if (dx*dx+dy*dy <= 32*32) {
                    if (SC64_BC_ALLOW_BUILDING_TRANSFER(i,SIDE)) { b[i].owner=SIDE; changed=1; }
                    break;
                }
            }
        }
#else
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == RESCUABLE) {
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE) {
            int dx = u[i].x-u[j].x, dy = u[i].y-u[j].y;
            if (dx*dx+dy*dy <= 32*32) {
                #ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
                if (!race_bc_before_unit_transfer(&u[i],SIDE,-1)) break;
                #endif
                u[i].owner=SIDE; u[i].selected=0; u[i].target=-1;
                u[i].order=ORDER_IDLE; u[i].tx=u[i].x; u[i].ty=u[i].y;
                changed=1; break;
            }
        }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == RESCUABLE) {
            for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE) {
                int dx=b[i].x-u[j].x, dy=b[i].y-u[j].y;
                if (dx*dx+dy*dy <= 32*32) {
                    if (SC64_BC_ALLOW_BUILDING_TRANSFER(i,SIDE)) { b[i].owner=SIDE; changed=1; }
                    break;
                }
            }
        }
#endif
    if (changed) { target_lists_valid=0; sc64_unit_bucket_active=0; sc64_target_index_end(); }
}

static int race_p6_campaign_tick(void)
{
    Sc64P6WorldOps ops;
    int result;
#ifdef SC64_P6_COUNT_SNAPSHOT
    Sc64P6CountSnapshot count_snapshot;
    Sc64P6CountSnapshotOpsContext count_context;
#endif
    if (!race_p6_active()) return 1;
    if (!sc64_p6_persistent_state_valid(&race_p6) ||
        !race_p6_doodad_bindings_still_valid()) return 0;
    race_p6_world.elapsed_frames = race_campaign.elapsed_frames;
    race_p6_world.victory_requested = 0;
    race_p6_world.defeat_requested = 0;
    race_p6_world.objective_refresh_requested = 0;
    ops.context = &race_p6_world;
    ops.condition = sc64_p6_world_condition;
    ops.action = sc64_p6_world_action;
    ops.opaque_ai = race_p6_source_ai;
#ifdef SC64_P6_COUNT_SNAPSHOT
    count_context.world = &race_p6_world;
    count_context.snapshot = &count_snapshot;
#ifdef SC64_P6_PRUNE_STARTED
    (void)sc64_p6_count_snapshot_begin(&count_snapshot, &race_p6_world,
                                      &race_p6.runtime.model, 1);
#else
    (void)sc64_p6_count_snapshot_begin(&count_snapshot, &race_p6_world,
                                      &race_p6.runtime.model, 0);
#endif
    ops.context = &count_context;
    ops.condition = sc64_p6_count_snapshot_condition;
    ops.action = sc64_p6_count_snapshot_action;
    ops.opaque_ai = sc64_p6_count_snapshot_opaque_ai;
#endif
#ifdef SC64_P6_PREVALIDATED_TICK
    /* init/rebind can set bindings_valid only after full immutable-program
       preflight and successful world binding. The still-valid check above is
       required on every frame before using this entry. */
    if (!race_p6_doodad_bindings_valid) return 0;
    result = sc64_p6_adapter_tick_prevalidated(&race_p6.runtime, &ops, 0);
#else
    result = sc64_p6_adapter_tick(&race_p6.runtime, &ops, 0);
#endif
#ifdef SC64_P6_COUNT_SNAPSHOT
    /* Also discard on paused/terminal/error returns; action callbacks already
       invalidate before any world mutation within the tick. */
    sc64_p6_count_snapshot_end(&count_snapshot);
#endif
    if (result < 0 || !sc64_p6_persistent_state_valid(&race_p6)) return 0;
    for (unsigned i=0;i<SC64_P6_DOODAD_SOURCE_COUNT;++i) {
        if (sc64_p6_doodads[i].sprite_only) continue;
        race_p6_doodad_bindings[i].enabled=(u8)race_p6_doodad_row_enabled(i);
    }
    if (race_p6.runtime.model.outcome == SC64_P6_WON) {
        won=1; lost=0; race_campaign.phase=SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags|=SC64_RACE_OBJECTIVE_PRIMARY;
        race_completed |= 1 << race_selected_mission;
    } else if (race_p6.runtime.model.outcome == SC64_P6_LOST) {
        won=0; lost=1; race_campaign.phase=SC64_RACE_PHASE_FAILED;
    }
    return 1;
}

static const char *race_p6_objective_text(void)
{ return "Rescue the heroes; bring Zeratul to the extraction point."; }
static const char *race_p6_victory_text(void)
{ return "THE PROTOSS FORCE SURVIVES"; }
static const char *race_p6_failure_text(void)
{ return "THE PROTOSS FORCE WAS LOST"; }

/* Numeric source presentation event. Text/audio assets are intentionally not
 * copied; the host UI can inspect these IDs and expose a policy-owned cue. */
static int race_p6_presentation(u8 *opcode, u16 *string_id, u16 *wav_id)
{
    if (!opcode || !string_id || !wav_id || !race_p6_active()) return 0;
    for (unsigned t=0;t<SC64_P6_TRIGGER_COUNT;++t) {
        u8 cursor=race_p6.runtime.model.action_cursor[t];
        const Sc64P6SourceAction *a;
        if (!cursor || !race_p6.runtime.action_ticks[t] ||
            !sc64_p6_source_action(t,cursor-1u,&a)) continue;
        if (a->opcode==SC64_P6_ACTION_TRANSMISSION ||
            a->opcode==SC64_P6_ACTION_DISPLAY_TEXT ||
            a->opcode==SC64_P6_ACTION_PLAY_WAV) {
            *opcode=a->opcode; *string_id=(u16)a->string_id; *wav_id=(u16)a->wav_id;
            return 1;
        }
    }
    return 0;
}

static unsigned race_p6_reserved_units(void)
{
    return race_p6_active() && race_p6_runtime_state_valid() ?
           sc64_p6_remaining_creations(&race_p6.runtime) :
           (race_p6_active() ? SC64_P6_CREATE_ACTIONS : 0u);
}

#endif /* SC64_P6_RUNTIME */
#endif
