/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exercises the real custom-game runtime and ordinary save/load path. */
#define main sc64_resume_baseline_main
#include "sc64_skirmish_resume.c"
#undef main

#ifndef SC64_SHARED_TRANSPORT
#error "compile with the staged shared transport capability"
#endif

static void check_jobs(int map)
{
    static const int owners[4] = { SIDE, ENEMY, RESCUABLE, 4 };
    int lair[4];
    launch(map, R_ZERG, 3, R_ZERG);
    for (int i = 0; i < 4; ++i) {
        lair[i] = spawn_build(owners[i], B_LAIR,
                              race_base_x[i] + 60, race_base_y[i] + 60, 0);
        assert(lair[i] >= 0);
        if (i == 0) { minerals = 1000; gas = 1000; }
        else { race_minerals[owners[i]] = 1000; race_gas[owners[i]] = 1000; }
        assert(race_sk_transport_start_owner(owners[i], lair[i]));
    }
    for (int i = 0; i < 60; ++i) race_sk_transport_research_tick();
    assert(race_save_game());
    memset(&race_skirmish_transport_tech, 0, sizeof(race_skirmish_transport_tech));
    assert(race_load_game());
    for (int i = 0; i < 4; ++i) {
        assert(race_skirmish_transport_tech.owner[i].remaining == 2940);
        assert(race_skirmish_transport_tech.owner[i].producer_plus_one == lair[i] + 1);
    }
    for (int i = 0; i < 2940; ++i) race_sk_transport_research_tick();
    for (int i = 0; i < 4; ++i) assert(race_skirmish_transport_tech.owner[i].researched);
    assert(race_save_game());
    assert(race_load_game());
    for (int i = 0; i < 4; ++i) assert(race_skirmish_transport_tech.owner[i].researched);
}

static void check_cargo(int map, int human)
{
    static const int raw_carriers[3] = {11, 42, 69};
    static const int passengers[3] = {U_MARINE, U_ZERGLING, U_ZEALOT};
    int type, ship, passenger;
    launch(map, human, 3, -1);
    if (human == R_ZERG) race_skirmish_transport_tech.owner[0].researched = 1;
    type = sc64_race_unit_from_raw(raw_carriers[human]);
    assert(type >= 0);
    ship = spawn_unit(SIDE, type, race_base_x[0], race_base_y[0]);
    passenger = spawn_unit(SIDE, passengers[human], race_base_x[0], race_base_y[0]);
    assert(ship >= 0 && passenger >= 0 && race_transport_carrier_valid(ship));
    assert(race_transport_request_board(passenger, ship));
    assert(race_transport_tick_unit(passenger));
    assert(u[passenger].order == ORDER_TRANSPORTED);
    assert(race_save_game());
    assert(race_load_game());
    assert(u[passenger].order == ORDER_TRANSPORTED && u[passenger].target == ship);
    assert(race_transport_cargo_space(ship) > 0);
    assert(race_transport_request_unload(ship, u[ship].x, u[ship].y));
    for (int y = 1; y < race_rows - 1 && u[passenger].order == ORDER_TRANSPORTED; ++y)
        for (int x = 1; x < race_cols - 1 && u[passenger].order == ORDER_TRANSPORTED; ++x) {
            if (!is_traversable(x, y, -1)) continue;
            u[ship].x = u[ship].tx = x * 16 + 8;
            u[ship].y = u[ship].ty = y * 16 + 8;
            race_transport_try_unload(ship);
        }
    assert(u[passenger].order == ORDER_IDLE);
}

int main(void)
{
    gba_init();
    for (int map = 0; map <= SC64_SKIRMISH_IMPORTED_COUNT; ++map) {
        check_jobs(map);
        for (int race = 0; race < 3; ++race) check_cargo(map, race);
    }
    puts("shared custom transport research/save: ok");
    return 0;
}
