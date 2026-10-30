/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void) {
    const int raws[]={0,2,12}, expected[]={130,195,260};
    for(int j=0;j<3;++j) {
        game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();
        int a=spawn_unit(ENEMY,unit_raw(12),500,500),t=spawn_unit(SIDE,unit_raw(raws[j]),510,500);
        assert(a>=0 && t>=0);u[a].work_timer=64<<3;u[a].carry=200*256;u[t].hp=u[t].maxhp=500;u[t].armor=0;u[t].shield=0;
        sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
        assert(race_bc_yamato_issue(a,t));if(!sc64_battlecruiser_source_tick_due())++frame;
        assert(race_bc_yamato_tick_caster(a));
        for(int n=0;n<55;++n){++frame;assert(race_bc_yamato_tick_caster(a));}
        #ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
        assert(sc64_bc_shots[0].state==SC64_BC_SHOT_FLIGHT);
        #else
        assert(race_bc_yamato_launch_resolved(0,64,0,1020,1000));
        #endif
        for(int n=0;n<30 && sc64_bc_shots[0].state;++n)assert(race_bc_yamato_projectile_step(0));
        assert(!sc64_bc_shots[0].state && u[t].hp==500-expected[j]);
    }
    /* Existing shared damage path consumes shields before armor/size. This
     * fixture checks its integer result, not full source Q8 shield armor. */
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();
    int a=spawn_unit(ENEMY,unit_raw(12),500,500),t=spawn_unit(SIDE,unit_raw(2),510,500);
    assert(a>=0 && t>=0);u[a].work_timer=64<<3;u[a].carry=200*256;u[t].hp=u[t].maxhp=500;u[t].armor=10;u[t].shield=100;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));if(!sc64_battlecruiser_source_tick_due())++frame;
    assert(race_bc_yamato_tick_caster(a));
    for(int n=0;n<55;++n){++frame;assert(race_bc_yamato_tick_caster(a));}
    #ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    assert(sc64_bc_shots[0].state==SC64_BC_SHOT_FLIGHT);
    #else
    assert(race_bc_yamato_launch_resolved(0,64,0,1020,1000));
    #endif
    for(int n=0;n<30 && sc64_bc_shots[0].state;++n)assert(race_bc_yamato_projectile_step(0));
    assert(u[t].shield==0 && u[t].hp==388);
    puts("private Yamato explosive damage: small, medium, large and engine shield/armor pass");
}
