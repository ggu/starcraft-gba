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

#if defined(SC64_ROUTE_MONOTONE_CLEAR_PREFIX) && !defined(SC64_ROUTE_MONOTONE_WITNESS_ENABLED) && \
    !defined(SC64_ROUTE_MONOTONE_DIAGNOSTICS)
#define SC64_ROUTE_MONOTONE_CLEAR_PREFIX_ENABLED 1
#endif

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
        enum { TRY_BOTH, TRY_VERTICAL, FINISHED };
        unsigned depth = 1;
        int root = roots[root_i];
        int root_row = (int)sc64_route_probe_row((unsigned)root, (unsigned)cols);
        int root_col = (int)sc64_route_probe_col((unsigned)root, (unsigned)cols, (unsigned)root_row);
        int distance = (ox > root_col ? ox-root_col : root_col-ox) +
                       (oy > root_row ? oy-root_row : root_row-oy);
        if (distance != min_distance) continue;
        unsigned horizontal = (unsigned)(ox > root_col ? ox-root_col : root_col-ox);
        unsigned vertical = (unsigned)(oy > root_row ? oy-root_row : root_row-oy);
        unsigned width = horizontal + 1u, height = vertical + 1u;
        unsigned local = width * height - 1u;
        int x_step = ox > root_col ? 1 : -1;
        int y_step = oy > root_row ? cols : -cols;
        int cell = root;
        bit_bytes = (width * height + 7u) / 8u;
        stack_offset = (bit_bytes + 1u) & ~1u;
        if (stack_offset > scratch_bytes) return SC64_ROUTE_PROBE_UNSUPPORTED;
        stack_capacity = (scratch_bytes - stack_offset) / sizeof(u16);
        if (stack_capacity < (unsigned)min_distance + 1u) return SC64_ROUTE_PROBE_UNSUPPORTED;
#ifdef SC64_ROUTE_MONOTONE_CLEAR_PREFIX_ENABLED
        unsigned prefix_x = 0, prefix_y = 0;
        if (distance > 0 && stats->nodes <= budget &&
            (unsigned)distance <= budget - stats->nodes) {
            while (prefix_x < horizontal && SC64_GRID_GET(walkable, cell + x_step)) {
                cell += x_step;
                ++prefix_x;
            }
            if (prefix_x == horizontal) {
                while (prefix_y < vertical && SC64_GRID_GET(walkable, cell + y_step)) {
                    cell += y_step;
                    ++prefix_y;
                }
                if (prefix_y == vertical) {
                    *predecessor = origin - (vertical ? y_step : x_step);
                    stats->nodes += (unsigned)distance;
                    stats->candidates += (unsigned)distance;
                    stats->memo_checks += (unsigned)distance - 1u;
                    ++stats->seeds_tried;
                    return 1;
                }
            }
        }
#endif
        stack = (u16 *)(void *)((u8 *)(void *)scratch + stack_offset);
        memset(failed, 0, bit_bytes);
        stats->memo_clear_bytes += bit_bytes;
        stats->seeds_tried++;
        if (!distance) {
            if (stats->nodes >= budget) return SC64_ROUTE_PROBE_BUDGET;
            stats->nodes++;
            *predecessor = origin;
            return 1;
        }
        /* Bitmap coordinates count steps remaining toward the origin.
         * The parent's phase identifies the edge to undo on backtracking. */
#ifdef SC64_ROUTE_MONOTONE_CLEAR_PREFIX_ENABLED
        /* Resume the exact DFS at the obstruction with the same ancestor
         * phases. The clear prefix has not read or written the memo. */
        for (unsigned i = 0; i < prefix_x; ++i) stack[i] = TRY_VERTICAL;
        unsigned prefix = prefix_x + prefix_y;
        for (unsigned i = prefix_x; i < prefix; ++i) stack[i] = FINISHED;
        depth += prefix;
        horizontal -= prefix_x;
        vertical -= prefix_y;
        local -= prefix_x + prefix_y * width;
        stats->nodes += prefix;
        stats->candidates += prefix;
        stats->memo_checks += prefix;
#endif
        stack[depth - 1] = TRY_BOTH;
        while (depth) {
            unsigned top = depth - 1u;
            unsigned phase = stack[top];
            if (stats->nodes >= budget) return SC64_ROUTE_PROBE_BUDGET;
            stats->nodes++;
            /* Horizontal before vertical matches FIFO L/R/U/D root expansion. */
            if (phase == TRY_BOTH) {
                stack[top] = TRY_VERTICAL;
                if (horizontal) {
                    int next = cell + x_step;
                    unsigned next_local = local - 1u;
                    stats->candidates++;
                    if (next == origin) { *predecessor = cell; return 1; }
                    stats->memo_checks++;
                    if (SC64_GRID_GET(walkable, next) &&
                        !((failed[next_local >> 3] >> (next_local & 7u)) & 1u)) {
                        if (depth >= stack_capacity) return SC64_ROUTE_PROBE_UNSUPPORTED;
                        cell = next;
                        local = next_local;
                        horizontal--;
                        stack[depth++] = TRY_BOTH;
                        continue;
                    }
                }
            }
            if (phase != FINISHED) {
                stack[top] = FINISHED;
                if (vertical) {
                    int next = cell + y_step;
                    unsigned next_local = local - width;
                    stats->candidates++;
                    if (next == origin) { *predecessor = cell; return 1; }
                    stats->memo_checks++;
                    if (SC64_GRID_GET(walkable, next) &&
                        !((failed[next_local >> 3] >> (next_local & 7u)) & 1u)) {
                        if (depth >= stack_capacity) return SC64_ROUTE_PROBE_UNSUPPORTED;
                        cell = next;
                        local = next_local;
                        vertical--;
                        stack[depth++] = TRY_BOTH;
                        continue;
                    }
                }
            }
            failed[local >> 3] |= (u8)(1u << (local & 7u));
            if (--depth == 0) break;
            if (stack[depth - 1u] == FINISHED) {
                cell -= y_step;
                local += width;
                vertical++;
            } else {
                cell -= x_step;
                local++;
                horizontal++;
            }
        }
    }
    return 0;
}

#endif
