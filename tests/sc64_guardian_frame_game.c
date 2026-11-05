/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST world, real game_update/render and guest-format journal. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
extern void guardian_test_reset(void);
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();
    assert(screen==SCREEN_PLAY);
    int slot=spawn_unit(SIDE,U_MUTALISK,50,50);assert(slot>=0);
    int spire=spawn_build(SIDE,B_GREATER_SPIRE,100,100,0);assert(spire>=0);
    minerals=500;gas=500;u[slot].selected=1;
    assert(race_guardian_morph_start(slot));
    int frames=0;game_render();
    for (;frames<375;frames++) {game_update(0,0);if ((frames%60)==0) game_render();}
    assert(screen==SCREEN_PLAY && u[slot].alive && u[slot].type==U_GUARDIAN_COCOON && u[slot].work_timer==375);
    guardian_test_reset();assert(race_save_game());assert(race_load_game());
    assert(u[slot].alive && u[slot].type==U_GUARDIAN_COCOON && u[slot].work_timer==375);
    for (;frames<750;frames++) {game_update(0,0);if ((frames%60)==0) game_render();}
    assert(screen==SCREEN_PLAY && u[slot].alive && u[slot].type==race_guardian_product_type());
    assert(u[slot].raw_type==44 && u[slot].work_timer==0);
    game_render();guardian_test_reset();assert(race_save_game());assert(race_load_game());
    assert(u[slot].alive && u[slot].type==race_guardian_product_type());
    puts("Guardian full HOST frame and render continuation passed");return 0;
}
