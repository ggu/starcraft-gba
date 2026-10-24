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

/* -1 is unknown size; other values match BWAPI's Small/Medium/Large/
 * Independent enumeration (0..3). Scenario actors have no training route. */
static inline const Sc64ZpActorFact *sc64_zp_actor_fact(int type)
{
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
