/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_PREFLIGHT_H
#define SC64_P8_PREFLIGHT_H

#include "sc64_p8_source_adapter.h"
#include "sc64_types.h"

enum {
    SC64_P8_PREFLIGHT_OK = 0,
    SC64_P8_PREFLIGHT_BAD_INPUT = 1,
    SC64_P8_PREFLIGHT_UNSUPPORTED_ACTOR = 2,
    SC64_P8_PREFLIGHT_BAD_PROPERTY = 3,
    SC64_P8_PREFLIGHT_BAD_CREATE = 4,
    SC64_P8_PREFLIGHT_CAPACITY = 5
};

enum {
    SC64_P8_ACTOR_UNIT = 1,
    SC64_P8_ACTOR_BUILDING = 2
};

typedef struct {
    unsigned units, buildings, minerals, geysers;
    unsigned create_actions, created_units, created_buildings;
    unsigned unresolved_actors, first_unresolved_raw;
    unsigned total_rows_seen;
} Sc64P8PreflightReport;

typedef struct {
    void *context;
    /* Return SC64_P8_ACTOR_UNIT/BUILDING, or zero for an unsupported source
     * type. Resources are classified by their source raw IDs in this helper. */
    unsigned (*actor_kind)(void *context, unsigned source_type);
    int (*property)(void *context, unsigned selector);
} Sc64P8PreflightResolvers;

/* Census the generated P8 map shape and its one-shot CreateUnitWithProperties
 * plan against actual runtime pool limits. The runtime caller must authenticate
 * the map pointer before calling this census helper. The source-contract test
 * verifies each create has one eligible executor in this fixed descriptor;
 * this is not a general multi-owner creation planner. It does not bind mission
 * actions or enable P8 campaign runtime. */
static inline int
sc64_p8_preflight(const Sc64RaceMap *map,
                  const Sc64P8SourceAction *actions, unsigned action_count,
                  unsigned unit_capacity, unsigned building_capacity,
                  unsigned mineral_capacity, unsigned geyser_capacity,
                  const Sc64P8PreflightResolvers *resolvers,
                  Sc64P8PreflightReport *report)
{
    Sc64P8PreflightReport out = {0};
    int error = SC64_P8_PREFLIGHT_OK;
    if (report) *report = out;
    if (!map || SC64_P8_EXPECTED_GLOBAL_ID >= SC64_RACE_MAP_COUNT ||
        map->width != SC64_P8_EXPECTED_MAP_COLS ||
        map->height != SC64_P8_EXPECTED_MAP_ROWS ||
        !map->units || !map->unit_count || !map->owner_modes ||
        !map->owner_races || !map->mission_id ||
        map->mission_id[0] != 'P' || map->mission_id[1] != '8' ||
        map->mission_id[2] != '\0' ||
        map->unit_count > MAX_UNITS + MAX_BUILDINGS + MAX_MINERALS +
                              MAX_RACE_GEYSERS ||
        !actions ||
        action_count != SC64_P8_EXPECTED_ACTION_COUNT ||
        !resolvers || !resolvers->actor_kind || !resolvers->property ||
        !report || !sc64_p8_source_metadata_valid())
        return SC64_P8_PREFLIGHT_BAD_INPUT;

    for (unsigned i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *row = &map->units[i];
        unsigned raw = row->source_type;
        if (row->x_px >= map->width * 16u ||
            row->y_px >= map->height * 16u ||
            row->hp_percent > 100u || row->owner >= 12u ||
            ((raw >= 176u && raw <= 178u) || raw == 188u ?
                row->owner != 11u : row->owner >= 8u)) {
            if (error == SC64_P8_PREFLIGHT_OK)
                error = SC64_P8_PREFLIGHT_BAD_INPUT;
        }
        ++out.total_rows_seen;
        if (raw == 176u || raw == 177u || raw == 178u) {
            ++out.minerals;
            continue;
        }
        if (raw == 188u) {
            ++out.geysers;
            continue;
        }
        unsigned kind = resolvers->actor_kind(resolvers->context, raw);
        if (kind == SC64_P8_ACTOR_UNIT) ++out.units;
        else if (kind == SC64_P8_ACTOR_BUILDING) ++out.buildings;
        else {
            if (!out.unresolved_actors) out.first_unresolved_raw = raw;
            ++out.unresolved_actors;
            if (error == SC64_P8_PREFLIGHT_OK)
                error = SC64_P8_PREFLIGHT_UNSUPPORTED_ACTOR;
        }
    }

    /* Every row must resolve to exactly one supported actor/resource class. */
    if (out.units + out.buildings + out.minerals + out.geysers +
            out.unresolved_actors != map->unit_count &&
        error == SC64_P8_PREFLIGHT_OK)
        error = SC64_P8_PREFLIGHT_BAD_INPUT;

    for (unsigned i = 0; i < action_count; ++i) {
        const Sc64P8SourceAction *a = &actions[i];
        if (a->opcode != 11u) continue;
        ++out.create_actions;
        /* A zero-count create is a valid source no-op, not an invalid actor
         * request. Keep it in the action census but reserve no pool entries. */
        if (!a->group1) continue;
        if (a->trigger >= SC64_P8_TRIGGER_LIMIT ||
            sc64_p8_source_triggers[a->trigger].execution_flags != 0u) {
            if (error == SC64_P8_PREFLIGHT_OK)
                error = SC64_P8_PREFLIGHT_BAD_CREATE;
            continue;
        }
        if (a->group2 && !resolvers->property(resolvers->context, a->group2)) {
            if (error == SC64_P8_PREFLIGHT_OK)
                error = SC64_P8_PREFLIGHT_BAD_PROPERTY;
            continue;
        }
        unsigned kind = resolvers->actor_kind(resolvers->context, a->unit);
        if (kind == SC64_P8_ACTOR_UNIT) out.created_units += a->group1;
        else if (kind == SC64_P8_ACTOR_BUILDING)
            out.created_buildings += a->group1;
        else {
            if (!out.unresolved_actors) out.first_unresolved_raw = a->unit;
            ++out.unresolved_actors;
            if (error == SC64_P8_PREFLIGHT_OK)
                error = SC64_P8_PREFLIGHT_UNSUPPORTED_ACTOR;
        }
    }

    if (out.create_actions != 24u ||
        out.units + out.created_units > unit_capacity ||
        out.buildings + out.created_buildings > building_capacity ||
        out.minerals > mineral_capacity || out.geysers > geyser_capacity) {
        if (error == SC64_P8_PREFLIGHT_OK)
            error = SC64_P8_PREFLIGHT_CAPACITY;
    }
    *report = out;
    return error;
}

#endif /* SC64_P8_PREFLIGHT_H */
