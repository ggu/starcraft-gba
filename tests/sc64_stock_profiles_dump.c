/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include "../starcraft/sc64_stock_profiles.h"
#include "../starcraft/sc64_n64_stock_overrides.h"
#ifdef SC64_STOCK_UPGRADE_DUMP
#include "../starcraft/sc64_stock_upgrade_families.h"
#endif

static unsigned int unit_field(const Sc64StockUnitProfile *p, int field)
{
    const unsigned int v[] = { p->flags, p->ground_weapon, p->air_weapon,
        p->ground_hits, p->air_hits, p->size, p->armor, p->minerals, p->gas,
        p->build_frames, p->supply_required, p->supply_provided, p->hp,
        p->shields, p->top_speed_q10 };
    return field >= 0 && field < (int)(sizeof v / sizeof v[0]) ? v[field] : ~0u;
}

static unsigned int weapon_field(const Sc64StockWeaponProfile *p, int field)
{
    const unsigned int v[] = { p->damage, p->upgrade_bonus, p->cooldown,
        p->min_range, p->max_range, p->damage_factor, p->damage_class };
    return field >= 0 && field < (int)(sizeof v / sizeof v[0]) ? v[field] : ~0u;
}

int main(void)
{
    /* Every recorded N64 departure is what the table holds, and differs
     * from the BWAPI value it replaced. O rows let the facts check map
     * BWAPI expectations onto the table. */
    for (int i = 0; i < SC64_N64_STOCK_OVERRIDE_COUNT; ++i) {
        const Sc64StockOverride *o = &sc64_n64_stock_overrides[i];
        unsigned int held = o->table == SC64_STOCK_OVERRIDE_UNIT
            ? unit_field(sc64_stock_unit_profile(o->row), o->field)
            : weapon_field(sc64_stock_weapon_profile(o->row), o->field);
        /* W rows print damage_factor third; O rows name the W column. */
        static const unsigned char weapon_column[] = { 0, 1, 3, 4, 5, 2, 6 };
        unsigned column = o->table == SC64_STOCK_OVERRIDE_UNIT ? o->field :
            o->field < sizeof weapon_column ? weapon_column[o->field] : 255u;
        if (held != o->n64 || o->bwapi == o->n64 || column == 255u) return 4;
        printf("O,%u,%u,%u,%u,%u\n", o->table, o->row, column, o->bwapi, o->n64);
    }
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
