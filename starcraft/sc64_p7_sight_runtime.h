/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P7_SIGHT_RUNTIME_H
#define SC64_P7_SIGHT_RUNTIME_H

#if defined(SC64_FACT_SIGHT_TABLES) && !defined(SC64_FACT_TABLES)
#error "SC64_FACT_SIGHT_TABLES requires SC64_FACT_TABLES"
#endif

#include "sc64_p7_sight_policy.h"
#ifdef SC64_SOURCE_SIGHT_ALL_RACES
#include "sc64_source_sight_tiles.h"
#endif

/* Runtime optical radius lookup shared by the P7 adapter and its focused
 * host fixture. Dependencies (race rules, source providers, and, when
 * enabled, the V2 getter) are declared by the including translation unit. */
static inline int sc64_p7_observer_sight_radius_runtime(int type, int building)
{
#ifdef SC64_SOURCE_SIGHT_ALL_RACES
    if (building && type < 0 && type >= -SC_RB_COUNT) return 64;
    int special = building ? sc64_zp_building_sight_source_pixels(type) :
                             sc64_zp_actor_sight_source_pixels(type);
    if (special > 0) return special / 2;
    int raw = building ? sc64_race_raw_building(type) : sc64_race_raw_unit(type);
    int source = sc64_source_sight_raw_pixels(raw);
    if (source > 0) return source / 2;
#endif
#ifdef SC64_FACT_SIGHT_TABLES
    return sc64_fact_sight_radius_runtime(type, building);
#else
    int source_pixels = building ?
        sc64_zp_building_sight_source_pixels(type) :
        sc64_zp_actor_sight_source_pixels(type);
    return sc64_p7_sight_radius_policy(type, building, source_pixels);
#endif
}

#endif
