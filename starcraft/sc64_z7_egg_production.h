/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Z7_EGG_PRODUCTION_H
#define SC64_Z7_EGG_PRODUCTION_H

#ifdef SC64_Z7_EGG_PRODUCTION
#ifndef SC64_RACES
#error "Z7 Egg production requires SC64_RACES"
#endif
#ifndef SC64_Z7_RUNTIME
#error "Z7 Egg production requires SC64_Z7_RUNTIME"
#endif

/* Only ordinary Zerg larva products become Egg jobs. The Egg's own rules
 * intentionally have no larva flag or producer, so this cannot recurse. */
static inline int sc64_z7_egg_product_valid(int type)
{
    const Sc64RaceUnitRule *rule = sc64_race_unit_rule(type);
    return rule && rule->race == R_ZERG &&
           (rule->flags & SC_RACE_F_LARVA) &&
           rule->producer == B_HATCHERY && rule->build_frames > 0;
}

static inline int sc64_z7_egg_product_count(int type)
{ return type == U_ZERGLING || type == U_SCOURGE ? 2 : 1; }

/* Runtime supply uses the same half-unit convention as Unit.supply. A pair
 * of Zerglings reserves both output slots' supply during Egg development. */
static inline int sc64_z7_egg_reserved_supply(int type)
{
    const Sc64RaceUnitRule *rule = sc64_race_unit_rule(type);
    return sc64_z7_egg_product_valid(type) ?
           (int)rule->supply * sc64_z7_egg_product_count(type) : -1;
}

static inline int sc64_z7_egg_duration(int type)
{
    const Sc64RaceUnitRule *rule = sc64_race_unit_rule(type);
    return sc64_z7_egg_product_valid(type) ?
           ((int)rule->build_frames * 5 + 3) / 4 : -1;
}
#endif

#endif
