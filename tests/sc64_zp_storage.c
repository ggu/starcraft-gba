/* Opt-in Zerg/Protoss storage checks. This fixture exercises enlarged geometry
 * and selected compact codecs; it does not enable campaign missions. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

/* Exercise the persistent implementation beside the profile-selected legacy
 * one. Keep its private index symbols renamed so this stays one oracle binary. */
#define sc64_unit_bucket_head zp_persist_bucket_head
#define sc64_unit_bucket_next zp_persist_bucket_next
#define sc64_unit_bucket_prev zp_persist_bucket_prev
#define sc64_unit_bucket_cell zp_persist_bucket_cell
#define sc64_unit_bucket_initialized zp_persist_bucket_initialized
#define sc64_unit_bucket_active zp_persist_bucket_active
#define sc64_unit_bucket_cell_at zp_persist_bucket_cell_at
#define sc64_unit_bucket_insert zp_persist_bucket_insert
#define sc64_unit_bucket_remove zp_persist_bucket_remove
#define sc64_unit_buckets_begin zp_persist_buckets_begin
#define sc64_unit_bucket_move zp_persist_bucket_move
#define sc64_unit_bucket_step_clear zp_persist_bucket_step_clear
#define sc64_unit_buckets_end zp_persist_buckets_end
#include "../starcraft/sc64_persistent_collision.h"
#undef sc64_unit_bucket_head
#undef sc64_unit_bucket_next
#undef sc64_unit_bucket_prev
#undef sc64_unit_bucket_cell
#undef sc64_unit_bucket_initialized
#undef sc64_unit_bucket_active
#undef sc64_unit_bucket_cell_at
#undef sc64_unit_bucket_insert
#undef sc64_unit_bucket_remove
#undef sc64_unit_buckets_begin
#undef sc64_unit_bucket_move
#undef sc64_unit_bucket_step_clear
#undef sc64_unit_buckets_end

#ifndef SC64_ZP_CAMPAIGN_STORAGE
#error "Compile with the opt-in Zerg/Protoss storage gate"
#endif

#ifdef SC64_P6_STORM_ENABLED
_Static_assert(SC64_RACE_SAVE_GENERATION == 30u, "distinct P6 Storm storage generation");
#elif defined(SC64_P5_SHUTTLE_ENABLED)
_Static_assert(SC64_RACE_SAVE_GENERATION == 26u, "distinct P5 Shuttle storage generation");
#elif defined(SC64_P4_STORM_ENABLED)
_Static_assert(SC64_RACE_SAVE_GENERATION == 21u, "distinct Storm storage generation");
#else
_Static_assert(SC64_RACE_SAVE_GENERATION == 20u, "distinct Z/P storage generation");
#endif
#if SC64_CAMPAIGN_RACE == 2
_Static_assert(MAX_UNITS == 240, "first-stage Protoss unit bound");
#else
_Static_assert(MAX_UNITS == 192, "first-stage Zerg unit bound");
#endif
#ifdef SC64_ZP_FULL_STORAGE
_Static_assert(MAX_BUILDINGS == 160, "P5 full-profile building bound");
_Static_assert(MAX_MINERALS == 144, "P5 full-profile mineral bound");
#else
_Static_assert(MAX_BUILDINGS == 96, "first-stage Z/P building bound");
_Static_assert(MAX_MINERALS == 48, "first-stage Z/P mineral bound");
#endif
_Static_assert(sizeof(Unit) == 56, "compact live Unit layout");
#ifdef SC64_ZP_FULL_STORAGE
typedef Sc64ZpPackedUnitSave StorageFixtureUnitSave;
#define storage_fixture_unit_encode sc64_zp_unit_save_encode
#define storage_fixture_unit_decode sc64_zp_unit_save_decode
_Static_assert(sizeof(StorageFixtureUnitSave) == 35, "ZP Unit save row");
#else
typedef Sc64PackedUnitSave StorageFixtureUnitSave;
#define storage_fixture_unit_encode sc64_unit_save_encode
#define storage_fixture_unit_decode sc64_unit_save_decode
_Static_assert(sizeof(StorageFixtureUnitSave) == 38, "legacy compact Unit save row");
#endif
_Static_assert(sizeof(Sc64PackedBuildingSave) == 28, "compact Building save row");
_Static_assert(sizeof(RaceSave) <= 16368, "save record remains bounded");
_Static_assert(SC64_ROUTE_SLOTS == 14, "large-map route cache has a fixed bound");
_Static_assert(SC64_ROUTE_BYTES == 6144, "128x128 packed route field size");

static int full_scan_step_clear(const Unit *q, int nx, int ny)
{
    if (IS_WORKER(q->type) && (q->order == ORDER_MINE || q->order == ORDER_GAS))
        return 1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (&u[i] == q || !u[i].alive || SC64_TRANSPORT_LOADED(&u[i])) continue;
        int radius = sc64_game_unit_collision_radius(q->type);
        int candidate_radius = sc64_game_unit_collision_radius(u[i].type);
        if (candidate_radius > radius) radius = candidate_radius;
        if (u[i].type == U_CHRYSALIS ||
            sc64_race_unit_flying(u[i].type) != sc64_race_unit_flying(q->type))
            continue;
        if ((sc64_game_unit_actor_flags(u[i].type) &&
             !(sc64_game_unit_actor_flags(u[i].type) & SC64_GAME_ACTOR_COLLIDABLE)) ||
            (sc64_game_unit_actor_flags(q->type) &&
             !(sc64_game_unit_actor_flags(q->type) & SC64_GAME_ACTOR_COLLIDABLE)))
            continue;
        int dx = u[i].x - nx, dy = u[i].y - ny;
        if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius) continue;
        int next_distance = dx * dx + dy * dy;
        if (next_distance < radius * radius) {
            int ox = u[i].x - q->x, oy = u[i].y - q->y;
            if (next_distance <= ox * ox + oy * oy) return 0;
        }
    }
    return 1;
}

static void collision_case(int qx, int qy, int nx, int ny,
                           int cx, int cy, int candidate_type,
                           int candidate_alive, int candidate_order,
                           int candidate_owner, int expected_clear)
{
    memset(u, 0, sizeof(u));
    u[0].alive = 1; u[0].type = U_ZP_ULTRALISK; u[0].owner = SIDE;
    u[0].x = qx; u[0].y = qy; u[0].order = ORDER_IDLE;
    u[1].alive = (u8)candidate_alive; u[1].type = candidate_type;
    u[1].owner = candidate_owner; u[1].x = cx; u[1].y = cy;
    u[1].order = candidate_order;

    /* Reset both implementations to force a clean 16px bucket rebuild. */
    sc64_unit_bucket_initialized = 0;
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    int oracle = full_scan_step_clear(&u[0], nx, ny);
    assert(oracle == expected_clear);
    assert(sc64_unit_bucket_step_clear(&u[0], nx, ny) == oracle);
    sc64_unit_buckets_end();

    zp_persist_bucket_initialized = 0;
    zp_persist_buckets_begin();
    assert(zp_persist_bucket_active);
    assert(zp_persist_bucket_step_clear(&u[0], nx, ny) == oracle);
    zp_persist_buckets_end();
}

static void test_large_actor_collision_bucket_boundaries(void)
{
    /* Raw 39 Ultralisk bounds (19+18 by 16+15 source pixels) yield a 10px
     * radius. Marine uses the six-pixel audited fallback; max radius is 10. */
    assert(sc64_game_unit_collision_radius(U_ZP_ULTRALISK) == 10);
    assert(sc64_game_unit_collision_radius(U_MARINE) == 6);
    /* x: old +/-5 broad phase ended at cell 5 and missed Marine at x=96. */
    collision_case(86, 80, 87, 80, 96, 80, U_MARINE, 1, ORDER_IDLE, SIDE, 0);
    /* y equivalent, and diagonal cell corner with a true overlap. */
    collision_case(80, 86, 80, 87, 80, 96, U_MARINE, 1, ORDER_IDLE, ENEMY, 0);
    collision_case(86, 86, 87, 87, 94, 94, U_MARINE, 1, ORDER_IDLE, ENEMY, 0);
    /* Corner bucket is visited, but a 9,9 offset lies outside radius 10. */
    collision_case(86, 86, 87, 87, 96, 96, U_MARINE, 1, ORDER_IDLE, ENEMY, 1);
    /* Exact radius is clear (strict overlap), as is a more distant actor. */
    collision_case(86, 80, 87, 80, 97, 80, U_MARINE, 1, ORDER_IDLE, ENEMY, 1);
    collision_case(86, 80, 87, 80, 98, 80, U_MARINE, 1, ORDER_IDLE, ENEMY, 1);
    /* Lifecycle and flight-class controls: dead and flying actors
     * cannot block a ground mover; owner does not affect collision. */
    collision_case(86, 80, 87, 80, 96, 80, U_MARINE, 1,
                   ORDER_IDLE, SIDE, 0);
    collision_case(86, 80, 87, 80, 96, 80, U_MARINE, 0,
                   ORDER_IDLE, ENEMY, 1);
    collision_case(86, 80, 87, 80, 96, 80, U_MUTALISK, 1,
                   ORDER_IDLE, ENEMY, 1);
}

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
#ifdef SC64_ZP_FULL_STORAGE
    assert(sc64_visibility_source_count(cell) == 400);
#else
    assert(sc64_visibility_source_count(cell) ==
           (SC64_CAMPAIGN_RACE == 2 ? 336 : 288));
#endif
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
    StorageFixtureUnitSave row, damaged;
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
    source.cd = source.work_timer = 65535;
#ifdef SC64_ZP_FULL_STORAGE
    source.path_timer = 24;
    source.path_retry = 12;
#else
    source.path_timer = source.path_retry = 65535;
#endif
    source.weapon = source.flags = 65535;
    source.armor = 255;
    source.raw_type = source.raw_owner = 255;
    source.shield_remainder = 1279;
    race_cols = race_rows = 128;
    assert(storage_fixture_unit_encode(&row, &source, 2048, 2048));
    assert(storage_fixture_unit_decode(&decoded, &row, 2048, 2048));
    assert(memcmp(&decoded, &source, sizeof(source)) == 0);
    damaged = row;
#ifdef SC64_ZP_FULL_STORAGE
    /* The 35-byte ZP record uses all 280 bits and has no padding. Set the
     * packed path_timer field (bits 187..191) to the invalid value 25. */
    for (unsigned i = 0; i < 5; ++i) {
        unsigned bit = 187u + i;
        u8 mask = (u8)(1u << (bit & 7u));
        if ((25u >> i) & 1u) damaged.bytes[bit >> 3] |= mask;
        else damaged.bytes[bit >> 3] &= (u8)~mask;
    }
#else
    /* Legacy 38-byte format has an explicit reserved high padding bit. */
    damaged.bytes[SC64_PACKED_UNIT_SAVE_BYTES - 1] |= 0x80;
#endif
    memset(&decoded, 0xA5, sizeof(decoded));
    Unit before = decoded;
    assert(!storage_fixture_unit_decode(&decoded, &damaged, 2048, 2048));
    assert(memcmp(&decoded, &before, sizeof(decoded)) == 0);
    assert(!storage_fixture_unit_encode(&row, &source, 2047, 2048));
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
    StorageFixtureUnitSave bad_unit;
    memset(&save, 0, sizeof(save));
    memset(&loaded, 0, sizeof(loaded));
    save.version = SAVE_VERSION;
    assert(save.version == 0x53435a30u + SC64_RACE_SAVE_GENERATION ||
           save.version == 0x53435030u + SC64_RACE_SAVE_GENERATION);
    memset(&source, 0, sizeof(source));
    source.x = source.y = 2047;
    source.hp = 8; source.maxhp = 10; source.owner = 2; source.type = U_DRONE;
    source.alive = 1; source.order = ORDER_IDLE; source.target = -1;
    assert(storage_fixture_unit_encode(&save.units[MAX_UNITS - 1], &source, 2048, 2048));
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
    assert(storage_fixture_unit_decode(&decoded, &loaded.units[MAX_UNITS - 1], 2048, 2048));
    assert(memcmp(&decoded, &source, sizeof(source)) == 0);
    assert(sc64_building_save_decode(&decoded_building,
                                     &loaded.buildings[MAX_BUILDINGS - 1]));
    assert(memcmp(&decoded_building, &building, sizeof(building)) == 0);
    assert((loaded.fog[(MAP_CELLS - 1) >> 3] &
            (u8)(1u << ((MAP_CELLS - 1) & 7))) != 0);
    bad_unit = loaded.units[MAX_UNITS - 1];
#ifdef SC64_ZP_FULL_STORAGE
    /* Same invalid path_timer value as the standalone malformed-row case. */
    for (unsigned i = 0; i < 5; ++i) {
        unsigned bit = 187u + i;
        u8 mask = (u8)(1u << (bit & 7u));
        if ((25u >> i) & 1u) bad_unit.bytes[bit >> 3] |= mask;
        else bad_unit.bytes[bit >> 3] &= (u8)~mask;
    }
#else
    bad_unit.bytes[SC64_PACKED_UNIT_SAVE_BYTES - 1] |= 0x80;
#endif
    assert(!storage_fixture_unit_decode(&decoded, &bad_unit, 2048, 2048));
    assert(sizeof(sc64_zp_route_save_scratch) >= sizeof(RaceSave));
    assert(sizeof(sc64_zp_route_save_scratch) >= MAP_CELLS * sizeof(u16));
#ifdef SC64_ZP_FULL_STORAGE
    /* P5 opts into the full campaign pools, including its scripted arrivals. */
    assert(MAX_UNITS == 240 && MAX_BUILDINGS == 160 && MAX_MINERALS == 144);
#else
    /* These are staged bounds, not full-campaign capacity. P4 needs room for
     * 87 scripted creations in addition to its initial roster; production
     * must preserve those slots rather than consuming them in advance. */
    assert(MAX_UNITS < 256 && MAX_BUILDINGS < 150 && MAX_MINERALS < 142);
#endif
}

int main(void)
{
    test_large_actor_collision_bucket_boundaries();
    test_geometry_and_grid();
    test_nine_bit_visibility_sources();
    test_compact_unit_roundtrip_and_rejection();
    test_compact_building_roundtrip_and_rejection();
    test_save_union_and_generation();
    printf("sc64 Z/P storage checks passed: units=%d buildings=%d minerals=%d RaceSave=%u\n",
           MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, (unsigned)sizeof(RaceSave));
    return 0;
}
