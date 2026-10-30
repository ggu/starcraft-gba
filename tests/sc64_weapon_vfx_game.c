/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual attack events feed cosmetic effects; rendering never applies damage. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void clear_world(void)
{
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); memset(tracers,0,sizeof(tracers));
    memset(visible,0xff,sizeof(visible)); camx=camy=0;
    weapons_level=armor_level=0; supply_used=0;
}

static void test_profiles(void)
{
    unsigned armed=0;
    for(int type=0;type<SC_RU_COUNT;++type) for(int air=0;air<2;++air) {
        if(!sc64_race_unit_damage_vs(type,air) ||
            (air&&!sc64_race_unit_can_attack_air(type))) continue;
        int profile=sc64_weapon_vfx_unit_profile(type,air);
        if(!profile) fprintf(stderr,"Missing VFX type=%d air=%d\n",type,air);
        assert(profile && sc64_weapon_vfx_style(profile)>0); ++armed;
    }
    assert(armed>=20);
    assert(sc64_weapon_vfx_unit_profile(U_MEDIC,0)==0);
    assert(sc64_weapon_vfx_unit_profile(U_OVERLORD,0)==0);
    int dual[]={U_GOLIATH,U_WRAITH,U_SCOUT};
    for(unsigned i=0;i<sizeof(dual)/sizeof(*dual);++i)
        assert(sc64_weapon_vfx_unit_profile(dual[i],0)!=sc64_weapon_vfx_unit_profile(dual[i],1));
    int defenses[]={B_PHOTON_CANNON,B_SUNKEN_COLONY,B_SPORE_COLONY,B_MISSILE_TURRET};
    for(unsigned i=0;i<sizeof(defenses)/sizeof(*defenses);++i)
        assert(sc64_weapon_vfx_building_profile(defenses[i]));
    printf("PASS %u armed unit/target-class profiles\n",armed);
}

static void test_actual_shot(int type,int owner,int air)
{
    clear_world();
    int shooter=spawn_unit(owner,type,64,48);
    /* Siege Mode cannot fire inside its stock minimum range. Keep every
     * real shot fixture inside that weapon's valid range interval. */
    int target=spawn_unit(owner==SIDE?ENEMY:SIDE,air?U_MUTALISK:U_ZEALOT,
                         type==U_SIEGE_TANK?104:68,48);
    assert(shooter>=0 && target>=0);
    int health=u[target].hp+u[target].shield;
    tick_combat(&u[shooter],target);
    assert(u[target].hp+u[target].shield<health);
    assert(tracers[0].color==sc64_weapon_vfx_unit_profile(type,air));
    int duration=sc64_weapon_vfx_lifetime(sc64_weapon_vfx_style(tracers[0].color));
    assert(tracers[0].life==duration+1);
    for(int life=duration;life>0;--life) {
        update_tracers(); assert(tracers[0].life==life);
        gfx_begin(C_PANEL); draw_tracers();
        unsigned changed=0;
        for(int y=0;y<160;++y) for(int x=0;x<240;++x) {
            int pixel=gfx_test_pixel(x,y);
            if(y<12||y>119) assert(pixel==C_PANEL);
            else changed+=pixel!=C_PANEL;
        }
        assert(changed>0);
        assert(u[target].hp+u[target].shield<health);
    }
    int after=u[target].hp+u[target].shield;
    update_tracers(); assert(!tracers[0].life);
    assert(u[target].hp+u[target].shield==after);
}

static void test_fog_camera_and_overflow(void)
{
    clear_world();
    int profile=sc64_weapon_vfx_unit_profile(U_DRAGOON,0);
    add_tracer(64,48,144,48,profile); update_tracers();
    memset(visible,0,sizeof(visible)); gfx_begin(C_PANEL); draw_tracers();
    for(int y=0;y<160;++y) for(int x=0;x<240;++x) assert(gfx_test_pixel(x,y)==C_PANEL);
    memset(visible,0xff,sizeof(visible));
    camx=32; camy=16; gfx_begin(C_PANEL); draw_tracers();
    assert(gfx_test_pixel(32,44)==C_WHITE);
    camx=camy=0; memset(tracers,0,sizeof(tracers));
    for(int i=0;i<MAX_TRACERS;++i) add_tracer(i,40,i+4,40,profile);
    update_tracers();
    add_tracer(100,60,120,60,profile);
    unsigned fresh=0;
    for(int i=0;i<MAX_TRACERS;++i) fresh+=tracers[i].x0==100&&tracers[i].y0==60;
    assert(fresh==1);
}

static void dump_gallery(void)
{
    const char *out=getenv("SC64_VFX_GALLERY");
    if(!out) return;
    clear_world();
    for(int tick=0;tick<8;++tick) {
        gfx_begin(C_PANEL);
        for(int style=1;style<=12;++style) {
            int col=(style-1)%3,row=(style-1)/3;
            int x=col*80+16,y=row*28+25;
            gfx_text(col*80+2,y-10,"VFX",C_DIM); gfx_num(col*80+20,y-10,style,C_DIM);
            gfx_rect(x-3,y-3,5,5,C_TERRAN); gfx_rect(x+42,y-3,5,5,C_ZERG);
            int duration=sc64_weapon_vfx_lifetime(style);
            int life=duration-tick;
            sc64_weapon_vfx_draw(style,style==SC64_WEAPON_VFX_STYLE_ACID?C_GREEN:C_CYAN,
                x,y,x+44,y,life);
        }
        char path[1024]; snprintf(path,sizeof(path),"%s/frame-%02d.ppm",out,tick);
        gfx_end(); gfx_dump_ppm(path);
    }
}

int main(void)
{
    gba_init(); game_init();
    race_mode=1; race_human=R_ZERG; race_bots=1;
    race_skirmish_init(); screen=SCREEN_PLAY;
    /* Visual metadata never grows a tracer; race builds keep 16-bit endpoints. */
    _Static_assert(sizeof(Tracer)==4*sizeof(s16)+2*sizeof(int),"visual metadata keeps tracer layout");
    test_profiles();
    int types[]={U_MARINE,U_FIREBAT,U_HYDRALISK,U_HUNTER_KILLER,U_ZERGLING,
        U_ZEALOT,U_DRAGOON,U_GOLIATH,U_WRAITH,U_SCOUT,U_TANK,U_SIEGE_TANK,
        U_VULTURE,U_MUTALISK,U_REAVER,U_TASSADAR,U_SCV,U_DRONE,U_PROBE};
    for(unsigned i=0;i<sizeof(types)/sizeof(*types);++i) {
        test_actual_shot(types[i],SIDE,0); test_actual_shot(types[i],ENEMY,0);
    }
    test_actual_shot(U_GOLIATH,SIDE,1); test_actual_shot(U_WRAITH,SIDE,1);
    test_actual_shot(U_SCOUT,SIDE,1); test_actual_shot(U_MUTALISK,SIDE,1);
    test_fog_camera_and_overflow(); dump_gallery();
    puts("PASS actual ground/air shots, impact lifetimes, HUD/fog/camera clipping, pool overflow and immediate-damage contract");
    return 0;
}
