/* SPDX-License-Identifier: GPL-3.0-or-later
 * All-roster proof for P7 optical sight axis-cull bounds. */
#include <assert.h>
#include <limits.h>
#include "../starcraft/game.c"

static int same_result(int dx,int dy,int radius,int bound)
{
    int original=dx*dx+dy*dy<=radius*radius;
    int candidate=0;
    if(dx>=-bound&&dx<=bound&&dy>=-bound&&dy<=bound)
        candidate=dx*dx+dy*dy<=radius*radius;
    return original==candidate;
}
int main(void)
{
    int actor_max=0,actor_max_type=-1,building_max=0,building_max_type=-1;
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    u[0].alive=1; u[0].owner=ENEMY; u[0].raw_owner=0;
    const int invalid[]={INT_MIN,INT_MAX,-1,SC_RU_COUNT,SC_RB_COUNT};
    for(int type=0;type<SC_RU_COUNT;++type) {
        int r=sc64_p7_observer_sight_radius_runtime(type,0);
        assert(r>=0&&r<=176);
        if(r>actor_max){actor_max=r;actor_max_type=type;}
        int ds[]={-177,-176,-r-1,-r,-1,0,1,r,r+1,176,177};
        int cx=(MAP_COLS/2)*16+8,cy=(MAP_ROWS/2)*16+8;
        for(unsigned x=0;x<sizeof(ds)/sizeof(ds[0]);++x)
            for(unsigned y=0;y<sizeof(ds)/sizeof(ds[0]);++y) {
                int dx=ds[x],dy=ds[y];
                assert(cx+dx>=0&&cx+dx<MAP_COLS*16);
                assert(cy+dy>=0&&cy+dy<MAP_ROWS*16);
                assert(same_result(dx,dy,r,SC64_P7_SIGHT_UNIT_AXIS_BOUND));
                u[0].type=type; u[0].x=cx+dx; u[0].y=cy+dy;
                assert(sc64_p7_observer_owner_sees_cell(0,ENEMY,
                    cx/16+(cy/16)*MAP_COLS)==(dx*dx+dy*dy<=r*r));
            }
    }
    u[0].alive=0;
    b[0].alive=1; b[0].owner=ENEMY; b[0].raw_owner=0;
    for(int type=0;type<SC_RB_COUNT;++type) {
        int r=sc64_p7_observer_sight_radius_runtime(type,1);
        assert(r>=0&&r<=160);
        if(r>building_max){building_max=r;building_max_type=type;}
        int ds[]={-161,-160,-r-1,-r,-1,0,1,r,r+1,160,161};
        int cx=(MAP_COLS/2)*16+8,cy=(MAP_ROWS/2)*16+8;
        for(unsigned x=0;x<sizeof(ds)/sizeof(ds[0]);++x)
            for(unsigned y=0;y<sizeof(ds)/sizeof(ds[0]);++y) {
                int dx=ds[x],dy=ds[y];
                assert(cx+dx>=0&&cx+dx<MAP_COLS*16);
                assert(cy+dy>=0&&cy+dy<MAP_ROWS*16);
                assert(same_result(dx,dy,r,SC64_P7_SIGHT_BUILDING_AXIS_BOUND));
                b[0].type=type; b[0].x=cx+dx; b[0].y=cy+dy;
                assert(sc64_p7_observer_owner_sees_cell(0,ENEMY,
                    cx/16+(cy/16)*MAP_COLS)==(dx*dx+dy*dy<=r*r));
            }
    }
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i) {
        int au=sc64_p7_observer_sight_radius_runtime(invalid[i],0);
        int bu=sc64_p7_observer_sight_radius_runtime(invalid[i],1);
        assert(au>=0&&au<=176&&bu>=0&&bu<=160);
        assert(same_result(176,0,au,176));
        assert(same_result(177,0,au,176));
        assert(same_result(160,0,bu,160));
        assert(same_result(161,0,bu,160));
    }
    assert(actor_max==176&&building_max==160);
    assert(same_result(176,0,176,176));
    assert(same_result(160,0,160,160));
    assert(same_result(176,176,176,176));
    assert(same_result(160,160,160,160));
    (void)actor_max_type;(void)building_max_type;
    return 0;
}
