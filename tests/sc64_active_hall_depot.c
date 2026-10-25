/* SPDX-License-Identifier: GPL-3.0-or-later
 * First-depot resource returns preserve native active Lair/Hive status. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void reset_world(void)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(geysers, 0, sizeof(geysers));
    race_mode = 0;
    race_human = R_ZERG;
    race_player[SIDE] = R_ZERG;
    race_player[ENEMY] = R_ZERG;
    minerals = gas = 5000;
    supply_used = 0;
    supply_cap = 200;
    frame = 1;
    path_grid_valid = 0;
    invalidate_path_grid();
}

static int hall(int owner, int type, int x, int y, int incomplete)
{
    int i = spawn_build(owner, type, x, y, incomplete);
    assert(i >= 0);
    if (incomplete) b[i].build_progress = 0;
    return i;
}

static int worker(int order, int target, int x, int y, int carry)
{
    int i = spawn_unit(SIDE, U_DRONE, x, y);
    assert(i >= 0);
    u[i].order = order;
    u[i].target = target;
    u[i].carry = carry;
    return i;
}

static void test_first_slot_and_unfinished_native_halls(void)
{
    reset_world();
    int lair = hall(SIDE, B_LAIR, 320, 320, 1);
    int hatch = hall(SIDE, B_HATCHERY, 560, 320, 0);
    assert(lair == 0 && hatch == 1);
    assert(owner_command_center(SIDE) == lair); /* Existing first-slot rule. */

    reset_world();
    int hive = hall(SIDE, B_HIVE, 320, 320, 1);
    assert(owner_command_center(SIDE) == hive); /* Native active-depot rule. */

    reset_world();
    int fresh = hall(SIDE, B_HATCHERY, 320, 320, 1);
    assert(owner_command_center(SIDE) == -1); /* Fresh incomplete Hatch stays inactive. */
    assert(fresh == 0);

    reset_world();
    hall(ENEMY, B_LAIR, 320, 320, 1);
    hall(SIDE, B_GATEWAY, 360, 320, 0);
    assert(owner_command_center(SIDE) == -1); /* Enemy hall / wrong type rejected. */
}

static void test_real_lair_morph_accepts_mineral_and_gas_cargo(void)
{
    reset_world();
    int hatch = hall(SIDE, B_HATCHERY, 320, 320, 0);
    assert(hall(SIDE, B_SPAWNING_POOL, 400, 320, 0) >= 0);
    assert(race_start_lair_morph(hatch));
    assert(b[hatch].type == B_LAIR && b[hatch].under_construction);
    assert(owner_command_center(SIDE) == hatch);

    int mineral_worker = worker(ORDER_MINE, 0, 328, 320, 6);
    m[0].x = 600; m[0].y = 600; m[0].amount = 100;
    int mineral_before = minerals;
    update_orders();
    assert(u[mineral_worker].carry == 0 && minerals == mineral_before + 6);

    int extractor = hall(SIDE, B_EXTRACTOR, 450, 320, 0);
    int gas_worker = worker(ORDER_GAS, extractor, 328, 320, 7);
    int gas_before = gas;
    update_orders();
    assert(u[gas_worker].carry == 0 && gas == gas_before + 7);
}

static void test_under_construction_hive_returns_cargo(void)
{
    reset_world();
    int hive = hall(SIDE, B_HIVE, 320, 320, 1);
    int mineral_worker = worker(ORDER_MINE, 0, 328, 320, 5);
    m[0].x = 600; m[0].y = 600; m[0].amount = 100;
    int mineral_before = minerals;
    update_orders();
    assert(owner_command_center(SIDE) == hive);
    assert(u[mineral_worker].carry == 0 && minerals == mineral_before + 5);
}


static void test_incomplete_hive_codec_death_and_slot_reuse(void)
{
    reset_world();
    int hive = hall(SIDE, B_HIVE, 320, 320, 1);
    Sc64PackedBuildingSave encoded;
    Building decoded;
    assert(sc64_building_save_encode(&encoded, &b[hive], WORLD_W, WORLD_H));
    assert(sc64_building_save_decode(&decoded, &encoded));
    b[hive] = decoded;
    assert(owner_command_center(SIDE) == hive); /* Save/load keeps native hall identity. */

    b[hive].alive = 0;
    assert(owner_command_center(SIDE) == -1);
    int reused = hall(SIDE, B_HATCHERY, 320, 320, 1);
    assert(reused == hive && owner_command_center(SIDE) == -1);
}
int main(void)
{
    test_first_slot_and_unfinished_native_halls();
    test_real_lair_morph_accepts_mineral_and_gas_cargo();
    test_under_construction_hive_returns_cargo();
    test_incomplete_hive_codec_death_and_slot_reuse();
    puts("active Zerg hall depot checks passed");
    return 0;
}
