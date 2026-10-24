#ifndef SC64_P6_COMBAT_H
#define SC64_P6_COMBAT_H

/* Narrow source-derived P6 combat primitives. These helpers model numeric
 * weapon policy only; they do not implement Brood War orders, animation,
 * projectiles, selection, or target acquisition. */
#include "../common/gba.h"
#include "sc64_p6_actor_rules.h"

enum {
    SC64_P6_EXPLOSION_DAMAGE = 500,
    SC64_P6_EXPLOSION_INNER_SOURCE_PX = 20,
    SC64_P6_EXPLOSION_MIDDLE_SOURCE_PX = 40,
    SC64_P6_EXPLOSION_OUTER_SOURCE_PX = 60,
    SC64_P6_WALL_TRAP_DAMAGE = 10,
    SC64_P6_WALL_TRAP_RANGE_SOURCE_PX = 160,
    SC64_P6_WALL_TRAP_COOLDOWN_TICKS = 22,
    SC64_P6_SIZE_SMALL = 0,
    SC64_P6_SIZE_MEDIUM = 1,
    SC64_P6_SIZE_LARGE = 2,
    SC64_P6_SIZE_INDEPENDENT = 3
};

enum {
    SC64_P6_SPLASH_NONE = 0,
    SC64_P6_SPLASH_INNER = 1,
    SC64_P6_SPLASH_MIDDLE = 2,
    SC64_P6_SPLASH_OUTER = 3
};

typedef struct {
    u32 hp_q8;
    u32 shields_q8;
    u32 armor_q8;
    u32 shield_armor_q8;
    u8 size;
    u8 has_shields;
    u8 invincible;
    u8 ground_target;
} Sc64P6DamageTarget;

typedef struct {
    u32 hp_damage_q8;
    u32 shield_damage_q8;
    u32 hp_after_q8;
    u32 shields_after_q8;
} Sc64P6DamageResult;

static inline int sc64_p6_damage_target_valid(const Sc64P6DamageTarget *target)
{
    return target && target->size <= 3 && target->has_shields <= 1 &&
           target->invincible <= 1 && target->ground_target <= 1 &&
           target->hp_q8 <= 0x7fffffffu && target->shields_q8 <= 0x7fffffffu &&
           target->armor_q8 <= 0x7fffffffu &&
           target->shield_armor_q8 <= 0x7fffffffu;
}

/* The source WeaponType uses target flags TARG_GROUND only. Caller supplies
 * distance from explosion center to the nearest point of the target bounds,
 * matching unit_distance_to; this is not center-to-center distance. */
static inline unsigned sc64_p6_infested_splash_tier(unsigned distance_source_px)
{
    if (distance_source_px <= SC64_P6_EXPLOSION_INNER_SOURCE_PX)
        return SC64_P6_SPLASH_INNER;
    if (distance_source_px <= SC64_P6_EXPLOSION_MIDDLE_SOURCE_PX)
        return SC64_P6_SPLASH_MIDDLE;
    if (distance_source_px <= SC64_P6_EXPLOSION_OUTER_SOURCE_PX)
        return SC64_P6_SPLASH_OUTER;
    return SC64_P6_SPLASH_NONE;
}

/* Source weapon_deal_damage arithmetic for the simple integer actor case:
 * Q8 values preserve half damage and its 0.5 HP minimum. Armor inputs are
 * effective armor after upgrades; matrix, acid-spore bonus, fractional shield
 * armor upgrades, and unit death side effects are deliberately caller-owned. */
static inline int sc64_p6_explosive_damage_q8(
    const Sc64P6DamageTarget *target, u32 base_damage, unsigned divisor,
    Sc64P6DamageResult *result)
{
    u32 damage, shield_damage = 0, hp_damage;
    if (!sc64_p6_damage_target_valid(target) || !result || !divisor ||
        divisor > 4 || base_damage > 0x7fffffu) return 0;
    result->hp_damage_q8 = 0;
    result->shield_damage_q8 = 0;
    result->hp_after_q8 = target->hp_q8;
    result->shields_after_q8 = target->shields_q8;
    if (!target->hp_q8 || target->invincible || !target->ground_target) return 1;

    /* Divide before scaling: the validated bound permits values whose Q8
     * product would overflow 32 bits. */
    damage = ((base_damage / divisor) * 256u) +
             (((base_damage % divisor) * 256u) / divisor);
    if (target->has_shields && target->shields_q8 >= 256u) {
        if (damage > target->shield_armor_q8)
            damage -= target->shield_armor_q8;
        else
            damage = 128u;
        shield_damage = damage < target->shields_q8 ? damage : target->shields_q8;
        damage -= shield_damage;
    }
    if (damage > target->armor_q8) damage -= target->armor_q8;
    else damage = 0;

    /* Canonical BW size chart expressed in the P6 fact table's normalized
     * order: Small=0, Medium=1, Large=2, Independent=3. OpenBW's internal
     * unit_size ordinals are implementation-specific and are not accepted
     * directly here. */
    if (target->size == SC64_P6_SIZE_SMALL) damage /= 2u;
    else if (target->size == SC64_P6_SIZE_MEDIUM)
        damage = (damage / 4u) * 3u + ((damage % 4u) * 3u) / 4u;
    else if (target->size == SC64_P6_SIZE_INDEPENDENT) damage = 0;
    if (!shield_damage && damage < 128u) damage = 128u;

    hp_damage = damage < target->hp_q8 ? damage : target->hp_q8;
    result->hp_damage_q8 = hp_damage;
    result->shield_damage_q8 = shield_damage;
    result->hp_after_q8 = target->hp_q8 - hp_damage;
    result->shields_after_q8 = target->shields_q8 - shield_damage;
    return 1;
}

static inline int sc64_p6_infested_explosion_damage(
    const Sc64P6DamageTarget *target, unsigned distance_source_px,
    Sc64P6DamageResult *result)
{
    unsigned tier = sc64_p6_infested_splash_tier(distance_source_px);
    unsigned divisor;
    if (!sc64_p6_damage_target_valid(target) || !result) return 0;
    if (!tier) {
        result->hp_damage_q8 = result->shield_damage_q8 = 0;
        result->hp_after_q8 = target->hp_q8;
        result->shields_after_q8 = target->shields_q8;
        return 1;
    }
    divisor = tier == SC64_P6_SPLASH_INNER ? 1u :
              tier == SC64_P6_SPLASH_MIDDLE ? 2u : 4u;
    return sc64_p6_explosive_damage_q8(target, SC64_P6_EXPLOSION_DAMAGE,
                                       divisor, result);
}

/* OpenBW's burrowed-target rule is unusual: only a ground unit in the inner
 * 20-source-pixel impact band can be hit. Other splash rings skip it. */
static inline int sc64_p6_infested_burrowed_targetable(
    unsigned distance_source_px, int burrowed)
{
    return !burrowed || distance_source_px <= SC64_P6_EXPLOSION_INNER_SOURCE_PX;
}

/* Dark Templar and Zeratul use the native requires-detector status rather than
 * an energy toggle. detected_mask is the attacker's native detected_flags. */
static inline int sc64_p6_requires_detector(int raw_type)
{
    return raw_type == SC64_P6_RAW_HERO_DARK_TEMPLAR ||
           raw_type == SC64_P6_RAW_ZERATUL;
}

static inline int sc64_p6_cloaked_target_detected(int raw_type,
                                                   unsigned viewer_owner,
                                                   u32 detected_mask)
{
    if (!sc64_p6_requires_detector(raw_type)) return 1;
    if (viewer_owner >= 32u) return 0;
    return (detected_mask & ((u32)1u << viewer_owner)) != 0;
}

/* Wall Missile Trap weapon facts: ground-only, 160 source-pixel range,
 * 10 Explosive damage, cooldown 22. Hostility/visibility are inputs because
 * they are resolved by the owner's acquisition path, not WeaponType flags. */
static inline int sc64_p6_wall_trap_can_target(int alive, int enemy,
                                               int flying, int invincible,
                                               int detected, unsigned distance_source_px)
{
    return alive && enemy && !flying && !invincible && detected &&
           distance_source_px <= SC64_P6_WALL_TRAP_RANGE_SOURCE_PX;
}

static inline int sc64_p6_wall_trap_fire(u16 *cooldown)
{
    if (!cooldown || *cooldown) return 0;
    *cooldown = SC64_P6_WALL_TRAP_COOLDOWN_TICKS;
    return 1;
}

static inline void sc64_p6_wall_trap_cooldown_tick(u16 *cooldown)
{
    if (cooldown && *cooldown) --*cooldown;
}

#endif
