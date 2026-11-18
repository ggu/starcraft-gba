/* SPDX-License-Identifier: GPL-3.0-or-later
 * P10-feature build of the shared L gesture and queued-supply witness. */
#include <assert.h>
#define SC64_QUEUE_GESTURE_ONLY
#define main sc64_selection_other_main
#include "sc64_selection_run.c"
#undef main

#ifdef SC64_PAID_WAITING_UNITS
static void waiting_producer(int slot, int owner, int type)
{
    b[slot].alive = 1;
    b[slot].owner = owner;
    b[slot].type = type;
    b[slot].x = 128 + slot * 32;
    b[slot].y = 128;
    b[slot].hp = b[slot].maxhp = 100;
    b[slot].raw_owner = -1;
    b[slot].raw_type = sc64_race_raw_building(type);
}

static void waiting_fixture(int producers, int scvs)
{
    fixture();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    race_mode = 0;
    race_selected_mission = -1;
    race_player[SIDE] = race_player[ENEMY] = R_TER;
    race_minerals[ENEMY] = 500;
    race_gas[ENEMY] = 0;
    for (int i = 0; i < producers; ++i) waiting_producer(i, SIDE, B_COMMAND);
    for (int i = 0; i < scvs; ++i)
        put_unit(i, SIDE, U_SCV, 96 + i * 8, 160, 0, 1);
    supply_used = scvs * unit_supply(U_SCV);
    supply_cap = race_owner_supply_capacity(SIDE);
}

static int terran_reserved(int owner)
{
    return race_supply_ledger_reserved(race_owner_supply_ledger(owner, R_TER));
}

static void paid_waiting_units(void)
{
    waiting_fixture(1, 9);
    assert(race_queue_unit(SIDE, 0, U_SCV));
    assert(b[0].queue_progress == 0 && !race_queue_head_waiting(&b[0]));
    assert(terran_reserved(SIDE) == 20 && race_owner_queued_supply(SIDE) == 2);
    assert(race_queue_unit(SIDE, 0, U_SCV));
    assert(b[0].queue_count == 2 && minerals == 400 && terran_reserved(SIDE) == 20);
    cancel_queue(0);
    assert(b[0].queue_count == 1 && minerals == 450 && terran_reserved(SIDE) == 20);
    assert(race_queue_unit(SIDE, 0, U_SCV));
    b[0].queue_progress = unit_build_time(U_SCV) - 1;
    race_tick_buildings();
    assert(b[0].queue_count == 1 && race_queue_head_waiting(&b[0]));
    assert(race_owner_supply(SIDE) == 20 && !race_owner_queued_supply(SIDE));
    assert(!race_queue_progress_for_hud(&b[0]) && minerals == 400);
    race_tick_buildings();
    assert(race_queue_head_waiting(&b[0]) && minerals == 400);
    u[0].alive = 0;
    race_tick_buildings();
    assert(!b[0].queue_progress && !race_queue_head_waiting(&b[0]));
    assert(terran_reserved(SIDE) == 20 && minerals == 400);
    race_tick_buildings();
    assert(b[0].queue_progress == 1);

    waiting_fixture(2, 18);
    for (int i = 0; i < 2; ++i) assert(race_queue_unit(SIDE, i, U_SCV));
    for (int i = 0; i < 2; ++i) {
        assert(race_queue_unit(SIDE, i, U_SCV));
        b[i].queue_progress = unit_build_time(U_SCV) - 1;
    }
    race_tick_buildings();
    assert(race_queue_head_waiting(&b[0]) && race_queue_head_waiting(&b[1]));
    u[0].alive = 0;
    race_tick_buildings();
    assert(!race_queue_head_waiting(&b[0]) && race_queue_head_waiting(&b[1]));
    assert(terran_reserved(SIDE) == 40);
    cancel_queue(1);
    assert(!b[1].queue_count && !b[1].queue_progress && terran_reserved(SIDE) == 40);

    waiting_producer(2, ENEMY, B_COMMAND);
    for (int i = 24; i < 32; ++i) put_unit(i, ENEMY, U_SCV, 80, 80, 0, 1);
    assert(race_queue_unit(ENEMY, 2, U_SCV) && race_queue_unit(ENEMY, 2, U_SCV));
    assert(terran_reserved(ENEMY) == 20 && !race_queue_unit(ENEMY, 2, U_SCV));
    assert(race_minerals[ENEMY] == 400 && b[2].queue_count == 2);
    race_mode = 1;
    waiting_producer(3, SIDE, B_LAIR);
    b[3].under_construction = 1;
    b[3].queue_count = 2;
    b[3].queue[0] = b[3].queue[1] = U_ZERGLING;
    Sc64RaceSupplyLedger zerg = race_owner_supply_ledger(SIDE, R_ZERG);
    assert(zerg.provided == 2 && zerg.queued == 4);
    assert(!race_queue_waiting_units_supported(&b[3], U_ZERGLING) && terran_reserved(SIDE) == 40);

    waiting_fixture(1, 1);
    for (int i = 1; i <= 24; ++i) waiting_producer(i, SIDE, B_DEPOT);
    for (int i = 1; i <= 99; ++i) put_unit(i, SIDE, U_TANK, 80, 80, 0, 1);
    assert(race_owner_supply_capacity(SIDE) > SC64_SUPPLY_LIMIT);
    assert(race_queue_unit(SIDE, 0, U_SCV) && terran_reserved(SIDE) == SC64_SUPPLY_LIMIT);
    assert(!race_queue_unit(SIDE, 0, U_SCV) && minerals == 450 && b[0].queue_count == 1);
    puts("PASS paid waiting heads, hard cap, competing producers and independent ledgers");
}
#endif

int main(void)
{
    game_init();
    if (queue_cancel_tests()) return fail(1, "P10 shared queue gesture");
    fixture();
    b[0].owner=SIDE; b[0].type=B_GATEWAY; b[0].alive=1;
    b[0].queue[0]=U_DRAGOON; b[0].queue_count=1;
    selected_building=0;
    int prior=race_owner_queued_supply(SIDE);
    if (prior != unit_supply(U_DRAGOON)) return fail(2, "P10 queued supply setup");
    tap(KEY_L);
    if (race_owner_queued_supply(SIDE)!=0 || b[0].queue_count ||
        minerals!=500+unit_cost(U_DRAGOON) || gas!=unit_gas_cost(U_DRAGOON))
        return fail(3, "P10 queued supply cancellation");
#ifdef SC64_PAID_WAITING_UNITS
    paid_waiting_units();
#endif
    puts("PASS P10 queue gesture and reservation");
    return 0;
}
