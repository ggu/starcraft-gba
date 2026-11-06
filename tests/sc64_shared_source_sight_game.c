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
    u[0].alive=1;u[0].owner=SIDE;u[0].type=type;u[0].raw_type=raw;u[0].x=523;u[0].y=525;
    assert(sc64_actor_unit_sight_pixels(type)==radius*2);
#ifdef SC64_P7_OBSERVER_ENABLED
    assert(sc64_p7_observer_sight_radius_runtime(type,0)==radius);
#endif
    sc64_update_exploration();check_single_circle(523,525,radius);
    for(int cell=0;cell<MAP_COLS*MAP_ROWS;cell++)
        assert(!!SC64_GRID_GET(visible,cell)==!!SC64_GRID_GET(explored,cell));
    u[0].x+=3;u[0].y+=5;sc64_update_exploration();check_single_circle(526,530,radius);
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
static void all_mapped_stock_sight(void)
{
    for (int raw=0;raw<228;raw++) {
        int t=sc64_race_unit_from_raw(raw);
        if(t>=0)assert(sc64_actor_unit_sight_pixels(t)==sc64_source_sight_raw_pixels(raw));
        t=sc64_race_building_from_raw(raw);
        if(t>=0)assert(sc64_actor_building_sight_pixels(t)==sc64_source_sight_raw_pixels(raw));
    }
#ifdef SC64_Z9_RUNTIME
    assert(sc64_actor_building_sight_pixels(B_ZL_KHAYDARIN_FORMATION)==320);
#endif
}
#if defined(SC64_P7_OBSERVER_ENABLED) && defined(SC64_P10_RUNTIME)
static void p10_owner_detection(void)
{
    int map[8];u8 powered[MAX_BUILDINGS]={0};
    race_mode=0;race_selected_mission=SC64_P10_GLOBAL_ID;
    assert(sc64_p7_optical_human_slot()==1);
    for(unsigned i=0;i<8;i++)map[i]=race_protoss_source_owner(i);
    assert(map[1]==SIDE && map[7]>=0 && map[7]!=SIDE);
    assert(sc64_p7_optical_source_mask(-1,map[7],map,1)&(1u<<7));
    reset_sight();
    u[0].alive=1;u[0].type=U_MARINE;u[0].raw_type=0;u[0].owner=map[0];u[0].raw_owner=0;u[0].x=u[0].y=520;
    u[1].alive=1;u[1].type=U_P7_OBSERVER;u[1].raw_type=84;u[1].owner=SIDE;u[1].raw_owner=1;u[1].x=u[1].y=520;
    u[2]=u[1];u[2].owner=map[7];u[2].raw_owner=7;
    int cell=32+32*MAP_COLS;
    assert(!sc64_p7_observer_owner_sees_cell(1,SIDE,cell));
    assert(sc64_p7_observer_owner_sees_cell(7,map[7],cell));
    assert(sc64_p7_arbiter_detected_mask(0,powered)==(1u<<7));
    Sc64P7DetectorLists lists;sc64_p7_detector_lists_build(&lists);
    assert(sc64_p7_arbiter_detected_mask_lists(0,powered,&lists)==(1u<<7));
    SC64_GRID_SET(visible,cell,1);
    assert(sc64_p7_observer_owner_sees_cell(1,SIDE,cell));
    assert(sc64_p7_arbiter_detected_mask(0,powered)==((1u<<1)|(1u<<7)));
    sc64_p7_detector_lists_build(&lists);
    assert(sc64_p7_arbiter_detected_mask_lists(0,powered,&lists)==((1u<<1)|(1u<<7)));
    u[2].raw_owner=-1;sc64_p7_detector_lists_build(&lists);
    assert(sc64_p7_arbiter_detected_mask_lists(0,powered,&lists)&(1u<<7));
    assert(!sc64_p7_observer_owner_sees_cell(1,ENEMY,cell));
    /* Captured/rescued detectors retain origin but operate for current SIDE. */
    u[2].alive=0;u[1].raw_owner=0;
    assert(sc64_p7_arbiter_detected_mask(0,powered)==(1u<<1));
    sc64_p7_detector_lists_build(&lists);
    assert(sc64_p7_arbiter_detected_mask_lists(0,powered,&lists)==(1u<<1));
    assert(u[1].raw_owner==0);
    u[1].raw_owner=-1;assert(sc64_p7_arbiter_detected_mask(0,powered)==(1u<<1));
    u[1].type=U_OVERLORD;u[1].raw_type=42;u[1].raw_owner=0;
    assert(sc64_p7_arbiter_detected_mask(0,powered)==(1u<<1));
    u[1].alive=0;
    b[0].alive=1;b[0].type=B_PHOTON_CANNON;b[0].raw_type=162;b[0].owner=SIDE;b[0].raw_owner=0;b[0].x=b[0].y=520;
    b[1].alive=1;b[1].type=B_PYLON;b[1].raw_type=156;b[1].owner=SIDE;b[1].raw_owner=0;b[1].x=b[1].y=520;
    assert(sc64_p7_observer_pylon_powers(0,1,SIDE));
    powered[0]=1;assert(sc64_p7_arbiter_detected_mask(0,powered)==(1u<<1));
    b[0].type=B_SPORE_COLONY;b[0].raw_type=144;b[0].raw_owner=-1;
    assert(sc64_p7_arbiter_detected_mask(0,powered)==(1u<<1));
}
#endif
static void save_roundtrip(void)
{
    static u8 expected_visible[sizeof visible], expected_explored[sizeof explored];
    game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;race_skirmish_map=0;init_state();
    sc64_update_exploration();
    memcpy(expected_visible,visible,sizeof visible);memcpy(expected_explored,explored,sizeof explored);
    assert(save_game());
    SC64_GRID_CLEAR(visible);SC64_GRID_CLEAR(explored);sc64_visibility_reset();
    assert(load_game());
    assert(memcmp(expected_visible,visible,sizeof visible)==0);
    assert(memcmp(expected_explored,explored,sizeof explored)==0);
}
int main(void)
{
    gba_init();game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;race_skirmish_map=0;init_state();
    unit_test(7,112);unit_test(41,112);unit_test(64,128);unit_test(65,112);unit_test(84,144);unit_test(12,176);
    building_test(154,176);building_test(156,128);building_test(160,160);building_test(162,176);building_test(106,160);building_test(131,144);
    mixed_test();all_mapped_stock_sight();save_roundtrip();
#if defined(SC64_P7_OBSERVER_ENABLED) && defined(SC64_P10_RUNTIME)
    p10_owner_detection();
#endif
    race_mode=1;
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
