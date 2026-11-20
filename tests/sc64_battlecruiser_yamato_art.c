/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
static u8 expected[240*160];
static void capture(void) {for(int y=0;y<160;++y)for(int x=0;x<240;++x)expected[y*240+x]=gfx_test_pixel(x,y);}
static void compare(void) {for(int y=0;y<160;++y)for(int x=0;x<240;++x)assert(expected[y*240+x]==gfx_test_pixel(x,y));}

static void draw_effects(int caster)
{
#ifdef SC64_WORLD_DEPTH
    draw_yamato_charge(&u[caster]);
#else
    (void)caster;
#endif
    draw_tracers();
}

#ifdef SC64_WORLD_DEPTH
static void check_charge_order(int caster, int foreground, unsigned charge_frame)
{
    Unit *ship = &u[caster];
    Sc64BcYamatoShot shot = sc64_bc_shots[0];
    unsigned work_timer = ship->work_timer;
    sc64_bc_shots[0].state = 0;
    ship->work_timer &= ~7u;

    gfx_begin(C_PANEL);
    draw_unit(ship);
    sc64_yamato_raster_draw(543, charge_frame, 0, ship->x, ship->y);
    draw_unit(&u[foreground]);
    capture();

    gfx_begin(C_PANEL);
    draw_unit(ship);
    draw_unit(&u[foreground]);
    sc64_yamato_raster_draw(543, charge_frame, 0, ship->x, ship->y);
    unsigned distinguishing = 0;
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            distinguishing += expected[y * 240 + x] != gfx_test_pixel(x, y);
    assert(distinguishing > 0);

    sc64_bc_shots[0] = shot;
    ship->work_timer = work_timer;
    gfx_begin(C_PANEL);
    sc64_draw_depth_ordered_actors();
    draw_tracers_pass(1);
    compare();
    printf("Yamato charge frame %u matches %u distinguishing pixels\n", charge_frame, distinguishing);
}

static void check_charge_view_edges(int caster)
{
    Unit *ship = &u[caster];
    const int cameras[][2] = {{ship->x + 16, 0}, {0, ship->y + 6}};
    ship->selected = 1;
    ship->work_timer = 2;
    for (unsigned edge = 0; edge < sizeof(cameras) / sizeof(cameras[0]); ++edge) {
        camx = cameras[edge][0];
        camy = cameras[edge][1];
        assert(!sc64_unit_body_anchor_in_view(ship, ship->x - camx, ship->y - camy + 12));
        gfx_begin(C_PANEL);
        sc64_yamato_raster_draw(543, 3, 0, ship->x, ship->y);
        capture();
        unsigned clipped = 0;
        for (int y = 0; y < 160; ++y)
            for (int x = 0; x < 240; ++x) {
                unsigned changed = expected[y * 240 + x] != C_PANEL;
                if (y < 12 || y >= 120) assert(!changed);
                clipped += changed;
            }
        assert(clipped > 0);
        gfx_begin(C_PANEL);
        sc64_draw_depth_ordered_actors();
        draw_tracers_pass(1);
        compare();
        printf("Offscreen Yamato charge edge %u matches %u clipped pixels\n", edge, clipped);
    }
}

static void check_yamato_charge_depth(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    geyser_count = 0;
    clear_tracers();
    int caster = spawn_unit(ENEMY, unit_raw(12), 80, 60);
    int foreground = spawn_unit(SIDE, unit_raw(8), 80, 60);
    assert(caster >= 0 && foreground >= 0);
    assert(sc64_unit_world_layer(&u[caster]) < sc64_unit_world_layer(&u[foreground]));

    sc64_bc_shots[0] = (Sc64BcYamatoShot){.state = SC64_BC_SHOT_CHARGING, .caster = caster};
    check_charge_order(caster, foreground, 4);

    sc64_bc_shots[0].state = 0;
    u[caster].order = ORDER_ATTACK_TARGET;
    u[caster].work_timer = 6;
    check_charge_order(caster, foreground, 1);

    u[foreground].alive = 0;
    check_charge_view_edges(caster);
}
#endif

int main(void) {
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();camx=camy=0;
    memset(visible,0xff,sizeof(visible));clear_tracers();
    int a=spawn_unit(ENEMY,unit_raw(12),80,60),t=spawn_unit(SIDE,unit_raw(0),200,60);
    assert(a>=0 && t>=0);u[a].work_timer=64<<3;u[a].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));if(!sc64_battlecruiser_source_tick_due())++frame;
    assert(race_bc_yamato_tick_caster(a));
    gfx_begin(C_PANEL);sc64_yamato_raster_draw(543,4,0,80,60);capture();
    gfx_begin(C_PANEL);draw_effects(a);compare();
    for(int n=0;n<55;++n){++frame;assert(race_bc_yamato_tick_caster(a));}
    #ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    assert(sc64_bc_shots[0].state==SC64_BC_SHOT_FLIGHT);
    #else
    assert(race_bc_yamato_launch_resolved(0,64,0,400,120));
    #endif
    for(int n=0;n<5;++n)assert(race_bc_yamato_projectile_step(0));
    Sc64BcMotionFields m;assert(sc64_bc_motion_unpack(&sc64_bc_shots[0],&m));
    int x=(int)(m.x_q8/512u),y=(int)(m.y_q8/512u),h=(int)(((unsigned)m.heading+4u)>>3)&31;
    gfx_begin(C_PANEL);
#ifndef SC64_WORLD_DEPTH
    sc64_yamato_raster_draw(541,0,h,x,y);
#endif
    sc64_yamato_raster_draw(543,1,0,80,60);
#ifdef SC64_WORLD_DEPTH
    sc64_yamato_raster_draw(541,0,h,x,y);
#endif
    sc64_yamato_raster_draw(542,0,h,x,y);capture();
    gfx_begin(C_PANEL);draw_effects(a);compare();
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
    gfx_begin(C_PANEL);draw_effects(a);compare();
#ifdef SC64_WORLD_DEPTH
    check_yamato_charge_depth();
#endif
    puts("private Yamato charge, moving projectile, trail and impact raster integration pass");
}
