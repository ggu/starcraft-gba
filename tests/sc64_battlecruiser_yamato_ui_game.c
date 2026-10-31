/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Host-seeded actors, then ordinary B/menu/A target input and game updates. */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void) {
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();screen=SCREEN_PLAY;race_skirmish_map=1;
    int a=spawn_unit(SIDE,unit_raw(12),500,500),t=spawn_unit(NEUTRAL,unit_raw(0),640,500);
    assert(a>=0 && t>=0);u[a].selected=1;u[a].carry=200*256;u[t].hp=u[t].maxhp=500;u[t].armor=0;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*SIDE));
    update_exploration();assert(currently_visible(u[t].x,u[t].y));
    int row=race_bc_yamato_menu_base();assert(field_menu_count()==row+1 && !strcmp(race_menu_label(row),"YAMATO GUN"));
    game_update(KEY_B,KEY_B);assert(menu_open);
    for(int guard=0;menu_cursor!=row && guard<field_menu_count();++guard)game_update(KEY_DOWN,KEY_DOWN);
    assert(menu_cursor==row);game_update(KEY_A,KEY_A);assert(!menu_open && mode==MODE_YAMATO);
    cx=u[t].x;cy=u[t].y;game_update(KEY_A,KEY_A);assert(mode==MODE_NORMAL && sc64_bc_shots[0].state);
    int charged=0,launched=0;
    for(int n=0;n<160;++n){game_update(0,0);if(sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING)charged=1;
      if(sc64_bc_shots[0].state==SC64_BC_SHOT_FLIGHT)launched=1;
      if(launched && !sc64_bc_shots[0].state)break;}
    assert(charged && launched && !sc64_bc_shots[0].state && u[t].hp==370);
    puts("private normal B/menu/A Yamato order reaches source-timed impact");
}
