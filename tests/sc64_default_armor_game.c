/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Regression for default-profile base armor and live armor upgrades. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifdef SC64_RACES
#error "This fixture targets the asset-free default profile"
#endif

static int add_unit(int owner, int type)
{
    int slot = spawn_unit(owner, type, 100, 100);
    assert(slot >= 0);
    u[slot].hp = u[slot].maxhp = 100;
    return slot;
}

static void expect_damage(int slot, int expected)
{
    u[slot].hp = 100;
    hit_target(slot, 10, U_MARINE, ENEMY);
    assert(u[slot].hp == 100 - expected);
}

int main(void)
{
    init_state();
    memset(u, 0, sizeof(u));
    armor_level = 0;
    int old_marine = add_unit(SIDE, U_MARINE);
    int old_firebat = add_unit(SIDE, U_FIREBAT);
    int old_medic = add_unit(SIDE, U_MEDIC);
    int enemy_marine = add_unit(ENEMY, U_MARINE);
    expect_damage(old_marine, 10);
    expect_damage(old_firebat, 9);
    expect_damage(old_medic, 9);

    armor_level = 1;
    int new_marine = add_unit(SIDE, U_MARINE);
    int new_firebat = add_unit(SIDE, U_FIREBAT);
    int new_medic = add_unit(SIDE, U_MEDIC);
    expect_damage(old_marine, 9);
    expect_damage(new_marine, 9);
    expect_damage(old_firebat, 8);
    expect_damage(new_firebat, 8);
    expect_damage(old_medic, 8);
    expect_damage(new_medic, 8);
    expect_damage(enemy_marine, 10);

    armor_level = 2;
    expect_damage(old_marine, 8);
    expect_damage(new_marine, 8);
    expect_damage(old_firebat, 7);
    expect_damage(new_firebat, 7);
    expect_damage(old_medic, 7);
    expect_damage(new_medic, 7);
    expect_damage(enemy_marine, 10);

    /* Serialize the historical representation: old units captured their
     * spawn-time upgrade in armor. Load through the real save system. */
    memset(b, 0, sizeof(b));
    supply_cap = 20;
    train_type = U_MARINE;
    u[new_marine].armor = 1;
    u[new_firebat].armor = u[new_medic].armor = 2;
    u[new_marine].hp = 73;
    assert(save_game());
    memset(u, 0, sizeof(u));
    armor_level = 0;
    assert(load_game());
    assert(armor_level == 2 && u[new_marine].hp == 73);
    assert(u[old_marine].armor == 0 && u[new_marine].armor == 0);
    assert(u[old_firebat].armor == 1 && u[new_firebat].armor == 1);
    assert(u[old_medic].armor == 1 && u[new_medic].armor == 1);
    expect_damage(old_marine, 8);
    expect_damage(new_marine, 8);
    expect_damage(old_firebat, 7);
    expect_damage(new_firebat, 7);
    expect_damage(old_medic, 7);
    expect_damage(new_medic, 7);
    expect_damage(enemy_marine, 10);
    int after_load = add_unit(SIDE, U_MARINE);
    expect_damage(after_load, 8);
    puts("Default base armor and live upgrades: PASS");
    return 0;
}
