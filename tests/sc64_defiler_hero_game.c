/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic setup using the actual raw-52 mapping, spawn and pad handlers. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#ifndef SC64_SHARED_DEFILER_HERO_PROBE
#error "requires disabled hero identity"
#endif
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    int type=sc64_race_unit_from_raw(52);assert(type==U_SHARED_UNCLEAN_ONE);
    assert(sc64_race_raw_unit(type)==52);
    int l,t,r,d;
    assert(sc64_zp_actor_sight_source_pixels(type)==320 &&
           sc64_zp_actor_bounds_source(type,&l,&t,&r,&d));
    assert(l==14 && t==14 && r==14 && d==14);
    assert(sc64_zp_actor_sight_source_pixels(U_SHARED_DEFILER)==320 &&
           sc64_zp_actor_bounds_source(U_SHARED_DEFILER,&l,&t,&r,&d));
    assert(l==13 && t==12 && r==13 && d==12);
    int before_supply=supply_used;
    int c=spawn_unit(SIDE,type,320,320);assert(c>=0);
    assert(supply_used==before_supply);
    assert(u[c].maxhp==250 && u[c].hp==250 && u[c].armor==3);
    assert(sc64_defiler_stage[c].live && sc64_defiler_stage[c].heading==104 &&
           sc64_defiler_stage[c].energy_q8==16000);
    assert(!race_save_game());
    screen=SCREEN_PLAY;race_mode=1;selected_building=-1;u[c].selected=1;
    for(int n=0;n<1600 && sc64_defiler_stage[c].energy_q8<100u*256u;n++)game_update(0,0);
    assert(u[c].alive && sc64_defiler_stage[c].energy_q8>=100u*256u);
    game_update(KEY_B,KEY_B);assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=field_menu_count_base();n++) {game_update(0,0);game_update(KEY_DOWN,KEY_DOWN);}
    assert(menu_cursor==field_menu_count_base());game_update(0,0);game_update(KEY_A,KEY_A);
    assert(mode==MODE_DARK_SWARM_STAGE);cx=u[c].x+80;cy=u[c].y;game_update(0,0);game_update(KEY_A,KEY_A);
    assert(mode==MODE_NORMAL);
    int did_bullet=0,did_field=0;
    for(int n=0;n<40;n++) {
        game_update(0,0);did_bullet|=sc64_shared_projectiles.active!=0;
        for(int i=0;i<MAX_UNITS;i++)did_field|=sc64_defiler_field_get(i).remaining!=0;
    }
    assert(did_bullet && did_field && sc64_defiler_stage[c].energy_q8<100u*256u);
    puts("raw52 distinct actor natural energy regeneration and actual menu cast passed");return 0;
}
