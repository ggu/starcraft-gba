/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ROUTE_COMPRESSED_POINT_H
#define SC64_ROUTE_COMPRESSED_POINT_H

enum {
    SC64_COMPRESSED_SIDE = 128,
    SC64_COMPRESSED_LIMIT = 128,
    SC64_COMPRESSED_UNSEEN = 0xffff
};

/* Axis boundaries where at least one pair of neighboring cells differs.
 * A retained run endpoint (including both query positions) can represent all
 * turns in the run. A straight link across omitted coordinates costs its
 * original length, including every skipped coordinate. */
static unsigned
sc64_compressed_axis(u8 *values, const u32 *changes, unsigned first,
                     unsigned last, unsigned *first_index, unsigned *last_index)
{
    unsigned count = 0, previous = 1;
    for (unsigned i = 0; i < SC64_COMPRESSED_SIDE; ++i) {
        unsigned next = (changes[i >> 5] >> (i & 31u)) & 1u;
        if (previous || next || i == first || i == last ||
            i == SC64_COMPRESSED_SIDE - 1) {
            if (i == first) *first_index = count;
            if (i == last) *last_index = count;
            values[count++] = (u8)i;
        }
        previous = next;
    }
    return count;
}

static int
sc64_compressed_cell_open(const u8 *walk, unsigned x, unsigned y)
{
    return (walk[y * 16u + (x >> 3)] >> (x & 7u)) & 1u;
}

/* Use four-way neighbor priority from the reverse field even when distance
 * edges jump across a run of identical axes. Return an inverse unit step. */
static unsigned
sc64_compressed_neighbor(const u8 *xs, const u8 *ys, unsigned cols,
                         unsigned rows, unsigned x, unsigned y, unsigned dir,
                         unsigned *weight)
{
    switch (dir) {
    case 0:
        if (!x) break;
        *weight = xs[x] - xs[x - 1];
        return y * cols + x - 1;
    case 1:
        if (x + 1 == cols) break;
        *weight = xs[x + 1] - xs[x];
        return y * cols + x + 1;
    case 2:
        if (!y) break;
        *weight = ys[y] - ys[y - 1];
        return (y - 1) * cols + x;
    case 3:
        if (y + 1 == rows) break;
        *weight = ys[y + 1] - ys[y];
        return (y + 1) * cols + x;
    }
    return SC64_COMPRESSED_LIMIT;
}

/* Dijkstra runs from the query origin. Walking from the goal down those
 * distances with FIFO neighbor priority gives the lexicographic shortest
 * path from the goal. In a uniform column strip, vertical moves commute
 * after horizontal moves. In a uniform row strip, horizontal moves commute
 * before vertical moves. This puts all turns of the chosen path on retained
 * coordinates, with no need to change the field or its parked FIFO. */
static int __attribute__((noinline))
sc64_compressed_point_direction(const u8 *walk, unsigned origin, unsigned goal,
                                 u16 *scratch)
{
    const u8 *aligned = __builtin_assume_aligned(walk, sizeof(u32));
    u32 x_changes[4] = {0}, y_changes[4] = {0}, previous[4] = {0};
    u8 xs[SC64_COMPRESSED_SIDE], ys[SC64_COMPRESSED_SIDE];
    unsigned ox = 0, oy = 0, gx = 0, gy = 0;
    if (origin == goal || !sc64_compressed_cell_open(walk, origin & 127u, origin >> 7) ||
        !sc64_compressed_cell_open(walk, goal & 127u, goal >> 7)) return -1;

    for (unsigned y = 0; y < SC64_COMPRESSED_SIDE; ++y) {
        u32 row[4];
        __builtin_memcpy(row, aligned + y * 16u, sizeof(row));
        if (y && ((row[0] ^ previous[0]) | (row[1] ^ previous[1]) |
                  (row[2] ^ previous[2]) | (row[3] ^ previous[3]))) {
            unsigned edge = y - 1;
            y_changes[edge >> 5] |= 1u << (edge & 31u);
        }
        for (unsigned i = 0; i < 3; ++i)
            x_changes[i] |= row[i] ^ ((row[i] >> 1) | (row[i + 1] << 31));
        x_changes[3] |= (row[3] ^ (row[3] >> 1)) & 0x7fffffffu;
        __builtin_memcpy(previous, row, sizeof(row));
    }
    unsigned cols = sc64_compressed_axis(xs, x_changes, origin & 127u, goal & 127u,
                                         &ox, &gx);
    unsigned rows = sc64_compressed_axis(ys, y_changes, origin >> 7, goal >> 7,
                                         &oy, &gy);
    unsigned nodes = rows * cols;
    if (nodes > SC64_COMPRESSED_LIMIT) return -1;

    /* The compact graph is small enough for a linear priority scan. Keep
     * occupancy and visited bits beside its distances in BFS head scratch. */
    u16 *distance = scratch;
    u16 *pending = distance + nodes;
    for (unsigned y = 0, slot = 0; y < rows; ++y) {
        for (unsigned x = 0; x < cols; ++x, ++slot) {
            distance[slot] = SC64_COMPRESSED_UNSEEN;
            pending[slot] = (u16)sc64_compressed_cell_open(walk, xs[x], ys[y]);
        }
    }
    unsigned start = oy * cols + ox, end = gy * cols + gx;
    distance[start] = 0;
    for (;;) {
        unsigned at = SC64_COMPRESSED_LIMIT, best = SC64_COMPRESSED_UNSEEN;
        for (unsigned i = 0; i < nodes; ++i) {
            if (pending[i] && distance[i] < best) {
                best = distance[i];
                at = i;
            }
        }
        if (at == SC64_COMPRESSED_LIMIT) return 7;
        if (at == end) break;
        pending[at] = 0;
        unsigned y = at / cols, x = at - y * cols;
        for (unsigned dir = 0; dir < 4; ++dir) {
            unsigned weight, next = sc64_compressed_neighbor(xs, ys, cols, rows,
                                                            x, y, dir, &weight);
            if (next < nodes && pending[next] && best + weight < distance[next])
                distance[next] = (u16)(best + weight);
        }
    }

    while (end != start) {
        unsigned y = end / cols, x = end - y * cols;
        unsigned next = SC64_COMPRESSED_LIMIT, dir;
        for (dir = 0; dir < 4; ++dir) {
            unsigned weight, candidate = sc64_compressed_neighbor(xs, ys, cols, rows,
                                                                 x, y, dir, &weight);
            if (candidate < nodes && distance[candidate] + weight == distance[end]) {
                next = candidate;
                break;
            }
        }
        if (next == start) return (int)dir + 1;
        if (next == SC64_COMPRESSED_LIMIT) return -1;
        end = next;
    }
    return -1;
}
#endif
