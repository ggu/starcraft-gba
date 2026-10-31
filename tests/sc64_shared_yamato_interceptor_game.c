/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#if !defined(SC64_SHARED_PROJECTILE_STAGE) || !defined(SC64_P7_PROJECTILES_ENABLED) || !defined(SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT)
#error "fixture needs both real projectile families and the stage gate"
#endif
static void setup(int *carrier,int *foe,int *bc,int *victim)
{
    game_init(); race_mode=0;race_selected_mission=SC64_P7_GLOBAL_ID;init_state();
    *carrier=spawn_unit(SIDE,U_P7_INTERCEPTOR,64,64);
    *foe=spawn_unit(ENEMY,U_MARINE,224,64);
    *bc=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),500,500);
    *victim=spawn_unit(SIDE,U_MARINE,510,500);
    assert(*carrier>=0 && *foe>=0 && *bc>=0 && *victim>=0);
    u[*foe].raw_type=0;u[*foe].raw_owner=0;
    u[*bc].raw_owner=0;u[*bc].work_timer=64<<3;u[*bc].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)SC64_BC_TECH_YAMATO;
}
static void charge_and_release(int bc)
{
    unsigned reserved=sc64_shared_projectiles.active;
    if(!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(race_bc_yamato_tick_caster(bc));
    assert(sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING);
    assert(sc64_shared_projectiles.active==reserved);
    assert(u[bc].carry==50*256 && u[bc].build>=13);
    for(int n=0;n<100 && sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING;n++) {
        ++frame;assert(race_bc_yamato_tick_caster(bc));
        if(sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING)
            assert(sc64_shared_projectiles.active==reserved);
    }
}
int main(void)
{
    int shooter,enemy,bc,victim;
    setup(&shooter,&enemy,&bc,&victim);
    for(int i=0;i<99;i++)assert(race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    assert(sc64_shared_projectiles.active==99);
    assert(race_bc_yamato_issue(bc,victim));
    assert(sc64_shared_projectiles.active==99); /* no slot for approach */
    charge_and_release(bc);
    assert(sc64_bc_shots[0].state==SC64_BC_SHOT_FLIGHT && sc64_shared_projectiles.active==100);
    assert(!race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    assert(!race_save_game() && !race_load_game());
    hit_target(bc,9999,U_MARINE,SIDE);assert(!u[bc].alive);
    hit_target(victim,9999,sc64_race_unit_from_raw(12),ENEMY);assert(!u[victim].alive);
    int replacement=spawn_unit(SIDE,U_MARINE,510,500); assert(replacement>=0);
    assert(sc64_bc_shots[0].target==SC64_BC_SHOT_NO_TARGET && sc64_shared_projectiles.active==100);
    for(int i=0;i<80 && sc64_bc_shots[0].state;i++)assert(race_bc_yamato_projectile_step(0));
    assert(!sc64_bc_shots[0].state && u[replacement].alive && sc64_shared_projectiles.active==100);
    assert(!race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    assert(!race_save_game() && !race_load_game());
    for(int i=0;i<27;i++) race_bc_yamato_tail_tick();
    assert(sc64_shared_projectiles.active==100);
    race_bc_yamato_tail_tick();
    assert(sc64_shared_projectiles.active==99);
    init_state();assert(!sc64_shared_projectiles.active && !sc64_shared_yamato_handles[0]);
    setup(&shooter,&enemy,&bc,&victim);
    for(int i=0;i<100;i++)assert(race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    assert(race_bc_yamato_issue(bc,victim));
    if(!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(race_bc_yamato_tick_caster(bc));
    while(sc64_bc_shots[0].age<43) {++frame;assert(race_bc_yamato_tick_caster(bc));}
    u32 before_release_rng=sc64_bc_spell_rng;
    while(sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING) {++frame;assert(race_bc_yamato_tick_caster(bc));}
    assert(sc64_bc_spell_rng==before_release_rng);
    assert(!sc64_bc_shots[0].state && sc64_shared_projectiles.active==100);
    assert(u[bc].carry==50*256 && (u[bc].work_timer&7)==6 && u[bc].build>=13);
    puts("real Yamato and Interceptor projectile contention passed");return 0;
}
