/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Physical-key lobby/save/restart, with combat checkpoints set up through
 * real actor creation/destruction to avoid needing a multi-hour match. */
#define main sc64_controller_main
#include "sc64_skirmish_campaign_run.c"
#undef main

static int building_of(int owner)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) return i;
    return -1;
}

static int worker_of(int owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && sc64_race_is_worker(u[i].type)) return i;
    return -1;
}

static void launch(int melee)
{
    gba_init(); game_init(); prior_keys = 0;
    host_ticks = 0; won = lost = 0; /* fresh process semantics for each case */
    gba_test_set_elapsed_milliseconds(0xf3ae);
    assert(race_front.page == SC_FRONT_HOME);
    for (int i = 0; i < 2 && race_front.row != 1; ++i) tap(KEY_DOWN);
    assert(race_front.row == 1);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_SKIRMISH);
    for (int i = 0; i < SC64_SKIRMISH_IMPORTED_COUNT && race_front.map != SC64_MAP_BLOOD_BATH; ++i)
        tap(KEY_RIGHT);
    assert(race_front.map == SC64_MAP_BLOOD_BATH);
    while (race_front.row < SC_FRONT_BOT_FIRST_ROW) tap(KEY_DOWN);
    for (int i = 0; i < 3; ++i, tap(KEY_DOWN))
        for (int n = 0; race_front.bot_races[i] != i && n < 5; ++n) tap(KEY_RIGHT);
    assert(race_front.row == SC_FRONT_SKIRMISH_MODE_ROW);
    if (race_front.skirmish_melee != (unsigned)melee) tap(KEY_RIGHT);
    tap(KEY_DOWN); tap(KEY_A);
    assert(screen == SCREEN_PLAY && race_mode && race_bots == 3 &&
           race_bot_races[0] == R_TER && race_bot_races[1] == R_ZERG && race_bot_races[2] == R_PROTOSS);
    assert(race_skirmish_start_order && race_skirmish_melee == (unsigned)melee &&
           !race_skirmish_eliminated);
}

static void check_word_policy(int melee, unsigned starts, int legacy, int tagged)
{
    int race = -1;
    unsigned sparse = 99, order = 99, allied = 99, eliminated = 99;
    assert(sc64_skirmish_read_match_word(1, 1, 3, legacy, &race, &sparse, &order, &allied));
    assert(race == R_PROTOSS && !sparse && order == starts && allied == (unsigned)melee);
    assert(!sc64_skirmish_read_match_word(1, 1, 3, tagged, &race, &sparse, &order, &allied));
    assert(!sc64_skirmish_read_elimination_word(1, 0, 3, tagged, &race, &sparse, &order,
                                               &allied, &eliminated));
    assert(!sc64_skirmish_read_elimination_word(0, 1, 3, tagged, &race, &sparse, &order,
                                               &allied, &eliminated));
    assert(!sc64_skirmish_read_elimination_word(1, 1, 1,
        sc64_skirmish_start_word(R_TER, 4, SC64_SKIRMISH_BLOOD_BATH_SOLO_ORDER) | (2 << 13),
        &race, &sparse, &order, &allied, &eliminated));
    assert(sc64_skirmish_read_elimination_word(1, 1, 1,
        sc64_skirmish_start_word(R_TER, 4, SC64_SKIRMISH_BLOOD_BATH_SOLO_ORDER) | (1 << 13),
        &race, &sparse, &order, &allied, &eliminated));
    assert(race == R_TER && sparse == 4 && order == SC64_SKIRMISH_BLOOD_BATH_SOLO_ORDER &&
           !allied && eliminated == 1);
}

static void check_elimination(int melee)
{
    launch(melee);
    const unsigned starts = race_skirmish_start_order;
    assert(race_save_game());
    const int legacy = race_data.state[23];
    game_init(); prior_keys = 0;
    assert(race_load_game() && !race_skirmish_eliminated && race_skirmish_start_order == starts);
    const int army = spawn_unit(ENEMY, U_MARINE, 360, 380);
    const int worker = worker_of(ENEMY);
    const int first = building_of(ENEMY);
    const int second_worker = worker_of(RESCUABLE);
    assert(army >= 0 && worker >= 0 && first >= 0 && second_worker >= 0);
    int depot = spawn_build(ENEMY, B_DEPOT, 400, 380, 1);
    assert(depot >= 0 && b[depot].alive);
    destroy_building(first, SIDE);
    game_update(0, 0);
    assert(!won && !lost && !race_skirmish_eliminated && u[army].owner == ENEMY);
    assert(building_of(ENEMY) == depot && b[depot].under_construction);
    u[army].order = ORDER_ATTACK_MOVE; u[army].tx = 460; u[army].ty = 460;
    u[worker].carry = 6;

    destroy_building(depot, SIDE);
    game_update(0, 0);
    assert(!won && !lost && screen == SCREEN_PLAY && race_skirmish_eliminated == 1);
    assert(u[army].alive && u[army].owner == NEUTRAL && u[army].raw_owner == 11 &&
           u[army].order == ORDER_IDLE && u[army].target == -1 && u[army].hp > 0);
    assert(u[worker].alive && u[worker].owner == NEUTRAL && u[worker].order == ORDER_IDLE &&
           !u[worker].carry && u[worker].target == -1);
    assert(!owners_hostile(SIDE, NEUTRAL) && !owners_hostile(RESCUABLE, NEUTRAL) &&
           !owners_hostile(NEUTRAL, SIDE) && owners_hostile(SIDE, RESCUABLE));
    assert(owners_hostile(RESCUABLE, 4) == !melee);
    assert(!race_skirmish_retired_owner(race_skirmish_eliminated, RESCUABLE));
    assert(building_of(RESCUABLE) >= 0 && building_of(4) >= 0 && u[second_worker].alive);
    int x = u[army].x, y = u[army].y;
    /* With no carry/order this worker can resume only when the surviving
     * CPU's planner gets its turn; ongoing mining alone would not prove it. */
    u[second_worker].order = ORDER_IDLE;
    u[second_worker].target = -1;
    u[second_worker].carry = 0;
#ifdef SC64_WORKER_CARGO_KIND
    sc64_worker_cargo_clear(&u[second_worker]);
    sc64_worker_gather_pending_set(&u[second_worker], 0);
#endif
    for (int i = 0; i < 120; ++i) game_update(0, 0);
    assert(u[army].x == x && u[army].y == y && u[army].order == ORDER_IDLE);
    assert(u[second_worker].alive && u[second_worker].owner == RESCUABLE &&
           u[second_worker].order == ORDER_MINE && u[second_worker].target >= 0);
    assert(race_save_game());
    const int tagged = race_data.state[23];
    assert(tagged == (legacy | (1 << SC64_SKIRMISH_ELIM_SHIFT)));
    check_word_policy(melee, starts, legacy, tagged);

    gba_test_set_elapsed_milliseconds(0xf6e0);
    game_init(); prior_keys = 0;
    assert(race_load_game());
    assert(race_skirmish_eliminated == 1 && race_skirmish_start_order == starts &&
           race_skirmish_melee == (unsigned)melee && u[army].owner == NEUTRAL &&
           u[army].raw_owner == 11 && u[army].x == x && u[army].y == y && !won && !lost);
    assert(race_save_game() && race_data.state[23] == tagged);
    {
        /* Re-tag the still living second CPU as defeated, then run the
         * detached candidate verifier. Neither live state nor SRAM changes. */
        RaceLoadPlan plan = {0};
        assert(race_load_preflight(&plan));
        race_data.state[23] = tagged | (2 << SC64_SKIRMISH_ELIM_SHIFT);
        assert(!race_load_preflight(&plan));
        assert(race_skirmish_eliminated == 1 && u[army].owner == NEUTRAL);
        assert(race_load_game() && race_data.state[23] == tagged);
    }
    assert(building_of(RESCUABLE) >= 0 && building_of(4) >= 0);

    tap(KEY_START); assert(screen == SCREEN_PAUSE);
    tap(KEY_R); assert(screen == SCREEN_PLAY && !race_skirmish_eliminated &&
                       race_skirmish_start_order == starts && building_of(ENEMY) >= 0);

    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && (b[i].owner == ENEMY || b[i].owner == RESCUABLE))
            destroy_building(i, SIDE);
    game_update(0, 0);
    assert(!won && !lost && race_skirmish_eliminated == 3 && building_of(4) >= 0);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == 4) destroy_building(i, SIDE);
    game_update(0, 0);
    assert(won && !lost && race_skirmish_eliminated == 7);
}

#ifdef SC64_TRANSPORT_RUNTIME
static void check_deferred_transfer(void)
{
    launch(0);
    int ship = spawn_unit(ENEMY, U_DROPSHIP, 500, 500);
    int passenger = worker_of(ENEMY);
    assert(ship >= 0 && passenger >= 0);
    u[passenger].order = ORDER_TRANSPORTED;
    u[passenger].target = ship;
    u[passenger].x = u[ship].x; u[passenger].y = u[ship].y;
    destroy_building(building_of(ENEMY), SIDE);
    race_skirmish_outcome();
    assert(!race_skirmish_eliminated && u[ship].owner == ENEMY &&
           u[passenger].owner == ENEMY && u[passenger].target == ship);
    u[passenger].order = ORDER_IDLE;
    u[passenger].target = -1;
    race_skirmish_outcome();
    assert(race_skirmish_eliminated == 1 && u[ship].owner == NEUTRAL &&
           u[passenger].owner == NEUTRAL);
}
#endif

int main(void)
{
    check_elimination(0);
    check_elimination(1);
#ifdef SC64_TRANSPORT_RUNTIME
    check_deferred_transfer();
#endif
    puts("Blood Bath Melee/FFA elimination, surviving CPUs and SRAM: pass");
    return 0;
}
