/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_FACT_TABLES_H
#define SC64_FACT_TABLES_H

#if !defined(SC64_RACES) || !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "SC64_FACT_TABLES requires the Z/P campaign actor facts"
#endif

/* Default-off build-time fact tables. tools/sc64_fact_tables_gen.c evaluates
 * the *_reference functions for every in-range type under this build's actor
 * gates and writes sc64_fact_tables_generated.h into an ignored build path;
 * that header rejects a build whose gates differ. Hot paths then read one ROM
 * word per type instead of walking the gate-specific fact switches. Types
 * outside the enum keep the reference answer. */
#include "sc64_fact_table_format.h"
#include "sc64_fact_tables_generated.h"

#undef sc64_game_unit_actor_flags
#undef sc64_game_building_actor_flags
#undef sc64_game_unit_collision_radius
#undef sc64_race_unit_flying

static inline unsigned
sc64_game_unit_actor_flags(int type)
{
    if ((unsigned)type < SC64_FACT_UNIT_COUNT)
        return sc64_fact_unit_table[type] & SC64_FACT_UNIT_FLAGS_MASK;
    return sc64_game_unit_actor_flags_reference(type);
}

static inline unsigned
sc64_game_building_actor_flags(int type)
{
    if ((unsigned)type < SC64_FACT_BUILDING_COUNT)
        return sc64_fact_building_table[type] & SC64_FACT_BUILDING_FLAGS_MASK;
    return sc64_game_building_actor_flags_reference(type);
}

static inline int
sc64_game_unit_collision_radius(int type)
{
    if ((unsigned)type < SC64_FACT_UNIT_COUNT)
        return (int)((sc64_fact_unit_table[type] >> SC64_FACT_UNIT_RADIUS_SHIFT) &
                     SC64_FACT_UNIT_RADIUS_MASK);
    return sc64_game_unit_collision_radius_reference(type);
}

static inline int
sc64_race_unit_flying(int type)
{
    if ((unsigned)type < SC64_FACT_UNIT_COUNT)
        return (sc64_fact_unit_table[type] & SC64_FACT_UNIT_FLYING) != 0;
    return sc64_race_unit_flying_reference(type);
}

#endif
