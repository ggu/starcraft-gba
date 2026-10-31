/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void) {
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();screen=SCREEN_PLAY;
    int a=spawn_unit(ENEMY,unit_raw(12),500,500),t=spawn_unit(SIDE,unit_raw(0),640,500);
    assert(a>=0 && t>=0);u[a].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));
    game_update(0,0);assert((u[a].work_timer>>3)==20 && sc64_bc_shots[0].state==SC64_BC_SHOT_APPROACH && u[a].carry==200*256);
    assert(race_save_game());u[a].work_timer=0;assert(race_load_game() && (u[a].work_timer>>3)==20);
    screen=SCREEN_PLAY;game_update(0,0);assert((u[a].work_timer>>3)==40 && sc64_bc_shots[0].state==SC64_BC_SHOT_APPROACH && u[a].carry==200*256);
    game_update(0,0);assert((u[a].work_timer>>3)==60 && sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING && u[a].carry>=50*256 && u[a].carry<51*256);
    unsigned heading=(unsigned)u[a].work_timer>>3;int cooldown=u[a].build;
    u[a].selected=1;stop_selected();assert(((unsigned)u[a].work_timer>>3)==heading && !(u[a].work_timer&7) && u[a].build==cooldown);
    puts("private persistent BC heading: turn, aim tolerance, save and STOP pass");
}
