/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void) {
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    int a=spawn_unit(ENEMY,unit_raw(12),500,500),t=spawn_unit(SIDE,unit_raw(0),640,500);
    assert(a>=0 && t>=0); u[a].work_timer=64<<3;u[a].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));
    if (!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(race_bc_yamato_tick_caster(a));
    for (int n=0;n<55;++n) {++frame;assert(race_bc_yamato_tick_caster(a));}
    assert(sc64_bc_shots[0].age==44);
    unsigned before_rng=sc64_bc_spell_rng; u[t].flags|=16;
    assert(!race_bc_yamato_launch_source(0,64,0,0) && sc64_bc_spell_rng==before_rng);
    u[t].flags&=~16;
    assert(race_bc_yamato_launch_source(0,64,0,0) && sc64_bc_spell_rng==3724055312u);
    assert(u[t].alive && race_bc_yamato_projectile_step(0) && u[t].alive);
    Sc64BcYamatoShot saved=sc64_bc_shots[0];
    assert(race_save_game()); memset(sc64_bc_shots,0,sizeof(sc64_bc_shots));
    assert(race_load_game() && !memcmp(&saved,&sc64_bc_shots[0],sizeof saved));
    /* Source type-0 fly snapshots its endpoint while retaining target ref. */
    u[t].x=800;
    for (int i=0;i<30 && sc64_bc_shots[0].state;++i) assert(race_bc_yamato_projectile_step(0));
    assert(!sc64_bc_shots[0].state && !u[t].alive);
    /* A miss reaches the endpoint without damaging its target. */
    init_state(); a=spawn_unit(ENEMY,unit_raw(12),500,500); t=spawn_unit(SIDE,unit_raw(0),640,500);
    assert(a>=0 && t>=0); u[a].work_timer=64<<3;u[a].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));
    if (!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(race_bc_yamato_tick_caster(a));
    for(int n=0;n<55;++n) {++frame;assert(race_bc_yamato_tick_caster(a));}
    assert(race_bc_yamato_launch_resolved(0,64,1,1220,1000));
    for(int n=0;n<30 && sc64_bc_shots[0].state;++n) assert(race_bc_yamato_projectile_step(0));
    assert(!sc64_bc_shots[0].state && u[t].alive);
    /* Removed target must not redirect the surviving missile to a reused slot. */
    init_state(); a=spawn_unit(ENEMY,unit_raw(12),500,500); t=spawn_unit(SIDE,unit_raw(0),640,500);
    u[a].work_timer=64<<3;u[a].carry=200*256; sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));
    if (!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(race_bc_yamato_tick_caster(a));
    for(int n=0;n<55;++n) {++frame;assert(race_bc_yamato_tick_caster(a));}
    assert(race_bc_yamato_launch_resolved(0,64,0,1280,1000));
    hit_target(a,9999,U_MARINE,SIDE); assert(!u[a].alive);
    assert(sc64_bc_shots[0].caster==SC64_BC_SHOT_NO_CASTER);
    hit_target(t,9999,unit_raw(12),ENEMY); assert(!u[t].alive);
    int replacement=spawn_unit(SIDE,unit_raw(0),640,500);
    assert(replacement>=0 && sc64_bc_shots[0].state==SC64_BC_SHOT_FLIGHT &&
           sc64_bc_shots[0].target==SC64_BC_SHOT_NO_TARGET);
    for(int n=0;n<30 && sc64_bc_shots[0].state;++n) assert(race_bc_yamato_projectile_step(0));
    assert(!sc64_bc_shots[0].state && u[replacement].alive);
    puts("private resolved launch: flight, in-place save, impact, miss and removal pass");
}
