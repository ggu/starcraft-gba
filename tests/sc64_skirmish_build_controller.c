/* SPDX-License-Identifier: GPL-3.0-or-later
 * Normal-input skirmish build-controller regression. The game is launched,
 * selected and commanded exclusively through the controller's button helpers.
 * Run once per race (0 Terran, 1 Zerg, 2 Protoss), optionally with "cancel",
 * "loss", "moving", or, for Terran, "witness".
 * CONTROLLER_SOURCE allows the unchanged baseline controller to be substituted;
 * CONTROLLER_BASELINE restricts that build to the pending-acceptance check.
 */
#ifndef CONTROLLER_SOURCE
#define CONTROLLER_SOURCE "sc64_skirmish_campaign_run.c"
#endif
#define main sc64_skirmish_controller_unused_main
#include CONTROLLER_SOURCE
#undef main
#include <string.h>

static void launch_test(int race)
{
    option_map = 0;
    option_human = race;
    option_bots = 1;
    option_bot_races[0] = (race + 1) % 3;
    match_limit = 10000;
    gba_init();
    assert(launch_match());
    while (host_ticks < 500) input(0);
}

#ifndef CONTROLLER_BASELINE
static void stop_selected_builder(void)
{
    /* The command menu pauses world updates, so the worker cannot arrive
     * while navigating to Stop. */
    tap(KEY_B);
    assert(menu_open);
    for (int guard = 0; menu_cursor != MENU_STOP && guard < 32; ++guard) {
#ifdef SC64_COMMAND_ICONS
        if (card_fits() && card_command_slot(menu_cursor) % CARD_COLUMNS !=
                           card_command_slot(MENU_STOP) % CARD_COLUMNS)
            tap(KEY_RIGHT);
        else
#endif
            tap(KEY_DOWN);
    }
    assert(menu_cursor == MENU_STOP);
    tap(KEY_A);
    assert(!menu_open);
}
#endif

int main(int argc, char **argv)
{
    int race = argc > 1 ? atoi(argv[1]) : R_TER;
    assert(race >= R_TER && race <= R_PROTOSS);
#ifndef CONTROLLER_BASELINE
    if (argc > 2 && !strcmp(argv[2], "loss")) {
        /* Isolated synthetic setup: model the observation after a tracked
         * foundation has died. No live game actor is altered in this case. */
        combat_build.state = BUILD_FOUNDATION;
        combat_build.worker = 0; combat_build.type = B_BARRACKS;
        combat_build.x = 64; combat_build.y = 64; combat_build.building = 0;
        b[0].alive = 0;
        observe_combat_build();
        assert(combat_build.state == BUILD_NONE);
        /* A failed or interrupted order must not be counted as a foundation. */
        combat_build.state = BUILD_PENDING;
        combat_build.worker = 0;
        observe_combat_build();
        assert(combat_build.state == BUILD_NONE);
    }
#endif
    launch_test(race);

#ifndef CONTROLLER_BASELINE
    /* A denied or exhausted attack selection is not an action witness. */
    assert(mode == MODE_NORMAL);
    trace = tmpfile();
    assert(trace);
    trace_first = 1;
    assert(!record_attack_result(cx, cy, 0));
    assert(ftell(trace) == 0);
    fclose(trace);
    trace = NULL;

    if (argc > 2 && !strcmp(argv[2], "moving")) {
        int worker = -1;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && sc64_race_is_worker(u[i].type) &&
                u[i].order == ORDER_MINE) { worker = i; break; }
        assert(worker >= 0);
        move_cursor(320, 320);
        int worker_x = u[worker].x, worker_y = u[worker].y;
        assert(select_unit(worker));
        assert(u[worker].x != worker_x || u[worker].y != worker_y);
        assert(u[worker].selected && selected_count() == 1);
    }
#endif

    /* Six pixels cannot be reached with repeated four-pixel cursor steps. */
    int old_x = cx, old_y = cy;
    unsigned before_move = host_ticks;
    move_cursor(old_x + 6, old_y);
    assert(host_ticks - before_move < 20);
    assert(abs(cx - (old_x + 6)) <= 1 && abs(cy - old_y) <= 1);

#ifndef CONTROLLER_BASELINE
    int minerals_before = minerals, gas_before = gas;
#endif
    assert(build_barracks());
#ifdef CONTROLLER_BASELINE
    return 0;
#else
    assert(combat_build.state == BUILD_PENDING);
    int worker = combat_build.worker;
    int type = combat_build.type;
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(type);
    assert(rule && u[worker].order == ORDER_BUILD_PENDING);
    assert(!build_worker_available(worker, SIDE, u[worker].type));
    assert(minerals >= minerals_before && gas >= gas_before);
    assert(count_buildings(SIDE, type) == 0);

    /* A remembered build command must not turn an attack request into a
     * second build request. Open and cancel the attack placement mode. */
    assert(choose_menu_command(MENU_ATTACK));
    assert(mode == MODE_ATTACK);
    tap(KEY_B); /* Cancel placement, opening the command menu. */
    if (menu_open) tap(KEY_B);
    assert(mode == MODE_NORMAL && combat_build.state == BUILD_PENDING);

    if (argc > 2 && !strcmp(argv[2], "cancel")) {
        stop_selected_builder();
        assert(combat_build.state == BUILD_NONE);
        assert(count_buildings(SIDE, type) == 0);
        assert(u[worker].alive && u[worker].selected && selected_count() == 1);
        unsigned selection_tick = host_ticks;
        assert(select_unit(worker));
        assert(host_ticks == selection_tick);
        assert(build_barracks());
        assert(combat_build.state == BUILD_PENDING || combat_build.state == BUILD_FOUNDATION);
    }

    int saw_foundation_debit = 0;
    while (combat_build.state == BUILD_PENDING && host_ticks < 4000) {
        int before_minerals = minerals, before_gas = gas;
        input(0);
        if (combat_build.state == BUILD_FOUNDATION) {
            /* A worker may deposit on this tick; the net debit can be smaller
             * than the price, but cannot exceed it. */
            int dm = before_minerals - minerals, dg = before_gas - gas;
            assert(dm >= 0 && dm <= rule->minerals);
            assert(dg >= 0 && dg <= rule->gas);
            assert(dm > 0 || dg > 0);
            saw_foundation_debit = 1;
        }
    }
    assert(combat_build.state == BUILD_FOUNDATION);
    assert(saw_foundation_debit || (argc > 2 && !strcmp(argv[2], "cancel")));
    assert(b[combat_build.building].under_construction);
    while (!combat_build.complete && combat_build.state == BUILD_FOUNDATION && host_ticks < 8000)
        input(0);
    assert(combat_build.complete);
    assert(building_complete(combat_build.building));
    if (argc > 2 && !strcmp(argv[2], "witness")) {
        assert(race == R_TER);
        int producer = combat_build.building;
        assert(select_building(producer));
        for (int step = 0; train_type != U_MARINE && step < 5; ++step) tap(KEY_R);
        assert(train_type == U_MARINE);
        int queue_before = b[producer].queue_count;
        tap(KEY_A);
        assert(b[producer].queue_count > queue_before);
        while (!count_units(SIDE, U_MARINE) && host_ticks < 7000) input(0);
        assert(count_units(SIDE, U_MARINE));
        int marine = -1;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_MARINE) { marine = i; break; }
        assert(select_unit(marine));

        /* The artificial tick limit interrupts a real normal-input attack
         * gesture while it is navigating, with no world-field mutation. */
        trace = tmpfile();
        assert(trace);
        trace_first = 1;
        match_limit = host_ticks + 100;
        assert(!attack_order(900, 900));
        assert(host_ticks == match_limit);
        assert(mode == MODE_ATTACK && (abs(cx - 900) > 1 || abs(cy - 900) > 1));
        rewind(trace);
        char line[512];
        while (fgets(line, sizeof(line), trace))
            assert(!strstr(line, "\"action\":\"attack_move\""));
        fclose(trace);
        trace = NULL;
    }
    fprintf(stderr, "controller regression race%d passed tick%u%s\n", race, host_ticks,
            argc > 2 ? argv[2] : "");
    return 0;
#endif
}
