/* Focused Z/P air-tech checks through the real engine command and production
 * paths. The world and resources are synthetic; this is not campaign proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_Z4_RUNTIME
#error "Compile with the expanded Z4 profile flags"
#endif

static u16 prior_keys;

static void input(u16 held)
{
    game_update(held, held & (u16)~prior_keys);
    prior_keys = held;
}

static void press(u16 key)
{
    input(key);
    input(0);
}

/* Field controls reopen the order list on the last command. These
 * sequences were written for a list that opens on Move, so a fresh
 * open starts there (no extra input, so the run's timing is unchanged). */
static void press_b(void)
{
    int fresh = !menu_open && mode == MODE_NORMAL;
    press(KEY_B);
    if (fresh && menu_open) menu_cursor = MENU_MOVE;
}

static void launch_z4(void)
{
    game_init();
    prior_keys = 0;
    press(KEY_A);                 /* faction menu */
    assert(race_front.page == SC_FRONT_MISSION);
    assert(race_front.faction == SC_FRONT_ZERG);
    press(KEY_RIGHT);
    press(KEY_DOWN);
    assert(sc64_catalog_playable_at(SC64_FACTION_ZERG, 3) == 15);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING);
    input(KEY_START);
    assert(screen == SCREEN_PLAY && race_selected_mission == 15);
    assert(race_human == R_ZERG && race_z4_active());
}

static void reset_synthetic_world(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(m, 0, sizeof(m));
    sc64_visibility_reset();
    invalidate_path_grid();
    selected_building = -1;
    menu_open = 0;
    menu_cursor = 0;
    mode = MODE_NORMAL;
    minerals = 5000;
    gas = 5000;
    supply_used = 0;
    supply_cap = 400;
    prior_keys = 0;
}

static int new_building(int type, int x, int y)
{
    int slot = spawn_build(SIDE, type, x, y, 0);
    assert(slot >= 0);
    b[slot].raw_owner = 0;
    return slot;
}

static int new_worker(int x, int y)
{
    int slot = spawn_unit(SIDE, U_DRONE, x, y);
    assert(slot >= 0);
    u[slot].raw_type = sc64_race_raw_unit(U_DRONE);
    u[slot].raw_owner = 0;
    u[slot].selected = 1;
    return slot;
}

static void select_command(int choice)
{
    int steps;
    press_b();
    for (steps = 0; steps < choice; ++steps) press(KEY_DOWN);
    assert(menu_open && menu_cursor == choice);
    input(KEY_A);
}

static void test_lair_command_and_mid_morph_save(void)
{
    int hatch, pool, lair, drone;
    int old_minerals, old_gas;
    s16 old_x, old_y, old_raw_type, old_raw_owner;
    u8 old_larva;
    launch_z4();
    reset_synthetic_world();
    hatch = new_building(B_HATCHERY, 420, 420);
    b[hatch].raw_type = sc64_race_raw_building(B_HATCHERY);
    drone = new_worker(430, 430);
    (void)drone;
    b[hatch].larva = 2;
    b[hatch].larva_timer = 117;
    old_x = b[hatch].x; old_y = b[hatch].y;
    old_raw_type = b[hatch].raw_type; old_raw_owner = b[hatch].raw_owner;
    old_larva = b[hatch].larva;
    old_minerals = minerals; old_gas = gas;
    assert(!race_start_lair_morph(hatch)); /* Spawning Pool is required. */
    b[hatch].under_construction = 1;
    assert(!race_start_lair_morph(hatch)); /* Hatchery must be complete. */
    b[hatch].under_construction = 0;
    pool = new_building(B_SPAWNING_POOL, 460, 420);

    /* The Zerg command list's final two rows scroll into the visible window. */
    selected_building = hatch;
    select_command(MENU_ZP_LAIR);
    assert(!menu_open && b[hatch].type == B_LAIR && b[hatch].under_construction);
    assert(b[hatch].x == old_x && b[hatch].y == old_y);
    assert(b[hatch].raw_owner == old_raw_owner &&
           b[hatch].raw_type == sc64_race_raw_building(B_LAIR));
    assert(old_raw_type == sc64_race_raw_building(B_HATCHERY));
    assert(b[hatch].larva == old_larva && b[hatch].larva_timer == 117);
    assert(b[hatch].build_progress == 0);
    assert(minerals == old_minerals - 150 && gas == old_gas - 100);

    /* A repeat command, incomplete Pool, queue, and insufficient bank must
     * all leave the building and resources unchanged. */
    old_minerals = minerals; old_gas = gas;
    selected_building = hatch;
    select_command(MENU_ZP_LAIR);
    assert(b[hatch].type == B_LAIR && minerals == old_minerals && gas == old_gas);
    b[pool].under_construction = 1;
    b[pool].build_progress = 0;
    assert(!race_start_lair_morph(hatch));
    b[pool].under_construction = 0;
    b[hatch].queue_count = 1;
    assert(!race_start_lair_morph(hatch));
    b[hatch].queue_count = 0;
    gas = 99;
    assert(!race_start_lair_morph(hatch));
    gas = old_gas;
    minerals = 149;
    assert(!race_start_lair_morph(hatch));
    assert(b[hatch].type == B_LAIR && b[hatch].under_construction);
    minerals = old_minerals;

    /* Save/load preserves the active morph and its bounded clock exactly. */
    while (b[hatch].build_progress < 23) {
        race_tick_buildings();
        race_tick_state();
    }
    assert(b[hatch].under_construction && b[hatch].build_progress == 23);
    assert(b[hatch].larva == old_larva && b[hatch].larva_timer == 117);
    assert(race_save_game());
    assert(race_load_game());
    assert(b[hatch].type == B_LAIR && b[hatch].under_construction);
    assert(b[hatch].build_progress == 23 && b[hatch].larva == old_larva);
    assert(b[hatch].x == old_x && b[hatch].y == old_y &&
           b[hatch].raw_type == sc64_race_raw_building(B_LAIR) &&
           b[hatch].raw_owner == old_raw_owner);
    for (int tick = 23; tick < 1875; ++tick) {
        race_tick_buildings();
        race_tick_state();
    }
    assert(!b[hatch].under_construction && b[hatch].type == B_LAIR);
    assert(b[hatch].build_progress == 0 && b[hatch].larva == 3);
    printf("synthetic Z4 air-tech: Lair morph, source position/owner, construction clock and save/load PASS\n");
}

static void test_spire_and_mutalisk_production(void)
{
    int hatch, pool, drone, spire, overlord;
    int old_minerals, old_gas;
    launch_z4();
    reset_synthetic_world();
    hatch = new_building(B_LAIR, 420, 420);
    pool = new_building(B_SPAWNING_POOL, 460, 420);
    drone = new_worker(430, 430);
    overlord = spawn_unit(SIDE, U_OVERLORD, 440, 430);
    assert(overlord >= 0);
    (void)pool;
    selected_building = hatch;

    /* Spire is the twelfth command entry (index 11), reached by scrolling. */
    select_command(MENU_ZP_SPIRE);
    assert(mode == MODE_BUILD && menu == B_SPIRE && !menu_open);
    assert(sc64_race_building_rules[B_SPIRE].minerals == 200);
    assert(sc64_race_building_rules[B_SPIRE].gas == 150);
    assert(race_can_site(SIDE, B_SPIRE, 500, 420));
    assert(!race_can_site(SIDE, B_SPIRE, 700, 700)); /* Lair creep/site limit. */
    b[hatch].under_construction = 1;
    assert(!race_start_build(SIDE, drone, B_SPIRE, 500, 420));
    b[hatch].under_construction = 0;
    assert(!race_start_build(SIDE, -1, B_SPIRE, 500, 420)); /* Drone required. */

    /* Real build-arrival path consumes the Drone and places a paid Spire. */
    input(0); /* release the command-menu A press before the placement click */
    minerals = 5000; gas = 5000;
    cx = 500; cy = 420;
    u[drone].selected = 1;
    interact();
    spire = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_SPIRE) spire = i;
    assert(spire >= 0 && b[spire].under_construction);
    assert(minerals == 4800 && gas == 4850);
    assert(u[drone].alive && u[drone].order == ORDER_BUILD && u[drone].target == spire);
    u[drone].x = cx; u[drone].y = cy;
    race_builder_arrived(drone);
    assert(!u[drone].alive && b[spire].build_progress == 0);
    b[hatch].larva = 2;
    minerals = 1000; gas = 1000;
    assert(!race_queue_unit(SIDE, hatch, U_MUTALISK)); /* Spire unfinished. */
    assert(b[hatch].larva == 2 && minerals == 1000 && gas == 1000);
    for (int tick = 0; tick < 2250; ++tick) race_tick_buildings();
    assert(building_complete(spire));

    /* Producer cycle has the source fifth Zerg product and real command A
     * spends both resources and exactly one larva. */
    selected_building = hatch;
    train_type = U_DRONE;
    for (int i = 0; i < 4; ++i) press(KEY_R);
    assert(train_type == U_MUTALISK);
    assert(sc64_race_unit_rules[U_MUTALISK].minerals == 100);
    assert(sc64_race_unit_rules[U_MUTALISK].gas == 100);
    assert(sc64_race_unit_rules[U_MUTALISK].prereq & SC_PR_SPIRE);
    b[hatch].larva = 2;
    old_minerals = minerals; old_gas = gas;
    cx = b[hatch].x; cy = b[hatch].y; /* A trains on the selected structure */
    interact();
    assert(b[hatch].queue_count == 1 && b[hatch].queue[0] == U_MUTALISK);
    assert(b[hatch].larva == 1 && minerals == old_minerals - 100 && gas == old_gas - 100);
    for (int tick = 0; tick < 750; ++tick) race_tick_buildings();
    int mutalisk = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_MUTALISK) mutalisk = i;
    assert(mutalisk >= 0 && b[hatch].queue_count == 0 && b[hatch].larva == 1);
    printf("synthetic Z4 air-tech: Spire placement/prerequisites, menu cycle, paid Mutalisk and larva PASS\n");
}

int main(void)
{
    gba_init();
    test_lair_command_and_mid_morph_save();
    test_spire_and_mutalisk_production();
    return 0;
}
