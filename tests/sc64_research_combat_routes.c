/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST weapon events, not an ordinary-input match or Q8 proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void observe_hit(int,int,int,int);
#define SC64_TEST_HIT_OBSERVER observe_hit
#include "../starcraft/game.c"
#ifndef SC64_EXPERIMENTAL_RESEARCH_JOBS
#error "requires staged research"
#endif
static unsigned interceptor_hits;
static int interceptor_packet;
static void observe_hit(int target,int damage,int source,int owner)
{
    (void)target; (void)owner;
    if(source==U_P7_INTERCEPTOR){++interceptor_hits;interceptor_packet=damage;}
}
static void reset_world(void)
{
    game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;
    race_bot_races[0]=R_PROTOSS;race_skirmish_map=0;init_state();
    memset(u,0,sizeof u);memset(b,0,sizeof b);
    memset(invincible_units,0,sizeof invincible_units);
    memset(invincible_buildings,0,sizeof invincible_buildings);
    memset(&sc64_experimental_research_levels,0,sizeof sc64_experimental_research_levels);
    weapons_level=armor_level=0;
    race_p7_projectile_reset();interceptor_hits=0;interceptor_packet=0;
    for(int cell=0;cell<MAP_CELLS;++cell)SC64_GRID_SET(visible,cell,1);
}
static int actor(int owner,int raw,int x,int y)
{
    int type=sc64_race_unit_from_raw(raw);assert(type>=0);
    int slot=spawn_unit(owner,type,x,y);assert(slot>=0);
    u[slot].raw_type=raw;u[slot].raw_owner=owner==SIDE?7:1;
    u[slot].hp=u[slot].maxhp=1000;u[slot].shield=0;u[slot].armor=0;
    return slot;
}
static void armor_routes(void)
{
    reset_world();int target=actor(SIDE,65,500,500);
    armor_level=1;u[target].armor=1;
    sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_ARMOR]=3;
    sc64_experimental_research_levels.level[SIDE][SC64_PR_AIR_ARMOR]=2;
    assert(sc64_unit_armor_upgrade_level(&u[target])==3);
    hit_target_impl(target,20,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL);
    assert(u[target].hp==984);
    u[target].hp=1000;hit_target(target,20,U_HYDRALISK,ENEMY);
    assert(u[target].hp==984);
    int shield_loss=-1;
    assert(sc64_siege_resolve(target,20*256,&shield_loss)==8 && !shield_loss);
    u[target].hp=1000;assert(race_p7_archon_damage_one(target,30*256,ENEMY));
    assert(u[target].hp==974);
    int air=actor(SIDE,70,600,500);
    assert(sc64_unit_armor_upgrade_level(&u[air])==2);
    int worker=actor(SIDE,7,700,500);
    assert(sc64_unit_armor_upgrade_level(&u[worker])==1); /* legacy, not Protoss family */
    int enemy=actor(ENEMY,65,800,500);
    assert(sc64_unit_armor_upgrade_level(&u[enemy])==0); /* no human floor leak */
    sc64_experimental_research_levels.level[ENEMY][SC64_PR_GROUND_ARMOR]=2;
    assert(sc64_unit_armor_upgrade_level(&u[enemy])==2);
}
static void archon_weapon(void)
{
    reset_world();int target=actor(ENEMY,0,500,500),shooter=actor(SIDE,68,480,500);
    sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]=3;
    sc64_experimental_research_levels.level[SIDE][SC64_PR_AIR_WEAPONS]=1;
    assert(race_p7_archon_splash_attack(&u[shooter],target));
    assert(u[target].hp==961); /* 30 + 3 per Ground Weapons level */
    assert(u[shooter].cd==SC64_P7_ARCHON_COOLDOWN_GBA_TICKS);
    reset_world();target=actor(SIDE,0,500,500);shooter=actor(ENEMY,68,480,500);
    weapons_level=2;
    sc64_experimental_research_levels.level[ENEMY][SC64_PR_GROUND_WEAPONS]=1;
    assert(race_p7_archon_splash_attack(&u[shooter],target));
    assert(u[target].hp==967); /* opponent never inherits human level two */
}
static void interceptor_weapon(int owner,unsigned air,unsigned ground,int legacy,int expected)
{
    reset_world();int victim=owner==SIDE?ENEMY:SIDE;
    int shooter=actor(owner,73,64,64),target=actor(victim,0,224,64);
    weapons_level=legacy;
    sc64_experimental_research_levels.level[owner][SC64_PR_AIR_WEAPONS]=(uint8_t)air;
    sc64_experimental_research_levels.level[owner][SC64_PR_GROUND_WEAPONS]=(uint8_t)ground;
    assert(race_p7_projectile_fire(shooter,target,64,64*512,64*512));
    assert(u[target].hp==1000 && !interceptor_hits);
    for(unsigned tick=0;tick<80 && !interceptor_hits;++tick)race_p7_projectile_source_tick();
    assert(!race_p7_projectile_fault && interceptor_hits==1);
    assert(interceptor_packet==expected && u[target].hp==1000-expected);
    for(unsigned tick=0;tick<20;++tick)race_p7_projectile_source_tick();
    assert(interceptor_hits==1); /* no repeated damage pulse */
}
int main(void)
{
    gba_init();armor_routes();archon_weapon();
    interceptor_weapon(SIDE,3,0,2,9);
    interceptor_weapon(SIDE,0,3,0,6);
    interceptor_weapon(ENEMY,1,3,2,7);
    puts("typed research ordinary/splash armor, Archon and Interceptor weapon events: PASS");
    return 0;
}
