/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "../common/gba.h"
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_stock_profiles.h"

int main(void)
{
    int raw = sc64_race_raw_unit(U_RAYNOR);
    const Sc64StockUnitProfile *stock = sc64_stock_unit_profile(raw);
    const Sc64StockWeaponProfile *ground;
    const Sc64StockWeaponProfile *air;
    assert(stock);
    assert(raw ==
#ifdef SC64_T4_RUNTIME
           19
#else
           20
#endif
    );
    assert(sc64_race_unit_rule(U_RAYNOR)->hp == stock->hp);
    assert(sc64_race_unit_rule(U_RAYNOR)->shields == stock->shields);
    assert(sc64_race_unit_armor(U_RAYNOR) == stock->armor);
    assert(sc64_race_unit_can_attack_air(U_RAYNOR) ==
           (stock->air_weapon != 130));
    ground = sc64_stock_weapon_profile(stock->ground_weapon);
    air = sc64_stock_weapon_profile(stock->air_weapon);
    assert(ground && air);
    assert(sc64_race_unit_damage_vs(U_RAYNOR, 0) == ground->damage);
    assert(sc64_race_unit_range_vs(U_RAYNOR, 0) == ground->max_range);
    assert(sc64_race_unit_cooldown_vs(U_RAYNOR, 0) == ground->cooldown);
    if (stock->air_weapon == 130) {
        assert(sc64_race_unit_damage_vs(U_RAYNOR, 1) == 0);
        assert(sc64_race_unit_range_vs(U_RAYNOR, 1) == 0);
        assert(sc64_race_unit_cooldown_vs(U_RAYNOR, 1) == 0);
    } else {
        assert(sc64_race_unit_damage_vs(U_RAYNOR, 1) == air->damage);
        assert(sc64_race_unit_range_vs(U_RAYNOR, 1) == air->max_range);
        assert(sc64_race_unit_cooldown_vs(U_RAYNOR, 1) == air->cooldown);
    }
    assert(sc64_race_building_max_hp(B_ZERG_BEACON) == 100000);
    return 0;
}
