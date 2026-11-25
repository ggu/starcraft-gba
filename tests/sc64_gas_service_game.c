/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static void exercise(int race)
{
    game_init();
    race_mode = 1;
    race_human = race;
    race_bots = 1;
    race_skirmish_map = 1;
    race_bot_races[0] = R_TER;
    init_state();
    int kind = race == R_TER ? B_REFINERY : race == R_ZERG ? B_EXTRACTOR : B_ASSIMILATOR;
    int worker_type = race == R_TER ? U_SCV : race == R_ZERG ? U_DRONE : U_PROBE;
    int sites[2], workers[5];
    for (int s = 0; s < 2; ++s) {
        sites[s] = spawn_build(SIDE, kind, sc64_gas_sites[s].x, sc64_gas_sites[s].y, 0);
        assert(sites[s] >= 0);
    }
    for (int i = 0; i < 5; ++i) {
        int site = sites[i == 4];
        int slot = spawn_unit(SIDE, worker_type, b[site].x, b[site].y);
        assert(slot >= 0);
        workers[i] = slot;
        u[slot].target = site;
        u[slot].order = ORDER_GAS;
    }
    /* Enroll workers in reverse slot order. Slot-order updates thereafter
     * must preserve the observed arrivals when deciding who mines next. */
    for (int t = 0; t < 60; ++t) {
        sc64_gas_service_begin();
        for (int i = 0; i < 5; ++i)
            race_update_resource_worker(&u[workers[t == 0 ? 4 - i : i]]);
        if (t < 59) {
            assert(u[workers[3]].work_timer == 59 - t && u[workers[4]].work_timer == 59 - t);
            for (int i = 0; i < 3; ++i) assert(!u[workers[i]].work_timer && !u[workers[i]].carry);
        }
    }
    assert(u[workers[3]].carry == 8 && u[workers[4]].carry == 8);
    assert(sc64_geyser_consumed[0] == 8 && sc64_geyser_consumed[1] == 8);
    assert(race_save_game());
    u[workers[2]].order = ORDER_MOVE;
    assert(race_save_game()); /* Pause directly after cancelling a queued order. */
    assert(gas_service.head[sites[0]] == workers[2] + 1);
    assert(race_load_game());
    assert(u[workers[2]].order == ORDER_MOVE && !gas_service.site[workers[2]]);
    assert(gas_service.head[sites[0]] == workers[1] + 1);
    sc64_gas_service_begin();
    race_update_resource_worker(&u[workers[0]]);
    race_update_resource_worker(&u[workers[1]]);
    assert(u[workers[1]].work_timer == 59 && u[workers[0]].work_timer == 0);
    u[workers[1]].alive = 0;
    sc64_gas_service_begin();
    race_update_resource_worker(&u[workers[0]]);
    assert(u[workers[0]].work_timer == 59);
    u[workers[2]].order = ORDER_GAS;
    u[workers[1]].alive = 1;
    u[workers[0]].work_timer = 24;
    u[workers[1]].work_timer = 6;
    u[workers[2]].work_timer = 6;
    sc64_gas_service_migrate_legacy();
    assert(gas_service.head[sites[0]] == workers[1] + 1);
    assert(gas_service.next[workers[1]] == workers[2] + 1);
    assert(gas_service.next[workers[2]] == workers[0] + 1);
    for (int t = 0; t < 6; ++t) {
        sc64_gas_service_begin();
        for (int i = 0; i < 3; ++i) race_update_resource_worker(&u[workers[i]]);
    }
    assert(u[workers[1]].carry == 8 && u[workers[2]].work_timer == 6);
    assert(u[workers[0]].work_timer == 24);
    b[sites[0]].alive = 0;
    sc64_gas_service_begin();
    assert(!gas_service.head[sites[0]] && u[workers[0]].work_timer == 0);
}

int main(void)
{
    for (int race = 0; race < 3; ++race) exercise(race);
    puts("All three races: independent service, ordered waits, cancellation, death and site loss passed");
    return 0;
}
