#ifndef SC64_T5_SCENARIO_ADAPTER_H
#define SC64_T5_SCENARIO_ADAPTER_H

#if defined(SC64_T5_RUNTIME) && defined(SC64_T4_RUNTIME) && \
    defined(SC64_UNIFIED) && defined(SC64_T5_ENABLED)

#include "sc64_t5_runtime.h"
#include "sc64_t5_ai.h"
#include "sc64_t4_queries.h"
#include "sc64_t5_scenario_generated.h"

/* The core runner is shared with T4, but all T5 source operands are resolved
 * against T5's own scenario/setup. The host callbacks below deliberately
 * separate that source policy from bounded actor-array operations. */
typedef struct {
    int (*count)(void *context, const Sc64T4Condition *condition,
                 unsigned trigger_owner, u32 *count);
    /* Unit creation, MoveUnit and Kill/Remove effects are implemented by the
     * engine binding. Return 1 applied, 0 atomic retry, -1 unsupported. */
    int (*world_action)(void *context, const Sc64T4Action *action,
                        unsigned trigger_owner);
    int (*set_resources)(void *context, unsigned source_player,
                         const Sc64ResourceActionState *balance);
    int (*center_view)(void *context, int x, int y);
    int (*ping)(void *context, int x, int y);
    Sc64T5AiOps ai;
} Sc64T5ScenarioOps;

typedef struct {
    Sc64T5RuntimeState *state;
    const Sc64T5ScenarioOps *ops;
    void *world_context;
    void *ai_context;
} Sc64T5ScenarioContext;

/* Raw source player slot to bounded runtime owner. Raw provenance remains on
 * actors; only recognized source slots participate in this mission. */
static int
sc64_t5_runtime_owner(unsigned source_slot)
{
    switch (source_slot) {
    case SC64_T5_SOURCE_HUMAN: return SIDE;
    case SC64_T5_SOURCE_RESCUE: return RESCUABLE;
    case SC64_T5_SOURCE_SPECIAL_NEUTRAL: return NEUTRAL;
    case SC64_T5_SOURCE_ENEMY: return ENEMY;
    default: return -1;
    }
}

/* Current owner takes precedence over source provenance after capture. */
static int
sc64_t5_current_source_owner(unsigned runtime_owner, int raw_owner)
{
    if (runtime_owner == SIDE) return SC64_T5_SOURCE_HUMAN;
    if (runtime_owner == ENEMY) return SC64_T5_SOURCE_ENEMY;
    if (runtime_owner == RESCUABLE && raw_owner == SC64_T5_SOURCE_RESCUE)
        return SC64_T5_SOURCE_RESCUE;
    if (runtime_owner == RESCUABLE &&
        raw_owner == SC64_T5_SOURCE_SPECIAL_NEUTRAL)
        return SC64_T5_SOURCE_SPECIAL_NEUTRAL;
    if (runtime_owner == NEUTRAL &&
        raw_owner == SC64_T5_SOURCE_SPECIAL_NEUTRAL)
        return SC64_T5_SOURCE_SPECIAL_NEUTRAL;
    return -1;
}

static int
sc64_t5_source_force(unsigned source_slot)
{
    if (source_slot >= 8) return 0;
    return (int)sc64_t5_source_setup.force_by_slot[source_slot] + 1;
}

static int
sc64_t5_slot_active(unsigned slot)
{
    return slot < 8 &&
        (SC64_T5_ACTIVE_SOURCE_MASK & (1u << slot)) != 0;
}

/* Resolve the verified trigger player selectors against current player
 * identity. Selector 12 (No Players) is a valid non-match; unsupported
 * selectors return -1 so a malformed program faults instead of broadening. */
static int
sc64_t5_selector_matches(unsigned selector, unsigned trigger_owner,
                         unsigned runtime_owner, int raw_owner)
{
    int source = sc64_t5_current_source_owner(runtime_owner, raw_owner);
    if (trigger_owner >= 8) return -1;
    if (selector <= 7) {
        if (!sc64_t5_slot_active(selector)) return 0;
        return source == (int)selector;
    }
    if (selector == 12) return 0;
    if (selector == 13) {
        if (!sc64_t5_slot_active(trigger_owner)) return 0;
        return source == (int)trigger_owner;
    }
    if (selector == 17) return source >= 0 && sc64_t5_slot_active((unsigned)source);
    if (selector >= 18 && selector <= 21) {
        if (source < 0 || !sc64_t5_slot_active((unsigned)source)) return 0;
        return sc64_t5_source_force((unsigned)source) == (int)(selector - 17);
    }
    return -1;
}

static int
sc64_t5_scenario_valid(const Sc64T4Scenario *scenario,
                       const Sc64T4SourceSetup *setup)
{
    if (!scenario || !setup || scenario->locations != sc64_t5_locations ||
        scenario->location_count != 64 || scenario->triggers != sc64_t5_triggers ||
        scenario->trigger_count != 16 || scenario->doodad_count != 14 ||
        !sc64_t4_trigger_program_valid(scenario)) return 0;
    if (setup->controllers[0] != 6 || setup->controllers[1] != 0 ||
        setup->controllers[2] != 3 || setup->controllers[3] != 7 ||
        setup->controllers[4] != 0 || setup->controllers[5] != 0 ||
        setup->controllers[6] != 5 || setup->controllers[7] != 0) return 0;
    if (setup->force_by_slot[0] != 0 || setup->force_by_slot[1] != 3 ||
        setup->force_by_slot[2] != 2 || setup->force_by_slot[3] != 2 ||
        setup->force_by_slot[4] != 3 || setup->force_by_slot[5] != 3 ||
        setup->force_by_slot[6] != 1 || setup->force_by_slot[7] != 3) return 0;
    return 1;
}

static int
sc64_t5_runtime_init(Sc64T5RuntimeState *state,
                     const Sc64T4Scenario *scenario,
                     const Sc64T4SourceSetup *setup)
{
    Sc64T5RuntimeState next;
    if (!state || !sc64_t5_scenario_valid(scenario, setup)) return 0;
    memset(&next, 0, sizeof(next));
    for (unsigned player = 0; player < 8; ++player) {
        unsigned force = (unsigned)sc64_t5_source_force(player);
        if (!sc64_t4_trigger_init(&next.player[player], scenario, player, force))
            return 0;
        if (setup->controllers[player]) next.active_mask |= (u8)(1u << player);
    }
    if (!sc64_t5_ai_init(&next.ai)) return 0;
    if (next.active_mask != SC64_T5_ACTIVE_SOURCE_MASK) return 0;
    *state = next;
    return 1;
}

static int
sc64_t5_location_source(const Sc64T4Scenario *scenario, unsigned id,
                        int map_width, int map_height, int *x, int *y)
{
    const Sc64T4Location *location;
    u32 center_x, center_y;
    if (!scenario || !x || !y || id == 0 || id > scenario->location_count)
        return 0;
    location = &scenario->locations[id - 1];
    if (location->elevation || location->left >= location->right ||
        location->top >= location->bottom ||
        location->right > (u32)map_width * 32u ||
        location->bottom > (u32)map_height * 32u) return 0;
    center_x = location->left + (location->right - location->left) / 2;
    center_y = location->top + (location->bottom - location->top) / 2;
    *x = (int)(center_x / 2u);
    *y = (int)(center_y / 2u);
    return 1;
}

/* BWAPI UnitType.cpp unitDimensions, pinned reference table at line 961.
 * Extents are source pixels (left, up, right, down); right/bottom are made
 * exclusive here. This covers every T5 initial actor and source-created actor
 * used by its Bring/Command conditions, plus each retained building type. */
static int
sc64_t5_raw_extents(unsigned raw, int building,
                    int *left, int *up, int *right, int *down)
{
    int l, u, r, d;
    if (building) {
        switch (raw) {
        case 106: l=58;u=41;r=58;d=41;break;
        case 109: l=38;u=22;r=38;d=26;break;
        case 110: l=56;u=32;r=56;d=31;break;
        case 111: l=48;u=40;r=56;d=32;break;
        case 112: l=40;u=32;r=44;d=24;break;
        case 113: l=56;u=40;r=56;d=40;break;
        case 114: l=48;u=40;r=48;d=38;break;
        case 115: l=47;u=24;r=28;d=22;break;
        case 124: l=16;u=32;r=16;d=16;break;
        case 125: l=32;u=24;r=32;d=16;break;
        case 195: l=48;u=32;r=47;d=31;break;
        default: return 0;
        }
    } else {
        switch (raw) {
        case 0: l=8;u=9;r=8;d=10;break;
        case 2: case 3: case 19: l=16;u=16;r=15;d=15;break;
        case 7: l=11;u=11;r=11;d=11;break;
        case 8: l=19;u=15;r=18;d=14;break;
        case 11: l=24;u=16;r=24;d=20;break;
        case 13: l=7;u=7;r=7;d=7;break;
        case 16: l=7;u=10;r=7;d=11;break;
        case 32: l=11;u=7;r=11;d=14;break;
        case 101: l=13;u=13;r=13;d=17;break;
        default: return 0;
        }
    }
    if (left) *left=l;
    if (up) *up=u;
    if (right) *right=r;
    if (down) *down=d;
    return 1;
}

static int
sc64_t5_actor_rect(unsigned raw, int building, int x, int y,
                   Sc64T4QueryRect *rect)
{
    int left, up, right, down;
    s32 center_x, center_y;
    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H) return 0;
    if (!rect || !sc64_t5_raw_extents(raw, building, &left, &up, &right, &down))
        return 0;
    center_x = (s32)x * 2; center_y = (s32)y * 2;
    rect->left = center_x - left; rect->top = center_y - up;
    rect->right = center_x + right + 1; rect->bottom = center_y + down + 1;
    return 1;
}

static int
sc64_t5_type_matches(unsigned wanted, int raw, int building)
{
    if (wanted == 229) return 1; /* Any Unit includes buildings. */
    if (wanted == 231) return building;
    if (wanted == 230)
        return !building && (raw == 0 || raw == 2 || raw == 3 || raw == 7 ||
               raw == 8 || raw == 11 || raw == 13 || raw == 16 || raw == 19 ||
               raw == 32);
    return raw == (int)wanted;
}

static int
sc64_t5_source_is_flying(unsigned raw)
{
    /* BWAPI UnitType::isFlyer facts for the retained T5 actors. */
    return raw == 8 || raw == 11;
}

static int
sc64_t5_query_match(const Sc64T4Condition *condition, unsigned trigger_owner,
                    unsigned live_owner, int raw_owner, unsigned raw_type,
                    int building)
{
    int selected;
    if (!condition) return -1;
    selected = sc64_t5_selector_matches(condition->player, trigger_owner,
                                        live_owner, raw_owner);
    if (selected < 0) return -1;
    return selected && sc64_t5_type_matches(condition->unit, (int)raw_type,
                                             building);
}

/* Return -1 for an actor whose pinned bounds are unavailable, otherwise a
 * boolean overlap. Cargo uses its carrier's position and elevation here. */
static int
sc64_t5_actor_overlaps(unsigned raw_type, int building, int ground_only,
                       const Sc64T4QueryRect *area, int x, int y)
{
    Sc64T4QueryRect actor;
    if (!area) return -1;
    if (!building && ground_only && sc64_t5_source_is_flying(raw_type)) return 0;
    if (area->left == 0 && area->top == 0 &&
        area->right == 96 * 32 && area->bottom == 96 * 32) return 1;
    if (!sc64_t5_actor_rect(raw_type, building, x, y, &actor)) return -1;
    return sc64_t4_query_rect_overlap(area, &actor);
}

static int
sc64_t5_query_actor(const Sc64T4Condition *condition, unsigned trigger_owner,
                    unsigned live_owner, int raw_owner, unsigned raw_type,
                    int building, int ground_only,
                    const Sc64T4QueryRect *area, int x, int y,
                    u32 *count)
{
    int match = sc64_t5_query_match(condition, trigger_owner, live_owner,
                                    raw_owner, raw_type, building);
    if (match < 0 || !count) return 0;
    if (!match) return 1;
    int overlap = sc64_t5_actor_overlaps(raw_type, building, ground_only,
                                         area, x, y);
    if (overlap < 0) return 0;
    if (overlap) ++*count;
    return 1;
}

static int
sc64_t5_world_count(void *context, const Sc64T4Condition *condition,
                    unsigned trigger_owner, u32 *count)
{
    Sc64T5RuntimeState *state = context;
    Sc64T4QueryRect area;
    u32 result = 0;
    int ground_only = 0;
    if (!state || !condition || !count || trigger_owner >= 8 ||
        (condition->opcode != 2 && condition->opcode != 3) ||
        condition->mask ||
        (condition->flags & ~0x12u)) return 0;
    unsigned selector = condition->player;
    if (!(selector <= 7 || selector == 12 || selector == 13 || selector == 17 ||
          (selector >= 18 && selector <= 21))) return 0;
    if (condition->opcode == 2) {
        area.left = 0; area.top = 0; area.right = 96 * 32; area.bottom = 96 * 32;
    } else {
        const Sc64T4Location *loc;
        if (!condition->location ||
            condition->location > sc64_t5_scenario.location_count) return 0;
        loc = &sc64_t5_scenario.locations[condition->location - 1];
        /* Location 18 carries source elevation flags 7. OpenBW's location
         * query interprets bits 0..2 as flying-height flags, excluding
         * airborne actors from this authored Bring condition. */
        if ((loc->elevation != 0 && loc->elevation != 7) ||
            loc->left >= loc->right || loc->top >= loc->bottom ||
            loc->right > 96u * 32u || loc->bottom > 96u * 32u) return 0;
        ground_only = loc->elevation == 7;
        area.left = (s32)loc->left; area.top = (s32)loc->top;
        area.right = (s32)loc->right; area.bottom = (s32)loc->bottom;
    }
    if (condition->unit != 229 && condition->unit != 230 &&
        condition->unit != 231 &&
        sc64_race_unit_from_raw(condition->unit) < 0 &&
        sc64_race_building_from_raw(condition->unit) < 0) return 0;
    if (condition->opcode == 2) {
        /* Command is a global count; loaded passengers remain live actors and
         * are included by the original command-count path. */
        int completed = condition->comparison != 1;
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
            int raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
            int match = sc64_t5_query_match(condition, trigger_owner,
                    (unsigned)u[i].owner, u[i].raw_owner, (unsigned)raw, 0);
            if (match < 0) return 0;
            if (match) ++result;
        }
        for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
            if (completed && b[i].under_construction) continue;
            int raw = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
            int match = sc64_t5_query_match(condition, trigger_owner,
                    (unsigned)b[i].owner, b[i].raw_owner, (unsigned)raw, 1);
            if (match < 0) return 0;
            if (match) ++result;
        }
    } else {
        int completed = condition->comparison != 1;
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
            int raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
            int match;
            if (SC64_TRANSPORT_LOADED(&u[i])) {
                int carrier = u[i].target, overlap;
                if (!completed) continue;
                if (carrier < 0 || carrier >= MAX_UNITS || carrier == i ||
                    !u[carrier].alive || u[carrier].raw_type != 11 ||
                    u[carrier].type != U_DROPSHIP ||
                    u[carrier].owner != u[i].owner) return 0;
                match = sc64_t5_query_match(condition, trigger_owner,
                        (unsigned)u[i].owner, u[i].raw_owner, (unsigned)raw, 0);
                if (match < 0) return 0;
                if (!match) continue;
                overlap = sc64_t5_actor_overlaps(11, 0, ground_only, &area,
                                                  u[carrier].x, u[carrier].y);
                if (overlap < 0) return 0;
                if (overlap) ++result;
                continue;
            }
            if (!sc64_t5_query_actor(condition, trigger_owner,
                    (unsigned)u[i].owner, u[i].raw_owner, (unsigned)raw, 0,
                    ground_only, &area, u[i].x, u[i].y, &result)) return 0;
        }
        for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
            if (completed && b[i].under_construction) continue;
            int raw = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
            if (!sc64_t5_query_actor(condition, trigger_owner,
                    (unsigned)b[i].owner, b[i].raw_owner, (unsigned)raw, 1,
                    0, &area, b[i].x, b[i].y, &result)) return 0;
        }
    }
    *count = result;
    return 1;
}

static int
sc64_t5_resource_action_apply(Sc64T5RuntimeState *state,
                              const Sc64T4Action *action)
{
    Sc64ResourceActionState candidate;
    unsigned target;
    if (!state || !action || action->opcode != 26 || action->location ||
        action->unit != SC64_RESOURCE_BOTH || action->modifier != SC64_RESOURCE_MOD_SET)
        return -1;
    if (action->player == 0) target = SC64_T5_SOURCE_HUMAN;
    else if (action->player == 19) target = SC64_T5_SOURCE_ENEMY;
    else return -1;
    unsigned expected = target == SC64_T5_SOURCE_HUMAN ? 300u : 6000u;
    if (action->argument != expected ||
        (target == SC64_T5_SOURCE_ENEMY && sc64_t5_source_force(target) != 2)) return -1;
    candidate = state->resources[target];
    if (target == SC64_T5_SOURCE_HUMAN) {
        if (minerals < 0 || gas < 0 || minerals > 65535 || gas > 65535) return -1;
        candidate.current_minerals = (u32)minerals;
        candidate.current_gas = (u32)gas;
    } else {
        if (race_minerals[ENEMY] < 0 || race_gas[ENEMY] < 0 ||
            race_minerals[ENEMY] > 65535 || race_gas[ENEMY] > 65535) return -1;
        candidate.current_minerals = (u32)race_minerals[ENEMY];
        candidate.current_gas = (u32)race_gas[ENEMY];
    }
    if (!sc64_resource_action_apply_one_player(&candidate, action->unit,
                                                action->modifier,
                                                action->argument)) return -1;
    if (target == SC64_T5_SOURCE_HUMAN) {
        minerals = (int)candidate.current_minerals;
        gas = (int)candidate.current_gas;
    } else {
        race_minerals[ENEMY] = (int)candidate.current_minerals;
        race_gas[ENEMY] = (int)candidate.current_gas;
    }
    state->resources[target] = candidate;
    return 1;
}

static int
race_t5_trigger_count(void *context, const Sc64T4Condition *condition,
                      unsigned owner, u32 *count)
{
    Sc64T5ScenarioContext *ctx = context;
    if (!ctx || !ctx->state || !ctx->ops || !ctx->ops->count ||
        !condition || !count || owner >= 8 ||
        (condition->opcode != 2 && condition->opcode != 3) ||
        (condition->flags & ~0x12u) ||
        (condition->mask != 0 && condition->mask != 7)) return 0;
    /* Validate the selector before scanning world arrays, including when
     * those arrays happen to be empty. The world callback uses this helper
     * for every candidate actor. */
    if (condition->player != 12 && condition->player != 13 &&
        condition->player != 17 &&
        (condition->player < 18 || condition->player > 21) &&
        condition->player > 7) return 0;
    return ctx->ops->count(ctx->world_context, condition, owner, count);
}

static int
race_t5_resource_action(Sc64T5ScenarioContext *ctx,
                        const Sc64T4Action *action)
{
    if (!ctx || !ctx->state) return -1;
    return sc64_t5_resource_action_apply(ctx->state, action);
}

static int
race_t5_create_action(Sc64T5RuntimeState *state, const Sc64T4Action *action,
                      unsigned trigger_owner)
{
    unsigned target_slot, count;
    int x, y, runtime_owner, type, slot;
    if (!state || !action || trigger_owner >= 8 || action->opcode != 11 ||
        action->argument || action->flags != 0x10 ||
        !sc64_t4_creation_count(action->modifier, &count) || count != 1)
        return -1;
    if (action->player == 13) target_slot = trigger_owner;
    else if (action->player <= 7) target_slot = action->player;
    else return -1;
    if (!sc64_t5_slot_active(target_slot)) return -1;
    if ((action->unit == 101 && action->player == 13 &&
         ((action->location < 5 || action->location > 16) && action->location != 1)) ||
        ((action->unit == 0 || action->unit == 7) &&
         (action->location != 1 || action->player != 3)) ||
        (action->unit != 101 && action->unit != 0 && action->unit != 7)) return -1;
    if (!sc64_t5_location_source(&sc64_t5_scenario, action->location,
                                 96, 96, &x, &y)) return -1;
    runtime_owner = sc64_t5_runtime_owner(target_slot);
    /* The source Rscu script activates slot 3's rescuable state. This is an
     * authored compatibility policy matching the world adapter's conversion;
     * creations before that script remain neutral and cannot be rescued. */
    if (target_slot == SC64_T5_SOURCE_SPECIAL_NEUTRAL &&
        (((state->ai.requested_scripts[2] | state->ai.requested_scripts[3]) &
          SC64_T5_SCRIPT_BIT_RSCU) != 0))
        runtime_owner = RESCUABLE;
    type = sc64_race_unit_from_raw(action->unit);
    if (runtime_owner < 0 || type < 0 || x < 0 || y < 0 ||
        x >= WORLD_W || y >= WORLD_H) return -1;
    slot = find_unit_slot();
    if (slot < 0) return 0;
    slot = spawn_unit(runtime_owner, type, x, y);
    if (slot < 0) return 0;
    u[slot].raw_type = action->unit;
    u[slot].raw_owner = (int)target_slot;
    return 1;
}

static int
race_t5_move_action(const Sc64T4Action *action, unsigned trigger_owner)
{
    u8 slots[MAX_UNITS];
    u16 xs[MAX_UNITS], ys[MAX_UNITS];
    unsigned count = 0;
    Sc64T4QueryRect source, destination;
    int tx, ty;
    if (!action || trigger_owner >= 8 || action->opcode != 39 ||
        action->player != 17 || action->unit != 16 || action->modifier != 0 ||
        action->flags != 0x10 ||
        !action->location || action->location > sc64_t5_scenario.location_count ||
        !action->argument || action->argument > sc64_t5_scenario.location_count)
        return -1;
    const Sc64T4Location *from = &sc64_t5_scenario.locations[action->location - 1];
    const Sc64T4Location *to = &sc64_t5_scenario.locations[action->argument - 1];
    if (from->elevation || to->elevation || from->left >= from->right ||
        from->top >= from->bottom || to->left >= to->right || to->top >= to->bottom ||
        from->right > 96u * 32u || from->bottom > 96u * 32u ||
        to->right > 96u * 32u || to->bottom > 96u * 32u) return -1;
    source.left = (s32)from->left; source.top = (s32)from->top;
    source.right = (s32)from->right; source.bottom = (s32)from->bottom;
    destination.left = (s32)to->left; destination.top = (s32)to->top;
    destination.right = (s32)to->right; destination.bottom = (s32)to->bottom;
    tx = ((destination.left + destination.right) / 2) / 2;
    ty = ((destination.top + destination.bottom) / 2) / 2;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        int selected = sc64_t5_selector_matches(action->player, trigger_owner,
                                                 (unsigned)u[i].owner,
                                                 u[i].raw_owner);
        Sc64T4QueryRect actor;
        if (selected < 0) return -1;
        if (!selected || raw != action->unit) continue;
        if (!sc64_t5_actor_rect((unsigned)raw, 0, u[i].x, u[i].y, &actor)) return -1;
        if (!sc64_t4_query_rect_overlap(&source, &actor)) continue;
        int found = 0;
        for (int radius = 0; radius <= 8 && !found; ++radius) {
            for (int dy = -radius; dy <= radius && !found; ++dy) {
                int dx = radius - FAST_ABS(dy);
                for (int sign = 0; sign < (dx ? 2 : 1) && !found; ++sign) {
                    int x = tx + (sign ? -dx : dx) * 16;
                    int y = ty + dy * 16;
                    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H ||
                        x * 2 < destination.left || x * 2 >= destination.right ||
                        y * 2 < destination.top || y * 2 >= destination.bottom ||
                        !is_traversable(x / 16, y / 16, -1) ||
                        !unit_step_clear(&u[i], x, y)) continue;
                    int apart = 1;
                    for (unsigned j = 0; j < count; ++j)
                        if (dist2(x, y, xs[j], ys[j]) < 36) { apart = 0; break; }
                    if (!apart) continue;
                    slots[count] = (u8)i; xs[count] = (u16)x; ys[count] = (u16)y;
                    ++count; found = 1;
                }
            }
        }
        if (!found) return 0;
    }
    /* OpenBW source MoveUnit relocates matching units; this deterministic
     * bounded-fit policy is the GBA implementation. Plan every fit first so a
     * blocked destination never moves only part of the group. */
    for (unsigned i = 0; i < count; ++i) {
        Unit *q = &u[slots[i]];
        q->x = xs[i]; q->y = ys[i];
        q->order = ORDER_IDLE; q->target = -1;
        q->tx = xs[i]; q->ty = ys[i];
        q->path_x = xs[i]; q->path_y = ys[i];
        q->path_timer = q->path_retry = 0;
        sc64_unit_bucket_move(q); sc64_target_index_move(q);
    }
    return 1;
}

static int
race_t5_remove_action(const Sc64T4Action *action, unsigned trigger_owner)
{
    int exact_unit;
    if (!action || trigger_owner >= 8 || action->location ||
        action->modifier || action->flags != 0x10) return -1;
    if (action->opcode == 22 && action->unit == 101 && action->player == 13)
        exact_unit = 101;
    else if (action->opcode == 24 && action->unit == 195 && action->player == 0)
        exact_unit = 195;
    else if (action->opcode == 24 && action->unit == 11 && action->player == 0)
        exact_unit = 11;
    else return -1;
    u8 remove_units[MAX_UNITS], remove_buildings[MAX_BUILDINGS];
    memset(remove_units, 0, sizeof(remove_units));
    memset(remove_buildings, 0, sizeof(remove_buildings));
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        int selected = sc64_t5_selector_matches(action->player, trigger_owner,
                      (unsigned)u[i].owner, u[i].raw_owner);
        if (selected < 0) return -1;
        if (selected && raw == exact_unit) remove_units[i] = 1;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int raw = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        int selected = sc64_t5_selector_matches(action->player, trigger_owner,
                      (unsigned)b[i].owner, b[i].raw_owner);
        if (selected < 0) return -1;
        if (selected && raw == exact_unit) remove_buildings[i] = 1;
    }
    int changed = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (remove_units[i]) {
        /* Source RemoveUnit may delete a Dropship directly. Its passengers
         * must be removed in this same action, before a later trigger query
         * can observe cargo whose carrier no longer exists. */
        if (u[i].raw_type == 11 && u[i].type == U_DROPSHIP)
            race_transport_destroy_cargo(i);
        u[i].alive = 0; u[i].selected = 0; changed = 1;
        for (int j = 0; j < MAX_UNITS; ++j)
            if (u[j].alive && u[j].order == ORDER_ATTACK_TARGET && u[j].target == i) {
                u[j].order = ORDER_IDLE; u[j].target = -1;
            }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (remove_buildings[i]) {
        b[i].alive = 0; changed = 1;
        if (selected_building == i) selected_building = -1;
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive) {
            if (u[j].order == ORDER_ATTACK_TARGET &&
                u[j].target == MAX_UNITS + i) {
                u[j].order = ORDER_IDLE; u[j].target = -1;
            } else if ((u[j].order == ORDER_BUILD || u[j].order == ORDER_GAS) &&
                       u[j].target == i) {
                u[j].order = ORDER_IDLE; u[j].target = -1;
            }
        }
    }
    if (changed) {
        supply_used = race_owner_supply(SIDE);
        supply_cap = race_owner_supply_cap(SIDE);
        invalidate_path_grid(); update_exploration();
    }
    return 1;
}

static int
race_t5_world_action(void *context, const Sc64T4Action *action,
                     unsigned trigger_owner)
{
    Sc64T5RuntimeState *state = context;
    if (!state || !action) return -1;
    if (action->opcode == 11)
        return race_t5_create_action(state, action, trigger_owner);
    if (action->opcode == 22 || action->opcode == 24)
        return race_t5_remove_action(action, trigger_owner);
    if (action->opcode == 39) return race_t5_move_action(action, trigger_owner);
    return -1;
}

static int
race_t5_world_center_view(void *context, int x, int y)
{
    (void)context;
    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H) return -1;
    cx = x; cy = y;
    return 1;
}

static int
race_t5_world_ping(void *context, int x, int y)
{
    (void)context; (void)x; (void)y;
    return 1;
}

static int
race_t5_trigger_action(void *context, const Sc64T4Action *action,
                       unsigned owner)
{
    Sc64T5ScenarioContext *ctx = context;
    int x, y;
    if (!ctx || !ctx->state || !ctx->ops || !action || owner >= 8) return -1;
    /* Disabled source actions are no-ops before inspecting unused operands. */
    if (action->flags & 2) return 1;
    switch (action->opcode) {
    case 7: case 8: case 9: case 12:
        if (owner != SC64_T5_SOURCE_HUMAN) return 1;
        if (action->time) {
            if (!sc64_t4_trigger_wait_duration(action->time,
                                                &ctx->state->message_ticks)) return -1;
        } else ctx->state->message_ticks = 90;
        ctx->state->message = action->opcode;
        return 1;
    case 10:
        if (owner != SC64_T5_SOURCE_HUMAN || !ctx->ops->center_view ||
            !sc64_t5_location_source(&sc64_t5_scenario, action->location,
                                     96, 96, &x, &y)) return -1;
        return ctx->ops->center_view(ctx->world_context, x, y);
    case 11: case 22: case 24: case 39:
        if (!ctx->ops->world_action) return -1;
        return ctx->ops->world_action(ctx->world_context, action, owner);
    case 13:
        /* SetSwitch (13) is handled by the shared generic trigger core. */
        return -1;
    case 15: case 16: {
        const Sc64T4Location *area = 0;
        if (action->opcode == 16) {
            if (!action->location || action->location > sc64_t5_scenario.location_count)
                return -1;
            area = &sc64_t5_scenario.locations[action->location - 1];
            if (area->elevation || area->left >= area->right ||
                area->top >= area->bottom || area->right > 96u * 32u ||
                area->bottom > 96u * 32u) return -1;
        }
        return sc64_t5_ai_action(&ctx->state->ai, action, owner, area,
                                 &ctx->ops->ai, ctx->ai_context);
    }
    case 26:
        return race_t5_resource_action(ctx, action);
    case 28:
        if (owner != SC64_T5_SOURCE_HUMAN || !ctx->ops->ping ||
            !sc64_t5_location_source(&sc64_t5_scenario, action->location,
                                     96, 96, &x, &y)) return -1;
        if (ctx->ops->ping(ctx->world_context, x, y) != 1) return 0;
        ctx->state->ping_x = (u16)x; ctx->state->ping_y = (u16)y;
        ctx->state->ping_ticks = 90;
        return 1;
    default:
        return -1;
    }
}

static int
sc64_t5_runtime_step(Sc64T5RuntimeState *state,
                     const Sc64T4Scenario *scenario,
                     const Sc64T5ScenarioOps *ops,
                     void *world_context, void *ai_context,
                     int paused)
{
    Sc64T5ScenarioContext context;
    Sc64T4TriggerOps trigger_ops;
    /* Scenario rows are immutable generated ROM data, and every player
     * context was fully checked by sc64_t5_runtime_init or the save loader.
     * Keep the per-frame guard constant-time rather than rescanning all
     * 1,280 source action/condition slots. */
    if (!state || !ops || scenario != &sc64_t5_scenario ||
        state->active_mask != SC64_T5_ACTIVE_SOURCE_MASK ||
        !sc64_t5_ai_state_valid(&state->ai)) return 0;
    context.state = state; context.ops = ops;
    context.world_context = world_context; context.ai_context = ai_context;
    trigger_ops.count = race_t5_trigger_count;
    trigger_ops.action = race_t5_trigger_action;
    if (paused) return 1;
    if (state->message_ticks && !--state->message_ticks) state->message = 0;
    if (state->ping_ticks && !--state->ping_ticks)
        state->ping_x = state->ping_y = 0;
    for (unsigned p = 0; p < 8; ++p)
        if ((state->active_mask & (1u << p)) &&
            !sc64_t4_trigger_step_prevalidated(&state->player[p], scenario,
                         &state->switches, &trigger_ops, &context, 0)) return 0;
    if (!sc64_t5_ai_tick(&state->ai, &ops->ai, ai_context)) return 0;
    race_campaign.message = state->message;
    race_campaign.message_ticks = state->message_ticks;
    if (state->player[0].outcome == SC64_T4_TR_LOST) {
        race_campaign.phase = SC64_RACE_PHASE_FAILED;
        lost = 1;
    } else if (state->player[0].outcome == SC64_T4_TR_WON) {
        race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
        won = 1;
        race_completed |= 1 << SC64_T5_GLOBAL_ID;
    }
    return 1;
}

/* game.c owns the mission-selection gate and the runtime object. These thin
 * bindings keep this adapter opt-in and use the same bounded callbacks as the
 * host fixture. */
static const Sc64T5ScenarioOps race_t5_scenario_ops = {
    sc64_t5_world_count,
    race_t5_world_action,
    0,
    race_t5_world_center_view,
    race_t5_world_ping,
    { race_t5_ai_script_callback, race_t5_ai_backend_tick }
};

static int
race_t5_init(void)
{
    return sc64_t5_runtime_init(&race_t5, &sc64_t5_scenario,
                                &sc64_t5_source_setup);
}

static int
race_t5_tick(void)
{
    if (!sc64_t5_runtime_step(&race_t5, &sc64_t5_scenario,
                              &race_t5_scenario_ops, &race_t5, 0,
                              screen != SCREEN_PLAY)) return 0;
    race_campaign.objective_stage = !!(race_t5.switches.bits[0] & 16);
    return 1;
}

#endif /* T5 combined runtime */
#endif
