"""Exactness tests for the private SC64 incremental fog helper."""
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
C_SOURCE = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
#define GBA_HOT
#ifdef SC64_LARGE_MAPS
#define MAP_COLS 128
#ifdef SC64_T4_RUNTIME
#define MAP_ROWS 128
#else
#define MAP_ROWS 96
#endif
#else
#define MAP_COLS 64
#define MAP_ROWS 64
#endif
enum { MAP_CELLS=MAP_COLS*MAP_ROWS,
#ifdef SC64_LATE_TERRAN_STORAGE
       MAX_UNITS=192, MAX_BUILDINGS=96,
#else
       MAX_UNITS=40, MAX_BUILDINGS=16,
#endif
       SIDE=1, U_SCV=1, B_COMMAND=2
#ifdef SC64_RACES
       , U_DRONE=3, B_HATCHERY=21, B_LAIR=22, B_HIVE=23, B_NEXUS=24
#endif
#ifdef SC64_T4_RUNTIME
       , U_MAP_REVEALER=5, U_GHOST=6, U_CIVILIAN=7, U_RHYNADON=8,
       U_RAGNASAUR=9, U_DATA_DISK=10, B_TERRAN_BEACON=25
#endif
       };
#ifdef SC64_RACES
#define IS_WORKER(type) ((type)==U_SCV || (type)==U_DRONE)
#define sc64_cell_col(cell) ((cell)%MAP_COLS)
#define sc64_cell_row(cell) ((cell)/MAP_COLS)
static int race_cols=MAP_COLS, race_rows=MAP_ROWS;
static int sc64_race_is_zerg_townhall(int type) {
 return type==B_HATCHERY || type==B_LAIR || type==B_HIVE;
}
#ifdef SC64_T4_RUNTIME
static int sc64_race_t4_sight_source_pixels(int type) {
 switch(type) {
 case U_GHOST:return 288;
 case U_CIVILIAN:case U_RHYNADON:case U_RAGNASAUR:return 224;
 case U_MAP_REVEALER:return 320;
 case U_DATA_DISK:return 160;
 default:return 0;
 }
}
static int sc64_race_t4_building_sight_source_pixels(int type) {
 return type==B_TERRAN_BEACON?256:0;
}
#endif
#endif
typedef struct { int alive, owner, x, y, type; } UnitFixture;
typedef struct { int alive, owner, x, y, type; } BuildingFixture;
static UnitFixture u[MAX_UNITS];
static BuildingFixture b[MAX_BUILDINGS];
#ifdef SC64_LARGE_MAPS
#define SC64_GRID_STORAGE_BYTES ((MAP_CELLS+7)/8)
static u8 visible[SC64_GRID_STORAGE_BYTES], explored[SC64_GRID_STORAGE_BYTES];
static int test_grid_get(const u8 *grid,int cell) {
 return (grid[(unsigned)cell>>3] >> ((unsigned)cell&7)) & 1;
}
static void test_grid_set(u8 *grid,int cell,int value) {
 u8 mask=(u8)(1u<<((unsigned)cell&7));
 if(value)grid[(unsigned)cell>>3]|=mask;
 else grid[(unsigned)cell>>3]&=(u8)~mask;
}
#define SC64_GRID_GET(grid,cell) test_grid_get((grid),(cell))
#define SC64_GRID_SET(grid,cell,value) test_grid_set((grid),(cell),(value))
#define SC64_GRID_CLEAR(grid) memset((grid),0,sizeof(grid))
#else
static u8 visible[MAP_CELLS], explored[MAP_CELLS];
#endif
static int fixture_grid_get(const u8 *grid,int cell) {
#ifdef SC64_LARGE_MAPS
 return (grid[(unsigned)cell>>3] >> ((unsigned)cell&7)) & 1;
#else
 return grid[cell] != 0;
#endif
}
#include "starcraft/sc64_visibility.h"

static u8 ref_visible[MAP_CELLS], ref_explored[MAP_CELLS];
static u8 ref_minimap_visible[24*24], ref_minimap_explored[24*24];
static void compare(int step);
static void ref_explore(int x,int y,int radius) {
 int c=x/16,r=y/16,col,row;
 for(row=r-radius;row<=r+radius;row++) for(col=c-radius;col<=c+radius;col++)
  if(col>=0&&row>=0&&col<MAP_COLS&&row<MAP_ROWS&&
     (col-c)*(col-c)+(row-r)*(row-r)<=radius*radius)
   ref_explored[col+row*MAP_COLS]=1;
}
static void ref_vision(int x,int y,int radius,int extent) {
 int c=x/16,r=y/16,col,row;
 for(row=r-extent;row<=r+extent;row++) for(col=c-extent;col<=c+extent;col++)
  if(col>=0&&row>=0&&col<MAP_COLS&&row<MAP_ROWS) {
   int dx=col*16+8-x,dy=row*16+8-y;
   if(dx*dx+dy*dy<=radius*radius)ref_visible[col+row*MAP_COLS]=1;
  }
}
static void ref_update(void) {
 int i;
 memset(ref_visible,0,sizeof(ref_visible));
 for(i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE){
#ifdef SC64_RACES
  int worker=IS_WORKER(u[i].type);
#ifdef SC64_T4_RUNTIME
  int source=sc64_race_t4_sight_source_pixels(u[i].type);
#else
  int source=0;
#endif
#else
  int worker=u[i].type==U_SCV;
  int source=0;
#endif
  ref_explore(u[i].x,u[i].y,source?source/32:worker?4:5);
  ref_vision(u[i].x,u[i].y,source?source/2:worker?56:72,
             source?(source/2+23)/16:5);
 }
 for(i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE){
  int building_source=0;
#ifdef SC64_T4_RUNTIME
  building_source=sc64_race_t4_building_sight_source_pixels(b[i].type);
#endif
  ref_explore(b[i].x,b[i].y,building_source?building_source/32:b[i].type==B_COMMAND?7:3);
#ifdef SC64_RACES
  if(building_source)ref_vision(b[i].x,b[i].y,building_source/2,(building_source/2+23)/16);
  else if(b[i].type==B_COMMAND || sc64_race_is_zerg_townhall(b[i].type) ||
     b[i].type==B_NEXUS)ref_vision(b[i].x,b[i].y,112,8);
  else ref_vision(b[i].x,b[i].y,64,8);
#else
  if(b[i].type==B_COMMAND)ref_vision(b[i].x,b[i].y,112,8);
#endif
 }
}
static void near_move_sweep(int type, int start_step){
 static const int delta[10][2]={{1,0},{-1,0},{0,1},{0,-1},
   {15,0},{-15,0},{0,15},{0,-15},{15,15},{-15,-15}};
 int xoff,yoff,k,step=start_step;
 memset(u,0,sizeof(u));memset(b,0,sizeof(b));
 memset(visible,0,sizeof(visible));memset(explored,0,sizeof(explored));
 memset(ref_visible,0,sizeof(ref_visible));memset(ref_explored,0,sizeof(ref_explored));
 sc64_visibility_reset();
 u[0].alive=u[1].alive=1;u[0].owner=u[1].owner=SIDE;
 u[0].type=type;u[1].type=type==U_SCV?0:U_SCV;
 for(xoff=0;xoff<16;xoff++)for(yoff=0;yoff<16;yoff++){
  int x=400+xoff,y=400+yoff;
  u[0].x=x;u[0].y=y;u[1].x=x+5;u[1].y=y+4;
  compare(step++);
  for(k=0;k<10;k++){
   u[0].x=x+delta[k][0];u[0].y=y+delta[k][1];
   u[1].x=x+5+delta[k][0];u[1].y=y+4+delta[k][1];
   compare(step++);
   u[0].x=x;u[0].y=y;u[1].x=x+5;u[1].y=y+4;
   compare(step++);
  }
 }
}
static void edge_move_sweep(int start_step){
 static const int points[8][2]={{0,0},{15,0},{0,15},{15,15},
   {(MAP_COLS-1)*16,(MAP_ROWS-1)*16},{MAP_COLS*16-1,(MAP_ROWS-1)*16},
   {(MAP_COLS-1)*16,MAP_ROWS*16-1},{MAP_COLS*16-1,MAP_ROWS*16-1}};
 int i,step=start_step;
 memset(u,0,sizeof(u));memset(b,0,sizeof(b));
 memset(visible,0,sizeof(visible));memset(explored,0,sizeof(explored));
 memset(ref_visible,0,sizeof(ref_visible));memset(ref_explored,0,sizeof(ref_explored));
 sc64_visibility_reset();
 u[0].alive=u[1].alive=1;u[0].owner=u[1].owner=SIDE;
 u[0].type=U_SCV;u[1].type=0;
 for(i=0;i<8;i++){
  int dx=points[i][0]<MAP_COLS*16-15?15:-15;
  int dy=points[i][1]<MAP_ROWS*16-15?15:-15;
  u[0].x=points[i][0];u[0].y=points[i][1];
  u[1].x=points[i][0]+(dx>0?5:-5);u[1].y=points[i][1]+(dy>0?4:-4);
  compare(step++);
  u[0].x+=dx;u[0].y+=dy;u[1].x+=dx;u[1].y+=dy;
  compare(step++);
 }
}
static unsigned rng=1234567;
static unsigned rnd(void){rng=rng*1664525u+1013904223u;return rng;}
static void compare(int step){
 int i;
 sc64_update_exploration(); ref_update();
 memset(ref_minimap_visible,0,sizeof(ref_minimap_visible));
 memset(ref_minimap_explored,0,sizeof(ref_minimap_explored));
 for(i=0;i<MAP_CELLS;i++) {
  int mx=(i%MAP_COLS)*24/MAP_COLS;
  int my=(i/MAP_COLS)*24/MAP_ROWS;
  int mini=mx+my*24;
  ref_minimap_visible[mini]+=ref_visible[i]!=0;
  ref_minimap_explored[mini]+=ref_explored[i]!=0;
 }
 for(i=0;i<MAP_CELLS;i++)
  if(fixture_grid_get(visible,i)!=(ref_visible[i]!=0)||
     fixture_grid_get(explored,i)!=(ref_explored[i]!=0)){
   fprintf(stderr,"grid mismatch at step %d cell %d\n",step,i);exit(1);
  }
 if(memcmp(sc64_minimap_visible,ref_minimap_visible,sizeof(ref_minimap_visible))||
    memcmp(sc64_minimap_explored,ref_minimap_explored,sizeof(ref_minimap_explored))){
  fprintf(stderr,"visibility mismatch at step %d\n",step);exit(1);
 }
}
int main(void){
 int i,step;
 sc64_visibility_reset();
#ifdef SC64_LATE_TERRAN_STORAGE
 /* All 288 live sources overlap, then disappear one at a time. In
    particular removal must remain exact across the 256->255 transition. */
 for(i=0;i<MAX_UNITS;i++)u[i]=(UnitFixture){1,SIDE,640,640,U_SCV};
 for(i=0;i<MAX_BUILDINGS;i++)b[i]=(BuildingFixture){1,SIDE,640,640,B_COMMAND};
 compare(-600);
 if(sc64_visibility_source_count(40+40*MAP_COLS)!=288)abort();
 for(i=0;i<MAX_UNITS;i++){u[i].alive=0;compare(-601-i);}
 for(i=0;i<MAX_BUILDINGS;i++){b[i].alive=0;compare(-801-i);}
 if(sc64_visibility_source_count(40+40*MAP_COLS)!=0)abort();
#endif
#ifdef SC64_RACES
 /* Imported Zerg town halls share building sight exactly, including circles
    clipped at all four map corners and edges. */
 for(i=0;i<3;i++){
  int j;
  int halls[3]={B_HATCHERY,B_LAIR,B_HIVE};
  int edge_x[4]={0,MAP_COLS*16-1,0,MAP_COLS*16-1};
  int edge_y[4]={0,0,MAP_ROWS*16-1,MAP_ROWS*16-1};
  memset(u,0,sizeof(u));memset(b,0,sizeof(b));
  memset(visible,0,sizeof(visible));memset(explored,0,sizeof(explored));
  memset(ref_visible,0,sizeof(ref_visible));memset(ref_explored,0,sizeof(ref_explored));
  sc64_visibility_reset();
  b[0].alive=1;b[0].owner=SIDE;b[0].type=halls[i];
  for(j=0;j<4;j++){
   b[0].x=edge_x[j];b[0].y=edge_y[j];
   compare(-100-i*4-j);
  }
 }
#endif
#ifdef SC64_T4_RUNTIME
 /* Every T4 special unit and the Beacon use their own source radius; exercise
    incremental replacement, overlap counts and all clipped map edges. */
 {
  static const int types[]={U_GHOST,U_CIVILIAN,U_RHYNADON,U_RAGNASAUR,
                            U_MAP_REVEALER,U_DATA_DISK};
  static const int px[4]={640,0,MAP_COLS*16-1,0};
  static const int py[4]={640,0,0,MAP_ROWS*16-1};
  int j,k;
  for(j=0;j<(int)(sizeof(types)/sizeof(types[0]));++j){
   memset(u,0,sizeof(u));memset(b,0,sizeof(b));
   memset(visible,0,sizeof(visible));memset(explored,0,sizeof(explored));
   memset(ref_visible,0,sizeof(ref_visible));memset(ref_explored,0,sizeof(ref_explored));
   sc64_visibility_reset();
   u[0].alive=1;u[0].owner=SIDE;u[0].type=types[j];
   for(k=0;k<4;k++){
    u[0].x=px[k];u[0].y=py[k];compare(-300-j*8-k);
    u[0].x+=u[0].x<MAP_COLS*16-15?15:-15;
    u[0].y+=u[0].y<MAP_ROWS*16-15?15:-15;compare(-340-j*8-k);
   }
   u[0].alive=0;compare(-400-j);
  }
  memset(u,0,sizeof(u));memset(b,0,sizeof(b));
  memset(visible,0,sizeof(visible));memset(explored,0,sizeof(explored));
  memset(ref_visible,0,sizeof(ref_visible));memset(ref_explored,0,sizeof(ref_explored));
  sc64_visibility_reset();
  b[0].alive=1;b[0].owner=SIDE;b[0].type=B_TERRAN_BEACON;
  for(k=0;k<4;k++){
   b[0].x=px[k];b[0].y=py[k];compare(-500-k);
   b[0].x+=b[0].x<MAP_COLS*16-15?15:-15;
   b[0].y+=b[0].y<MAP_ROWS*16-15?15:-15;compare(-510-k);
  }
  b[0].alive=0;compare(-520);
 }
#endif
 near_move_sweep(U_SCV,-2000);
 near_move_sweep(0,-12000);
 edge_move_sweep(-22000);
 /* Moving sources by one pixel changes their exact sight-circle boundary,
    while their exploration tile remains the same. Two nearby sources overlap
    so source-count transitions and minimap occupancy must remain exact. */
 u[0].alive=1;u[0].owner=SIDE;u[0].type=U_SCV;u[0].x=511;u[0].y=513;
 u[1].alive=1;u[1].owner=SIDE;u[1].type=0;u[1].x=518;u[1].y=514;
 for(i=0;i<40;i++){
  u[0].x++;u[1].y--;
  compare(-40+i);
 }
 u[1].x=527;u[1].y=529;compare(-1);
 u[1].owner=0;compare(-2);
 u[1].owner=SIDE;u[1].type=U_SCV;compare(-3);
 u[1].alive=0;compare(-4);
 for(step=0;step<1000;step++){
  for(i=0;i<MAX_UNITS;i++)if((rnd()%5)==0){
   u[i].alive=(rnd()%4)!=0;u[i].owner=(int)(rnd()%3);
   u[i].type=(rnd()%2)?U_SCV:0;
   u[i].x=(int)(rnd()%(MAP_COLS*16));u[i].y=(int)(rnd()%(MAP_ROWS*16));
  }
  for(i=0;i<MAX_BUILDINGS;i++)if((rnd()%7)==0){
   b[i].alive=(rnd()%4)!=0;b[i].owner=(int)(rnd()%3);
#ifdef SC64_RACES
   switch(rnd()%5){
    case 0:b[i].type=B_COMMAND;break;
    case 1:b[i].type=B_HATCHERY;break;
    case 2:b[i].type=B_LAIR;break;
    case 3:b[i].type=B_HIVE;break;
    default:b[i].type=0;break;
   }
#else
   b[i].type=(rnd()%3)==0?B_COMMAND:0;
#endif
   b[i].x=(int)(rnd()%(MAP_COLS*16));b[i].y=(int)(rnd()%(MAP_ROWS*16));
  }
  compare(step);
  if(step==499){
   /* Simulate load: persistent explored tiles survive, visible state/cache
      must be rebuilt from restored units/buildings. */
   sc64_visibility_reset();
   compare(step+1);
  }
 }
 puts("incremental visibility matches full recomputation");
 return 0;
}
'''


class VisibilityTests(unittest.TestCase):
    def test_incremental_cache_matches_reference(self):
        with tempfile.TemporaryDirectory() as temp:
            source = Path(temp) / "visibility.c"
            source.write_text(C_SOURCE)
            for variant, flags in (("legacy", []), ("races", ["-DSC64_RACES"])):
                with self.subTest(variant=variant):
                    for spans, span_flags in (("baseline", []),
                                              ("spans", ["-DSC64_VISIBILITY_SPANS"])):
                        with self.subTest(path=spans):
                            binary = Path(temp) / f"visibility-{variant}-{spans}"
                            subprocess.run([
                                "clang", "-std=c11", "-O1", "-fsanitize=address,undefined",
                                *flags, *span_flags, "-I", str(ROOT), str(source),
                                "-o", str(binary),
                            ], check=True)
                            result = subprocess.run([str(binary)], check=True, text=True,
                                                    capture_output=True)
                            self.assertIn("matches full recomputation", result.stdout)
            with self.subTest(variant="large-packed-races-spans"):
                binary = Path(temp) / "visibility-large-packed-spans"
                subprocess.run([
                    "clang", "-std=c11", "-O1", "-fsanitize=address,undefined",
                    "-DSC64_RACES", "-DSC64_LARGE_MAPS", "-DSC64_VISIBILITY_SPANS",
                    "-I", str(ROOT), str(source), "-o", str(binary),
                ], check=True)
                result = subprocess.run([str(binary)], check=True, text=True,
                                        capture_output=True)
                self.assertIn("matches full recomputation", result.stdout)
            for spans, span_flags in (("baseline", []),
                                      ("spans", ["-DSC64_VISIBILITY_SPANS"]),
                                      ("late-baseline", ["-DSC64_LATE_TERRAN_STORAGE"]),
                                      ("late-spans", ["-DSC64_LATE_TERRAN_STORAGE", "-DSC64_VISIBILITY_SPANS"])):
                with self.subTest(variant=f"t4-128x128-packed-{spans}"):
                    binary = Path(temp) / f"visibility-t4-128x128-{spans}"
                    subprocess.run([
                        "clang", "-std=c11", "-O1", "-fsanitize=address,undefined",
                        "-DSC64_RACES", "-DSC64_LARGE_MAPS", "-DSC64_T4_RUNTIME",
                        *span_flags, "-I", str(ROOT), str(source), "-o", str(binary),
                    ], check=True)
                    result = subprocess.run([str(binary)], check=True, text=True,
                                            capture_output=True)
                    self.assertIn("matches full recomputation", result.stdout)


if __name__ == "__main__":
    unittest.main()
