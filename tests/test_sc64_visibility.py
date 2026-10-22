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
#define GBA_HOT
enum { MAP_COLS=64, MAP_ROWS=64, MAP_CELLS=4096, MAX_UNITS=40,
       MAX_BUILDINGS=16, SIDE=1, U_SCV=1, B_COMMAND=2 };
typedef struct { int alive, owner, x, y, type; } UnitFixture;
typedef struct { int alive, owner, x, y, type; } BuildingFixture;
static UnitFixture u[MAX_UNITS];
static BuildingFixture b[MAX_BUILDINGS];
static u8 visible[MAP_CELLS], explored[MAP_CELLS];
#include "starcraft/sc64_visibility.h"

static u8 ref_visible[MAP_CELLS], ref_explored[MAP_CELLS];
static u8 ref_minimap_visible[24*24], ref_minimap_explored[24*24];
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
  int r=u[i].type==U_SCV?4:5;
  ref_explore(u[i].x,u[i].y,r);
  ref_vision(u[i].x,u[i].y,u[i].type==U_SCV?56:72,5);
 }
 for(i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE){
  ref_explore(b[i].x,b[i].y,b[i].type==B_COMMAND?7:3);
  if(b[i].type==B_COMMAND)ref_vision(b[i].x,b[i].y,112,8);
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
 if(memcmp(visible,ref_visible,sizeof(visible))||
    memcmp(explored,ref_explored,sizeof(explored))||
    memcmp(sc64_minimap_visible,ref_minimap_visible,sizeof(ref_minimap_visible))||
    memcmp(sc64_minimap_explored,ref_minimap_explored,sizeof(ref_minimap_explored))){
  fprintf(stderr,"visibility mismatch at step %d\n",step);exit(1);
 }
}
int main(void){
 int i,step;
 sc64_visibility_reset();
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
   u[i].x=(int)(rnd()%1024);u[i].y=(int)(rnd()%1024);
  }
  for(i=0;i<MAX_BUILDINGS;i++)if((rnd()%7)==0){
   b[i].alive=(rnd()%4)!=0;b[i].owner=(int)(rnd()%3);
   b[i].type=(rnd()%3)==0?B_COMMAND:0;
   b[i].x=(int)(rnd()%1024);b[i].y=(int)(rnd()%1024);
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
            binary = Path(temp) / "visibility-test"
            source.write_text(C_SOURCE)
            subprocess.run([
                "clang", "-std=c11", "-O1", "-fsanitize=address,undefined",
                "-I", str(ROOT), str(source), "-o", str(binary),
            ], check=True)
            result = subprocess.run([str(binary)], check=True, text=True,
                                    capture_output=True)
        self.assertIn("matches full recomputation", result.stdout)


if __name__ == "__main__":
    unittest.main()
