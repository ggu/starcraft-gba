/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed actual-engine HOST scenes; not saved, ordinary or ARM proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/game.c"
#ifndef SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT
#error "requires the isolated fractional combat stage"
#endif
static void reset_world(void)
{
    game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;
    race_bot_races[0]=R_PROTOSS;race_skirmish_map=0;init_state();
    memset(u,0,sizeof u);memset(b,0,sizeof b);
    memset(invincible_units,0,sizeof invincible_units);
    memset(invincible_buildings,0,sizeof invincible_buildings);
    memset(&sc64_experimental_research_levels,0,sizeof sc64_experimental_research_levels);
    weapons_level=armor_level=0;race_p7_projectile_reset();
    for(int cell=0;cell<MAP_CELLS;++cell)SC64_GRID_SET(visible,cell,1);
}
static int actor(int owner,int raw,int x,int y)
{
    int type=sc64_race_unit_from_raw(raw);assert(type>=0);
    int slot=spawn_unit(owner,type,x,y);assert(slot>=0);
    u[slot].raw_type=raw;u[slot].raw_owner=owner==SIDE?7:1;
    return slot;
}
static unsigned hp_q8(int slot)
{
    unsigned result;assert(sc64_pr_q8_hp(u[slot].hp,sc64_pr_hp_debt[slot],&result));return result;
}
static void shield_and_hp_packets(void)
{
    reset_world();int v=actor(SIDE,65,500,500);
    sc64_experimental_research_levels.level[SIDE][SC64_PR_SHIELDS]=3;
    hit_target_impl(v,6,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL);
    assert(u[v].shield==57 && !u[v].shield_remainder && u[v].hp==100);
    u[v].shield=1;u[v].shield_remainder=640;
    hit_target_impl(v,6,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL);
    assert(u[v].shield==0 && !u[v].shield_remainder && hp_q8(v)==100*256-128);
    /* Below one shield point, the source damage routine bypasses shields. */
    u[v].hp=100;sc64_pr_hp_debt[v]=0;u[v].shield_remainder=640;
    hit_target_impl(v,6,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL);
    assert(u[v].hp==95 && u[v].shield==0 && u[v].shield_remainder==640);
    /* Two half-point HP hits consume exactly one point, not two. */
    u[v].armor=10;u[v].shield_remainder=0;
    hit_target_impl(v,1,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL);
    assert(u[v].hp==95 && sc64_pr_hp_debt[v]==128);
    hit_target_impl(v,1,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL);
    assert(u[v].hp==94 && !sc64_pr_hp_debt[v]);
    /* Shield mitigation keeps the fractional remainder and time carry. */
    u[v].shield=10;u[v].shield_remainder=3;
    hit_target_impl_ex(v,128,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL,SC64_HIT_PACKET_Q8);
    assert(u[v].shield==9 && u[v].shield_remainder==643);
    race_tick_state();assert(u[v].shield==9 && u[v].shield_remainder==671);
    Unit before=u[v];unsigned debt=sc64_pr_hp_debt[v];u[v].flags|=16;before.flags|=16;
    hit_target_impl(v,20,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL);
    assert(!memcmp(&before,&u[v],sizeof before)&&debt==sc64_pr_hp_debt[v]);
}
static void shared_special_routes(void)
{
    reset_world();int v=actor(SIDE,65,500,500);u[v].shield=0;
    assert(race_p7_archon_damage_one(v,20*256+128,ENEMY));
    assert(hp_q8(v)==100*256-(19*256+128));
    assert(race_p7_archon_damage_one(v,20*256+128,ENEMY));
    assert(hp_q8(v)==61*256);
    reset_world();v=actor(SIDE,65,224,64);u[v].shield=1;u[v].shield_remainder=640;
    sc64_experimental_research_levels.level[SIDE][SC64_PR_SHIELDS]=3;
    int shooter=actor(ENEMY,73,64,64);
    assert(race_p7_projectile_fire(shooter,v,64,64*512,64*512));
    for(unsigned n=0;n<80 && u[v].shield;++n)race_p7_projectile_source_tick();
    assert(!race_p7_projectile_fault && hp_q8(v)==100*256-128 && !u[v].shield);
    for(unsigned n=0;n<20;++n)race_p7_projectile_source_tick();
    assert(hp_q8(v)==100*256-128);
    reset_world();int primary=actor(ENEMY,7,500,500),outer=actor(ENEMY,37,523,500);
    shooter=actor(SIDE,30,450,500);
    int distance=sc64_siege_distance(37,523,500,500,500);assert(distance>25&&distance<=40);
    race_siege_splash_attack(&u[shooter],primary,5);
    assert(hp_q8(outer)==35*256-160); /* 5/4 Explosive vs Small = 5/8 HP. */
}
static void buildings_and_lifetime(void)
{
    reset_world();int type=sc64_race_building_from_raw(162);
    int slot=spawn_build(SIDE,type,500,500,0);assert(slot>=0);
    sc64_experimental_research_levels.level[SIDE][SC64_PR_SHIELDS]=3;
    int shield=b[slot].shield;
    hit_target_impl(MAX_UNITS+slot,6,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL);
    assert(b[slot].shield==shield-3 && !b[slot].shield_remainder);
    b[slot].shield_remainder=640;race_tick_state();assert(b[slot].shield_remainder==668);
    int v=actor(SIDE,7,600,500);u[v].shield_remainder=123;u[v].armor=10;
    hit_target_impl(v,1,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL);
    assert(sc64_pr_hp_debt[v]==128 && u[v].shield_remainder==123);
    u[v].hp=1;sc64_pr_hp_debt[v]=128;supply_used=unit_slot_supply(&u[v]);
    hit_target_impl(v,1,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL);
    assert(!u[v].alive && !sc64_pr_hp_debt[v] && !supply_used);
    int replacement=actor(SIDE,7,600,500);assert(replacement==v&&!sc64_pr_hp_debt[v]);
    static unsigned char prior[32768];memset(host_save,0xa5,sizeof host_save);memcpy(prior,host_save,sizeof prior);
    assert(!save_game()&&!race_save_game()&&!load_game()&&!race_load_game());
    assert(!memcmp(prior,host_save,sizeof prior));
    sc64_pr_hp_debt[v]=128;init_state();
    for(unsigned n=0;n<sizeof sc64_pr_hp_debt;++n)assert(!sc64_pr_hp_debt[n]);
}
int main(void)
{
    gba_init();shield_and_hp_packets();shared_special_routes();buildings_and_lifetime();
    puts("actual shared fractional damage, plasma armor, splash, regen and lifecycle HOST stage: PASS");return 0;
}
