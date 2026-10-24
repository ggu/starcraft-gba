/* Opt-in Zerg/Protoss storage foundation checks. This fixture exercises the
 * enlarged geometry and compact record codecs; it does not enable campaign
 * missions or assert full-campaign pool capacity. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_ZP_CAMPAIGN_STORAGE
#error "Compile with the opt-in Zerg/Protoss storage gate"
#endif

_Static_assert(SC64_RACE_SAVE_GENERATION == 20u, "distinct Z/P storage generation");
#if SC64_CAMPAIGN_RACE == 2
_Static_assert(MAX_UNITS == 240, "first-stage Protoss unit bound");
#else
_Static_assert(MAX_UNITS == 192, "first-stage Zerg unit bound");
#endif
_Static_assert(MAX_BUILDINGS == 96, "first-stage Z/P building bound");
_Static_assert(MAX_MINERALS == 48, "first-stage Z/P mineral bound");
_Static_assert(sizeof(Unit) == 56, "compact live Unit layout");
_Static_assert(sizeof(Sc64PackedUnitSave) == 38, "compact Unit save row");
_Static_assert(sizeof(Sc64PackedBuildingSave) == 28, "compact Building save row");
_Static_assert(sizeof(RaceSave) <= 16368, "save record remains bounded");
_Static_assert(SC64_ROUTE_SLOTS == 14, "large-map route cache has a fixed bound");
_Static_assert(SC64_ROUTE_BYTES == 6144, "128x128 packed route field size");

static void test_geometry_and_grid(void)
{
    race_cols = race_rows = 128;
    assert(MAP_CELLS == 128 * 128);
    assert(sc64_race_storage_geometry_valid(128, 128));
    assert(!sc64_race_storage_geometry_valid(129, 128));
    assert(!sc64_race_storage_geometry_valid(128, 129));
    assert(sc64_cell_col(MAP_CELLS - 1) == 127);
    assert(sc64_cell_row(MAP_CELLS - 1) == 127);
    SC64_GRID_CLEAR(explored);
    SC64_GRID_SET(explored, MAP_CELLS - 1, 1);
    assert(SC64_GRID_GET(explored, MAP_CELLS - 1));
    assert(sc64_minimap_cell(MAP_CELLS - 1) == 23 * 24 + 23);
}

static void test_nine_bit_visibility_sources(void)
{
    const int cell = 127 + 127 * 128;
    sc64_visibility_reset();
    for (int i = 0; i < MAX_UNITS + MAX_BUILDINGS; ++i)
        sc64_visibility_source_change(cell, 1);
    assert(sc64_visibility_source_count(cell) == MAX_UNITS + MAX_BUILDINGS);
    assert(sc64_visibility_source_count(cell) ==
           (SC64_CAMPAIGN_RACE == 2 ? 336 : 288));
    assert(SC64_GRID_GET(visible, cell));
    assert(sc64_visible_sources_high[(unsigned)cell >> 3] &
           (u8)(1u << ((unsigned)cell & 7u)));
    for (int i = 0; i < MAX_UNITS + MAX_BUILDINGS; ++i)
        sc64_visibility_source_change(cell, 0);
    assert(sc64_visibility_source_count(cell) == 0);
    assert(!SC64_GRID_GET(visible, cell));
}

static void test_compact_unit_roundtrip_and_rejection(void)
{
    Unit source = {0}, decoded;
    Sc64PackedUnitSave row, damaged;
    source.hp = source.maxhp = 131071;
    source.carry = -65536;
    source.shield = 4095;
    source.x = source.y = source.tx = source.ty = 2047;
    source.path_x = source.path_y = 2047;
    source.build = -512;
    source.target = -1;
    source.owner = 4;
    source.type = U_ZERGLING;
    source.alive = 1;
    source.selected = 1;
    source.order = ORDER_ATTACK_MOVE;
    source.cd = source.work_timer = source.path_timer = source.path_retry = 65535;
    source.weapon = source.flags = 65535;
    source.armor = 255;
    source.raw_type = source.raw_owner = 255;
    source.shield_remainder = 1279;
    race_cols = race_rows = 128;
    assert(sc64_unit_save_encode(&row, &source, 2048, 2048));
    assert(sc64_unit_save_decode(&decoded, &row, 2048, 2048));
    assert(memcmp(&decoded, &source, sizeof(source)) == 0);
    damaged = row;
    damaged.bytes[SC64_PACKED_UNIT_SAVE_BYTES - 1] |= 0x80;
    memset(&decoded, 0xA5, sizeof(decoded));
    Unit before = decoded;
    assert(!sc64_unit_save_decode(&decoded, &damaged, 2048, 2048));
    assert(memcmp(&decoded, &before, sizeof(decoded)) == 0);
    assert(!sc64_unit_save_encode(&row, &source, 2047, 2048));
}

static void test_compact_building_roundtrip_and_rejection(void)
{
    Building source = {0}, decoded;
    Sc64PackedBuildingSave row, damaged;
    source.x = source.y = 2047;
    source.hp = source.maxhp = 131071;
    source.owner = 4;
    source.type = B_HATCHERY;
    source.alive = 1;
    source.queue[0] = U_ZERGLING;
    source.queue_head = 4;
    source.queue_count = 1;
    source.queue_progress = 65535;
    source.build_progress = 65534;
    source.under_construction = 1;
    source.level = 127;
    source.shield = source.flags = 65535;
    source.raw_type = source.raw_owner = 255;
    source.larva = 3;
    source.larva_timer = 511;
    source.shield_remainder = 1279;
    assert(sc64_building_save_encode(&row, &source, 2048, 2048));
    assert(sc64_building_save_decode(&decoded, &row));
    assert(memcmp(&decoded, &source, sizeof(source)) == 0);
    damaged = row;
    damaged.bytes[SC64_PACKED_BUILDING_SAVE_BYTES - 1] |= 0x80;
    memset(&decoded, 0xA5, sizeof(decoded));
    Building before = decoded;
    assert(!sc64_building_save_decode(&decoded, &damaged));
    assert(memcmp(&decoded, &before, sizeof(decoded)) == 0);
}

static void test_save_union_and_generation(void)
{
    static RaceSave save, loaded;
    Unit source, decoded;
    Building building, decoded_building;
    Sc64PackedUnitSave bad_unit;
    memset(&save, 0, sizeof(save));
    memset(&loaded, 0, sizeof(loaded));
    save.version = SAVE_VERSION;
    assert(save.version == 0x53435a30u + 20u || save.version == 0x53435030u + 20u);
    memset(&source, 0, sizeof(source));
    source.x = source.y = 2047;
    source.hp = 8; source.maxhp = 10; source.owner = 2; source.type = U_DRONE;
    source.alive = 1; source.order = ORDER_IDLE; source.target = -1;
    assert(sc64_unit_save_encode(&save.units[MAX_UNITS - 1], &source, 2048, 2048));
    memset(&building, 0, sizeof(building));
    building.x = building.y = 2047;
    building.hp = 10; building.maxhp = 10; building.owner = 2;
    building.type = B_HATCHERY; building.alive = 1;
    assert(sc64_building_save_encode(&save.buildings[MAX_BUILDINGS - 1],
                                     &building, 2048, 2048));
    save.fog[(MAP_CELLS - 1) >> 3] |= (u8)(1u << ((MAP_CELLS - 1) & 7));
    save_write(&save, sizeof(save), SAVE_VERSION);
    assert(save_read(&loaded, sizeof(loaded), SAVE_VERSION));
    assert(loaded.version == SAVE_VERSION);
    assert(sc64_unit_save_decode(&decoded, &loaded.units[MAX_UNITS - 1], 2048, 2048));
    assert(memcmp(&decoded, &source, sizeof(source)) == 0);
    assert(sc64_building_save_decode(&decoded_building,
                                     &loaded.buildings[MAX_BUILDINGS - 1]));
    assert(memcmp(&decoded_building, &building, sizeof(building)) == 0);
    assert((loaded.fog[(MAP_CELLS - 1) >> 3] &
            (u8)(1u << ((MAP_CELLS - 1) & 7))) != 0);
    bad_unit = loaded.units[MAX_UNITS - 1];
    bad_unit.bytes[SC64_PACKED_UNIT_SAVE_BYTES - 1] |= 0x80;
    assert(!sc64_unit_save_decode(&decoded, &bad_unit, 2048, 2048));
    assert(sizeof(sc64_zp_route_save_scratch) >= sizeof(RaceSave));
    assert(sizeof(sc64_zp_route_save_scratch) >= MAP_CELLS * sizeof(u16));
    /* These are staged bounds, not full-campaign capacity. P4 needs room for
     * 87 scripted creations in addition to its initial roster; production
     * must preserve those slots rather than consuming them in advance. */
    assert(MAX_UNITS < 256 && MAX_BUILDINGS < 150 && MAX_MINERALS < 142);
}

int main(void)
{
    test_geometry_and_grid();
    test_nine_bit_visibility_sources();
    test_compact_unit_roundtrip_and_rejection();
    test_compact_building_roundtrip_and_rejection();
    test_save_union_and_generation();
    printf("sc64 Z/P storage checks passed: units=%d buildings=%d minerals=%d RaceSave=%u\n",
           MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, (unsigned)sizeof(RaceSave));
    return 0;
}
