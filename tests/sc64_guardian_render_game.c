/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored fake bank; actual game draw path, no guest SRAM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
static int changed(void)
{
    int n=0;for(int y=12;y<120;y++)for(int x=0;x<240;x++)n+=gfx_test_pixel(x,y)!=20;
    return n;
}
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();
    int slot=spawn_unit(SIDE,U_MUTALISK,100,38);assert(slot>=0);
    Unit *q=&u[slot];q->type=U_GUARDIAN_COCOON;q->raw_type=59;
    q->target=44;q->hp=120;q->maxhp=200;q->work_timer=750;q->armor=0;
    camx=camy=0;for(int i=0;i<256;i++)fake_lut[i]=(u8)(i+1);
    for(int page=0;page<2;page++) {
        gfx_begin(20);gfx_set_clip_y(12,120);q->selected=0;
        draw_unit_pass(q,0);
        assert(gfx_test_pixel(116,49)==3 && gfx_test_pixel(116,50)==3);
        assert(gfx_test_pixel(116,51)==21 && gfx_test_pixel(118,51)==20);
        gfx_end();
    }
    gfx_begin(20);gfx_set_clip_y(12,120);q->selected=1;
    draw_unit_pass(q,1);assert(changed()>0);
    /* Frame width 2 gives the clamped radius 3, not the fallback radius 5. */
    assert(gfx_test_pixel(103,50)==C_GREEN && gfx_test_pixel(105,50)==20);
    gfx_begin(20);gfx_set_clip_y(12,120);q->selected=1;
    draw_unit_pass(q,0);assert(gfx_test_pixel(116,49)==3 && changed()>4); /* body + vitals */
    gfx_begin(20);gfx_set_clip_y(50,51);q->selected=0;
    draw_unit_pass(q,0);assert(gfx_test_pixel(116,49)==20 && gfx_test_pixel(116,50)==3);
    assert(gfx_test_pixel(116,51)==20);
    gfx_begin(20);gfx_set_clip_y(12,120);q->owner=ENEMY;memset(visible,0,sizeof(visible));
    draw_unit_pass(q,0);assert(changed()==0); /* hidden enemy */
    SC64_GRID_SET(visible,6+2*MAP_COLS,1); /* actor visible, raster tile hidden */
    draw_unit_pass(q,0);assert(changed()==0);
    SC64_GRID_SET(visible,7+2*MAP_COLS,1);
    draw_unit_pass(q,0);assert(gfx_test_pixel(116,49)==3);
    gfx_begin(20);gfx_set_clip_y(12,120);q->owner=SIDE;camx=109;
    draw_unit_pass(q,0);assert(gfx_test_pixel(7,49)==3); /* center is offscreen */
    camx=0;gfx_begin(20);gfx_set_clip_y(12,120);
    cocoon_bank.shadow_lut=0;draw_unit_pass(q,0);assert(changed()==0);
    q->selected=1;draw_unit_pass(q,1);assert(changed()==0);q->selected=0;
    cocoon_bank.shadow_lut=fake_lut;q->work_timer=751;draw_unit_pass(q,0);assert(changed()==0);
    q->work_timer=750;screen=SCREEN_PLAY;game_render();
    puts("Guardian fake-bank actual game render passed");return 0;
}
