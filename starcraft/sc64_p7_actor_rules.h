#ifndef SC64_P7_ACTOR_RULES_H
#define SC64_P7_ACTOR_RULES_H

/* P7 source facts transcribed from the pinned BWAPI revision
 * d727fed68558c506163048ea889131d8cbb33915. Verify unit rows/flags in
 * bwapi/BWAPILIB/Source/UnitType.cpp and raw enum IDs in
 * bwapi/include/BWAPI/UnitType.h; weapon identities are in
 * bwapi/include/BWAPI/WeaponType.h and numeric damage/cooldown/range/splash
 * tables in bwapi/BWAPILIB/Source/WeaponType.cpp. This is data only; no
 * upstream implementation is incorporated here. */
enum {
    SC64_P7_NEEDS_CARRIER_INTERCEPTORS = 1u << 0,
    SC64_P7_NEEDS_OBSERVER_DETECTION_AND_CLOAK = 1u << 1,
    SC64_P7_NEEDS_ARBITER_STASIS_AND_CLOAK_FIELD = 1u << 2,
    SC64_P7_NEEDS_SHIELD_BATTERY_RECHARGE = 1u << 3,
    SC64_P7_NEEDS_ARCHON_RADIAL_SPLASH = 1u << 4
};

typedef struct {
    Sc64ZpActorFact fact;
    int source_ground_weapon, source_air_weapon;
    int ground_splash_inner, ground_splash_mid, ground_splash_outer;
    int air_splash_inner, air_splash_mid, air_splash_outer;
    int supply_required, can_move, can_attack;
    unsigned source_flags, required_capabilities, implemented_capabilities;
} Sc64P7ActorFact;

typedef struct {
    Sc64ZpBuildingFact fact;
    unsigned source_flags, required_capabilities, implemented_capabilities;
} Sc64P7BuildingFact;

/* These bits are BWAPI UnitPrototypeFlags, retained verbatim for provenance. */
#define SC64_P7_SRC_FLYER          0x00000004u
#define SC64_P7_SRC_ROBOTIC         0x00004000u
#define SC64_P7_SRC_DETECTOR        0x00008000u
#define SC64_P7_SRC_REQUIRES_PSI    0x00080000u
#define SC64_P7_SRC_SPELLCASTER     0x00200000u
#define SC64_P7_SRC_PERMA_CLOAK     0x00400000u
#define SC64_P7_SRC_LARGE           0x02000000u
#define SC64_P7_SRC_HUGE            0x04000000u
#define SC64_P7_SRC_AUTO_ATTACK     0x08000000u
#define SC64_P7_SRC_ATTACK          0x10000000u
#define SC64_P7_SRC_MECHANICAL      0x40000000u
#define SC64_P7_SRC_PRODUCES_UNITS  0x80000000u
#define SC64_P7_SRC_BUILDING        0x00000001u
#define SC64_P7_SRC_ANIMATED_IDLE   0x00000100u

static inline const Sc64P7ActorFact *sc64_p7_actor_fact(int type)
{
    static const Sc64P7ActorFact facts[] = {
        /* raw68 Archon: weapon70 Psionic Shockwave, 30+3, cooldown20,
         * range64; source splash radii are 3/15/30 pixels. Generic GBA combat
         * only hits the selected target, so retain the splash gap explicitly. */
        {{68,10,350,0,0,0,300,256,16,16,15,15,0,2,
          30,3,20,64,1, 30,3,20,64,1,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
          SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|
          SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_WEAPON},
         70,70,3,15,30,3,15,30,8,1,1,
         SC64_P7_SRC_ANIMATED_IDLE|SC64_P7_SRC_HUGE|
         SC64_P7_SRC_AUTO_ATTACK|SC64_P7_SRC_ATTACK,
         SC64_P7_NEEDS_ARCHON_RADIAL_SPLASH,0},
        /* raw71 Arbiter: weapon77 Phase Disruptor Cannon, 10+1, cooldown45,
         * range160, no splash in the pinned source table. */
        {{71,200,150,1,100,350,2400,288,22,22,21,21,1,2,
          10,1,45,160,1, 10,1,45,160,1,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
          SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|
          SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_WEAPON},
         77,77,0,0,0,0,0,0,8,1,1,
         SC64_P7_SRC_FLYER|SC64_P7_SRC_SPELLCASTER|SC64_P7_SRC_LARGE|
         SC64_P7_SRC_AUTO_ATTACK|SC64_P7_SRC_ATTACK|
         SC64_P7_SRC_MECHANICAL,
         SC64_P7_NEEDS_ARBITER_STASIS_AND_CLOAK_FIELD,0},
        /* raw72 Carrier. Native canAttack is special-cased despite no weapon
         * row; do not advertise GBA weapon combat until interceptors exist. */
        {{72,300,150,4,350,250,2100,352,32,32,31,31,1,2,
          0,0,0,0,0, 0,0,0,0,0,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
          SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN},
         130,130,0,0,0,0,0,0,12,1,1,
         SC64_P7_SRC_FLYER|SC64_P7_SRC_HUGE|SC64_P7_SRC_AUTO_ATTACK|
         SC64_P7_SRC_ATTACK|SC64_P7_SRC_MECHANICAL|
         SC64_P7_SRC_PRODUCES_UNITS,
         SC64_P7_NEEDS_CARRIER_INTERCEPTORS,0},
        /* raw84 Observer. Detection and permanent cloak remain unsupported. */
        {{84,40,20,0,25,75,600,288,16,16,15,15,1,0,
          0,0,0,0,0, 0,0,0,0,0,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
          SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN},
         130,130,0,0,0,0,0,0,2,1,0,
         SC64_P7_SRC_FLYER|SC64_P7_SRC_ROBOTIC|SC64_P7_SRC_DETECTOR|
         SC64_P7_SRC_PERMA_CLOAK|SC64_P7_SRC_AUTO_ATTACK|
         SC64_P7_SRC_ATTACK|SC64_P7_SRC_MECHANICAL,
         SC64_P7_NEEDS_OBSERVER_DETECTION_AND_CLOAK,0}
#ifdef SC64_P7_PROJECTILES_ENABLED
        /* raw73 numeric facts from pinned BWAPI UnitType/WeaponType tables.
         * Pulse Cannon table cooldown 1 is not its IScript firing cadence.
         * No generic WEAPON/SELECTABLE policy: the pending child adapter owns
         * those orders, and must call the real moving projectile launcher. */
        ,{{73,40,40,0,25,0,300,192,8,8,7,7,1,0,
           6,1,1,128,1,6,1,1,128,1,
           SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_DRAWABLE},
          79,79,0,0,0,0,0,0,0,1,1,
          SC64_P7_SRC_FLYER|SC64_P7_SRC_AUTO_ATTACK|SC64_P7_SRC_ATTACK|
          SC64_P7_SRC_MECHANICAL,SC64_P7_NEEDS_CARRIER_INTERCEPTORS,0}
#endif
    };
    switch (type) {
    case U_P7_ARCHON: return &facts[0];
    case U_P7_ARBITER: return &facts[1];
    case U_P7_CARRIER: return &facts[2];
    case U_P7_OBSERVER: return &facts[3];
#ifdef SC64_P7_PROJECTILES_ENABLED
    case U_P7_INTERCEPTOR: return &facts[4];
#endif
    default: return 0;
    }
}

static inline const Sc64P7BuildingFact *sc64_p7_building_fact(int type)
{
    static const Sc64P7BuildingFact facts[] = {
        {{159,250,250,1,50,100,450,320,3,2,44,16,44,28,2,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
          SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
         SC64_P7_SRC_BUILDING|SC64_P7_SRC_REQUIRES_PSI|
         SC64_P7_SRC_HUGE|SC64_P7_SRC_MECHANICAL,0,0},
        {{163,450,450,1,150,100,900,320,3,2,24,24,40,24,2,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
          SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
         SC64_P7_SRC_BUILDING|SC64_P7_SRC_REQUIRES_PSI|
         SC64_P7_SRC_HUGE|SC64_P7_SRC_MECHANICAL,0,0},
        {{165,500,500,1,150,200,900,320,3,2,32,24,32,24,2,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
          SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
         SC64_P7_SRC_BUILDING|SC64_P7_SRC_REQUIRES_PSI|
         SC64_P7_SRC_HUGE|SC64_P7_SRC_MECHANICAL,0,0},
        {{169,500,500,1,300,200,900,320,3,2,40,32,47,24,2,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
          SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
         SC64_P7_SRC_BUILDING|SC64_P7_SRC_REQUIRES_PSI|
         SC64_P7_SRC_HUGE|SC64_P7_SRC_MECHANICAL,0,0},
        {{170,500,500,1,200,150,900,320,3,2,44,28,44,28,2,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
          SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
         SC64_P7_SRC_BUILDING|SC64_P7_SRC_REQUIRES_PSI|
         SC64_P7_SRC_HUGE|SC64_P7_SRC_MECHANICAL,
         SC64_P7_NEEDS_ARBITER_STASIS_AND_CLOAK_FIELD,0},
        {{171,450,450,1,150,100,450,320,3,2,32,32,32,20,2,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
          SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
         SC64_P7_SRC_BUILDING|SC64_P7_SRC_REQUIRES_PSI|
         SC64_P7_SRC_HUGE|SC64_P7_SRC_MECHANICAL,0,0},
        {{172,200,200,1,100,0,450,320,3,2,32,16,32,16,2,
          SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
          SC64_ZP_ACTOR_BUILDING|SC64_ZP_ACTOR_DRAWABLE},
         SC64_P7_SRC_BUILDING|SC64_P7_SRC_REQUIRES_PSI|
         SC64_P7_SRC_SPELLCASTER|SC64_P7_SRC_HUGE|
         SC64_P7_SRC_MECHANICAL,
         SC64_P7_NEEDS_SHIELD_BATTERY_RECHARGE,0}
    };
    switch (type) {
    case B_P7_OBSERVATORY: return &facts[0];
    case B_P7_CITADEL_ADUN: return &facts[1];
    case B_P7_TEMPLAR_ARCHIVES: return &facts[2];
    case B_P7_FLEET_BEACON: return &facts[3];
    case B_P7_ARBITER_TRIBUNAL: return &facts[4];
    case B_P7_ROBOTICS_SUPPORT_BAY: return &facts[5];
    case B_P7_SHIELD_BATTERY: return &facts[6];
    default: return 0;
    }
}

#endif
