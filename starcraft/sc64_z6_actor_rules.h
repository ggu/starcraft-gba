#ifndef SC64_Z6_ACTOR_RULES_H
#define SC64_Z6_ACTOR_RULES_H

/* Authored source facts for the Z6 Guardian, taken from the pinned BWAPI
 * UnitType.cpp and WeaponType.cpp tables at
 * d727fed68558c506163048ea889131d8cbb33915. This is numeric fact data only;
 * it does not enable a mission adapter or assert full source combat behavior. */
enum {
    SC64_Z6_SOURCE_FLYER = 1u << 0,
    SC64_Z6_SOURCE_REGENERATES_HP = 1u << 1,
    SC64_Z6_SOURCE_ORGANIC = 1u << 2,
    SC64_Z6_SOURCE_MORPH = 1u << 3,
    SC64_Z6_SOURCE_HUGE = 1u << 4,
    SC64_Z6_SOURCE_AUTO_ATTACK_MOVE = 1u << 5,
    SC64_Z6_SOURCE_ATTACK_FLAG = 1u << 6,
    SC64_Z6_SOURCE_GROUND_ONLY = 1u << 7
};
enum {
    SC64_Z6_DAMAGE_NORMAL = 0,
    SC64_Z6_UPGRADE_ZERG_FLYER_ATTACKS = 1
};

typedef struct {
    int raw, hp, shields, armor, minerals, gas, build_frames, sight_px;
    int width_tiles, height_tiles, left, up, right, down, size;
    int ground_damage_type, ground_upgrade;
    int ground_damage, ground_bonus, ground_cooldown, ground_range, ground_hits;
    int air_damage, air_bonus, air_cooldown, air_range, air_hits;
    unsigned flags;
} Sc64Z6ActorFact;

static inline const Sc64Z6ActorFact *sc64_z6_actor_fact(int raw)
{
    static const Sc64Z6ActorFact guardian = {
        44, 150, 0, 2, 50, 100, 600, 11 * 32,
        2, 2, 22, 22, 21, 21, 2,
        SC64_Z6_DAMAGE_NORMAL, SC64_Z6_UPGRADE_ZERG_FLYER_ATTACKS,
        20, 2, 30, 256, 1, 0, 0, 0, 0, 0,
        SC64_Z6_SOURCE_FLYER|SC64_Z6_SOURCE_REGENERATES_HP|
        SC64_Z6_SOURCE_ORGANIC|SC64_Z6_SOURCE_MORPH|
        SC64_Z6_SOURCE_HUGE|SC64_Z6_SOURCE_AUTO_ATTACK_MOVE|
        SC64_Z6_SOURCE_ATTACK_FLAG|SC64_Z6_SOURCE_GROUND_ONLY
    };
    return raw == 44 ? &guardian : 0;
}

/* Conservative engine broadphase radius from source pixel bounds. */
static inline int sc64_z6_actor_radius_px(const Sc64Z6ActorFact *f)
{
    if (!f) return 0;
    int w = f->left + f->right, h = f->up + f->down;
    int extent = w > h ? w : h;
    int radius = (extent + 3) / 4;
    return radius < 6 ? 6 : radius;
}

#endif
