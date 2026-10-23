/* Internal generation-17 storage fixture. Synthetic overflow actors verify
 * slot capacity and lossless saves; this is not a campaign-playability test. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_LATE_TERRAN_STORAGE
#error "Build with the late-Terran storage profile"
#endif

_Static_assert(MAX_UNITS == 192, "late profile unit pool");
_Static_assert(MAX_BUILDINGS == 96, "late profile building pool");
_Static_assert(MAX_MINERALS == 80, "late profile mineral pool");
_Static_assert(sizeof(Unit) == 56, "compact late-profile runtime Unit");
_Static_assert(sizeof(Sc64PackedUnitSave) == 38, "packed unit row size");
_Static_assert(sizeof(RaceSave) == 15312, "generation-17 payload size");
_Static_assert(sizeof(RaceSave) <= 16368, "late save fits one SRAM record");

static Unit saved_units[MAX_UNITS];
static Building saved_buildings[MAX_BUILDINGS];
static Mineral saved_minerals[MAX_MINERALS];
static RaceSave saved_record;
static int repair_worker = -1;
static int repair_building = -1;

static unsigned count_live_units(void)
{
    unsigned count = 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) count += u[i].alive != 0;
    return count;
}

static unsigned count_live_buildings(void)
{
    unsigned count = 0;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) count += b[i].alive != 0;
    return count;
}

static void fill_all_entity_slots(void)
{
    unsigned serial = 0;
    while (count_live_units() < MAX_UNITS) {
        int x = 1200 + (int)(serial % 24u) * 8;
        int y = 1240 + (int)(serial / 24u) * 8;
        assert(x < race_cols * 16 && y < race_rows * 16);
        assert(spawn_unit(SIDE, U_MARINE, x, y) >= 0);
        ++serial;
    }
    serial = 0;
    while (count_live_buildings() < MAX_BUILDINGS) {
        int x = 1200 + (int)(serial % 12u) * 16;
        int y = 1040 + (int)(serial / 12u) * 16;
        assert(x < race_cols * 16 && y < race_rows * 16);
        assert(spawn_build(SIDE, B_DEPOT, x, y, 0) >= 0);
        ++serial;
    }
    for (unsigned i = 0; i < MAX_MINERALS; ++i) {
        if (i < 35) continue; /* Keep all imported resource rows intact. */
        m[i].x = 1000 + (int)(i % 20u) * 12;
        m[i].y = 1000 + (int)(i / 20u) * 12;
        m[i].amount = 1500 + (int)i;
        m[i].style = (int)(i % 8u);
    }
}

static void make_pending_repair(void)
{
    repair_worker = -1;
    repair_building = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV) {
            repair_worker = i;
            break;
        }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_DEPOT &&
            !b[i].under_construction) {
            repair_building = i;
            break;
        }
    assert(repair_worker >= 0 && repair_building >= 0);
    assert(b[repair_building].hp == b[repair_building].maxhp);
    --b[repair_building].hp;
    assert(race_t6_repair_building_valid(repair_building));
    u[repair_worker].order = ORDER_REPAIR;
    u[repair_worker].target = MAX_UNITS + repair_building;
    u[repair_worker].tx = b[repair_building].x;
    u[repair_worker].ty = b[repair_building].y;
    u[repair_worker].carry = 123;
    u[repair_worker].work_timer = 12;
}

static void remember_world(void)
{
    memcpy(saved_units, u, sizeof(saved_units));
    memcpy(saved_buildings, b, sizeof(saved_buildings));
    memcpy(saved_minerals, m, sizeof(saved_minerals));
}

static void assert_world_restored(void)
{
    assert(!memcmp(saved_units, u, sizeof(saved_units)));
    assert(!memcmp(saved_buildings, b, sizeof(saved_buildings)));
    assert(!memcmp(saved_minerals, m, sizeof(saved_minerals)));
}

static void start_source_mission(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T6_GLOBAL_ID;
    init_state();
    assert(race_t6_active() && screen == SCREEN_PLAY);
    assert(race_cols == 96 && race_rows == 96);
    assert(count_live_units() == 104 && count_live_buildings() == 74);
    unsigned minerals = 0;
    for (unsigned i = 0; i < MAX_MINERALS; ++i) minerals += m[i].amount > 0;
    assert(minerals == 35);
}

static void test_complete_capacity_roundtrip(void)
{
    start_source_mission();
    fill_all_entity_slots();
    assert(count_live_units() == 192 && count_live_buildings() == 96);
    make_pending_repair();
    remember_world();
    assert(race_save_game());
    saved_record = race_data;

    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit decoded;
        assert(sc64_unit_save_decode(&decoded, &saved_record.units[i],
                                     96u * 16u, 96u * 16u));
        assert(!memcmp(&decoded, &saved_units[i], sizeof(decoded)));
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        Building decoded;
        assert(sc64_building_save_decode(&decoded, &saved_record.buildings[i]));
        assert(!memcmp(&decoded, &saved_buildings[i], sizeof(decoded)));
    }

    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    assert(race_load_game());
    assert_world_restored();
    assert(u[repair_worker].order == ORDER_REPAIR &&
           u[repair_worker].target == MAX_UNITS + repair_building &&
           u[repair_worker].carry == 123 && u[repair_worker].work_timer == 12);
}

static void reject_bad_repair_record(RaceSave *bad)
{
    remember_world();
    save_write(bad, sizeof(*bad), SAVE_VERSION);
    save_write(bad, sizeof(*bad), SAVE_VERSION);
    assert(!race_load_game());
    assert_world_restored();
    save_write(&saved_record, sizeof(saved_record), SAVE_VERSION);
}

static void replace_saved_worker(RaceSave *record, const Unit *worker)
{
    assert(sc64_unit_save_encode(&record->units[repair_worker], worker,
                                 96u * 16u, 96u * 16u));
}

static void test_invalid_repair_rows_rejected_atomically(void)
{
    RaceSave bad;
    Unit worker;
    Building building;
    start_source_mission();
    fill_all_entity_slots();
    make_pending_repair();
    assert(race_save_game());
    saved_record = race_data;

    bad = saved_record;
    worker = u[repair_worker]; worker.owner = ENEMY;
    replace_saved_worker(&bad, &worker);
    reject_bad_repair_record(&bad);

    bad = saved_record;
    worker = u[repair_worker]; worker.type = U_MARINE;
    replace_saved_worker(&bad, &worker);
    reject_bad_repair_record(&bad);

    bad = saved_record;
    worker = u[repair_worker]; worker.target = MAX_UNITS + MAX_BUILDINGS;
    replace_saved_worker(&bad, &worker);
    reject_bad_repair_record(&bad);

    bad = saved_record;
    worker = u[repair_worker]; worker.carry = 256;
    replace_saved_worker(&bad, &worker);
    reject_bad_repair_record(&bad);

    bad = saved_record;
    assert(sc64_building_save_decode(&building, &bad.buildings[repair_building]));
    building.owner = ENEMY;
    assert(sc64_building_save_encode(&bad.buildings[repair_building], &building,
                                     96u * 16u, 96u * 16u));
    reject_bad_repair_record(&bad);

    bad = saved_record;
    assert(sc64_building_save_decode(&building, &bad.buildings[repair_building]));
    building.hp = building.maxhp;
    assert(sc64_building_save_encode(&bad.buildings[repair_building], &building,
                                     96u * 16u, 96u * 16u));
    reject_bad_repair_record(&bad);
}

static void test_corrupt_unit_rejected_before_world_mutation(void)
{
    RaceSave bad;
    start_source_mission();
    fill_all_entity_slots();
    assert(race_save_game());
    saved_record = race_data;
    remember_world();

    bad = saved_record;
    bad.units[MAX_UNITS - 1].bytes[37] |= 0x80u; /* Reserved codec bit. */
    save_write(&bad, sizeof(bad), SAVE_VERSION);
    save_write(&bad, sizeof(bad), SAVE_VERSION);
    assert(!race_load_game());
    assert_world_restored();
    save_write(&saved_record, sizeof(saved_record), SAVE_VERSION);
}

int main(void)
{
    test_complete_capacity_roundtrip();
    test_invalid_repair_rows_rejected_atomically();
    test_corrupt_unit_rejected_before_world_mutation();
    printf("late Terran storage: %zu-byte save, %d units, %d buildings, %d resources\n",
           sizeof(RaceSave), MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS);
    return 0;
}
