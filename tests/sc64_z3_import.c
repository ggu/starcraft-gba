/* Private generated-header fixture for the opt-in New Dominion import.
 * Compile with SC64_UNIFIED, SC64_Z3_ENABLED, SC64_CAMPAIGN_RACE=1. */
#include <assert.h>
#include <stddef.h>
#include "../starcraft/race_rules.h"
#include "sc64_races_generated.h"

int main(void)
{
    const Sc64RaceMap *m = &sc64_race_maps[14];
    unsigned int units = 0, buildings = 0, minerals = 0, geysers = 0;
    unsigned int i;

    assert(SC64_RACE_MAP_COUNT == 15);
    assert(m->width == 64 && m->height == 96);
    assert(m->title && m->mission_id);
    assert(m->unit_count == 122); /* two start markers excluded, nothing else */
    assert(m->human_slot == 3);
    assert(m->owner_modes[3] == 0 && m->owner_races[3] == R_ZERG);
    assert(m->owner_modes[6] == 1 && m->owner_races[6] == R_TER);

    for (i = 0; i < m->unit_count; ++i) {
        const Sc64ImportedUnit *u = &m->units[i];
        assert(u->x_px < m->width * 16u && u->y_px < m->height * 16u);
        if (u->source_type == 176 || u->source_type == 177 || u->source_type == 178) {
            ++minerals;
        } else if (u->source_type == 188) {
            ++geysers;
        } else if (sc64_race_building_from_raw(u->source_type) >= 0) {
            ++buildings;
        } else {
            ++units;
            assert(sc64_race_unit_from_raw(u->source_type) >= 0);
        }
    }
    assert(units == 58 && buildings == 33 && minerals == 28 && geysers == 3);
    assert(sc64_race_unit_from_raw(150) == -1);
    assert(sc64_race_building_from_raw(150) == B_MATURE_CHRYSALIS);
    assert(sc64_race_unit_from_raw(11) == U_DROPSHIP);
    for (i = 0; i < (unsigned int)m->width * m->height; ++i) {
        assert(m->tile_lookup[i] < m->tile_count);
        assert(m->blocked[i] <= 1);
    }
    /* No holes other than the deliberately unavailable global-ID interval. */
#ifdef SC64_T3_ENABLED
#if !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE == 0
    assert(sc64_race_maps[6].width != 0);
#else
    assert(sc64_race_maps[6].width == 0);
#endif
#else
    assert(sc64_race_maps[6].width == 0);
#endif
    for (i = 7; i <= 13; ++i) assert(sc64_race_maps[i].width == 0);
    return 0;
}
