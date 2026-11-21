/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_COMBAT_FACTS_H
#define SC64_COMBAT_FACTS_H

enum {
    SC64_COMBAT_HIDDEN = 1u,
    SC64_COMBAT_FLYING = 2u,
    SC64_COMBAT_GROUND_WEAPON = 4u,
    SC64_COMBAT_AIR_WEAPON = 8u
};

static inline unsigned sc64_combat_target_facts_reference(int type)
{
    return ((sc64_game_unit_actor_flags(type) & SC64_GAME_ACTOR_HIDDEN) ?
            SC64_COMBAT_HIDDEN : 0u) |
           (sc64_race_unit_flying(type) ? SC64_COMBAT_FLYING : 0u);
}

#ifndef SC64_FACT_TABLE_GENERATOR
#if !defined(SC64_RACES) || !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "Combat facts require the Z/P campaign actor rules"
#endif
#include "sc64_fact_tables_generated.h"
#if !defined(SC64_COMBAT_FACT_TABLE_REVISION) || SC64_COMBAT_FACT_TABLE_REVISION != 1
#error "Regenerate fact tables with SC64_COMBAT_FACT_TABLE_PROBE"
#endif

/* Only type facts are stored in ROM. Eligibility still reads live actor
 * types, status, cloaking, sight, and building flight in the original path. */
static inline unsigned sc64_combat_target_facts(int type)
{
    if ((unsigned)type < SC_RU_COUNT) return sc64_fact_combat_table[type];
    return sc64_combat_target_facts_reference(type);
}

static inline int sc64_combat_ground_weapon(int type)
{
    if ((unsigned)type < SC_RU_COUNT)
        return (sc64_fact_combat_table[type] & SC64_COMBAT_GROUND_WEAPON) != 0;
    return sc64_race_unit_damage(type) > 0;
}

static inline int sc64_combat_air_weapon(int type)
{
    if ((unsigned)type < SC_RU_COUNT)
        return (sc64_fact_combat_table[type] & SC64_COMBAT_AIR_WEAPON) != 0;
    return sc64_race_unit_can_attack_air(type);
}
#endif

#endif
