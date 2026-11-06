/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Family witnesses independently checked in owned N64 unit table 00C/001
 * and weapon table 00C/007. The separate 00C/006 table supplies prices.
 * These values are not a claim about native timing or campaign permissions. */
#include <assert.h>
#include "../starcraft/sc64_stock_combat.h"
#include "../starcraft/sc64_stock_upgrade_families.h"

int main(void)
{
    Sc64StockAttack attack;
    /* Dragoon air attacks still use Protoss Ground Weapons. */
    for (int air = 0; air <= 1; ++air) {
        assert(sc64_stock_attack_select(66, air, &attack));
        assert(attack.weapon_id == 66);
        assert(sc64_stock_weapon_upgrade_id(attack.weapon_id) == 13);
        assert(sc64_stock_attack_damage_per_impact(&attack, 3) == 26);
        assert(sc64_stock_attack_select(70, air, &attack));
        assert(attack.weapon_id == (air ? 74 : 73));
        assert(sc64_stock_weapon_upgrade_id(attack.weapon_id) == 14);
    }
    assert(sc64_stock_weapon_upgrade_id(79) == 14); /* Interceptor */
    assert(sc64_stock_weapon_upgrade_id(82) == 35); /* Scarab, not ground weapons */
    assert(sc64_stock_weapon_upgrade_id(62) == 60); /* Probe, not ground weapons */
    assert(sc64_stock_armor_upgrade_id(64) == 5);
    assert(sc64_stock_armor_upgrade_id(66) == 5);
    assert(sc64_stock_armor_upgrade_id(70) == 6);
    assert(sc64_stock_armor_upgrade_id(73) == 6);
    assert(sc64_stock_armor_upgrade_id(83) == 5);
    return 0;
}
