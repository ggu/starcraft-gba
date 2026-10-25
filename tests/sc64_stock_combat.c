/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "../starcraft/sc64_stock_combat.h"

int main(void)
{
    Sc64StockAttack attack;

    assert(sc64_stock_attack_select(0, 0, &attack)); /* Marine */
    assert(attack.weapon->damage == 6);
    assert(attack.weapon->upgrade_bonus == 1);
    assert(attack.unit_hits == 1 && attack.weapon->damage_factor == 1);
    assert(sc64_stock_attack_damage_per_impact(&attack, 3) == 9);
    assert(sc64_stock_attack_hp_damage(&attack, 0, 1, 0) == 5);

    assert(sc64_stock_attack_select(3, 1, &attack)); /* Goliath air */
    assert(attack.weapon->damage == 10);
    assert(attack.unit_hits == 1 && attack.weapon->damage_factor == 2);
    assert(sc64_stock_attack_impact_count(&attack) == 2);
    assert(sc64_stock_attack_hp_damage(&attack, 5, 1, 0) == 18);
    assert(sc64_stock_attack_in_range(&attack, 160));
    assert(!sc64_stock_attack_in_range(&attack, 161));

    assert(sc64_stock_attack_select(65, 0, &attack)); /* Zealot blades */
    assert(attack.unit_hits == 2 && attack.weapon->damage_factor == 1);
    assert(sc64_stock_attack_hp_damage(&attack, 0, 1, 0) == 14);

    assert(sc64_stock_attack_select(32, 0, &attack)); /* Firebat */
    assert(attack.unit_hits == 3);
    assert(sc64_stock_attack_hp_damage(&attack, 5, 2, 0) == 3);

    assert(sc64_stock_attack_select(30, 0, &attack)); /* Siege Mode Tank */
    assert(attack.weapon->damage == 70);
    assert(attack.weapon->min_range == 64 && attack.weapon->max_range == 384);
    assert(!sc64_stock_attack_in_range(&attack, 63));
    assert(sc64_stock_attack_in_range(&attack, 64));
    assert(sc64_stock_attack_in_range(&attack, 384));
    assert(!sc64_stock_attack_in_range(&attack, 385));

    assert(sc64_stock_attack_select(47, 1, &attack)); /* Scourge */
    assert(attack.weapon->damage_class == SC64_STOCK_DMG_NORMAL);
    assert(sc64_stock_attack_hp_damage(&attack, 0, 100, 0) == 10);

    assert(!sc64_stock_attack_select(64, 1, &attack)); /* Probe has no weapon */
    assert(!sc64_stock_attack_select(0, 0, 0));
    assert(sc64_stock_hp_damage_after_armor(0, 0,
           SC64_STOCK_DMG_NORMAL, SC64_STOCK_SIZE_SMALL) == 0);
    assert(sc64_stock_hp_damage_after_armor(1, 100,
           SC64_STOCK_DMG_NORMAL, SC64_STOCK_SIZE_SMALL) == 1);
    assert(sc64_stock_hp_damage_after_armor(8, 0,
           SC64_STOCK_DMG_EXPLOSIVE, SC64_STOCK_SIZE_SMALL) == 4);
    assert(sc64_stock_hp_damage_after_armor(8, 0,
           SC64_STOCK_DMG_CONCUSSIVE, SC64_STOCK_SIZE_LARGE) == 2);
    return 0;
}
