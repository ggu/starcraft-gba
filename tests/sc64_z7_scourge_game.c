/* Actual-engine Z7 Scourge production and combat fixture. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_Z7_SCOURGE_COMBAT
#error "Compile this fixture with SC64_Z7_SCOURGE_COMBAT"
#endif

static int hatchery;

static void press(unsigned key)
{
    game_update((u16)key, (u16)key);
    game_update(0, 0);
}

static void reset_world(void)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(geysers, 0, sizeof(geysers));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    race_mode = 0;
    screen = SCREEN_PLAY;
    mode = MODE_NORMAL;
    menu_open = 0;
    race_human = R_ZERG;
    race_selected_mission = SC64_Z7_GLOBAL_ID;
    race_player[SIDE] = R_ZERG;
    race_player[ENEMY] = R_ZERG;
    race_player[NEUTRAL] = R_ZERG;
    race_player[RESCUABLE] = R_ZERG;
    race_player[4] = R_ZERG;
    race_z7_map = &sc64_race_maps[SC64_Z7_GLOBAL_ID];
    minerals = 5000;
    gas = 5000;
    supply_used = 0;
    supply_cap = 0;
    weapons_level = armor_level = 0;
    target_lists_valid = 0;
    sc64_target_index_active = 0;
    hatchery = spawn_build(SIDE, B_HATCHERY, 160, 160, 0);
    assert(hatchery >= 0 && building_complete(hatchery));
    b[hatchery].larva = 3;
    assert(spawn_unit(SIDE, U_OVERLORD, 96, 96) >= 0);
    assert(race_z7_active());
}

static int find_egg(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_Z7_EGG && u[i].target == U_SCOURGE)
            return i;
    return -1;
}

static int find_type(int owner, int type)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].type == type) return i;
    return -1;
}

static void step_until_scourges(void)
{
    int egg = find_egg();
    assert(egg >= 0);
    int duration = sc64_z7_egg_duration(U_SCOURGE);
    assert(duration == 563 && u[egg].work_timer == duration);
    for (int tick = 0; tick < duration; ++tick) {
        update_orders();
        race_tick_state();
    }
    assert(!u[egg].alive || u[egg].type == U_SCOURGE);
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].owner == SIDE && u[i].type == U_SCOURGE;
    assert(count == 2 && supply_used == 2);
}

static void test_spire_gate_and_playable_selection(void)
{
    /* Keep the real campaign's source context and enemy placements alive
       while exercising normal field inputs. The isolated combat fixtures
       below intentionally do not tick the mission trigger adapter. */
    game_init();
    int ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,
                                                SC64_Z7_GLOBAL_ID);
    assert(ordinal == 6);
    press(KEY_A);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    press(KEY_A);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && race_z7_active());
    hatchery = spawn_build(SIDE, B_HATCHERY, 160, 160, 0);
    assert(hatchery >= 0);
    b[hatchery].larva = 3;
    assert(spawn_unit(SIDE, U_OVERLORD, 96, 96) >= 0);
    minerals = gas = 5000;
    int ore = minerals, gas_before = gas, larva = b[hatchery].larva;
    assert(!race_queue_unit(SIDE, hatchery, U_SCOURGE));
    assert(minerals == ore && gas == gas_before && b[hatchery].larva == larva);

    assert(spawn_build(SIDE, B_SPIRE, 224, 160, 0) >= 0);
    int left, top, right, bottom;
    assert(sc64_z7_world_building_rect(141, 200, 300,
                                      &left, &top, &right, &bottom));
    assert(left == 372 && top == 568 && right == 429 && bottom == 625);
    selected_building = hatchery;
    cx = b[hatchery].x;
    cy = b[hatchery].y;
    train_type = U_DRONE;
    for (int i = 0; i < 5; ++i) press(KEY_R);
    assert(screen == SCREEN_PLAY && !race_z7.runtime.fault &&
           train_type == U_SCOURGE);
    ore = minerals;
    gas_before = gas;
    int used_before = supply_used;
    press(KEY_A);

    int egg = find_egg();
    assert(egg >= 0 && b[hatchery].larva == larva - 1);
    assert(u[egg].raw_type == 36 &&
           u[egg].raw_owner == race_z7_product_source_owner(SIDE));
    assert(u[egg].work_timer == sc64_z7_egg_duration(U_SCOURGE) - 2);
    assert(minerals == ore - 25 && gas == gas_before - 75);
    assert(supply_used == used_before + 2);
    int remaining = u[egg].work_timer;
    assert(race_save_game() && race_load_game());
    assert(train_type == U_SCOURGE && u[egg].alive &&
           u[egg].type == U_Z7_EGG && u[egg].target == U_SCOURGE &&
           u[egg].work_timer == remaining && supply_used == used_before + 2);
    /* The Z7 experiment must not expose a serial, single-Scourge purchase
       through the same ROM's skirmish controls. */
    race_mode = 1;
    selected_building = hatchery;
    train_type = U_DRONE;
    for (int i = 0; i < 12; ++i) {
        select_next_train_type();
        assert(train_type != U_SCOURGE);
    }
    ore = minerals;
    gas_before = gas;
    assert(!race_queue_unit(SIDE, hatchery, U_SCOURGE));
    assert(minerals == ore && gas == gas_before);
}

static int hatch_paid_scourges(void)
{
    reset_world();
    assert(spawn_build(SIDE, B_SPIRE, 224, 160, 0) >= 0);
    assert(race_queue_unit(SIDE, hatchery, U_SCOURGE));
    assert(supply_used == 2);
    step_until_scourges();
    return find_type(SIDE, U_SCOURGE);
}

static void test_air_only_validity_and_guarded_targets(void)
{
    int scourge = hatch_paid_scourges();
    int ground = spawn_unit(ENEMY, U_MARINE, 401, 400);
    int ally = spawn_unit(RESCUABLE, U_Z6_GUARDIAN, 401, 400);
    int hidden = spawn_unit(ENEMY, U_ZP_MAP_REVEALER, 401, 400);
    int immune = spawn_unit(ENEMY, U_Z6_GUARDIAN, 401, 400);
    int valid = spawn_unit(ENEMY, U_Z6_GUARDIAN, 408, 400);
    assert(scourge >= 0 && ground >= 0 && ally >= 0 && hidden >= 0 &&
           immune >= 0 && valid >= 0);
    assert(sc64_race_unit_damage_vs(U_SCOURGE, 0) == 0);
    assert(sc64_race_unit_damage_vs(U_SCOURGE, 1) == 110);
    assert(sc64_race_unit_range_vs(U_SCOURGE, 1) == 3);
    assert(sc64_race_unit_cooldown_vs(U_SCOURGE, 1) == 1);
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, ground, SIDE));
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, ally, SIDE));
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, hidden, SIDE));
    invincible_units[immune] = 1;
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, immune, SIDE));
#ifdef SC64_TRANSPORT_RUNTIME
    u[valid].order = ORDER_TRANSPORTED;
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, valid, SIDE));
    u[valid].order = ORDER_IDLE;
#endif
    assert(SC64_CAN_ATTACK_FROM(U_SCOURGE, valid, SIDE));
    assert(nearest_enemy_typed(400, 400, SIDE, 16, U_SCOURGE) == valid);

    tick_combat(&u[scourge], MAX_UNITS + MAX_BUILDINGS);
    assert(u[scourge].alive);
    int building = spawn_build(ENEMY, B_NEXUS, 402, 400, 0);
    assert(building >= 0);
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, MAX_UNITS + building, SIDE));
    tick_combat(&u[scourge], -1);
    assert(u[scourge].alive && supply_used == 2);
}

static void test_range_cooldown_fixed_damage_and_suicide(void)
{
    int scourge = hatch_paid_scourges();
    int target = spawn_unit(ENEMY, U_Z6_GUARDIAN, 405, 400);
    assert(target >= 0);
    u[scourge].x = u[scourge].tx = 400;
    u[scourge].y = u[scourge].ty = 400;
    u[target].hp = u[target].maxhp = 200;
    u[target].shield = 20;
    u[target].armor = 2;
    weapons_level = 7;

    /* A source range of three pixels becomes two in the half-scale world. */
    tick_combat(&u[scourge], target);
    assert(u[scourge].alive && u[target].hp == 200 && u[target].shield == 20);
    u[target].x = 402;
    u[scourge].cd = 1;
    tick_combat(&u[scourge], target);
    assert(u[scourge].alive && u[target].hp == 200 && u[target].shield == 20);

    u[scourge].cd = 0;
    tick_combat(&u[scourge], target);
    assert(!u[scourge].alive && !u[scourge].selected);
    assert(u[target].shield == 0 && u[target].hp == 112);
    assert(supply_used == 1); /* The second member of the paid pair remains. */
    int hp = u[target].hp;
    tick_combat(&u[scourge], target);
    assert(u[target].hp == hp && supply_used == 1);
}

int main(void)
{
    test_spire_gate_and_playable_selection();
    test_air_only_validity_and_guarded_targets();
    test_range_cooldown_fixed_damage_and_suicide();
    puts("Z7 Scourge Spire gate, paid production, air-only combat and suicide passed");
    return 0;
}
