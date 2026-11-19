/* Differential tests of Frontier point routing against a scalar BFS. */
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

typedef struct { int found, x, y; } PointRoute;

static PointRoute reference_route(int x, int y, int tx, int ty)
{
    static const int dx[] = {1, -1, 0, 0};
    static const int dy[] = {0, 0, 1, -1};
    u8 visited[MAP_CELLS] = {0};
    u16 queue[MAP_CELLS], first[MAP_CELLS];
    int start = limit(x / 16, 0, MAP_COLS - 1) + limit(y / 16, 0, MAP_ROWS - 1) * MAP_COLS;
    int goal_col = limit(tx / 16, 0, MAP_COLS - 1);
    int goal_row = limit(ty / 16, 0, MAP_ROWS - 1);
    int goal_open = SC64_GRID_GET(path_walkable, goal_col + goal_row * MAP_COLS);
    int head = 0, tail = 1, best = -1, best_distance = 3;

    queue[0] = first[start] = (u16)start;
    visited[start] = 1;
    while (head < tail) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
        int x_error = col - goal_col, y_error = row - goal_row;
        int distance = x_error * x_error + y_error * y_error;
        if (distance < best_distance) {
            best = cell;
            best_distance = distance;
            if (!distance || !goal_open) break;
        }
        for (int dir = 0; dir < 4; ++dir) {
            int nc = col + dx[dir], nr = row + dy[dir];
            if (nc < 0 || nc >= MAP_COLS || nr < 0 || nr >= MAP_ROWS) continue;
            int next = nc + nr * MAP_COLS;
            if (visited[next] || !SC64_GRID_GET(path_walkable, next)) continue;
            visited[next] = 1;
            first[next] = cell == start ? (u16)next : first[cell];
            queue[tail++] = (u16)next;
        }
    }
    if (best < 0) return (PointRoute){0, -123, -456};
    if (best == start) return (PointRoute){1, tx, ty};
    return (PointRoute){1, first[best] % MAP_COLS * 16 + 8,
                        first[best] / MAP_COLS * 16 + 8};
}

static void check_route(int x, int y, int tx, int ty, PointRoute expected)
{
    int nx = -123, ny = -456;
    int found = path_next_tile(x, y, tx, ty, &nx, &ny);
    if (found != expected.found || nx != expected.x || ny != expected.y) {
        fprintf(stderr, "mismatch %d,%d to %d,%d: %d %d,%d expected %d %d,%d\n",
                x, y, tx, ty, found, nx, ny, expected.found, expected.x, expected.y);
        exit(1);
    }
}

static unsigned rng = 19;
static unsigned random_value(void) { rng = rng * 1664525u + 1013904223u; return rng; }
static void check(int x, int y, int tx, int ty, int tx2, int ty2)
{
    PointRoute expected = reference_route(x, y, tx2, ty2);
    int nx, ny;
    frontier_route_cache_invalidate();
    check_route(x, y, tx2, ty2, expected);
    frontier_route_cache_invalidate();
    (void)path_next_tile(x, y, tx, ty, &nx, &ny);
    check_route(x, y, tx2, ty2, expected);
}

static void set_grid(const u8 (*cells)[2], size_t count)
{
    invalidate_path_grid();
    memset(path_walkable, 0, sizeof path_walkable);
    for (size_t i = 0; i < count; ++i)
        SC64_GRID_SET(path_walkable, cells[i][0] + cells[i][1] * MAP_COLS, 1);
    path_grid_valid = 1;
}

static void check_directed_routes(void)
{
    static const u8 x_first[][2] = {{3, 2}, {4, 2}, {4, 3}, {4, 4}};
    static const u8 y_first[][2] = {{2, 3}, {2, 4}, {3, 4}, {4, 4}};
    static const u8 bent_x_first[][2] = {{3, 2}, {3, 3}, {3, 4}, {2, 3}, {2, 4}, {4, 4}};
    static const u8 near_open_goal[][2] = {
        {3, 2}, {3, 3}, {2, 3}, {2, 4}, {2, 5}, {3, 5}, {4, 5}, {4, 4}
    };
    static const u8 blocked_projection[][2] = {{2, 3}, {2, 4}, {3, 4}};

    /* The starting cell stays blocked in each fixture. */
    set_grid(x_first, sizeof x_first / sizeof *x_first);
    check_route(40, 40, 72, 72, (PointRoute){1, 56, 40});
    set_grid(y_first, sizeof y_first / sizeof *y_first);
    check_route(40, 40, 72, 72, (PointRoute){1, 40, 56});
    set_grid(bent_x_first, sizeof bent_x_first / sizeof *bent_x_first);
    check_route(40, 40, 72, 72, (PointRoute){1, 56, 40});

    /* A nearer diagonal is visited first, but the open center is reachable. */
    set_grid(near_open_goal, sizeof near_open_goal / sizeof *near_open_goal);
    check_route(40, 40, 72, 72, (PointRoute){1, 40, 56});
    /* An isolated open center falls back to the first reachable diagonal. */
    invalidate_path_grid();
    SC64_GRID_SET(path_walkable, 4 + 5 * MAP_COLS, 0);
    path_grid_valid = 1;
    check_route(40, 40, 72, 72, (PointRoute){1, 56, 40});

    set_grid(blocked_projection, sizeof blocked_projection / sizeof *blocked_projection);
    check_route(40, 40, 72, 72, (PointRoute){1, 40, 56});
    set_grid(NULL, 0);
    check_route(56, 56, 64, 65, (PointRoute){1, 64, 65});
    check_route(56, 56, 79, 78, (PointRoute){1, 79, 78});
    check_route(-16, -16, -12, -24, (PointRoute){1, -12, -24});
    check_route(40, 40, 72, 72, (PointRoute){0, -123, -456});
}

static void check_authored_maps(void)
{
    static const int origins[][2] = {{585, 792}, {569, 792}, {184, 713}, {184, 697},
                                     {160, 160}, {864, 160}, {160, 864}, {864, 864}};
    game_init();
    for (int map = 0; map < ORIGINAL_MAP_COUNT; ++map) {
        original_skirmish_map = map;
        memset(b, 0, sizeof b);
        for (int seat = 0; seat < 4; ++seat) {
            b[seat].alive = 1;
            b[seat].x = original_skirmish_maps[map].start_x[seat];
            b[seat].y = original_skirmish_maps[map].start_y[seat];
        }
        invalidate_path_grid();
        build_path_grid();
        for (size_t i = 0; i < sizeof origins / sizeof *origins; ++i)
            for (int seat = 0; seat < 4; ++seat)
                check(origins[i][0], origins[i][1], b[seat].x, b[seat].y,
                      b[seat].x + 1, b[seat].y + 1);
    }
}

int main(void)
{
    check_directed_routes();
    check_authored_maps();
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
    set_grid(NULL, 0);
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
    puts("Frontier point routes match independent BFS on authored, random, blocked, edge and pixel cases");
    return 0;
}
