/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main previous_defiler_test
#include "sc64_defiler_executor_game.c"
#undef main
#ifndef SC64_SHARED_DEFILER_APPROACH_PROBE
#error "requires disabled clear-approach probe"
#endif
int main(void)
{
    const int dx[5]={200,0,-150,0,140},dy[5]={0,200,0,-150,140};
    for(int k=0;k<5;k++) {
        game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
        /* Synthetic empty, clear map; no gameplay actors are injected into the cast. */
        memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
        int c=spawn_unit(SIDE,U_SHARED_DEFILER,160,160);assert(c>=0);
        test_phase=0;click_cast(c,160+dx[k],160+dy[k]);
        int t=1;
        while(t<=60) {
            Sc64DefilerStageActor *a=&sc64_defiler_stage[c];
            int x=a->motion_age?sc64_interceptor_integer_part(a->x_q8):u[c].x*2;
            int y=a->motion_age?sc64_interceptor_integer_part(a->y_q8):u[c].y*2;
            printf("case=%d t=%d x=%d y=%d heading=%d phase=%d blocked=%d qx=%d qy=%d\n",k,t,x,y,(int)a->heading,a->phase,a->blocked,(int)a->x_q8,(int)a->y_q8);
            game_update(0,0);if(sc64_battlecruiser_source_tick_due())++t;
        }
#ifdef SC64_SHARED_DEFILER_SE_APPROACH_PROBE
        assert(!sc64_defiler_stage[c].blocked);
#else
        if(k==4)assert(sc64_defiler_stage[c].blocked);
        else assert(!sc64_defiler_stage[c].blocked);
#endif
    }
    /* STOP during a real clear-path approach cancels the cast and movement. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
    int walking=spawn_unit(SIDE,U_SHARED_DEFILER,160,160);assert(walking>=0);
    test_phase=0;click_cast(walking,360,160);
    for(int n=0;n<12;n++)game_update(0,0);
    assert(u[walking].x>160 && sc64_defiler_stage[walking].intent);
    stop_selected();int stopped_x=u[walking].x,stopped_y=u[walking].y;
    for(int n=0;n<30;n++)game_update(0,0);
    assert(!sc64_defiler_stage[walking].intent && u[walking].x==stopped_x &&
           u[walking].y==stopped_y && sc64_defiler_stage[walking].energy_q8>=100u*256u &&
           !sc64_shared_projectiles.active);
    /* A blocked terrain corridor stays pending without walking or paying. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
    SC64_GRID_SET(path_walkable,12+10*MAP_COLS,0);
    int c=spawn_unit(SIDE,U_SHARED_DEFILER,160,160);assert(c>=0);
    test_phase=0;click_cast(c,360,160);
    for(int n=0;n<40;n++)game_update(0,0);
    assert(sc64_defiler_stage[c].blocked && u[c].x==160 && u[c].y==160 &&
           sc64_defiler_stage[c].energy_q8>=100u*256u && !sc64_shared_projectiles.active);
    stop_selected();assert(!sc64_defiler_stage[c].intent);
    return 0;
}
