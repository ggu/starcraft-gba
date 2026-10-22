/* Focused Higher Ground Scourge combat fixture. Compile with the private
 * unified P3 map inputs; exercises the real game.c combat path. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_p3.h"
#include "../starcraft/game.c"

static int scourge_at(int x, int y)
{
    return spawn_unit(SIDE, U_SCOURGE, x, y);
}

static int enemy_unit_at(int type, int x, int y)
{
    return spawn_unit(ENEMY, type, x, y);
}

static void reset_world(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(path_walkable, 0xff, sizeof(path_walkable));
    race_mode = 0;
    race_human = R_PROTOSS;
    race_selected_mission = SC64_P3_GLOBAL_ID;
    race_cols = SC64_P3_WIDTH_TILES;
    race_rows = SC64_P3_HEIGHT_TILES;
    supply_used = 0;
    weapons_level = armor_level = 0;
    unit_tap = -1;
    target_lists_valid = 0;
    sc64_target_index_active = 0;
    path_grid_valid = 1;
    lost = won = 0;
}

static void test_scourge_targets_only_hostile_air_units(void)
{
    int scourge, ground, building, ally, invincible, valid_air;
    reset_world();
    scourge = scourge_at(400, 400);
    ground = enemy_unit_at(U_MARINE, 401, 400);
    assert(scourge >= 0 && ground >= 0);
    assert(sc64_race_unit_damage_vs(U_SCOURGE, 0) == 0);
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, ground, SIDE));
    tick_combat(&u[scourge], ground);
    assert(u[scourge].alive && u[ground].hp == u[ground].maxhp);

    building = spawn_build(ENEMY, B_NEXUS, 402, 400, 0);
    assert(building >= 0);
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, MAX_UNITS + building, SIDE));
    tick_combat(&u[scourge], MAX_UNITS + building);
    assert(u[scourge].alive && b[building].alive);

    ally = spawn_unit(RESCUABLE, U_WRAITH, 399, 400);
    assert(ally >= 0);
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, ally, SIDE));
    tick_combat(&u[scourge], ally);
    assert(u[scourge].alive && u[ally].hp == u[ally].maxhp);

    invincible = enemy_unit_at(U_WRAITH, 403, 400);
    valid_air = enemy_unit_at(U_WRAITH, 408, 400);
    assert(invincible >= 0);
    assert(valid_air >= 0);
    u[invincible].flags |= 16;
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, invincible, SIDE));
    tick_combat(&u[scourge], invincible);
    assert(u[scourge].alive && u[invincible].hp == u[invincible].maxhp);
    u[invincible].flags &= ~16;
    invincible_units[invincible] = 1;
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE, invincible, SIDE));
    tick_combat(&u[scourge], invincible);
    assert(u[scourge].alive && u[invincible].hp == u[invincible].maxhp);

    assert(sc64_race_unit_damage_vs(U_SCOURGE, 1) == 110);
    assert(sc64_race_unit_range_vs(U_SCOURGE, 1) == 3);
    assert(sc64_race_unit_cooldown_vs(U_SCOURGE, 1) == 1);
    assert(nearest_enemy_typed(400, 400, SIDE, 16, U_SCOURGE) == valid_air);
}

static void test_scourge_range_cooldown_damage_and_suicide(void)
{
    int scourge, target;
    int starting_supply;
    reset_world();
    scourge = scourge_at(400, 400);
    target = enemy_unit_at(U_WRAITH, 405, 400);
    assert(scourge >= 0 && target >= 0);
    assert(supply_used == 1);

    /* BWAPI's 3px weapon range is converted to a 2px simulation range. */
    tick_combat(&u[scourge], target);
    assert(u[scourge].alive && u[target].hp == u[target].maxhp);

    u[scourge].x = 400;
    u[scourge].y = 400;
    u[target].x = 402;
    u[target].y = 400;
    u[target].hp = 80;
    u[target].maxhp = 80;
    u[target].shield = 50;
    u[target].armor = 3;
    u[scourge].cd = 1;
    tick_combat(&u[scourge], target);
    assert(u[scourge].alive && u[target].hp == 80 && u[target].shield == 50);

    weapons_level = 2;
    u[scourge].cd = 0;
    starting_supply = supply_used;
    tick_combat(&u[scourge], target);
    assert(!u[scourge].alive && !u[scourge].selected);
    assert(supply_used == starting_supply - 1);
    assert(u[target].shield == 0 && u[target].hp == 23);

    /* A consumed Scourge cannot apply the same attack again. */
    tick_combat(&u[scourge], target);
    assert(u[target].hp == 23 && !u[scourge].alive);
}

static void test_attack_move_approaches_and_hits_once(void)
{
    int scourge, target, start_x;
    reset_world();
    scourge = scourge_at(400, 400);
    /* Dropships are flying, valid targets without a weapon that can kill the
     * Scourge while it approaches. */
    target = enemy_unit_at(U_DROPSHIP, 480, 400);
    assert(scourge >= 0 && target >= 0);
    u[target].hp = 100;
    u[target].maxhp = 100;
    u[scourge].order = ORDER_ATTACK_MOVE;
    u[scourge].tx = 520;
    u[scourge].ty = 400;
    start_x = u[scourge].x;

    for (int tick = 0; tick < 400 && u[scourge].alive; ++tick)
        update_orders();

    assert(!u[scourge].alive);
    assert(!u[target].alive);
    assert(u[scourge].x > start_x);
    assert(supply_used == 0);
}

static void test_lethal_hit_clears_target_and_supply(void)
{
    int scourge, target;
    reset_world();
    scourge = scourge_at(400, 400);
    target = enemy_unit_at(U_WRAITH, 402, 400);
    assert(scourge >= 0 && target >= 0);
    u[target].hp = 25;
    u[target].maxhp = 25;
    tick_combat(&u[scourge], target);
    assert(!u[scourge].alive && !u[target].alive);
    assert(supply_used == 0);
    assert(u[target].hp <= 0);
}

static void test_invalid_target_indices_are_ignored(void)
{
    int scourge;
    const int invalid_targets[] = {
        -1,
        MAX_UNITS + MAX_BUILDINGS,
        0x7fffffff
    };
    reset_world();
    scourge = scourge_at(400, 400);
    assert(scourge >= 0 && supply_used == 1);
    for (unsigned i = 0; i < sizeof(invalid_targets) / sizeof(invalid_targets[0]); ++i) {
        tick_combat(&u[scourge], invalid_targets[i]);
        assert(u[scourge].alive);
        assert(supply_used == 1);
    }
}

int main(void)
{
    test_scourge_targets_only_hostile_air_units();
    test_scourge_range_cooldown_damage_and_suicide();
    test_attack_move_approaches_and_hits_once();
    test_lethal_hit_clears_target_and_supply();
    test_invalid_target_indices_are_ignored();
    puts("P3 Scourge air-only suicide combat passed");
    return 0;
}
