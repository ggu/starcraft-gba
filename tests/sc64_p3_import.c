/* Private import fixture. Compile with unified P3-enabled Protoss flags. */
#include <assert.h>
#include <stddef.h>
#include "../starcraft/race_rules.h"
#include "sc64_races_generated.h"

int main(void)
{
    const Sc64RaceMap *m = &sc64_race_maps[22];
    unsigned int units = 0, buildings = 0, minerals = 0, geysers = 0;
    unsigned int scourges = 0, hives = 0, greater_spires = 0;
    unsigned int force2_units = 0, force3_units = 0;
    unsigned int i;

    assert(SC64_RACE_MAP_COUNT == 23);
    assert(m->width == 128 && m->height == 96);
    assert(m->unit_count == 189); /* all 192 source UNIT rows minus 3 starts */
    assert(m->human_slot == 7);
    assert(m->owner_modes[7] == 0 && m->owner_races[7] == R_PROTOSS);
    assert(m->owner_modes[0] == 1 && m->owner_races[0] == R_ZERG);
    assert(m->owner_modes[6] == 1 && m->owner_races[6] == R_ZERG);

    for (i = 0; i < m->unit_count; ++i) {
        const Sc64ImportedUnit *u = &m->units[i];
        assert(u->x_px < m->width * 16u && u->y_px < m->height * 16u);
        if (u->source_type >= 176 && u->source_type <= 178) {
            ++minerals;
        } else if (u->source_type == 188) {
            ++geysers;
        } else if (sc64_race_building_from_raw(u->source_type) >= 0) {
            ++buildings;
            if (u->source_type == 133) ++hives;
            if (u->source_type == 137) ++greater_spires;
        } else {
            ++units;
            assert(sc64_race_unit_from_raw(u->source_type) >= 0);
            if (u->source_type == 47) ++scourges;
        }
        if (u->owner == 0) ++force2_units;
        if (u->owner == 6) ++force3_units;
    }
    assert(units == 96 && buildings == 42 && minerals == 48 && geysers == 3);
    assert(scourges == 8 && hives == 1 && greater_spires == 2);
    assert(force2_units > 0 && force3_units > 0); /* source slots remain distinct */
    assert(sc64_race_raw_unit(U_SCOURGE) == 47);
    assert(sc64_race_unit_from_raw(47) == U_SCOURGE);
    assert(sc64_race_building_from_raw(133) == B_HIVE);
    assert(sc64_race_building_from_raw(137) == B_GREATER_SPIRE);
    assert(sc64_race_unit_from_raw(150) == -1);
    for (i = 0; i < (unsigned int)m->width * m->height; ++i) {
        assert(m->tile_lookup[i] < m->tile_count);
        assert(m->blocked[i] <= 3);
    }
    assert(sc64_race_maps[14].width == 0); /* Z3 disabled in this fixture */
    for (i = 15; i <= 21; ++i) assert(sc64_race_maps[i].width == 0);
    return 0;
}
