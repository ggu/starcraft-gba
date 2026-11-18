/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored contention using the normal resource order and race journal. */
#define main worker_gather_fixture_main
#include "sc64_worker_gather_game.c"
#undef main

enum { TEST_PATCH = 4 };

static void assign_mineral(int slot, int patch)
{
    Unit *q = &u[slot];
    q->x = q->tx = m[patch].x;
    q->y = q->ty = m[patch].y;
    q->order = ORDER_MINE;
    q->target = patch;
    q->carry = q->work_timer = 0;
    q->build = -1;
    sc64_worker_cargo_clear(q);
    sc64_worker_gather_pending_set(q, 0);
}

static void contested_scene(int race, int worker_type, int depot_type, int workers[2])
{
    reset_scene(race, R_TER);
    set_depot(0, SIDE, depot_type, 248, 320);
    m[TEST_PATCH].x = 320;
    m[TEST_PATCH].y = 320;
    m[TEST_PATCH].amount = 120;
    for (int i = 0; i < 2; ++i) {
        workers[i] = spawn_unit(SIDE, worker_type, 320, 320);
        assert(workers[i] >= 0);
        assign_mineral(workers[i], TEST_PATCH);
    }
}

static void test_handoff(int race, int worker_type, int depot_type)
{
    int workers[2];
    contested_scene(race, worker_type, depot_type, workers);
    Unit *first = &u[workers[0]], *second = &u[workers[1]];
    for (int elapsed = 1; elapsed < 45; ++elapsed) {
        one_tick();
        assert(first->work_timer == 45 - elapsed && !first->carry);
        assert(!second->work_timer && !second->carry && m[TEST_PATCH].amount == 120);
    }
    one_tick();
    assert(first->carry == 8 && !first->work_timer && m[TEST_PATCH].amount == 112);
    assert(!second->work_timer && !second->carry);
    one_tick();
    assert(first->carry == 8 && second->work_timer == 44);
}

static void test_interruption_and_independence(void)
{
    int workers[2];
    contested_scene(R_TER, U_SCV, B_COMMAND, workers);
    Unit *first = &u[workers[0]], *second = &u[workers[1]];
    int other = spawn_unit(SIDE, U_SCV, 344, 320);
    assert(other >= 0);
    m[TEST_PATCH + 1].x = 344;
    m[TEST_PATCH + 1].y = 320;
    m[TEST_PATCH + 1].amount = 120;
    assign_mineral(other, TEST_PATCH + 1);
    one_tick();
    assert(first->work_timer == 44 && !second->work_timer && u[other].work_timer == 44);
    first->selected = 1;
    stop_selected();
    assert(first->order == ORDER_IDLE && !first->work_timer);
    one_tick();
    assert(m[TEST_PATCH].amount == 120 && second->work_timer == 44);
    assert(u[other].work_timer == 43);
}

static void test_ineligible_partial_timer(void)
{
    int workers[2];
    contested_scene(R_TER, U_SCV, B_COMMAND, workers);
    /* A different owner with no depot cannot progress automatic mining. Its
     * saved partial timer must not block the eligible worker on this patch. */
    u[workers[0]].owner = ENEMY;
    u[workers[0]].work_timer = 7;
    one_tick();
    assert(u[workers[0]].work_timer == 7);
    assert(u[workers[1]].work_timer == 44);
    assert(m[TEST_PATCH].amount == 120);
}

static void test_contested_journal(int legacy_overlap)
{
    gba_init();
    game_init();
    race_mode = 1;
    race_human = R_PROTOSS;
    race_bots = 1;
    race_bot_races[0] = R_TER;
    race_skirmish_map = 0;
    race_skirmish_init();

    int workers[2], count = 0, patch = -1;
    for (int i = 0; i < MAX_UNITS && count < 2; ++i)
        if (u[i].alive && u[i].owner == SIDE && IS_WORKER(u[i].type))
            workers[count++] = i;
    for (int i = 0; i < MAX_MINERALS && patch < 0; ++i)
        if (m[i].amount >= 16) patch = i;
    assert(count == 2 && patch >= 0);
    assign_mineral(workers[0], patch);
    assign_mineral(workers[1], patch);
    Unit *first = &u[workers[0]], *second = &u[workers[1]];
    int amount = m[patch].amount;
    first->work_timer = legacy_overlap ? 3 : 0;
    second->work_timer = 2;
    assert(race_save_game());
    first->order = second->order = ORDER_IDLE;
    first->work_timer = second->work_timer = 0;
    memset(sc64_mineral_requests, 0xff, sizeof(sc64_mineral_requests));
    assert(race_load_game());
    assert(first->work_timer == (legacy_overlap ? 3 : 0) && second->work_timer == 2);

    one_tick();
    assert(m[patch].amount == amount);
    assert(first->work_timer == (legacy_overlap ? 2 : 0));
    assert(second->work_timer == (legacy_overlap ? 2 : 1));
    if (legacy_overlap) {
        one_tick();
        one_tick();
        assert(first->carry == 8 && second->work_timer == 2);
    } else {
        one_tick();
        assert(second->carry == 8 && !first->work_timer);
    }
    assert(m[patch].amount == amount - 8);
    one_tick();
    assert(legacy_overlap ? second->work_timer == 1 : first->work_timer == 44);
}

int main(void)
{
    test_handoff(R_TER, U_SCV, B_COMMAND);
    test_handoff(R_ZERG, U_DRONE, B_HATCHERY);
    test_handoff(R_PROTOSS, U_PROBE, B_NEXUS);
    test_interruption_and_independence();
    test_ineligible_partial_timer();
    test_contested_journal(0);
    test_contested_journal(1);
    puts("shared mineral contention, release and journal continuation passed");
    return 0;
}
