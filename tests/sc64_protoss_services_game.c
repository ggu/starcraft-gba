/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual-engine checks for the shared Protoss unit services in the modes the
 * generic binding serves (custom games and Showcase scenes), and for their
 * absence in campaign missions without a binding. Feature flags come from the
 * production compile vector; this fixture enables nothing locally.
 */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#if !defined(SC64_P7_BATTERY_ENABLED) || !defined(SC64_P7_OBSERVER_ENABLED) || \
    !defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED) || \
    !defined(SC64_P7_GEOMETRY_RECT_HOIST_ENABLED)
#error "Compile with a product vector that carries the shared Protoss services"
#endif

static void custom_game(void)
{
    game_init();
    race_mode = 1; race_human = R_PROTOSS; race_bots = 1;
    race_skirmish_map = 0;
    race_skirmish_init();
    screen = SCREEN_PLAY;
}

static void run(int frames)
{
    for (int i = 0; i < frames && screen == SCREEN_PLAY; ++i) game_update(0, 0);
}

static int spawn_complete(int owner, int type, int x, int y)
{
    int slot = spawn_build(owner, type, x, y, 0);
    assert(slot >= 0 && building_complete(slot));
    return slot;
}

static void test_bindings(void)
{
    custom_game();
    assert(race_protoss_service_binding() == SC64_PROTOSS_BIND_GENERIC);
    assert(race_protoss_source_owner(SC64_PROTOSS_HUMAN_SLOT) == SIDE);
    assert(race_protoss_source_owner(ENEMY) == ENEMY);
    assert(race_protoss_source_owner(0) == -1);
    race_mode = 0; race_selected_mission = 0;
    assert(race_protoss_service_binding() == SC64_PROTOSS_BIND_NONE);
    race_selected_mission = SC64_P7_GLOBAL_ID;
    assert(race_protoss_service_binding() == SC64_PROTOSS_BIND_P7);
    race_protoss_bind_candidate(1, SC64_P7_GLOBAL_ID);
    assert(race_protoss_service_binding() == SC64_PROTOSS_BIND_GENERIC);
    race_protoss_unbind_candidate();
    assert(race_protoss_service_binding() == SC64_PROTOSS_BIND_P7);
}

static void test_unbound_campaign_runs_no_service(void)
{
    custom_game();
    race_mode = 0; race_selected_mission = 0;
    int carrier = spawn_unit(SIDE, U_P7_CARRIER, 600, 600);
    assert(carrier >= 0);
    assert(u[carrier].weapon != (u16)(1u << 13));
    u[carrier].selected = 1;
#ifdef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
    assert(!race_p7_carrier_training_selected());
#endif
}

#ifndef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
static void test_incomplete_carrier_service_cannot_train(void)
{
    custom_game();
    minerals = gas = 5000;
    (void)spawn_complete(SIDE, B_PYLON, 600, 600);
    int gate = spawn_complete(SIDE, B_STARGATE, 610, 620);
    int beacon = sc64_race_building_from_raw(169);
    int carrier = sc64_race_unit_from_raw(72);
    assert(carrier >= 0 && beacon >= 0);
    (void)spawn_complete(SIDE, beacon, 620, 640);
    assert(race_skirmish_product_producer(carrier) == 255);
    assert(!race_queue_unit(SIDE, gate, carrier));
    assert(minerals == 5000 && gas == 5000 && !b[gate].queue_count);
}
#endif

#ifdef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
static int carrier_children(int carrier, unsigned state)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        n += u[i].alive && u[i].type == U_P7_INTERCEPTOR && u[i].build == carrier &&
             sc64_p7_carrier_child_state(&u[i]) == state;
    return n;
}

static void test_paid_carrier_and_interceptor_commands(void)
{
    custom_game();
    minerals = gas = 5000;
    (void)spawn_complete(SIDE, B_PYLON, 600, 600);
    int gate = spawn_complete(SIDE, B_STARGATE, 610, 620);
    int beacon_type = sc64_race_building_from_raw(169);
    assert(beacon_type >= 0);
    const Sc64SkirmishRecord *carrier_row = 0;
    for (unsigned i = 0; i < sc64_skirmish_product_count(167); ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_product_at(167, i);
        if (r->raw_id == 72) carrier_row = r;
    }
    assert(carrier_row);
    assert(!race_skirmish_product_admitted(SIDE, gate, carrier_row));
    (void)spawn_complete(SIDE, beacon_type, 620, 640);
    assert(race_skirmish_product_admitted(SIDE, gate, carrier_row));
    assert(!race_skirmish_product_admitted(ENEMY, gate, carrier_row));

    selected_building = gate; cx = b[gate].x; cy = b[gate].y;
    train_type = U_P7_CARRIER;
    int old_minerals = minerals, old_gas = gas;
    interact();
    assert(b[gate].queue_count == 1);
    assert(minerals == old_minerals - 350 && gas == old_gas - 250);
    selected_building = -1;
    for (int tick = 0; tick < unit_build_time(U_P7_CARRIER) + 8 &&
         b[gate].queue_count; ++tick) race_tick_buildings();
    assert(!b[gate].queue_count);
    int carrier = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_P7_CARRIER)
            carrier = i;
    assert(carrier >= 0 && !carrier_children(carrier, SC64_P7_INTERCEPTOR_INSIDE));

    u[carrier].selected = 1;
    int command = (int)sc64_skirmish_menu_count((unsigned)race_human);
    assert(field_menu_count() == command + 1);
    assert(!strcmp(race_menu_label(command), "BUILD INTERCEPTOR"));
    assert(!strcmp(race_menu_label(MENU_STOP + 1), "Nexus"));
    old_minerals = minerals; old_gas = gas;
    game_update(KEY_B, KEY_B);
    assert(menu_open);
    for (int guard = 0; menu_cursor != command && guard < field_menu_count(); ++guard)
        game_update(KEY_DOWN, KEY_DOWN);
    assert(menu_cursor == command);
    game_update(KEY_A, KEY_A);
    assert(!menu_open);
    assert(minerals == old_minerals - 25 && gas == old_gas);
    assert(u[carrier].carry == 1);
    u[carrier].selected = 0;
    run(SC64_P7_INTERCEPTOR_BUILD_TICKS * 5 / 4 + 40);
    assert(carrier_children(carrier, SC64_P7_INTERCEPTOR_INSIDE) == 1);
    assert(race_p7_carrier_training_live_world_valid());
    assert(race_save_game());
    assert(race_load_game());
    assert(carrier_children(carrier, SC64_P7_INTERCEPTOR_INSIDE) == 1);
    assert(race_p7_carrier_training_live_world_valid());
}

static void test_mixed_context_commands(void)
{
    custom_game();
    int carrier = spawn_unit(SIDE, U_P7_CARRIER, 600, 600);
    assert(carrier >= 0);
    u[carrier].selected = 1;
    int base = (int)sc64_skirmish_menu_count((unsigned)race_human);
    int transport = 0;
#ifdef SC64_TRANSPORT_RUNTIME
    if (sc64_race_unit_from_raw(11) == U_DROPSHIP) {
        int ship = spawn_unit(SIDE, U_DROPSHIP, 640, 600);
        int passenger = spawn_unit(SIDE, U_MARINE, 640, 600);
        assert(ship >= 0 && passenger >= 0);
        assert(race_transport_request_board(passenger, ship));
        assert(race_transport_tick_unit(passenger));
        assert(SC64_TRANSPORT_LOADED(&u[passenger]));
        u[ship].selected = 1;
        transport = 1;
        assert(field_menu_count() == base + 2);
        assert(!strcmp(race_menu_label(base), "UNLOAD CARGO"));
        assert(!strcmp(race_menu_label(base + 1), "BUILD INTERCEPTOR"));
        int funds = minerals;
        choose_command(base);
        assert(mode == MODE_UNLOAD && minerals == funds && !u[carrier].carry);
        mode = MODE_NORMAL;
    } else puts("Mixed Dropship branch unavailable in actual product raw map");
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    int cruiser = spawn_unit(SIDE, sc64_race_unit_from_raw(12), 680, 600);
    assert(cruiser >= 0);
    u[cruiser].selected = 1;
    sc64_battlecruiser_tech_bits = (u16)(SC64_BC_TECH_YAMATO << (2 * SIDE));
    assert(race_bc_yamato_command_available());
    assert(!strcmp(race_menu_label(base), "YAMATO GUN"));
    assert(field_menu_count() == base + 2 + transport);
    if (transport) assert(!strcmp(race_menu_label(base + 1), "UNLOAD CARGO"));
    assert(!strcmp(race_menu_label(base + 1 + transport), "BUILD INTERCEPTOR"));
    choose_command(base);
    assert(mode == MODE_YAMATO && !u[carrier].carry);
    mode = MODE_NORMAL;
#endif
    int row = race_carrier_command_base();
    minerals = 100;
    choose_command(row);
    assert(minerals == 75 && u[carrier].carry == 1 && mode == MODE_NORMAL);
    /* Building selection takes precedence over a stale unit selection. */
    selected_building = spawn_complete(SIDE, B_NEXUS, 400, 600);
    assert(!race_p7_carrier_training_menu_available());
    /* Custom exceptions must never enable scenario-only production. */
    race_mode = 0;
    assert(race_skirmish_product_producer(U_P7_CARRIER) == 255);
}

static void test_carrier_training_and_flight(void)
{
    custom_game();
    int carrier = spawn_unit(SIDE, U_P7_CARRIER, 600, 600);
    assert(carrier >= 0);
    assert(SC64_P7_ARBITER_CACHE_VALID(u[carrier].weapon));
    u[carrier].selected = 1;
    assert(race_p7_carrier_training_selected());
    minerals = SC64_P7_INTERCEPTOR_COST;
    assert(race_p7_carrier_training_queue_selected() == 1 && minerals == 0);
    u[carrier].selected = 0;
    run(SC64_P7_INTERCEPTOR_BUILD_TICKS * 5 / 4 + 40);
    assert(carrier_children(carrier, SC64_P7_INTERCEPTOR_INSIDE) == 1);
    assert(race_p7_carrier_training_stock(carrier, 8) ==
           (int)SC64_P7_CARRIER_CAPACITY - 1);
    assert(carrier_children(carrier, SC64_P7_INTERCEPTOR_INSIDE) ==
           (int)SC64_P7_CARRIER_CAPACITY);
    assert(race_p7_carrier_training_live_world_valid());

    int prey = spawn_unit(ENEMY, U_ZERGLING, 660, 600);
    assert(prey >= 0);
    int hp = u[prey].hp, launched = 0;
    for (int i = 0; i < 400 && u[prey].alive; ++i) {
        game_update(0, 0);
        int out = carrier_children(carrier, SC64_P7_INTERCEPTOR_OUTSIDE);
        if (out > launched) launched = out;
    }
    assert(launched > 0);
    assert(!u[prey].alive || u[prey].hp < hp);
}
#endif

static void test_cannon_detects_observer(void)
{
    custom_game();
    int observer = spawn_unit(ENEMY, U_P7_OBSERVER, 700, 700);
    int second = spawn_unit(ENEMY, U_P7_OBSERVER, 704, 700);
    int watcher = spawn_unit(SIDE, U_DRAGOON, 690, 700);
    assert(observer >= 0 && second >= 0 && watcher >= 0);
    /* Isolate detector power cycling from attacks on revealed targets. */
    invincible_units[observer] = invincible_units[second] = 1;
    int observer_hp = u[observer].hp, observer_shield = u[observer].shield;
    int second_hp = u[second].hp, second_shield = u[second].shield;
    run(40);
    assert(u[observer].alive && u[second].alive);
    assert(u[observer].hp == observer_hp && u[observer].shield == observer_shield);
    assert(u[second].hp == second_hp && u[second].shield == second_shield);
    assert(sc64_p7_observer_target_is_cloaked(observer));
    assert(!sc64_p7_observer_target_revealed_to(observer, SC64_PROTOSS_HUMAN_SLOT, SIDE));
    int pylon=spawn_complete(SIDE, B_PYLON, 640, 720);
    (void)spawn_complete(SIDE, B_PHOTON_CANNON, 660, 700);
    run(40);
    assert(u[observer].alive && u[second].alive);
    assert(u[observer].hp == observer_hp && u[observer].shield == observer_shield);
    assert(u[second].hp == second_hp && u[second].shield == second_shield);
    assert(sc64_p7_observer_target_revealed_to(observer, SC64_PROTOSS_HUMAN_SLOT, SIDE));
    /* The asker's identity follows its live owner, whatever slot it names. */
    assert(sc64_p7_observer_target_revealed_to(observer, -1, SIDE));
    assert(sc64_p7_observer_target_revealed_to(second, SC64_PROTOSS_HUMAN_SLOT, SIDE));
    b[pylon].alive=0;
    run(40);
    assert(u[observer].alive && u[second].alive);
    assert(u[observer].hp == observer_hp && u[observer].shield == observer_shield);
    assert(u[second].hp == second_hp && u[second].shield == second_shield);
    assert(!sc64_p7_observer_target_revealed_to(observer, SC64_PROTOSS_HUMAN_SLOT, SIDE));
    assert(!sc64_p7_observer_target_revealed_to(second, SC64_PROTOSS_HUMAN_SLOT, SIDE));
    (void)spawn_complete(SIDE, B_PYLON, 640, 720);
    run(40);
    assert(u[observer].alive && u[second].alive);
    assert(u[observer].hp == observer_hp && u[observer].shield == observer_shield);
    assert(u[second].hp == second_hp && u[second].shield == second_shield);
    assert(sc64_p7_observer_target_revealed_to(observer, SC64_PROTOSS_HUMAN_SLOT, SIDE));
    assert(sc64_p7_observer_target_revealed_to(second, SC64_PROTOSS_HUMAN_SLOT, SIDE));
}

static void test_overlord_detects_zeratul(void)
{
    custom_game();
    int zeratul_type = sc64_race_unit_from_raw(75);
    assert(zeratul_type >= 0);
    int zeratul = spawn_unit(SIDE, zeratul_type, 800, 300);
    int overlord = spawn_unit(ENEMY, U_OVERLORD, 800, 700);
    assert(zeratul >= 0 && overlord >= 0);
    run(40);
    assert(sc64_p7_observer_target_is_cloaked(zeratul));
    assert(!sc64_p7_observer_target_revealed_to(zeratul, ENEMY, ENEMY));
    /* Detection also needs the detector's own sight of the cell. */
    u[overlord].x = u[overlord].tx = 800; u[overlord].y = u[overlord].ty = 340;
    run(40);
    assert(sc64_p7_observer_target_revealed_to(zeratul, ENEMY, ENEMY));
}

static void test_arbiter_cloaks_nearby_units(void)
{
    custom_game();
    int arbiter = spawn_unit(SIDE, U_P7_ARBITER, 500, 400);
    int zealot = spawn_unit(SIDE, U_ZEALOT, 520, 400);
    assert(arbiter >= 0 && zealot >= 0);
    assert(!sc64_p7_observer_target_is_cloaked(zealot));
    run(40);
    assert(u[zealot].weapon & SC64_P7_ARBITER_AURA);
    assert(sc64_p7_observer_target_is_cloaked(zealot));
    assert(!sc64_p7_observer_target_is_cloaked(arbiter));
}

static RaceSave saved_candidate;

/* Use the active product envelope so candidate checks exercise semantic
 * validation rather than failing earlier on a mismatched record layout. */
static void write_test_candidate(void)
{
#ifdef SC64_IMPACT_EVENTS
    race_data = saved_candidate;
    assert(race_impact_save_write());
#else
    save_write(&saved_candidate, sizeof(saved_candidate), SAVE_VERSION);
#endif
}

static void expect_candidate_rejected(void)
{
    Unit before[MAX_UNITS];
    memcpy(before, u, sizeof(u));
    int old_minerals = minerals, old_frame = frame;
    write_test_candidate();
    assert(!race_load_game());
    assert(memcmp(before, u, sizeof(u)) == 0);
    assert(minerals == old_minerals && frame == old_frame);
    assert(race_protoss_binding_override < 0);
}

static void test_battery_service_and_save(void)
{
    custom_game();
    (void)spawn_complete(SIDE, B_PYLON, 300, 640);
    int battery = spawn_complete(SIDE, B_P7_SHIELD_BATTERY, 330, 640);
    int zealot = spawn_unit(SIDE, U_ZEALOT, 360, 640);
    assert(zealot >= 0 && b[battery].queue_progress == 100 * 256);
    assert(race_p7_battery_live_world_valid());
    /* Every owner can site a Battery by its own Pylon in a custom game. */
    race_player[ENEMY] = R_PROTOSS;
    (void)spawn_complete(ENEMY, B_GATEWAY, 900, 220);
    (void)spawn_complete(ENEMY, B_PYLON, 900, 300);
    assert(race_can_site(ENEMY, B_P7_SHIELD_BATTERY, 930, 300));
    assert(!race_can_site(ENEMY, B_P7_SHIELD_BATTERY, 930, 600));

    u[zealot].shield = 10;
    selected_building = battery;
    assert(race_p7_battery_command_available());
    assert(race_p7_battery_issue_command(battery, u[zealot].x, u[zealot].y) == 1);
    selected_building = -1;
    run(90);
    assert(u[zealot].shield > 10);
    int energy = b[battery].queue_progress, shield = u[zealot].shield;
    assert(energy < 100 * 256);

    run(30);
    assert(race_save_game());
    int word = race_data.state[SC64_PROTOSS_CLOCK_STATE_INDEX];
    assert(((unsigned)word & 0xffff0000u) == SC64_PROTOSS_CLOCK_STATE_TAG);
    saved_candidate = race_data;
    u8 clock[3];
    memcpy(clock, race_protoss_generic_clock, sizeof(clock));
    energy = b[battery].queue_progress; shield = u[zealot].shield;
    b[battery].queue_progress = 0; u[zealot].shield = 0;
    memset(race_protoss_generic_clock, 0, sizeof(race_protoss_generic_clock));
    assert(race_load_game());
    assert(race_protoss_binding_override < 0);
    assert(b[battery].queue_progress == energy && u[zealot].shield == shield);
    assert(memcmp(clock, race_protoss_generic_clock, sizeof(clock)) == 0);

    /* An unknown clock word is rejected before the world changes. */
    saved_candidate.state[SC64_PROTOSS_CLOCK_STATE_INDEX] = 0x12345;
    expect_candidate_rejected();
    /* Custom games saved before the shared services load with an empty
       clock and empty cache words. */
    saved_candidate.state[SC64_PROTOSS_CLOCK_STATE_INDEX] = 0;
    write_test_candidate();
    assert(race_load_game());
    assert(!race_protoss_generic_clock[0] && !race_protoss_generic_clock[1] &&
           !race_protoss_generic_clock[2]);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive) assert(SC64_P7_ARBITER_CACHE_VALID(u[i].weapon));
}

#if defined(SC64_SHOWCASE_ENABLED) && defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED)
static void test_showcase_carriers_fight(void)
{
    game_init();
    sc64_showcase_start(0);
    assert(race_protoss_service_binding() == SC64_PROTOSS_BIND_GENERIC);
    int carriers = 0, stocked = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].type == U_P7_CARRIER) {
        ++carriers;
        stocked += carrier_children(i, SC64_P7_INTERCEPTOR_INSIDE) == 4;
    }
    assert(carriers == 3 && stocked == 3);
    screen = SCREEN_PLAY;
    int launched = 0;
    for (int f = 0; f < 600 && screen == SCREEN_PLAY; ++f) {
        game_update(0, 0);
        int out = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            out += u[i].alive && u[i].type == U_P7_INTERCEPTOR &&
                   sc64_p7_carrier_child_state(&u[i]) == SC64_P7_INTERCEPTOR_OUTSIDE;
        if (out > launched) launched = out;
    }
    assert(launched > 0);
}
#endif

int main(void)
{
    test_bindings();
    test_unbound_campaign_runs_no_service();
#ifdef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
    test_paid_carrier_and_interceptor_commands();
    test_mixed_context_commands();
    test_carrier_training_and_flight();
#else
    test_incomplete_carrier_service_cannot_train();
    /* Do not inject flight flags into a production-vector check. This is
     * explicit missing capability, not a passing Carrier implementation. */
    puts("Carrier training/flight absent from this product; not qualified here");
#endif
    test_cannon_detects_observer();
    test_overlord_detects_zeratul();
    test_arbiter_cloaks_nearby_units();
    test_battery_service_and_save();
#if defined(SC64_SHOWCASE_ENABLED) && defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED)
    test_showcase_carriers_fight();
#endif
    printf("sc64_protoss_services: ok\n");
    return 0;
}
