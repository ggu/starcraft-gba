/* Differential tests of Frontier point-route memo against the real BFS. */
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

static unsigned rng = 19;
static unsigned random_value(void) { rng = rng * 1664525u + 1013904223u; return rng; }
static void check(int x, int y, int tx, int ty, int tx2, int ty2)
{
    int ax = -123, ay = -456, bx = -123, by = -456;
    int a, b;
    frontier_route_cache_invalidate();
    a = path_next_tile(x, y, tx2, ty2, &ax, &ay);
    frontier_route_cache_invalidate();
    (void)path_next_tile(x, y, tx, ty, &bx, &by);
    bx = -123; by = -456;
    b = path_next_tile(x, y, tx2, ty2, &bx, &by);
    if (a != b || ax != bx || ay != by) {
        fprintf(stderr, "mismatch %d,%d to %d,%d: %d %d,%d / %d %d,%d\n",
                x,y,tx2,ty2,a,ax,ay,b,bx,by);
        exit(1);
    }
}
int main(void)
{
    for (int map = 0; map < 12; ++map) {
        invalidate_path_grid();
        for (int i = 0; i < MAP_CELLS; ++i)
            SC64_GRID_SET(path_walkable, i, map == 0 || (map != 1 && (random_value() >> 29) != 0));
        path_grid_valid = 1;
        for (int n = 0; n < 240; ++n) {
            int x = (int)(random_value() % 1100) - 38;
            int y = (int)(random_value() % 1100) - 38;
            int tx = (int)(random_value() % 1024), ty = (int)(random_value() % 1024);
            check(x, y, tx, ty, (tx & ~15) + 15, (ty & ~15) + 1);
        }
        check(8, 8, 1, 1, 15, 15);
        check(-16, -16, 1023, 1023, 1020, 1022);
    }
    /* Changing topology through the real invalidator must discard old rows. */
    memset(path_walkable, 0, sizeof(path_walkable)); path_grid_valid = 1;
    { int x = -1, y = -1;
      (void)path_next_tile(8, 8, 1000, 1000, &x, &y);
      invalidate_path_grid();
      for (int i = 0; i < MAP_CELLS; ++i) SC64_GRID_SET(path_walkable, i, 1);
      path_grid_valid = 1; x = -1; y = -1;
      if (!path_next_tile(8, 8, 1000, 1000, &x, &y) || x == -1) return 2;
    }
    frontier_route_cache_generation = 65535;
    frontier_route_cache_invalidate();
    if (frontier_route_cache_generation != 1) return 3;
    for (unsigned i = 0; i < 64; ++i) if (frontier_route_cache[i].generation) return 4;
    puts("Frontier route cache matches uncached BFS across random, blocked, edge and pixel cases");
    return 0;
}
