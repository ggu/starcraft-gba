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

static void launch_as(int melee, int human)
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
    if (human >= 0) {
        tap(KEY_DOWN);
        for (int i = 0; i < 4 && race_front.race != human; ++i) tap(KEY_RIGHT);
        assert(race_front.race == human);
    }
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

static void launch(int melee) { launch_as(melee, -1); }

static void prepare_written_save(RaceLoadPlan *plan)
{
#ifdef SC64_WORKER_CARGO_KIND
    /* The compact Carrier writer packs into the candidate scratch buffer.
     * Reopen the journal to get decoded state and its authenticated plan. */
    GbaSaveSelection selected;
    assert(race_worker_cargo_select(plan, &selected));
#else
    assert(race_load_preflight(plan));
#endif
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
    RaceLoadPlan plan = {0};
    const unsigned starts = race_skirmish_start_order;
    assert(race_save_game());
    prepare_written_save(&plan);
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
    prepare_written_save(&plan);
    const int tagged = race_data.state[23];
    assert(tagged == (legacy | (1 << SC64_SKIRMISH_ELIM_SHIFT)));
    check_word_policy(melee, starts, legacy, tagged);

    gba_test_set_elapsed_milliseconds(0xf6e0);
    game_init(); prior_keys = 0;
    assert(race_load_game());
    assert(race_skirmish_eliminated == 1 && race_skirmish_start_order == starts &&
           race_skirmish_melee == (unsigned)melee && u[army].owner == NEUTRAL &&
           u[army].raw_owner == 11 && u[army].x == x && u[army].y == y && !won && !lost);
    assert(race_save_game());
    prepare_written_save(&plan);
    assert(race_data.state[23] == tagged);
    /* Re-tag the still living second CPU as defeated, then validate the
     * decoded candidate without publishing it. */
    race_data.state[23] = tagged | (2 << SC64_SKIRMISH_ELIM_SHIFT);
    assert(!race_load_preflight(&plan));
    assert(race_skirmish_eliminated == 1 && u[army].owner == NEUTRAL);
    assert(race_load_game() && race_data.state[23] == tagged);
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
    assert(!won && !lost && screen == SCREEN_PLAY && race_skirmish_eliminated == 7);
    const int retired_frame = frame;

    /* Pause and save between the two passes, then reload through HOME. */
    tap(KEY_START);
    assert(screen == SCREEN_PAUSE && frame == retired_frame);
    tap(KEY_B);
    prepare_written_save(&plan);
    assert(race_data.state[7] == retired_frame && !race_data.state[18] &&
           !race_data.state[19] && ((unsigned)race_data.state[23] & SC64_SKIRMISH_ELIM_BITS) ==
           SC64_SKIRMISH_ELIM_BITS);
    game_init(); prior_keys = 0;
    assert(screen == SCREEN_TITLE);
    input(KEY_L);
    assert(!won && !lost && screen == SCREEN_PLAY && race_skirmish_eliminated == 7 &&
           race_skirmish_melee == (unsigned)melee && frame == retired_frame);
    input(0);
    assert(won && !lost && screen == SCREEN_RESULT && race_skirmish_eliminated == 7 &&
           frame == retired_frame + 1);
    game_init(); prior_keys = 0;
    won = lost = 0; /* Model a fresh session before loading the terminal save. */
    input(KEY_L);
    assert(won && !lost && screen == SCREEN_RESULT && race_skirmish_eliminated == 7 &&
           frame == retired_frame + 1);
}

#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
static void check_docked_carrier_retirement(void)
{
    launch(0);
    const int owner = 4;
    const int gap = spawn_unit(owner, U_ZEALOT, 480, 500);
    const int carrier = spawn_unit(owner, U_P7_CARRIER, 500, 500);
    const int empty = spawn_unit(owner, U_P7_CARRIER, 520, 500);
    assert(gap >= 0 && carrier > gap && empty > carrier);
    hit_target(gap, 5000, U_MARINE, SIDE);
    assert(!u[gap].alive && race_p7_carrier_training_stock(carrier, 2) == 2);
    int second = -1;
    for (int i = carrier + 1; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR && u[i].build == carrier) second = i;
    assert(second > carrier && u[gap].alive && u[gap].build == carrier &&
           sc64_p7_carrier_child_state(&u[gap]) == SC64_P7_INTERCEPTOR_INSIDE);
    const unsigned early_rank = sc64_p7_carrier_child_rank(&u[gap]);
    const unsigned late_rank = sc64_p7_carrier_child_rank(&u[second]);
    assert(early_rank != late_rank);
    const int ore = race_owner_minerals(owner), vespene = race_owner_gas(owner);
    const int neutral_bank = race_owner_minerals(NEUTRAL);
    assert(race_p7_carrier_training_queue_owner(carrier));
    assert(race_p7_carrier_training_queue_owner(empty));
    assert(u[carrier].carry == 1 && u[empty].carry == 1);
    const int paid_bank = ore - 2 * SC64_P7_INTERCEPTOR_COST;
    assert(race_owner_minerals(owner) == paid_bank);

#ifdef SC64_TRANSPORT_RUNTIME
    const int transport = spawn_unit(owner, U_DROPSHIP, 540, 500);
    const int passenger = spawn_unit(owner, U_MARINE, 540, 500);
    assert(transport >= 0 && passenger >= 0);
    assert(race_transport_request_board(passenger, transport));
    assert(race_transport_tick_unit(passenger));
    assert(u[passenger].order == ORDER_TRANSPORTED);
#endif
    destroy_building(building_of(owner), SIDE);
    static Unit before[MAX_UNITS];
#ifdef SC64_TRANSPORT_RUNTIME
    ++u[passenger].x;
    memcpy(before, u, sizeof(u));
    race_skirmish_outcome();
    assert(!race_skirmish_eliminated && !memcmp(before, u, sizeof(u)));
    assert(race_owner_minerals(owner) == paid_bank && race_owner_gas(owner) == vespene);
    --u[passenger].x;
#endif
    sc64_p7_carrier_child_set_rank(&u[second], early_rank);
    memcpy(before, u, sizeof(u));
    race_skirmish_outcome();
    assert(!race_skirmish_eliminated && !memcmp(before, u, sizeof(u)));
    assert(race_owner_minerals(owner) == paid_bank && race_owner_gas(owner) == vespene);
    sc64_p7_carrier_child_set_rank(&u[second], late_rank);

    race_skirmish_outcome();
    assert(!won && !lost && screen == SCREEN_PLAY && race_skirmish_eliminated == 4);
    assert(u[carrier].alive && u[carrier].owner == NEUTRAL && u[carrier].raw_owner == 11 &&
           u[empty].alive && u[empty].owner == NEUTRAL && u[empty].raw_owner == 11);
    assert(!u[carrier].carry && !u[empty].carry &&
           race_owner_minerals(owner) == ore && race_owner_gas(owner) == vespene &&
           race_owner_minerals(NEUTRAL) == neutral_bank);
    assert(race_p7_carrier_training_child_count(carrier) == 2 &&
           !race_p7_carrier_training_child_count(empty));

    tap(KEY_START);
    assert(screen == SCREEN_PAUSE);
    tap(KEY_B);
    game_init(); prior_keys = 0;
    input(KEY_L);
    assert(screen == SCREEN_PLAY && !won && !lost && race_skirmish_eliminated == 4);
    assert(u[carrier].alive && u[carrier].owner == NEUTRAL && u[carrier].raw_owner == 11 &&
           u[empty].alive && u[empty].owner == NEUTRAL && u[empty].raw_owner == 11 &&
           u[carrier].order == ORDER_IDLE && u[empty].order == ORDER_IDLE);
    assert(!u[carrier].carry && !u[empty].carry &&
           race_owner_minerals(owner) == ore && race_owner_gas(owner) == vespene &&
           race_owner_minerals(NEUTRAL) == neutral_bank);
    race_minerals[NEUTRAL] = SC64_P7_INTERCEPTOR_COST;
    assert(!race_p7_carrier_training_queue_owner(carrier) &&
           !race_p7_carrier_training_queue_owner(empty));
    race_minerals[NEUTRAL] = neutral_bank;
    for (int i = 0; i < 8; ++i) game_update(0, 0);
    assert(u[gap].alive && u[gap].owner == NEUTRAL && u[gap].raw_owner == 11 &&
           u[gap].build == carrier && sc64_p7_carrier_child_rank(&u[gap]) == early_rank &&
           sc64_p7_carrier_child_state(&u[gap]) == SC64_P7_INTERCEPTOR_INSIDE);
    assert(u[second].alive && u[second].owner == NEUTRAL && u[second].raw_owner == 11 &&
           u[second].build == carrier && sc64_p7_carrier_child_rank(&u[second]) == late_rank &&
           sc64_p7_carrier_child_state(&u[second]) == SC64_P7_INTERCEPTOR_INSIDE);
    hit_target(carrier, 5000, U_MARINE, SIDE);
    assert(!u[carrier].alive && !u[gap].alive && !u[second].alive && u[empty].alive);
    assert(race_save_game());
}

static void check_fenced_final_result(int human_defeat)
{
    launch(0);
    const int owner = 4, bit = 4, type = U_P7_CARRIER;
    const int ship = spawn_unit(owner, type, 500, 500);
    const int survivor = spawn_unit(owner, U_MARINE, 480, 500);
    assert(ship >= 0 && survivor >= 0 && race_p7_carrier_training_queue_owner(ship));
    assert(race_skirmish_neutral_transfer_ready(&u[ship]));
    const int trainee = race_p7_carrier_training_start(ship);
    assert(trainee >= 0 && !race_skirmish_neutral_transfer_ready(&u[trainee]));
    assert(race_p7_carrier_training_queue_owner(ship));
    const int paid_bank = race_owner_minerals(owner);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner != SIDE && b[i].owner != owner)
            destroy_building(i, SIDE);
    game_update(0, 0);
    assert(!won && !lost && race_skirmish_eliminated == (7 ^ bit));
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) destroy_building(i, SIDE);
    game_update(0, 0);
    assert(!won && !lost && screen == SCREEN_PLAY && race_skirmish_eliminated == (7 ^ bit));
    assert(u[ship].owner == owner && u[survivor].owner == owner && owners_hostile(SIDE, owner));
    assert(u[ship].carry == 1 && race_owner_minerals(owner) == paid_bank);
    if (human_defeat) {
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == SIDE) destroy_building(i, owner);
        game_update(0, 0);
        assert(!won && lost && screen == SCREEN_RESULT && race_skirmish_eliminated == (7 ^ bit));
    } else {
        hit_target(ship, 5000, U_MARINE, SIDE);
        assert(!u[ship].alive);
        game_update(0, 0);
        assert(!won && !lost && screen == SCREEN_PLAY && race_skirmish_eliminated == 7);
        assert(u[survivor].alive && u[survivor].owner == NEUTRAL &&
               !owners_hostile(SIDE, NEUTRAL));
        game_update(0, 0);
        assert(won && !lost && screen == SCREEN_RESULT && race_skirmish_eliminated == 7);
    }
}
#endif

#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
static void check_battlecruiser_retirement(unsigned phase)
{
    launch(0);
    const int reactor = phase != SC64_BC_SHOT_CHARGING;
    const int type = sc64_race_unit_from_raw(12);
    assert(type >= 0);
    const int ship = spawn_unit(ENEMY, type, 500, 500);
    const int target = spawn_unit(SIDE, U_MARINE, 520, 500);
    assert(ship >= 0 && target >= 0);
    const u16 old_tech = (SC64_BC_TECH_YAMATO | (reactor ? SC64_BC_TECH_REACTOR : 0)) << (2 * ENEMY);
    sc64_battlecruiser_tech_bits = old_tech;
    u[ship].carry = reactor ? 225 * 256 + 7 : 200 * 256;
    u[ship].work_timer = 64 << 3;
    const int initial_energy = u[ship].carry;
    assert(race_bc_yamato_issue(ship, target));
    if (phase != SC64_BC_SHOT_APPROACH) {
        if (!sc64_battlecruiser_source_tick_due()) ++frame;
        assert(race_bc_yamato_tick_caster(ship));
        assert(u[ship].carry == initial_energy - 150 * 256);
    }
    if (phase == SC64_BC_SHOT_FLIGHT) {
        sc64_bc_shots[0].age = SC64_BC_YAMATO_RELEASE_SOURCE_TICK;
        assert(race_bc_yamato_launch_resolved(0, 64, 0, 1040, 1000));
    }
    assert(sc64_bc_shots[0].state == phase);
    const Sc64BcYamatoShot shot = sc64_bc_shots[0];
    const int energy = u[ship].carry;
    const int last = building_of(ENEMY);
    assert(last >= 0);

#ifdef SC64_TRANSPORT_RUNTIME
    const int transport = spawn_unit(ENEMY, U_DROPSHIP, 540, 500);
    const int passenger = spawn_unit(ENEMY, U_MARINE, 540, 500);
    assert(transport > ship && passenger > transport);
    assert(race_transport_request_board(passenger, transport));
    assert(race_transport_tick_unit(passenger));
    if (phase == SC64_BC_SHOT_FLIGHT) {
        assert(race_save_game());
        destroy_building(last, SIDE);
        ++u[passenger].x;
        static Unit before[MAX_UNITS];
        memcpy(before, u, sizeof(u));
        race_skirmish_outcome();
        assert(!race_skirmish_eliminated && !won && !lost && screen == SCREEN_PLAY);
        assert(sc64_battlecruiser_tech_bits == old_tech && !memcmp(before, u, sizeof(u)) &&
               !memcmp(&shot, &sc64_bc_shots[0], sizeof(shot)));
        game_init(); prior_keys = 0;
        assert(race_load_game() && b[last].alive && !race_skirmish_eliminated &&
               u[passenger].x == u[transport].x && u[passenger].target == transport &&
               sc64_battlecruiser_tech_bits == old_tech &&
               !memcmp(&shot, &sc64_bc_shots[0], sizeof(shot)));
    }
#endif

    destroy_building(last, SIDE);
    race_skirmish_outcome();
    const u16 new_tech = old_tech | ((reactor ? SC64_BC_TECH_REACTOR : 0) << (2 * NEUTRAL));
    assert(race_skirmish_eliminated == 1 && !won && !lost && screen == SCREEN_PLAY);
    assert(u[ship].alive && u[ship].owner == NEUTRAL && u[ship].raw_owner == 11 &&
           u[ship].carry == energy && u[ship].order == ORDER_IDLE && u[ship].target == -1 &&
           u[ship].build == -1 && !u[ship].work_timer && sc64_battlecruiser_tech_bits == new_tech);
    Sc64BcYamatoShot expected = {0};
    if (phase == SC64_BC_SHOT_FLIGHT) {
        expected = shot;
        expected.caster = SC64_BC_SHOT_NO_CASTER;
    }
    assert(!memcmp(&expected, &sc64_bc_shots[0], sizeof(expected)));

    tap(KEY_START);
    assert(screen == SCREEN_PAUSE);
    tap(KEY_B);
    game_init(); prior_keys = 0;
    input(KEY_L);
    assert(screen == SCREEN_PLAY && race_skirmish_eliminated == 1 && u[ship].alive &&
           u[ship].owner == NEUTRAL && u[ship].raw_owner == 11 && u[ship].carry == energy &&
           sc64_battlecruiser_tech_bits == new_tech &&
           !memcmp(&expected, &sc64_bc_shots[0], sizeof(expected)));
#ifdef SC64_TRANSPORT_RUNTIME
    assert(u[transport].owner == NEUTRAL && u[passenger].owner == NEUTRAL &&
           u[passenger].order == ORDER_TRANSPORTED && u[passenger].target == transport &&
           race_transport_cargo_space(transport) == 1);
#endif
    assert(!race_bc_yamato_issue(ship, target));
    if (phase == SC64_BC_SHOT_FLIGHT) {
        for (int step = 0; step < 255 && sc64_bc_shots[0].state; ++step)
            assert(race_bc_yamato_projectile_step(0));
        assert(!sc64_bc_shots[0].state && !u[target].alive);
    }
}
#endif

#ifdef SC64_TRANSPORT_RUNTIME
static void check_neutral_cargo(int ship_type, int owner)
{
    launch(0);
    int ship = spawn_unit(owner, ship_type, 500, 500);
    int passenger = spawn_unit(owner, U_MARINE, 500, 500);
    int pending = spawn_unit(owner, U_MARINE, 470, 500);
    assert(ship >= 0 && passenger >= 0 && pending >= 0);
    assert(race_transport_request_board(passenger, ship));
    assert(race_transport_tick_unit(passenger) && u[passenger].order == ORDER_TRANSPORTED);
    assert(race_transport_request_board(pending, ship));
    assert(race_transport_request_unload(ship, 510, 510));
    destroy_building(building_of(owner), SIDE);
    race_skirmish_outcome();
    const unsigned mark = 1u << (owner == ENEMY ? 0 : 2);
    assert(race_skirmish_eliminated == mark && !won && !lost);
    assert(u[ship].alive && u[ship].owner == NEUTRAL && u[ship].raw_owner == 11 &&
           u[ship].order == ORDER_IDLE && u[ship].target == -1);
    assert(u[passenger].alive && u[passenger].owner == NEUTRAL &&
           u[passenger].raw_owner == 11 && u[passenger].order == ORDER_TRANSPORTED &&
           u[passenger].target == ship && !u[passenger].selected &&
           u[passenger].x == u[ship].x && u[passenger].y == u[ship].y);
    assert(u[pending].alive && u[pending].owner == NEUTRAL &&
           u[pending].order == ORDER_IDLE && u[pending].target == -1);
    assert(race_transport_cargo_space(ship) == 1);
    assert(!race_transport_request_board(pending, ship) &&
           !race_transport_request_unload(ship, 510, 510));
    assert(race_save_game());
    u[passenger].target = pending;
    assert(!race_save_game()); /* a neutral nontransport is not a cargo host */
    u[passenger].target = ship;
    u[passenger].raw_owner = 3;
    assert(!race_save_game()); /* marked cargo must have neutral provenance */
    u[passenger].raw_owner = 11;

    game_init(); prior_keys = 0;
    assert(race_load_game());
    for (int i = 0; i < 8; ++i) game_update(0, 0);
    assert(race_skirmish_eliminated == mark && !won && !lost &&
           u[ship].owner == NEUTRAL && u[ship].order == ORDER_IDLE &&
           u[passenger].owner == NEUTRAL && u[passenger].order == ORDER_TRANSPORTED &&
           u[passenger].target == ship && race_transport_cargo_space(ship) == 1);
    assert(!race_transport_request_board(pending, ship) &&
           !race_transport_request_unload(ship, 510, 510));
    assert(race_save_game());
}

#ifdef SC64_SHARED_TRANSPORT
static void check_neutral_ventral(void)
{
    launch(0);
    const int owner = RESCUABLE;
    const int research = sc64_sk_transport_owner_index(owner);
    int ship = spawn_unit(owner, U_OVERLORD, 500, 500);
    int passenger = spawn_unit(owner, U_ZERGLING, 500, 500);
    assert(ship >= 0 && passenger >= 0 && research >= 0);
    assert(!race_transport_request_board(passenger, ship));
    race_skirmish_transport_tech.owner[research].researched = 1;
    assert(race_transport_request_board(passenger, ship));
    assert(race_transport_tick_unit(passenger) && u[passenger].order == ORDER_TRANSPORTED);
    destroy_building(building_of(owner), SIDE);

    /* Reject the entire retirement, including the capability, if cargo is
     * malformed. Restore the valid group and let the normal outcome run. */
    int x = u[passenger].x;
    ++u[passenger].x;
    race_skirmish_outcome();
    assert(!race_skirmish_eliminated && !race_skirmish_neutral_ventral &&
           u[ship].owner == owner && u[passenger].owner == owner);
    u[passenger].x = x;
    race_skirmish_outcome();
    assert(race_skirmish_eliminated == 2 && race_skirmish_neutral_ventral &&
           u[ship].owner == NEUTRAL && u[ship].order == ORDER_IDLE &&
           u[passenger].owner == NEUTRAL && u[passenger].order == ORDER_TRANSPORTED &&
           u[passenger].target == ship && race_transport_cargo_space(ship) == 1 &&
           !race_transport_request_unload(ship, 510, 510));
    assert(race_save_game());
    RaceLoadPlan plan = {0};
    prepare_written_save(&plan);
    const int marked = race_data.state[23];
    u8 wire[16];
    Sc64SkTransportTech previous;
    race_sk_transport_wire_encode(wire);
    assert((wire[SC64_NEUTRAL_VENTRAL_BYTE] & SC64_NEUTRAL_VENTRAL_BIT) &&
           !sc64_sk_transport_decode(&previous, wire, MAX_BUILDINGS));
    assert(race_sk_transport_wire_decode(wire) && race_sk_transport_candidate_valid(&race_data));
    race_data.state[23] = marked & ~(int)SC64_SKIRMISH_ELIM_BITS;
    assert(!race_sk_transport_candidate_valid(&race_data));
    race_data.state[23] = marked;
    race_skirmish_transport_candidate.owner[research].researched = 0;
    assert(!race_sk_transport_candidate_valid(&race_data));
    wire[SC64_NEUTRAL_VENTRAL_BYTE] &= (u8)~SC64_NEUTRAL_VENTRAL_BIT;
    assert(sc64_sk_transport_decode(&previous, wire, MAX_BUILDINGS) &&
           race_sk_transport_wire_decode(wire) && !race_skirmish_neutral_ventral_candidate &&
           !race_skirmish_transport_save_valid(&race_data, race_cols, race_rows));

    game_init(); prior_keys = 0;
    assert(!race_skirmish_neutral_ventral && race_load_game());
    game_update(0, 0);
    assert(race_skirmish_neutral_ventral && race_skirmish_eliminated == 2 &&
           race_transport_carrier_valid(ship) && race_transport_cargo_space(ship) == 1 &&
           u[passenger].alive && u[passenger].target == ship);
    hit_target(ship, 5000, U_MARINE, SIDE);
    assert(!u[ship].alive && !u[passenger].alive && race_skirmish_neutral_ventral);
    assert(race_save_game());
    game_init(); prior_keys = 0;
    assert(race_load_game() && race_skirmish_neutral_ventral &&
           !u[ship].alive && !u[passenger].alive);
    tap(KEY_START); assert(screen == SCREEN_PAUSE);
    tap(KEY_R); assert(screen == SCREEN_PLAY && !race_skirmish_neutral_ventral);
}

static void check_ventral_seed_scope(void)
{
    launch(0);
    const int owner = RESCUABLE;
    const int research = sc64_sk_transport_owner_index(owner);
    assert(research >= 0);
    race_skirmish_transport_tech.owner[research].researched = 1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].type == U_OVERLORD)
            hit_target(i, 5000, U_MARINE, SIDE);
    destroy_building(building_of(owner), SIDE);
    race_skirmish_outcome();
    assert(race_skirmish_eliminated == 2 && !race_skirmish_neutral_ventral);

    launch(0);
    int empty_ship = spawn_unit(owner, U_OVERLORD, 500, 500);
    assert(empty_ship >= 0);
    race_skirmish_transport_tech.owner[research].researched = 1;
    destroy_building(building_of(owner), SIDE);
    race_skirmish_outcome();
    assert(race_skirmish_eliminated == 2 && race_skirmish_neutral_ventral &&
           race_transport_carrier_valid(empty_ship) && race_transport_cargo_space(empty_ship) == 0);
}

static void check_human_retirement(void)
{
    launch_as(1, R_ZERG);
    int ship = spawn_unit(SIDE, U_OVERLORD, 500, 500);
    int passenger = spawn_unit(SIDE, U_DRONE, 500, 500);
    assert(race_human == R_ZERG && ship >= 0 && passenger >= 0);
    race_skirmish_transport_tech.owner[0].researched = 1;
    assert(race_transport_request_board(passenger, ship));
    assert(race_transport_tick_unit(passenger));
    destroy_building(building_of(SIDE), ENEMY);
    game_update(0, 0);
    assert(lost && !won && screen == SCREEN_RESULT && !race_skirmish_eliminated);
    assert(u[ship].owner == NEUTRAL && u[ship].raw_owner == 11 &&
           u[passenger].owner == NEUTRAL && u[passenger].raw_owner == 11 &&
           u[passenger].order == ORDER_TRANSPORTED && u[passenger].target == ship);
    assert(race_skirmish_neutral_ventral && race_save_game());
    game_init(); prior_keys = 0;
    assert(race_load_game() && lost && !won && !race_skirmish_eliminated &&
           screen == SCREEN_RESULT && race_skirmish_neutral_ventral);
    assert(race_transport_carrier_valid(ship) && race_transport_cargo_space(ship) == 1 &&
           u[passenger].alive && u[passenger].owner == NEUTRAL);
}

#endif
#endif

#ifdef SC64_IMPACT_EVENTS
static void check_queued_elimination(void)
{
    launch(0);
    int human = building_of(SIDE), first = building_of(ENEMY);
    assert(human >= 0 && first >= 0);
    /* Exercise the real projectile queue and normal update ordering without
     * depending on exact weapon travel or the CPU's launch schedule. The
     * earlier impact retires the attacker while other CPUs keep play active. */
    race_impact_launch(MAX_UNITS + first, 4000, 1, SIDE,
                       SC64_STOCK_DMG_NORMAL, U_MARINE, 1);
    race_impact_launch(MAX_UNITS + human, 4000, 1, ENEMY,
                       SC64_STOCK_DMG_NORMAL, U_MARINE, 2);
    game_update(0, 0);
    assert(!b[first].alive && b[human].alive && race_skirmish_eliminated == 1 &&
           screen == SCREEN_PLAY && race_impact_count == 1);
    game_update(0, 0);
    assert(!b[human].alive && !won && lost && screen == SCREEN_RESULT &&
           race_skirmish_eliminated == 1 && race_impact_count == 0);

    /* Both final buildings fall in the same impact pass. Assert the existing
     * HOST precedence in both queue orders, with no native precedence claim. */
    for (int reversed = 0; reversed < 2; ++reversed) {
        launch(0);
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && (b[i].owner == RESCUABLE || b[i].owner == 4))
                destroy_building(i, SIDE);
        game_update(0, 0);
        assert(!won && !lost && race_skirmish_eliminated == 6);
        human = building_of(SIDE); first = building_of(ENEMY);
        assert(human >= 0 && first >= 0);
        for (int n = 0; n < 2; ++n) {
            int hits_human = n ^ reversed;
            race_impact_launch(MAX_UNITS + (hits_human ? human : first), 4000, 1,
                               hits_human ? ENEMY : SIDE, SC64_STOCK_DMG_NORMAL,
                               U_MARINE, 1);
        }
        game_update(0, 0);
        assert(!b[human].alive && !b[first].alive && !won && lost &&
               screen == SCREEN_RESULT && race_skirmish_eliminated == 6 &&
               race_impact_count == 0);
    }
}
#endif

int main(void)
{
    check_elimination(0);
    check_elimination(1);
#ifdef SC64_IMPACT_EVENTS
    check_queued_elimination();
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    check_docked_carrier_retirement();
    check_fenced_final_result(0);
    check_fenced_final_result(1);
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    check_battlecruiser_retirement(SC64_BC_SHOT_APPROACH);
    check_battlecruiser_retirement(SC64_BC_SHOT_CHARGING);
    check_battlecruiser_retirement(SC64_BC_SHOT_FLIGHT);
#endif
#ifdef SC64_TRANSPORT_RUNTIME
    check_neutral_cargo(U_DROPSHIP, ENEMY);
#ifdef SC64_SHARED_TRANSPORT
    int shuttle = sc64_race_unit_from_raw(SC64_TRANSPORT_SHUTTLE_RAW);
    assert(shuttle >= 0);
    check_neutral_cargo(shuttle, 4);
    check_neutral_ventral();
    check_ventral_seed_scope();
    check_human_retirement();
#endif
#endif
    puts("Blood Bath Melee/FFA elimination, surviving CPUs and SRAM: pass");
    return 0;
}
