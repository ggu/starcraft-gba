/* SPDX-License-Identifier: GPL-3.0-or-later
 * Asset-free checks for packed range-seeded attack approach routes.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8;
#define GBA_HOT
#define SC64_RACES
#define SC64_ROUTE_PACK3
#define SC64_ATTACK_RANGE_ROUTES
#define SC64_REACHABLE_FORMATION
#define SC64_IMPORTED_MISSION
enum { MAP_COLS=24, MAP_ROWS=20, MAP_CELLS=MAP_COLS*MAP_ROWS };
static u8 path_walkable[MAP_CELLS];
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid=1;
static int limit(int n,int lo,int hi){return n<lo?lo:(n>hi?hi:n);}
static int sc64_cell_row(unsigned c){return (int)c/MAP_COLS;}
static int sc64_cell_col(unsigned c){return (int)c%MAP_COLS;}
static void build_path_grid(void){path_grid_valid=1;}
#include "../starcraft/sc64_routes.h"

/* Independent distance oracle: multi-source BFS over four-way walkable cells. */
static int oracle[MAP_CELLS], queue[MAP_CELLS];
static int oracle_range(int tx,int ty,int lo,int hi)
{
    int head=0,tail=0;
    for(int i=0;i<MAP_CELLS;i++) oracle[i]=-1;
    for(int y=0;y<MAP_ROWS;y++) for(int x=0;x<MAP_COLS;x++) {
        int dx=x*16+8-tx,dy=y*16+8-ty,d2=dx*dx+dy*dy,c=x+y*MAP_COLS;
        if(path_walkable[c]&&d2>=lo*lo&&d2<=hi*hi) {
            oracle[c]=0; queue[tail++]=c;
        }
    }
    while(head<tail) {
        int c=queue[head++],x=c%MAP_COLS,y=c/MAP_COLS;
        const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
        for(int d=0;d<4;d++) {
            int nx=x+dx[d],ny=y+dy[d],n=nx+ny*MAP_COLS;
            if(nx>=0&&ny>=0&&nx<MAP_COLS&&ny<MAP_ROWS&&path_walkable[n]&&oracle[n]<0) {
                oracle[n]=oracle[c]+1;queue[tail++]=n;
            }
        }
    }
    return tail;
}
static void clear_map(void)
{
    memset(path_walkable,1,sizeof(path_walkable));
    path_grid_valid=1;sc64_routes_reset();
}
static int query(int x,int y,int tx,int ty,int lo,int hi,int *nx,int *ny)
{
#ifdef SC64_ROUTE_VISIT_GRID
    u8 before[sizeof(path_walkable)];memcpy(before,path_walkable,sizeof(before));
    int result=sc64_route_range_next(x,y,tx,ty,lo,hi,nx,ny);
    assert(memcmp(before,path_walkable,sizeof(before))==0);
    return result;
#else
    return sc64_route_range_next(x,y,tx,ty,lo,hi,nx,ny);
#endif
}
static void assert_step(int x,int y,int tx,int ty,int lo,int hi)
{
    int nx=-1,ny=-1,cur=x/16+(y/16)*MAP_COLS,next;
    assert(oracle_range(tx,ty,lo,hi)>0);
    assert(query(x,y,tx,ty,lo,hi,&nx,&ny));
    assert(nx>=8&&ny>=8&&nx<MAP_COLS*16&&ny<MAP_ROWS*16);
    next=nx/16+(ny/16)*MAP_COLS;
    assert(path_walkable[next]);
    if(!path_walkable[cur]) {
        assert(abs(nx/16-x/16)+abs(ny/16-y/16)==1);
        assert(oracle[next]>=0);
    } else {
        assert(abs(nx/16-x/16)+abs(ny/16-y/16)==1);
        assert(oracle[next]==oracle[cur]-1);
    }
}
static void assert_descent(int x,int y,int tx,int ty,int lo,int hi)
{
    int cell=x/16+(y/16)*MAP_COLS;
    assert(oracle_range(tx,ty,lo,hi)>0 && oracle[cell]>=0);
    for(int steps=oracle[cell];steps>0;steps--) {
        int nx,ny,next;
        assert(query(x,y,tx,ty,lo,hi,&nx,&ny));
        next=nx/16+(ny/16)*MAP_COLS;
        assert(path_walkable[next]&&oracle[next]==steps-1);
        x=nx;y=ny;cell=next;
    }
    assert(query(x,y,tx,ty,lo,hi,&x,&y));
    cell=x/16+(y/16)*MAP_COLS;
    {
        int dx=x-tx,dy=y-ty,d2=dx*dx+dy*dy;
        assert(path_walkable[cell]&&d2>=lo*lo&&d2<=hi*hi);
    }
}
int main(void)
{
    int nx,ny;u32 builds,hits;u32 rng=0x6d2b79f5u;
    clear_map();
    /* Isolate the target tile in a blocked 3x3 ring; point routing fails,
       while a reachable outer firing band succeeds. */
    for(int y=9;y<=11;y++)for(int x=11;x<=13;x++)path_walkable[x+y*MAP_COLS]=0;
    path_walkable[12+10*MAP_COLS]=1;
    assert(!sc64_route_next(4*16+8,10*16+8,12*16+8,10*16+8,&nx,&ny));
    assert(oracle_range(12*16+8,10*16+8,32,64)>0);
    assert_step(4*16+8,10*16+8,12*16+8,10*16+8,32,64);
    assert_descent(4*16+8,10*16+8,12*16+8,10*16+8,32,64);
    builds=sc64_route_builds;hits=sc64_route_hits;
    assert(query(4*16+8,10*16+8,12*16+8,10*16+8,32,64,&nx,&ny));
    assert(sc64_route_builds==builds&&sc64_route_hits==hits+1);
    /* Exact target subtiles and each range bound participate in the key. */
    assert(query(4*16+8,10*16+8,12*16+9,10*16+8,32,64,&nx,&ny));
    assert(sc64_route_builds==builds+1);builds=sc64_route_builds;
    assert(query(4*16+8,10*16+8,12*16+9,10*16+8,33,64,&nx,&ny));
    assert(sc64_route_builds==builds+1);builds=sc64_route_builds;
    assert(query(4*16+8,10*16+8,12*16+9,10*16+8,33,65,&nx,&ny));
    assert(sc64_route_builds==builds+1);builds=sc64_route_builds;
    /* The reserved range field does not replace a normal point cache slot. */
    assert(sc64_route_slot(3+3*MAP_COLS)>=0);
    assert(sc64_route_valid[SC64_ROUTE_SLOTS-1]==3);
    assert(sc64_route_slot(3+3*MAP_COLS)>=0);
    assert(sc64_route_valid[SC64_ROUTE_SLOTS-1]==3);
    /* Seeds exist, but the attacker's disconnected component cannot reach
       any firing tile. */
    clear_map();
    memset(path_walkable,0,sizeof(path_walkable));
    path_walkable[2+2*MAP_COLS]=1;
    for(int y=10;y<MAP_ROWS;y++)for(int x=15;x<MAP_COLS;x++)path_walkable[x+y*MAP_COLS]=1;
    assert(oracle_range(18*16+8,15*16+8,32,48)>0);
    assert(!query(2*16+8,2*16+8,18*16+8,15*16+8,32,48,&nx,&ny));
    /* Blocked starts may step out through a reachable adjacent gradient. */
    clear_map();path_walkable[5+5*MAP_COLS]=0;
    assert(oracle_range(15*16+8,5*16+8,16,48)>0);
    assert_step(5*16+8,5*16+8,15*16+8,5*16+8,16,48);
    /* Topology reset must invalidate the cached field. */
    builds=sc64_route_builds;clear_map();
    assert(query(2*16+8,10*16+8,18*16+8,10*16+8,16,32,&nx,&ny));
    assert(sc64_route_builds==builds+1);
    /* Target at map edge, with a non-center coordinate: the seed is still a
       tile center in the exact target-coordinate annulus. */
    clear_map();
    assert(oracle_range(0,0,8,24)>0);
    assert_descent(8,8,0,0,8,24);
    /* Deterministic randomized obstacle layouts: compare reachability and
       each chosen next step against the independent BFS oracle. */
    for(int trial=0;trial<120;trial++) {
        int tx,ty,sx,sy,lo=16+(trial%3)*8,hi=48+(trial%4)*16;
        clear_map();
        for(int c=0;c<MAP_CELLS;c++) {
            rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
            path_walkable[c]=(u8)((rng%100u)<73u);
        }
        tx=(trial%MAP_COLS)*16+(trial%16);ty=((trial*7)%MAP_ROWS)*16+((trial*3)%16);
        sx=((trial*11)%MAP_COLS)*16+8;sy=((trial*13)%MAP_ROWS)*16+8;
        int seeds=oracle_range(tx,ty,lo,hi),start=sx/16+(sy/16)*MAP_COLS;
        int expected=seeds>0 && oracle[start]>=0;
        if(seeds>0&&!path_walkable[start]) {
            int x=start%MAP_COLS,y=start/MAP_COLS;
            const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
            for(int d=0;d<4;d++) {
                int ax=x+dx[d],ay=y+dy[d];
                if(ax>=0&&ay>=0&&ax<MAP_COLS&&ay<MAP_ROWS&&oracle[ax+ay*MAP_COLS]>=0)expected=1;
            }
        }
        int actual=query(sx,sy,tx,ty,lo,hi,&nx,&ny);
        if(!expected) assert(!actual);
        else {
            assert(actual);
            assert(nx>=8&&ny>=8&&nx<MAP_COLS*16&&ny<MAP_ROWS*16);
            int next=nx/16+(ny/16)*MAP_COLS;
            assert(path_walkable[next]);
            if(path_walkable[start]) {
                assert(oracle[start]>=0);
                if(oracle[start]==0) assert(next==start);
                else assert(oracle[next]==oracle[start]-1);
            }
            else assert(oracle[next]>=0);
        }
    }
    /* Reject malformed target/range inputs without touching a null output. */
    assert(!query(0,0,-1,0,0,1,&nx,&ny));
    assert(!query(0,0,0,MAP_ROWS*16,0,1,&nx,&ny));
    assert(!query(0,0,0,0,-1,1,&nx,&ny));
    assert(!query(0,0,0,0,2,1,&nx,&ny));
    assert(!query(0,0,0,0,0,4097,&nx,&ny));
    assert(!query(0,0,0,0,0,1,0,&ny));
#if !defined(SC64_ROUTE_PARTIAL_FLOOD) && !defined(SC64_ROUTE_VISIT_GRID)
    /* Fill the reserved field with range seeds in both components. Formation
       connectivity must ignore its valid==3 marker and report only the
       component reached from the left start. */
    clear_map();memset(path_walkable,0,sizeof(path_walkable));
    for(int x=2;x<=5;x++)path_walkable[x+10*MAP_COLS]=1;
    for(int x=18;x<=21;x++)path_walkable[x+10*MAP_COLS]=1;
    {
        int starts[]={2+10*MAP_COLS},goals[]={5+10*MAP_COLS};
        int other[]={18+10*MAP_COLS},hs[SC64_ROUTE_COMPONENT_HINTS];
        int hr[SC64_ROUTE_COMPONENT_HINTS],hn=0;
        for(int i=0;i<SC64_ROUTE_COMPONENT_HINTS;i++)hs[i]=hr[i]=-1;
        assert(query(2*16+8,10*16+8,12*16+8,10*16+8,80,160,&nx,&ny));
        assert(sc64_route_valid[SC64_ROUTE_SLOTS-1]==3);
        hs[0]=SC64_ROUTE_SLOTS-1;hr[0]=12+10*MAP_COLS;
        assert(sc64_route_cells_connected(starts,1,goals,1,other,1,hs,hr,&hn,0)==1);
        assert(sc64_route_valid[SC64_ROUTE_SLOTS-1]==3);
    }
#endif
    puts("SC64 attack-range route checks passed");
    return 0;
}
