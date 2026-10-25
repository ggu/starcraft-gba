/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STOCK_COMBAT_H
#define SC64_STOCK_COMBAT_H

#include "sc64_stock_profiles.h"

/* This table describes stock weapons; callers retain profile eligibility and
 * special-lifecycle checks. UnitType.maxGroundHits/maxAirHits and
 * WeaponType.damageFactor are distinct
 * source fields. A selected attack retains both; impact_count() combines them
 * only for callers that need a bounded list of individual damage impacts. */
typedef struct {
    const Sc64StockUnitProfile *unit;
    const Sc64StockWeaponProfile *weapon;
    unsigned char weapon_id, unit_hits, target_flying;
} Sc64StockAttack;

enum {
    SC64_STOCK_SIZE_INDEPENDENT = 0,
    SC64_STOCK_SIZE_SMALL = 1,
    SC64_STOCK_SIZE_MEDIUM = 2,
    SC64_STOCK_SIZE_LARGE = 3
};

static inline int sc64_stock_attack_select(int raw_unit, int target_flying,
                                           Sc64StockAttack *out)
{
    const Sc64StockUnitProfile *unit = sc64_stock_unit_profile(raw_unit);
    const Sc64StockWeaponProfile *weapon;
    int weapon_id, hits;
    if (!unit || !out) return 0;
    if (target_flying) {
        weapon_id = unit->air_weapon;
        hits = unit->air_hits;
    } else {
        weapon_id = unit->ground_weapon;
        hits = unit->ground_hits;
    }
    weapon = sc64_stock_weapon_profile(weapon_id);
    if (!weapon || weapon_id == 130 || !weapon->damage ||
        !weapon->damage_factor || !hits || !weapon->max_range) return 0;
    out->unit = unit;
    out->weapon = weapon;
    out->weapon_id = (unsigned char)weapon_id;
    out->unit_hits = (unsigned char)hits;
    out->target_flying = (unsigned char)!!target_flying;
    return 1;
}

/* Unit hit count and weapon factor remain separately available. This is the
 * aggregate number of individual weapon impacts if an adapter resolves them
 * separately for per-impact armor. */
static inline unsigned int sc64_stock_attack_impact_count(const Sc64StockAttack *a)
{
    if (!a || !a->weapon) return 0;
    return (unsigned int)a->unit_hits * a->weapon->damage_factor;
}

static inline int sc64_stock_attack_in_range(const Sc64StockAttack *a,
                                             unsigned int distance_pixels)
{
    return a && a->weapon && distance_pixels >= a->weapon->min_range &&
           distance_pixels <= a->weapon->max_range;
}

/* BWAPI's damage bonus is weapon-family specific. `upgrade_levels` is the
 * research level for this weapon family (bounded to the game's 0..3 range). */
static inline int sc64_stock_attack_damage_per_impact(const Sc64StockAttack *a,
                                                       int upgrade_levels)
{
    int levels;
    if (!a || !a->weapon) return 0;
    levels = upgrade_levels < 0 ? 0 : upgrade_levels > 3 ? 3 : upgrade_levels;
    return (int)a->weapon->damage +
           (int)a->weapon->upgrade_bonus * levels;
}

/* Apply armor first, then the stock damage-class/size ratio, with the engine's
 * existing minimum-one-HP policy for every positive hit. DamageFactor is
 * modeled as individual impacts by sc64_stock_attack_hp_damage(), so armor is
 * applied per factor and per UnitType hit. */
static inline int sc64_stock_hp_damage_after_armor(int damage, int armor,
                                                    int damage_class,
                                                    int target_size)
{
    int after_armor, numerator = 1, denominator = 1;
    if (damage <= 0) return 0;
    if (armor < 0) armor = 0;
    if (damage_class != SC64_STOCK_DMG_IGNORE_ARMOR) {
        after_armor = damage - armor;
        if (after_armor < 1) after_armor = 1;
    } else {
        after_armor = damage;
    }
    if (damage_class == SC64_STOCK_DMG_EXPLOSIVE) {
        if (target_size == SC64_STOCK_SIZE_SMALL) { numerator = 1; denominator = 2; }
        else if (target_size == SC64_STOCK_SIZE_MEDIUM) { numerator = 3; denominator = 4; }
    } else if (damage_class == SC64_STOCK_DMG_CONCUSSIVE) {
        if (target_size == SC64_STOCK_SIZE_MEDIUM) { numerator = 1; denominator = 2; }
        else if (target_size == SC64_STOCK_SIZE_LARGE) { numerator = 1; denominator = 4; }
    }
    after_armor = (after_armor * numerator) / denominator;
    return after_armor > 0 ? after_armor : 1;
}

static inline int sc64_stock_attack_hp_damage(const Sc64StockAttack *a,
                                               int target_raw,
                                               int target_armor,
                                               int upgrade_levels)
{
    const Sc64StockUnitProfile *target = sc64_stock_unit_profile(target_raw);
    int per_impact;
    unsigned int impacts;
    if (!a || !a->weapon || !target) return 0;
    per_impact = sc64_stock_hp_damage_after_armor(
        sc64_stock_attack_damage_per_impact(a, upgrade_levels), target_armor,
        a->weapon->damage_class, target->size);
    impacts = sc64_stock_attack_impact_count(a);
    if (impacts > 64u) impacts = 64u;
    return per_impact * (int)impacts;
}

#endif
