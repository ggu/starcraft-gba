/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_stock_profiles.h"

int main(void)
{
    unsigned checked = 0;
    for (int type = 0; type < SC_RB_COUNT; ++type) {
        int raw = sc64_race_raw_building(type);
        /* Scenario props and doodads occupy Building slots too, but their
           stock UnitType rows intentionally omit the Building flag. Compare
           every mapped raw fact rather than filtering by that metadata. */
        const Sc64StockUnitProfile *stock = sc64_stock_unit_profile(raw);
        assert(raw < 0 || stock); /* Every mapped identity must have a source row. */
        if (!stock) continue;
        assert(sc64_race_building_max_hp(type) == (int)stock->hp);
        assert(sc64_race_building_shields(type) == stock->shields);
        assert(sc64_race_building_armor(type) == stock->armor);
        ++checked;
    }
    /* Each compilation variant has common stock buildings. Optional variants
       also exercise their appended profile identities through the same loop. */
    assert(checked >= 25);
    return 0;
}
