/* Packed RACES route-field checks against an independent four-neighbour BFS. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8;
#define GBA_HOT
#define SC64_RACES
#ifndef RACE_TEST_COLS
#define RACE_TEST_COLS 64
#define RACE_TEST_ROWS 64
#endif
enum { MAP_COLS=RACE_TEST_COLS,MAP_ROWS=RACE_TEST_ROWS,MAP_CELLS=MAP_COLS*MAP_ROWS };
static u8 path_walkable[MAP_CELLS];
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid = 1;
static int limit(int n, int lo, int hi) { return n < lo ? lo : (n > hi ? hi : n); }
static int sc64_cell_row(unsigned c) { return (int)c / MAP_COLS; }
static int sc64_cell_col(unsigned c) { return (int)c % MAP_COLS; }
static void build_path_grid(void) { path_grid_valid = 1; }
#include "starcraft/sc64_routes.h"

_Static_assert(sizeof(sc64_route_field) == (16*MAP_CELLS/2), "sixteen fields hold two cells per byte");
static int distance[MAP_CELLS];
static unsigned rng = 0x4398a12bu;
static unsigned random32(void) { rng = rng*1664525u+1013904223u; return rng; }

static void reference_bfs(int goal)
{
    int head=0,tail=0;
    for(int i=0;i<MAP_CELLS;i++) distance[i]=-1;
    if(path_walkable[goal]) { distance[goal]=0; bfs_queue[tail++]=(u16)goal; }
    while(head<tail) {
        int c=bfs_queue[head++], x=c%MAP_COLS, y=c/MAP_COLS;
        int n[4]={x?c-1:-1,x+1<MAP_COLS?c+1:-1,y?c-MAP_COLS:-1,y+1<MAP_ROWS?c+MAP_COLS:-1};
        for(int i=0;i<4;i++) if(n[i]>=0&&path_walkable[n[i]]&&distance[n[i]]<0) {
            distance[n[i]]=distance[c]+1; bfs_queue[tail++]=(u16)n[i];
        }
    }
}

static void verify_field(int goal)
{
    int nx,ny;
    reference_bfs(goal);
    sc64_routes_reset();
    for(int c=0;c<MAP_CELLS;c++) if(path_walkable[c]) {
        int x=c%MAP_COLS,y=c/MAP_COLS,ok=sc64_route_next(x*16+8,y*16+8,
            (goal%MAP_COLS)*16+8,(goal/MAP_COLS)*16+8,&nx,&ny);
        if(distance[c]<0) assert(!ok);
        else if(c==goal) assert(ok&&nx==(goal%MAP_COLS)*16+8&&ny==(goal/MAP_COLS)*16+8);
        else {
            assert(ok);
            int next=nx/16+(ny/16)*MAP_COLS;
            assert(next>=0&&next<MAP_CELLS&&path_walkable[next]);
            assert(distance[next]+1==distance[c]);
        }
    }
}

int main(void)
{
    for(int map=0;map<4;map++) {
        for(int c=0;c<MAP_CELLS;c++) path_walkable[c]=(random32()%100)<72;
        int goal=(map*733+1777)%MAP_CELLS;
        path_walkable[goal]=1;
        verify_field(goal);
    }
    /* A worker may be spawned inside the inflated footprint of a building;
       it must exit toward a shortest reachable path without distance storage. */
    memset(path_walkable,1,sizeof(path_walkable));
    path_walkable[32+32*MAP_COLS]=0;
    sc64_routes_reset();
    int nx,ny;
    assert(sc64_route_next(32*16+8,32*16+8,40*16+8,40*16+8,&nx,&ny));
    int next=nx/16+(ny/16)*MAP_COLS;
    reference_bfs(40+40*MAP_COLS);
    assert(distance[next]==15); /* one step out plus 14 remaining */
    /* Sixteen slots retain a mixed economy/army working set and count hits. */
    sc64_routes_reset(); sc64_route_builds=0; sc64_route_hits=0;
    for(int g=0;g<16;g++) assert(sc64_route_next(8*16+8,8*16+8,(g+16)*16+8,16*16+8,&nx,&ny));
    for(int g=0;g<16;g++) assert(sc64_route_next(8*16+8,8*16+8,(g+16)*16+8,16*16+8,&nx,&ny));
    assert(sc64_route_builds==16&&sc64_route_hits==16);
    puts("packed race routes match BFS; blocked-start and 16-slot cache pass");
    return 0;
}
