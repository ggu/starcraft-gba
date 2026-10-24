/* P6 damage-size mapping regression cases. Expected sizes below are taken
 * independently from the pinned BWAPI UnitType::size table, not inferred
 * from the combat helper under test. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#ifndef SC64_P6_COMBAT_ENABLED
#error "Compile this fixture with P6 combat enabled"
#endif

static void expect_unit_size(int raw, int expected)
{
    int type = sc64_race_unit_from_raw(raw);
    assert(type >= 0);
    assert(race_p6_combat_target_size(raw, type) == expected);
}

static void expect_building_size(int raw, int expected)
{
    int type = sc64_race_building_from_raw(raw);
    assert(type >= 0);
    /* Building enums live in a distinct namespace from unit enums. */
    int actual = race_p6_combat_target_size(raw, type);
    if (actual != expected)
        fprintf(stderr, "raw building %d: expected size %d, got %d\n",
                raw, expected, actual);
    assert(actual == expected);
}

static void target_size_census(void)
{
    /* Common actors used in the P6 map / initial force. */
    expect_unit_size(0, SC64_P6_SIZE_SMALL); /* Marine */
    expect_unit_size(32, SC64_P6_SIZE_SMALL); /* Firebat */
    expect_unit_size(37, SC64_P6_SIZE_SMALL); /* Zergling */
    expect_unit_size(38, SC64_P6_SIZE_MEDIUM); /* Hydralisk */
    expect_unit_size(65, SC64_P6_SIZE_SMALL); /* Zealot */
    expect_unit_size(66, SC64_P6_SIZE_LARGE); /* Dragoon */
    expect_unit_size(79, SC64_P6_SIZE_SMALL); /* Hero Tassadar */

    /* Campaign additions and P6 trap/prop classes. */
    expect_unit_size(50, SC64_P6_SIZE_SMALL); /* Infested Terran */
    expect_unit_size(74, SC64_P6_SIZE_SMALL); /* Hero Dark Templar */
    expect_unit_size(75, SC64_P6_SIZE_SMALL); /* Zeratul */
    expect_building_size(195, SC64_P6_SIZE_INDEPENDENT); /* Terran beacon */
    expect_building_size(196, SC64_P6_SIZE_INDEPENDENT); /* Protoss beacon */
    expect_building_size(203, SC64_P6_SIZE_LARGE); /* floor missile trap */
    expect_building_size(205, SC64_P6_SIZE_INDEPENDENT); /* upper door */
    expect_building_size(206, SC64_P6_SIZE_INDEPENDENT); /* right upper door */
    expect_building_size(207, SC64_P6_SIZE_INDEPENDENT); /* pit door */
    expect_building_size(208, SC64_P6_SIZE_INDEPENDENT); /* right pit door */
    expect_building_size(209, SC64_P6_SIZE_LARGE); /* floor gun trap */
    expect_building_size(210, SC64_P6_SIZE_LARGE); /* wall missile trap */
    expect_building_size(211, SC64_P6_SIZE_LARGE); /* wall flame trap */
    expect_building_size(213, SC64_P6_SIZE_LARGE); /* right wall flame trap */
}

static Sc64P6DamageTarget marine_target(unsigned hp, unsigned shields,
                                       unsigned armor)
{
    Sc64P6DamageTarget target = {0};
    target.hp_q8 = hp * 256u;
    target.shields_q8 = shields * 256u;
    target.armor_q8 = armor * 256u;
    target.size = SC64_P6_SIZE_SMALL;
    target.has_shields = shields != 0;
    target.ground_target = 1;
    return target;
}

static void shield_overflow_and_size_packet(void)
{
    Sc64P6DamageResult result;
    Sc64P6DamageTarget target = marine_target(100, 5, 3);
    /* 10 Explosive damage: shields take 5, 5 overflows, armor leaves 2,
       and Small size halves to 1 HP. */
    assert(sc64_p6_explosive_damage_q8(&target, 10, 1, &result));
    assert(result.shield_damage_q8 == 5u * 256u);
    assert(result.hp_damage_q8 == 1u * 256u);
    assert(result.shields_after_q8 == 0);
    assert(result.hp_after_q8 == 99u * 256u);

    /* A packet entirely consumed by shields does not leak a minimum hit to
       HP merely because size/armor later reduce the zero remainder. */
    target = marine_target(100, 10, 3);
    assert(sc64_p6_explosive_damage_q8(&target, 10, 1, &result));
    assert(result.shield_damage_q8 == 10u * 256u);
    assert(result.hp_damage_q8 == 0);
    assert(result.hp_after_q8 == 100u * 256u);
}

static void reset_combat_state(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(&race_p6, 0, sizeof(race_p6));
    memset(race_p6_doodad_bindings, 0xff, sizeof(race_p6_doodad_bindings));
    race_mode = 0;
    race_selected_mission = SC64_P6_GLOBAL_ID;
    sc64_p6_runtime_init(&race_p6.runtime);
    race_p6_world.source_current_player = 7;
    race_p6_world.map_cols = 128;
    race_p6_world.map_rows = 128;
}

static void live_overflow_and_source_death_accounting(void)
{
    reset_combat_state();
    u[0].alive=1; u[0].owner=SIDE; u[0].raw_owner=7;
    u[0].type=U_MARINE; u[0].raw_type=0;
    u[0].hp=100; u[0].maxhp=100; u[0].armor=3; u[0].shield=5;
    /* A 10-damage Explosive packet overflows five shields; remaining five
       loses three armor then is halved for a Small Marine. */
    assert(race_p6_combat_apply_explosive(0, 10, 1, U_P6_INFESTED_TERRAN, ENEMY));
    assert(u[0].shield==0 && u[0].hp==99 && u[0].alive);

    reset_combat_state();
    u[0].alive=1; u[0].owner=ENEMY; u[0].raw_owner=3;
    u[0].type=U_P6_INFESTED_TERRAN; u[0].raw_type=50;
    u[0].x=100; u[0].y=100; u[0].hp=u[0].maxhp=60;
    u[1].alive=1; u[1].owner=SIDE; u[1].raw_owner=7;
    u[1].type=U_MARINE; u[1].raw_type=0;
    u[1].x=110; u[1].y=100; u[1].hp=u[1].maxhp=100;
    assert(race_p6_combat_infested_explode(0)==2);
    assert(!u[0].alive && !u[1].alive);
    /* P6 accounting bins deaths by the actor's source player, not the
       attacker. The source Infested Terran is source 3; Marine source 7. */
    assert(race_p6.deaths_by_source[3]==1);
    assert(race_p6.deaths_by_source[7]==1);

    reset_combat_state();
    b[0].alive=1; b[0].owner=ENEMY; b[0].raw_owner=3;
    b[0].type=B_P6_WALL_MISSILE_TRAP; b[0].raw_type=210;
    b[0].hp=1; b[0].maxhp=50;
    hit_target(MAX_UNITS, 10, U_MARINE, SIDE);
    assert(!b[0].alive);
    assert(race_p6.deaths_by_source[3]==1);
    assert(race_p6.deaths_by_source[7]==0);
}

int main(void)
{
    target_size_census();
    shield_overflow_and_size_packet();
    live_overflow_and_source_death_accounting();
    return 0;
}
