/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include "../starcraft/sc64_stock_profiles.h"
#ifdef SC64_STOCK_UPGRADE_DUMP
#include "../starcraft/sc64_stock_upgrade_families.h"
#endif

int main(void)
{
    for (int i = 0; i < SC64_STOCK_UNIT_LIMIT; ++i) {
        const Sc64StockUnitProfile *p = sc64_stock_unit_profile(i);
        const Sc64StockUnitProfile *b = sc64_stock_building_profile(i);
        printf("U,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
               i, p->flags, p->ground_weapon, p->air_weapon,
               p->ground_hits, p->air_hits, p->size, p->armor, p->minerals,
               p->gas, p->build_frames, p->supply_required,
               p->supply_provided, p->hp, p->shields, p->top_speed_q10);
        if ((p->flags & SC64_STOCK_F_BUILDING) ? b != p : b != 0) return 1;
    }
    for (int i = 0; i < SC64_STOCK_WEAPON_ROWS; ++i) {
        const Sc64StockWeaponProfile *p = sc64_stock_weapon_profile(i);
        printf("W,%d,%u,%u,%u,%u,%u,%u,%u\n", i, p->damage,
               p->upgrade_bonus, p->damage_factor, p->cooldown,
               p->min_range, p->max_range, p->damage_class);
    }
#ifdef SC64_STOCK_UPGRADE_DUMP
    for (int i = 0; i < SC64_STOCK_UNIT_LIMIT; ++i)
        printf("A,%d,%d\n", i, sc64_stock_armor_upgrade_id(i));
    for (int i = 0; i < SC64_STOCK_WEAPON_ROWS; ++i)
        printf("F,%d,%d\n", i, sc64_stock_weapon_upgrade_id(i));
    if (sc64_stock_armor_upgrade_id(-1) != SC64_STOCK_UPGRADE_NONE ||
        sc64_stock_armor_upgrade_id(228) != SC64_STOCK_UPGRADE_NONE ||
        sc64_stock_weapon_upgrade_id(-1) != SC64_STOCK_UPGRADE_NONE ||
        sc64_stock_weapon_upgrade_id(SC64_STOCK_WEAPON_ROWS) != SC64_STOCK_UPGRADE_NONE)
        return 3;
#endif
    if (sc64_stock_unit_profile(-1) ||
        sc64_stock_unit_profile(SC64_STOCK_UNIT_LIMIT) ||
        sc64_stock_weapon_profile(-1) ||
        sc64_stock_weapon_profile(SC64_STOCK_WEAPON_ROWS)) return 2;
    return 0;
}
