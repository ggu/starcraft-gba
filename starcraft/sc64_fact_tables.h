/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_FACT_TABLES_H
#define SC64_FACT_TABLES_H

#if !defined(SC64_RACES) || !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "SC64_FACT_TABLES requires the Z/P campaign actor facts"
#endif
#include "sc64_fact_table_format.h"
#include "sc64_fact_tables_generated.h"
#if defined(SC64_SHARED_DEFILER_STAGE) && !defined(SC64_FACT_GENERATED_DEFILER_GEOMETRY)
#error "Defiler stage requires regenerated geometry and sight facts"
#endif
#if defined(SC64_SHARED_DEFILER_HERO_PROBE) && !defined(SC64_FACT_GENERATED_UNCLEAN_ONE)
#error "Unclean One probe requires facts generated for its added identity"
#endif

/* Older unversioned generated headers are the original V1 actor/collision
 * format. Continue accepting them for FACT_TABLES-only builds. Features need
 * a new generated header and its feature bit. */
#ifndef SC64_FACT_TABLE_FORMAT_VERSION
#if defined(SC64_FACT_SIGHT_TABLES) || defined(SC64_P7_RECT_TABLES)
#error "feature tables require a versioned generated fact header"
#else
#define SC64_FACT_TABLE_FORMAT_VERSION SC64_FACT_TABLE_FORMAT_V1
#endif
#endif
#ifndef SC64_FACT_GENERATED_FEATURES
#define SC64_FACT_GENERATED_FEATURES 0u
#endif
#if SC64_FACT_TABLE_FORMAT_VERSION != SC64_FACT_TABLE_FORMAT_V1 && \
    SC64_FACT_TABLE_FORMAT_VERSION != SC64_FACT_TABLE_FORMAT_V2
#error "unsupported generated fact table format version"
#endif
#if SC64_FACT_TABLE_FORMAT_VERSION == SC64_FACT_TABLE_FORMAT_V1 && \
    SC64_FACT_GENERATED_FEATURES != 0u
#error "V1 generated fact tables cannot carry feature payloads"
#endif
#ifdef SC64_FACT_SIGHT_TABLES
#if SC64_FACT_TABLE_FORMAT_VERSION != SC64_FACT_TABLE_FORMAT_V2 || \
    !(SC64_FACT_GENERATED_FEATURES & SC64_FACT_FEATURE_SIGHT)
#error "sight lookup requires V2 with the sight feature"
#endif
#endif
#ifdef SC64_P7_RECT_TABLES
#if !defined(SC64_P7_ACTORS)
#error "SC64_P7_RECT_TABLES requires SC64_P7_ACTORS"
#endif
#if SC64_FACT_TABLE_FORMAT_VERSION != SC64_FACT_TABLE_FORMAT_V2 || \
    !(SC64_FACT_GENERATED_FEATURES & SC64_FACT_FEATURE_P7_RECT)
#error "rectangle lookup requires V2 with the P7 rectangle feature"
#endif
#if !defined(SC64_FACT_P7_RECT_REVISION) || \
    SC64_FACT_P7_RECT_REVISION != SC64_FACT_P7_RECT_REVISION_CURRENT
#error "rectangle tables require current geometry revision; regenerate fact tables"
#endif
#endif

#undef sc64_game_unit_actor_flags
#undef sc64_game_building_actor_flags
#undef sc64_game_unit_collision_radius
#undef sc64_race_unit_flying

static inline unsigned sc64_game_unit_actor_flags(int type)
{
    if ((unsigned)type < SC64_FACT_UNIT_COUNT)
        return sc64_fact_unit_table[type] & SC64_FACT_UNIT_FLAGS_MASK;
    return sc64_game_unit_actor_flags_reference(type);
}
static inline unsigned sc64_game_building_actor_flags(int type)
{
    if ((unsigned)type < SC64_FACT_BUILDING_COUNT)
        return sc64_fact_building_table[type] & SC64_FACT_BUILDING_FLAGS_MASK;
    return sc64_game_building_actor_flags_reference(type);
}
static inline int sc64_game_unit_collision_radius(int type)
{
    if ((unsigned)type < SC64_FACT_UNIT_COUNT)
        return (int)((sc64_fact_unit_table[type] >> SC64_FACT_UNIT_RADIUS_SHIFT) &
                     SC64_FACT_UNIT_RADIUS_MASK);
    return sc64_game_unit_collision_radius_reference(type);
}
static inline int sc64_race_unit_flying(int type)
{
    if ((unsigned)type < SC64_FACT_UNIT_COUNT)
        return (sc64_fact_unit_table[type] & SC64_FACT_UNIT_FLYING) != 0;
    return sc64_race_unit_flying_reference(type);
}

#ifdef SC64_FACT_SIGHT_TABLES
#include "sc64_p7_sight_policy.h"
static inline int sc64_fact_sight_radius_runtime(int type, int building)
{
    if (building) {
        if ((unsigned)type < SC64_FACT_BUILDING_COUNT)
            return sc64_fact_building_sight_radius_table[type];
        return sc64_p7_sight_radius_policy(
            type,1,sc64_zp_building_sight_source_pixels(type));
    }
    if ((unsigned)type < SC64_FACT_UNIT_COUNT)
        return (int)((sc64_fact_unit_table[type] >>
                      SC64_FACT_UNIT_SIGHT_SHIFT) &
                     SC64_FACT_UNIT_SIGHT_MASK);
    return sc64_p7_sight_radius_policy(
        type,0,sc64_zp_actor_sight_source_pixels(type));
}
#endif

#ifdef SC64_P7_RECT_TABLES
static inline int sc64_fact_p7_rect_extents(unsigned raw,int building,
                                             int *left,int *up,
                                             int *right,int *down)
{
    unsigned word;
    if (!left || !up || !right || !down || raw >= SC64_FACT_RECT_RAW_COUNT)
        return 0;
    word=building ? sc64_fact_p7_building_rect_table[raw] :
                    sc64_fact_p7_unit_rect_table[raw];
    if (!(word & SC64_FACT_RECT_VALID)) return 0;
    *left=(int)(word & SC64_FACT_RECT_FIELD_MASK);
    *up=(int)((word >> SC64_FACT_RECT_UP_SHIFT) & SC64_FACT_RECT_FIELD_MASK);
    *right=(int)((word >> SC64_FACT_RECT_RIGHT_SHIFT) & SC64_FACT_RECT_FIELD_MASK);
    *down=(int)((word >> SC64_FACT_RECT_DOWN_SHIFT) & SC64_FACT_RECT_FIELD_MASK);
    return 1;
}
#endif

#endif
