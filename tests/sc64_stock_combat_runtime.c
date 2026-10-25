/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Engine-level fixtures for source-table weapon facts wired into tick_combat. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void reset_fixture(void)
{
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(path_walkable, 0xff, sizeof(path_walkable));
    path_grid_valid = 1;
    target_lists_valid = 0;
    sc64_target_index_active = 0;
    race_mode = 0;
    race_selected_mission = SC64_RACE_MISSION_T1;
    weapons_level = armor_level = 0;
    supply_used = 0;
}

static int add_unit(int owner, int raw, int x, int y)
{
    int type = sc64_race_unit_from_raw(raw);
    int slot;
    assert(type >= 0);
    slot = spawn_unit(owner, type, x, y);
    assert(slot >= 0);
    u[slot].raw_type = raw;
    u[slot].raw_owner = owner == ENEMY ? 1 : 0;
    return slot;
}

static void test_goliath_and_scout_weapon_factors(void)
{
    int attacker, target, initial;
    reset_fixture();
    attacker = add_unit(SIDE, 3, 100, 100);
    target = add_unit(ENEMY, 8, 120, 100);
    u[target].shield = 0;
    u[target].armor = 1;
    initial = u[target].hp;
    tick_combat(&u[attacker], target);
    assert(u[target].hp == initial - 18); /* 2x(10 - armor 1). */

    reset_fixture();
    attacker = add_unit(SIDE, 70, 100, 100);
    target = add_unit(ENEMY, 8, 120, 100);
    u[target].shield = 0;
    u[target].armor = 2;
    initial = u[target].hp;
    tick_combat(&u[attacker], target);
    assert(u[target].hp == initial - 24); /* 2x(14 - armor 2). */
}

static void test_zealot_two_blade_hits(void)
{
    int attacker, target, initial;
    reset_fixture();
    attacker = add_unit(SIDE, 65, 100, 100);
    target = add_unit(ENEMY, 0, 108, 100); /* Zealot range: 15 source px / 2. */
    u[target].shield = 0;
    u[target].armor = 1;
    initial = u[target].hp;
    tick_combat(&u[attacker], target);
    assert(u[target].hp == initial - 14); /* 2x(8 - armor 1). */
}

static void test_hydralisk_shield_first_and_explosive_size(void)
{
    int attacker, target;
    reset_fixture();
    attacker = add_unit(SIDE, 38, 100, 100);
    target = add_unit(ENEMY, 0, 120, 100); /* Small Marine. */
    u[target].shield = 5;
    u[target].armor = 2;
    u[target].hp = u[target].maxhp = 100;
    tick_combat(&u[attacker], target);
    assert(u[target].shield == 0);
    assert(u[target].hp == 99); /* (10 - 5 shield - 2 armor) * 1/2 => min 1. */
}

static void test_siege_minimum_range_and_cooldown(void)
{
    int attacker, target;
    reset_fixture();
    attacker = add_unit(SIDE, 30, 100, 100);
    target = add_unit(ENEMY, 5, 130, 100); /* 30 px: inside 32 px dead zone. */
    u[target].shield = 0;
    u[target].hp = u[target].maxhp = 100;
    tick_combat(&u[attacker], target);
    assert(u[target].hp == 100 && u[attacker].cd == 0);

    u[target].x = 132; /* Exactly 32 px is attackable. */
    tick_combat(&u[attacker], target);
    assert(u[target].hp == 31); /* Explosive 70, large target, armor 1. */
    assert(u[attacker].cd == 94); /* ceil(75 source frames * 5/4). */
}

static void test_early_raynor_mission_source_health(void)
{
    int i, found = -1;
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_RACE_MISSION_T1;
    init_state();
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == 19) { found = i; break; }
    assert(found >= 0);
    assert(u[found].type == U_RAYNOR); /* Preserve the campaign alias. */
    assert(u[found].maxhp == 300 && u[found].hp == 300);
}

int main(void)
{
    test_goliath_and_scout_weapon_factors();
    test_zealot_two_blade_hits();
    test_hydralisk_shield_first_and_explosive_size();
    test_siege_minimum_range_and_cooldown();
    test_early_raynor_mission_source_health();
    puts("source-profile engine combat fixtures passed");
    return 0;
}
