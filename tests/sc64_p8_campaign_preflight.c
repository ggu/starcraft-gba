/* SPDX-License-Identifier: GPL-3.0-or-later */
/* P8 prepared-map actor and bounded creation preflight; no campaign wiring. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/game.c"
#include "../starcraft/sc64_p8_preflight.h"

static Sc64ImportedUnit synthetic_rows[SC64_P8_EXPECTED_MAP_COLS * SC64_P8_EXPECTED_MAP_ROWS];
static Sc64P8SourceAction synthetic_actions[SC64_P8_SOURCE_ACTION_COUNT];

typedef struct { int allow_raw_28; } ResolverContext;

static unsigned resolve_actor(void *context, unsigned raw)
{
    ResolverContext *r = context;
    if (r && r->allow_raw_28 && raw == 28u) return SC64_P8_ACTOR_UNIT;
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

static Sc64P8PreflightResolvers resolvers_for(ResolverContext *context)
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
    ResolverContext source_resolver = {0};
    Sc64P8PreflightResolvers source_ops = resolvers_for(&source_resolver);
    Sc64P8PreflightReport report;
    int result = sc64_p8_preflight(map,
        sc64_p8_source_actions, SC64_P8_SOURCE_ACTION_COUNT,
        MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, MAX_RACE_GEYSERS,
        &source_ops, &report);

    /* The actual prepared map is deliberately blocked by one or more source
     * rows with no current canonical actor binding. Its resolvable source
     * rows and creation plan still undergo the same capacity/property checks. */
    assert(result == SC64_P8_PREFLIGHT_UNSUPPORTED_ACTOR);
    assert(report.total_rows_seen == map->unit_count &&
           report.units + report.buildings + report.minerals +
               report.geysers + report.unresolved_actors == map->unit_count);
    assert(report.create_actions == 24u && report.created_units > 0u &&
           report.created_buildings > 0u);
    assert(report.units + report.created_units <= MAX_UNITS &&
           report.buildings + report.created_buildings <= MAX_BUILDINGS);
    assert(report.unresolved_actors > 0u);
    /* The unresolved Hero Hyperion source actor also lacks a sprite frame. */
    assert(!sc64_race_unit_sprite((unsigned short)report.first_unresolved_raw));

    /* Synthetic resolver fact isolates the rest of the preflight and lets
     * negative fixtures prove unknown actor, selector and capacity rejection. */
    ResolverContext complete_resolver = {1};
    Sc64P8PreflightResolvers complete_ops = resolvers_for(&complete_resolver);
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
    puts("sc64_p8_campaign_preflight: source census and fail-closed actor/property/capacity checks pass");
    return 0;
}
