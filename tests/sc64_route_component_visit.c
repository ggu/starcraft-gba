/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independent graph oracle for complete route-component fields with VISIT_GRID.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
#define GBA_HOT
#ifndef RACE_TEST_COLS
#define RACE_TEST_COLS 64
#define RACE_TEST_ROWS 64
#endif
enum { MAP_COLS=RACE_TEST_COLS, MAP_ROWS=RACE_TEST_ROWS, MAP_CELLS=MAP_COLS*MAP_ROWS };

#ifdef SC64_LARGE_MAPS
#define GBA_HOST
#include "starcraft/sc64_bitgrid.h"
static u8 path_walkable[(MAP_CELLS+7)/8];
#define SC64_GRID_GET(grid, cell) sc64_bitgrid_get((grid), MAP_CELLS, (cell))
#define SC64_GRID_SET(grid, cell, value) sc64_bitgrid_set((grid), MAP_CELLS, (cell), (value))
#define TEST_GRID_BYTES ((MAP_CELLS+7)/8)
#else
static u8 path_walkable[MAP_CELLS];
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_GRID_SET(grid, cell, value) ((grid)[(cell)] = (u8)((value) != 0))
#define TEST_GRID_BYTES MAP_CELLS
#endif
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid=1;
static int limit(int n,int lo,int hi){return n<lo?lo:(n>hi?hi:n);}
static int sc64_cell_row(unsigned c){return (int)c/MAP_COLS;}
static int sc64_cell_col(unsigned c){return (int)c%MAP_COLS;}
static void build_path_grid(void){path_grid_valid=1;}
#include "starcraft/sc64_routes.h"

static u8 grid_before[TEST_GRID_BYTES];
static u8 oracle_seen[MAP_CELLS];
static u16 oracle_queue[MAP_CELLS];
static unsigned rng=0x8f31a6c5u;
static unsigned random_u32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static int pixel_cell(int x,int y)
{
    int col=limit(x/16,0,MAP_COLS-1),row=limit(y/16,0,MAP_ROWS-1);
    return col+row*MAP_COLS;
}
static void grid_snapshot(void){memcpy(grid_before,path_walkable,sizeof(grid_before));}
static void grid_assert_unchanged(void)
{assert(memcmp(grid_before,path_walkable,sizeof(grid_before))==0);}
static void grid_clear(void)
{
#ifdef SC64_LARGE_MAPS
    sc64_bitgrid_clear(path_walkable,MAP_CELLS);
#else
    memset(path_walkable,0,sizeof(path_walkable));
#endif
    path_grid_valid=1;
    sc64_routes_reset();
}
static void grid_fill(int value)
{
    grid_clear();
    for(int cell=0;cell<MAP_CELLS;cell++)SC64_GRID_SET(path_walkable,cell,value);
}

/* Independent four-neighbour flood with the route API's eight-neighbour
 * blocked-goal seeds and cardinal exits from blocked starts. */
static int graph_oracle(int sx,int sy,int tx,int ty)
{
    static const s8 dx8[8]={0,1,1,1,0,-1,-1,-1};
    static const s8 dy8[8]={-1,-1,0,1,1,1,0,-1};
    static const s8 dx4[4]={1,-1,0,0};
    static const s8 dy4[4]={0,0,1,-1};
    int start=pixel_cell(sx,sy),goal=pixel_cell(tx,ty);
    int scol=start%MAP_COLS,srow=start/MAP_COLS,gcol=goal%MAP_COLS,grow=goal/MAP_COLS;
    int head=0,tail=0;
    memset(oracle_seen,0,sizeof(oracle_seen));
    if(SC64_GRID_GET(path_walkable,goal)){
        oracle_seen[goal]=1;oracle_queue[tail++]=(u16)goal;
    }else for(int d=0;d<8;d++){
        int x=gcol+dx8[d],y=grow+dy8[d],cell;
        if(x<0||y<0||x>=MAP_COLS||y>=MAP_ROWS)continue;
        cell=x+y*MAP_COLS;
        if(SC64_GRID_GET(path_walkable,cell)&&!oracle_seen[cell]){
            oracle_seen[cell]=1;oracle_queue[tail++]=(u16)cell;
        }
    }
    while(head<tail){
        int cell=oracle_queue[head++],x=cell%MAP_COLS,y=cell/MAP_COLS;
        for(int d=0;d<4;d++){
            int nx=x+dx4[d],ny=y+dy4[d],next;
            if(nx<0||ny<0||nx>=MAP_COLS||ny>=MAP_ROWS)continue;
            next=nx+ny*MAP_COLS;
            if(SC64_GRID_GET(path_walkable,next)&&!oracle_seen[next]){
                oracle_seen[next]=1;oracle_queue[tail++]=(u16)next;
            }
        }
    }
    if(SC64_GRID_GET(path_walkable,start))return oracle_seen[start]!=0;
    for(int d=0;d<4;d++){
        int x=scol+dx4[d],y=srow+dy4[d];
        if(x<0||y<0||x>=MAP_COLS||y>=MAP_ROWS)continue;
        if(oracle_seen[x+y*MAP_COLS])return 1;
    }
    return 0;
}

static void check_query_family(int sx,int sy,int slot_x,int slot_y,int tx,int ty,
                               int *hint_slots,int *hint_roots,int *hint_next)
{
    int expected_slot=graph_oracle(sx,sy,slot_x,slot_y);
    int expected_target=graph_oracle(sx,sy,tx,ty);
    int expected_pair=expected_slot|(expected_target<<1);
    int nx,ny,got;

    grid_snapshot();
    got=sc64_route_reachable(sx,sy,slot_x,slot_y);
    assert(got==expected_slot);grid_assert_unchanged();

    grid_snapshot();
    got=sc64_route_reachable_pair(sx,sy,slot_x,slot_y,tx,ty);
    assert(got==expected_pair);grid_assert_unchanged();

    grid_snapshot();
    got=sc64_route_reachable_pair_with_hint(sx,sy,slot_x,slot_y,tx,ty,
                                             hint_slots,hint_roots,hint_next,0);
    assert(got==expected_pair);grid_assert_unchanged();

    grid_snapshot();
    got=sc64_route_formation_fallback_with_hint(sx,sy,slot_x,slot_y,tx,ty,
                                                 hint_slots,hint_roots,hint_next);
    assert(got==(!expected_slot&&expected_target));grid_assert_unchanged();

    /* The ordinary route API also shares the borrowed walkability grid. */
    grid_snapshot();
    got=path_grid_valid ? sc64_route_next(sx,sy,tx,ty,&nx,&ny) : 0;
    assert(got==expected_target);grid_assert_unchanged();
}

static void reset_hints(int *slots,int *roots,int *next)
{
    for(int i=0;i<SC64_ROUTE_COMPONENT_HINTS;i++)slots[i]=roots[i]=-1;
    *next=0;
}

static void test_empty_full_and_blocked_edges(void)
{
    int hs[SC64_ROUTE_COMPONENT_HINTS],hr[SC64_ROUTE_COMPONENT_HINTS],hn;
    reset_hints(hs,hr,&hn);
    grid_clear();
    check_query_family(8*16+8,8*16+8,20*16+8,20*16+8,20*16+8,20*16+8,hs,hr,&hn);

    grid_fill(1);reset_hints(hs,hr,&hn);
    check_query_family(-1,-17,MAP_COLS*16+200,MAP_ROWS*16+100,
                       3*16+8,5*16+8,hs,hr,&hn);

    /* Blocked start exits cardinally to an open component. */
    grid_clear();reset_hints(hs,hr,&hn);
    SC64_GRID_SET(path_walkable,31+30*MAP_COLS,1);
    SC64_GRID_SET(path_walkable,32+30*MAP_COLS,1);
    check_query_family(30*16+8,30*16+8,32*16+8,30*16+8,
                       31*16+8,30*16+8,hs,hr,&hn);

    /* A diagonal-only blocked-start exit is not a valid connection. */
    grid_clear();reset_hints(hs,hr,&hn);
    SC64_GRID_SET(path_walkable,31+31*MAP_COLS,1);
    check_query_family(30*16+8,30*16+8,32*16+8,32*16+8,
                       31*16+8,31*16+8,hs,hr,&hn);

    /* A blocked goal is approached through any open eight-neighbour seed. */
    grid_clear();reset_hints(hs,hr,&hn);
    for(int x=34;x<=41;x++)SC64_GRID_SET(path_walkable,x+42*MAP_COLS,1);
    check_query_family(34*16+8,42*16+8,40*16+8,42*16+8,
                       42*16+8,42*16+8,hs,hr,&hn);

    /* A blocked goal with isolated diagonal seeds remains disconnected. */
    grid_clear();reset_hints(hs,hr,&hn);
    SC64_GRID_SET(path_walkable,35+35*MAP_COLS,1);
    SC64_GRID_SET(path_walkable,36+36*MAP_COLS,1);
    check_query_family(35*16+8,35*16+8,38*16+8,38*16+8,
                       37*16+8,37*16+8,hs,hr,&hn);
}

static void island_cell(unsigned i,int shift,int *col,int *row)
{
    *col=4+(int)(i%6u)*9+shift;
    *row=4+(int)(i/6u)*9;
}
static void build_islands(unsigned count,int shift)
{
    grid_clear();
    for(unsigned i=0;i<count;i++){
        int col,row;island_cell(i,shift,&col,&row);
        assert(col>=0&&row>=0&&col<MAP_COLS&&row<MAP_ROWS);
        SC64_GRID_SET(path_walkable,col+row*MAP_COLS,1);
    }
    path_grid_valid=1;
}
static void test_hint_reuse_eviction_and_reset(void)
{
    int hs[SC64_ROUTE_COMPONENT_HINTS],hr[SC64_ROUTE_COMPONENT_HINTS],hn;
    unsigned active=(unsigned)SC64_ROUTE_ACTIVE_SLOTS;
    unsigned count=active+8u;
    assert(active>0&&count<=64u);
    reset_hints(hs,hr,&hn);
    build_islands(count,0);
    sc64_route_hits=0;sc64_route_builds=0;

    for(unsigned i=0;i<count;i++){
        unsigned next=(i+1u)%count;int sc,sr,nc,nr;
        island_cell(i,0,&sc,&sr);island_cell(next,0,&nc,&nr);
        check_query_family(sc*16+8,sr*16+8,nc*16+8,nr*16+8,
                           sc*16+8,sr*16+8,hs,hr,&hn);
    }
    assert(sc64_route_builds>active);
    assert(hn>0);
    {
        u32 before_builds=sc64_route_builds,before_hits=sc64_route_hits;
        int sc,sr,next_col,next_row;island_cell(count-1u,0,&sc,&sr);
        island_cell(0,0,&next_col,&next_row);
        check_query_family(sc*16+8,sr*16+8,next_col*16+8,next_row*16+8,
                           sc*16+8,sr*16+8,hs,hr,&hn);
        assert(sc64_route_builds==before_builds);
        assert(sc64_route_hits>before_hits);
    }

    /* Keep stale caller-owned slot hints across cache reset and topology edit. */
    build_islands(count,1);
    for(unsigned q=0;q<count;q++){
        unsigned i=q%count,next=(i+1u)%count;int sc,sr,nc,nr;
        island_cell(i,1,&sc,&sr);island_cell(next,1,&nc,&nr);
        check_query_family(sc*16+8,sr*16+8,nc*16+8,nr*16+8,
                           sc*16+8,sr*16+8,hs,hr,&hn);
    }
}

static void test_random_shapes(void)
{
    int hs[SC64_ROUTE_COMPONENT_HINTS],hr[SC64_ROUTE_COMPONENT_HINTS],hn;
    for(unsigned map=0;map<16;map++){
        grid_clear();reset_hints(hs,hr,&hn);
        for(int cell=0;cell<MAP_CELLS;cell++)
            if((random_u32()%100u)<61u)SC64_GRID_SET(path_walkable,cell,1);
        for(unsigned q=0;q<12;q++){
            int sx=(int)(random_u32()%(unsigned)MAP_COLS)*16+8;
            int sy=(int)(random_u32()%(unsigned)MAP_ROWS)*16+8;
            int tx=(int)(random_u32()%(unsigned)MAP_COLS)*16+8;
            int ty=(int)(random_u32()%(unsigned)MAP_ROWS)*16+8;
            int sx2=(int)(random_u32()%(unsigned)MAP_COLS)*16+8;
            int sy2=(int)(random_u32()%(unsigned)MAP_ROWS)*16+8;
            check_query_family(sx,sy,tx,ty,sx2,sy2,hs,hr,&hn);
        }
    }
}

int main(void)
{
    test_empty_full_and_blocked_edges();
    test_hint_reuse_eviction_and_reset();
    test_random_shapes();
#ifdef SC64_LARGE_MAPS
    puts("packed VISIT_GRID component fields match independent graph oracle; restoration and hint tests pass");
#else
    puts("byte-grid VISIT_GRID component fields match independent graph oracle; restoration and hint tests pass");
#endif
    return 0;
}
