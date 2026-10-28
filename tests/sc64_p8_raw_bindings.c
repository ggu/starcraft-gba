/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "../starcraft/race_rules.h"

int main(void)
{
#ifdef SC64_P8_DATA_ENABLED
    assert(sc64_race_unit_from_raw(28) == U_P8_HYPERION);
    assert(sc64_race_raw_unit(U_P8_HYPERION) == 28);
    assert(sc64_race_unit_from_raw(12) != U_P8_HYPERION);
    assert(sc64_race_unit_rules[U_P8_HYPERION].hp == 850);
    assert(sc64_race_unit_rules[U_P8_HYPERION].producer == 255);
#else
    assert(sc64_race_unit_from_raw(28) == -1);
#endif
#if defined(SC64_P8_DATA_ENABLED) || defined(SC64_TERRAN_LATE_ACTORS)
    assert(sc64_race_building_from_raw(168) == B_STASIS_CELL_PRISON);
    assert(sc64_race_raw_building(B_STASIS_CELL_PRISON) == 168);
#else
    assert(sc64_race_building_from_raw(168) == -1);
#endif
    return 0;
}
