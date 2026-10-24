/* Packed RACES route-field checks against an independent four-neighbour BFS. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8;
#define GBA_HOT
#define SC64_RACES
#ifndef RACE_TEST_COLS
#define RACE_TEST_COLS 64
#define RACE_TEST_ROWS 64
#endif
enum { MAP_COLS=RACE_TEST_COLS,MAP_ROWS=RACE_TEST_ROWS,MAP_CELLS=MAP_COLS*MAP_ROWS };
#ifdef SC64_T4_RUNTIME
_Static_assert(MAP_COLS==128 && MAP_ROWS==128 && MAP_CELLS==16384,
               "T4 route fixture uses the full 128x128 grid");
_Static_assert(((MAP_CELLS+7)/8)==2048, "T4 fog/path grids use 2048 bytes");
#endif
static u8 path_walkable[MAP_CELLS];
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid = 1;
static int limit(int n, int lo, int hi) { return n < lo ? lo : (n > hi ? hi : n); }
static int sc64_cell_row(unsigned c) { return (int)c / MAP_COLS; }
static int sc64_cell_col(unsigned c) { return (int)c % MAP_COLS; }
static void build_path_grid(void) { path_grid_valid = 1; }
#ifdef SC64_LARGE_MAPS
#define GBA_HOST
#include "starcraft/sc64_bitgrid.h"
static u8 packed_walkable[(MAP_CELLS + 7) / 8];
#define path_walkable packed_walkable
#define SC64_GRID_GET(grid, cell) sc64_bitgrid_get((grid), MAP_CELLS, (cell))
#define SC64_GRID_SET(grid, cell, value) sc64_bitgrid_set((grid), MAP_CELLS, (cell), (value))
#endif
#include "starcraft/sc64_routes.h"
#ifdef SC64_ROUTE_VISIT_GRID
static int route_next_with_grid_snapshot(int x,int y,int tx,int ty,int *nx,int *ny)
{
#ifdef SC64_LARGE_MAPS
    u8 before[sizeof(packed_walkable)];
    memcpy(before,packed_walkable,sizeof(packed_walkable));
    int result=sc64_route_next(x,y,tx,ty,nx,ny);
    assert(memcmp(before,packed_walkable,sizeof(packed_walkable))==0);
#else
    u8 before[sizeof(path_walkable)];
    memcpy(before,path_walkable,sizeof(path_walkable));
    int result=sc64_route_next(x,y,tx,ty,nx,ny);
    assert(memcmp(before,path_walkable,sizeof(path_walkable))==0);
#endif
    return result;
}
#define sc64_route_next route_next_with_grid_snapshot
#endif
#ifdef SC64_LARGE_MAPS
#undef path_walkable
#undef SC64_GRID_GET
#undef SC64_GRID_SET
#endif

#ifdef SC64_ROUTE_PACK3
_Static_assert(sizeof(sc64_route_field) == (SC64_ROUTE_SLOTS*SC64_ROUTE_BYTES), "3-bit route fields fit packed byte storage");
#ifdef SC64_T4_RUNTIME
_Static_assert(SC64_ROUTE_BYTES==6144 && sizeof(sc64_route_field)==86016,
               "T4 uses fourteen 6144-byte route fields");
#endif
#else
_Static_assert(sizeof(sc64_route_field) == (SC64_ROUTE_SLOTS*MAP_CELLS/2), "route fields hold two cells per byte");
#endif
#ifdef SC64_ROUTE_PACK3
#if defined(SC64_LARGE_MAPS)
#if defined(SC64_T4_RUNTIME)
_Static_assert(SC64_ROUTE_SLOTS==14, "T4 128x128 PACK3 cache capacity");
#else
_Static_assert(SC64_ROUTE_SLOTS==20, "large PACK3 race cache capacity");
#endif
#elif defined(SC64_T3_ENABLED)
_Static_assert(SC64_ROUTE_SLOTS==22, "T3 PACK3 cache capacity");
#else
_Static_assert(SC64_ROUTE_SLOTS==24, "standard PACK3 race cache capacity");
#endif
#else
#if defined(SC64_LARGE_MAPS)
_Static_assert(SC64_ROUTE_SLOTS==15, "large race cache capacity");
#elif defined(SC64_T3_ENABLED)
_Static_assert(SC64_ROUTE_SLOTS==17, "T3 cache capacity");
#else
_Static_assert(SC64_ROUTE_SLOTS==18, "standard race cache capacity");
#endif
#endif
static int cached_goal(int goal)
{
    for(int i=0;i<SC64_ROUTE_SLOTS;i++)
        if(sc64_route_valid[i]&&sc64_route_goal[i]==goal)return 1;
    return 0;
}
static void reset_routes(void)
{
#ifdef SC64_LARGE_MAPS
    sc64_bitgrid_pack(packed_walkable, path_walkable, MAP_CELLS);
#endif
    sc64_routes_reset();
}
static int distance[MAP_CELLS];
static u8 reference_direction[MAP_CELLS];
static unsigned rng = 0x4398a12bu;
static unsigned random32(void) { rng = rng*1664525u+1013904223u; return rng; }

static void reference_bfs(int goal)
{
    int head=0,tail=0;
    for(int i=0;i<MAP_CELLS;i++) { distance[i]=-1; reference_direction[i]=15; }
    if(path_walkable[goal]) { distance[goal]=0; bfs_queue[tail++]=(u16)goal; }
    else {
        int gx=goal%MAP_COLS,gy=goal/MAP_COLS;
        /* A blocked destination seeds its walkable eight-neighborhood with
           direction zero; a query there returns the requested goal directly. */
        for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++) {
            int x=gx+dx,y=gy+dy,cell;
            if((!dx&&!dy)||x<0||y<0||x>=MAP_COLS||y>=MAP_ROWS)continue;
            cell=x+y*MAP_COLS;
            if(path_walkable[cell]&&distance[cell]<0) {
                distance[cell]=0;reference_direction[cell]=0;bfs_queue[tail++]=(u16)cell;
            }
        }
    }
    while(head<tail) {
        int c=bfs_queue[head++], x=c%MAP_COLS, y=c/MAP_COLS;
        /* Preserve the route flood's stable left/right/up/down tie order. */
        int n[4]={x?c-1:-1,x+1<MAP_COLS?c+1:-1,y?c-MAP_COLS:-1,y+1<MAP_ROWS?c+MAP_COLS:-1};
        int toward[4]={1,2,3,4};
        for(int i=0;i<4;i++) if(n[i]>=0&&path_walkable[n[i]]&&distance[n[i]]<0) {
            distance[n[i]]=distance[c]+1; reference_direction[n[i]]=(u8)toward[i];
            bfs_queue[tail++]=(u16)n[i];
        }
    }
}

static int check_cached_route(int sx,int sy,int gx,int gy)
{
    int start=sx+sy*MAP_COLS,goal=gx+gy*MAP_COLS,nx,ny,expected=-1;
    reference_bfs(goal);
    if(path_walkable[start]) {
        if(distance[start]>=0) {
            if(start==goal||reference_direction[start]==0)expected=goal;
            else expected=start+(reference_direction[start]==1?1:
                                reference_direction[start]==2?-1:
                                reference_direction[start]==3?MAP_COLS:
                                reference_direction[start]==4?-MAP_COLS:0);
        }
    } else {
        static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
        int best=0x7fffffff;
        for(int i=0;i<4;i++) {
            int x=sx+dx[i],y=sy+dy[i],cell,d;
            if(x<0||y<0||x>=MAP_COLS||y>=MAP_ROWS)continue;
            cell=x+y*MAP_COLS;
            if(distance[cell]<0)continue;
            d=distance[cell];
            if(d<best){best=d;expected=cell;}
        }
    }
    int ok=sc64_route_next(sx*16+8,sy*16+8,gx*16+8,gy*16+8,&nx,&ny);
#ifdef SC64_ROUTE_VISIT_GRID
    {
        int slot=-1;
        for(int i=0;i<SC64_ROUTE_SLOTS;i++)
            if(sc64_route_valid[i]&&sc64_route_goal[i]==goal)slot=i;
        assert(slot>=0);
        for(int cell=0;cell<MAP_CELLS;cell++) {
            int expected_direction=distance[cell]<0 ? SC64_ROUTE_UNREACHABLE :
                                   cell==goal ? 0 : reference_direction[cell];
            assert(sc64_route_get(slot,cell)==expected_direction);
        }
    }
#endif
    if(expected<0)return !ok;
    if(!ok)return 0;
    if(expected==goal)return nx==gx*16+8&&ny==gy*16+8;
    return nx/16+(ny/16)*MAP_COLS==expected;
}

static void verify_field(int goal)
{
    int nx,ny;
    reference_bfs(goal);
    reset_routes();
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
#ifdef SC64_ROUTE_VISIT_GRID
    {
        int slot=-1;
        for(int i=0;i<SC64_ROUTE_SLOTS;i++)
            if(sc64_route_valid[i]&&sc64_route_goal[i]==goal)slot=i;
        assert(slot>=0);
        for(int cell=0;cell<MAP_CELLS;cell++) {
            int expected=distance[cell]<0 ? SC64_ROUTE_UNREACHABLE :
                         cell==goal ? 0 : reference_direction[cell];
            int actual=sc64_route_get(slot,cell);
            if(actual!=expected) {
                fprintf(stderr,"route field mismatch goal=%d cell=%d dist=%d expected=%d actual=%d\\n",
                        goal,cell,distance[cell],expected,actual);
                abort();
            }
        }
    }
#endif
}

int main(void)
{
#ifdef SC64_ROUTE_PACK3
    /* Exhaust every cell's bit offset, including cross-byte fields. */
    memset(sc64_route_field[0],255,SC64_ROUTE_BYTES);
    for(int cell=0;cell<MAP_CELLS;cell++)
        sc64_route_put(0,cell,(cell*5+3)&7);
    for(int cell=0;cell<MAP_CELLS;cell++)
        assert(sc64_route_get(0,cell)==((cell*5+3)&7));
#endif
#ifdef SC64_T4_RUNTIME
    memset(path_walkable,1,sizeof(path_walkable));
    reset_routes();
    int edge_x,edge_y,edge_goal=MAP_CELLS-1;
    assert(sc64_route_next(8,8,(MAP_COLS-1)*16+8,(MAP_ROWS-1)*16+8,
                           &edge_x,&edge_y));
    int edge_slot=-1;
    for(int slot=0;slot<SC64_ROUTE_SLOTS;slot++)
        if(sc64_route_valid[slot]&&sc64_route_goal[slot]==edge_goal)edge_slot=slot;
    assert(edge_slot>=0);
    assert(sc64_route_get(edge_slot,edge_goal)==0);
    assert(sc64_route_get(edge_slot,0)!=SC64_ROUTE_UNREACHABLE);
    assert(edge_x/16+edge_y/16*MAP_COLS==1 ||
           edge_x/16+edge_y/16*MAP_COLS==MAP_COLS);
#endif
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
    reset_routes();
    int nx,ny;
    assert(sc64_route_next(32*16+8,32*16+8,40*16+8,40*16+8,&nx,&ny));
    int next=nx/16+(ny/16)*MAP_COLS;
    reference_bfs(40+40*MAP_COLS);
    assert(distance[next]==15); /* one step out plus 14 remaining */
    assert(check_cached_route(32,32,40,40)); /* open-grid equal-path tie */

    /* Sparse exits, corner bounds, blocked-goal zero seeds, and an isolated
       blocked origin all exercise the out-of-line PACK3 recovery path. */
    memset(path_walkable,1,sizeof(path_walkable));
    int ox=MAP_COLS/2,oy=MAP_ROWS/2;
    path_walkable[ox+oy*MAP_COLS]=0;
    path_walkable[(ox+1)+oy*MAP_COLS]=0;
    path_walkable[ox+(oy-1)*MAP_COLS]=0;
    reset_routes();
    assert(check_cached_route(ox,oy,ox-8,oy+8));
    memset(path_walkable,1,sizeof(path_walkable));
    path_walkable[0]=0;
    reset_routes();
    assert(check_cached_route(0,0,6,6));
    memset(path_walkable,1,sizeof(path_walkable));
    int blocked_goal_x=ox+6,blocked_goal_y=oy-4;
    path_walkable[blocked_goal_x+blocked_goal_y*MAP_COLS]=0;
    reset_routes();
    assert(check_cached_route(blocked_goal_x-1,blocked_goal_y,
                              blocked_goal_x,blocked_goal_y));
    path_walkable[(blocked_goal_x-2)+blocked_goal_y*MAP_COLS]=0;
    reset_routes();
    assert(check_cached_route(blocked_goal_x-2,blocked_goal_y,
                              blocked_goal_x,blocked_goal_y));
    memset(path_walkable,0,sizeof(path_walkable));
    reset_routes();
    assert(check_cached_route(4,4,12,12)); /* empty flood restores no bits */
    int isolated_x=4,isolated_y=4;
    path_walkable[isolated_x+1+isolated_y*MAP_COLS]=1;
    path_walkable[12+12*MAP_COLS]=1;
    reset_routes();
    assert(check_cached_route(isolated_x,isolated_y,12,12));

    /* All slots retain a mixed economy/army working set and count hits. */
    memset(path_walkable,1,sizeof(path_walkable));
    reset_routes(); sc64_route_builds=0; sc64_route_hits=0;
    for(int g=0;g<SC64_ROUTE_SLOTS;g++) assert(sc64_route_next(8*16+8,8*16+8,(g+16)*16+8,16*16+8,&nx,&ny));
    for(int g=0;g<SC64_ROUTE_SLOTS;g++) assert(sc64_route_next(8*16+8,8*16+8,(g+16)*16+8,16*16+8,&nx,&ny));
    assert(sc64_route_builds==SC64_ROUTE_SLOTS&&sc64_route_hits==SC64_ROUTE_SLOTS);

    /* Capacity+1 independent destinations exercise the real LRU boundary.
       The scalar BFS oracle checks exact tie direction, not merely distance. */
    memset(path_walkable,1,sizeof(path_walkable));
    reset_routes(); sc64_route_clock=0; sc64_route_builds=0; sc64_route_hits=0;
    const int start_x=MAP_COLS/2,start_y=MAP_ROWS/2;
    int goals[SC64_ROUTE_SLOTS+2];
    for(int i=0;i<SC64_ROUTE_SLOTS+2;i++) {
        int gx=10+i%10,gy=10+i/10;
        goals[i]=gx+gy*MAP_COLS;
    }
    for(int i=0;i<SC64_ROUTE_SLOTS+1;i++)
        assert(check_cached_route(start_x,start_y,goals[i]%MAP_COLS,goals[i]/MAP_COLS));
    assert(sc64_route_builds==SC64_ROUTE_SLOTS+1&&sc64_route_hits==0);
    for(int i=1;i<=SC64_ROUTE_SLOTS;i++)assert(cached_goal(goals[i]));
    assert(!cached_goal(goals[0]));
    /* Touch the oldest still-resident key, then force one miss. That key must
       remain a hit; the next-oldest key must have been evicted and rebuild. */
    assert(check_cached_route(start_x,start_y,goals[1]%MAP_COLS,goals[1]/MAP_COLS));
    assert(sc64_route_hits==1);
    assert(check_cached_route(start_x,start_y,goals[SC64_ROUTE_SLOTS+1]%MAP_COLS,
                              goals[SC64_ROUTE_SLOTS+1]/MAP_COLS));
    assert(sc64_route_builds==SC64_ROUTE_SLOTS+2);
    assert(cached_goal(goals[1])&&!cached_goal(goals[2]));
    assert(cached_goal(goals[SC64_ROUTE_SLOTS+1]));
    assert(check_cached_route(start_x,start_y,goals[1]%MAP_COLS,goals[1]/MAP_COLS));
    assert(sc64_route_hits==2);
    assert(check_cached_route(start_x,start_y,goals[2]%MAP_COLS,goals[2]/MAP_COLS));
    assert(sc64_route_builds==SC64_ROUTE_SLOTS+3);
    assert(cached_goal(goals[2]));

    /* A reset drops all derived keys (without changing route semantics); an
       occupied-but-blocked start exits along the oracle's shortest gradient. */
    sc64_routes_reset();
    for(int i=0;i<SC64_ROUTE_SLOTS;i++)assert(sc64_route_valid[i]==0);
    assert(check_cached_route(start_x,start_y,goals[2]%MAP_COLS,goals[2]/MAP_COLS));
    assert(sc64_route_builds==SC64_ROUTE_SLOTS+4);
    path_walkable[start_x+start_y*MAP_COLS]=0;
    reset_routes();
    assert(check_cached_route(start_x,start_y,goals[3]%MAP_COLS,goals[3]/MAP_COLS));
    assert(sc64_route_builds==SC64_ROUTE_SLOTS+5);

    /* A full-height wall makes the goal unreachable from the query side. */
    memset(path_walkable,1,sizeof(path_walkable));
    for(int row=0;row<MAP_ROWS;row++)path_walkable[MAP_COLS/2+row*MAP_COLS]=0;
    reset_routes();
    assert(check_cached_route(2,MAP_ROWS/2,MAP_COLS-3,MAP_ROWS/2));
    printf("packed race routes match BFS; blocked-start and %d-slot cache pass\n", SC64_ROUTE_SLOTS);
    return 0;
}
