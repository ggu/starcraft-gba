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

#ifdef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
static int carrier_children(int carrier, unsigned state)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        n += u[i].alive && u[i].type == U_P7_INTERCEPTOR && u[i].build == carrier &&
             sc64_p7_carrier_child_state(&u[i]) == state;
    return n;
}

static void test_carrier_ai_owners(void)
{
    custom_game();
    race_bots = 3;
    for (int owner = 1; owner < 5; ++owner) {
        if (owner == SIDE) continue;
        int carrier = spawn_unit(owner, U_P7_CARRIER, 600 + owner * 8, 600);
        assert(carrier >= 0);
        race_minerals[owner] = 25;
        assert(!race_p7_carrier_training_queue_owner(carrier, SIDE));
        assert(race_p7_carrier_training_queue_owner(carrier, owner));
        assert(race_minerals[owner] == 0 && u[carrier].carry == 1);
        assert(!race_p7_carrier_training_queue_owner(carrier, owner));
        race_p7_carrier_training_source_tick();
        assert(carrier_children(carrier, SC64_P7_INTERCEPTOR_TRAINING) == 1);
    }
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
    int watcher = spawn_unit(SIDE, U_DRAGOON, 690, 700);
    assert(observer >= 0 && watcher >= 0);
    run(40);
    assert(sc64_p7_observer_target_is_cloaked(observer));
    assert(!sc64_p7_observer_target_revealed_to(observer, SC64_PROTOSS_HUMAN_SLOT, SIDE));
    (void)spawn_complete(SIDE, B_PYLON, 640, 720);
    (void)spawn_complete(SIDE, B_PHOTON_CANNON, 660, 700);
    run(40);
    assert(sc64_p7_observer_target_revealed_to(observer, SC64_PROTOSS_HUMAN_SLOT, SIDE));
    /* The asker's identity follows its live owner, whatever slot it names. */
    assert(sc64_p7_observer_target_revealed_to(observer, -1, SIDE));
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
    test_carrier_ai_owners();
    test_carrier_training_and_flight();
#else
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
