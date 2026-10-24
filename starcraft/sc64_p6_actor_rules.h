#ifndef SC64_P6_ACTOR_RULES_H
#define SC64_P6_ACTOR_RULES_H

/* P6-only source facts absent from the existing shared actor tables.
 * Numeric source values are transcribed from BWAPI UnitType.cpp and
 * WeaponType.cpp at d727fed68558c506163048ea889131d8cbb33915. Raw 1,
 * raw 195, and doors 205..208 deliberately reuse sc64_z5_actor_rules.h;
 * ordinary units and raw 196 reuse race_rules.h / sc64_zp_actor_rules.h.
 * These facts do not imply GBA rendering, collision, cloak, or combat policy.
 */
#include "sc64_z5_actor_rules.h"

enum {
    SC64_P6_SOURCE_ORGANIC          = 1u << 0,
    SC64_P6_SOURCE_REGENERATES_HP  = 1u << 1,
    SC64_P6_SOURCE_BURROWABLE      = 1u << 2,
    SC64_P6_SOURCE_PERMANENT_CLOAK = 1u << 3,
    SC64_P6_SOURCE_HERO            = 1u << 4,
    SC64_P6_SOURCE_AUTO_ATTACK     = 1u << 5,
    SC64_P6_SOURCE_ATTACK          = 1u << 6,
    SC64_P6_SOURCE_DETECTOR        = 1u << 7,
    SC64_P6_SOURCE_HUGE_UNIT       = 1u << 8,
    SC64_P6_SOURCE_MECHANICAL      = 1u << 9
};

enum {
    SC64_P6_WEAPON_DAMAGE_NORMAL = 0,
    SC64_P6_WEAPON_DAMAGE_CONCUSSIVE = 1,
    SC64_P6_WEAPON_DAMAGE_EXPLOSIVE = 2,
    SC64_P6_WEAPON_DAMAGE_INDEPENDENT = 3,
    SC64_P6_WEAPON_TARGET_AIR = 1u << 0,
    SC64_P6_WEAPON_TARGET_GROUND = 1u << 1,
    SC64_P6_WEAPON_RADIAL_SPLASH = 1u << 2
};

enum {
    SC64_P6_RAW_GHOST = 1,
    SC64_P6_RAW_INFESTED_TERRAN = 50,
    SC64_P6_RAW_HERO_DARK_TEMPLAR = 74,
    SC64_P6_RAW_ZERATUL = 75,
    SC64_P6_RAW_WALL_MISSILE_TRAP = 210,
    SC64_P6_RAW_TERRAN_BEACON = 195,
    SC64_P6_RAW_DOOR_FIRST = 205,
    SC64_P6_RAW_DOOR_LAST = 208,
    SC64_P6_RAW_PROTOSS_BEACON = 196
};

typedef struct {
    int raw, hp, shields, armor, minerals, gas, build_frames, sight_tiles;
    int tile_width, tile_height, left, up, right, down;
    int flying, size;
    int ground_weapon, ground_damage, ground_bonus, ground_cooldown;
    int ground_range, ground_hits, ground_damage_type, ground_target_flags;
    int ground_splash_inner, ground_splash_middle, ground_splash_outer;
    int air_weapon, air_damage, air_bonus, air_cooldown, air_range, air_hits;
    unsigned flags;
} Sc64P6AddedActorFact;

/* GBA policy requirements are explicitly separate from source data. A zero
 * support bit means this table records a requirement, not a completed GBA
 * implementation. In particular, neither permanent cloak nor suicide
 * detonation is approximated as an ordinary selectable melee actor here.
 */
enum {
    SC64_P6_POLICY_NEEDS_PERMANENT_CLOAK_AND_DETECTORS = 1u << 0,
    SC64_P6_POLICY_NEEDS_SUICIDE_EXPLOSION = 1u << 1,
    SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY = 1u << 2,
    SC64_P6_POLICY_NEEDS_WALL_TRAP_ART_AND_RUNTIME = 1u << 3
};

typedef struct {
    int raw;
    unsigned required_capabilities;
    unsigned implemented_capabilities;
} Sc64P6ActorPolicyRequirement;

static inline const Sc64P6AddedActorFact *
sc64_p6_added_actor_fact(int raw)
{
    static const Sc64P6AddedActorFact facts[] = {
        /* 210: Special Wall Missile Trap. The pinned source supplies a
         * ground-only Hellfire Missile Pack weapon even though the UnitType
         * lacks Attack and AutoAttackAndMove flags. Preserve that source
         * asymmetry; GBA target selection/attack behavior and prop art remain
         * unimplemented policy, not inferred from the weapon row. */
        {210,50,0,0,0,0,0,6, 1,1,16,16,15,15, 0,2,
         97,10,1,22,160,1,SC64_P6_WEAPON_DAMAGE_EXPLOSIVE,
         SC64_P6_WEAPON_TARGET_GROUND,0,0,0,
         0,0,0,0,0,0,
         SC64_P6_SOURCE_DETECTOR|SC64_P6_SOURCE_HUGE_UNIT|
         SC64_P6_SOURCE_MECHANICAL},
        /* 50: Zerg Infested Terran. Weapon 54 is Suicide_Infested_Terran:
         * 500 Explosive damage, one hit, ground-only, radial splash. */
        {50,60,0,0,100,50,600,5, 1,1,8,9,8,10, 0,0,
         54,500,0,1,3,1,SC64_P6_WEAPON_DAMAGE_EXPLOSIVE,
         SC64_P6_WEAPON_TARGET_GROUND|SC64_P6_WEAPON_RADIAL_SPLASH,
         20,40,60,
         0,0,0,0,0,0,
         SC64_P6_SOURCE_ORGANIC|SC64_P6_SOURCE_REGENERATES_HP|
         SC64_P6_SOURCE_BURROWABLE|SC64_P6_SOURCE_AUTO_ATTACK|
         SC64_P6_SOURCE_ATTACK},
        /* 74: Hero Dark Templar. Warp Blades Hero is weapon 86. */
        {74,40,80,0,150,150,750,7, 1,1,12,6,11,19, 0,0,
         86,45,1,30,15,1,SC64_P6_WEAPON_DAMAGE_NORMAL,
         SC64_P6_WEAPON_TARGET_GROUND,0,0,0,
         0,0,0,0,0,0,
         SC64_P6_SOURCE_ORGANIC|SC64_P6_SOURCE_PERMANENT_CLOAK|
         SC64_P6_SOURCE_AUTO_ATTACK|SC64_P6_SOURCE_ATTACK},
        /* 75: Hero Zeratul. Warp Blades Zeratul is weapon 85. */
        {75,60,400,0,100,300,1500,7, 1,1,12,6,11,19, 0,0,
         85,100,1,22,15,1,SC64_P6_WEAPON_DAMAGE_NORMAL,
         SC64_P6_WEAPON_TARGET_GROUND,0,0,0,
         0,0,0,0,0,0,
         SC64_P6_SOURCE_ORGANIC|SC64_P6_SOURCE_PERMANENT_CLOAK|
         SC64_P6_SOURCE_HERO|SC64_P6_SOURCE_AUTO_ATTACK|
         SC64_P6_SOURCE_ATTACK}
    };
    switch (raw) {
    case SC64_P6_RAW_WALL_MISSILE_TRAP: return &facts[0];
    case SC64_P6_RAW_INFESTED_TERRAN: return &facts[1];
    case SC64_P6_RAW_HERO_DARK_TEMPLAR: return &facts[2];
    case SC64_P6_RAW_ZERATUL: return &facts[3];
    default: return 0;
    }
}

static inline const Sc64P6ActorPolicyRequirement *
sc64_p6_actor_policy_requirement(int raw)
{
    static const Sc64P6ActorPolicyRequirement requirements[] = {
        /* The first 13 used P6 UPRP rows all request special value 0x2.
         * P6 applies these rows to raw 37/38/50 creations only. Existing T6
         * native audit associates that value with an order-117 request, but
         * the P6 live creation path has not independently validated the
         * resulting actor state. */
        {37,SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY,0},
        {38,SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY,0},
        {50,SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY|
            SC64_P6_POLICY_NEEDS_SUICIDE_EXPLOSION,0},
        {74,SC64_P6_POLICY_NEEDS_PERMANENT_CLOAK_AND_DETECTORS,0},
        {75,SC64_P6_POLICY_NEEDS_PERMANENT_CLOAK_AND_DETECTORS,0},
        {210,SC64_P6_POLICY_NEEDS_WALL_TRAP_ART_AND_RUNTIME,0}
    };
    switch (raw) {
    case 37: return &requirements[0];
    case 38: return &requirements[1];
    case 50: return &requirements[2];
    case 74: return &requirements[3];
    case 75: return &requirements[4];
    case 210: return &requirements[5];
    default: return 0;
    }
}

/* Reuse source tables for the overlapping P6 special actors. */
static inline const Sc64Z5ActorFact *sc64_p6_shared_actor_fact(int raw)
{ return raw == SC64_P6_RAW_GHOST ? sc64_z5_actor_fact(raw) : 0; }

static inline const Sc64Z5BuildingFact *sc64_p6_shared_building_fact(int raw)
{
    if (raw == SC64_P6_RAW_TERRAN_BEACON ||
        (raw >= SC64_P6_RAW_DOOR_FIRST && raw <= SC64_P6_RAW_DOOR_LAST))
        return sc64_z5_building_fact(raw);
    return 0;
}

/* Native UnitType::isBuilding() flags for the P6 door raws are clear. The
 * map imports them as THG2 doodad props; any GBA building-pool placement is
 * an integration policy, not a source type flag. */
static inline int sc64_p6_source_is_building(int raw)
{
    if (raw >= SC64_P6_RAW_DOOR_FIRST && raw <= SC64_P6_RAW_DOOR_LAST)
        return 0;
    return raw == SC64_P6_RAW_TERRAN_BEACON || raw == SC64_P6_RAW_PROTOSS_BEACON;
}

#endif
