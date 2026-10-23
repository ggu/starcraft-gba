#ifndef SC64_TERRAN_LATE_ACTOR_RULES_H
#define SC64_TERRAN_LATE_ACTOR_RULES_H

#ifdef SC64_TERRAN_LATE_ACTORS
/* Public default facts transcribed from the pinned BWAPI tables. These are
 * data-only values; no source implementation or asset data is included. */
typedef struct {
    int raw, hp, shields, armor, minerals, gas, build_frames, supply;
    int sight_px, left, up, right, down, flying;
    int ground_damage, ground_bonus, ground_cooldown, ground_hits, ground_range;
    int air_damage, air_bonus, air_cooldown, air_hits, air_range;
    int footprint_width_tiles, footprint_height_tiles;
} Sc64TerranLateActorFact;

enum {
    SC64_LATE_ACTOR_SELECTABLE = 1u,
    SC64_LATE_ACTOR_MOVABLE = 2u,
    SC64_LATE_ACTOR_COLLIDABLE = 4u,
    SC64_LATE_ACTOR_DRAWABLE = 8u,
    SC64_LATE_ACTOR_MEN = 16u,
    SC64_LATE_ACTOR_BUILDING = 32u,
    SC64_LATE_ACTOR_INVINCIBLE = 64u,
    SC64_LATE_ACTOR_WEAPON = 256u
};

static inline const Sc64TerranLateActorFact *sc64_terran_late_unit_fact(int type)
{
    static const Sc64TerranLateActorFact facts[] = {
        /* raw, HP, shields, armor, minerals, gas, frames, supply, sight,
           L/U/R/D source-pixel bounds, flying, ground dmg/bonus/cd/hits/range,
           air dmg/bonus/cd/hits/range */
        { 9, 200, 0, 1, 100,225,1200,4,320, 24,16,24,20,1, 0,0,0,0,0, 0,0,0,0,0, 2,2 },
        {12, 500, 0, 3, 400,300,2000,12,352, 37,29,37,29,1, 25,3,30,1,192, 25,3,30,1,192, 2,2 },
        {29, 700, 0, 4, 800,600,4800,0,352, 37,29,37,29,1, 50,3,30,1,192, 50,3,30,1,192, 2,2 },
        {39, 400, 0, 1, 200,200, 900,8,224, 19,16,18,15,0, 20,3,15,1,25, 0,0,0,0,0, 2,2 },
        {69,  80,60, 1, 200,  0, 900,4,256, 20,16,19,15,1,  0,0,0,0,0,  0,0,0,0,0, 2,1 },
        {217,800, 0, 0,   1,  1,   1,0,160, 16,16,15,15,0,  0,0,0,0,0,  0,0,0,0,0, 1,1 }
    };
    unsigned index = (unsigned)(type - U_SCIENCE_VESSEL);
    return index < sizeof(facts)/sizeof(facts[0]) ? &facts[index] : 0;
}

static inline const Sc64TerranLateActorFact *sc64_terran_late_building_fact(int type)
{
    static const Sc64TerranLateActorFact facts[] = {
        {108,600,0,1,100,100,1200,0,256,37,16,31,25,0,0,0,0,0,0,0,0,0,0,0,2,2},
        {117,750,0,1,50,50,600,0,256,47,24,28,22,0,0,0,0,0,0,0,0,0,0,0,2,2},
        {120,750,0,1,50,50,600,0,256,39,24,31,24,0,0,0,0,0,0,0,0,0,0,0,2,2},
        {123,750,0,1,100,50,1200,0,256,48,32,47,22,0,0,0,0,0,0,0,0,0,0,0,3,2},
        {136,850,0,1,100,100,900,0,256,48,32,48,4,0,0,0,0,0,0,0,0,0,0,0,4,2},
        {140,600,0,1,150,200,1200,0,256,40,32,32,31,0,0,0,0,0,0,0,0,0,0,0,3,2},
        {163,450,450,1,150,100,900,0,320,24,24,40,24,0,0,0,0,0,0,0,0,0,0,0,3,2},
        {168,2000,0,1,150,0,1,0,256,64,48,63,47,0,0,0,0,0,0,0,0,0,0,0,4,3},
        {169,500,500,1,300,200,900,0,320,40,32,47,24,0,0,0,0,0,0,0,0,0,0,0,3,2},
        {171,450,450,1,150,100,450,0,320,32,32,32,20,0,0,0,0,0,0,0,0,0,0,0,3,2},
        {172,200,200,1,100,0,450,0,320,32,16,32,16,0,0,0,0,0,0,0,0,0,0,0,3,2},
        {127,2000,0,1,200,0,900,0,256,48,32,47,31,0,0,0,0,0,0,0,0,0,0,0,3,2},
        {116,850,0,1,100,150,900,0,320,48,38,48,38,0,0,0,0,0,0,0,0,0,0,0,4,3},
        {118,600,0,1,50,50,600,0,256,47,24,28,22,0,0,0,0,0,0,0,0,0,0,0,2,2}
    };
    unsigned index = (unsigned)(type - B_NUCLEAR_SILO);
    return index < sizeof(facts)/sizeof(facts[0]) ? &facts[index] : 0;
}

static inline int sc64_terran_late_actor_bounds(int type, int building,
                                                int *left, int *up,
                                                int *right, int *down)
{
    const Sc64TerranLateActorFact *f = building ?
        sc64_terran_late_building_fact(type) : sc64_terran_late_unit_fact(type);
    if (!f) return 0;
    if (left) *left=f->left;
    if (up) *up=f->up;
    if (right) *right=f->right;
    if (down) *down=f->down;
    return 1;
}

static inline int sc64_terran_late_unit_sight_source_pixels(int type)
{
    const Sc64TerranLateActorFact *f = sc64_terran_late_unit_fact(type);
    return f ? f->sight_px : 0;
}
static inline int sc64_terran_late_building_sight_source_pixels(int type)
{
    const Sc64TerranLateActorFact *f = sc64_terran_late_building_fact(type);
    return f ? f->sight_px : 0;
}
static inline int sc64_terran_late_unit_default_hp(int type)
{
    const Sc64TerranLateActorFact *f = sc64_terran_late_unit_fact(type);
    return f ? f->hp : 0;
}
static inline int sc64_terran_late_building_default_hp(int type)
{
    const Sc64TerranLateActorFact *f = sc64_terran_late_building_fact(type);
    return f ? f->hp : 0;
}
static inline unsigned sc64_terran_late_unit_actor_flags(int type)
{
    switch (type) {
    case U_SCIENCE_VESSEL: case U_BATTLECRUISER: case U_NORAD_II:
    case U_ULTRALISK: case U_SHUTTLE:
        return SC64_LATE_ACTOR_SELECTABLE | SC64_LATE_ACTOR_MOVABLE |
               SC64_LATE_ACTOR_COLLIDABLE | SC64_LATE_ACTOR_DRAWABLE |
               SC64_LATE_ACTOR_MEN |
               (sc64_terran_late_unit_fact(type)->ground_damage ||
                sc64_terran_late_unit_fact(type)->air_damage ? SC64_LATE_ACTOR_WEAPON : 0);
    case U_PSI_EMITTER:
        return SC64_LATE_ACTOR_DRAWABLE | SC64_LATE_ACTOR_INVINCIBLE;
    default: return 0;
    }
}
static inline unsigned sc64_terran_late_building_actor_flags(int type)
{
    return sc64_terran_late_building_fact(type) ?
        SC64_LATE_ACTOR_SELECTABLE | SC64_LATE_ACTOR_COLLIDABLE |
        SC64_LATE_ACTOR_DRAWABLE | SC64_LATE_ACTOR_BUILDING : 0;
}
#endif
#endif
