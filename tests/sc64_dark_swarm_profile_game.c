/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The same real menu/field query exercised under individual product vectors. */
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include "../starcraft/game.c"
#ifndef SC64_SHARED_DARK_SWARM_SOURCE_RECTS
#error "requires authenticated generated source rectangles"
#endif
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    int c=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(c>=0);
    screen=SCREEN_PLAY;race_mode=1;selected_building=-1;u[c].selected=1;
    sc64_defiler_stage[c].energy_q8=100u*256u; /* controlled cast setup */
    game_update(KEY_B,KEY_B);assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=field_menu_count_base();n++) {game_update(0,0);game_update(KEY_DOWN,KEY_DOWN);}
    assert(menu_cursor==field_menu_count_base());game_update(0,0);game_update(KEY_A,KEY_A);
    assert(mode==MODE_DARK_SWARM_STAGE);cx=400;cy=320;game_update(0,0);game_update(KEY_A,KEY_A);
    for(int n=0;n<40;n++)game_update(0,0);
    int field=-1;for(int i=0;i<MAX_UNITS;i++)if(sc64_defiler_fields[i].remaining && sc64_defiler_fields[i].remaining<=900)field=i;
    assert(field>=0 && !SC64_UNIT_SLOT_FREE(field));
    int marine=spawn_unit(ENEMY,U_MARINE,356,320),zergling=spawn_unit(ENEMY,U_ZERGLING,400,320);
    assert(marine>=0 && zergling>=0);
    assert(sc64_world_dark_swarm_affects_target(marine)==1 && sc64_world_dark_swarm_affects_target(zergling)==1);
    u[marine].x=355; /* adjacent GBA-representable source boundary */
    assert(sc64_world_dark_swarm_affects_target(marine)==0);
    u[marine].x=WORLD_W;assert(sc64_world_dark_swarm_affects_target(marine)==-1);
#if !defined(SC64_LATE_TERRAN_STORAGE) && !defined(SC64_ZP_CAMPAIGN_STORAGE)
    u[marine].x=INT_MAX;assert(sc64_world_dark_swarm_affects_target(marine)==-1);
#endif
    assert(!race_save_game() && !race_load_game());
    puts("actual menu field and source rectangles under product vector passed");return 0;
}
