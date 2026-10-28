/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "../starcraft/race_rules.h"

int main(void)
{
#if defined(SC64_P8_DATA_ENABLED) || defined(SC64_TERRAN_LATE_ACTORS)
    assert(sc64_race_building_from_raw(168) == B_STASIS_CELL_PRISON);
    assert(sc64_race_raw_building(B_STASIS_CELL_PRISON) == 168);
#else
    assert(sc64_race_building_from_raw(168) == -1);
#endif
    return 0;
}
