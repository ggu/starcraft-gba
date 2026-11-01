/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P9_RUNTIME_ADAPTER_H
#define SC64_P9_RUNTIME_ADAPTER_H

#if defined(SC64_P9_RUNTIME)

#if !defined(SC64_P9_DATA_ENABLED) || !defined(SC64_P8_RUNTIME) || \
    !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 2
#error "P9 runtime requires authenticated P9 data on the P1-P8 Protoss profile"
#endif

/* P9 "Shadow Hunters" campaign binding. The authenticated compact source
 * program (12 triggers) runs for each of its four executing source owners.
 * Unlike P8 it uses PreserveTrigger, KillUnitAtLocation and MoveUnit: each
 * Cerebrate that dies while Zeratul is absent from its location is recreated.
 * Census and owner selection reuse the shared Z8 world services over the
 * live pools. Mission strings stay in the private CHK and are not shown.
 * Included after the P8 runtime adapter. */
#include "sc64_p9_scenario_generated.h"
#include "sc64_p9_state.h"

enum {
    SC64_P9_MAP_UNITS = 343,
    SC64_P9_HUMAN_SLOT = 7,
    SC64_P9_FRAME_MS = 42,   /* Same source clock as P8 (native Fastest). */
    SC64_P9_PASS_UPDATES = 8, /* Authored trigger-pass cadence. */
    SC64_P9_ACTION_PENDING = 0, SC64_P9_ACTION_COMPLETE = 1,
    SC64_P9_ACTION_WON = 2, SC64_P9_ACTION_LOST = 3, SC64_P9_ACTION_BLOCKED = 4,
    SC64_P9_KILL_DAMAGE = 1 << 24
};

_Static_assert(SC64_P9_SOURCE_GLOBAL_ID == 28u && SC64_P9_SOURCE_MAP_COLS == 128u &&
               SC64_P9_SOURCE_MAP_ROWS == 128u &&
               SC64_P9_SOURCE_TRIGGER_COUNT == SC64_P9_TRIGGER_LIMIT &&
               SC64_P9_SOURCE_ACTION_COUNT == 34u &&
               SC64_P9_SOURCE_CONDITION_COUNT == 17u &&
               SC64_P9_SOURCE_LOCATION_COUNT == 64u,
               "P9 source identity mismatch");

static Sc64P9PersistentState race_p9;
static Sc64Z8WorldView race_p9_view;
static Sc64Z8GameContext race_p9_game;
static const Sc64RaceMap *race_p9_map;
static u16 race_p9_preserve_mask;
static s8 race_p9_product_slot = -1;

static const u8 sc64_p9_owner_modes_expected[8] = { 1, 2, 3, 2, 2, 1, 1, 0 };

static int race_p9_active(void)
{ return !race_mode && race_selected_mission == SC64_P9_GLOBAL_ID; }

/* Validate the compact program before any cursor indexes it, and record which
 * triggers carry PreserveTrigger. Only the opcodes implemented below and
 * switches 0-7 are accepted. */
static int sc64_p9_source_valid(u16 *preserve_mask)
{
    unsigned next_action = 0, next_condition = 0;
    u16 preserve = 0;
    for (unsigned t = 0; t < SC64_P9_TRIGGER_LIMIT; ++t) {
        const Sc64P9SourceTrigger *tr = &sc64_p9_source_triggers[t];
        if (tr->first_action != next_action || tr->first_condition != next_condition ||
            tr->execution_flags || tr->initial_cursor || tr->action_count > 16u ||
            tr->first_action + tr->action_count > SC64_P9_SOURCE_ACTION_COUNT ||
            tr->first_condition + tr->condition_count > SC64_P9_SOURCE_CONDITION_COUNT)
            return 0;
        for (unsigned p = 0; p < 27u; ++p)
            if (tr->enabled_players[p] > 1u ||
                (tr->enabled_players[p] && p != 7u && (p < 18u || p > 21u))) return 0;
        for (unsigned i = 0; i < tr->action_count; ++i) {
            const Sc64P9SourceAction *a = &sc64_p9_source_actions[tr->first_action + i];
            if (a->trigger != t || a->index != i || a->location > 64u ||
                a->trailing[0] || a->trailing[1] || a->trailing[2]) return 0;
            switch (a->opcode) {
            case 1: case 2:
                if (i + 1u != tr->action_count) return 0;
                break;
            case 3: preserve |= (u16)(1u << t); break;
            case 4: if (a->time > 65535u) return 0; break;
            case 8: case 9: case 12: case 16: case 23: case 26: case 39: break;
            case 11: if (a->group2) return 0; break;
            case 13: if (a->group2 >= 8u) return 0; break;
            default: return 0;
            }
        }
        for (unsigned i = 0; i < tr->condition_count; ++i) {
            const Sc64P9SourceCondition *c =
                &sc64_p9_source_conditions[tr->first_condition + i];
            if (c->trigger != t || c->index != i || c->location > 64u) return 0;
            if (c->opcode == 11u && c->resource >= 8u) return 0;
            if (c->opcode != 2u && c->opcode != 3u && c->opcode != 11u &&
                c->opcode != 12u) return 0;
        }
        next_action += tr->action_count;
        next_condition += tr->condition_count;
    }
    if (next_action != SC64_P9_SOURCE_ACTION_COUNT ||
        next_condition != SC64_P9_SOURCE_CONDITION_COUNT) return 0;
    for (unsigned i = 0; i < SC64_P9_SOURCE_LOCATION_COUNT; ++i) {
        const Sc64P9SourceLocation *l = &sc64_p9_source_locations[i];
        if (l->left > l->right || l->top > l->bottom || l->right > 4096u ||
            l->bottom > 4096u) return 0;
    }
    if (preserve_mask) *preserve_mask = preserve;
    return 1;
}

static int race_p9_map_shape_valid(const Sc64RaceMap *map)
{
    if (!map || SC64_P9_GLOBAL_ID >= SC64_RACE_MAP_COUNT ||
        map != &sc64_race_maps[SC64_P9_GLOBAL_ID] || map->width != 128u ||
        map->height != 128u || map->human_slot != SC64_P9_HUMAN_SLOT ||
        map->unit_count != SC64_P9_MAP_UNITS || !map->owner_modes || !map->units ||
        !map->mission_id || map->mission_id[0] != 'P' || map->mission_id[1] != '9' ||
        map->mission_id[2] != '\0' || !sc64_p9_source_valid(0)) return 0;
    for (unsigned i = 0; i < 8; ++i)
        if (map->owner_modes[i] != sc64_p9_owner_modes_expected[i] ||
            sc64_p9_source_forces[i] > 3u) return 0;
    for (unsigned r = 0; r < SC64_P9_RUNNER_COUNT; ++r)
        if (sc64_p9_source_forces[sc64_p9_runner_slot[r]] + 1u != sc64_p9_runner_force[r])
            return 0;
    return 1;
}

/* From the canonical descriptor, so save candidates can be checked before
 * the world is bound. */
static int race_p9_source_owner(unsigned slot)
{
    const Sc64RaceMap *map;
    if (slot >= 8u || SC64_P9_GLOBAL_ID >= SC64_RACE_MAP_COUNT) return -1;
    map = &sc64_race_maps[SC64_P9_GLOBAL_ID];
    if (map->human_slot != SC64_P9_HUMAN_SLOT || !map->owner_modes) return -1;
    return slot == SC64_P9_HUMAN_SLOT ? SIDE : map->owner_modes[slot];
}

/* P9 runs the shared Protoss unit services too (its human Arbiter cloaks,
 * Zeratul is permanently cloaked). These dispatchers cover P8 and P9; the
 * P8 adapter's own versions are renamed race_p8_later_* in this build. */
static int race_protoss_later_services_active(void)
{ return race_p9_active() || race_p8_later_services_active(); }
static int race_protoss_later_source_owner(unsigned slot)
{ return race_p9_active() ? race_p9_source_owner(slot) : race_p8_later_source_owner(slot); }
static u8 *race_protoss_later_service_clock(void)
{ return race_p9_active() ? race_p9.service_clock : race_p8_later_service_clock(); }
static int race_protoss_later_service_mission(int mission)
{ return mission == SC64_P9_GLOBAL_ID || race_p8_later_service_mission(mission); }
static int race_protoss_later_mission_source_owner(int mission, unsigned slot)
{
    return mission == SC64_P9_GLOBAL_ID ? race_p9_source_owner(slot) :
        race_p8_later_mission_source_owner(mission, slot);
}

/* The P9 map places no Shield Battery; only human-built ones exist. */
static int race_p9_battery_ok(const Building *q)
{
    return q->raw_type == 172 && q->raw_owner == SC64_P9_HUMAN_SLOT &&
        q->owner == SIDE && !q->queue_count && !q->queue_head &&
        q->queue_progress >= 0 && q->queue_progress <= SC64_P7_BATTERY_MAX_ENERGY_Q8 &&
        q->level >= 0 && q->level < SC64_P7_BATTERY_REGEN_DEN &&
        sc64_p7_battery_energy_valid((unsigned)q->queue_progress, (unsigned)q->level) &&
        q->shield <= sc64_race_building_shields(B_P7_SHIELD_BATTERY);
}

static int race_protoss_later_battery_candidate_valid(int mission, const Building *q)
{
    if (mission == SC64_P9_GLOBAL_ID) return q && race_p9_battery_ok(q);
    return race_p8_later_battery_candidate_valid(mission, q);
}

static int race_protoss_later_battery_world_valid(void)
{
    if (!race_p9_active()) return race_p8_later_battery_world_valid();
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        b[i].type == B_P7_SHIELD_BATTERY &&
        (!sc64_p7_battery_fields_valid(i) || !race_p9_battery_ok(&b[i]))) return 0;
    return 1;
}

static int race_protoss_later_battery_initialize(const Sc64RaceMap *map)
{
    if (!race_p9_active()) return race_p8_later_battery_initialize(map);
    if (map != &sc64_race_maps[SC64_P9_GLOBAL_ID]) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_P7_SHIELD_BATTERY) return 0;
    return 1;
}

static void race_p9_note_unit_spawn(int slot, int owner, int type)
{
    if (!race_p9_active() || slot < 0 || slot >= MAX_UNITS) return;
    u[slot].raw_type = (s16)sc64_race_raw_unit(type);
    u[slot].raw_owner = owner == SIDE ? SC64_P9_HUMAN_SLOT : race_p9_product_slot;
}

static void race_p9_note_building_spawn(int slot, int owner, int type)
{
    if (!race_p9_active() || slot < 0 || slot >= MAX_BUILDINGS) return;
    b[slot].raw_type = (s16)sc64_race_raw_building(type);
    b[slot].raw_owner = owner == SIDE ? SC64_P9_HUMAN_SLOT : -1;
}

static void race_p9_note_product_building(int building)
{
    race_p9_product_slot = race_p9_active() && building >= 0 &&
        building < MAX_BUILDINGS ? (s8)b[building].raw_owner : -1;
}

static const Sc64Z8Rect *race_p9_rect(unsigned location, Sc64Z8Rect *out)
{
    const Sc64P9SourceLocation *l;
    if (!location || location > SC64_P9_SOURCE_LOCATION_COUNT) return 0;
    l = &sc64_p9_source_locations[location - 1u];
    *out = (Sc64Z8Rect){(s16)l->left, (s16)l->top, (s16)l->right, (s16)l->bottom};
    return out;
}

static int race_p9_condition(const Sc64P9SourceCondition *c, unsigned owner)
{
    Sc64Z8Condition z = {0};
    Sc64Z8Rect rect;
    u32 count = 0;
    if (c->flags & 2u) return 1;
    switch (c->opcode) {
    case 2: case 3:
        z.location = c->location; z.player = c->player; z.amount = c->amount;
        z.unit = c->unit; z.mask = c->mask; z.comparison = c->comparison;
        z.opcode = c->opcode; z.resource = c->resource; z.flags = c->flags;
        z.index = c->index;
        if ((c->flags & ~0x12u) || !sc64_z8_comparison_known(z.opcode, z.comparison))
            return -1;
        if (c->opcode == 3u && !c->location) return 0;
        if (sc64_z8_world_count(&race_p9_view, &z, owner,
                                c->opcode == 3u ? race_p9_rect(c->location, &rect) : 0,
                                &count) != 1) return -1;
        return sc64_z8_compare(z.comparison, count, z.amount);
    case 11: {
        int set = (race_p9.switches >> c->resource) & 1;
        return c->comparison == 2u ? set : c->comparison == 3u ? !set : -1;
    }
    case 12:
        return sc64_z8_compare(c->comparison,
            (u32)(((unsigned long long)race_campaign.elapsed_frames * SC64_P9_FRAME_MS) / 1000u),
            c->amount);
    default: return -1;
    }
}

static int race_p9_owner_match(unsigned selector, unsigned trigger_owner, int slot)
{
    if (slot < 0 || slot >= (int)SC64_Z8_SOURCE_OWNER_LIMIT) return -1;
    return sc64_z8_world_owner_matches(&race_p9_view, selector, trigger_owner,
                                       (unsigned)slot);
}

/* SetResources applies to every active slot the selector names. */
static int race_p9_set_resources(const Sc64P9SourceAction *a, unsigned trigger_owner)
{
    int matched = 0;
    if (a->location || a->unit > SC64_RESOURCE_BOTH ||
        (a->modifier != SC64_RESOURCE_MOD_SET && a->modifier != SC64_RESOURCE_MOD_ADD &&
         a->modifier != SC64_RESOURCE_MOD_SUBTRACT)) return -1;
    for (int slot = 0; slot < 8; ++slot) {
        Sc64ResourceActionState v = {0, 0, 0, 0};
        int w = sc64_p9_wallet_index((unsigned)slot), human = slot == SC64_P9_HUMAN_SLOT;
        int match = race_p9_owner_match(a->group1, trigger_owner, slot);
        if (match < 0) return -1;
        if (!match) continue;
        if (!human && w < 0) return -1;
        v.current_minerals = human ? (u32)minerals : race_p9.wallet_minerals[w];
        v.current_gas = human ? (u32)gas : race_p9.wallet_gas[w];
        if (!sc64_resource_action_apply_one_player(&v, (u16)a->unit, a->modifier,
                                                    a->group2) ||
            v.current_minerals > 65535u || v.current_gas > 65535u) return -1;
        if (human) {
            minerals = race_minerals[SIDE] = (int)v.current_minerals;
            gas = race_gas[SIDE] = (int)v.current_gas;
        } else {
            race_p9.wallet_minerals[w] = (u16)v.current_minerals;
            race_p9.wallet_gas[w] = (u16)v.current_gas;
        }
        ++matched;
    }
    return matched ? SC64_P9_ACTION_COMPLETE : -1;
}

/* Match one live actor against an action's owner, type and location. */
static int race_p9_actor_selected(unsigned index, const Sc64P9SourceAction *a,
                                  unsigned trigger_owner, const Sc64Z8Rect *rect)
{
    Sc64Z8ActorView v;
    int owner, type, overlap;
    if (sc64_z8_game_actor_at(0, index, &v) != 1) return -1;
    if (!v.alive) return 0;
    owner = race_p9_owner_match(a->group1, trigger_owner, v.source_owner);
    if (owner <= 0) return owner;
    type = sc64_z8_world_actor_type(&v, a->unit);
    if (type <= 0) return type;
    overlap = sc64_z8_world_overlap(&v, rect);
    return overlap;
}

/* KillUnitAtLocation, all matching actors (modifier 0). Validate the whole
 * set before the first death so a bad row cannot leave a partial effect. */
static int race_p9_kill_at(const Sc64P9SourceAction *a, unsigned trigger_owner)
{
    Sc64Z8Rect rect;
    if (a->modifier || a->group2 || !race_p9_rect(a->location, &rect)) return -1;
    for (unsigned pass = 0; pass < 2; ++pass)
        for (unsigned i = 0; i < MAX_UNITS + MAX_BUILDINGS; ++i) {
            int selected = race_p9_actor_selected(i, a, trigger_owner, &rect);
            if (selected < 0) return -1;
            if (!selected || !pass) continue;
            if (i < MAX_UNITS) invincible_units[i] = 0;
            else invincible_buildings[i - MAX_UNITS] = 0;
            hit_target_impl_ex((int)i, SC64_P9_KILL_DAMAGE, -1, -1, -1,
                               SC64_HIT_PRECOMPUTED_HP | SC64_HIT_SKIP_HALLUCINATION_SCALE);
        }
    return SC64_P9_ACTION_COMPLETE;
}

/* MoveUnit, all matching units (modifier 0), to the destination centre. The
 * native placement search is not modeled; collision separation resolves the
 * shared centre, as in the established Z5 adapter. */
static int race_p9_move_unit(const Sc64P9SourceAction *a, unsigned trigger_owner)
{
    Sc64Z8Rect from, to;
    int moved = 0;
    if (a->modifier || a->unit == 231u || !race_p9_rect(a->location, &from) ||
        !race_p9_rect(a->group2, &to)) return -1;
    for (unsigned pass = 0; pass < 2; ++pass)
        for (unsigned i = 0; i < MAX_UNITS; ++i) {
            int selected = race_p9_actor_selected(i, a, trigger_owner, &from);
            if (selected < 0) return -1;
            if (!selected || !pass) continue;
            u[i].x = ((int)to.left + (int)to.right) / 4;
            u[i].y = ((int)to.top + (int)to.bottom) / 4;
            u[i].tx = u[i].x; u[i].ty = u[i].y;
            u[i].order = ORDER_IDLE; u[i].target = -1;
            u[i].path_x = u[i].x; u[i].path_y = u[i].y;
            u[i].path_timer = 0; u[i].path_retry = 0;
            moved = 1;
        }
    if (moved) {
        target_lists_valid = 0;
        sc64_unit_bucket_active = 0;
        sc64_target_index_end();
    }
    return SC64_P9_ACTION_COMPLETE;
}

/* CreateUnitWithProperties without a property: the Cerebrate respawn. Its
 * building footprint is centred on the location and clamped to the map. */
static int race_p9_create(const Sc64P9SourceAction *a, unsigned trigger_owner)
{
    Sc64Z8Rect rect;
    unsigned slot;
    int type, live, left, top, right, bottom, x, y, created;
    if (a->flags != 0x10u || a->modifier || a->group2 || !race_p9_rect(a->location, &rect) ||
        !sc64_z8_world_resolve_one(&race_p9_view, a->group1, trigger_owner, &slot))
        return -1;
    type = sc64_race_building_from_raw(a->unit);
    live = race_p9_source_owner(slot);
    if (type < 0 || live < SIDE || live > 4 ||
        !sc64_z8_game_source_extents(a->unit, 0, 0, 1, &left, &top, &right, &bottom))
        return -1;
    x = ((int)rect.left + (int)rect.right) / 2;
    y = ((int)rect.top + (int)rect.bottom) / 2;
    if (x < left) x = left;
    if (y < top) y = top;
    if (x + right >= 4096) x = 4096 - right - 1;
    if (y + bottom >= 4096) y = 4096 - bottom - 1;
    if (find_build_slot() < 0) return -1;
    created = spawn_build(live, type, x / 2, y / 2, 0);
    if (created < 0) return -1;
    b[created].raw_type = (s16)a->unit;
    b[created].raw_owner = (s16)slot;
    return SC64_P9_ACTION_COMPLETE;
}

#include "sc64_p9_ai.h"

static int race_p9_action(unsigned runner, const Sc64P9SourceAction *a, int resume)
{
    unsigned owner = sc64_p9_runner_slot[runner];
    u8 bit = (u8)(1u << runner);
    if (a->flags & 2u) return SC64_P9_ACTION_COMPLETE;
    if (resume && a->opcode != 4u) return -1;
    switch (a->opcode) {
    case 1: return SC64_P9_ACTION_WON;
    case 2: return SC64_P9_ACTION_LOST;
    case 3: return SC64_P9_ACTION_COMPLETE; /* PreserveTrigger: see the runner. */
    case 4: /* One shared wait timer per runner, as in the P8 timed service. */
        if (race_p9.waiting_mask & bit)
            return resume ? SC64_P9_ACTION_PENDING : SC64_P9_ACTION_BLOCKED;
        if (resume) return SC64_P9_ACTION_COMPLETE;
        race_p9.wait_ms[runner] = (u16)a->time;
        race_p9.waiting_mask |= bit;
        return SC64_P9_ACTION_PENDING;
    case 8: case 9: case 12: return SC64_P9_ACTION_COMPLETE; /* Not presented. */
    case 11: return race_p9_create(a, owner);
    case 13:
        if (a->modifier == 4u) race_p9.switches |= (u8)(1u << a->group2);
        else if (a->modifier == 5u) race_p9.switches &= (u8)~(1u << a->group2);
        else if (a->modifier == 6u) race_p9.switches ^= (u8)(1u << a->group2);
        else return -1;
        return SC64_P9_ACTION_COMPLETE;
    case 16: return sc64_p9_ai_start(&race_p9, a, owner) ? SC64_P9_ACTION_COMPLETE : -1;
    case 23: return race_p9_kill_at(a, owner);
    case 26: return race_p9_set_resources(a, owner);
    case 39: return race_p9_move_unit(a, owner);
    default: return -1;
    }
}

static int race_p9_trigger_enabled(const Sc64P9SourceTrigger *tr, unsigned runner)
{
    return tr->enabled_players[sc64_p9_runner_slot[runner]] || tr->enabled_players[17] ||
           tr->enabled_players[17u + sc64_p9_runner_force[runner]];
}

/* One source pass for one runner, following the shared P8 runner's cursor,
 * pending and blocked contract. A completed trigger with PreserveTrigger is
 * re-armed: it may run again on a later pass when its conditions hold. */
static int race_p9_runner_step(unsigned runner)
{
    Sc64P9Runner *r = &race_p9.runner[runner];
    for (unsigned t = 0; t < SC64_P9_TRIGGER_LIMIT; ++t) {
        const Sc64P9SourceTrigger *tr = &sc64_p9_source_triggers[t];
        u16 bit = (u16)(1u << t);
        if (!race_p9_trigger_enabled(tr, runner) || (r->done & bit)) continue;
        if (!(r->running & bit)) {
            int matches = 1;
            for (unsigned c = 0; c < tr->condition_count && matches; ++c) {
                int result = race_p9_condition(
                    &sc64_p9_source_conditions[tr->first_condition + c],
                    sc64_p9_runner_slot[runner]);
                if (result < 0 || result > 1) return 0;
                matches = result;
            }
            if (!matches) continue;
            r->started |= bit;
            r->running |= bit;
            r->cursor[t] = 0;
        }
        while (r->running & bit) {
            int resume = !!(r->waiting & bit);
            int result = race_p9_action(runner,
                &sc64_p9_source_actions[tr->first_action + r->cursor[t]], resume);
            if (result == SC64_P9_ACTION_PENDING) { r->waiting |= bit; break; }
            r->waiting &= (u16)~bit;
            if (result == SC64_P9_ACTION_BLOCKED) break;
            if (result < SC64_P9_ACTION_COMPLETE || result > SC64_P9_ACTION_LOST) return 0;
            if (result == SC64_P9_ACTION_WON) r->outcome = SC64_P9_OUTCOME_WON;
            else if (result == SC64_P9_ACTION_LOST && r->outcome != SC64_P9_OUTCOME_WON)
                r->outcome = SC64_P9_OUTCOME_LOST;
            if (++r->cursor[t] == tr->action_count) {
                r->running &= (u16)~bit;
                r->cursor[t] = 0;
                if (race_p9_preserve_mask & bit) r->started &= (u16)~bit;
                else r->done |= bit;
            }
        }
    }
    return 1;
}

static int race_p9_bind(const Sc64RaceMap *map)
{
    s8 live[SC64_Z8_SOURCE_OWNER_LIMIT];
    if (!race_p9_map_shape_valid(map) || !sc64_p9_source_valid(&race_p9_preserve_mask))
        return 0;
    race_p9_map = map;
    for (unsigned slot = 0; slot < SC64_Z8_SOURCE_OWNER_LIMIT; ++slot)
        live[slot] = slot < 8u ? (s8)race_p9_source_owner(slot) : (s8)-1;
    memset(&race_p9_view, 0, sizeof(race_p9_view));
    if (!sc64_z8_game_context_init(&race_p9_game, live, 4096u, 4096u)) return 0;
    sc64_z8_game_bind_actor_pool(&race_p9_view);
    /* Census rows are the OWNR-active players: 0, 2, 5, 6 and 7. */
    for (unsigned slot = 0; slot < 8u; ++slot) {
        if (sc64_p9_source_owners[slot]) race_p9_view.active_mask |= (u8)(1u << slot);
        race_p9_view.force[slot] = (u8)(sc64_p9_source_forces[slot] + 1u);
    }
    return 1;
}

/* Check candidate cursors against immutable source bounds before commit. */
static int sc64_p9_source_state_valid(const Sc64P9PersistentState *state)
{
    if (!sc64_p9_persistent_state_valid(state)) return 0;
    for (unsigned r = 0; r < SC64_P9_RUNNER_COUNT; ++r)
        for (unsigned t = 0; t < SC64_P9_TRIGGER_LIMIT; ++t)
            if ((state->runner[r].running & (1u << t)) &&
                state->runner[r].cursor[t] >= sc64_p9_source_triggers[t].action_count)
                return 0;
    return 1;
}

static int race_p9_runtime_state_valid(void)
{
    return sc64_p9_source_state_valid(&race_p9);
}

static int race_p9_runtime_init(const Sc64RaceMap *map)
{
    memset(&race_p9, 0, sizeof(race_p9));
    race_p9_map = 0;
    race_p9_product_slot = -1;
    if (!race_p9_active()) return 1;
    if (!race_p9_bind(map)) return 0;
    /* Fresh shared-service clock; the first source update seeds the
     * Arbiter/detection cache. */
    sc64_p7_arbiter_cache_due = 0;
    sc64_p7_arbiter_cache_initialized = 0;
    if (!race_p7_battery_initialize_source(map)) return 0;
    return race_p9_runtime_state_valid();
}

static int race_p9_rebind_loaded_world(const Sc64RaceMap *map)
{
    race_p9_product_slot = -1;
    if (!race_p9_active()) return 1;
    /* As in P7: zero is the saved, not-yet-started service clock. */
    sc64_p7_arbiter_cache_due = 0;
    sc64_p7_arbiter_cache_initialized =
        race_p9.service_clock[1] || race_p9.service_clock[2];
    return race_p9_runtime_state_valid() && race_p9_bind(map) &&
           race_p7_battery_live_world_valid();
}

/* Rescue (P4/P6 contact rule). Native rescue gives the actors to the
 * rescuing player, so they take the human source slot: the source counts
 * "current player" Zeratul at each Cerebrate. */
static void race_p9_rescue_tick(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == RESCUABLE) {
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
            u[j].raw_owner == SC64_P9_HUMAN_SLOT) {
            int dx = u[i].x - u[j].x, dy = u[i].y - u[j].y;
            if (dx * dx + dy * dy <= 32 * 32) {
                u[i].owner = SIDE; u[i].raw_owner = SC64_P9_HUMAN_SLOT;
                u[i].selected = 0; u[i].target = -1;
                u[i].order = ORDER_IDLE; u[i].tx = u[i].x; u[i].ty = u[i].y;
                break;
            }
        }
    }
}

static int race_p9_runtime_tick(u32 elapsed_updates)
{
    if (!race_p9_active()) return 1;
    if (!race_p9_map || !race_p9_runtime_state_valid()) return 0;
    for (unsigned r = 0; r < SC64_P9_RUNNER_COUNT; ++r) {
        u8 bit = (u8)(1u << r);
        if (!(race_p9.waiting_mask & bit)) continue;
        if (race_p9.wait_ms[r] > SC64_P9_FRAME_MS)
            race_p9.wait_ms[r] = (u16)(race_p9.wait_ms[r] - SC64_P9_FRAME_MS);
        else { race_p9.wait_ms[r] = 0; race_p9.waiting_mask &= (u8)~bit; }
    }
    if (race_p9.pass_timer) --race_p9.pass_timer;
    if (!race_p9.pass_timer) {
        race_p9.pass_timer = SC64_P9_PASS_UPDATES;
        for (unsigned r = 0; r < SC64_P9_RUNNER_COUNT; ++r)
            if (!race_p9_runner_step(r)) return 0;
    }
    race_p9_rescue_tick();
    sc64_p9_ai_tick(&race_p9, elapsed_updates);
    return race_p9_runtime_state_valid();
}

static int race_p9_outcome(void)
{ return race_p9.runner[SC64_P9_RUNNER_HUMAN].outcome; }

#endif /* SC64_P9_RUNTIME */
#endif /* SC64_P9_RUNTIME_ADAPTER_H */
