"""Host checks for SC64 cached route fields against reference BFS."""
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
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8;
#define GBA_HOT
enum { MAP_COLS=64, MAP_ROWS=64, MAP_CELLS=4096 };
static u8 path_walkable[MAP_CELLS];
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid=1;
static int limit(int n,int lo,int hi){return n<lo?lo:(n>hi?hi:n);}
static int dist2(int x,int y,int a,int b){int dx=x-a,dy=y-b;return dx*dx+dy*dy;}
static void build_path_grid(void){path_grid_valid=1;}
#include "starcraft/sc64_routes.h"

static unsigned seed=0x7123abceu;
static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static void reference_distance(int goal,u8 *distance){
 int head=0,tail=0,dx,dy;
 memset(distance,255,MAP_CELLS);
 if(path_walkable[goal]){distance[goal]=0;bfs_queue[tail++]=(u16)goal;}
 else {
  int gx=goal%MAP_COLS,gy=goal/MAP_COLS;
  for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++){
   int x=gx+dx,y=gy+dy,cell;
   if(x<0||y<0||x>=MAP_COLS||y>=MAP_ROWS)continue;
   cell=x+y*MAP_COLS;
   if(path_walkable[cell]&&distance[cell]==255){distance[cell]=0;bfs_queue[tail++]=(u16)cell;}
  }
 }
 while(head<tail){
  int cell=bfs_queue[head++],x=cell%MAP_COLS,y=cell/MAP_COLS;
  int n[4]={x>0?cell-1:-1,x+1<MAP_COLS?cell+1:-1,y>0?cell-MAP_COLS:-1,y+1<MAP_ROWS?cell+MAP_COLS:-1};
  for(int i=0;i<4;i++)if(n[i]>=0&&path_walkable[n[i]]&&distance[n[i]]==255){
   distance[n[i]]=distance[cell]+1;bfs_queue[tail++]=(u16)n[i];
  }
 }
}
static int check_route(int sx,int sy,int gx,int gy,int test){
 u8 distance[MAP_CELLS]; int nx=-1,ny=-1;
 int start=sx+sy*MAP_COLS,goal=gx+gy*MAP_COLS;
 int ok,step,expected=255;
 reference_distance(goal,distance);
 ok=sc64_route_next(sx*16+8,sy*16+8,gx*16+8,gy*16+8,&nx,&ny);
 if(!path_walkable[start]){
  int x,y,n,best=255;
  static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
  for(int i=0;i<4;i++){
   x=sx+dx[i];y=sy+dy[i];
   if(x<0||y<0||x>=MAP_COLS||y>=MAP_ROWS)continue;
   n=x+y*MAP_COLS;if(distance[n]<best)best=distance[n];
  }
  expected=best;
 } else expected=distance[start];
 if(expected==255){if(ok){fprintf(stderr,"unexpected route test=%d\n",test);return 0;}return 1;}
 if(!ok){fprintf(stderr,"missing route test=%d expected=%d\n",test,expected);return 0;}
 if(nx==gx*16+8&&ny==gy*16+8){
  if(start==goal||(!path_walkable[goal]&&expected==0))return 1;
  fprintf(stderr,"route jumps to goal test=%d\n",test);return 0;
 }
 step=nx/16+(ny/16)*MAP_COLS;
 if(step<0||step>=MAP_CELLS||!path_walkable[step]){
  fprintf(stderr,"route step blocked test=%d from=%d,%d to=%d,%d\n",test,sx,sy,nx/16,ny/16);return 0;
 }
 if(!path_walkable[start]){
  if(distance[step]!=expected){fprintf(stderr,"blocked-start nonshortest test=%d expected=%d got=%d\n",test,expected,distance[step]);return 0;}
 }else if(distance[start] && distance[step]+1!=distance[start]){
  fprintf(stderr,"nonshortest step test=%d dist=%d next=%d\n",test,distance[start],distance[step]);return 0;
 }
 return 1;
}
static int randomized(void){
 for(int map=0;map<100;map++){
  memset(sc64_route_valid,0,sizeof(sc64_route_valid));
  for(int i=0;i<MAP_CELLS;i++)path_walkable[i]=(rnd()%100)<68;
  for(int q=0;q<100;q++){
   int sx=rnd()%MAP_COLS,sy=rnd()%MAP_ROWS;
   int gx=rnd()%MAP_COLS,gy=rnd()%MAP_ROWS;
   if(!check_route(sx,sy,gx,gy,map*100+q))return 0;
  }
 }
 return 1;
}
static int cache_tests(void){
 int x,y,start=32+32*MAP_COLS;
 memset(path_walkable,1,sizeof(path_walkable));
 sc64_routes_reset();sc64_route_clock=0;sc64_route_hits=0;sc64_route_builds=0;
 for(int g=0;g<8;g++)if(!sc64_route_next(32*16+8,32*16+8,(g+2)*16+8,2*16+8,&x,&y))return 0;
 if(sc64_route_builds!=8)return 0;
 sc64_route_next(32*16+8,32*16+8,2*16+8,2*16+8,&x,&y); /* make goal 0 MRU */
 sc64_route_next(32*16+8,32*16+8,10*16+8,2*16+8,&x,&y); /* evict goal 1 */
 if(sc64_route_builds!=9)return 0;
 sc64_route_next(32*16+8,32*16+8,3*16+8,2*16+8,&x,&y);
 if(sc64_route_builds!=10)return 0;
 path_walkable[start-1]=0;
 sc64_routes_reset();
 sc64_route_next(32*16+8,32*16+8,2*16+8,2*16+8,&x,&y);
 if(sc64_route_builds!=11)return 0;
 if(x/16==31&&y/16==32){fprintf(stderr,"route cache failed changed-grid reset\n");return 0;}
 return 1;
}
int main(void){
 if(!randomized())return 1;
 if(!cache_tests()){fprintf(stderr,"cache/invalidation test failed\n");return 1;}
 puts("route fields match reference BFS; cache tests pass");return 0;
}
'''


class RouteTests(unittest.TestCase):
    def test_cached_fields_match_reference_bfs(self):
        with tempfile.TemporaryDirectory() as temp:
            source = Path(temp) / "routes.c"
            binary = Path(temp) / "routes-test"
            source.write_text(C_SOURCE)
            subprocess.run([
                "clang", "-std=c11", "-O1", "-fsanitize=address,undefined",
                "-I", str(ROOT), str(source), "-o", str(binary),
            ], check=True)
            result = subprocess.run([str(binary)], check=False, text=True,
                                    capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr or result.stdout)
        self.assertIn("match reference BFS", result.stdout)

    def test_t4_128x128_packed_route_profile(self):
        source = ROOT / "tests/race_routes.c"
        with tempfile.TemporaryDirectory() as temp:
            binary = Path(temp) / "routes-t4-test"
            subprocess.run([
                "clang", "-std=c11", "-O1", "-fsanitize=address,undefined",
                "-I", str(ROOT), "-I", str(ROOT / "common"),
                "-DSC64_ROUTE_PACK3", "-DSC64_LARGE_MAPS",
                "-DSC64_T4_RUNTIME", "-DRACE_TEST_COLS=128", "-DRACE_TEST_ROWS=128",
                str(source), "-o", str(binary),
            ], check=True)
            result = subprocess.run([str(binary)], check=False, text=True,
                                    capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr or result.stdout)
        self.assertIn("14-slot cache pass", result.stdout)

    def test_packed_visit_grid_matches_fields_and_restores_grid(self):
        source = ROOT / "tests/race_routes.c"
        with tempfile.TemporaryDirectory() as temp:
            binary = Path(temp) / "routes-visit-grid-test"
            subprocess.run([
                "clang", "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                "-fsanitize=address,undefined", "-I", str(ROOT), "-I", str(ROOT / "common"),
                "-DSC64_ROUTE_PACK3", "-DSC64_LARGE_MAPS", "-DSC64_T4_RUNTIME",
                "-DSC64_ROUTE_VISIT_GRID", "-DRACE_TEST_COLS=128", "-DRACE_TEST_ROWS=128",
                str(source), "-o", str(binary),
            ], check=True)
            result = subprocess.run([str(binary)], check=False, text=True,
                                    capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr or result.stdout)
        self.assertIn("14-slot cache pass", result.stdout)


if __name__ == "__main__":
    unittest.main()
