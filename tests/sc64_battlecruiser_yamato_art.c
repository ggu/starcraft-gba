/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
static u8 expected[240*160];
static void capture(void) {for(int y=0;y<160;++y)for(int x=0;x<240;++x)expected[y*240+x]=gfx_test_pixel(x,y);}
static void compare(void) {for(int y=0;y<160;++y)for(int x=0;x<240;++x)assert(expected[y*240+x]==gfx_test_pixel(x,y));}
int main(void) {
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();camx=camy=0;
    memset(visible,0xff,sizeof(visible));clear_tracers();
    int a=spawn_unit(ENEMY,unit_raw(12),80,60),t=spawn_unit(SIDE,unit_raw(0),200,60);
    assert(a>=0 && t>=0);u[a].work_timer=64<<3;u[a].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));if(!sc64_battlecruiser_source_tick_due())++frame;
    assert(race_bc_yamato_tick_caster(a));
    gfx_begin(C_PANEL);sc64_yamato_raster_draw(543,4,0,80,60);capture();
    gfx_begin(C_PANEL);draw_tracers();compare();
    for(int n=0;n<55;++n){++frame;assert(race_bc_yamato_tick_caster(a));}
    #ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    assert(sc64_bc_shots[0].state==SC64_BC_SHOT_FLIGHT);
    #else
    assert(race_bc_yamato_launch_resolved(0,64,0,400,120));
    #endif
    for(int n=0;n<5;++n)assert(race_bc_yamato_projectile_step(0));
    Sc64BcMotionFields m;assert(sc64_bc_motion_unpack(&sc64_bc_shots[0],&m));
    int x=(int)(m.x_q8/512u),y=(int)(m.y_q8/512u),h=(int)(((unsigned)m.heading+4u)>>3)&31;
    gfx_begin(C_PANEL);sc64_yamato_raster_draw(541,0,h,x,y);
    sc64_yamato_raster_draw(543,1,0,80,60);sc64_yamato_raster_draw(542,0,h,x,y);capture();
    gfx_begin(C_PANEL);draw_tracers();compare();
    /* Advance to the impact and isolate its first frame. */
    for(int n=0;n<30 && sc64_bc_shots[0].state;++n) {
        assert(sc64_bc_motion_unpack(&sc64_bc_shots[0],&m));Sc64BcMotionFields next=m;
        int hit=sc64_yamato_motion_step(&next,(unsigned)WORLD_W*2,(unsigned)WORLD_H*2);
        if(hit)clear_tracers();
        assert(race_bc_yamato_projectile_step(0));
        if(hit){x=(int)(next.x_q8/512u);y=(int)(next.y_q8/512u);break;}
    }
    assert(!sc64_bc_shots[0].state && !u[t].alive);
    gfx_begin(C_PANEL);sc64_yamato_raster_draw(543,1,0,80,60);sc64_yamato_raster_draw(544,0,0,x,y);capture();
    gfx_begin(C_PANEL);draw_tracers();compare();
    puts("private Yamato charge, moving projectile, trail and impact raster integration pass");
}
