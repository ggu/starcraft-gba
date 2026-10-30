/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void) {
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();
    int a=spawn_unit(ENEMY,unit_raw(12),500,500),t=spawn_unit(SIDE,unit_raw(0),683,500);
    assert(a>=0 && t>=0); u[a].work_timer=64<<3;u[a].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t)); if(!sc64_battlecruiser_source_tick_due())++frame;
    assert(race_bc_yamato_tick_caster(a));
    assert(sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING && u[a].carry==50*256);
    init_state();a=spawn_unit(ENEMY,unit_raw(12),500,500);t=spawn_unit(SIDE,unit_raw(0),684,500);
    assert(a>=0 && t>=0);u[a].work_timer=64<<3;u[a].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));if(!sc64_battlecruiser_source_tick_due())++frame;
    assert(race_bc_yamato_tick_caster(a));
    assert(sc64_bc_shots[0].state==SC64_BC_SHOT_APPROACH && u[a].carry==200*256);
    puts("private Yamato source-rectangle range boundary pass");
}
