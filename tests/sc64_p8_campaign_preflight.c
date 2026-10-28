/* SPDX-License-Identifier: GPL-3.0-or-later */
/* P8 prepared-map actor and bounded creation preflight; no campaign wiring. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/game.c"
#include "../starcraft/sc64_p8_preflight.h"

static Sc64ImportedUnit synthetic_rows[SC64_P8_EXPECTED_MAP_COLS * SC64_P8_EXPECTED_MAP_ROWS];
static Sc64P8SourceAction synthetic_actions[SC64_P8_SOURCE_ACTION_COUNT];

static unsigned resolve_actor(void *context, unsigned raw)
{
    (void)context;
    if (sc64_race_unit_from_raw((int)raw) >= 0) return SC64_P8_ACTOR_UNIT;
    if (sc64_race_building_from_raw((int)raw) >= 0)
        return SC64_P8_ACTOR_BUILDING;
    return 0;
}

static int resolve_property(void *context, unsigned selector)
{
    (void)context;
    const u8 *record = 0;
    return sc64_p8_source_property(selector, &record) && record;
}

static Sc64P8PreflightResolvers resolvers_for(void *context)
{
    Sc64P8PreflightResolvers out = {context, resolve_actor, resolve_property};
    return out;
}

int main(void)
{
    assert(SC64_RACE_MAP_COUNT > SC64_P8_EXPECTED_GLOBAL_ID);
    const Sc64RaceMap *map = &sc64_race_maps[SC64_P8_EXPECTED_GLOBAL_ID];
    assert(map->width == SC64_P8_EXPECTED_MAP_COLS &&
           map->height == SC64_P8_EXPECTED_MAP_ROWS &&
           map->unit_count > 0u && map->units &&
           !strcmp(map->mission_id, "P8"));
    /* The fixed P8 create plan runs once per action, not once per active
     * member of a multi-player force. Guard that source-data assumption
     * before treating the sum of create counts as a capacity bound. FORC
     * stores zero-based groups; the runtime selector takes groups 1..4. */
    for (unsigned i = 0; i < SC64_P8_SOURCE_ACTION_COUNT; ++i) {
        const Sc64P8SourceAction *action = &sc64_p8_source_actions[i];
        if (action->opcode != 11u || !action->group1) continue;
        Sc64P8TriggerInfo info = {0};
        assert(sc64_p8_source_trigger_info(0, action->trigger, &info));
        unsigned executors = 0;
        for (unsigned owner = 0; owner < 8u; ++owner) {
            unsigned mode = sc64_p8_source_owners[owner];
            if (mode != 5u && mode != 6u) continue; /* Computer and human. */
            assert(sc64_p8_source_forces[owner] < 4u);
            int enabled = sc64_p8_runtime_trigger_enabled(
                &info, owner, sc64_p8_source_forces[owner] + 1u);
            assert(enabled >= 0);
            executors += enabled != 0;
        }
        assert(executors == 1u);
    }
    Sc64P8PreflightResolvers source_ops = resolvers_for(0);
    Sc64P8PreflightReport report;
    int result = sc64_p8_preflight(map,
        sc64_p8_source_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, MAX_RACE_GEYSERS,
        &source_ops, &report);

    /* Raw28 now resolves through its P8-only logical identity and the exact
     * shared native art chain. Census success does not enable the mission. */
    assert(result == SC64_P8_PREFLIGHT_OK);
    assert(report.total_rows_seen == map->unit_count &&
           report.units + report.buildings + report.minerals +
               report.geysers + report.unresolved_actors == map->unit_count);
    assert(report.create_actions == 24u && report.created_units > 0u &&
           report.created_buildings > 0u);
    assert(report.units + report.created_units <= MAX_UNITS &&
           report.buildings + report.created_buildings <= MAX_BUILDINGS);
    assert(report.unresolved_actors == 0u);
    unsigned hyperion_rows = 0;
    for (unsigned i = 0; i < map->unit_count; ++i)
        hyperion_rows += map->units[i].source_type == 28u;
    assert(hyperion_rows > 0u);
    assert(sc64_race_unit_from_raw(28) == U_P8_HYPERION);
    assert(sc64_race_raw_unit(U_P8_HYPERION) == 28);
    const Sc64ZpActorFact *hyperion = sc64_zp_actor_fact(U_P8_HYPERION);
    assert(hyperion && hyperion->raw == 28 && hyperion->hp == 850 &&
           hyperion->armor == 4 && hyperion->size == 2 &&
           hyperion->ground_damage == 30 &&
           hyperion->air_damage == 30 && hyperion->ground_cooldown == 22 &&
           hyperion->air_cooldown == 22 && hyperion->left == 37 &&
           hyperion->up == 29 && hyperion->right == 37 && hyperion->down == 29);
    assert(sc64_p8_unit_sprite_for_raw(28) == sc64_race_unit_sprite(12));
    assert(sc64_p8_unit_sprite_for_raw(28));
    Sc64StockAttack hyperion_ground = {0}, hyperion_air = {0};
    assert(sc64_stock_attack_select(28, 0, &hyperion_ground));
    assert(sc64_stock_attack_select(28, 1, &hyperion_air));
    assert(hyperion_ground.weapon_id == 23 && hyperion_air.weapon_id == 24);

    /* Reuse the real resolver for negative fixtures covering unknown actor,
     * selector and capacity rejection. */
    Sc64P8PreflightResolvers complete_ops = resolvers_for(0);
    int complete_result = sc64_p8_preflight(map,
        sc64_p8_source_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, MAX_RACE_GEYSERS,
        &complete_ops, &report);
    assert(complete_result == SC64_P8_PREFLIGHT_OK);
    const Sc64P8PreflightReport positive = report;

    assert(map->unit_count <= sizeof(synthetic_rows) / sizeof(synthetic_rows[0]));
    memcpy(synthetic_rows, map->units,
           map->unit_count * sizeof(synthetic_rows[0]));
    unsigned unknown_actor_row = 0;
    while (unknown_actor_row < map->unit_count &&
           synthetic_rows[unknown_actor_row].owner >= 8u)
        ++unknown_actor_row;
    assert(unknown_actor_row < map->unit_count);
    synthetic_rows[unknown_actor_row].source_type = 250u;
    Sc64RaceMap synthetic_map = *map;
    synthetic_map.units = synthetic_rows;
    assert(sc64_p8_preflight(&synthetic_map,
        sc64_p8_source_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, MAX_RACE_GEYSERS,
        &complete_ops, &report) == SC64_P8_PREFLIGHT_UNSUPPORTED_ACTOR);
    assert(report.first_unresolved_raw == 250u);

    memcpy(synthetic_rows, map->units,
           map->unit_count * sizeof(synthetic_rows[0]));
    synthetic_rows[unknown_actor_row].x_px = (u16)(map->width * 16u);
    assert(sc64_p8_preflight(&synthetic_map,
        sc64_p8_source_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, MAX_RACE_GEYSERS,
        &complete_ops, &report) == SC64_P8_PREFLIGHT_BAD_INPUT);

    memcpy(synthetic_actions, sc64_p8_source_actions,
           sizeof(synthetic_actions));
    unsigned property_action = SC64_P8_SOURCE_ACTION_COUNT;
    for (unsigned i = 0; i < SC64_P8_SOURCE_ACTION_COUNT; ++i)
        if (synthetic_actions[i].opcode == 11u &&
            synthetic_actions[i].group2) { property_action = i; break; }
    assert(property_action < SC64_P8_SOURCE_ACTION_COUNT);
    synthetic_actions[property_action].group2 = 64u;
    assert(sc64_p8_preflight(map,
        synthetic_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, MAX_RACE_GEYSERS,
        &complete_ops, &report) == SC64_P8_PREFLIGHT_BAD_PROPERTY);

    memcpy(synthetic_actions, sc64_p8_source_actions,
           sizeof(synthetic_actions));
    unsigned create_action = SC64_P8_SOURCE_ACTION_COUNT;
    for (unsigned i = 0; i < SC64_P8_SOURCE_ACTION_COUNT; ++i)
        if (synthetic_actions[i].opcode == 11u &&
            synthetic_actions[i].group1) { create_action = i; break; }
    assert(create_action < SC64_P8_SOURCE_ACTION_COUNT);
    synthetic_actions[create_action].trigger = SC64_P8_TRIGGER_LIMIT;
    assert(sc64_p8_preflight(map,
        synthetic_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, MAX_RACE_GEYSERS,
        &complete_ops, &report) == SC64_P8_PREFLIGHT_BAD_CREATE);

    memcpy(synthetic_actions, sc64_p8_source_actions,
           sizeof(synthetic_actions));
    synthetic_actions[create_action].unit = 250u;
    assert(sc64_p8_preflight(map,
        synthetic_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, MAX_RACE_GEYSERS,
        &complete_ops, &report) == SC64_P8_PREFLIGHT_UNSUPPORTED_ACTOR);
    assert(report.first_unresolved_raw == 250u);

    assert(sc64_p8_preflight(map,
        sc64_p8_source_actions, SC64_P8_SOURCE_ACTION_COUNT,
        positive.units + positive.created_units - 1u, MAX_BUILDINGS,
        MAX_MINERALS, MAX_RACE_GEYSERS,
        &complete_ops, &report) == SC64_P8_PREFLIGHT_CAPACITY);
    assert(sc64_p8_preflight(map,
        sc64_p8_source_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, positive.buildings + positive.created_buildings - 1u,
        MAX_MINERALS, MAX_RACE_GEYSERS,
        &complete_ops, &report) == SC64_P8_PREFLIGHT_CAPACITY);
    assert(sc64_p8_preflight(map,
        sc64_p8_source_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, MAX_BUILDINGS, positive.minerals - 1u,
        MAX_RACE_GEYSERS,
        &complete_ops, &report) == SC64_P8_PREFLIGHT_CAPACITY);
    assert(sc64_p8_preflight(map,
        sc64_p8_source_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS,
        positive.geysers - 1u,
        &complete_ops, &report) == SC64_P8_PREFLIGHT_CAPACITY);
    static const char *const invalid_labels[] = {"", "P", "P8X", "Z8"};
    synthetic_map = *map;
    for (unsigned i = 0; i < sizeof(invalid_labels) / sizeof(*invalid_labels); ++i) {
        synthetic_map.mission_id = invalid_labels[i];
        assert(sc64_p8_preflight(&synthetic_map,
            sc64_p8_source_actions, SC64_P8_SOURCE_ACTION_COUNT,
            MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, MAX_RACE_GEYSERS,
            &complete_ops, &report) == SC64_P8_PREFLIGHT_BAD_INPUT);
    }
    puts("sc64_p8_campaign_preflight: source census and fail-closed actor/property/capacity checks pass");
    return 0;
}
