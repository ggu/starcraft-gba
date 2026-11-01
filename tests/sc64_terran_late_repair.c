/* T8 late-repair integration fixture. Direct HP/position/order writes are
 * labeled test setup, not controller gameplay evidence. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#include "sc64_save_candidate_fixture.h"

#ifndef SC64_TERRAN_LATE_RUNTIME
#error "Compile with the late Terran runtime profile"
#endif

static void start_t8(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T8;
    init_state();
    assert(race_late_active());
    assert(screen == SCREEN_PLAY);
}

static void assert_bad_load_rejected(const RaceSave *bad, const RaceSave *good)
{
    write_candidate_record(bad);
    assert(!race_load_game());
    write_candidate_record(good);
    assert(race_load_game());
}

static void test_source_mechanical_predicates(void)
{
    assert(race_late_repair_unit_raw_mechanical(2));
    assert(race_late_repair_unit_raw_mechanical(3));
    assert(race_late_repair_unit_raw_mechanical(5));
    assert(race_late_repair_unit_raw_mechanical(7));
    assert(race_late_repair_unit_raw_mechanical(8));
    assert(race_late_repair_unit_raw_mechanical(9));
    assert(race_late_repair_unit_raw_mechanical(11));
    assert(race_late_repair_unit_raw_mechanical(12));
    assert(race_late_repair_unit_raw_mechanical(19));
    assert(race_late_repair_unit_raw_mechanical(29));
    assert(!race_late_repair_unit_raw_mechanical(0));
    assert(!race_late_repair_unit_raw_mechanical(16));
    assert(race_late_repair_building_raw_mechanical(116)); /* Science Facility */
    assert(race_late_repair_building_raw_mechanical(126)); /* Crashed Norad */
    assert(!race_late_repair_building_raw_mechanical(195)); /* Terran Beacon */
    assert(!race_late_repair_building_raw_mechanical(138)); /* Zerg building */
}

static void test_selected_scv_repairs_mobile_target(void)
{
    start_t8();
    int worker = spawn_unit(SIDE, U_SCV, 600, 600);
    int target = spawn_unit(SIDE, U_VULTURE, 606, 600);
    assert(worker >= 0 && target >= 0);
    u[target].hp -= 20;
    u[worker].selected = 1;
    cx = u[target].x; cy = u[target].y;
    minerals = gas = 10000;
    assert(race_late_repair_target_unit_valid(target));
    assert(race_late_request_repair() == 1);
    assert(u[worker].order == ORDER_REPAIR && u[worker].target == target);

    int before = u[target].hp;
    for (int t = 1; t <= 300 && u[target].hp == before; ++t) {
        frame = t;
        assert(race_late_repair_tick_unit(worker));
    }
    assert(u[target].hp > before);
    assert(u[worker].order == ORDER_REPAIR);

    /* Mechanical, friendly, unloaded and damaged checks match source gates. */
    assert(!race_late_repair_unit_fields_valid(0, SIDE, 1, 20, 40, 0));
    assert(!race_late_repair_unit_fields_valid(2, ENEMY, 1, 20, 40, 0));
    assert(!race_late_repair_unit_fields_valid(2, SIDE, 1, 40, 40, 0));
    assert(!race_late_repair_unit_fields_valid(2, SIDE, 1, 20, 40, 1));
    assert(!race_late_repair_unit_fields_valid(2, SIDE, 0, 20, 40, 0));
}

static void test_late_save_accepts_and_rejects_repair_targets(void)
{
    start_t8();
    int worker = spawn_unit(SIDE, U_SCV, 700, 700);
    int target = spawn_unit(SIDE, U_WRAITH, 706, 700);
    assert(worker >= 0 && target >= 0);
    u[target].hp -= 10;
    u[worker].order = ORDER_REPAIR;
    u[worker].target = target;
    u[worker].carry = 37;
    u[worker].work_timer = 4;
    assert(race_save_game());
    RaceSave valid;
    fixture_capture_save(&valid);
    assert(race_terran_late_save_candidate_valid(&valid, race_cols, race_rows));

    /* A dead/foreign/organic target cannot be smuggled into a repair order. */
    for (int mutation = 0; mutation < 3; ++mutation) {
        RaceSave bad = valid;
        Unit candidate;
        assert(race_save_decode_unit(&bad, (unsigned)target,
                                     (unsigned)race_cols * 16u,
                                     (unsigned)race_rows * 16u, &candidate));
        if (mutation == 0) candidate.owner = ENEMY;
        if (mutation == 1) candidate.raw_type = 0;
        if (mutation == 2) candidate.alive = 0;
        assert(sc64_unit_save_encode(&bad.units[target], &candidate,
                                     (unsigned)race_cols * 16u,
                                     (unsigned)race_rows * 16u));
        assert_bad_load_rejected(&bad, &valid);
    }

    /* Building targets use the same combined namespace and must be completed,
     * friendly and mechanical; Science Facility is an imported scenario type. */
    start_t8();
    worker = spawn_unit(SIDE, U_SCV, 800, 800);
    int facility = spawn_build(SIDE, B_SCIENCE_FACILITY, 806, 800, 0);
    assert(worker >= 0 && facility >= 0);
    b[facility].hp -= 25;
    u[worker].order = ORDER_REPAIR;
    u[worker].target = MAX_UNITS + facility;
    assert(race_late_repair_target_building_valid(facility));
    assert(race_save_game());
    fixture_capture_save(&valid);
    assert(race_terran_late_save_candidate_valid(&valid, race_cols, race_rows));
    RaceSave bad = valid;
    Building decoded;
    assert(race_save_decode_building(&bad, (unsigned)facility, &decoded));
    decoded.raw_type = 195;
    assert(sc64_building_save_encode(&bad.buildings[facility], &decoded,
                                     (unsigned)race_cols * 16u,
                                     (unsigned)race_rows * 16u));
    assert_bad_load_rejected(&bad, &valid);
}

int main(void)
{
    test_source_mechanical_predicates();
    test_selected_scv_repairs_mobile_target();
    test_late_save_accepts_and_rejects_repair_targets();
    puts("late Terran repair integration: ok");
    return 0;
}
