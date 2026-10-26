/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual order/movement coverage for stock ground-weapon contact reach. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

static void reset_contact_scene(int attacker_type, int target_type)
{
    const Sc64RaceUnitRule *attacker_rule=sc64_race_unit_rule(attacker_type);
    const Sc64RaceUnitRule *target_rule=sc64_race_unit_rule(target_type);
    game_init();
    init_state();
    memset(u,0,sizeof(u));
    memset(b,0,sizeof(b));
    memset(path_walkable,0xff,sizeof(path_walkable));
    memset(race_player,0xff,sizeof(race_player));
    race_player[SIDE]=attacker_rule?attacker_rule->race:R_TER;
    race_player[ENEMY]=target_rule?target_rule->race:R_TER;
    race_mode=1;
    race_human=race_player[SIDE];
    race_selected_mission=0;
    weapons_level=armor_level=0;
    path_grid_valid=1;
    target_lists_valid=0;
    sc64_target_index_active=0;
    SC64_GRID_CLEAR(visible);
    for (int y=0;y<MAP_ROWS;++y)
        for (int x=0;x<MAP_COLS;++x)
            SC64_GRID_SET(visible,x+y*MAP_COLS,1);
}

static int run_attack_order(int attacker_type, int attacker_raw,
                            int target_type, int target_raw)
{
    int attacker,target,initial_hp;
    reset_contact_scene(attacker_type,target_type);
    attacker=spawn_unit(SIDE,attacker_type,100,100);
    target=spawn_unit(ENEMY,target_type,120,100);
    assert(attacker>=0&&target>=0);
    u[attacker].raw_type=attacker_raw;
    u[attacker].raw_owner=0;
    u[target].raw_type=target_raw;
    u[target].raw_owner=1;
    u[target].hp=u[target].maxhp=200;
    u[target].shield=0;

    assert(sc64_direct_attack_issue_target(target)==0);
    u[attacker].selected=1;
    assert(sc64_direct_attack_issue_target(target)==1);
    assert(u[attacker].order==ORDER_ATTACK_TARGET&&u[attacker].target==target);

    initial_hp=u[target].hp;
    for (int ticks=0;ticks<180&&u[target].hp==initial_hp;++ticks) {
        ++frame;
        update_orders();
    }
    if (u[target].hp==initial_hp)
        fprintf(stderr,"attacker raw%d stalled (%d,%d) target raw%d (%d,%d), cd%d\n",
                attacker_raw,u[attacker].x,u[attacker].y,target_raw,
                u[target].x,u[target].y,u[attacker].cd);
    return initial_hp-u[target].hp;
}

static void test_base_scv_contact(void)
{
    assert(run_attack_order(U_SCV,7,U_MARINE,0)>0);
}

#ifdef SC64_ZP_CAMPAIGN_ACTORS
static void test_small_melee_units_reach_large_actor(void)
{
    assert(sc64_game_unit_collision_radius(U_ZP_ULTRALISK)==10);
    assert(run_attack_order(U_SCV,7,U_ZP_ULTRALISK,39)>0);
    assert(run_attack_order(U_DRONE,41,U_ZP_ULTRALISK,39)>0);
    assert(run_attack_order(U_PROBE,64,U_ZP_ULTRALISK,39)>0);
    assert(run_attack_order(U_ZEALOT,65,U_ZP_ULTRALISK,39)>0);
}

static void test_ultralisk_reciprocal_contact(void)
{
    assert(run_attack_order(U_ZP_ULTRALISK,39,U_MARINE,0)>0);
}

static void test_contact_range_exact_boundary(void)
{
    int attacker,target;
    reset_contact_scene(U_SCV,U_ZP_ULTRALISK);
    attacker=spawn_unit(SIDE,U_SCV,100,100);
    target=spawn_unit(ENEMY,U_ZP_ULTRALISK,111,100);
    assert(attacker>=0&&target>=0);
    u[attacker].raw_type=7;
    u[target].raw_type=39;
    u[target].hp=u[target].maxhp=100;
    u[target].shield=0;

    tick_combat(&u[attacker],target);
    assert(u[target].hp==100&&u[attacker].cd==0); /* 11 px stays out. */
    u[target].x=110;
    tick_combat(&u[attacker],target);
    assert(u[target].hp<100&&u[attacker].cd>0); /* Radius equality is in range. */
}

static void test_minimum_range_is_not_widened(void)
{
    int attacker,target;
    reset_contact_scene(U_SIEGE_TANK,U_ZP_ULTRALISK);
    attacker=spawn_unit(SIDE,U_SIEGE_TANK,100,100);
    target=spawn_unit(ENEMY,U_ZP_ULTRALISK,110,100);
    assert(attacker>=0&&target>=0);
    u[attacker].raw_type=30;
    u[target].raw_type=39;
    u[target].hp=u[target].maxhp=100;
    u[target].shield=0;

    tick_combat(&u[attacker],target);
    assert(u[target].hp==100&&u[attacker].cd==0);
    u[target].x=132; /* Source minimum range 64 px -> 32 center px. */
    tick_combat(&u[attacker],target);
    assert(u[target].hp<100&&u[attacker].cd>0);
}
#endif

int main(void)
{
    gba_init();
    test_base_scv_contact();
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    test_small_melee_units_reach_large_actor();
    test_ultralisk_reciprocal_contact();
    test_contact_range_exact_boundary();
    test_minimum_range_is_not_widened();
#endif
    puts("stock melee contact fixtures passed");
    return 0;
}
