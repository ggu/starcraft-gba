/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FRONTIER_ROUTE_CACHE_H
#define FRONTIER_ROUTE_CACHE_H

/* Direct-mapped transient point-query memo. Results -1 and -2 mean failure
 * and return the caller's original target pixels, respectively. */
typedef struct {
    u16 start, goal, generation;
    short result;
} FrontierRouteCacheEntry;
static FrontierRouteCacheEntry frontier_route_cache[64];
static u16 frontier_route_cache_generation = 1;

static void frontier_route_cache_invalidate(void)
{
    if (++frontier_route_cache_generation == 0) {
        memset(frontier_route_cache, 0, sizeof(frontier_route_cache));
        frontier_route_cache_generation = 1;
    }
}

static FrontierRouteCacheEntry *frontier_route_cache_slot(int start, int goal)
{
    unsigned hash = (unsigned)start * 37u + (unsigned)goal * 101u;
    hash ^= hash >> 6;
    return &frontier_route_cache[hash & 63u];
}

/* A clear horizontal-then-vertical route reaches the Manhattan lower bound
 * and starts in the direction that the breadth-first neighbor order prefers.
 * For a blocked goal the projection into its 3x3 approach area is uniquely
 * closest. If any tile on this route is blocked, let the full search decide. */
static int frontier_shortest_axis_step(int col, int row, int goal_col, int goal_row)
{
    if (!SC64_GRID_GET(path_walkable, goal_col + goal_row * MAP_COLS)) {
        goal_col = limit(col, goal_col - 1, goal_col + 1);
        goal_row = limit(row, goal_row - 1, goal_row + 1);
    }
    if (col == goal_col && row == goal_row) return -2;

    int col_step = goal_col > col ? 1 : -1;
    int row_step = goal_row > row ? 1 : -1;
    int first = col != goal_col ? col + col_step + row * 256 : col + (row + row_step) * 256;
    int cell = col + row * MAP_COLS;
    while (col != goal_col) {
        col += col_step;
        cell += col_step;
        if (!SC64_GRID_GET(path_walkable, cell)) return -1;
    }
    while (row != goal_row) {
        row += row_step;
        cell += row_step * MAP_COLS;
        if (!SC64_GRID_GET(path_walkable, cell)) return -1;
    }
    return first;
}

static int frontier_route_result(int result, int tx, int ty, int *next_x, int *next_y)
{
    if (result == -1) return 0;
    if (result == -2) { *next_x = tx; *next_y = ty; }
    else { *next_x = (result & 255) * 16 + 8; *next_y = (result >> 8) * 16 + 8; }
    return 1;
}

/* -1 means miss, 0 means cached failure, 1 means cached success. */
static int frontier_route_cache_get(int start, int goal, int tx, int ty,
                                    int *next_x, int *next_y)
{
    FrontierRouteCacheEntry *entry = frontier_route_cache_slot(start, goal);
    if (entry->generation != frontier_route_cache_generation ||
        entry->start != start || entry->goal != goal) return -1;
    return frontier_route_result(entry->result, tx, ty, next_x, next_y);
}

static void frontier_route_cache_put(int start, int goal, int result)
{
    FrontierRouteCacheEntry *entry = frontier_route_cache_slot(start, goal);
    entry->start = (u16)start;
    entry->goal = (u16)goal;
    entry->result = (short)result;
    entry->generation = frontier_route_cache_generation;
}
#endif
