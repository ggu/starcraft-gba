/* Private-input T6 import fixture. Requires the locally generated race header. */
#include <assert.h>
#include <string.h>
#include "sc64_races_generated.h"
#include "race_rules.h"
#include "sc64_race_sprites_generated.h"

static int mineral(unsigned type) {
    return type == 176 || type == 177 || type == 178;
}

static int source_building(unsigned type) {
    switch (type) {
        case 106: case 109: case 110: case 111: case 112: case 113: case 114:
        case 122: case 124: case 125: case 126: case 130: case 131: case 132:
        case 133: case 135: case 137: case 138: case 139: case 141: case 142:
        case 143: case 144: case 146: case 149: case 150: case 151: case 154:
        case 155: case 156: case 157: case 160: case 162: case 164: case 166:
        case 167:
            return 1;
        default:
            return 0;
    }
}

int main(void) {
#if !defined(SC64_UNIFIED) || !defined(SC64_T6_ENABLED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 0
#error "compile this fixture for the opt-in Terran T6 import profile"
#endif
    assert(SC64_RACE_MAP_COUNT == 10);
    const Sc64RaceMap *map = &sc64_race_maps[9];
    assert(map->width == 96 && map->height == 96);
    assert(map->tileset_id == 0 && map->human_slot == 0);
    assert(strcmp(map->mission_id, "T6") == 0);
    assert(map->unit_count == 216);
    assert(map->units && map->tile_lookup && map->blocked && map->tile_pixels);
    assert(map->owner_modes[0] == 0); /* source slot 0: human */
    assert(map->owner_modes[1] == 1 && map->owner_modes[4] == 1);
    assert(map->owner_modes[2] == 3); /* source slot 2 remains rescuable */

    unsigned source_units = 0, buildings = 0, minerals = 0, geysers = 0;
    unsigned mapped_units = 0, mapped_buildings = 0;
    unsigned norad = 0, queens_nest = 0, beacon = 0, starts = 0;
    for (unsigned i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *u = &map->units[i];
        assert(u->source_type != 214);
        assert(u->x_px < map->width * 16u && u->y_px < map->height * 16u);
        source_units += !mineral(u->source_type) && u->source_type != 188 &&
                        !source_building(u->source_type) && u->source_type != 195;
        buildings += source_building(u->source_type);
        minerals += mineral(u->source_type);
        geysers += u->source_type == 188;
        norad += u->source_type == 126;
        queens_nest += u->source_type == 138;
        beacon += u->source_type == 195;
        starts += u->source_type == 214;
        if (!mineral(u->source_type) && u->source_type != 188) {
            int unit = sc64_race_unit_from_raw(u->source_type);
            int building = sc64_race_building_from_raw(u->source_type);
            assert((unit >= 0) != (building >= 0));
            mapped_units += unit >= 0;
            mapped_buildings += building >= 0;
        }
        assert(sc64_race_unit_sprite(u->source_type));
    }
    /* Source categories count raw195 in UNIT, while runtime maps it into the
     * building pool. Preserve the full roster and report those categories
     * separately rather than dropping or aliasing the actor. */
    assert(source_units == 104); /* mapped ordinary units, excluding raw195 */
    assert(source_units + beacon == 105); /* source UNIT-category count */
    assert(buildings == 73 && buildings + beacon == 74); /* mapped pool count */
    assert(mapped_units == 104 && mapped_buildings == 74);
    assert(minerals == 35 && geysers == 3);
    assert(norad == 1 && queens_nest == 1 && beacon == 1 && starts == 0);
    return 0;
}
