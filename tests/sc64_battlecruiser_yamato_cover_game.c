/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Host-seeded test with separately validated CV5 cover overlay. */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
#ifndef SC64_CV5_COVER
#error "requires cover-encoded terrain inputs"
#endif
int main(void) {
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;init_state();
    race_mode=0;race_selected_mission=0;race_cols=sc64_race_maps[0].width;race_rows=sc64_race_maps[0].height;
    int cell=-1;for(int i=0;i<MAP_COLS*MAP_ROWS;++i)if ((sc64_race_maps[0].blocked[i]&3)==2){cell=i;break;}
    assert(cell>=0);int x=(cell%MAP_COLS)*16+8,y=(cell/MAP_COLS)*16+8;
    assert(terrain_provides_cover(x,y) && !terrain_blocked(x/16,y/16));
    int a=spawn_unit(ENEMY,unit_raw(12),x,y),t=spawn_unit(SIDE,unit_raw(0),x,y);
    assert(a>=0 && t>=0);u[a].raw_owner=0;u[a].work_timer=64<<3;u[a].carry=200*256;
    sc64_battlecruiser_tech_bits=SC64_BC_TECH_YAMATO;
    assert(race_bc_yamato_issue(a,t)); if(!sc64_battlecruiser_source_tick_due())++frame;
    assert(race_bc_yamato_tick_caster(a));for(int n=0;n<55;++n){++frame;assert(race_bc_yamato_tick_caster(a));}
    sc64_bc_spell_rng=199u; /* next pinned LCG result has low byte 50 */
    assert(race_bc_yamato_launch_world(0,64));
    Sc64BcMotionFields m;assert(sc64_bc_motion_unpack(&sc64_bc_shots[0],&m));
    assert((sc64_bc_shots[0].phase&64u) && m.destination_x==x*2-30 && m.destination_y==y*2);
    /* The same RNG sample hits uncovered ground and a flyer over cover. */
    int clear=-1;for(int i=0;i<MAP_COLS*MAP_ROWS;++i)if ((sc64_race_maps[0].blocked[i]&3)==0){clear=i;break;}
    assert(clear>=0);int ux=(clear%MAP_COLS)*16+8,uy=(clear/MAP_COLS)*16+8;
    sc64_bc_shot_clear(&sc64_bc_shots[0]);u[a].work_timer&=~7;u[a].build=-1;u[a].work_timer=64<<3;u[a].carry=200*256;
    u[a].x=u[t].x=ux;u[a].y=u[t].y=uy;
    assert(!terrain_provides_cover(ux,uy) && race_bc_yamato_issue(a,t));
    if(!sc64_battlecruiser_source_tick_due())++frame;assert(race_bc_yamato_tick_caster(a));
    for(int n=0;n<55;++n){++frame;assert(race_bc_yamato_tick_caster(a));}
    sc64_bc_spell_rng=199u;assert(race_bc_yamato_launch_world_cover(0,64,0));
    assert(!(sc64_bc_shots[0].phase&64u));
    sc64_bc_shot_clear(&sc64_bc_shots[0]);u[a].work_timer&=~7;u[a].build=-1;u[a].work_timer=64<<3;u[a].carry=200*256;
    u[a].x=x;u[a].y=y;int flyer=spawn_unit(SIDE,unit_raw(12),x,y);assert(flyer>=0);
    assert(race_bc_yamato_issue(a,flyer));if(!sc64_battlecruiser_source_tick_due())++frame;
    assert(race_bc_yamato_tick_caster(a));for(int n=0;n<55;++n){++frame;assert(race_bc_yamato_tick_caster(a));}
    sc64_bc_spell_rng=199u;assert(race_bc_yamato_launch_world_cover(0,64,0));
    assert(!(sc64_bc_shots[0].phase&64u));
    puts("private CV5 cover miss and uncovered/flying hits pass");
}
