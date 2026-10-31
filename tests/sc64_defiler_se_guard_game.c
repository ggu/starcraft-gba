/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main previous_defiler_test
#include "sc64_defiler_executor_game.c"
#undef main
#ifndef SC64_SHARED_DEFILER_SE_APPROACH_PROBE
#error "requires the disabled southeast approach probe"
#endif
int main(void)
{
    const int sx[5]={160,160,160,161,160},sy[5]={160,160,160,160,160};
    const int tx[5]={20,20,300,301,385},ty[5]={300,20,20,300,385};
    for(int k=0;k<5;k++) {
        game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
        memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
        int c=spawn_unit(SIDE,U_SHARED_DEFILER,sx[k],sy[k]);assert(c>=0);
        test_phase=0;click_cast(c,tx[k],ty[k]);
        for(int n=0;n<30;n++)game_update(0,0);
        assert(sc64_defiler_stage[c].blocked && u[c].x==sx[k] && u[c].y==sy[k]);
        assert(sc64_defiler_stage[c].energy_q8>=100u*256u && !sc64_shared_projectiles.active);
    }
#if defined(SC64_SHARED_DEFILER_HERO_PROBE) && !defined(SC64_SHARED_DEFILER_SE_HERO_PROBE)
    /* Hero movement is not covered unless its independent probe is enabled. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
    int hero=spawn_unit(SIDE,U_SHARED_UNCLEAN_ONE,160,160);assert(hero>=0);
    test_phase=0;click_cast(hero,300,300);
    assert(sc64_defiler_stage[hero].blocked && u[hero].x==160 && u[hero].y==160);
#endif
    /* A mismatched explicit raw identity must not inherit raw-46 motion. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
    int alias=spawn_unit(SIDE,U_SHARED_DEFILER,160,160);assert(alias>=0);
    u[alias].raw_type=52; /* deliberately malformed legacy alias */
    test_phase=0;click_cast(alias,240,160); /* in-range path also rejects */
    assert(!sc64_defiler_stage[alias].intent && u[alias].x==160 && u[alias].y==160);
    game_update(0,0);click_cast(alias,300,300);
    assert(!sc64_defiler_stage[alias].intent && u[alias].x==160 && u[alias].y==160);
    /* A non-source-tested initial simulation heading fails closed. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
    int rotated=spawn_unit(SIDE,U_SHARED_DEFILER,160,160);assert(rotated>=0);
    screen=SCREEN_PLAY;selected_building=-1;u[rotated].selected=1;
    sc64_defiler_stage[rotated].energy_q8=100u*256u;
    game_update(KEY_B,KEY_B);assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=field_menu_count_base();n++) {game_update(0,0);game_update(KEY_DOWN,KEY_DOWN);}
    assert(menu_cursor==field_menu_count_base());game_update(0,0);game_update(KEY_A,KEY_A);
    assert(mode==MODE_DARK_SWARM_STAGE);cx=300;cy=300;game_update(0,0);
    sc64_defiler_stage[rotated].heading=0;frame=0;game_update(KEY_A,KEY_A);
    assert(sc64_defiler_stage[rotated].blocked && u[rotated].x==160 && u[rotated].y==160);
    /* Real menu STOP interrupts a supported diagonal while moving. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
    int c=spawn_unit(SIDE,U_SHARED_DEFILER,160,160);assert(c>=0);
    test_phase=0;click_cast(c,300,300);
    for(int n=0;n<12;n++)game_update(0,0);
    assert(u[c].x>160 && u[c].y>160 && sc64_defiler_stage[c].intent);
    game_update(0,0);game_update(KEY_B,KEY_B);assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=MENU_STOP;n++) {game_update(0,0);game_update(KEY_DOWN,KEY_DOWN);}
    assert(menu_cursor==MENU_STOP);game_update(0,0);game_update(KEY_A,KEY_A);
    int x=u[c].x,y=u[c].y;
    for(int n=0;n<25;n++)game_update(0,0);
    assert(!sc64_defiler_stage[c].intent && u[c].x==x && u[c].y==y &&
           sc64_defiler_stage[c].energy_q8>=100u*256u && !sc64_shared_projectiles.active);
#ifdef SC64_SHARED_DEFILER_SE_HERO_PROBE
    /* The separately gated hero path also obeys real menu STOP. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
    int hero_stop=spawn_unit(SIDE,U_SHARED_UNCLEAN_ONE,160,160);assert(hero_stop>=0);
    test_phase=0;click_cast(hero_stop,300,300);
    for(int n=0;n<12;n++)game_update(0,0);
    assert(u[hero_stop].x>160 && u[hero_stop].y>160 && sc64_defiler_stage[hero_stop].intent);
    game_update(0,0);game_update(KEY_B,KEY_B);assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=MENU_STOP;n++) {game_update(0,0);game_update(KEY_DOWN,KEY_DOWN);}
    assert(menu_cursor==MENU_STOP);game_update(0,0);game_update(KEY_A,KEY_A);
    int hx=u[hero_stop].x,hy=u[hero_stop].y;
    for(int n=0;n<25;n++)game_update(0,0);
    assert(!sc64_defiler_stage[hero_stop].intent && u[hero_stop].x==hx && u[hero_stop].y==hy &&
           sc64_defiler_stage[hero_stop].energy_q8>=100u*256u && !sc64_shared_projectiles.active);
#endif
    puts("unsupported diagonal starts fail closed without movement or debit");return 0;
}
