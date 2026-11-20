/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ROUTE_POINT_FAR_READ_H
#define SC64_ROUTE_POINT_FAR_READ_H

static inline int sc64_point_far_gap(int a, int b)
{
    return a > b ? a - b : b - a;
}

/* Look along the preferred horizontal-then-vertical route for an interior
 * separator. Any path crosses that entire column or row. Accept only a
 * unique walkable crossing that minimizes the via-cell Manhattan bound. */
static int sc64_point_far_pivot(int gx, int gy, int ox, int oy)
{
    const int cols = MAP_COLS, rows = MAP_ROWS;
    const int x_step = gx < ox ? 1 : -1, y_step = gy < oy ? 1 : -1;
    const int x_gap = sc64_point_far_gap(gx, ox), y_gap = sc64_point_far_gap(gy, oy);
    int separator = -1, along_col = 1;
    for (int i = 1; i < x_gap; ++i) {
        int col = gx + i * x_step;
        if (!SC64_GRID_GET(path_walkable, col + gy * cols)) {
            separator = col;
            break;
        }
    }
    if (separator < 0) {
        along_col = 0;
        for (int i = 1; i < y_gap; ++i) {
            int row = gy + i * y_step;
            if (!SC64_GRID_GET(path_walkable, ox + row * cols)) {
                separator = row;
                break;
            }
        }
    }
    if (separator < 0) return -1;

    int best = -1, lower = 0x7fffffff;
    const int count = along_col ? rows : cols;
    const int start = along_col ? separator : separator * cols;
    const int step = along_col ? cols : 1;
    const int from = along_col ? gy : gx, to = along_col ? oy : ox;
    for (int i = 0; i < count; ++i) {
        int cell = start + i * step;
        if (!SC64_GRID_GET(path_walkable, cell)) continue;
        int distance = sc64_point_far_gap(i, from) + sc64_point_far_gap(i, to);
        if (distance < lower) { lower = distance; best = cell; }
        else if (distance == lower) best = -1;
    }
    return best;
}

/* Reuse the exact monotone oracle from the point movement path. Both legs
 * of a shortest path through a unique minimum crossing are monotone when
 * they attain the via-cell lower bound. The normal goal-rooted FIFO then
 * chooses the first lexicographic path on each leg. If the proof declines,
 * the existing cached-reader path can still materialize the packed field. */
static int __attribute__((noinline))
sc64_point_far_read(int origin, int goal, int *direction)
{
    enum { NEAR_GAP = 16, NODE_ALLOWANCE = 512 };
    const int cols = MAP_COLS, rows = MAP_ROWS;
    const int gy = sc64_cell_row((unsigned)goal), gx = goal - gy * cols;
    const int oy = sc64_cell_row((unsigned)origin), ox = origin - oy * cols;
    if (sc64_point_far_gap(gx, ox) + sc64_point_far_gap(gy, oy) <= NEAR_GAP ||
        !SC64_GRID_GET(path_walkable, goal) ||
        !SC64_GRID_GET(path_walkable, origin)) return 0;

    const unsigned scratch_bytes = sizeof(bfs_queue)
#ifdef SC64_ROUTE_POINT_TAIL_PROBE
                                   - SC64_ROUTE_POINT_BYTES
#endif
                                   ;
    const int pivot = sc64_point_far_pivot(gx, gy, ox, oy);
    unsigned remaining = NODE_ALLOWANCE;
    int predecessor = -1;
    sc64_route_probe_stats stats;
    if (pivot >= 0) {
        if (sc64_route_monotone_probe(path_walkable, cols, rows, cols * rows,
                                      bfs_queue, scratch_bytes, pivot, goal,
                                      remaining, &predecessor, &stats) != 1) return 0;
        remaining -= stats.nodes;
    }
    if (sc64_route_monotone_probe(path_walkable, cols, rows, cols * rows,
                                  bfs_queue, scratch_bytes, origin, pivot >= 0 ? pivot : goal,
                                  remaining, &predecessor, &stats) != 1) return 0;

    const int step = predecessor - origin;
    if (step == 1) *direction = 1;
    else if (step == -1) *direction = 2;
    else if (step == cols) *direction = 3;
    else if (step == -cols) *direction = 4;
    else return 0;
    return 1;
}

#endif
