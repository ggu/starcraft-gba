#ifndef SC64_RANGE_WORK_H
#define SC64_RANGE_WORK_H

/* Standalone, caller-owned incremental 4-neighbour range-field worker.
 * Include after defining u8. The walk grid is one bit per tile; the gradient
 * is the route pool's existing 3-bit-per-tile encoding (0 seed, 1..4 toward
 * a seed, 7 undiscovered). Both frontier planes share one packed route field.
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
    unsigned cells, walk_bytes, gradient_bytes, frontier_bytes, bit_bytes;
    unsigned cursor, current_plane, next_count;
    int cols, rows, target_x, target_y, min_range, max_range;
    unsigned char phase;
} Sc64RangeWork;

static void sc64_range_work_reset(Sc64RangeWork *work);

static unsigned sc64_range_work_ceil8(unsigned n) { return (n + 7u) / 8u; }
static unsigned sc64_range_work_ceil3(unsigned n) { return (n * 3u + 7u) / 8u; }

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

static int sc64_range_work_begin(Sc64RangeWork *work,
                                 int cols, int rows,
                                 const u8 *walk, unsigned walk_bytes,
                                 u8 *gradient, unsigned gradient_bytes,
                                 u8 *frontier, unsigned frontier_bytes,
                                 int target_x, int target_y,
                                 int min_range, int max_range)
{
    unsigned cells, walk_need, gradient_need, frontier_need;
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
    frontier_need = 2u * walk_need;
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
    work->walk_bytes = walk_need;
    work->gradient_bytes = gradient_need;
    work->frontier_bytes = frontier_need;
    work->bit_bytes = walk_need;
    work->cols = cols;
    work->rows = rows;
    work->target_x = target_x;
    work->target_y = target_y;
    work->min_range = min_range;
    work->max_range = max_range;
    work->phase = SC64_RANGE_WORK_CLEAR_GRADIENT;
    return SC64_RANGE_WORK_PENDING;
}

static void sc64_range_work_reset(Sc64RangeWork *work)
{
    if (!work) return;
    work->walk = 0;
    work->gradient = 0;
    work->frontier = 0;
    work->cells = work->walk_bytes = work->gradient_bytes = 0;
    work->frontier_bytes = work->bit_bytes = 0;
    work->cursor = work->current_plane = work->next_count = 0;
    work->cols = work->rows = 0;
    work->target_x = work->target_y = 0;
    work->min_range = work->max_range = 0;
    work->phase = SC64_RANGE_WORK_IDLE;
}

/* One work unit is one clear byte, one seed candidate cell, or one frontier
 * byte (at most eight nodes times four neighbors). No work unit scans an
 * unbounded run. `used` reports the exact units consumed, never > budget. */
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
                work->gradient[work->cursor++] = 0xffu;
                ++done;
                continue;
            }
            work->cursor = 0;
            work->phase = SC64_RANGE_WORK_CLEAR_FRONTIER;
            continue;
        }
        if (work->phase == SC64_RANGE_WORK_CLEAR_FRONTIER) {
            if (work->cursor < work->frontier_bytes) {
                work->frontier[work->cursor++] = 0;
                ++done;
                continue;
            }
            work->cursor = 0;
            work->phase = SC64_RANGE_WORK_SEED;
            continue;
        }
        if (work->phase == SC64_RANGE_WORK_SEED) {
            unsigned cell;
            if (work->cursor < work->cells) {
                int col, row, dx, dy, d2;
                cell = work->cursor++;
                ++done;
                if (!sc64_range_work_walkable(work, cell)) continue;
                col = (int)(cell % (unsigned)work->cols);
                row = (int)(cell / (unsigned)work->cols);
                dx = col * 16 + 8 - work->target_x;
                dy = row * 16 + 8 - work->target_y;
                d2 = dx * dx + dy * dy;
                if (d2 < work->min_range * work->min_range ||
                    d2 > work->max_range * work->max_range) continue;
                sc64_range_work_put3(work->gradient, cell, 0);
                work->frontier[cell >> 3] |= (u8)(1u << (cell & 7u));
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
            unsigned char *plane = work->frontier +
                work->current_plane * work->bit_bytes;
            if (work->cursor < work->bit_bytes) {
                unsigned byte_index = work->cursor++;
                unsigned bits = plane[byte_index];
                plane[byte_index] = 0;
                ++done;
                for (unsigned bit = 0; bit < 8; ++bit) if (bits & (1u << bit)) {
                    unsigned cell = byte_index * 8u + bit;
                    int col, row;
                    if (cell >= work->cells) continue;
                    col = (int)(cell % (unsigned)work->cols);
                    row = (int)(cell / (unsigned)work->cols);
#define SC64_RANGE_WORK_EXPAND(ok, next_cell, direction) do { \
    if (ok) { unsigned n_ = (unsigned)(next_cell); \
        if (sc64_range_work_walkable(work, n_) && \
            sc64_range_work_get3(work->gradient, n_) == 7) { \
            sc64_range_work_put3(work->gradient, n_, (direction)); \
            work->frontier[(1u - work->current_plane) * work->bit_bytes + (n_ >> 3)] |= \
                (u8)(1u << (n_ & 7u)); \
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
                }
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
