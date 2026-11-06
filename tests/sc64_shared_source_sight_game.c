/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Source-radius optical visibility, memory and owner-query oracle. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#if !defined(SC64_SOURCE_SIGHT_ALL_RACES) || !defined(SC64_RACES)
#error "requires shared-source sight and races"
#endif
static void reset_sight(void)
{
    memset(u,0,sizeof u); memset(b,0,sizeof b);
    SC64_GRID_CLEAR(explored); SC64_GRID_CLEAR(visible); sc64_visibility_reset();
}
static void check_single_circle(int x,int y,int radius)
{
    for (int row=0;row<MAP_ROWS;row++) for(int col=0;col<MAP_COLS;col++) {
        int cell=col+row*MAP_COLS,dx=col*16+8-x,dy=row*16+8-y;
        int expected=dx*dx+dy*dy<=radius*radius;
        assert(!!SC64_GRID_GET(visible,cell)==expected);
        if(expected)assert(SC64_GRID_GET(explored,cell));
    }
}
static void unit_test(int raw,int radius)
{
    reset_sight(); int type=sc64_race_unit_from_raw(raw); if(type<0) { fprintf(stderr,"unmapped unit %d\n",raw); return; }
    u[0].alive=1;u[0].owner=SIDE;u[0].type=type;u[0].raw_type=raw;u[0].x=520;u[0].y=520;
    assert(sc64_actor_unit_sight_pixels(type)==radius*2);
#ifdef SC64_P7_OBSERVER_ENABLED
    assert(sc64_p7_observer_sight_radius_runtime(type,0)==radius);
#endif
    sc64_update_exploration();check_single_circle(520,520,radius);
    u[0].x+=3;u[0].y+=5;sc64_update_exploration();check_single_circle(523,525,radius);
    u[0].alive=0;sc64_update_exploration();
    assert(!SC64_GRID_GET(visible,32+32*MAP_COLS));
    assert(SC64_GRID_GET(explored,32+32*MAP_COLS));
}
static void building_test(int raw,int radius)
{
    reset_sight();int type=sc64_race_building_from_raw(raw);if(type<0) { fprintf(stderr,"unmapped building %d\n",raw); return; }
    b[0].alive=1;b[0].owner=SIDE;b[0].type=type;b[0].raw_type=raw;b[0].x=520;b[0].y=520;
    b[0].under_construction=1;sc64_update_exploration();check_single_circle(520,520,64);
    b[0].under_construction=0;sc64_update_exploration();check_single_circle(520,520,radius);
#ifdef SC64_P7_OBSERVER_ENABLED
    assert(sc64_p7_observer_sight_radius_runtime(type,1)==radius);
#endif
}
static void mixed_test(void)
{
    static const int unit_raw[]={7,41,64,84,12}, unit_radius[]={112,112,128,144,176};
    static const int build_raw[]={154,162,160}, build_radius[]={176,176,64};
    reset_sight();
    for(int i=0;i<5;i++) {
        int type=sc64_race_unit_from_raw(unit_raw[i]); if(type<0)continue;
        u[i].alive=1;u[i].owner=i==4?ENEMY:SIDE;u[i].type=type;u[i].raw_type=unit_raw[i];
        u[i].x=40+i*137;u[i].y=40+i*91;
    }
    for(int i=0;i<3;i++) {
        int type=sc64_race_building_from_raw(build_raw[i]); assert(type>=0);
        b[i].alive=1;b[i].owner=SIDE;b[i].type=type;b[i].raw_type=build_raw[i];
        b[i].x=80+i*201;b[i].y=640-i*137;b[i].under_construction=i==2;
    }
    for(int step=0;step<4;step++) {
        sc64_update_exploration();
        for(int row=0;row<MAP_ROWS;row++)for(int col=0;col<MAP_COLS;col++) {
            int cell=col+row*MAP_COLS,x=col*16+8,y=row*16+8,seen=0;
            for(int i=0;i<5;i++)if(u[i].alive&&u[i].owner==SIDE) {
                int dx=x-u[i].x,dy=y-u[i].y;seen|=dx*dx+dy*dy<=unit_radius[i]*unit_radius[i];
            }
            for(int i=0;i<3;i++)if(b[i].alive&&b[i].owner==SIDE) {
                int dx=x-b[i].x,dy=y-b[i].y;seen|=dx*dx+dy*dy<=build_radius[i]*build_radius[i];
            }
            assert(!!SC64_GRID_GET(visible,cell)==seen);
            if(seen)assert(SC64_GRID_GET(explored,cell));
        }
        u[0].x+=3;u[1].y+=7;u[2].x+=16;u[3].alive=step!=1;
        if(step==2)b[1].alive=0;
    }
}
int main(void)
{
    gba_init();game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;race_skirmish_map=0;init_state();
    unit_test(7,112);unit_test(41,112);unit_test(64,128);unit_test(65,112);unit_test(84,144);unit_test(12,176);
    building_test(154,176);building_test(156,128);building_test(160,160);building_test(162,176);building_test(106,160);building_test(131,144);
    mixed_test();
    reset_sight();b[0].alive=1;b[0].owner=ENEMY;b[0].type=B_NEXUS;b[0].raw_type=154;b[0].x=b[0].y=520;
#ifdef SC64_P7_OBSERVER_ENABLED
    int c=(520+168)/16,r=520/16;
    assert(sc64_p7_observer_owner_sees_cell(1,ENEMY,c+r*MAP_COLS));
    assert(!sc64_p7_observer_owner_sees_cell(2,ENEMY,c+r*MAP_COLS));
    b[0].under_construction=1;
    assert(!sc64_p7_observer_owner_sees_cell(1,ENEMY,c+r*MAP_COLS));
#else
    b[0].under_construction=1;
#endif
    assert(sc64_shared_building_visibility_key(&b[0])==-(B_NEXUS+1));
    b[0].type=B_LAIR;assert(sc64_shared_building_visibility_key(&b[0])==B_HATCHERY);
    b[0].type=B_HIVE;assert(sc64_shared_building_visibility_key(&b[0])==B_LAIR);
    b[0].type=B_GREATER_SPIRE;assert(sc64_shared_building_visibility_key(&b[0])==B_SPIRE);
    puts("shared source sight circles, explored superset, owners and morphs: ok");return 0;
}
