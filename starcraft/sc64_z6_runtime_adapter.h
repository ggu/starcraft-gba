#ifndef SC64_Z6_RUNTIME_ADAPTER_H
#define SC64_Z6_RUNTIME_ADAPTER_H

#if defined(SC64_Z6_RUNTIME)

#if !defined(SC64_Z6_ACTORS) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_ZP_CAMPAIGN_STORAGE) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 1
#error "Z6 runtime requires Z6 actors and the full unified Zerg storage profile"
#endif

#include "sc64_z6_state.h"
#include "sc64_z6_world_adapter.h"

enum {
    SC64_Z6_RUNTIME_FAULT_MAP = 0x40,
    SC64_Z6_RUNTIME_FAULT_WORLD = 0x41,
    SC64_Z6_RUNTIME_FAULT_HOOK = 0x42,
    SC64_Z6_RUNTIME_FAULT_ACTOR = 0x43,
    SC64_Z6_AI_INTERVAL_TICKS = 240,
    SC64_Z6_AI_SCRIPT_ZER6 = 0x3672655a,
    SC64_Z6_AI_OWNER_SLOT = 1,
    SC64_Z6_AI_LOCATION = 3
};

/* Hallucination and energy are applied through the existing Unit.flags/carry
 * fields below. The opaque Zer6 program has a deliberately small authored
 * replacement: slot 1's existing combat units attack-move to source location
 * 3, then at most one normally-paid Protoss unit is queued every 240 active
 * mission ticks. This is a bounded campaign policy, not native AI bytecode. */
typedef struct {
    void *context;
    void (*mark_hallucinated)(void *context, unsigned unit_slot);
    void (*set_energy_percent)(void *context, unsigned unit_slot, u8 percent);
    int (*run_ai_script)(void *context, unsigned script_id,
                         unsigned location_id, unsigned trigger_player);
} Sc64Z6RuntimeHooks;

typedef struct {
    Sc64Z6PersistentState *state;
    Sc64Z6WorldContext world;
    Sc64Z6WorldOps ops;
    Sc64Z6RuntimeHooks hooks;
    const Sc64RaceMap *map;
} Sc64Z6RuntimeBinding;

_Static_assert(sizeof(Sc64Z6RuntimeBinding) <= 208,
               "Z6 transient world binding bound");

static int race_z6_active(void)
{ return !race_mode && race_selected_mission == SC64_Z6_GLOBAL_ID; }

static Sc64Z6WorldContext race_z6_world;
static Sc64Z6RuntimeHooks race_z6_hooks;
static Sc64Z6WorldOps race_z6_ops;
static const Sc64RaceMap *race_z6_map;

static int race_z6_authored_ai_pulse(void);
static int race_z6_source_owner(unsigned slot);
static int race_z6_hallucination_mark_slot(unsigned slot);
static int race_z6_hallucination_cleanup_dead(Unit *q);

static int race_z6_product_source_owner(int owner)
{
    /* Slot 1 is the only Protoss economy. Source-created slot-2 Tassadars
     * and imported actors overwrite this product default with exact CHK
     * provenance before any condition query can observe them. */
    return owner == SIDE ? 3 : owner == ENEMY ? 1 : -1;
}

static int race_z6_authored_run_ai(void *context, unsigned script_id,
                                    unsigned location_id,
                                    unsigned trigger_player)
{
    Sc64Z6WorldContext *ctx = (Sc64Z6WorldContext *)context;
    if (!ctx || script_id != SC64_Z6_AI_SCRIPT_ZER6 ||
        location_id != SC64_Z6_AI_LOCATION ||
        trigger_player != SC64_Z6_AI_OWNER_SLOT ||
        ctx->source_current_player != SC64_Z6_AI_OWNER_SLOT ||
        !race_z6_map || race_z6_map->owner_races[SC64_Z6_AI_OWNER_SLOT] != R_PROTOSS ||
        race_z6_source_owner(SC64_Z6_AI_OWNER_SLOT) != ENEMY)
        return 0;
    if (!race_z6_authored_ai_pulse()) return 0;
    race_z6.ai_plan_ticks = SC64_Z6_AI_INTERVAL_TICKS;
    return 1;
}

static int race_z6_source_owner(unsigned slot)
{
    const Sc64RaceMap *map = race_z6_map;
    /* Map import asks this before runtime binding. Resolve the immutable
       descriptor directly in that phase, then use the bound source context. */
    if (!map && SC64_Z6_GLOBAL_ID < SC64_RACE_MAP_COUNT)
        map = &sc64_race_maps[SC64_Z6_GLOBAL_ID];
    if (slot < 8u && map && map->owner_modes && map->human_slot < 8u)
        return slot == map->human_slot ? SIDE : map->owner_modes[slot];
    if (slot >= SC64_Z6_SOURCE_SLOT_COUNT || !race_z6_world.source_slot_active[slot])
        return -1;
    return race_z6_world.live_owner_for_source[slot];
}

static int race_z6_ai_source_unit(const Unit *q)
{
    return q && q->alive && q->owner == race_z6_source_owner(SC64_Z6_AI_OWNER_SLOT) && q->raw_owner ==
        SC64_Z6_AI_OWNER_SLOT;
}

static int race_z6_ai_product_spawn_match(const Unit *q)
{
    int expected_producer;
    if (!q) return 0;
    switch (q->type) {
    case U_ZEALOT: case U_DRAGOON: expected_producer = B_GATEWAY; break;
    case U_SCOUT: expected_producer = B_STARGATE; break;
    case U_REAVER: expected_producer = B_ROBOTICS_FACILITY; break;
    default: return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        b[i].owner == race_z6_source_owner(SC64_Z6_AI_OWNER_SLOT) && b[i].raw_owner == SC64_Z6_AI_OWNER_SLOT &&
        b[i].type == expected_producer) {
        int spawn_x = b[i].x + 20;
        if (spawn_x >= WORLD_W - 8) spawn_x = b[i].x - 20;
        if (q->x == spawn_x && q->y == b[i].y + 4) return 1;
    }
    return 0;
}

/* Generic Protoss queue completion does not carry CHK source identity. In
 * this profile only the computer slot-1 economy runs this authored producer
 * plan, so reconcile its four possible paid products into source ownership
 * before trigger queries/death accounting observe them. Source-created actors
 * already have raw_owner set and are left untouched. */
static void race_z6_reconcile_ai_products(void)
{
    if (!race_z6_map || race_z6_map->owner_races[SC64_Z6_AI_OWNER_SLOT] != R_PROTOSS ||
        race_z6_source_owner(SC64_Z6_AI_OWNER_SLOT) != ENEMY) return;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        int raw;
        if (!q->alive || q->owner != race_z6_source_owner(SC64_Z6_AI_OWNER_SLOT) || q->raw_owner >= 0 ||
            !race_z6_ai_product_spawn_match(q)) continue;
        raw = sc64_race_raw_unit(q->type);
        if (raw < 0 || raw > 255) continue;
        q->raw_type = (s16)raw;
        q->raw_owner = SC64_Z6_AI_OWNER_SLOT;
    }
}

static unsigned race_z6_ai_pending_source_creates(void)
{
    unsigned pending = 0;
    for (unsigned trigger = 0; trigger < SC64_Z6_TRIGGER_COUNT; ++trigger) {
        const Sc64Z6SourceTrigger *tr = &sc64_z6_source_triggers[trigger];
        u8 cursor = race_z6.runtime.model.action_cursor[trigger];
        int started = (race_z6.runtime.model.started & ((u16)1u << trigger)) != 0;
        unsigned first_pending = started ? cursor : 0;
        if (started && !cursor) continue; /* Already completed. */
        if (first_pending > tr->action_count) continue;
        for (unsigned index = first_pending; index < tr->action_count; ++index) {
            const Sc64Z6SourceAction *a = &sc64_z6_source_actions[tr->first_action + index];
            if (a->opcode == 11) ++pending;
        }
    }
    return pending;
}

static unsigned race_z6_ai_committed_unit_slots(void)
{
    unsigned committed = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive)
        for (int q = 0; q < b[i].queue_count; ++q) {
            int type = b[i].queue[(b[i].queue_head + q) % 5];
            committed += type == U_ZERGLING ? 2u : 1u;
        }
    return committed;
}

static int race_z6_ai_find_producer(int building_type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (building_complete(i) && b[i].owner == race_z6_source_owner(SC64_Z6_AI_OWNER_SLOT) &&
            b[i].raw_owner == SC64_Z6_AI_OWNER_SLOT &&
            b[i].raw_type == sc64_race_raw_building(building_type) &&
            b[i].type == building_type && b[i].queue_count == 0)
            return i;
    return -1;
}

/* The replacement script directs only slot 1's combat roster to location 3.
 * A round-robin priority tries cheaper products after a failed normal queue
 * attempt; each successful queue advances the persisted phase. It never
 * creates actors or resources directly, and it reserves slots for every
 * remaining authenticated source CreateUnit action. */
static int race_z6_authored_ai_pulse(void)
{
    static const u8 product_type[4] = { U_DRAGOON, U_ZEALOT, U_SCOUT, U_REAVER };
    static const u8 producer_type[4] = {
        B_GATEWAY, B_GATEWAY, B_STARGATE, B_ROBOTICS_FACILITY
    };
    int left, top, right, bottom;
    int target_x, target_y;
    unsigned free_slots = 0;
    if (!race_z6_map || !race_z6_active() ||
        race_z6_map->owner_races[SC64_Z6_AI_OWNER_SLOT] != R_PROTOSS ||
        race_z6_source_owner(SC64_Z6_AI_OWNER_SLOT) != ENEMY ||
        race_owner_identity(ENEMY) != R_PROTOSS ||
        !sc64_z6_world_location(&race_z6_world, SC64_Z6_AI_LOCATION,
                                &left, &top, &right, &bottom)) return 0;
    target_x = (left + right) / 2;
    target_y = (top + bottom) / 2;
    race_z6_reconcile_ai_products();
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (!race_z6_ai_source_unit(q) || sc64_race_is_worker(q->type) ||
            sc64_race_unit_damage(q->type) <= 0) continue;
        if (q->order != ORDER_ATTACK_MOVE || q->tx != target_x || q->ty != target_y) {
            q->order = ORDER_ATTACK_MOVE;
            q->target = -1;
            q->tx = (s16)target_x;
            q->ty = (s16)target_y;
            q->path_x = q->path_y = 0;
        }
    }
    for (int i = 0; i < MAX_UNITS; ++i) free_slots += !u[i].alive;
    if (free_slots <= race_z6_ai_pending_source_creates() +
                      race_z6_ai_committed_unit_slots()) return 1;
    for (unsigned offset = 0; offset < 4; ++offset) {
        unsigned phase = (race_z6.ai_plan_phase + offset) & 3u;
        int producer = race_z6_ai_find_producer(producer_type[phase]);
        if (producer < 0) continue;
        if (!race_queue_unit(ENEMY, producer, product_type[phase])) continue;
        race_z6.ai_plan_phase = (u8)((phase + 1u) & 3u);
        return 1;
    }
    return 1;
}

/* One persistent scheduler edge for the authored replacement. Kept separate
 * from the trigger executor so host fixtures can verify timer/reload behavior
 * without manufacturing unrelated mission conditions. */
static int race_z6_authored_ai_advance(int user_paused)
{
    if (user_paused || race_z6.runtime.model.outcome != SC64_Z6_RUNNING ||
        !(race_z6.runtime.model.started & (1u << 8))) return 1;
    race_z6_reconcile_ai_products();
    if (race_z6.ai_plan_ticks) --race_z6.ai_plan_ticks;
    if (!race_z6.ai_plan_ticks) {
        if (!race_z6_authored_ai_pulse()) return 0;
        race_z6.ai_plan_ticks = SC64_Z6_AI_INTERVAL_TICKS;
    }
    return 1;
}

/* This engine already reserves flags bit 3 as the hallucination marker (the
 * Z5 Consume path checks it) and stores caster energy in carry as Q8. Z6's
 * only energy-bearing creation is raw Tassadar (79), whose source maximum is
 * 250. These adapters mutate only those fields. */
static void race_z6_mark_hallucinated(void *context, unsigned slot)
{
    (void)context;
    if (!race_z6_hallucination_mark_slot(slot))
        race_z6.runtime.fault = SC64_Z6_RUNTIME_FAULT_ACTOR;
}

static void race_z6_set_energy_percent(void *context, unsigned slot, u8 percent)
{
    (void)context;
    if (slot >= MAX_UNITS || !u[slot].alive || u[slot].raw_type != 79 ||
        percent > 100) return;
    u[slot].carry = (int)((250u * 256u * percent) / 100u);
}

/* Convert each authenticated TRIG execution-player mask to the single
 * source player whose CurrentPlayer selector is in force for this record.
 * Z6 has player 2 for trigger 8 and Force 1 (one member in this scenario) for
 * triggers 0..7. Ambiguous future force masks fail closed. */
static int race_z6_trigger_executor(const Sc64Z6WorldContext *ctx,
                                    unsigned trigger, unsigned *slot)
{
    unsigned resolved = SC64_Z6_SOURCE_SLOT_COUNT;
    if (!ctx || !slot || trigger >= SC64_Z6_TRIGGER_COUNT) return 0;
    for (unsigned p = 0; p < 27; ++p) {
        if (!sc64_z6_trigger_enabled(trigger, p)) continue;
        if (p < 8) {
            if (!ctx->source_slot_active[p] || resolved != SC64_Z6_SOURCE_SLOT_COUNT)
                return 0;
            resolved = p;
        } else if (p >= 18 && p <= 21) {
            unsigned force = p - 18u;
            for (unsigned s = 0; s < 8; ++s)
                if (ctx->source_slot_active[s] &&
                    ctx->source_force_for_slot[s] == force) {
                    if (resolved != SC64_Z6_SOURCE_SLOT_COUNT) return 0;
                    resolved = s;
                }
            if (resolved == SC64_Z6_SOURCE_SLOT_COUNT) return 0;
        } else return 0;
    }
    if (resolved >= SC64_Z6_SOURCE_SLOT_COUNT) return 0;
    *slot = resolved;
    return 1;
}

static int race_z6_runtime_condition(void *context,
                                      const Sc64Z6RuntimeState *state,
                                      const Sc64Z6SourceCondition *condition)
{
    Sc64Z6WorldContext *ctx = (Sc64Z6WorldContext *)context;
    unsigned executor;
    u8 previous;
    int result;
    if (!ctx || !state || !condition ||
        !race_z6_trigger_executor(ctx, condition->trigger, &executor)) return -1;
    previous = ctx->source_current_player;
    ctx->source_current_player = (u8)executor;
    result = sc64_z6_world_condition(ctx, state, condition);
    ctx->source_current_player = previous;
    return result;
}

static int race_z6_runtime_action(void *context,
                                   const Sc64Z6SourceAction *action)
{
    Sc64Z6WorldContext *ctx = (Sc64Z6WorldContext *)context;
    unsigned executor;
    u8 previous;
    int result;
    if (!ctx || !action || !race_z6_trigger_executor(ctx, action->trigger,
                                                       &executor)) return 0;
    previous = ctx->source_current_player;
    ctx->source_current_player = (u8)executor;
    result = sc64_z6_world_action(ctx, action);
    ctx->source_current_player = previous;
    if (result == 1) for (unsigned i = 0; i < MAX_UNITS; ++i)
        if (!u[i].alive && (u[i].flags & 8u) &&
            !race_z6_hallucination_cleanup_dead(&u[i])) return 0;
    return result;
}

static int race_z6_map_shape_valid(const Sc64RaceMap *map)
{
    unsigned units = 0, buildings = 0, minerals_count = 0, geysers_count = 0;
    unsigned creates = 0;
    if (!map || race_mode || race_selected_mission != SC64_Z6_GLOBAL_ID ||
        map->width != SC64_Z6_MAP_COLS ||
        map->height != SC64_Z6_MAP_ROWS || map->human_slot >= 8 ||
        !map->owner_modes || !map->owner_races || !map->units ||
        !sc64_z6_runtime_metadata_valid()) return 0;
    for (unsigned slot = 0; slot < 8; ++slot)
        if (map->owner_modes[slot] > 4 || map->owner_races[slot] > 2) return 0;
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        unsigned raw = src->source_type;
        if (src->x_px >= map->width * 16u || src->y_px >= map->height * 16u)
            return 0;
        if (raw == 214u) continue; /* Source start-location marker. */
        if (raw >= 176u && raw <= 178u) {
            if (src->owner != 11u) return 0;
            ++minerals_count;
        } else if (raw == 188u) {
            if (src->owner != 11u) return 0;
            ++geysers_count;
        } else if (sc64_race_unit_from_raw((int)raw) >= 0) {
            if (src->owner >= 8u) return 0;
            ++units;
        } else if (sc64_race_building_from_raw((int)raw) >= 0) {
            if (src->owner >= 8u) return 0;
            ++buildings;
        } else return 0;
    }
    for (unsigned i = 0; i < SC64_Z6_SOURCE_ACTION_COUNT; ++i) {
        const Sc64Z6SourceAction *a = &sc64_z6_source_actions[i];
        if (a->opcode == 11) {
            if (a->modifier) return 0; /* All authenticated create actions are one actor. */
            ++creates;
        }
    }
    return units + creates <= MAX_UNITS && buildings <= MAX_BUILDINGS &&
           minerals_count <= MAX_MINERALS && geysers_count <= MAX_RACE_GEYSERS;
}

static int race_z6_bind_world(const Sc64RaceMap *map,
                              Sc64Z6PersistentState *state,
                              Sc64Z6RuntimeBinding *binding)
{
    if (!binding || !state || !race_z6_map_shape_valid(map) ||
        !sc64_z6_persistent_state_valid(state)) return 0;
    memset(binding, 0, sizeof(*binding));
    binding->state = state;
    binding->map = map;
    binding->hooks = race_z6_hooks;
    binding->world.source_current_player = map->human_slot;
    binding->world.deaths_by_source = state->deaths_by_source;
    binding->world.moved_locations_mask = state->moved_locations_mask;
    memcpy(binding->world.moved_location, state->moved_location,
           sizeof(state->moved_location));
    binding->world.mark_hallucinated = binding->hooks.mark_hallucinated ?
        binding->hooks.mark_hallucinated : race_z6_mark_hallucinated;
    binding->world.hallucination_context = binding->hooks.context;
    binding->world.set_energy_percent = binding->hooks.set_energy_percent ?
        binding->hooks.set_energy_percent : race_z6_set_energy_percent;
    binding->world.energy_context = binding->hooks.context;
    binding->world.run_ai_script = binding->hooks.run_ai_script ?
        binding->hooks.run_ai_script : race_z6_authored_run_ai;
    binding->world.ai_context = binding->hooks.run_ai_script ?
        binding->hooks.context : &race_z6_world;
    for (unsigned slot = 0; slot < 8; ++slot) {
        int owner = slot == map->human_slot ? SIDE : map->owner_modes[slot];
        if (owner < SIDE || owner > 4 || sc64_z6_source_forces[slot] > 3u)
            return 0;
        binding->world.source_slot_active[slot] = 1;
        binding->world.live_owner_for_source[slot] = (s8)owner;
        binding->world.source_force_for_slot[slot] = sc64_z6_source_forces[slot];
    }
    for (unsigned slot = 8; slot < SC64_Z6_SOURCE_SLOT_COUNT; ++slot)
        binding->world.live_owner_for_source[slot] = -1;
    binding->ops.context = &binding->world;
    binding->ops.condition = race_z6_runtime_condition;
    binding->ops.action = race_z6_runtime_action;
    return 1;
}

static int race_z6_starting_world_matches_source(const Sc64RaceMap *map)
{
    u8 seen_units[MAX_UNITS] = {0}, seen_buildings[MAX_BUILDINGS] = {0};
    u8 seen_minerals[MAX_MINERALS] = {0}, seen_geysers[MAX_RACE_GEYSERS] = {0};
    unsigned units = 0, buildings = 0, mineral_count = 0;
    if (!map || geyser_count < 0 || geyser_count > MAX_RACE_GEYSERS) return 0;
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        unsigned raw = src->source_type;
        int found = 0;
        if (raw == 214u) continue;
        if (raw >= 176u && raw <= 178u) {
            for (int i = 0; i < MAX_MINERALS; ++i)
                if (!seen_minerals[i] && m[i].amount > 0 && m[i].style == (int)raw &&
                    m[i].x == src->x_px && m[i].y == src->y_px) {
                    seen_minerals[i] = 1; ++mineral_count; found = 1; break;
                }
        } else if (raw == 188u) {
            for (int i = 0; i < geyser_count; ++i)
                if (!seen_geysers[i] && geysers[i].x == src->x_px &&
                    geysers[i].y == src->y_px) {
                    seen_geysers[i] = 1; found = 1; break;
                }
        } else if (sc64_race_unit_from_raw((int)raw) >= 0) {
            int owner = src->owner == map->human_slot ? SIDE : map->owner_modes[src->owner];
            for (int i = 0; i < MAX_UNITS; ++i)
                if (!seen_units[i] && u[i].alive && u[i].raw_type == (int)raw &&
                    u[i].raw_owner == src->owner && u[i].owner == owner &&
                    u[i].x == src->x_px && u[i].y == src->y_px) {
                    seen_units[i] = 1; ++units; found = 1; break;
                }
        } else if (sc64_race_building_from_raw((int)raw) >= 0) {
            int owner = src->owner == map->human_slot ? SIDE : map->owner_modes[src->owner];
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
        if ((u[i].alive != 0) != (seen_units[i] != 0)) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if ((b[i].alive != 0) != (seen_buildings[i] != 0)) return 0;
    for (int i = 0; i < MAX_MINERALS; ++i)
        if ((m[i].amount > 0) != (seen_minerals[i] != 0)) return 0;
    if (geyser_count < 0 || geyser_count > MAX_RACE_GEYSERS) return 0;
    for (int i = 0; i < geyser_count; ++i) if (!seen_geysers[i]) return 0;
    return units + buildings + mineral_count + (unsigned)geyser_count <= map->unit_count;
}

static int race_z6_runtime_init(const Sc64RaceMap *map)
{
    memset(&race_z6, 0, sizeof(race_z6));
    memset(&race_z6_world, 0, sizeof(race_z6_world));
    race_z6_map = 0;
    sc64_z6_runtime_init(&race_z6.runtime);
    Sc64Z6RuntimeBinding binding;
    if (!race_z6_bind_world(map, &race_z6, &binding)) {
        race_z6.runtime.fault = SC64_Z6_RUNTIME_FAULT_MAP;
        return 0;
    }
    if (
        !race_z6_starting_world_matches_source(map)) {
        race_z6.runtime.fault = SC64_Z6_RUNTIME_FAULT_ACTOR;
        return 0;
    }
    race_z6_world = binding.world;
    race_z6_ops.context = &race_z6_world;
    race_z6_ops.condition = race_z6_runtime_condition;
    race_z6_ops.action = race_z6_runtime_action;
    race_z6_map = map;
    return sc64_z6_persistent_state_valid(&race_z6);
}

static int race_z6_rebind_loaded_world(const Sc64RaceMap *map)
{
    Sc64Z6RuntimeBinding binding;
    if (!sc64_z6_persistent_state_valid(&race_z6) ||
        !race_z6_bind_world(map, &race_z6, &binding)) return 0;
    race_z6_world = binding.world;
    race_z6_ops.context = &race_z6_world;
    race_z6_ops.condition = race_z6_runtime_condition;
    race_z6_ops.action = race_z6_runtime_action;
    race_z6_map = map;
    return 1;
}

static void race_z6_set_hooks(const Sc64Z6RuntimeHooks *hooks)
{
    if (hooks) race_z6_hooks = *hooks;
    else memset(&race_z6_hooks, 0, sizeof(race_z6_hooks));
    if (race_z6_map) (void)race_z6_rebind_loaded_world(race_z6_map);
}

/* Central combat kill hooks should call this before clearing a unit. Source
 * selector Deaths is event-based and is therefore persisted independently. */
static void race_z6_note_unit_death(const Unit *q)
{
    unsigned source;
    if (!race_z6_active() || !q) return;
    source = q->owner == SIDE && race_z6_map ? race_z6_map->human_slot :
             q->raw_owner >= 0 ? (unsigned)q->raw_owner : SC64_Z6_SOURCE_SLOT_COUNT;
    if (source < SC64_Z6_SOURCE_SLOT_COUNT &&
        race_z6.deaths_by_source[source] < SC64_Z6_MAX_SOURCE_DEATHS)
        ++race_z6.deaths_by_source[source];
}

static void race_z6_note_building_death(const Building *q)
{
    unsigned source;
    if (!race_z6_active() || !q) return;
    source = q->owner == SIDE && race_z6_map ? race_z6_map->human_slot :
             q->raw_owner >= 0 ? (unsigned)q->raw_owner : SC64_Z6_SOURCE_SLOT_COUNT;
    if (source < SC64_Z6_SOURCE_SLOT_COUNT &&
        race_z6.deaths_by_source[source] < SC64_Z6_MAX_SOURCE_DEATHS)
        ++race_z6.deaths_by_source[source];
}

static int race_z6_runtime_tick(u32 elapsed_frames, int user_paused)
{
    int result;
    int ai_started_before;
    if (!race_z6_active()) return 1;
    if (!race_z6_map || !sc64_z6_persistent_state_valid(&race_z6)) return 0;
    race_z6_world.elapsed_frames = elapsed_frames;
    race_z6_world.victory_requested = race_z6_world.defeat_requested = 0;
    race_z6_world.objective_refresh_requested = 0;
    ai_started_before = (race_z6.runtime.model.started & (1u << 8)) != 0;
    result = sc64_z6_adapter_tick(&race_z6.runtime, &race_z6_ops, user_paused);
    race_z6.moved_locations_mask = race_z6_world.moved_locations_mask;
    memcpy(race_z6.moved_location, race_z6_world.moved_location,
           sizeof(race_z6.moved_location));
    if (result < 0 || !sc64_z6_persistent_state_valid(&race_z6)) return 0;
    /* The initial plan runs inside the source action. Its first interval
       starts on the following active tick and survives save/load. */
    if (ai_started_before && !race_z6_authored_ai_advance(user_paused)) {
        race_z6.runtime.fault = SC64_Z6_RUNTIME_FAULT_HOOK;
        return 0;
    }
    if (!sc64_z6_persistent_state_valid(&race_z6)) return 0;
    return 1;
}

#endif /* SC64_Z6_RUNTIME */
#endif /* SC64_Z6_RUNTIME_ADAPTER_H */
