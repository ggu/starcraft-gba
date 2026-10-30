/* Private-input integration contract; no map data is embedded in this test. */
#include <assert.h>
#include <stdio.h>
#include "race_rules.h"
#include "sc64_races_generated.h"

int main(void)
{
    const Sc64RaceMap *map = &sc64_race_maps[6];
    int units = 0, buildings = 0, minerals = 0, geysers = 0;
    int vultures = 0, engineering_bays = 0;
    assert(SC64_RACE_MAP_COUNT == 7);
    assert(map->width == 96 && map->height == 96);
    assert(map->human_slot == 1 && map->unit_count == 196);
    for (unsigned i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *row = &map->units[i];
        assert(row->x_px < map->width * 16 && row->y_px < map->height * 16);
        if (row->source_type >= 176 && row->source_type <= 178) ++minerals;
        else if (row->source_type == 188) ++geysers;
        else if (sc64_race_unit_from_raw(row->source_type) >= 0) ++units;
        else {
            assert(sc64_race_building_from_raw(row->source_type) >= 0);
            ++buildings;
        }
        if (row->source_type == 2) ++vultures;
        if (row->source_type == 122) ++engineering_bays;
    }
    assert(units == 138 && buildings == 31 && minerals == 25 && geysers == 2);
    assert(vultures == 3 && engineering_bays == 1);
    for (unsigned cell = 0; cell < map->width * map->height; ++cell) {
        assert(map->tile_lookup[cell] < map->tile_count);
        assert(map->blocked[cell] <= 3);
    }
    puts("T3 import: all 196 runtime rows mapped; 138 units, 31 buildings, 25 minerals, 2 geysers");
    return 0;
}
