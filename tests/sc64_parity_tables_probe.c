/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Engine side of tools/sc64_parity_tables.py: print the rule values the
 * engine reads, one CSV row per record, for the profile flags it is built
 * with. Read-only: it includes headers and calls their pure helpers.
 *
 * Without PARITY_RULES it prints the raw-ID tables shared by every profile:
 *   U  stock unit profile      W  stock weapon profile
 *   S  sight tiles             A/F  armour and weapon upgrade family
 *   Q  Protoss research quote (family, completed level)
 * With PARITY_RULES (and a profile's -D flags) it prints that profile's race
 * rules by engine enum:
 *   R  unit rule               B  building rule
 * PARITY_NAMES is a generated header with the enum names of that profile.
 */
#include <stdio.h>
#ifdef PARITY_RULES
#include "../common/gba.h"
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_stock_combat.h"
#include PARITY_NAMES

static void unit_attack(int type, int raw, int flying)
{
    Sc64StockAttack stock;
    /* Target-class eligibility gates both paths; then game.c takes the stock
     * weapon when one is selected for the raw unit and falls back to these
     * race-rule values otherwise. An ineligible class prints as no attack. */
    if (!sc64_race_unit_can_attack_target_class(type, flying)) {
        printf(",0,0,0,0,0");
        return;
    }
    printf(",%d,%d,%d,%d,%d", sc64_stock_attack_select(raw, flying, &stock),
           sc64_race_unit_damage_vs(type, flying),
           sc64_race_unit_cooldown_vs(type, flying),
           sc64_race_unit_range_vs(type, flying),
           sc64_race_unit_hits_vs(type, flying));
}

int main(void)
{
    for (int i = 0; i < SC_RU_COUNT; ++i) {
        const Sc64RaceUnitRule *r = sc64_race_unit_rule(i);
        int raw = sc64_race_raw_unit(i), hp = r->hp, shields = r->shields;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        /* The unit constructor in game.c applies this HP override; campaign
         * actor facts carry the shield maximum the adapters regenerate to. */
        if (sc64_zp_actor_default_hp(i)) hp = sc64_zp_actor_default_hp(i);
        if (sc64_zp_actor_fact(i)) shields = sc64_zp_actor_fact(i)->shields;
#endif
        printf("R,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               parity_unit_names[i] ? parity_unit_names[i] : "", i, raw,
               r->minerals, r->gas, r->build_frames, r->supply,
               sc64_race_unit_supply_provided(i), hp, shields,
               sc64_race_unit_armor(i), sc64_race_unit_flying(i));
        unit_attack(i, raw, 0);
        unit_attack(i, raw, 1);
        printf("\n");
    }
    for (int i = 0; i < SC_RB_COUNT; ++i) {
        const Sc64RaceBuildingRule *r = sc64_race_building_rule(i);
        int hp = sc64_race_building_max_hp(i);
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        /* The building constructor in game.c applies this override. */
        if (sc64_zp_building_default_hp(i)) hp = sc64_zp_building_default_hp(i);
#endif
        printf("B,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
               parity_building_names[i] ? parity_building_names[i] : "", i,
               sc64_race_raw_building(i), r->minerals, r->gas,
               r->build_frames, r->supply_provided, hp,
               sc64_race_building_shields(i), sc64_race_building_armor(i));
    }
    return 0;
}
#else
#include "../starcraft/sc64_stock_profiles.h"
#include "../starcraft/sc64_stock_upgrade_families.h"
#include "../starcraft/sc64_source_sight_tiles.h"
#include "../starcraft/sc64_protoss_research.h"

int main(void)
{
    for (int i = 0; i < SC64_STOCK_UNIT_LIMIT; ++i) {
        const Sc64StockUnitProfile *p = sc64_stock_unit_profile(i);
        printf("U,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
               i, p->flags, p->ground_weapon, p->air_weapon,
               p->ground_hits, p->air_hits, p->size, p->armor, p->minerals,
               p->gas, p->build_frames, p->supply_required,
               p->supply_provided, p->hp, p->shields, p->top_speed_q10);
        printf("S,%d,%d\n", i, sc64_source_sight_raw_pixels(i) / 32);
        printf("A,%d,%d\n", i, sc64_stock_armor_upgrade_id(i));
    }
    for (int i = 0; i < SC64_STOCK_WEAPON_ROWS; ++i) {
        const Sc64StockWeaponProfile *p = sc64_stock_weapon_profile(i);
        printf("W,%d,%u,%u,%u,%u,%u,%u,%u\n", i, p->damage,
               p->upgrade_bonus, p->damage_factor, p->cooldown,
               p->min_range, p->max_range, p->damage_class);
        printf("F,%d,%d\n", i, sc64_stock_weapon_upgrade_id(i));
    }
    for (unsigned family = 0; family < SC64_PR_FAMILY_COUNT; ++family)
        for (unsigned level = 0; level < 3; ++level) {
            Sc64ProtossResearchQuote q;
            if (!sc64_pr_quote(family, level, &q)) continue;
            printf("Q,%u,%u,%u,%u,%u,%u\n", family, level, q.mineral, q.gas,
                   q.ticks, q.max_level);
        }
    return 0;
}
#endif
