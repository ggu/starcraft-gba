/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Firebat primary hits use stock damage and the common shield/armor path.
 * Flame collateral geometry is deliberately unimplemented. */
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
    assert(type >= 0);
    int slot = spawn_unit(owner, type, x, y);
    assert(slot >= 0);
    u[slot].raw_type = raw;
    u[slot].raw_owner = owner == ENEMY ? 1 : 0;
    return slot;
}

int main(void)
{
    reset_fixture();
    int firebat = add_unit(SIDE, 32, 100, 100);
    int primary = add_unit(ENEMY, 0, 108, 100);
    int secondary = add_unit(ENEMY, 0, 109, 100);
    u[primary].hp = u[secondary].hp = 100;
    u[primary].shield = u[secondary].shield = 20;
    tick_combat(&u[firebat], primary);
    assert(u[primary].hp == 100 && u[primary].shield == 12);
    assert(u[secondary].hp == 100 && u[secondary].shield == 20);
    assert(u[firebat].cd > 0);

    reset_fixture();
    firebat = add_unit(SIDE, 32, 100, 100);
    primary = add_unit(ENEMY, 0, 108, 100);
    u[primary].hp = 100; u[primary].shield = 0; u[primary].armor = 2;
    weapons_level = 1;
    tick_combat(&u[firebat], primary);
    assert(u[primary].hp == 93); /* (8 + 1 upgrade - 2 armor), small. */

    reset_fixture();
    firebat = add_unit(SIDE, 32, 100, 100);
    u[firebat].raw_type = 10; /* Gui Montag's stock flame weapon. */
    primary = add_unit(ENEMY, 0, 108, 100);
    u[primary].hp = 100; u[primary].shield = 0; u[primary].armor = 0;
    tick_combat(&u[firebat], primary);
    assert(u[primary].hp == 84); /* One 16-point impact, not three. */

    reset_fixture();
    firebat = add_unit(SIDE, 32, 100, 100);
    primary = add_unit(ENEMY, 0, 108, 100);
    secondary = add_unit(ENEMY, 0, 109, 100);
    u[primary].hp = 6; u[primary].shield = 0; u[primary].armor = 0;
    u[secondary].hp = 100; u[secondary].shield = 20;
    tick_combat(&u[firebat], primary);
    assert(!u[primary].alive);
    assert(u[secondary].hp == 100 && u[secondary].shield == 20);

    reset_fixture();
    firebat = add_unit(SIDE, 32, 100, 100);
    primary = add_unit(ENEMY, 8, 108, 100); /* Flying Wraith. */
    int hp = u[primary].hp;
    tick_combat(&u[firebat], primary);
    assert(u[primary].hp == hp && u[firebat].cd == 0);

    puts("Firebat stock primary damage and bounded collateral: PASS");
    return 0;
}
