#ifndef SC64_Z5_ACTOR_RULES_H
#define SC64_Z5_ACTOR_RULES_H

/* Authored source facts for the unusual Z5 campaign actors. Values are
 * transcribed from pinned BWAPI UnitType.cpp and WeaponType.cpp at
 * d727fed68558c506163048ea889131d8cbb33915. This header intentionally has
 * no dependency on the shared race enums or runtime actor gate.
 *
 * The source tables expose movement, flight, building, invincibility and
 * weapon properties, but do not define selectable/collidable/drawable UI
 * policy. The separate GBA runtime facts in sc64_zp_actor_rules.h reuse
 * existing same-raw T4 policies where available; Bengalaas reuses the critter
 * policy with native UI/pathing equivalence explicitly unverified. These are
 * authored GBA policies, not claims that the references settle those flags.
 * The sprite-presence and attackable/move-interaction facts below come from
 * the pinned reference. OpenBW makes invincible targets immune to damage and
 * turns right-click on special beacons and powerups into Move, but that does
 * not establish general pathing walkability.
 */

enum {
    SC64_Z5_SOURCE_CLOAKABLE          = 1u << 0,
    SC64_Z5_SOURCE_ORGANIC             = 1u << 1,
    SC64_Z5_SOURCE_SPELLCASTER         = 1u << 2,
    SC64_Z5_SOURCE_AUTO_ATTACK_MOVE    = 1u << 3,
    SC64_Z5_SOURCE_ATTACK_FLAG         = 1u << 4,
    SC64_Z5_SOURCE_BUILDING            = 1u << 5,
    SC64_Z5_SOURCE_INVINCIBLE          = 1u << 6,
    SC64_Z5_SOURCE_NEUTRAL_ACCESSORIES = 1u << 7,
    SC64_Z5_SOURCE_NPC_ACCESSORIES     = 1u << 8,
    SC64_Z5_SOURCE_FLYER               = 1u << 9,
    SC64_Z5_SOURCE_CRITTER             = 1u << 10,
    SC64_Z5_SOURCE_POWERUP             = 1u << 11,
    SC64_Z5_SOURCE_BEACON              = 1u << 12,
    SC64_Z5_REF_ATTACKABLE             = 1u << 13,
    SC64_Z5_REF_MOVE_INTERACTION       = 1u << 14,
    SC64_Z5_REF_HAS_SPRITE             = 1u << 15,
    SC64_Z5_SOURCE_HAS_GROUND_WEAPON   = 1u << 16,
    SC64_Z5_SOURCE_DETECTOR            = 1u << 17
};

typedef struct {
    int damage, bonus, cooldown, range, hits;
} Sc64Z5WeaponFact;

typedef struct {
    int raw, hp, shields, armor, minerals, gas, build_frames, sight_px;
    int left, up, right, down, flying, size;
    Sc64Z5WeaponFact ground, air;
    unsigned flags;
} Sc64Z5ActorFact;

typedef struct {
    int raw, hp, shields, armor, minerals, gas, build_frames, sight_px;
    int width_tiles, height_tiles, left, up, right, down, size;
    unsigned flags;
} Sc64Z5BuildingFact;

static inline const Sc64Z5ActorFact *sc64_z5_actor_fact(int raw)
{
    static const Sc64Z5ActorFact facts[] = {
        /* raw 1 Terran Ghost: Small, cloakable organic spellcaster. */
        {1,45,0,0,25,75,750,288, 7,10,7,11,0,0,
         {10,1,22,224,1},{10,1,22,224,1},
         SC64_Z5_SOURCE_CLOAKABLE|SC64_Z5_SOURCE_ORGANIC|
         SC64_Z5_SOURCE_SPELLCASTER|SC64_Z5_SOURCE_AUTO_ATTACK_MOVE|
         SC64_Z5_SOURCE_ATTACK_FLAG|SC64_Z5_REF_ATTACKABLE|SC64_Z5_REF_HAS_SPRITE},
        /* raw 15 Terran Civilian: Attack prototype bit, but no weapon. */
        {15,40,0,0,0,0,1,224, 8,9,8,10,0,0,
         {0,0,0,0,0},{0,0,0,0,0},
         SC64_Z5_SOURCE_ORGANIC|SC64_Z5_SOURCE_AUTO_ATTACK_MOVE|
         SC64_Z5_SOURCE_ATTACK_FLAG|SC64_Z5_REF_ATTACKABLE|SC64_Z5_REF_HAS_SPRITE},
        /* raw 89 Rhynadon, raw 90 Bengalaas, raw 95 Ragnasaur. */
        {89,60,0,0,1,1,1,224, 16,16,15,15,0,0,
         {0,0,0,0,0},{0,0,0,0,0},
         SC64_Z5_SOURCE_ORGANIC|SC64_Z5_SOURCE_AUTO_ATTACK_MOVE|
         SC64_Z5_SOURCE_ATTACK_FLAG|SC64_Z5_SOURCE_CRITTER|
         SC64_Z5_REF_ATTACKABLE|SC64_Z5_REF_HAS_SPRITE},
        {90,60,0,0,1,1,1,224, 16,16,15,15,0,0,
         {0,0,0,0,0},{0,0,0,0,0},
         SC64_Z5_SOURCE_ORGANIC|SC64_Z5_SOURCE_AUTO_ATTACK_MOVE|
         SC64_Z5_SOURCE_ATTACK_FLAG|SC64_Z5_SOURCE_CRITTER|
         SC64_Z5_REF_ATTACKABLE|SC64_Z5_REF_HAS_SPRITE},
        {95,60,0,0,1,1,1,224, 16,16,15,15,0,0,
         {0,0,0,0,0},{0,0,0,0,0},
         SC64_Z5_SOURCE_ORGANIC|SC64_Z5_SOURCE_AUTO_ATTACK_MOVE|
         SC64_Z5_SOURCE_ATTACK_FLAG|SC64_Z5_SOURCE_CRITTER|
         SC64_Z5_REF_ATTACKABLE|SC64_Z5_REF_HAS_SPRITE},
        /* raw 218 Data Disk is a powerup, not a building or flyer. */
        {218,800,0,0,1,1,1,160, 16,16,15,15,0,3,
         {0,0,0,0,0},{0,0,0,0,0},
         SC64_Z5_SOURCE_INVINCIBLE|SC64_Z5_SOURCE_NEUTRAL_ACCESSORIES|
         SC64_Z5_SOURCE_NPC_ACCESSORIES|SC64_Z5_SOURCE_POWERUP|
         SC64_Z5_REF_MOVE_INTERACTION|SC64_Z5_REF_HAS_SPRITE}
    };
    switch (raw) {
    case 1: return &facts[0];
    case 15: return &facts[1];
    case 89: return &facts[2];
    case 90: return &facts[3];
    case 95: return &facts[4];
    case 218: return &facts[5];
    default: return 0;
    }
}

static inline const Sc64Z5BuildingFact *sc64_z5_building_fact(int raw)
{
    static const Sc64Z5BuildingFact facts[] = {
        /* raw 195 Special Terran Beacon. This is Building|Invincible in
         * UnitType.cpp, but isBeacon() is special interaction, not powerup. */
        {195,100000,0,0,50,50,1,256, 3,2,48,32,47,31,3,
         SC64_Z5_SOURCE_BUILDING|SC64_Z5_SOURCE_INVINCIBLE|
         SC64_Z5_SOURCE_BEACON|SC64_Z5_REF_MOVE_INTERACTION|
         SC64_Z5_REF_HAS_SPRITE},
        /* The four doors are large, invincible structures with 100000 HP. */
        {205,100000,0,0,1,1,1,32, 3,2,25,17,44,20,3,
         SC64_Z5_SOURCE_BUILDING|SC64_Z5_SOURCE_INVINCIBLE|SC64_Z5_REF_HAS_SPRITE},
        {206,100000,0,0,1,1,1,32, 3,2,44,17,25,20,3,
         SC64_Z5_SOURCE_BUILDING|SC64_Z5_SOURCE_INVINCIBLE|SC64_Z5_REF_HAS_SPRITE},
        {207,100000,0,0,1,1,1,32, 3,2,41,17,28,20,3,
         SC64_Z5_SOURCE_BUILDING|SC64_Z5_SOURCE_INVINCIBLE|SC64_Z5_REF_HAS_SPRITE},
        {208,100000,0,0,1,1,1,32, 3,2,28,17,41,20,3,
         SC64_Z5_SOURCE_BUILDING|SC64_Z5_SOURCE_INVINCIBLE|SC64_Z5_REF_HAS_SPRITE},
        /* These unit-backed trap props live in the runtime building pool,
         * but their native UnitType flags do not include Building. They are
         * detectors; OpenBW therefore uses their nominal sight range. */
        {203,50,0,0,1,1,1,192, 2,2,32,32,31,31,3,
         SC64_Z5_SOURCE_DETECTOR|SC64_Z5_SOURCE_HAS_GROUND_WEAPON|SC64_Z5_REF_HAS_SPRITE},
        {209,50,0,0,1,1,1,192, 2,2,32,32,31,31,3,
         SC64_Z5_SOURCE_DETECTOR|SC64_Z5_SOURCE_HAS_GROUND_WEAPON|SC64_Z5_REF_HAS_SPRITE},
        {211,50,0,0,1,1,1,96, 1,1,16,16,15,15,3,
         SC64_Z5_SOURCE_DETECTOR|SC64_Z5_SOURCE_HAS_GROUND_WEAPON|SC64_Z5_REF_HAS_SPRITE},
        {213,50,0,0,1,1,1,96, 1,1,16,16,15,15,3,
         SC64_Z5_SOURCE_DETECTOR|SC64_Z5_SOURCE_HAS_GROUND_WEAPON|SC64_Z5_REF_HAS_SPRITE}
    };
    switch (raw) {
    case 195: return &facts[0]; case 205: return &facts[1];
    case 206: return &facts[2]; case 207: return &facts[3];
    case 208: return &facts[4]; case 203: return &facts[5];
    case 209: return &facts[6]; case 211: return &facts[7];
    case 213: return &facts[8]; default: return 0;
    }
}

/* Bounds dimensions are BWAPI source pixels. Keep the same conservative
 * collision-radius conversion used by campaign actor fixtures: ceil(max
 * axis extent / 4), with six pixels minimum. */
static inline int sc64_z5_actor_radius_px(const Sc64Z5ActorFact *f)
{
    if (!f) return 0;
    int w = f->left + f->right, h = f->up + f->down;
    int extent = w > h ? w : h;
    int radius = (extent + 3) / 4;
    return radius < 6 ? 6 : radius;
}

static inline int sc64_z5_building_radius_px(const Sc64Z5BuildingFact *f)
{
    if (!f) return 0;
    int w = f->left + f->right, h = f->up + f->down;
    int extent = w > h ? w : h;
    int radius = (extent + 3) / 4;
    return radius < 6 ? 6 : radius;
}

#endif
