/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void) {
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();screen=SCREEN_PLAY;
    race_skirmish_map=1; /* authenticated Blood Bath has no CV5 cover cells */
    int a=spawn_unit(ENEMY,unit_raw(12),500,500),t=spawn_unit(NEUTRAL,unit_raw(0),640,500);
    assert(a>=0 && t>=0);u[a].carry=200*256;u[t].hp=u[t].maxhp=500;u[t].armor=0;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));
    int charge=-1,launch=-1,impact=-1,source_age=0;
    for(int n=0;n<150;++n) {
        int before=sc64_bc_shots[0].state;game_update(0,0);
        if(before==SC64_BC_SHOT_CHARGING && sc64_battlecruiser_source_tick_due())++source_age;
        if(charge<0 && sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING){charge=frame;assert(((unsigned)u[a].work_timer>>3)==60);}
        if(launch<0 && sc64_bc_shots[0].state==SC64_BC_SHOT_FLIGHT){launch=frame;assert(source_age==44);}
        if(launch>=0 && !sc64_bc_shots[0].state){impact=frame;break;}
    }
    assert(charge>0 && launch>charge && impact>launch && u[t].hp==370);
    printf("private automatic cast: charge=%d launch=%d impact=%d source-age=%d damage=%d\\n",charge,launch,impact,source_age,500-u[t].hp);
}
