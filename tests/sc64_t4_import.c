/* Private source-data fixture for the opt-in T4 import header.
 * Compile with SC64_UNIFIED, SC64_T4_ENABLED, SC64_CAMPAIGN_RACE=0 and
 * GBA_HOST. It checks imported CHK rows/geometry only; this is not a runtime
 * or campaign-playability claim. */
#include <assert.h>
#include <stddef.h>
#include "../.local/sc64/races/generated/sc64_races_generated.h"

static unsigned count_type(const Sc64RaceMap *map, unsigned type)
{
    unsigned count = 0;
    for (unsigned i = 0; i < map->unit_count; ++i)
        count += map->units[i].source_type == type;
    return count;
}

static unsigned count_type_owner(const Sc64RaceMap *map, unsigned type,
                                 unsigned owner)
{
    unsigned count = 0;
    for (unsigned i = 0; i < map->unit_count; ++i)
        count += map->units[i].source_type == type && map->units[i].owner == owner;
    return count;
}

static unsigned long long hash_byte(unsigned long long hash, unsigned value)
{
    return (hash ^ (value & 0xffu)) * 1099511628211ull;
}

static unsigned long long hash_u16(unsigned long long hash, unsigned value)
{
    hash = hash_byte(hash, value);
    return hash_byte(hash, value >> 8);
}

int main(void)
{
    assert(SC64_RACE_MAP_COUNT == 8);
    const Sc64RaceMap *map = &sc64_race_maps[7];
    assert(map->width == 128 && map->height == 128);
    assert(map->tileset_id == 2 && map->human_slot == 0);
    assert(map->unit_count == 95);

    static const unsigned types[] = {0, 1, 2, 3, 7, 15, 20, 32, 37, 89, 95, 195};
    static const unsigned counts[] = {27, 1, 2, 1, 6, 31, 1, 8, 5, 2, 3, 8};
    for (unsigned i = 0; i < sizeof(types) / sizeof(types[0]); ++i)
        assert(count_type(map, types[i]) == counts[i]);
    assert(count_type(map, 214) == 0); /* Seven start markers are not actors. */

    /* Source slots are retained as owners. Raw 195 stays raw 195 and is not
     * guessed into an ordinary-building mapping by the importer. */
    assert(count_type_owner(map, 0, 0) == 7 && count_type_owner(map, 0, 6) == 20);
    assert(count_type_owner(map, 15, 4) == 1 && count_type_owner(map, 15, 6) == 30);
    assert(count_type_owner(map, 37, 3) == 5);
    assert(count_type_owner(map, 89, 11) == 2 && count_type_owner(map, 95, 11) == 3);
    assert(count_type_owner(map, 195, 1) == 2 && count_type_owner(map, 195, 6) == 4);
    assert(count_type_owner(map, 195, 7) == 2);

    unsigned ghost_found = 0, player_five_civilian_found = 0;
    unsigned hp100 = 0, hp75 = 0, hp50 = 0;
    for (unsigned i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *u = &map->units[i];
        assert(u->x_px < map->width * 16 && u->y_px < map->height * 16);
        assert(u->flags == 0 && u->resources == 0);
        hp100 += u->hp_percent == 100;
        hp75 += u->hp_percent == 75;
        hp50 += u->hp_percent == 50;
        if (u->source_type == 1 && u->owner == 6)
            ghost_found++;
        if (u->source_type == 15 && u->owner == 4)
            player_five_civilian_found++;
    }
    assert(hp100 == 91 && hp75 == 3 && hp50 == 1);
    assert(ghost_found == 1 && player_five_civilian_found == 1);

    /* FNV-1a digests cover every imported source row field (in canonical
     * little-endian field order) and all converted terrain lookup/walk/pixel
     * bytes. This catches a misplaced or silently omitted source row/cell. */
    unsigned long long roster_hash = 14695981039346656037ull;
    for (unsigned i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *u = &map->units[i];
        roster_hash = hash_u16(roster_hash, u->x_px);
        roster_hash = hash_u16(roster_hash, u->y_px);
        roster_hash = hash_u16(roster_hash, u->source_type);
        roster_hash = hash_byte(roster_hash, u->owner);
        roster_hash = hash_byte(roster_hash, u->hp_percent);
        roster_hash = hash_u16(roster_hash, u->flags);
        roster_hash = hash_u16(roster_hash, u->resources);
    }
    assert(roster_hash == 0xa415b5973e020be8ull);

    assert(map->tile_count > 0 && map->tile_lookup && map->blocked);
    assert(map->tile_count == 736);
    unsigned blocked = 0;
    for (unsigned i = 0; i < (unsigned)map->width * map->height; ++i) {
        assert(map->tile_lookup[i] < map->tile_count);
        assert(map->blocked[i] <= 3);
        blocked += (map->blocked[i] & 1) != 0;
    }
    assert(blocked == 3384);
    unsigned long long terrain_hash = 14695981039346656037ull;
    for (unsigned i = 0; i < (unsigned)map->width * map->height; ++i)
        terrain_hash = hash_u16(terrain_hash, map->tile_lookup[i]);
    for (unsigned i = 0; i < (unsigned)map->width * map->height; ++i)
        terrain_hash = hash_byte(terrain_hash, map->blocked[i] & 1);
    for (unsigned i = 0; i < (unsigned)map->tile_count * 256; ++i)
        terrain_hash = hash_byte(terrain_hash, ((const unsigned char *)map->tile_pixels)[i]);
    assert(terrain_hash == 0x3451be6ebfefb88full);
    return 0;
}
