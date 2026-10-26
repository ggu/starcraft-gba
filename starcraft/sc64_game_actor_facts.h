/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GAME_ACTOR_FACTS_H
#define SC64_GAME_ACTOR_FACTS_H

/* Game-level actor policy and collision facts. These are pure functions of
 * the actor type under the compile-time actor gates; SC64_FACT_TABLES
 * replaces their hot-path lookups with build-time generated ROM tables and
 * keeps these definitions as the *_reference oracle. */
#ifdef SC64_FACT_TABLES
#define sc64_game_unit_actor_flags sc64_game_unit_actor_flags_reference
#define sc64_game_building_actor_flags sc64_game_building_actor_flags_reference
#define sc64_game_unit_collision_radius sc64_game_unit_collision_radius_reference
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
/* Actor flag values deliberately share a stable bit layout across the T4
 * scenario adapter and the dedicated Z/P source-fact provider. */
enum {
    SC64_GAME_ACTOR_SELECTABLE = 1u, SC64_GAME_ACTOR_MOVABLE = 2u,
    SC64_GAME_ACTOR_COLLIDABLE = 4u, SC64_GAME_ACTOR_DRAWABLE = 8u,
    SC64_GAME_ACTOR_MEN = 16u, SC64_GAME_ACTOR_BUILDING = 32u,
    SC64_GAME_ACTOR_INVINCIBLE = 64u, SC64_GAME_ACTOR_HIDDEN = 128u,
    SC64_GAME_ACTOR_WEAPON = 256u
};
#ifdef SC64_Z6_RUNTIME
/* Share this expanded source-policy lookup in ROM instead of duplicating it
 * across the hot movement/target loops and consuming their stack reserve. */
static unsigned __attribute__((noinline))
#else
static unsigned
#endif
sc64_game_unit_actor_flags(int type)
{
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    unsigned flags = sc64_zp_actor_flags(type);
    if (flags) return flags;
#endif
#ifdef SC64_T4_RUNTIME
    return sc64_race_t4_actor_flags(type);
#else
    return 0;
#endif
}
static unsigned
sc64_game_building_actor_flags(int type)
{
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    unsigned flags = sc64_zp_building_flags(type);
    if (flags) return flags;
#endif
#ifdef SC64_T4_RUNTIME
    return sc64_race_t4_building_flags(type);
#else
    return 0;
#endif
}
static int sc64_game_unit_has_flag(int type, unsigned flag)
{ return (sc64_game_unit_actor_flags(type) & flag) != 0; }
static int sc64_game_building_has_flag(int type, unsigned flag)
{ return (sc64_game_building_actor_flags(type) & flag) != 0; }
static int sc64_game_unit_selectable(int type)
{ unsigned f=sc64_game_unit_actor_flags(type); return !f || (f & SC64_GAME_ACTOR_SELECTABLE); }
static int sc64_game_building_selectable(int type)
{ unsigned f=sc64_game_building_actor_flags(type); return !f || (f & SC64_GAME_ACTOR_SELECTABLE); }
#ifdef SC64_ZP_CAMPAIGN_ACTORS
/* Unit broad-phase overlap used by the simple mover is a radius in GBA
 * pixels. The source box is in StarCraft pixels; half its widest axis gives
 * the actor's half-extent at the renderer's 2:1 scale. Keep the historical
 * six-pixel fallback for all actors without an audited source box. */
static int
sc64_game_unit_collision_radius(int type)
{
    int left, up, right, down;
    if (!sc64_zp_actor_bounds_source(type, &left, &up, &right, &down)) return 6;
    int width = left + right, height = up + down;
    int source_extent = width > height ? width : height;
    int gba_extent = (source_extent + 3) / 4;
    return gba_extent > 6 ? gba_extent : 6;
}
static int
sc64_game_building_bounds_source(int type, int *left, int *up, int *right, int *down)
{
    return sc64_zp_building_bounds_source(type, left, up, right, down);
}
#endif
#endif
#ifdef SC64_FACT_TABLES
#include "sc64_fact_tables.h"
#endif

#endif
