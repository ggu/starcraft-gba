/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_RANGE_WORK_H
#define SC64_RANGE_WORK_H

/* Standalone, caller-owned incremental 4-neighbour range-field worker.
 * Include after defining u8. The walk grid is one bit per tile; the gradient
 * is the route pool's existing 3-bit-per-tile encoding (0 seed, 1..4 toward
 * a seed, 7 undiscovered). Both frontier planes and their occupancy summaries
 * share one packed route field.
 * This header deliberately knows nothing about engine globals or save data. */

enum {
    SC64_RANGE_WORK_ERROR = -1,
    SC64_RANGE_WORK_PENDING = 0,
    SC64_RANGE_WORK_READY = 1,
    SC64_RANGE_WORK_NO_PATH = 2
};

enum {
    SC64_RANGE_WORK_IDLE = 0,
    SC64_RANGE_WORK_CLEAR_GRADIENT,
    SC64_RANGE_WORK_CLEAR_FRONTIER,
    SC64_RANGE_WORK_SEED,
    SC64_RANGE_WORK_FLOOD,
    SC64_RANGE_WORK_DONE,
    SC64_RANGE_WORK_FAILED
};

typedef struct Sc64RangeWork {
    const u8 *walk;
    u8 *gradient;
    u8 *frontier;
    unsigned short cells, gradient_bytes, frontier_bytes, bit_bytes;
    unsigned short plane_bytes;
    unsigned short cursor, current_plane, next_count;
    int cols, rows, target_x, target_y, min_range, max_range;
    unsigned short summary_offset[5];
    unsigned short seed_left, seed_top, seed_width, seed_height, seed_total;
    unsigned short seed_col, seed_row;
    unsigned char summary_depth;
    unsigned char phase;
    /* Zero means general stride; otherwise this stores log2(cols) + 1. */
    unsigned char column_shift;
} Sc64RangeWork;

static void sc64_range_work_reset(Sc64RangeWork *work);

static unsigned sc64_range_work_ceil8(unsigned n) { return (n + 7u) / 8u; }
static unsigned sc64_range_work_ceil3(unsigned n) { return (n * 3u + 7u) / 8u; }

static unsigned sc64_range_work_plane_bytes(unsigned cells)
{
    unsigned base = sc64_range_work_ceil8(cells), bytes = base;
    if (base > 8u) while (base > 1u) {
        base = sc64_range_work_ceil8(base);
        bytes += base;
    }
    return bytes;
}

static unsigned sc64_range_work_scratch_required(unsigned cells)
{
    return 2u * sc64_range_work_plane_bytes(cells);
}

static int sc64_range_work_get3(const u8 *field, unsigned cell)
{
    unsigned bit = cell * 3u, byte = bit >> 3, shift = bit & 7u;
    unsigned pair = field[byte];
    if (shift > 5u) pair |= (unsigned)field[byte + 1u] << 8;
    return (int)((pair >> shift) & 7u);
}

static void sc64_range_work_put3(u8 *field, unsigned cell, unsigned value)
{
    unsigned bit = cell * 3u, byte = bit >> 3, shift = bit & 7u;
    unsigned pair = field[byte], mask = 7u << shift;
    if (shift > 5u) pair |= (unsigned)field[byte + 1u] << 8;
    pair = (pair & ~mask) | ((value & 7u) << shift);
    field[byte] = (u8)pair;
    if (shift > 5u) field[byte + 1u] = (u8)(pair >> 8);
}

static int sc64_range_work_walkable(const Sc64RangeWork *work, unsigned cell)
{
    return (work->walk[cell >> 3] >> (cell & 7u)) & 1u;
}

static unsigned sc64_range_work_lowbit(unsigned bits)
{
    unsigned bit = 0;
    while ((bits & 1u) == 0u) { bits >>= 1; ++bit; }
    return bit;
}

static void sc64_range_work_frontier_set(Sc64RangeWork *work,
                                        unsigned plane_index, unsigned cell)
{
    u8 *plane = work->frontier + plane_index * work->plane_bytes;
    unsigned byte_index = cell >> 3, mask = 1u << (cell & 7u);
    if (plane[byte_index] & mask) return;
    if (plane[byte_index] == 0u && work->summary_depth) {
        unsigned child_index = byte_index;
        for (unsigned level = 0; level < work->summary_depth; ++level) {
            unsigned summary_byte = child_index >> 3;
            u8 *summary = plane + work->summary_offset[level] + summary_byte;
            unsigned summary_mask = 1u << (child_index & 7u);
            int was_empty = *summary == 0;
            *summary |= (u8)summary_mask;
            if (!was_empty) break;
            child_index = summary_byte;
        }
    }
    plane[byte_index] |= (u8)mask;
}

/* Remove and return the lowest-index set cell. Flat maps inspect at most
 * eight bytes; larger maps descend a summary chain of at most four levels. */
static int sc64_range_work_frontier_pop(Sc64RangeWork *work,
                                        unsigned plane_index,
                                        unsigned *cell_out)
{
    u8 *plane = work->frontier + plane_index * work->plane_bytes;
    unsigned byte_index = 0, bits = 0, bit;
    if (work->summary_depth) {
        unsigned top = work->summary_depth - 1u;
        bits = plane[work->summary_offset[top]];
        if (!bits) return 0;
        byte_index = sc64_range_work_lowbit(bits);
        for (int level = (int)top - 1; level >= 0; --level) {
            bits = plane[work->summary_offset[level] + byte_index];
            if (!bits) return 0;
            byte_index = byte_index * 8u + sc64_range_work_lowbit(bits);
        }
        bits = plane[byte_index];
        if (!bits) return 0;
    } else {
        while (byte_index < work->bit_bytes && !plane[byte_index]) ++byte_index;
        if (byte_index == work->bit_bytes) return 0;
        bits = plane[byte_index];
    }
    bit = sc64_range_work_lowbit(bits);
    *cell_out = byte_index * 8u + bit;
    plane[byte_index] &= (u8)~(1u << bit);
    if (plane[byte_index] == 0u && work->summary_depth) {
        unsigned child_index = byte_index;
        for (unsigned level = 0; level < work->summary_depth; ++level) {
            unsigned summary_byte = child_index >> 3;
            u8 *summary = plane + work->summary_offset[level] + summary_byte;
            *summary &= (u8)~(1u << (child_index & 7u));
            if (*summary) break;
            child_index = summary_byte;
        }
    }
    return 1;
}

static int sc64_range_work_begin(Sc64RangeWork *work,
                                 int cols, int rows,
                                 const u8 *walk, unsigned walk_bytes,
                                 u8 *gradient, unsigned gradient_bytes,
                                 u8 *frontier, unsigned frontier_bytes,
                                 int target_x, int target_y,
                                 int min_range, int max_range)
{
    unsigned cells, walk_need, gradient_need, plane_need, frontier_need;
    if (!work) return SC64_RANGE_WORK_ERROR;
    sc64_range_work_reset(work);
    if (cols <= 0 || rows <= 0 || cols > 128 || rows > 128 ||
        cols * rows < 3 || !walk || !gradient || !frontier ||
        target_x < 0 || target_y < 0 || target_x >= cols * 16 ||
        target_y >= rows * 16 || min_range < 0 || max_range < min_range ||
        max_range > 4096) {
        work->phase = SC64_RANGE_WORK_FAILED;
        return SC64_RANGE_WORK_ERROR;
    }
    cells = (unsigned)cols * (unsigned)rows;
    walk_need = sc64_range_work_ceil8(cells);
    gradient_need = sc64_range_work_ceil3(cells);
    plane_need = sc64_range_work_plane_bytes(cells);
    frontier_need = 2u * plane_need;
    if (walk_bytes < walk_need || gradient_bytes < gradient_need ||
        frontier_bytes < frontier_need ||
        frontier_need > gradient_need) {
        work->phase = SC64_RANGE_WORK_FAILED;
        return SC64_RANGE_WORK_ERROR;
    }
    work->walk = walk;
    work->gradient = gradient;
    work->frontier = frontier;
    work->cells = cells;
    work->gradient_bytes = gradient_need;
    work->frontier_bytes = frontier_need;
    work->bit_bytes = walk_need;
    work->plane_bytes = plane_need;
    work->cols = cols;
    work->rows = rows;
    work->target_x = target_x;
    work->target_y = target_y;
    work->min_range = min_range;
    work->max_range = max_range;
    work->column_shift = 0;
    if (((unsigned)cols & ((unsigned)cols - 1u)) == 0u) {
        unsigned shift = 0, stride = (unsigned)cols;
        while (stride > 1u) { stride >>= 1; ++shift; }
        work->column_shift = (unsigned char)(shift + 1u);
    }
    work->seed_left = (unsigned short)(target_x > max_range ?
        (target_x - max_range) / 16 : 0);
    work->seed_top = (unsigned short)(target_y > max_range ?
        (target_y - max_range) / 16 : 0);
    {
        unsigned right = (unsigned)(target_x + max_range) / 16u;
        unsigned bottom = (unsigned)(target_y + max_range) / 16u;
        unsigned left = work->seed_left, top = work->seed_top;
        if (right >= (unsigned)cols) right = (unsigned)cols - 1u;
        if (bottom >= (unsigned)rows) bottom = (unsigned)rows - 1u;
        work->seed_width = (unsigned short)(right - left + 1u);
        work->seed_height = (unsigned short)(bottom - top + 1u);
        work->seed_total = (unsigned short)(work->seed_width * work->seed_height);
    }
    work->seed_col = work->seed_left;
    work->seed_row = work->seed_top;
    if (walk_need > 8u) {
        unsigned level_bytes = walk_need, offset = walk_need;
        while (level_bytes > 1u) {
            level_bytes = sc64_range_work_ceil8(level_bytes);
            if (work->summary_depth >= 5u) break;
            work->summary_offset[work->summary_depth++] = (unsigned short)offset;
            offset += level_bytes;
        }
    }
    work->phase = SC64_RANGE_WORK_CLEAR_GRADIENT;
    return SC64_RANGE_WORK_PENDING;
}

static void sc64_range_work_reset(Sc64RangeWork *work)
{
    if (!work) return;
    work->walk = 0;
    work->gradient = 0;
    work->frontier = 0;
    work->cells = work->gradient_bytes = 0;
    work->frontier_bytes = work->bit_bytes = 0;
    work->plane_bytes = 0;
    work->cursor = work->current_plane = work->next_count = 0;
    work->cols = work->rows = 0;
    work->target_x = work->target_y = 0;
    work->min_range = work->max_range = 0;
    memset(work->summary_offset, 0, sizeof(work->summary_offset));
    work->seed_left = work->seed_top = work->seed_width = 0;
    work->seed_height = work->seed_total = 0;
    work->seed_col = work->seed_row = 0;
    work->summary_depth = 0;
    work->phase = SC64_RANGE_WORK_IDLE;
    work->column_shift = 0;
}

/* One unit clears at most 64 bytes, tests one seed cell, or pops one frontier
 * node (summary descent is at most four levels; up to four neighbors). */
static int sc64_range_work_pump(Sc64RangeWork *work, unsigned budget,
                                unsigned *used)
{
    unsigned done = 0;
    if (used) *used = 0;
    if (!work || work->phase == SC64_RANGE_WORK_IDLE ||
        work->phase == SC64_RANGE_WORK_FAILED) return SC64_RANGE_WORK_ERROR;
    while (done < budget) {
        if (work->phase == SC64_RANGE_WORK_CLEAR_GRADIENT) {
            if (work->cursor < work->gradient_bytes) {
                unsigned count = work->gradient_bytes - work->cursor;
                if (count > 64u) count = 64u;
                memset(work->gradient + work->cursor, 0xff, count);
                work->cursor += count;
                ++done;
                continue;
            }
            work->cursor = 0;
            work->phase = SC64_RANGE_WORK_CLEAR_FRONTIER;
            continue;
        }
        if (work->phase == SC64_RANGE_WORK_CLEAR_FRONTIER) {
            if (work->cursor < work->frontier_bytes) {
                unsigned count = work->frontier_bytes - work->cursor;
                if (count > 64u) count = 64u;
                memset(work->frontier + work->cursor, 0, count);
                work->cursor += count;
                ++done;
                continue;
            }
            work->cursor = 0;
            work->phase = SC64_RANGE_WORK_SEED;
            continue;
        }
        if (work->phase == SC64_RANGE_WORK_SEED) {
            unsigned cell;
            if (work->cursor < work->seed_total) {
                int col, row, dx, dy, d2;
                col = work->seed_col;
                row = work->seed_row;
                cell = (unsigned)col + (unsigned)row * (unsigned)work->cols;
                ++work->cursor;
                ++work->seed_col;
                if (work->seed_col >= work->seed_left + work->seed_width) {
                    work->seed_col = work->seed_left;
                    ++work->seed_row;
                }
                ++done;
                if (!sc64_range_work_walkable(work, cell)) continue;
                dx = col * 16 + 8 - work->target_x;
                dy = row * 16 + 8 - work->target_y;
                d2 = dx * dx + dy * dy;
                if (d2 < work->min_range * work->min_range ||
                    d2 > work->max_range * work->max_range) continue;
                sc64_range_work_put3(work->gradient, cell, 0);
                sc64_range_work_frontier_set(work, 0u, cell);
                ++work->next_count;
                continue;
            }
            work->cursor = 0;
            work->current_plane = 0;
            if (work->next_count == 0) {
                work->phase = SC64_RANGE_WORK_DONE;
                continue;
            }
            work->next_count = 0;
            work->phase = SC64_RANGE_WORK_FLOOD;
            continue;
        }
        if (work->phase == SC64_RANGE_WORK_FLOOD) {
            unsigned cell;
            if (sc64_range_work_frontier_pop(work, work->current_plane, &cell)) {
                int col, row;
                ++done;
                if (work->column_shift) {
                    col = (int)(cell & ((unsigned)work->cols - 1u));
                    row = (int)(cell >> (work->column_shift - 1u));
                } else {
                    col = (int)(cell % (unsigned)work->cols);
                    row = (int)(cell / (unsigned)work->cols);
                }
#define SC64_RANGE_WORK_EXPAND(ok, next_cell, direction) do { \
    if (ok) { unsigned n_ = (unsigned)(next_cell); \
        if (sc64_range_work_walkable(work, n_) && \
            sc64_range_work_get3(work->gradient, n_) == 7) { \
            sc64_range_work_put3(work->gradient, n_, (direction)); \
            sc64_range_work_frontier_set(work, 1u - work->current_plane, n_); \
            ++work->next_count; \
        } \
    } \
} while (0)
                    SC64_RANGE_WORK_EXPAND(col > 0, cell - 1u, 1u);
                    SC64_RANGE_WORK_EXPAND(col + 1 < work->cols, cell + 1u, 2u);
                    SC64_RANGE_WORK_EXPAND(row > 0, cell - (unsigned)work->cols, 3u);
                    SC64_RANGE_WORK_EXPAND(row + 1 < work->rows,
                                           cell + (unsigned)work->cols, 4u);
#undef SC64_RANGE_WORK_EXPAND
                continue;
            }
            if (work->next_count == 0) {
                work->phase = SC64_RANGE_WORK_DONE;
            } else {
                work->current_plane = 1u - work->current_plane;
                work->cursor = 0;
                work->next_count = 0;
            }
            continue;
        }
        if (work->phase == SC64_RANGE_WORK_DONE) break;
        return SC64_RANGE_WORK_ERROR;
    }
    if (used) *used = done;
    return work->phase == SC64_RANGE_WORK_DONE ?
        SC64_RANGE_WORK_READY : SC64_RANGE_WORK_PENDING;
}

/* For walkable starts, READY returns the next tile toward a seed; a seed
 * returns itself. For a blocked start, READY returns a 4-bit mask of cardinal
 * exits whose gradients are already known (E,S,W,N = bits 0..3). The caller
 * chooses an exit and handles occupied-cell movement policy. */
static int sc64_range_work_query(const Sc64RangeWork *work,
                                int col, int row,
                                int *next_col, int *next_row,
                                unsigned *exit_mask)
{
    unsigned cell;
    int direction;
    unsigned mask = 0;
    if (exit_mask) *exit_mask = 0;
    if (!work || work->phase == SC64_RANGE_WORK_IDLE ||
        work->phase == SC64_RANGE_WORK_FAILED || col < 0 || row < 0 ||
        col >= work->cols || row >= work->rows) return SC64_RANGE_WORK_ERROR;
    if (work->phase != SC64_RANGE_WORK_FLOOD &&
        work->phase != SC64_RANGE_WORK_DONE) return SC64_RANGE_WORK_PENDING;
    cell = (unsigned)col + (unsigned)row * (unsigned)work->cols;
    if (sc64_range_work_walkable(work, cell)) {
        direction = sc64_range_work_get3(work->gradient, cell);
        if (direction == 7)
            return work->phase == SC64_RANGE_WORK_DONE ?
                SC64_RANGE_WORK_NO_PATH : SC64_RANGE_WORK_PENDING;
        if (!next_col || !next_row) return SC64_RANGE_WORK_ERROR;
        *next_col = col;
        *next_row = row;
        if (direction == 1) ++*next_col;
        else if (direction == 2) --*next_col;
        else if (direction == 3) ++*next_row;
        else if (direction == 4) --*next_row;
        else if (direction != 0) return SC64_RANGE_WORK_ERROR;
        return SC64_RANGE_WORK_READY;
    }
#define SC64_RANGE_WORK_EXIT(ok, ncell, bit) do { \
    if (ok) { unsigned n_ = (unsigned)(ncell); \
        if (sc64_range_work_walkable(work, n_) && \
            sc64_range_work_get3(work->gradient, n_) != 7) mask |= 1u << (bit); \
    } \
} while (0)
    SC64_RANGE_WORK_EXIT(col + 1 < work->cols, cell + 1u, 0u);
    SC64_RANGE_WORK_EXIT(row + 1 < work->rows, cell + (unsigned)work->cols, 1u);
    SC64_RANGE_WORK_EXIT(col > 0, cell - 1u, 2u);
    SC64_RANGE_WORK_EXIT(row > 0, cell - (unsigned)work->cols, 3u);
#undef SC64_RANGE_WORK_EXIT
    if (mask) {
        if (!exit_mask) return SC64_RANGE_WORK_ERROR;
        *exit_mask = mask;
        return SC64_RANGE_WORK_READY;
    }
    return work->phase == SC64_RANGE_WORK_DONE ?
        SC64_RANGE_WORK_NO_PATH : SC64_RANGE_WORK_PENDING;
}

#endif
