#ifndef SC64_T6_WORLD_ADAPTER_H
#define SC64_T6_WORLD_ADAPTER_H

/* The T6 world binding is deliberately opt-in.  It applies the verified
 * trigger program to the bounded GBA actor arrays; opaque AI scripts, source
 * strings, and exact native scheduler timing remain authored GBA policies. */
#if defined(SC64_T6_RUNTIME) && defined(SC64_T4_RUNTIME) && \
    defined(SC64_UNIFIED) && defined(SC64_T6_ENABLED)

#include "sc64_t6_mission_state.h"
#include "sc64_t6_scenario_adapter.h"
#include "sc64_t6_actor_rules.h"
#include "sc64_t4_queries.h"
#include "sc64_t4_actions.h"

enum {
    SC64_T6_SOURCE_HUMAN = 0,
    SC64_T6_SOURCE_ENEMY1 = 1,
    SC64_T6_SOURCE_RESCUE2 = 2,
    SC64_T6_SOURCE_ENEMY4 = 4,
    SC64_T6_SOURCE_RESCUE6 = 6,
    SC64_T6_SOURCE_ACTIVE_MASK = 0x57,
    SC64_T6_RESCUE_CONTACT_RADIUS = 32, /* bounded GBA policy; source scale 64 */
    SC64_T6_AI_TICK_PERIOD = 60
};

/* IDs are the little-endian CHK operands, not ASCII-order integers. The
 * referenced AI bodies are opaque; this adapter records the request and uses
 * the flags only to authorize bounded fallback behavior below. */
enum {
    SC64_T6_SCRIPT_RSCU = 0x75637352u, /* Rscu */
    SC64_T6_SCRIPT_TER6 = 0x36726554u, /* Ter6 */
    SC64_T6_SCRIPT_TE6B = 0x62366554u  /* Te6b */
};

static int race_t6_active(void);

/* Source player slot to current world owner. Source provenance remains on
 * every actor; an ownership transfer changes only the runtime owner. */
static int
sc64_t6_runtime_owner(unsigned source_slot)
{
    switch (source_slot) {
    case 0: return SIDE;
    case 1: return ENEMY;
    case 2: case 6: return RESCUABLE;
    case 4: return 4;
    case 3: case 5: case 7: return NEUTRAL;
    default: return -1;
    }
}

/* Current controller identity takes precedence after capture. For neutral
 * actors, their original source slot is still useful to trigger selectors. */
static int
sc64_t6_current_source_slot(int owner, int raw_owner)
{
    if (owner == SIDE) return SC64_T6_SOURCE_HUMAN;
    if (owner == ENEMY && raw_owner == SC64_T6_SOURCE_ENEMY1) return raw_owner;
    if (owner == 4 && raw_owner == SC64_T6_SOURCE_ENEMY4) return raw_owner;
    if (owner == RESCUABLE &&
        (raw_owner == SC64_T6_SOURCE_RESCUE2 ||
         raw_owner == SC64_T6_SOURCE_RESCUE6)) return raw_owner;
    if (owner == NEUTRAL &&
        (raw_owner == 3 || raw_owner == 5 || raw_owner == 7)) return raw_owner;
    return -1;
}

/* T6's two hostile computer owners are allied to each other. Rescue owners
 * are on the human side; neutral actors do not become hostile by default. */
static int
sc64_t6_owners_hostile(int a, int other)
{
    int a_side, b_side;
    if (a == other || a == NEUTRAL || other == NEUTRAL) return 0;
    a_side = (a == SIDE || a == RESCUABLE);
    b_side = (other == SIDE || other == RESCUABLE);
    if (a_side && b_side) return 0;
    if ((a == ENEMY || a == 4) && (other == ENEMY || other == 4)) return 0;
    return a_side != b_side;
}

static int
race_t6_unit_burrowed(int slot)
{
    return race_t6_active() && slot >= 0 && slot < MAX_UNITS && u[slot].alive &&
           sc64_t6_burrowed_get(&race_t6, (unsigned)slot);
}

/* Called after a pool slot has been reset for a new actor. */
static void
race_t6_unit_created(int slot)
{
    if (slot < 0 || slot >= SC64_T6_UNIT_LIMIT) return;
    sc64_t6_burrowed_set(&race_t6, (unsigned)slot, 0);
    if (slot < MAX_UNITS) u[slot].flags &= ~2u;
}

static void
race_t6_set_unit_burrowed(int slot, int burrowed)
{
    if (slot < 0 || slot >= MAX_UNITS ||
        (u[slot].raw_type != 37 && u[slot].raw_type != 38)) return;
    sc64_t6_burrowed_set(&race_t6, (unsigned)slot, burrowed != 0);
    if (burrowed) u[slot].flags |= 2u;
    else u[slot].flags &= ~2u;
}

/* The retained Deaths conditions query Buildings (unit selector 231) for
 * source slots 1 and 4. `raw_owner` preserves source provenance after capture. */
static void
race_t6_building_died(int owner, int raw_owner)
{
    int source = sc64_t6_current_source_slot(owner, raw_owner);
    if ((source == 1 || source == 4) && race_t6.deaths[source] != 0xffffffffu)
        ++race_t6.deaths[source];
}

static int
sc64_t6_source_active(unsigned slot)
{
    return slot < 8 && (SC64_T6_SOURCE_ACTIVE_MASK & (1u << slot)) != 0;
}

static int
sc64_t6_source_force(unsigned slot)
{
    if (slot >= 8) return -1;
    return (int)sc64_t6_source_setup.force_by_slot[slot] + 1;
}

static int
sc64_t6_selector_matches(unsigned selector, unsigned trigger_owner,
                          int live_owner, int raw_owner)
{
    int source = sc64_t6_current_source_slot(live_owner, raw_owner);
    int trigger_live;
    if (trigger_owner >= 8) return -1;
    if (selector <= 7) return source == (int)selector;
    if (selector == 12) return 0; /* No Players. */
    if (selector == 13) return source == (int)trigger_owner;
    if (selector == 14) {
        trigger_live = sc64_t6_runtime_owner(trigger_owner);
        if (trigger_live < 0 || source < 0) return 0;
        return sc64_t6_owners_hostile(live_owner, trigger_live);
    }
    if (selector == 17) return source >= 0;
    if (selector >= 18 && selector <= 21)
        return source >= 0 && sc64_t6_source_force((unsigned)source) ==
                                   (int)(selector - 17);
    return -1;
}

/* Pinned BWAPI UnitType.cpp unitDimensions for every actor type retained in
 * this T6 map. Coordinates here are source pixels; right/bottom are exclusive.
 * Keeping a fixed small table avoids treating a large building as a point in
 * Bring/Command-style area queries. */
typedef struct {
    u8 raw, building, left, up, right, down;
} Sc64T6ActorBound;

static int
sc64_t6_actor_rect(unsigned raw, int building, int x, int y,
                   Sc64T4QueryRect *rect)
{
    static const Sc64T6ActorBound bounds[] = {
        {0,0,8,9,8,10}, {1,0,7,10,7,11}, {2,0,16,16,15,15},
        {3,0,16,16,15,15}, {7,0,11,11,11,11}, {8,0,19,15,18,14},
        {11,0,24,16,24,20}, {19,0,16,16,15,15},
        {32,0,11,7,11,14}, {34,0,8,9,8,10}, {37,0,8,4,7,11},
        {38,0,10,10,10,12}, {41,0,11,11,11,11}, {42,0,25,25,24,24},
        {43,0,22,22,21,21}, {45,0,24,24,23,23}, {47,0,12,12,11,11},
        {106,1,58,41,58,41}, {109,1,38,22,38,26}, {110,1,56,32,56,31},
        {111,1,48,40,56,32}, {112,1,40,32,44,24}, {113,1,56,40,56,40},
        {114,1,48,40,48,38}, {115,1,47,24,28,22}, {116,1,48,38,48,38},
        {122,1,48,32,48,28}, {124,1,16,32,16,16}, {125,1,32,24,32,16},
        {126,1,48,32,47,31}, {132,1,49,32,49,32}, {133,1,49,32,49,32},
        {135,1,40,32,40,24}, {138,1,38,28,32,28}, {139,1,44,32,32,20},
        {141,1,28,32,28,24}, {142,1,36,28,40,18}, {144,1,24,24,23,23},
        {146,1,24,24,23,23}, {149,1,64,32,63,31}, {195,1,48,32,47,31}
    };
    if (!rect || x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H) return 0;
    for (unsigned i = 0; i < sizeof(bounds) / sizeof(bounds[0]); ++i) {
        const Sc64T6ActorBound *p = &bounds[i];
        if (p->raw == raw && p->building == (unsigned)building) {
            int sx = x * 2, sy = y * 2;
            rect->left = sx - p->left; rect->top = sy - p->up;
            rect->right = sx + p->right + 1;
            rect->bottom = sy + p->down + 1;
            return 1;
        }
    }
    return 0;
}

static int
sc64_t6_area(unsigned id, Sc64T4QueryRect *area)
{
    const Sc64T4Location *p;
    if (!area || !id || id > sc64_t6_scenario.location_count) return 0;
    p = &sc64_t6_scenario.locations[id - 1];
    if (p->elevation || p->left >= p->right || p->top >= p->bottom ||
        p->right > 96u * 32u || p->bottom > 96u * 32u) return 0;
    area->left = (s32)p->left; area->top = (s32)p->top;
    area->right = (s32)p->right; area->bottom = (s32)p->bottom;
    return 1;
}

static int
sc64_t6_type_match(unsigned wanted, unsigned raw, int building)
{
    if (wanted == 229) return 1; /* Any Unit includes buildings. */
    if (wanted == 231) return building;
    if (wanted == 230) return !building &&
        (raw == 0 || raw == 3 || raw == 7 || raw == 19 || raw == 32);
    return raw == wanted;
}

static int
sc64_t6_world_count(void *context, const Sc64T4Condition *condition,
                    unsigned trigger_owner, u32 *count)
{
    Sc64T6MissionState *state = (Sc64T6MissionState *)context;
    Sc64T4QueryRect area;
    u32 result = 0;
    int global = condition && condition->opcode == 2;
    if (!state || !condition || !count || trigger_owner >= 8 ||
        (condition->opcode != 2 && condition->opcode != 3 && condition->opcode != 15)) return 0;
    if (condition->opcode == 15) {
        if (condition->unit != 231 ||
            (condition->player != 1 && condition->player != 4)) return 0;
        *count = state->deaths[condition->player];
        return 1;
    }
    if (global) {
        area.left = area.top = 0; area.right = area.bottom = 0x7fffffff;
    } else if (!sc64_t6_area(condition->location, &area)) return 0;

    /* `comparison == 1` is used only for exact-zero rows in the retained
     * program; those include incomplete structures, while positive-presence
     * queries ignore foundations. */
    int include_foundations = condition->comparison == 1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        unsigned raw = u[i].raw_type >= 0 ? (unsigned)u[i].raw_type :
                       (unsigned)sc64_race_raw_unit(u[i].type);
        int selected = sc64_t6_selector_matches(condition->player, trigger_owner,
                           u[i].owner, u[i].raw_owner);
        if (selected < 0) return 0;
        if (!selected || !sc64_t6_type_match(condition->unit, raw, 0)) continue;
        Sc64T4QueryRect actor;
        if (!global && !sc64_t6_actor_rect(raw, 0, u[i].x, u[i].y, &actor)) return 0;
        if (global || sc64_t4_query_rect_overlap(&area, &actor)) ++result;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (!include_foundations && b[i].under_construction) continue;
        unsigned raw = b[i].raw_type >= 0 ? (unsigned)b[i].raw_type :
                       (unsigned)sc64_race_raw_building(b[i].type);
        int selected = sc64_t6_selector_matches(condition->player, trigger_owner,
                           b[i].owner, b[i].raw_owner);
        if (selected < 0) return 0;
        if (!selected || !sc64_t6_type_match(condition->unit, raw, 1)) continue;
        Sc64T4QueryRect actor;
        if (!global && !sc64_t6_actor_rect(raw, 1, b[i].x, b[i].y, &actor)) return 0;
        if (global || sc64_t4_query_rect_overlap(&area, &actor)) ++result;
    }
    *count = result;
    return 1;
}

static int
sc64_t6_location_center(unsigned id, int *x, int *y)
{
    const Sc64T4Location *loc;
    if (!x || !y || !id || id > sc64_t6_scenario.location_count) return 0;
    loc = &sc64_t6_scenario.locations[id - 1];
    if (loc->elevation || loc->left >= loc->right || loc->top >= loc->bottom ||
        loc->right > 96u * 32u || loc->bottom > 96u * 32u) return 0;
    *x = (int)((loc->left + (loc->right - loc->left) / 2u) / 2u);
    *y = (int)((loc->top + (loc->bottom - loc->top) / 2u) / 2u);
    return 1;
}

static int
sc64_t6_spawn_site(unsigned location_id, int x, int y, int type,
                   int *out_x, int *out_y)
{
    Sc64T4QueryRect area;
    if (!sc64_t6_area(location_id, &area)) return 0;
    /* Deterministic bounded ring search. A single accepted action is atomic:
     * no actor or trigger cursor is changed if the cell cannot be reserved. */
    for (int radius = 0; radius <= 8; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            int dx = radius - FAST_ABS(dy);
            for (int sign = 0; sign < (dx ? 2 : 1); ++sign) {
                int px = x + (sign ? -dx : dx) * 16;
                int py = y + dy * 16;
                int blocked = 0;
                if (px < 16 || py < 16 || px >= WORLD_W - 16 || py >= WORLD_H - 16 ||
                    px * 2 < area.left || px * 2 >= area.right ||
                    py * 2 < area.top || py * 2 >= area.bottom ||
                    !is_traversable(px / 16, py / 16, -1)) continue;
                for (int i = 0; i < MAX_UNITS; ++i)
                    if (u[i].alive && dist2(px, py, u[i].x, u[i].y) < 12 * 12) {
                        blocked = 1; break;
                    }
                if (!blocked) {
                    *out_x = px; *out_y = py; return 1;
                }
            }
        }
    }
    (void)type;
    return 0;
}

static int
sc64_t6_world_create(void *context, const Sc64T4Action *action,
                     unsigned trigger_owner)
{
    Sc64T6MissionState *state = (Sc64T6MissionState *)context;
    unsigned property_index, count;
    int x, y, type, slot;
    if (!state || !action || trigger_owner >= 8 || action->opcode != 11 ||
        action->player != 4 || (action->unit != 37 && action->unit != 38) ||
        !sc64_t4_creation_count(action->modifier, &count) || count != 1 ||
        !sc64_t4_creation_property_index(action->argument, &property_index) ||
        property_index >= 30 || !sc64_t6_location_center(action->location, &x, &y)) return -1;
    type = sc64_race_unit_from_raw(action->unit);
    if (type < 0 || (action->unit == 37 && type != U_ZERGLING) ||
        (action->unit == 38 && type != U_HYDRALISK)) return -1;
    slot = find_unit_slot();
    if (slot < 0) return 0;
    if (!sc64_t6_spawn_site(action->location, x, y, type, &x, &y)) return 0;
    slot = spawn_unit(sc64_t6_runtime_owner(4), type, x, y);
    if (slot < 0) return 0;
    race_t6_unit_created(slot);
    u[slot].raw_type = action->unit;
    u[slot].raw_owner = 4;
    /* Verified UPRP rows 0..29 request 100% HP and native order 117. The
     * T6 world mirrors that order in its independent burrow bitset; this
     * does not infer the separate native +0xd8 flag's meaning. */
    const Sc64T4UnitProperties *property = &sc64_t6_properties[property_index];
    if ((property->valid_fields & 2u) && property->hp_percent == 100)
        u[slot].hp = u[slot].maxhp;
    if ((property->valid_special & 2u) && (property->special & 2u))
        race_t6_set_unit_burrowed(slot, 1);
    return 1;
}

static int
sc64_t6_resource_target_slots(unsigned selector, u8 targets[8])
{
    unsigned n = 0;
    if (selector <= 7) targets[n++] = (u8)selector;
    else if (selector == 13) targets[n++] = 0; /* All retained actions execute in slot 0 context. */
    else if (selector >= 18 && selector <= 21) {
        for (unsigned i = 0; i < 8; ++i)
            if (sc64_t6_source_force(i) == (int)(selector - 17)) targets[n++] = (u8)i;
    } else return 0;
    return (int)n;
}

static int
sc64_t6_world_resources(void *context, const Sc64T4Action *action,
                        unsigned trigger_owner)
{
    Sc64T6MissionState *state = (Sc64T6MissionState *)context;
    u8 targets[8];
    int n;
    if (!state || !action || action->opcode != 26 || trigger_owner >= 8 ||
        action->modifier != SC64_RESOURCE_MOD_SET)
        return -1;
    /* The retained T6 rows are intentionally exact: slot 0 gets 75 minerals,
     * while source Force 3 and Force 4 receive 8000 of both resources. */
    if ((action->player == 0 && action->unit != SC64_RESOURCE_MINERALS) ||
        ((action->player == 20 || action->player == 21) &&
         action->unit != SC64_RESOURCE_BOTH) ||
        (action->player != 0 && action->player != 20 && action->player != 21) ||
        (action->player == 0 && action->argument != 75u) ||
        ((action->player == 20 || action->player == 21) && action->argument != 8000u))
        return -1;
    n = sc64_t6_resource_target_slots(action->player, targets);
    if (!n) return -1;
    /* Validate all target banks before committing the action. The retained
     * operands are small (75 / 8000), but this keeps the callback atomic if a
     * future importer accepts a different resource target. */
    for (int k = 0; k < n; ++k) {
        unsigned source = targets[k];
        int owner = sc64_t6_runtime_owner(source);
        if (owner < 0 || source >= 8 || action->argument > 65535u) return -1;
        if (owner == SIDE && (minerals < 0 || gas < 0 || minerals > 65535 || gas > 65535)) return -1;
        if (owner != SIDE && owner != NEUTRAL &&
            (race_minerals[owner] < 0 || race_gas[owner] < 0 ||
             race_minerals[owner] > 65535 || race_gas[owner] > 65535)) return -1;
    }
    for (int k = 0; k < n; ++k) {
        unsigned source = targets[k];
        int owner = sc64_t6_runtime_owner(source);
        Sc64ResourceActionState candidate = state->resources[source];
        if (owner == SIDE) {
            candidate.current_minerals = (u32)minerals;
            candidate.current_gas = (u32)gas;
        } else if (owner != NEUTRAL) {
            candidate.current_minerals = (u32)race_minerals[owner];
            candidate.current_gas = (u32)race_gas[owner];
        }
        if (!sc64_resource_action_apply_one_player(&candidate, action->unit,
                action->modifier, action->argument)) return -1;
        if (owner == SIDE) { minerals = (int)candidate.current_minerals; gas = (int)candidate.current_gas; }
        else if (owner != NEUTRAL) { race_minerals[owner] = (int)candidate.current_minerals; race_gas[owner] = (int)candidate.current_gas; }
        state->resources[source] = candidate;
    }
    return 1;
}

static u8
sc64_t6_ai_script_bit(u32 id)
{
    if (id == SC64_T6_SCRIPT_RSCU) return SC64_T6_AI_RSCU;
    if (id == SC64_T6_SCRIPT_TER6) return SC64_T6_AI_T6A;
    if (id == SC64_T6_SCRIPT_TE6B) return SC64_T6_AI_T6B;
    return 0;
}

static int
sc64_t6_world_ai(void *context, const Sc64T4Action *action,
                 unsigned trigger_owner)
{
    Sc64T6MissionState *state = (Sc64T6MissionState *)context;
    unsigned bit;
    if (!state || !action || trigger_owner >= 8 ||
        (action->opcode != 15 && action->opcode != 16) || action->player != 0)
        return -1;
    bit = sc64_t6_ai_script_bit(action->argument);
    if (!bit) return -1;
    if ((bit == SC64_T6_AI_RSCU && trigger_owner != 2 && trigger_owner != 6) ||
        (bit == SC64_T6_AI_T6A && trigger_owner != 4) ||
        (bit == SC64_T6_AI_T6B && trigger_owner != 1 && trigger_owner != 3 &&
         trigger_owner != 5 && trigger_owner != 7)) return -1;
    if (action->opcode == 15) {
        if (action->location) return -1;
    } else {
        Sc64T4QueryRect ignored;
        if (!sc64_t6_area(action->location, &ignored)) return -1;
    }
    state->ai_requests[trigger_owner] |= (u8)bit;
    return 1;
}

static int
sc64_t6_world_presentation(void *context, const Sc64T4Action *action,
                           unsigned trigger_owner)
{
    Sc64T6MissionState *state = (Sc64T6MissionState *)context;
    int x, y;
    if (!state || !action || trigger_owner >= 8) return -1;
    switch (action->opcode) {
    case 7: case 8: case 9: case 12:
        if (trigger_owner != 0) return 1;
        state->message = action->opcode;
        /* Source transcript duration/audio is not decoded; this fixed
         * ninety-tick display is an authored GBA presentation policy. */
        state->message_ticks = 90;
        race_campaign.message = state->message;
        race_campaign.message_ticks = state->message_ticks;
        return 1;
    case 10:
        if (trigger_owner != 0 || !sc64_t6_location_center(action->location, &x, &y) ||
            x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H) return -1;
        cx = x; cy = y;
        return 1;
    default: return -1;
    }
}

static const Sc64T6RuntimeOps race_t6_world_ops = {
    sc64_t6_world_count, sc64_t6_world_create, sc64_t6_world_resources,
    sc64_t6_world_ai, sc64_t6_world_presentation
};

static int
race_t6_init(void)
{
    memset(&race_t6, 0, sizeof(race_t6));
    if (!sc64_t6_scenario_runtime_init(&race_t6.triggers)) return 0;
    /* Initial UNIT flags preserve the source's burrowed actors. Keep the
     * state in the dedicated bitmap, not by reinterpreting Unit.flags later. */
    for (int i = 0; i < MAX_UNITS; ++i) {
        sc64_t6_burrowed_set(&race_t6, (unsigned)i, 0);
        if (u[i].alive && (u[i].raw_type == 37 || u[i].raw_type == 38) &&
            (u[i].flags & 2u))
            sc64_t6_burrowed_set(&race_t6, (unsigned)i, 1);
    }
    return 1;
}

static void
sc64_t6_transfer_unit(int slot)
{
    Unit *q = &u[slot];
    q->owner = SIDE; q->selected = 0; q->order = ORDER_IDLE;
    q->target = -1; q->tx = q->x; q->ty = q->y;
    q->path_timer = q->path_retry = 0;
    q->path_x = q->x; q->path_y = q->y;
    race_t6_set_unit_burrowed(slot, 0);
}

static int
sc64_t6_rescue_contact_near(int x, int y)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE &&
            FAST_ABS(x - u[i].x) <= SC64_T6_RESCUE_CONTACT_RADIUS &&
            FAST_ABS(y - u[i].y) <= SC64_T6_RESCUE_CONTACT_RADIUS) return 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE &&
            FAST_ABS(x - b[i].x) <= SC64_T6_RESCUE_CONTACT_RADIUS &&
            FAST_ABS(y - b[i].y) <= SC64_T6_RESCUE_CONTACT_RADIUS) return 1;
    return 0;
}

static void
sc64_t6_rescue_group(unsigned raw_owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == RESCUABLE && u[i].raw_owner == (int)raw_owner)
            sc64_t6_transfer_unit(i);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == RESCUABLE && b[i].raw_owner == (int)raw_owner)
            b[i].owner = SIDE;
    if (raw_owner < 8) race_t6.rescue_mask |= (u8)(1u << raw_owner);
    target_lists_valid = 0;
    sc64_target_index_end();
}

static void
sc64_t6_rescue_tick(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (!q->alive || q->owner != RESCUABLE ||
            (q->raw_owner != 2 && q->raw_owner != 6) ||
            !sc64_t6_rescue_contact_near(q->x, q->y)) continue;
        unsigned source = (unsigned)q->raw_owner;
        sc64_t6_transfer_unit(i);
        race_t6.rescue_mask |= (u8)(1u << source);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *q = &b[i];
        if (!q->alive || q->owner != RESCUABLE ||
            (q->raw_owner != 2 && q->raw_owner != 6) ||
            !sc64_t6_rescue_contact_near(q->x, q->y)) continue;
        unsigned source = (unsigned)q->raw_owner;
        if (q->type == B_COMMAND && building_complete(i))
            sc64_t6_rescue_group(source);
        else {
            q->owner = SIDE;
            race_t6.rescue_mask |= (u8)(1u << source);
        }
    }
}

/* The native script bytecode is unavailable. This fallback only advances
 * already-requested enemy Zerg actors from their explicit Burrowed request
 * to active combat when a hostile is within 96 GBA pixels; it does not add
 * units, resources, or an economic AI. The threshold/cadence are GBA policy. */
static void
sc64_t6_burrow_release_tick(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        int source = sc64_t6_current_source_slot(q->owner, q->raw_owner);
        if (!q->alive || !sc64_t6_burrowed_get(&race_t6, (unsigned)i) ||
            (q->raw_type != 37 && q->raw_type != 38) || source < 0 ||
            !(race_t6.ai_requests[source] &
              (SC64_T6_AI_T6A | SC64_T6_AI_T6B))) continue;
        for (int j = 0; j < MAX_UNITS; ++j)
            if (u[j].alive && sc64_t6_owners_hostile(q->owner, u[j].owner) &&
                dist2(q->x, q->y, u[j].x, u[j].y) <= 96 * 96) {
                race_t6_set_unit_burrowed(i, 0);
                break;
            }
    }
}

/* Opaque Ter6/Te6b bodies use the same conservative guard policy as the
 * generic campaign fallback. Workers mine; at most two idle armed actors per
 * requested computer source slot leave their posts every 180 seconds. The
 * remaining imported squads defend locally. Sending every guard at once
 * created an unsupported whole-map rush before the transport tech could
 * finish. This cadence is authored GBA behavior, not recovered native AI. */
static void
sc64_t6_ai_backend_tick(void)
{
    if ((frame % SC64_T6_AI_TICK_PERIOD) != 0) return;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        int source, best = -1, best_distance = 0x7fffffff;
        if (!q->alive || (q->owner != ENEMY && q->owner != 4) ||
            q->order != ORDER_IDLE) continue;
        source = sc64_t6_current_source_slot(q->owner, q->raw_owner);
        if (source < 0 || !(race_t6.ai_requests[source] &
                (SC64_T6_AI_T6A | SC64_T6_AI_T6B))) continue;
        if (sc64_race_is_worker(q->type)) {
            for (int j = 0; j < MAX_MINERALS; ++j) if (m[j].amount > 0) {
                int d = dist2(q->x, q->y, m[j].x, m[j].y);
                if (d < best_distance) { best_distance = d; best = j; }
            }
            if (best >= 0) {
                q->order = ORDER_MINE; q->target = best;
                q->carry = 0; q->work_timer = 0;
            }
        }
    }
    if ((frame % (180u * 30u)) != 0) return;
    int tx = -1, ty = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) { tx = u[i].x; ty = u[i].y; break; }
    if (tx < 0) for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE) { tx = b[i].x; ty = b[i].y; break; }
    if (tx < 0) return;
    u8 sent[8] = {0};
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        int source;
        if (!q->alive || (q->owner != ENEMY && q->owner != 4) ||
            q->order != ORDER_IDLE || race_t6_unit_burrowed(i) ||
            sc64_race_unit_damage(q->type) <= 0) continue;
        source = sc64_t6_current_source_slot(q->owner, q->raw_owner);
        if (source < 0 || !(race_t6.ai_requests[source] &
                (SC64_T6_AI_T6A | SC64_T6_AI_T6B)) || sent[source] >= 2) continue;
        q->order = ORDER_ATTACK_MOVE; q->target = -1; q->tx = tx; q->ty = ty;
        ++sent[source];
    }
}

static int
race_t6_tick(void)
{
    if (!race_t6_active()) return 1;
    if (race_t6.message_ticks && !--race_t6.message_ticks)
        race_t6.message = 0;
    if (race_t6.ping_ticks && !--race_t6.ping_ticks)
        race_t6.ping_x = race_t6.ping_y = 0;
    /* Keep the resource state as an opcode-26 event ledger, not a mirror of
     * live wallets that change through ordinary mining and spending. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (!u[i].alive) sc64_t6_burrowed_set(&race_t6, (unsigned)i, 0);
    if (!sc64_t6_scenario_runtime_step_prevalidated(&race_t6.triggers,
            &race_t6_world_ops, &race_t6, screen != SCREEN_PLAY)) return 0;
    sc64_t6_rescue_tick();
    sc64_t6_burrow_release_tick();
    sc64_t6_ai_backend_tick();
    race_campaign.message = race_t6.message;
    race_campaign.message_ticks = race_t6.message_ticks;
    race_campaign.objective_stage = race_t6.triggers.switches.bits[0];
    if (race_t6.triggers.player[0].outcome == SC64_T4_TR_LOST) {
        race_campaign.phase = SC64_RACE_PHASE_FAILED;
        lost = 1;
    } else if (race_t6.triggers.player[0].outcome == SC64_T4_TR_WON) {
        race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
        race_completed |= 1 << SC64_T6_GLOBAL_ID;
        won = 1;
    }
    return 1;
}

#endif /* T6 world runtime */
#endif /* SC64_T6_WORLD_ADAPTER_H */
