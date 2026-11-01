/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P10_RUNTIME_ADAPTER_H
#define SC64_P10_RUNTIME_ADAPTER_H

#if defined(SC64_P10_RUNTIME)

#if !defined(SC64_P10_DATA_ENABLED) || !defined(SC64_P9_RUNTIME) || \
    !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 2
#error "P10 runtime requires authenticated P10 data on the P1-P9 Protoss profile"
#endif

/* P10 "Eye of the Storm" campaign binding. The authenticated compact source
 * program (10 triggers) runs for each of its three executing source owners,
 * with the runner contract of the P9 adapter. Its start-up trigger creates
 * Raynor's Terran base for the human and the rescuable Hyperion with strict
 * UPRP properties; victory is the Overmind's death, defeat the loss of
 * Zeratul, the Gantrithor or the Hyperion. Census and owner selection reuse
 * the shared Z8 world services over the live pools. Mission strings and
 * sounds stay in the private CHK and are not presented. Included after the
 * P9 runtime adapter. */
#include "sc64_p10_scenario_generated.h"
#include "sc64_p10_state.h"

enum {
    SC64_P10_MAP_UNITS = 341,
    SC64_P10_HUMAN_SLOT = 1,
    SC64_P10_RESCUABLE_SLOT = 2,
    SC64_P10_FRAME_MS = 42,   /* Same source clock as P8 and P9. */
    SC64_P10_PASS_UPDATES = 8, /* Authored trigger-pass cadence. */
    /* Every P10 Transmission plays a sound whose length is not imported;
     * this bounded authored duration stands in for it, as in P8. */
    SC64_P10_TRANSMISSION_MS = 4000,
    SC64_P10_ACTION_PENDING = 0, SC64_P10_ACTION_COMPLETE = 1,
    SC64_P10_ACTION_WON = 2, SC64_P10_ACTION_LOST = 3, SC64_P10_ACTION_BLOCKED = 4
};

_Static_assert(SC64_P10_SOURCE_GLOBAL_ID == 29u && SC64_P10_SOURCE_MAP_COLS == 128u &&
               SC64_P10_SOURCE_MAP_ROWS == 128u &&
               SC64_P10_SOURCE_TRIGGER_COUNT == SC64_P10_TRIGGER_LIMIT &&
               SC64_P10_SOURCE_ACTION_COUNT == 75u &&
               SC64_P10_SOURCE_CONDITION_COUNT == 9u &&
               SC64_P10_SOURCE_LOCATION_COUNT == 64u,
               "P10 source identity mismatch");

static Sc64P10PersistentState race_p10;
static Sc64Z8WorldView race_p10_view;
static Sc64Z8GameContext race_p10_game;
static const Sc64RaceMap *race_p10_map;
static s8 race_p10_product_slot = -1;

/* Live owners by source slot: Zerg brood (0), human (1), the rescuable
 * heroes (2) and the Overmind's brood (3); slots 4-7 are unused. */
static const u8 sc64_p10_owner_modes_expected[8] = { 1, 0, 3, 1, 2, 2, 2, 2 };

static int race_p10_active(void)
{ return !race_mode && race_selected_mission == SC64_P10_GLOBAL_ID; }

static unsigned sc64_p10_le16(const u8 *p)
{ return (unsigned)p[0] | ((unsigned)p[1] << 8); }

/* The 28 used UPRP rows request full HP, plus full energy on the Hyperion's
 * row. No row sets an owner, shields, resources, a hangar or a status bit.
 * This is a strict decoder for those rows, not a general UPRP interpreter. */
static int sc64_p10_property_valid(unsigned selector)
{
    const u8 *p;
    unsigned special, fields;
    if (!selector || selector > 64u || sc64_p10_source_property_used[selector - 1u] != 1u)
        return 0;
    p = sc64_p10_source_properties[selector - 1u];
    special = sc64_p10_le16(p);
    fields = sc64_p10_le16(p + 2);
    if ((special != 0x10u && special != 0x14u && special != 0x18u) ||
        (fields != 0x02u && fields != 0x0au) || (p[4] && p[4] != 0xffu) ||
        p[5] != 100u || p[6] || p[7] != ((fields & 0x08u) ? 100u : 0u))
        return 0;
    for (unsigned i = 8; i < 20u; ++i) if (p[i]) return 0;
    return 1;
}

/* Validate the compact program before any cursor indexes it. Only the
 * opcodes implemented below, switches 0-15 and the executing owners'
 * enabled-player columns are accepted. */
static int sc64_p10_source_valid(void)
{
    unsigned next_action = 0, next_condition = 0;
    for (unsigned t = 0; t < SC64_P10_TRIGGER_LIMIT; ++t) {
        const Sc64P10SourceTrigger *tr = &sc64_p10_source_triggers[t];
        if (tr->first_action != next_action || tr->first_condition != next_condition ||
            tr->execution_flags || tr->initial_cursor ||
            tr->action_count > SC64_P10_ACTION_LIMIT ||
            tr->first_action + tr->action_count > SC64_P10_SOURCE_ACTION_COUNT ||
            tr->first_condition + tr->condition_count > SC64_P10_SOURCE_CONDITION_COUNT)
            return 0;
        for (unsigned p = 0; p < 27u; ++p)
            if (tr->enabled_players[p] > 1u ||
                (tr->enabled_players[p] && p != 0u && p != 1u && p != 3u &&
                 (p < 18u || p > 21u))) return 0;
        for (unsigned i = 0; i < tr->action_count; ++i) {
            const Sc64P10SourceAction *a = &sc64_p10_source_actions[tr->first_action + i];
            if (a->trigger != t || a->index != i || a->location > 64u ||
                a->trailing[0] || a->trailing[1] || a->trailing[2]) return 0;
            switch (a->opcode) {
            case 1: case 2:
                if (i + 1u != tr->action_count) return 0;
                break;
            case 4: if (a->time > 65535u) return 0; break;
            case 7: case 10: if (!a->location) return 0; break;
            case 8: case 9: case 12: case 16: case 26: break;
            case 11:
                if (a->flags != 0x18u || a->modifier || !a->location ||
                    !sc64_p10_property_valid(a->group2)) return 0;
                break;
            case 13: if (a->group2 >= 16u) return 0; break;
            case 38:
                if (a->location != 64u || a->group2 != SC64_P10_MOVED_LOCATION)
                    return 0;
                break;
            default: return 0;
            }
        }
        for (unsigned i = 0; i < tr->condition_count; ++i) {
            const Sc64P10SourceCondition *c =
                &sc64_p10_source_conditions[tr->first_condition + i];
            if (c->trigger != t || c->index != i || c->location > 64u) return 0;
            if (c->opcode == 11u && c->resource >= 16u) return 0;
            if (c->opcode != 2u && c->opcode != 11u && c->opcode != 12u) return 0;
        }
        next_action += tr->action_count;
        next_condition += tr->condition_count;
    }
    if (next_action != SC64_P10_SOURCE_ACTION_COUNT ||
        next_condition != SC64_P10_SOURCE_CONDITION_COUNT) return 0;
    for (unsigned i = 0; i < SC64_P10_SOURCE_LOCATION_COUNT; ++i) {
        const Sc64P10SourceLocation *l = &sc64_p10_source_locations[i];
        if (l->left > l->right || l->top > l->bottom || l->right > 4096u ||
            l->bottom > 4096u) return 0;
    }
    return 1;
}

static int race_p10_map_shape_valid(const Sc64RaceMap *map)
{
    if (!map || SC64_P10_GLOBAL_ID >= SC64_RACE_MAP_COUNT ||
        map != &sc64_race_maps[SC64_P10_GLOBAL_ID] || map->width != 128u ||
        map->height != 128u || map->human_slot != SC64_P10_HUMAN_SLOT ||
        map->unit_count != SC64_P10_MAP_UNITS || !map->owner_modes || !map->units ||
        !map->mission_id || map->mission_id[0] != 'P' || map->mission_id[1] != '1' ||
        map->mission_id[2] != '0' || map->mission_id[3] != '\0' ||
        !sc64_p10_source_valid()) return 0;
    for (unsigned i = 0; i < 8; ++i)
        if (map->owner_modes[i] != sc64_p10_owner_modes_expected[i] ||
            sc64_p10_source_forces[i] > 3u) return 0;
    for (unsigned r = 0; r < SC64_P10_RUNNER_COUNT; ++r)
        if (sc64_p10_source_forces[sc64_p10_runner_slot[r]] + 1u != sc64_p10_runner_force[r])
            return 0;
    return 1;
}

/* From the canonical descriptor, so save candidates can be checked before
 * the world is bound. */
static int race_p10_source_owner(unsigned slot)
{
    const Sc64RaceMap *map;
    if (slot >= 8u || SC64_P10_GLOBAL_ID >= SC64_RACE_MAP_COUNT) return -1;
    map = &sc64_race_maps[SC64_P10_GLOBAL_ID];
    if (map->human_slot != SC64_P10_HUMAN_SLOT || !map->owner_modes) return -1;
    return slot == SC64_P10_HUMAN_SLOT ? SIDE : map->owner_modes[slot];
}

/* P10 runs the shared Protoss unit services too (Zeratul and the Dark
 * Templar are cloaked; Cannons, Overlords and Spore Colonies detect). These
 * dispatchers cover P8, P9 and P10; the P9 adapter's own versions are renamed
 * race_p9_later_* in this build. */
static int race_protoss_later_services_active(void)
{ return race_p10_active() || race_p9_later_services_active(); }
static int race_protoss_later_source_owner(unsigned slot)
{ return race_p10_active() ? race_p10_source_owner(slot) : race_p9_later_source_owner(slot); }
static u8 *race_protoss_later_service_clock(void)
{ return race_p10_active() ? race_p10.service_clock : race_p9_later_service_clock(); }
static int race_protoss_later_service_mission(int mission)
{ return mission == SC64_P10_GLOBAL_ID || race_p9_later_service_mission(mission); }
static int race_protoss_later_mission_source_owner(int mission, unsigned slot)
{
    return mission == SC64_P10_GLOBAL_ID ? race_p10_source_owner(slot) :
        race_p9_later_mission_source_owner(mission, slot);
}

/* The P10 map places no Shield Battery; only human-built ones exist. */
static int race_p10_battery_ok(const Building *q)
{
    return q->raw_type == 172 && q->raw_owner == SC64_P10_HUMAN_SLOT &&
        q->owner == SIDE && !q->queue_count && !q->queue_head &&
        q->queue_progress >= 0 && q->queue_progress <= SC64_P7_BATTERY_MAX_ENERGY_Q8 &&
        q->level >= 0 && q->level < SC64_P7_BATTERY_REGEN_DEN &&
        sc64_p7_battery_energy_valid((unsigned)q->queue_progress, (unsigned)q->level) &&
        q->shield <= sc64_race_building_shields(B_P7_SHIELD_BATTERY);
}

static int race_protoss_later_battery_candidate_valid(int mission, const Building *q)
{
    if (mission == SC64_P10_GLOBAL_ID) return q && race_p10_battery_ok(q);
    return race_p9_later_battery_candidate_valid(mission, q);
}

static int race_protoss_later_battery_world_valid(void)
{
    if (!race_p10_active()) return race_p9_later_battery_world_valid();
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        b[i].type == B_P7_SHIELD_BATTERY &&
        (!sc64_p7_battery_fields_valid(i) || !race_p10_battery_ok(&b[i]))) return 0;
    return 1;
}

static int race_protoss_later_battery_initialize(const Sc64RaceMap *map)
{
    if (!race_p10_active()) return race_p9_later_battery_initialize(map);
    if (map != &sc64_race_maps[SC64_P10_GLOBAL_ID]) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_P7_SHIELD_BATTERY) return 0;
    return 1;
}

static void race_p10_note_unit_spawn(int slot, int owner, int type)
{
    if (!race_p10_active() || slot < 0 || slot >= MAX_UNITS) return;
    u[slot].raw_type = (s16)sc64_race_raw_unit(type);
    u[slot].raw_owner = owner == SIDE ? SC64_P10_HUMAN_SLOT : race_p10_product_slot;
}

static void race_p10_note_building_spawn(int slot, int owner, int type)
{
    if (!race_p10_active() || slot < 0 || slot >= MAX_BUILDINGS) return;
    b[slot].raw_type = (s16)sc64_race_raw_building(type);
    b[slot].raw_owner = owner == SIDE ? SC64_P10_HUMAN_SLOT : -1;
}

static void race_p10_note_product_building(int building)
{
    race_p10_product_slot = race_p10_active() && building >= 0 &&
        building < MAX_BUILDINGS ? (s8)b[building].raw_owner : -1;
}

/* Location 32 may have been moved by MoveLocation; every other location is
 * the authenticated source rectangle. */
static const Sc64Z8Rect *race_p10_rect(unsigned location, Sc64Z8Rect *out)
{
    const Sc64P10SourceLocation *l;
    if (!location || location > SC64_P10_SOURCE_LOCATION_COUNT) return 0;
    if (location == SC64_P10_MOVED_LOCATION && race_p10.moved_location_valid) {
        *out = (Sc64Z8Rect){race_p10.moved_location[0], race_p10.moved_location[1],
                            race_p10.moved_location[2], race_p10.moved_location[3]};
        return out;
    }
    l = &sc64_p10_source_locations[location - 1u];
    *out = (Sc64Z8Rect){(s16)l->left, (s16)l->top, (s16)l->right, (s16)l->bottom};
    return out;
}

static int race_p10_condition(const Sc64P10SourceCondition *c, unsigned owner)
{
    Sc64Z8Condition z = {0};
    u32 count = 0;
    if (c->flags & 2u) return 1;
    switch (c->opcode) {
    case 2:
        z.location = c->location; z.player = c->player; z.amount = c->amount;
        z.unit = c->unit; z.mask = c->mask; z.comparison = c->comparison;
        z.opcode = c->opcode; z.resource = c->resource; z.flags = c->flags;
        z.index = c->index;
        if ((c->flags & ~0x12u) || !sc64_z8_comparison_known(z.opcode, z.comparison))
            return -1;
        if (sc64_z8_world_count(&race_p10_view, &z, owner, 0, &count) != 1) return -1;
        return sc64_z8_compare(z.comparison, count, z.amount);
    case 11: {
        int set = (race_p10.switches >> c->resource) & 1;
        return c->comparison == 2u ? set : c->comparison == 3u ? !set : -1;
    }
    case 12:
        return sc64_z8_compare(c->comparison,
            (u32)(((unsigned long long)race_campaign.elapsed_frames * SC64_P10_FRAME_MS) / 1000u),
            c->amount);
    default: return -1;
    }
}

static int race_p10_owner_match(unsigned selector, unsigned trigger_owner, int slot)
{
    if (slot < 0 || slot >= (int)SC64_Z8_SOURCE_OWNER_LIMIT) return -1;
    return sc64_z8_world_owner_matches(&race_p10_view, selector, trigger_owner,
                                       (unsigned)slot);
}

/* SetResources applies to every active slot the selector names. */
static int race_p10_set_resources(const Sc64P10SourceAction *a, unsigned trigger_owner)
{
    int matched = 0;
    if (a->location || a->unit > SC64_RESOURCE_BOTH ||
        (a->modifier != SC64_RESOURCE_MOD_SET && a->modifier != SC64_RESOURCE_MOD_ADD &&
         a->modifier != SC64_RESOURCE_MOD_SUBTRACT)) return -1;
    for (int slot = 0; slot < 8; ++slot) {
        Sc64ResourceActionState v = {0, 0, 0, 0};
        int w = sc64_p10_wallet_index((unsigned)slot), human = slot == SC64_P10_HUMAN_SLOT;
        int match = race_p10_owner_match(a->group1, trigger_owner, slot);
        if (match < 0) return -1;
        if (!match) continue;
        if (!human && w < 0) return -1;
        v.current_minerals = human ? (u32)minerals : race_p10.wallet_minerals[w];
        v.current_gas = human ? (u32)gas : race_p10.wallet_gas[w];
        if (!sc64_resource_action_apply_one_player(&v, (u16)a->unit, a->modifier,
                                                    a->group2) ||
            v.current_minerals > 65535u || v.current_gas > 65535u) return -1;
        if (human) {
            minerals = race_minerals[SIDE] = (int)v.current_minerals;
            gas = race_gas[SIDE] = (int)v.current_gas;
        } else {
            race_p10.wallet_minerals[w] = (u16)v.current_minerals;
            race_p10.wallet_gas[w] = (u16)v.current_gas;
        }
        ++matched;
    }
    return matched ? SC64_P10_ACTION_COMPLETE : -1;
}

/* CreateUnitWithProperties, one actor (modifier 0 means one), centred on the
 * location and clamped to the map as in the P8 property service. The rows'
 * only effects are full HP and, on the Hyperion, full energy; raw 28 has no
 * energy in this engine, so that request is checked but has no effect.
 * Buildings are created complete. */
static int race_p10_create(const Sc64P10SourceAction *a, unsigned trigger_owner)
{
    Sc64Z8Rect rect;
    unsigned slot;
    int raw = a->unit, building, type, live, left, top, right, bottom, x, y, created;
    if (a->flags != 0x18u || a->modifier || !sc64_p10_property_valid(a->group2) ||
        !race_p10_rect(a->location, &rect) ||
        !sc64_z8_world_resolve_one(&race_p10_view, a->group1, trigger_owner, &slot))
        return -1;
    type = sc64_race_unit_from_raw(raw);
    building = type < 0;
    if (building) type = sc64_race_building_from_raw(raw);
    live = race_p10_source_owner(slot);
    if (type < 0 || live < SIDE || live > 4 ||
        !sc64_z8_game_source_extents((unsigned)raw, 0, 0, building,
                                     &left, &top, &right, &bottom))
        return -1;
    x = ((int)rect.left + (int)rect.right) / 2;
    y = ((int)rect.top + (int)rect.bottom) / 2;
    if (x < left) x = left;
    if (y < top) y = top;
    if (x + right >= 4096) x = 4096 - right - 1;
    if (y + bottom >= 4096) y = 4096 - bottom - 1;
    if (building) {
        if (find_build_slot() < 0) return -1;
        created = spawn_build(live, type, x / 2, y / 2, 0);
        if (created < 0) return -1;
        b[created].raw_type = (s16)raw;
        b[created].raw_owner = (s16)slot;
        b[created].hp = b[created].maxhp;
    } else {
        if (find_unit_slot() < 0) return -1;
        created = spawn_unit(live, type, x / 2, y / 2);
        if (created < 0) return -1;
        u[created].raw_type = (s16)raw;
        u[created].raw_owner = (s16)slot;
        u[created].hp = u[created].maxhp;
    }
    return SC64_P10_ACTION_COMPLETE;
}

/* MoveLocation centres location 32 on the first matching actor anywhere
 * (location 64), in native actor order; with no match it centres on the
 * search area, as in the P8 world adapter. Only presentation reads it. */
static int race_p10_move_location(const Sc64P10SourceAction *a, unsigned trigger_owner)
{
    Sc64Z8Action z = {0};
    Sc64Z8Rect search, moved;
    s16 x, y;
    int found;
    if (a->modifier || !race_p10_rect(a->location, &search) ||
        !race_p10_rect(SC64_P10_MOVED_LOCATION, &moved)) return -1;
    z.location = a->location; z.group1 = a->group1; z.group2 = a->group2;
    z.unit = a->unit; z.opcode = a->opcode; z.flags = a->flags; z.index = a->index;
    found = sc64_z8_world_find_unit(&race_p10_view, &z, trigger_owner, &search, &x, &y);
    if (found < 0) return -1;
    if (!found) {
        x = (s16)(((s32)search.left + search.right) / 2);
        y = (s16)(((s32)search.top + search.bottom) / 2);
    }
    sc64_z8_move_location(&moved, x, y, 4096, 4096);
    race_p10.moved_location[0] = moved.left; race_p10.moved_location[1] = moved.top;
    race_p10.moved_location[2] = moved.right; race_p10.moved_location[3] = moved.bottom;
    race_p10.moved_location_valid = 1;
    return SC64_P10_ACTION_COMPLETE;
}

/* CenterView snaps the camera; the human runner owns every such row. */
static int race_p10_center_view(const Sc64P10SourceAction *a, unsigned trigger_owner)
{
    Sc64Z8Rect rect;
    if (trigger_owner != SC64_P10_HUMAN_SLOT || !race_p10_rect(a->location, &rect))
        return -1;
    cx = limit(((int)rect.left + (int)rect.right) / 4, 120, race_cols * 16 - 120);
    cy = limit(((int)rect.top + (int)rect.bottom) / 4, 54, race_rows * 16 - 54);
    return SC64_P10_ACTION_COMPLETE;
}

#include "sc64_p10_ai.h"

/* Wait and Transmission share one timer per runner, as in the P9 adapter. */
static int race_p10_timed(unsigned runner, u32 duration_ms, int resume)
{
    u8 bit = (u8)(1u << runner);
    if (race_p10.waiting_mask & bit)
        return resume ? SC64_P10_ACTION_PENDING : SC64_P10_ACTION_BLOCKED;
    if (resume) return SC64_P10_ACTION_COMPLETE;
    if (duration_ms > 65535u) return -1;
    race_p10.wait_ms[runner] = (u16)duration_ms;
    race_p10.waiting_mask |= bit;
    return SC64_P10_ACTION_PENDING;
}

static int race_p10_action(unsigned runner, const Sc64P10SourceAction *a, int resume)
{
    unsigned owner = sc64_p10_runner_slot[runner];
    if (a->flags & 2u) return SC64_P10_ACTION_COMPLETE;
    if (resume && a->opcode != 4u && a->opcode != 7u) return -1;
    switch (a->opcode) {
    case 1: return SC64_P10_ACTION_WON;
    case 2: return SC64_P10_ACTION_LOST;
    case 4: return race_p10_timed(runner, a->time, resume);
    case 7:
        if (owner != SC64_P10_HUMAN_SLOT) return -1;
        if (!resume && !(race_p10.waiting_mask & (1u << runner)))
            race_campaign_set_message(0);
        return race_p10_timed(runner, SC64_P10_TRANSMISSION_MS, resume);
    case 8: case 9: case 12: return SC64_P10_ACTION_COMPLETE; /* Not presented. */
    case 10: return race_p10_center_view(a, owner);
    case 11: return race_p10_create(a, owner);
    case 13:
        if (a->modifier == 4u) race_p10.switches |= (u16)(1u << a->group2);
        else if (a->modifier == 5u) race_p10.switches &= (u16)~(1u << a->group2);
        else if (a->modifier == 6u) race_p10.switches ^= (u16)(1u << a->group2);
        else return -1;
        return SC64_P10_ACTION_COMPLETE;
    case 16: return sc64_p10_ai_start(&race_p10, a, owner) ? SC64_P10_ACTION_COMPLETE : -1;
    case 26: return race_p10_set_resources(a, owner);
    case 38: return race_p10_move_location(a, owner);
    default: return -1;
    }
}

static int race_p10_trigger_enabled(const Sc64P10SourceTrigger *tr, unsigned runner)
{
    return tr->enabled_players[sc64_p10_runner_slot[runner]] || tr->enabled_players[17] ||
           tr->enabled_players[17u + sc64_p10_runner_force[runner]];
}

/* One source pass for one runner, following the shared P8/P9 runner's
 * cursor, pending and blocked contract. P10 has no PreserveTrigger. */
static int race_p10_runner_step(unsigned runner)
{
    Sc64P10Runner *r = &race_p10.runner[runner];
    for (unsigned t = 0; t < SC64_P10_TRIGGER_LIMIT; ++t) {
        const Sc64P10SourceTrigger *tr = &sc64_p10_source_triggers[t];
        u16 bit = (u16)(1u << t);
        if (!race_p10_trigger_enabled(tr, runner) || (r->done & bit)) continue;
        if (!(r->running & bit)) {
            int matches = 1;
            for (unsigned c = 0; c < tr->condition_count && matches; ++c) {
                int result = race_p10_condition(
                    &sc64_p10_source_conditions[tr->first_condition + c],
                    sc64_p10_runner_slot[runner]);
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
            int result = race_p10_action(runner,
                &sc64_p10_source_actions[tr->first_action + r->cursor[t]], resume);
            if (result == SC64_P10_ACTION_PENDING) { r->waiting |= bit; break; }
            r->waiting &= (u16)~bit;
            if (result == SC64_P10_ACTION_BLOCKED) break;
            if (result < SC64_P10_ACTION_COMPLETE || result > SC64_P10_ACTION_LOST) return 0;
            if (result == SC64_P10_ACTION_WON) r->outcome = SC64_P10_OUTCOME_WON;
            else if (result == SC64_P10_ACTION_LOST && r->outcome != SC64_P10_OUTCOME_WON)
                r->outcome = SC64_P10_OUTCOME_LOST;
            if (++r->cursor[t] == tr->action_count) {
                r->running &= (u16)~bit;
                r->cursor[t] = 0;
                r->done |= bit;
            }
        }
    }
    return 1;
}

static int race_p10_bind(const Sc64RaceMap *map)
{
    s8 live[SC64_Z8_SOURCE_OWNER_LIMIT];
    if (!race_p10_map_shape_valid(map)) return 0;
    race_p10_map = map;
    for (unsigned slot = 0; slot < SC64_Z8_SOURCE_OWNER_LIMIT; ++slot)
        live[slot] = slot < 8u ? (s8)race_p10_source_owner(slot) : (s8)-1;
    memset(&race_p10_view, 0, sizeof(race_p10_view));
    if (!sc64_z8_game_context_init(&race_p10_game, live, 4096u, 4096u)) return 0;
    sc64_z8_game_bind_actor_pool(&race_p10_view);
    /* Census rows are the OWNR-active players: 0, 1, 2 and 3. */
    for (unsigned slot = 0; slot < 8u; ++slot) {
        if (sc64_p10_source_owners[slot]) race_p10_view.active_mask |= (u8)(1u << slot);
        race_p10_view.force[slot] = (u8)(sc64_p10_source_forces[slot] + 1u);
    }
    return 1;
}

/* Pure candidate check: use the same source-action bounds before committing a
 * save candidate and when validating the live runtime. */
static int sc64_p10_source_state_valid(const Sc64P10PersistentState *state)
{
    if (!sc64_p10_persistent_state_valid(state)) return 0;
    for (unsigned r = 0; r < SC64_P10_RUNNER_COUNT; ++r)
        for (unsigned t = 0; t < SC64_P10_TRIGGER_LIMIT; ++t)
            if ((state->runner[r].running & (1u << t)) &&
                state->runner[r].cursor[t] >= sc64_p10_source_triggers[t].action_count)
                return 0;
    return 1;
}

static int race_p10_runtime_state_valid(void)
{
    return sc64_p10_source_state_valid(&race_p10);
}

static int race_p10_runtime_init(const Sc64RaceMap *map)
{
    memset(&race_p10, 0, sizeof(race_p10));
    race_p10_map = 0;
    race_p10_product_slot = -1;
    if (!race_p10_active()) return 1;
    if (!race_p10_bind(map)) return 0;
    /* Fresh shared-service clock; the first source update seeds the
     * Arbiter/detection cache. */
    sc64_p7_arbiter_cache_due = 0;
    sc64_p7_arbiter_cache_initialized = 0;
    if (!race_p7_battery_initialize_source(map)) return 0;
    return race_p10_runtime_state_valid();
}

static int race_p10_rebind_loaded_world(const Sc64RaceMap *map)
{
    race_p10_product_slot = -1;
    if (!race_p10_active()) return 1;
    /* As in P7: zero is the saved, not-yet-started service clock. */
    sc64_p7_arbiter_cache_due = 0;
    sc64_p7_arbiter_cache_initialized =
        race_p10.service_clock[1] || race_p10.service_clock[2];
    return race_p10_runtime_state_valid() && race_p10_bind(map) &&
           race_p7_battery_live_world_valid();
}

/* Rescue (P4/P6 contact rule). Native rescue gives the actors to the
 * rescuing player, so they take the human source slot, as in P9. */
static void race_p10_rescue_tick(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == RESCUABLE) {
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
            u[j].raw_owner == SC64_P10_HUMAN_SLOT) {
            int dx = u[i].x - u[j].x, dy = u[i].y - u[j].y;
            if (dx * dx + dy * dy <= 32 * 32) {
                u[i].owner = SIDE; u[i].raw_owner = SC64_P10_HUMAN_SLOT;
                u[i].selected = 0; u[i].target = -1;
                u[i].order = ORDER_IDLE; u[i].tx = u[i].x; u[i].ty = u[i].y;
                break;
            }
        }
    }
}

static int race_p10_runtime_tick(u32 elapsed_updates)
{
    if (!race_p10_active()) return 1;
    if (!race_p10_map || !race_p10_runtime_state_valid()) return 0;
    for (unsigned r = 0; r < SC64_P10_RUNNER_COUNT; ++r) {
        u8 bit = (u8)(1u << r);
        if (!(race_p10.waiting_mask & bit)) continue;
        if (race_p10.wait_ms[r] > SC64_P10_FRAME_MS)
            race_p10.wait_ms[r] = (u16)(race_p10.wait_ms[r] - SC64_P10_FRAME_MS);
        else { race_p10.wait_ms[r] = 0; race_p10.waiting_mask &= (u8)~bit; }
    }
    if (race_p10.pass_timer) --race_p10.pass_timer;
    if (!race_p10.pass_timer) {
        race_p10.pass_timer = SC64_P10_PASS_UPDATES;
        for (unsigned r = 0; r < SC64_P10_RUNNER_COUNT; ++r)
            if (!race_p10_runner_step(r)) return 0;
    }
    race_p10_rescue_tick();
    sc64_p10_ai_tick(&race_p10, elapsed_updates);
    return race_p10_runtime_state_valid();
}

static int race_p10_outcome(void)
{ return race_p10.runner[SC64_P10_RUNNER_HUMAN].outcome; }

#endif /* SC64_P10_RUNTIME */
#endif /* SC64_P10_RUNTIME_ADAPTER_H */
