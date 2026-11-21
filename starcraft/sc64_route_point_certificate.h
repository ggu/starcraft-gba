/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ROUTE_POINT_CERTIFICATE_H
#define SC64_ROUTE_POINT_CERTIFICATE_H

/* Search only paths up to four steps longer than the Manhattan lower
 * bound. A path meeting that bound cannot stray more than two cells
 * outside the bounding box of the origin and possible seeds, since it
 * would have to undo each step outside. The same seed and neighbor order
 * finds the canonical path. Any shorter or equal path is inside the box. */
static int __attribute__((noinline))
sc64_point_certificate(int slot, int origin, int goal, int clear)
{
    enum { MAX_GAP = 16, MAX_RETRY_GAP = 18, EXTRA_STEPS = 4, MARGIN = EXTRA_STEPS / 2,
           MAX_SIDE = MAX_GAP + 3 + 2 * MARGIN + 2,
           MAX_CELLS = MAX_SIDE * MAX_SIDE };
    const int cols = MAP_COLS, rows = MAP_ROWS;
    if ((unsigned)origin >= (unsigned)(cols * rows) ||
        !SC64_GRID_GET(path_walkable, origin)) return 0;
    const int oy = sc64_cell_row((unsigned)origin), ox = origin - oy * cols;
    const int gy = sc64_cell_row((unsigned)goal), gx = goal - gy * cols;
    const int dx = ox > gx ? ox - gx : gx - ox;
    const int dy = oy > gy ? oy - gy : gy - oy;
    const int gap = dx + dy;
    /* A parked field may try a slightly wider proof on a cache hit.
     * Initial insertions keep their existing certification limit. */
    if (gap > (clear ? MAX_GAP : MAX_RETRY_GAP)) return 0;

    const int spread = !SC64_GRID_GET(path_walkable, goal);
    const int seed_left = gx > 0 ? gx - spread : gx;
    const int seed_right = gx + 1 < cols ? gx + spread : gx;
    const int seed_top = gy > 0 ? gy - spread : gy;
    const int seed_bottom = gy + 1 < rows ? gy + spread : gy;
    const int left = limit((ox < seed_left ? ox : seed_left) - MARGIN, 0, cols - 1);
    const int right = limit((ox > seed_right ? ox : seed_right) + MARGIN, 0, cols - 1);
    const int top = limit((oy < seed_top ? oy : seed_top) - MARGIN, 0, rows - 1);
    const int bottom = limit((oy > seed_bottom ? oy : seed_bottom) + MARGIN, 0, rows - 1);
    const int stride = right - left + 3;
    const int height = bottom - top + 3;
    if (gap > MAX_GAP && stride * height > MAX_CELLS) return 0;

    /* The prefix expansion uses this scratch after this function returns.
     * The independent walkability grid and the borrowed tail stay intact. */
#ifdef SC64_ROUTE_POINT_TAIL_PROBE
    _Static_assert(MAX_CELLS * 3u <= sizeof(bfs_queue) - SC64_ROUTE_POINT_BYTES,
                   "the certificate must leave the parked point tail intact");
#else
    _Static_assert(MAX_CELLS * 3u <= sizeof(bfs_queue), "certificate scratch must fit");
#endif
    u8 *dense = (u8 *)(void *)(bfs_queue + MAX_CELLS);
    memset(dense, 0, (size_t)(stride * height));
    for (int y = top; y <= bottom; ++y) {
        const int row = y * cols;
        u8 *cell = dense + (y - top + 1) * stride + 1;
        for (int x = left; x <= right; ++x)
            *cell++ = SC64_GRID_GET(path_walkable, row + x) ? 7 : 0;
    }

    int lower = gap > MAX_GAP ? gap + 3 : MAX_GAP + 1;
    int head = 0, tail = 0;
    for (int y = seed_top; y <= seed_bottom; ++y) {
        for (int x = seed_left; x <= seed_right; ++x) {
            int seed = (y - top + 1) * stride + x - left + 1;
            if (dense[seed] != 7) continue;
            int distance = (x > ox ? x - ox : ox - x) + (y > oy ? y - oy : oy - y);
            if (distance < lower) lower = distance;
            dense[seed] = 0;
            bfs_queue[tail++] = (u16)seed;
        }
    }
    const int start = (oy - top + 1) * stride + ox - left + 1;
    for (int distance = 0; distance < lower + EXTRA_STEPS && head < tail && dense[start] == 7;
         ++distance) {
        const int end = tail;
        do {
            int cell = bfs_queue[head++];
#define SC64_CERT_STEP(offset, direction) \
            if (dense[cell + (offset)] == 7) { \
                dense[cell + (offset)] = direction; \
                bfs_queue[tail++] = (u16)(cell + (offset)); \
            }
            SC64_CERT_STEP(-1, 1)
            SC64_CERT_STEP(1, 2)
            SC64_CERT_STEP(-stride, 3)
            SC64_CERT_STEP(stride, 4)
#undef SC64_CERT_STEP
        } while (head != end && dense[start] == 7);
    }

    if (dense[start] == 7) return 0;
    if (clear) sc64_route_field_clear(sc64_route_field[slot]);
    /* Every vertex on the canonical path retains its canonical path to
     * the seed. Publishing the whole chain also supports gradient readers. */
    const int packed_step[] = {0, 1, -1, cols, -cols};
    const int dense_step[] = {0, 1, -1, stride, -stride};
    int cell = origin, at = start;
    for (;;) {
        int direction = dense[at];
        sc64_route_put(slot, cell, direction);
        if (!direction) return 1;
        cell += packed_step[direction];
        at += dense_step[direction];
    }
}

#endif
