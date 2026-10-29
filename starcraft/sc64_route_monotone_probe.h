/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ROUTE_MONOTONE_PROBE_H
#define SC64_ROUTE_MONOTONE_PROBE_H

/* Exact bounded witness for reverse-BFS tie order. Scratch is borrowed from
 * the caller only for this synchronous query; no search survives the call.
 * A successful Manhattan-monotone path attains the global distance lower
 * bound. Horizontal-before-vertical DFS preserves FIFO L/R/U/D path ties.
 * Unsupported inputs and exhausted budgets fall back to the full route query. */
#include "sc64_map_math.h"

typedef struct {
    unsigned nodes, candidates, memo_checks, memo_clear_bytes, seeds_tried;
} sc64_route_probe_stats;

enum { SC64_ROUTE_PROBE_BUDGET = -1, SC64_ROUTE_PROBE_UNSUPPORTED = -2 };

static inline unsigned sc64_route_probe_row(unsigned cell, unsigned cols)
{
    return sc64_map_math_width_supported(cols) ? sc64_map_cell_row(cell, cols) : cell / cols;
}

static inline unsigned sc64_route_probe_col(unsigned cell, unsigned cols, unsigned row)
{
    return cell - row * cols;
}

static int __attribute__((noinline))
sc64_route_monotone_probe(const u8 *walkable, int cols, int rows,
                          int cells, u16 *scratch, unsigned scratch_bytes,
                          int origin, int goal, unsigned budget,
                          int *predecessor, sc64_route_probe_stats *stats)
{
    int roots[9], root_count = 0, min_distance = 0x7fffffff;
    int ox, oy, gx, gy, dx, dy, root_i;
    unsigned bit_bytes, stack_offset, stack_capacity;
    u8 *failed;
    u16 *stack;

    if (!stats) return SC64_ROUTE_PROBE_UNSUPPORTED;
    stats->nodes = stats->candidates = stats->memo_checks = 0;
    stats->memo_clear_bytes = stats->seeds_tried = 0;
    if (!walkable || !scratch || !predecessor || cols <= 0 || rows <= 0 ||
        cells <= 0 || cells > 16384 || cols > 128 || rows > 128 ||
        cols * rows != cells || origin < 0 || origin >= cells || goal < 0 || goal >= cells ||
        !SC64_GRID_GET(walkable, origin)) return SC64_ROUTE_PROBE_UNSUPPORTED;
    oy = (int)sc64_route_probe_row((unsigned)origin, (unsigned)cols);
    ox = (int)sc64_route_probe_col((unsigned)origin, (unsigned)cols, (unsigned)oy);
    gy = (int)sc64_route_probe_row((unsigned)goal, (unsigned)cols);
    gx = (int)sc64_route_probe_col((unsigned)goal, (unsigned)cols, (unsigned)gy);
    if (SC64_GRID_GET(walkable, goal)) {
        roots[root_count++] = goal;
        min_distance = (ox > gx ? ox-gx : gx-ox) + (oy > gy ? oy-gy : gy-oy);
    } else {
        for (dy = -1; dy <= 1; ++dy) for (dx = -1; dx <= 1; ++dx) {
            int cx = gx + dx, cy = gy + dy, seed, distance;
            if (cx < 0 || cy < 0 || cx >= cols || cy >= rows) continue;
            seed = cx + cy * cols;
            if (!SC64_GRID_GET(walkable, seed)) continue;
            distance = (ox > cx ? ox-cx : cx-ox) + (oy > cy ? oy-cy : cy-oy);
            roots[root_count++] = seed;
            if (distance < min_distance) min_distance = distance;
        }
    }
    if (!root_count) return 0;

    failed = (u8 *)(void *)scratch;
    for (root_i = 0; root_i < root_count; ++root_i) {
        unsigned depth = 0, min_col, min_row, width, height;
        int root = roots[root_i];
        int root_row = (int)sc64_route_probe_row((unsigned)root, (unsigned)cols);
        int root_col = (int)sc64_route_probe_col((unsigned)root, (unsigned)cols, (unsigned)root_row);
        int distance = (ox > root_col ? ox-root_col : root_col-ox) +
                       (oy > root_row ? oy-root_row : root_row-oy);
        if (distance != min_distance) continue;
        min_col = (unsigned)(ox < root_col ? ox : root_col);
        min_row = (unsigned)(oy < root_row ? oy : root_row);
        width = (unsigned)(ox > root_col ? ox-root_col : root_col-ox) + 1u;
        height = (unsigned)(oy > root_row ? oy-root_row : root_row-oy) + 1u;
        bit_bytes = (width * height + 7u) / 8u;
        stack_offset = (bit_bytes + 1u) & ~1u;
        if (stack_offset > scratch_bytes) return SC64_ROUTE_PROBE_UNSUPPORTED;
        stack_capacity = (scratch_bytes - stack_offset) / sizeof(u16);
        if (stack_capacity < (unsigned)min_distance + 1u) return SC64_ROUTE_PROBE_UNSUPPORTED;
        stack = (u16 *)(void *)((u8 *)(void *)scratch + stack_offset);
        memset(failed, 0, bit_bytes);
        stats->memo_clear_bytes += bit_bytes;
        stats->seeds_tried++;
        stack[depth++] = (u16)root;
        while (depth) {
            unsigned top = depth - 1u;
            u16 state = stack[top];
            unsigned cell = state & 0x3fffu;
            unsigned row_u = sc64_route_probe_row(cell, (unsigned)cols);
            unsigned col_u = sc64_route_probe_col(cell, (unsigned)cols, row_u);
            int row = (int)row_u, col = (int)col_u;
            unsigned flags = state & 0xc000u;
            if (stats->nodes >= budget) return SC64_ROUTE_PROBE_BUDGET;
            stats->nodes++;
            if ((int)cell == origin) {
                *predecessor = depth > 1u ? (int)(stack[depth - 2u] & 0x3fffu) : origin;
                return 1;
            }
            /* Horizontal before vertical matches FIFO L/R/U/D root expansion. */
            if (!(flags & 0x4000u)) {
                stack[top] = (u16)(state | 0x4000u);
                if (col != ox) {
                    int next_col = col + (ox > col ? 1 : -1);
                    int next = (int)cell + (ox > col ? 1 : -1);
                    unsigned local = (unsigned)(row - (int)min_row) * width +
                                     (unsigned)(next_col - (int)min_col);
                    stats->candidates++;
                    if (next == origin) { *predecessor = (int)cell; return 1; }
                    stats->memo_checks++;
                    if (SC64_GRID_GET(walkable, next) &&
                        !((failed[local >> 3] >> (local & 7u)) & 1u)) {
                        if (depth >= stack_capacity) return SC64_ROUTE_PROBE_UNSUPPORTED;
                        stack[depth++] = (u16)next;
                        continue;
                    }
                }
                state = stack[top]; flags = state & 0xc000u;
            }
            if (!(flags & 0x8000u)) {
                stack[top] = (u16)(state | 0x8000u);
                if (row != oy) {
                    int next_row = row + (oy > row ? 1 : -1);
                    int next = (int)cell + (oy > row ? cols : -cols);
                    unsigned local = (unsigned)(next_row - (int)min_row) * width +
                                     (unsigned)(col - (int)min_col);
                    stats->candidates++;
                    if (next == origin) { *predecessor = (int)cell; return 1; }
                    stats->memo_checks++;
                    if (SC64_GRID_GET(walkable, next) &&
                        !((failed[local >> 3] >> (local & 7u)) & 1u)) {
                        if (depth >= stack_capacity) return SC64_ROUTE_PROBE_UNSUPPORTED;
                        stack[depth++] = (u16)next;
                        continue;
                    }
                }
                state = stack[top]; flags = state & 0xc000u;
            }
            if ((flags & 0xc000u) == 0xc000u) {
                unsigned local = (unsigned)(row - (int)min_row) * width +
                                 (unsigned)(col - (int)min_col);
                failed[local >> 3] |= (u8)(1u << (local & 7u));
                depth--;
            }
        }
    }
    return 0;
}

#endif
