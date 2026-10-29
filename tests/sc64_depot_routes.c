/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent pixel-circle and four-neighbour BFS oracle for depot routing. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8;
#define GBA_HOT
#ifndef SC64_DEPOT_TEST_LEGACY
#define SC64_RACES
#endif
#ifdef SC64_ROUTE_ACTIVE_STRIDE
static int fixture_cols=64,fixture_rows=64;
#define MAP_COLS fixture_cols
#define MAP_ROWS fixture_rows
enum { MAP_CELLS=128*128 };
#else
#ifndef RACE_TEST_COLS
#define RACE_TEST_COLS 64
#define RACE_TEST_ROWS 64
#endif
enum { MAP_COLS=RACE_TEST_COLS,MAP_ROWS=RACE_TEST_ROWS,MAP_CELLS=MAP_COLS*MAP_ROWS };
#endif
static u8 path_walkable[MAP_CELLS];
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid=1;
static int limit(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
static int sc64_cell_row(unsigned c){return (int)c/MAP_COLS;}
static int sc64_cell_col(unsigned c){return (int)c%MAP_COLS;}
static void build_path_grid(void){path_grid_valid=1;}
#define SC64_GRID_GET(grid,cell) ((grid)[cell]!=0)
#define SC64_GRID_SET(grid,cell,value) ((grid)[cell]=(u8)((value)!=0))
#include "starcraft/sc64_routes.h"
#ifdef SC64_ROUTE_ACTIVE_STRIDE
_Static_assert(sizeof(sc64_route_pool)==14u*SC64_ROUTE_BYTES,"depot routes must reuse existing pool");
#elif defined(SC64_RACES)
_Static_assert(sizeof(sc64_route_field)==SC64_ROUTE_SLOTS*SC64_ROUTE_BYTES,"depot routes must reuse existing fields");
#else
_Static_assert(sizeof(sc64_route_field)==SC64_ROUTE_SLOTS*MAP_CELLS*sizeof(u16),"legacy depot routes must reuse existing fields");
#endif
static int dist[MAP_CELLS];
static u8 direction[MAP_CELLS], saved_grid[MAP_CELLS];
static u32 seed=0x174cc0deu;
static u32 rnd(void){seed=seed*1664525u+1013904223u;return seed;}

/* Enumerate every integer pixel, independently of the production clamp. */
static int pixel_oracle(int cx,int cy,int col,int row,int *distance)
{
    int best=0x7fffffff;
    for(int y=row*16;y<row*16+16;y++)for(int x=col*16;x<col*16+16;x++){
        int dx=x-cx,dy=y-cy,d=dx*dx+dy*dy;
        if(d<best)best=d;
    }
    *distance=best;return best<=32*32;
}
static void geometry_test(void)
{
    for(int cy=96;cy<112;cy++)for(int cx=96;cx<112;cx++)
        for(int row=3;row<=10;row++)for(int col=3;col<=10;col++){
            int d,x=-999,y=-999,expect=pixel_oracle(cx,cy,col,row,&d);
            int got=sc64_depot_cell_point(cx,cy,col,row,&x,&y);
            assert(got==expect);
            if(got){int dx=x-cx,dy=y-cy;assert(x>=col*16&&x<col*16+16&&y>=row*16&&y<row*16+16);assert(dx*dx+dy*dy==d);}
        }
    /* Circle touches a corner at exactly 32 pixels and misses the next cell. */
    {int x,y;assert(sc64_depot_cell_point(0,0,2,0,&x,&y)&&x==32&&y==0);assert(!sc64_depot_cell_point(0,0,3,0,&x,&y));}
}

static int make_bfs(int cx,int cy,int point_goal)
{
    int cells=MAP_COLS*MAP_ROWS,head=0,tail=0;
    for(int i=0;i<cells;i++){dist[i]=-1;direction[i]=255;}
    for(int cell=0;cell<cells;cell++){
        int d,seeded=point_goal>=0 ? cell==point_goal : pixel_oracle(cx,cy,cell%MAP_COLS,cell/MAP_COLS,&d);
        if(path_walkable[cell]&&seeded){dist[cell]=0;direction[cell]=0;bfs_queue[tail++]=(u16)cell;}
    }
    int initial=tail;
    while(head<tail){
        int c=bfs_queue[head++],x=c%MAP_COLS,y=c/MAP_COLS;
        int n[4]={x?c-1:-1,x+1<MAP_COLS?c+1:-1,y?c-MAP_COLS:-1,y+1<MAP_ROWS?c+MAP_COLS:-1};
        for(int j=0;j<4;j++)if(n[j]>=0&&path_walkable[n[j]]&&dist[n[j]]<0){dist[n[j]]=dist[c]+1;direction[n[j]]=(u8)(j+1);bfs_queue[tail++]=(u16)n[j];}
    }
    return initial;
}
static void reset(void){sc64_routes_reset();}
/* Check every byte's semantic field value against the oracle after the
 * complete depot flood, including the legacy distance/direction encoding. */
static void check_depot_field(int id)
{
    int slot=-1;
    for(int i=0;i<SC64_ROUTE_ACTIVE_SLOTS;i++)
        if(sc64_route_valid[i]&&sc64_route_goal[i]==(u16)(SC64_ROUTE_DEPOT_TAG|id)){slot=i;break;}
    assert(slot>=0);
    for(int i=0;i<MAP_COLS*MAP_ROWS;i++) {
#ifdef SC64_RACES
        int value=sc64_route_get(slot,i);
        assert(value==(dist[i]<0?SC64_ROUTE_UNREACHABLE:direction[i]));
#else
        u16 value=sc64_route_field[slot][i];
        assert(value==(dist[i]<0?0xffff:(u16)((dist[i]<<3)|direction[i])));
#endif
    }
}
static void depot_query(int sx,int sy,int cx,int cy,int id)
{
    int cell=sx+sy*MAP_COLS,nx=-1,ny=-1,expected=-1,reachable=dist[cell]>=0;
    if(!path_walkable[cell]){
        int best=0x7fffffff;static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
        for(int j=0;j<4;j++){int x=sx+dx[j],y=sy+dy[j];if(x<0||y<0||x>=MAP_COLS||y>=MAP_ROWS)continue;int d=dist[x+y*MAP_COLS];if(d>=0&&d<best){best=d;expected=x+y*MAP_COLS;}}
        reachable=best!=0x7fffffff;
    }
    memcpy(saved_grid,path_walkable,sizeof(saved_grid));
    int ok=sc64_route_depot_next(sx*16+8,sy*16+8,id,cx,cy,&nx,&ny);
    assert(memcmp(saved_grid,path_walkable,sizeof(saved_grid))==0);
    if(ok!=reachable){fprintf(stderr,"depot reach mismatch src=%d,%d depot=%d,%d id=%d expected=%d got=%d\n",sx,sy,cx,cy,id,reachable,ok);assert(0);}
    if(!ok)return;
    int col=nx/16,row=ny/16;assert(col>=0&&row>=0&&col<MAP_COLS&&row<MAP_ROWS);
    int next=col+row*MAP_COLS;
    if(path_walkable[cell]&&dist[cell]==0){int dx=nx-cx,dy=ny-cy;assert(next==cell&&dx*dx+dy*dy<=1024);}
    else {assert((col==sx&&(row==sy+1||row==sy-1))||(row==sy&&(col==sx+1||col==sx-1)));assert(dist[next]>=0);if(path_walkable[cell]){assert(dist[next]==dist[cell]-1);int d=direction[cell];int want=cell+(d==1?1:d==2?-1:d==3?MAP_COLS:-MAP_COLS);assert(next==want);}else assert(next==expected);}
}
static void point_query(int sx,int sy,int gx,int gy)
{
    int cell=sx+sy*MAP_COLS,nx,ny;assert(path_walkable[cell]&&path_walkable[gx+gy*MAP_COLS]);
    make_bfs(0,0,gx+gy*MAP_COLS);
    int ok=sc64_route_next(sx*16+8,sy*16+8,gx*16+8,gy*16+8,&nx,&ny);
    assert(ok==(dist[cell]>=0));if(!ok)return;
    int col=nx/16,row=ny/16,next=col+row*MAP_COLS;
    if(dist[cell]==0)assert(next==cell);
    else {assert(next>=0&&next<MAP_COLS*MAP_ROWS);assert(dist[next]==dist[cell]-1);int d=direction[cell];int want=cell+(d==1?1:d==2?-1:d==3?MAP_COLS:-MAP_COLS);assert(next==want);}
}
static void exercise(int cx,int cy,int id)
{
    make_bfs(cx,cy,-1);
    for(int j=0;j<30;j++){int sx=(int)(rnd()%(u32)MAP_COLS),sy=(int)(rnd()%(u32)MAP_ROWS);depot_query(sx,sy,cx,cy,id);if(j==0)check_depot_field(id);}
}
int main(void)
{
    geometry_test();
    {int x=91,y=92;assert(!sc64_route_depot_next(-1,0,0,0,0,&x,&y));
     assert(!sc64_route_depot_next(0,0,-1,0,0,&x,&y));
     assert(!sc64_route_depot_next(0,0,32768,0,0,&x,&y));
     assert(!sc64_route_depot_next(0,0,0,-1,0,&x,&y));
     assert(!sc64_route_depot_next(0,0,0,0,0,0,&y));}
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    fixture_cols=64;fixture_rows=64;
#endif
    int cells=MAP_COLS*MAP_ROWS;
    for(int trial=0;trial<48;trial++){
        for(int i=0;i<cells;i++)path_walkable[i]=(rnd()%100)<72;
        reset();int cx=(int)(rnd()%(u32)(MAP_COLS*16)),cy=(int)(rnd()%(u32)(MAP_ROWS*16));
        exercise(cx,cy,trial%13);
        /* Interleave point goal and tagged depot goal in the same cache. */
        int sx=MAP_COLS/2,sy=MAP_ROWS/2,gx=MAP_COLS/2+3,gy=MAP_ROWS/2+3;
        path_walkable[sx+sy*MAP_COLS]=path_walkable[gx+gy*MAP_COLS]=1;reset();
        point_query(sx,sy,gx,gy);make_bfs(cx,cy,-1);depot_query(sx,sy,cx,cy,trial%13);point_query(sx,sy,gx,gy);
    }
    /* Every low-bit center pair, including cells across a map boundary. */
    memset(path_walkable,1,sizeof(path_walkable));
    for(int oy=0;oy<16;oy++)for(int ox=0;ox<16;ox++){
        int cx=ox,cy=oy;reset();make_bfs(cx,cy,-1);depot_query(7,7,cx,cy,0);
    }
    /* One deposit circle intersects both sides of an impassable wall. */
    memset(path_walkable,1,sizeof(path_walkable));
    for(int y=0;y<MAP_ROWS;y++)path_walkable[MAP_COLS/2+y*MAP_COLS]=0;
    int cx=MAP_COLS/2*16+8,cy=MAP_ROWS/2*16+8;reset();
    assert(make_bfs(cx,cy,-1)>1);
    depot_query(MAP_COLS/2-2,MAP_ROWS/2,cx,cy,1);
    depot_query(MAP_COLS/2+2,MAP_ROWS/2,cx,cy,1);
    /* One walkable seed at an exact edge intersection, then one corner cell. */
    memset(path_walkable,0,sizeof(path_walkable));
    path_walkable[7+5*MAP_COLS]=1;reset();assert(make_bfs(80,80,-1)==1);
    depot_query(7,5,80,80,2);depot_query(8,5,80,80,2);
    memset(path_walkable,0,sizeof(path_walkable));
    path_walkable[6+6*MAP_COLS]=1;reset();assert(make_bfs(79,79,-1)==1);
    depot_query(6,6,79,79,3);depot_query(7,6,79,79,3);
    /* Empty field and a long corridor with an obstructed origin. */
    memset(path_walkable,0,sizeof(path_walkable));reset();make_bfs(0,0,-1);depot_query(MAP_COLS-1,MAP_ROWS-1,0,0,1);
    for(int y=1;y<MAP_ROWS-1;y++)path_walkable[1+y*MAP_COLS]=1;
    for(int x=1;x<MAP_COLS-1;x++)path_walkable[x+(MAP_ROWS-2)*MAP_COLS]=1;
    reset();make_bfs(16,16,-1);depot_query(MAP_COLS-2,MAP_ROWS-2,16,16,2);
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    fixture_cols=96;fixture_rows=48;memset(path_walkable,1,sizeof(path_walkable));reset();exercise(1535,767,3);
    fixture_cols=128;fixture_rows=128;memset(path_walkable,1,sizeof(path_walkable));reset();exercise(2047,2047,3);
#endif
    puts("depot geometry and route BFS oracle passed");return 0;
}
