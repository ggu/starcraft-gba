/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A synthetic normal spawn; no live energy or owner assignment. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    int c=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(c>=0);
    assert(SC64_DEFILER_ACTOR(c).energy_q8==50u*256u && SC64_DEFILER_ACTOR(c).heading==104);
    screen=SCREEN_PLAY;race_mode=1;selected_building=-1;u[c].selected=1;
    int updates=0;
    while(updates<2400 && u[c].alive && SC64_DEFILER_ACTOR(c).energy_q8<100u*256u) {game_update(0,0);++updates;}
    assert(u[c].alive && screen==SCREEN_PLAY && SC64_DEFILER_ACTOR(c).energy_q8>=100u*256u);
    game_update(KEY_B,KEY_B);assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=field_menu_count_base();n++) {game_update(0,0);game_update(KEY_DOWN,KEY_DOWN);}
    assert(menu_cursor==field_menu_count_base());game_update(0,0);game_update(KEY_A,KEY_A);
    assert(mode==MODE_DARK_SWARM_STAGE);cx=u[c].x+80;cy=u[c].y;game_update(0,0);game_update(KEY_A,KEY_A);
    int bullet=0,field=0;
    for(int n=0;n<40;n++) {
        game_update(0,0);bullet|=sc64_shared_projectiles.active!=0;
        for(int i=0;i<MAX_UNITS;i++)field|=sc64_defiler_field_get(i).remaining!=0;
    }
    assert(bullet && field && SC64_DEFILER_ACTOR(c).energy_q8<100u*256u);
    printf("natural raw46 regeneration after %d game updates and actual menu cast passed\n",updates);
    return 0;
}
