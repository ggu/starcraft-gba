#ifndef SC64_ZP_ACTOR_RULES_H
#define SC64_ZP_ACTOR_RULES_H

#ifdef SC64_ZP_CAMPAIGN_ACTORS
/* Source-backed compact facts for campaign actors outside the standard
 * skirmish roster. Numeric values were checked against pinned BWAPI UnitType
 * tables (d727fed68558c506163048ea889131d8cbb33915); this is data, not code. */
enum {
    SC64_ZP_ACTOR_SELECTABLE = 1u,
    SC64_ZP_ACTOR_MOVABLE = 2u,
    SC64_ZP_ACTOR_COLLIDABLE = 4u,
    SC64_ZP_ACTOR_DRAWABLE = 8u,
    SC64_ZP_ACTOR_MEN = 16u,
    SC64_ZP_ACTOR_BUILDING = 32u,
    SC64_ZP_ACTOR_INVINCIBLE = 64u,
    SC64_ZP_ACTOR_HIDDEN = 128u,
    SC64_ZP_ACTOR_WEAPON = 256u
#ifdef SC64_Z5_ACTORS
    ,
    /* GBA policy bits. For raw IDs that overlap existing T4 actors, the
       authored policy reuses those project policies; source flags remain in
       Sc64Z5ActorFact and do not imply these bits. Bengalaas intentionally
       reuses the critter policy, with native UI/pathing equivalence unverified. */
    SC64_ZP_ACTOR_POWERUP = 512u,
    SC64_ZP_ACTOR_CRITTER = 1024u,
    SC64_ZP_ACTOR_BEACON = 2048u
#endif
};

typedef struct {
    int raw, hp, shields, armor, minerals, gas, build_frames, sight_px;
    int left, up, right, down, flying, size;
    int ground_damage, ground_bonus, ground_cooldown, ground_range, ground_hits;
    int air_damage, air_bonus, air_cooldown, air_range, air_hits;
    unsigned flags;
} Sc64ZpActorFact;

typedef struct {
    int raw, hp, shields, armor, minerals, gas, build_frames, sight_px;
    int width_tiles, height_tiles, left, up, right, down, size;
    unsigned flags;
} Sc64ZpBuildingFact;

#ifdef SC64_P7_ACTORS
#include "sc64_p7_actor_rules.h"
#endif

#ifdef SC64_Z5_ACTORS
#include "sc64_z5_actor_rules.h"
#endif
#ifdef SC64_P6_ACTORS
#include "sc64_p6_actor_rules.h"
#endif

/* Authored normalized size: -1 unknown, Small/Medium/Large/Independent
 * encoded as 0/1/2/3. This differs from BWAPI native enum ordinals.
 * Scenario actors have no training route. */
static inline const Sc64ZpActorFact *sc64_zp_actor_fact(int type)
{
#ifdef SC64_ZP_FACT_RANGE_CULL
    /* Campaign actor facts occupy a contiguous enum suffix. Reject the
       overwhelmingly common skirmish misses before the gate-specific facts
       and switch below. Relational checks also handle INT_MIN/INT_MAX safely. */
    if (type < U_ZP_INFESTED_KERRIGAN || type >= SC_RU_COUNT) return 0;
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
    /* Pinned raw36 UnitType facts: 200 HP, 10 armor, 4-tile/128-source-px
       sight, L16/U16/R15/D15 bounds, Medium size, and no weapon or motion.
       Egg product identity/timing is kept separately in target/work_timer. */
    static const Sc64ZpActorFact z7_egg = {
        36,200,0,10,0,0,0,128, 16,16,15,15,0,1,
        0,0,0,0,0, 0,0,0,0,0,
        SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
        SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN
    };
    if (type == U_Z7_EGG) return &z7_egg;
#endif
#ifdef SC64_P7_ACTORS
    { const Sc64P7ActorFact *p7 = sc64_p7_actor_fact(type);
      if (p7) return &p7->fact; }
#endif
#ifdef SC64_Z6_ACTORS
    /* Guardian facts use the same pinned source values as the Z6 combat table;
       generic actor consumers now see its HP, sight, bounds, and policy too. */
    static const Sc64ZpActorFact guardian = {
        44,150,0,2,50,100,600,352, 22,22,21,21,1,2,
        20,2,30,256,1, 0,0,0,0,0,
        SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
        SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|
        SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_WEAPON
    };
    if (type == U_Z6_GUARDIAN) return &guardian;
#endif
#ifdef SC64_P5_ACTORS
    /* Shuttle source facts are shared; the optional P5 transport profile
       supplies bounded cargo orders and saves. */
    static const Sc64ZpActorFact shuttle = {
        69,80,60,1,200,0,900,256, 20,16,19,15,1,2,
        0,0,0,0,0, 0,0,0,0,0,
        SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
        SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN
    };
    static const Sc64ZpActorFact battlecruiser = {
        12,500,0,3,400,300,2000,352, 37,29,37,29,1,2,
        25,3,30,192,1, 25,3,30,192,1,
        SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
        SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|
        SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_WEAPON
    };
    if (type == U_P5_SHUTTLE) return &shuttle;
    if (type == U_P5_BATTLECRUISER) return &battlecruiser;
#endif
#ifdef SC64_P6_ACTORS
#ifdef SC64_P6_COMBAT_ENABLED
#define SC64_P6_WEAPON_POLICY SC64_ZP_ACTOR_WEAPON
#else
#define SC64_P6_WEAPON_POLICY 0u
#endif
    static const Sc64ZpActorFact p6_facts[] = {
        /* Source weapon values are retained for audit. WEAPON is deliberately
           absent until the suicide/cloak mechanics have dedicated handlers. */
        {50,60,0,0,100,50,600,160, 8,9,8,10,0,0,
         500,0,1,3,1, 0,0,0,0,0,
         SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
         SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|SC64_P6_WEAPON_POLICY},
        {74,40,80,0,150,150,750,224, 12,6,11,19,0,0,
         45,1,30,15,1, 0,0,0,0,0,
         SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
         SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|SC64_P6_WEAPON_POLICY},
        {75,60,400,0,100,300,1500,224, 12,6,11,19,0,0,
         100,1,22,15,1, 0,0,0,0,0,
         SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
         SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|SC64_P6_WEAPON_POLICY}
    };
    switch (type) {
    case U_P6_INFESTED_TERRAN: return &p6_facts[0];
    case U_P6_HERO_DARK_TEMPLAR: return &p6_facts[1];
    case U_P6_ZERATUL: return &p6_facts[2];
    default: break;
    }
#undef SC64_P6_WEAPON_POLICY
#endif
#ifdef SC64_Z5_ACTORS
    static const Sc64ZpActorFact z5_facts[] = {
        /* raw, HP/shields/armor/cost/time/sight, bounds, flight/size,
           ground damage/bonus/cooldown/range/hits, air weapon, policy tags */
        {1,45,0,0,25,75,750,288, 7,10,7,11,0,0,
         10,1,22,224,1, 10,1,22,224,1,
         SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
         SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|
         SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_WEAPON},
        {15,40,0,0,0,0,1,224, 8,9,8,10,0,0,
         0,0,0,0,0, 0,0,0,0,0,
         SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
         SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN},
        {89,60,0,0,1,1,1,224, 16,16,15,15,0,0,
         0,0,0,0,0, 0,0,0,0,0,
         SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_COLLIDABLE|
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_CRITTER},
        {90,60,0,0,1,1,1,224, 16,16,15,15,0,0,
         0,0,0,0,0, 0,0,0,0,0,
         /* Reuses the critter GBA policy; native UI/pathing equivalence is
            not established by BWAPI/OpenBW and remains unverified. */
         SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_COLLIDABLE|
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_CRITTER},
        {95,60,0,0,1,1,1,224, 16,16,15,15,0,0,
         0,0,0,0,0, 0,0,0,0,0,
         SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_COLLIDABLE|
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_CRITTER},
        {218,800,0,0,1,1,1,160, 16,16,15,15,0,3,
         0,0,0,0,0, 0,0,0,0,0,
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_INVINCIBLE|
         SC64_ZP_ACTOR_POWERUP}
    };
    switch (type) {
    case U_Z5_GHOST: return &z5_facts[0];
    case U_Z5_CIVILIAN: return &z5_facts[1];
    case U_Z5_RHYNADON: return &z5_facts[2];
    case U_Z5_BENGALAAS: return &z5_facts[3];
    case U_Z5_RAGNASAUR: return &z5_facts[4];
    case U_Z5_DATA_DISK: return &z5_facts[5];
    default: break;
    }
#endif
    static const Sc64ZpActorFact facts[] = {
        /* raw, default HP/shields/armor, default costs/time, sight,
           dimensions, flight, BWAPI size, ground/air weapon default facts */
        {51,400,0,2,200,300,1500,288, 7,10,7,11,0,0, 50,1,15,15,1, 0,0,0,0,0,
         SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_WEAPON},
        {101,1,0,0,0,0,1,320, 13,13,13,17,1,3, 0,0,0,0,0, 0,0,0,0,0,
         SC64_ZP_ACTOR_HIDDEN},
        {19,300,0,3,150,0,900,256, 16,16,15,15,0,1, 30,2,22,160,1, 0,0,0,0,0,
         SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_WEAPON},
        {67,40,40,0,50,150,750,224, 12,10,11,13,0,0, 0,0,0,0,0, 0,0,0,0,0,
         SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN},
        {39,400,0,1,200,200,900,224, 19,16,18,15,0,2, 20,3,15,25,1, 0,0,0,0,0,
         SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_WEAPON}
    };
    switch (type) {
    case U_ZP_INFESTED_KERRIGAN: return &facts[0];
    case U_ZP_MAP_REVEALER: return &facts[1];
    case U_ZP_RAYNOR_VULTURE: return &facts[2];
    case U_ZP_HIGH_TEMPLAR: return &facts[3];
    case U_ZP_ULTRALISK: return &facts[4];
    default: return 0;
    }
}

static inline unsigned sc64_zp_actor_flags(int type)
{ const Sc64ZpActorFact *f = sc64_zp_actor_fact(type); return f ? f->flags : 0; }
static inline int sc64_zp_actor_sight_source_pixels(int type)
{ const Sc64ZpActorFact *f = sc64_zp_actor_fact(type); return f ? f->sight_px : 0; }
static inline int sc64_zp_actor_bounds_source(int type, int *left, int *up,
                                               int *right, int *down)
{
    const Sc64ZpActorFact *f = sc64_zp_actor_fact(type);
    if (!f) return 0;
    if (left) *left=f->left;
    if (up) *up=f->up;
    if (right) *right=f->right;
    if (down) *down=f->down;
    return 1;
}
static inline int sc64_zp_actor_default_hp(int type)
{ const Sc64ZpActorFact *f = sc64_zp_actor_fact(type); return f ? f->hp : 0; }
static inline int sc64_zp_actor_default_shields(int type)
{ const Sc64ZpActorFact *f = sc64_zp_actor_fact(type); return f ? f->shields : 0; }

static inline const Sc64ZpBuildingFact *sc64_zp_building_fact(int type)
{
#ifdef SC64_ZP_FACT_RANGE_CULL
    /* Campaign building facts likewise occupy an enum suffix. */
    if (type < B_ZP_CONTROL_TOWER || type >= SC_RB_COUNT) return 0;
#endif
#ifdef SC64_P7_ACTORS
    { const Sc64P7BuildingFact *p7 = sc64_p7_building_fact(type);
      if (p7) return &p7->fact; }
#endif
#if defined(SC64_Z6_RUNTIME) && !defined(SC64_TERRAN_LATE_ACTORS)
    static const Sc64ZpBuildingFact z6_support_bay = {
        171,450,450,1,150,100,450,320,3,2,32,32,32,20,2,
        SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
        SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE
    };
    static const Sc64ZpBuildingFact z6_shield_battery = {
        172,200,200,1,100,0,450,320,3,2,32,16,32,16,2,
        SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
        SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE
    };
    if (type == B_Z6_ROBOTICS_SUPPORT_BAY) return &z6_support_bay;
    if (type == B_Z6_SHIELD_BATTERY) return &z6_shield_battery;
#endif
#ifdef SC64_NYDUS_ENABLED
    /* Pinned BWAPI raw-134 facts; see sc64_nydus_rules.h. */
    static const Sc64ZpBuildingFact nydus_canal = {
        134,250,0,1,150,0,600,256,2,2,32,32,31,31,3,
        SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
        SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE
    };
    if (type == B_NYDUS_CANAL) return &nydus_canal;
#endif
#ifdef SC64_Z5_ACTORS
    static const Sc64ZpBuildingFact z5_beacon =
        {195,100000,0,0,50,50,1,256,3,2,48,32,47,31,3,
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|
         SC64_ZP_ACTOR_INVINCIBLE|SC64_ZP_ACTOR_BEACON};
    if (type == B_Z5_TERRAN_BEACON) return &z5_beacon;
#endif
#ifdef SC64_Z5_DOODADS
    static const Sc64ZpBuildingFact z5_doodads[] = {
        /* These are authored GBA policies copied from the established T4
         * prop adapter. Source invincibility/weapon facts are separate. */
        {203,50,0,0,0,0,1,192,2,2,32,32,31,31,2,
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_WEAPON},
        {205,100000,0,0,0,0,1,32,3,2,25,17,44,20,3,
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_INVINCIBLE},
        {206,100000,0,0,0,0,1,32,3,2,44,17,25,20,3,
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_INVINCIBLE},
        {207,100000,0,0,0,0,1,32,3,2,41,17,28,20,3,
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_INVINCIBLE},
        {208,100000,0,0,0,0,1,32,3,2,28,17,41,20,3,
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_INVINCIBLE},
        {209,50,0,0,0,0,1,192,2,2,32,32,31,31,2,
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_WEAPON},
        {211,50,0,0,0,0,1,96,1,1,16,16,15,15,2,
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_WEAPON},
        {213,50,0,0,0,0,1,96,1,1,16,16,15,15,2,
         SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_WEAPON}
    };
    switch (type) {
    case B_Z5_FLOOR_MISSILE_TRAP: return &z5_doodads[0];
    case B_Z5_UPPER_DOOR: return &z5_doodads[1];
    case B_Z5_RIGHT_UPPER_DOOR: return &z5_doodads[2];
    case B_Z5_PIT_DOOR: return &z5_doodads[3];
    case B_Z5_RIGHT_PIT_DOOR: return &z5_doodads[4];
    case B_Z5_FLOOR_GUN_TRAP: return &z5_doodads[5];
    case B_Z5_WALL_FLAME_TRAP: return &z5_doodads[6];
    case B_Z5_RIGHT_WALL_FLAME_TRAP: return &z5_doodads[7];
    default: break;
    }
#endif
#ifdef SC64_P6_ACTORS
    static const Sc64ZpBuildingFact p6_wall_missile_trap = {
        210,50,0,0,0,0,0,192,1,1,16,16,15,15,2,
        SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|
        SC64_ZP_ACTOR_COLLIDABLE
    };
    if (type == B_P6_WALL_MISSILE_TRAP) return &p6_wall_missile_trap;
#endif
    static const Sc64ZpBuildingFact facts[] = {
        /* raw, hp, shields, armor, sight, width, height, size, flags */
        {115,500,0,1,50,50,600,256,2,2,47,24,28,22,2,SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
        {120,750,0,1,50,50,600,256,2,2,39,24,31,24,2,SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
        {123,750,0,1,100,50,1200,256,3,2,48,32,47,22,2,SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
        {138,850,0,1,150,100,900,256,3,2,38,28,32,28,2,SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
        {140,600,0,1,150,200,1200,256,3,2,40,32,32,31,2,SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
        {196,100000,0,0,100,100,1,256,3,2,48,32,47,31,3,SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_INVINCIBLE}
    };
    switch (type) {
    case B_ZP_CONTROL_TOWER: return &facts[0];
    case B_ZP_MACHINE_SHOP: return &facts[1];
    case B_ZP_ARMORY: return &facts[2];
    case B_ZP_QUEENS_NEST: return &facts[3];
    case B_ZP_ULTRALISK_CAVERN: return &facts[4];
    case B_ZP_PROTOSS_BEACON: return &facts[5];
    default: return 0;
    }
}

static inline unsigned sc64_zp_building_flags(int type)
{ const Sc64ZpBuildingFact *f = sc64_zp_building_fact(type); return f ? f->flags : 0; }
static inline int sc64_zp_building_sight_source_pixels(int type)
{ const Sc64ZpBuildingFact *f = sc64_zp_building_fact(type); return f ? f->sight_px : 0; }
static inline int sc64_zp_building_bounds_source(int type, int *left, int *up,
                                                  int *right, int *down)
{
    const Sc64ZpBuildingFact *f = sc64_zp_building_fact(type);
    if (!f) return 0;
    if (left) *left=f->left;
    if (up) *up=f->up;
    if (right) *right=f->right;
    if (down) *down=f->down;
    return 1;
}
static inline int sc64_zp_building_default_hp(int type)
{ const Sc64ZpBuildingFact *f = sc64_zp_building_fact(type); return f ? f->hp : 0; }
#endif
#endif
