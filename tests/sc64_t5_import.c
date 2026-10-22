/* Private-input import fixture for the T5 data-only stage.
 * Build only after tools/sc64_races_pack.py has generated the ignored header.
 */
#include <assert.h>
#include <string.h>
#include "sc64_races_generated.h"

static int is_mineral(unsigned type) {
    return type == 176 || type == 177 || type == 178;
}

static int is_building(unsigned type) {
    switch (type) {
        case 106: case 109: case 110: case 111: case 112: case 113: case 114:
        case 122: case 124: case 125: case 130: case 131: case 132: case 133:
        case 135: case 137: case 139: case 141: case 142: case 143: case 144:
        case 146: case 149: case 150: case 151: case 154: case 155: case 156:
        case 157: case 160: case 162: case 164: case 166: case 167: case 195:
            return 1;
        default:
            return 0;
    }
}

static unsigned long long hash_byte(unsigned long long hash, unsigned value) {
    return (hash ^ (value & 0xffu)) * 1099511628211ull;
}

static unsigned long long hash_u16(unsigned long long hash, unsigned value) {
    hash = hash_byte(hash, value);
    return hash_byte(hash, value >> 8);
}

int main(void) {
    assert(SC64_RACE_MAP_COUNT == 9);
    const Sc64RaceMap *map = &sc64_race_maps[8];
    assert(map->width == 96 && map->height == 96);
    assert(map->tileset_id == 0 && map->human_slot == 0);
    assert(strcmp(map->mission_id, "T5") == 0);
    assert(map->unit_count == 168);
    assert(map->units != 0 && map->tile_lookup != 0 && map->blocked != 0);
    assert(map->tile_count != 0 && map->title != 0);
    assert(map->owner_modes[0] == 0); /* source OWNR slot 0 is the human */
    assert(map->owner_modes[2] == 3); /* raw OWNR 3 remains rescuable */
    assert(map->owner_modes[6] == 1); /* source OWNR slot 6 is computer */

    unsigned owners[12] = {0};
    unsigned raw_types[256] = {0};
    unsigned minerals = 0, buildings = 0, beacon195 = 0;
    unsigned long long roster_hash = 14695981039346656037ull;
    for (unsigned i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *unit = &map->units[i];
        assert(unit->source_type != 214); /* all start-location markers excluded */
        assert(unit->x_px < map->width * 16u && unit->y_px < map->height * 16u);
        assert(unit->owner < 12);
        ++owners[unit->owner];
        ++raw_types[unit->source_type];
        minerals += is_mineral(unit->source_type);
        buildings += is_building(unit->source_type);
        beacon195 += unit->source_type == 195;
        roster_hash = hash_u16(roster_hash, unit->x_px);
        roster_hash = hash_u16(roster_hash, unit->y_px);
        roster_hash = hash_u16(roster_hash, unit->source_type);
        roster_hash = hash_byte(roster_hash, unit->owner);
        roster_hash = hash_byte(roster_hash, unit->hp_percent);
        roster_hash = hash_u16(roster_hash, unit->flags);
        roster_hash = hash_u16(roster_hash, unit->resources);
    }
    assert(owners[6] == 78 && owners[3] == 39 && owners[11] == 41);
    assert(owners[0] == 8 && owners[2] == 2);
    assert(minerals == 41 && buildings == 48 && beacon195 == 1);
    assert(is_building(195));
    /* Independently computed from all 172 framed source UNIT records
     * (payload SHA256 0d0f95f646b412172a9f54b7c237e13b245dc967ed2896f9e3a180ff88500426)
     * with tools/sc64_import.py::unit_rows, excluding only the four raw-214
     * starts. It covers every emitted field in canonical little-endian order. */
    assert(roster_hash == 0xd742e1079b3e79a3ull);
    assert(raw_types[0] == 41 && raw_types[177] == 16 && raw_types[178] == 14);
    assert(raw_types[109] == 12 && raw_types[124] == 11 && raw_types[125] == 10);
    assert(raw_types[195] == 1);

    assert(map->tile_count == 1138);
    unsigned blocked = 0;
    unsigned long long terrain_hash = 14695981039346656037ull;
    unsigned cells = (unsigned)map->width * map->height;
    for (unsigned i = 0; i < cells; ++i) {
        assert(map->tile_lookup[i] < map->tile_count);
        assert(map->blocked[i] <= 1);
        blocked += map->blocked[i] != 0;
        terrain_hash = hash_u16(terrain_hash, map->tile_lookup[i]);
    }
    for (unsigned i = 0; i < cells; ++i)
        terrain_hash = hash_byte(terrain_hash, map->blocked[i]);
    for (unsigned i = 0; i < (unsigned)map->tile_count * 256; ++i)
        terrain_hash = hash_byte(terrain_hash,
                                 ((const unsigned char *)map->tile_pixels)[i]);
    assert(blocked == 3979);
    /* Converted output comes from the isolated generated header. Its source
     * MTXM framed-record SHA256 is
     * 5edc520c1e669bf67047ba075aad4c672353e91815ee853208d610b139139cb6. */
    assert(terrain_hash == 0xc2941cacca7d9c2cull);
    return 0;
}
