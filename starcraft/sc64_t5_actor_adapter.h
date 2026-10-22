#ifndef SC64_T5_ACTOR_ADAPTER_H
#define SC64_T5_ACTOR_ADAPTER_H

#if defined(SC64_T5_RUNTIME)
#include "race_rules.h"
#include "sc64_t5_actor_rules.h"

/* Explicit actor roles for the first T5 actor slice. The mine receives only
 * its autonomous-role bit: it must not enter ordinary attack-order targeting
 * until its armed/acquire/explode lifecycle is implemented. */
enum {
    SC64_T5_ACTOR_SELECTABLE = 1u << 0,
    SC64_T5_ACTOR_MOVABLE = 1u << 1,
    SC64_T5_ACTOR_COLLIDABLE = 1u << 2,
    SC64_T5_ACTOR_DRAWABLE = 1u << 3,
    SC64_T5_ACTOR_MEN = 1u << 4,
    SC64_T5_ACTOR_WEAPON = 1u << 5,
    SC64_T5_ACTOR_AUTONOMOUS_MINE = 1u << 6
};

static inline unsigned sc64_t5_actor_flags(int type)
{
    switch (type) {
    case U_KERRIGAN:
    case U_RAYNOR_VULTURE:
        return SC64_T5_ACTOR_SELECTABLE | SC64_T5_ACTOR_MOVABLE |
               SC64_T5_ACTOR_COLLIDABLE | SC64_T5_ACTOR_DRAWABLE |
               SC64_T5_ACTOR_MEN | SC64_T5_ACTOR_WEAPON;
    case U_SPIDER_MINE:
        return SC64_T5_ACTOR_AUTONOMOUS_MINE;
    default: return 0;
    }
}

static inline int sc64_t5_actor_intrinsically_invincible(int type)
{ (void)type; return 0; }

/* BWAPI UnitType.cpp UnitType::dimensionLeft/Up/Right/Down values, in source
 * pixels. These dimensions support placement/Bring overlap without inferring
 * collision boxes from sprite canvas geometry. */
static inline int sc64_t5_actor_bounds_source(int type, int *left, int *up,
                                               int *right, int *down)
{
    int l, u, r, d;
    switch (type) {
    case U_KERRIGAN: l=7; u=10; r=7; d=11; break;
    case U_RAYNOR_VULTURE: l=16; u=16; r=15; d=15; break;
    case U_SPIDER_MINE: l=7; u=7; r=7; d=7; break;
    default: return 0;
    }
    if (left) *left=l;
    if (up) *up=u;
    if (right) *right=r;
    if (down) *down=d;
    return 1;
}

/* Weapon values are per hit. BWAPI's damageBonus field is the per-upgrade
 * increment: Kerrigan gains 1 per level, Raynor's hero Vulture 2 per level. */
static inline int sc64_t5_actor_weapon_damage(int type, int upgrade_level,
                                               int target_flying)
{
    const Sc64T5ActorRule *rule;
    int raw = sc64_race_raw_unit(type);
    if (upgrade_level < 0) upgrade_level = 0;
    if (upgrade_level > 3) upgrade_level = 3;
    if (type == U_SPIDER_MINE) return 0;
    if (target_flying && type == U_RAYNOR_VULTURE) return 0;
    rule = raw >= 0 ? sc64_t5_actor_rule((unsigned)raw) : 0;
    return rule && (target_flying ? rule->normal_air_attack :
                    rule->normal_ground_attack) ?
           (int)rule->weapon_damage +
               (int)rule->weapon_damage_bonus * upgrade_level : 0;
}

/* Post-shield HP damage follows the existing integer Concussive helper:
 * subtract armor, then apply size factors with minimum 1 HP. BW fractional HP
 * rounding is not represented by the integer GBA state. */
static inline int sc64_t5_concussive_hp_damage(int raw_target, int damage,
                                                int armor)
{
    return sc64_race_t4_ghost_hp_damage(raw_target, damage, armor);
}

/* Integer GBA policy for Explosive damage after shield absorption: subtract
 * armor, then Small 1/2, Medium 3/4, Large full, minimum one integer HP.
 * BW retains fractional HP; this bounded representation rounds down. */
static inline int sc64_t5_explosive_hp_damage(int raw_target, int damage,
                                               int armor)
{
    if (damage < 0) damage = 0;
    if (damage > 100000) damage = 100000;
    if (armor < 0) armor = 0;
    if (armor > 100000) armor = 100000;
    damage -= armor;
    if (damage < 1) return 1;
    int size = sc64_race_t4_source_size(raw_target);
    if (size == SC64_T4_SIZE_SMALL) damage /= 2;
    else if (size == SC64_T4_SIZE_MEDIUM) damage = damage * 3 / 4;
    return damage < 1 ? 1 : damage;
}

#endif /* SC64_T5_RUNTIME */
#endif
