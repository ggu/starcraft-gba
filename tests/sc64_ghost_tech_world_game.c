/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

static int facility(int owner, int x, int y)
{
    int slot = spawn_build(owner, sc64_race_building_from_raw(116), x, y, 0);
    assert(slot >= 0);
    return slot;
}

static int covert(int owner, int x, int y)
{
    int slot = spawn_build(owner, sc64_race_building_from_raw(117), x + 48, y + 8, 0);
    assert(slot >= 0 && race_ghost_covert_parent(slot, -1) >= 0);
    return slot;
}

static int ghost(int owner, int x, int y)
{
    int slot = spawn_unit(owner, sc64_race_unit_from_raw(1), x, y);
    assert(slot >= 0);
    return slot;
}

static void choose(int slot, int kind, const char *label)
{
    selected_building = slot;
    assert(!menu_open);
    game_update(KEY_B, KEY_B);
    assert(menu_open);
    /* Custom Terran uses the scroll list. Drive the actual D-pad and A,
     * checking what the field HUD presents at the highlighted row. */
    for (int i = 0; race_ghost_tech_menu_kind(menu_cursor) != kind; ++i) {
        assert(i < field_menu_count());
        game_update(KEY_DOWN, KEY_DOWN);
    }
    assert(strcmp(race_menu_label(menu_cursor), label) == 0);
    int ore, vespene;
    const char *need;
    assert(ui_row_state(menu_cursor, &ore, &vespene, &need) == UI_SITE_OK);
    if (kind == SC64_GHOST_MENU_CANCEL) assert(!ore && !vespene);
    else assert(ore == sc64_ghost_tech_fact((unsigned)kind)->minerals && ore == vespene);
    game_update(KEY_A, KEY_A);
    assert(!menu_open);
}

static void source_ticks(unsigned count)
{
    while (count) {
        ++frame;
        if (sc64_battlecruiser_source_tick_due()) --count;
        race_ghost_tech_tick();
    }
}

static void read_energy(int slot, int current, int capacity)
{
    int max = -1;
    assert(sc64_unit_energy(&u[slot], &max) == current && max == capacity);
}

static void custom_world(void)
{
    gba_init(); game_init();
    race_mode = 1; race_human = R_TER; race_bots = 1;
    race_bot_races[0] = R_TER; race_skirmish_map = 0;
    init_state();
    assert(screen == SCREEN_PLAY && race_player[ENEMY] == R_TER);
    static u8 base_save[sizeof host_save];

    int parent = facility(SIDE, 312, 296), first = covert(SIDE, 312, 296);
    facility(SIDE, 312, 568);
    int second = covert(SIDE, 312, 568);
    facility(ENEMY, 792, 792);
    int rival = covert(ENEMY, 792, 792);
    int human_ghost = ghost(SIDE, 408, 408), enemy_ghost = ghost(ENEMY, 808, 648);
    read_energy(human_ghost, 50 * 256, 200 * 256);
    read_energy(enemy_ghost, 50 * 256, 200 * 256);
    int command = spawn_build(SIDE, sc64_race_building_from_raw(106), 624, 720, 0);
    int silo = spawn_build(SIDE, sc64_race_building_from_raw(108), 672, 728, 0);
    int depot = spawn_build(SIDE, sc64_race_building_from_raw(109), 576, 720, 0);
    assert(command >= 0 && silo >= 0 && depot >= 0);
    minerals = gas = 1000;
    assert(race_nuke_arm(silo, SIDE) == 4);
    while (race_nuke_silos[silo].ammo.remaining) { ++frame; race_nuke_tick(); }
    assert(race_nuke_armed_slot(SIDE) == silo && race_save_game());
    memcpy(base_save, host_save, sizeof base_save);
    assert(race_nuke_issue(human_ghost, SIDE, 420, 408));
    assert(!race_save_game() && memcmp(base_save, host_save, sizeof base_save) == 0);
    assert(race_load_game() && race_nuke_armed_slot(SIDE) == silo);
    read_energy(human_ghost, 50 * 256, 200 * 256);
    update_exploration();
    assert(!currently_visible(568, 408));
    assert(race_nuke_ghost_in_range(human_ghost, 552, 408));
    assert(!race_nuke_ghost_in_range(human_ghost, 553, 408));

    minerals = gas = 1000;
    race_minerals[ENEMY] = race_gas[ENEMY] = 1000;
    selected_building = first;
    assert(race_ghost_tech_menu_width() == 2);
    choose(first, SC64_GHOST_TECH_OCULAR, "OCULAR IMPLANTS");
    assert(minerals == 900 && gas == 900 && race_ghost_job_at(first)->paid.kind == SC64_GHOST_TECH_OCULAR);
    assert(!race_save_game() && memcmp(base_save, host_save, sizeof base_save) == 0);
    assert(race_ghost_tech_start(second, SC64_GHOST_TECH_OCULAR) == 8);
    assert(race_ghost_tech_start(rival, SC64_GHOST_TECH_REACTOR) == 0);
    assert(race_minerals[ENEMY] == 850 && race_gas[ENEMY] == 850);
    choose(second, SC64_GHOST_TECH_REACTOR, "MOEBIUS REACTOR");
    assert(minerals == 750 && gas == 750);
    choose(second, SC64_GHOST_MENU_CANCEL, "CANCEL RESEARCH");
    assert(minerals == 900 && gas == 900);

    unsigned before = race_ghost_job_at(first)->paid.remaining;
    assert(before && race_bc_facility_lift(parent));
    assert(b[first].owner == NEUTRAL && race_ghost_covert_parent(first, -1) < 0);
    source_ticks(25);
    assert(race_ghost_job_at(first)->paid.remaining == before);
    assert(race_bc_facility_land(parent, b[parent].x, b[parent].y));
    ++frame; race_bc_facility_flight_tick();
    assert(b[first].owner == SIDE && race_ghost_covert_parent(first, -1) == parent);
    source_ticks(before - 1);
    assert(race_ghost_job_at(first)->paid.remaining == 1);
    assert(!race_nuke_ghost_in_range(human_ghost, 568, 408));
    source_ticks(1);
    assert(!race_ghost_job_at(first));
    assert(race_nuke_ghost_in_range(human_ghost, 584, 408));
    assert(!race_nuke_ghost_in_range(human_ghost, 585, 408));
    assert(!race_nuke_ghost_in_range(enemy_ghost, 968, 648));
    update_exploration();
    assert(currently_visible(568, 408)); /* Same stationary Ghost. */
    assert(!race_save_game() && memcmp(base_save, host_save, sizeof base_save) == 0);

    choose(first, SC64_GHOST_TECH_REACTOR, "MOEBIUS REACTOR");
    before = race_ghost_job_at(first)->paid.remaining;
    source_ticks(before - 1);
    int max;
    (void)sc64_unit_energy(&u[human_ghost], &max);
    assert(max == 200 * 256);
    source_ticks(1);
    (void)sc64_unit_energy(&u[human_ghost], &max);
    assert(max == 250 * 256);
    int fresh = ghost(SIDE, 440, 408);
    read_energy(fresh, 250 * 256 / 4, 250 * 256);
    u[human_ghost].carry = 200 * 256;
    do { ++frame; } while (!sc64_battlecruiser_source_tick_due());
    update_orders();
    read_energy(human_ghost, 200 * 256 + 8, 250 * 256);

    assert(race_ghost_tech_start(rival, SC64_GHOST_TECH_OCULAR) == 0);
    assert(race_minerals[ENEMY] == 750 && race_gas[ENEMY] == 750);
    b[rival].owner = SIDE; /* Foreign control cannot use or cancel a paid receipt. */
    selected_building = rival;
    assert(!race_ghost_tech_menu_width() && !race_ghost_tech_cancel(rival, 0));
    destroy_building(rival, SIDE);
    assert(!race_ghost_job_at(rival) && race_minerals[ENEMY] == 825 && race_gas[ENEMY] == 825);
    assert(minerals == 750 && gas == 750);
    assert(race_load_game());
    assert(race_ghost_tech_save_safe() && !sc64_ghost_tech.completed && !sc64_ghost_tech.pending);
    assert(race_nuke_armed_slot(SIDE) == silo);
}

#ifdef SC64_TERRAN_LATE_RUNTIME
static void campaign_source_rows(void)
{
    race_mode = 0; race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T8;
    init_state();
    unsigned found = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].raw_type == 1 && u[i].owner == SIDE) {
        read_energy(i, 200 * 256, 200 * 256);
        ++found;
    }
    assert(found == 2 && race_ghost_tech_policy(0) == SC64_GHOST_LIVE_UPGRADES);
    race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T10;
    init_state();
    assert(race_ghost_tech_save_safe());
    assert(sc64_ghost_tech_has(&sc64_ghost_tech, 0, SC64_GHOST_TECH_OCULAR));
    assert(sc64_ghost_tech_has(&sc64_ghost_tech, 6, SC64_GHOST_TECH_REACTOR));
    assert(!sc64_ghost_tech_has(&sc64_ghost_tech, 1, SC64_GHOST_TECH_REACTOR));
    found = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].raw_type == 1) {
        read_energy(i, 250 * 256, 250 * 256);
        ++found;
    }
    assert(found == 9);
}
#endif

int main(void)
{
    custom_world();
#ifdef SC64_TERRAN_LATE_RUNTIME
    campaign_source_rows();
#endif
    puts("Ghost upgrades: D-pad menus, receipts, two owners, detach/resume, refunds, range, fog, energy and SRAM guard");
    return 0;
}
