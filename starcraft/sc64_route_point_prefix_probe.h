/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ROUTE_POINT_PREFIX_PROBE_H
#define SC64_ROUTE_POINT_PREFIX_PROBE_H

static int sc64_route_fill(int slot, int goal);

#ifdef SC64_ROUTE_POINT_COLD_CERTIFICATE
#include "sc64_route_point_certificate.h"
enum { SC64_POINT_CERTIFIED_ONLY = 255 };
#endif

static Sc64PointPrefix *sc64_point_prefix_find(int slot)
{
    unsigned id = sc64_point_prefix_index[slot];
    if (!id) return 0;
    Sc64PointPrefix *prefix = &sc64_point_prefixes[id - 1];
    if (prefix->count && sc64_route_valid[slot] == 1 &&
        sc64_route_goal[slot] == prefix->goal) return prefix;
    return 0;
}

static void sc64_point_prefix_discard(Sc64PointPrefix *prefix)
{
    if (!prefix->count) return;
#ifdef SC64_ROUTE_POINT_NEAR_RESERVE_PROBE
    if (sc64_point_prefix_index[prefix->slot] == (unsigned)(prefix - sc64_point_prefixes + 1))
        sc64_point_prefix_index[prefix->slot] = 0;
#else
    sc64_point_prefix_index[prefix->slot] = 0;
#endif
    prefix->count = 0;
}

static void sc64_point_prefix_forget(int slot)
{
    unsigned id = sc64_point_prefix_index[slot];
    if (id) sc64_point_prefix_discard(&sc64_point_prefixes[id - 1]);
}

static Sc64PointPrefix *sc64_point_prefix_free(void)
{
    for (unsigned i = 0; i < SC64_POINT_PREFIX_SLOTS; ++i) {
        Sc64PointPrefix *prefix = &sc64_point_prefixes[i];
        /* A range snapshot or facility query may have repurposed the slot. */
        if (!prefix->count || sc64_route_valid[prefix->slot] != 1 ||
            sc64_route_goal[prefix->slot] != prefix->goal) return prefix;
    }
    return 0;
}

/* The caller already checked stride and the open origin. Check for it after
 * whole 32-pop batches, preserving seed and neighbour order even on decline.
 * Showcase keeps this loop in ROM to reserve its IWRAM stack. */
static int
#ifndef SC64_SHOWCASE_ENABLED
GBA_HOT
#endif
__attribute__((noinline))
sc64_point_prefix_expand(u8 *dense, int start, int stride, int cap,
                         int *head, int tail)
{
    while (*head < tail && *head < SC64_POINT_PREFIX_LIMIT && dense[start] == 7u) {
        int limit = *head + (cap - tail) / 4;
        if (limit > tail) limit = tail;
        if (limit > *head + SC64_POINT_PREFIX_BATCH)
            limit = *head + SC64_POINT_PREFIX_BATCH;
        if (limit > SC64_POINT_PREFIX_LIMIT) limit = SC64_POINT_PREFIX_LIMIT;
        if (limit <= *head) break;
        const u16 *read = bfs_queue + *head, *end = bfs_queue + limit;
        u16 *write = bfs_queue + tail;
        do {
            int cell = *read++;
#define SC64_PREFIX_STEP(offset, direction) \
            if (dense[cell + (offset)] == 7u) { \
                dense[cell + (offset)] = direction; \
                *write++ = (u16)(cell + (offset)); \
            }
            SC64_PREFIX_STEP(-1, 1)
            SC64_PREFIX_STEP(1, 2)
            SC64_PREFIX_STEP(-stride, 3)
            SC64_PREFIX_STEP(stride, 4)
#undef SC64_PREFIX_STEP
        } while (read != end);
        *head = limit;
        tail = (int)(write - bfs_queue);
    }
    return tail;
}

static int __attribute__((noinline)) sc64_point_prefix_fill(int slot, int goal, int origin)
{
    const int cols = MAP_COLS, rows = MAP_ROWS;
    sc64_point_prefix_forget(slot);
    if (cols != SC64_LINEAR_WIDE_COLS || rows != SC64_LINEAR_MAX_ROWS ||
        sc64_route_pcount || origin < 0 || origin >= cols * rows ||
        !SC64_GRID_GET(path_walkable, origin)) return sc64_route_fill(slot, goal);
#ifdef SC64_ROUTE_LINEAR_EMPTY_GOAL_PROBE
    /* No prefix can be parked without a seed. Use the normal empty fill
     * before borrowing dense scratch. It publishes the usual field metadata. */
    if (sc64_route_linear_goal_is_empty(goal, cols, rows))
        return sc64_route_fill(slot, goal);
#endif
    Sc64PointPrefix *prefix = sc64_point_prefix_free();
    if (!prefix) return sc64_route_fill(slot, goal);
#ifdef SC64_ROUTE_POINT_COLD_CERTIFICATE
    if (sc64_point_certificate(slot, origin, goal, 1)) {
        sc64_point_prefix_discard(prefix);
        prefix->goal = (u16)goal;
        prefix->slot = (u8)slot;
        prefix->count = SC64_POINT_CERTIFIED_ONLY;
        sc64_point_prefix_index[slot] = (u8)(prefix - sc64_point_prefixes + 1);
        SC64_ROUTE_LOGN(slot) = sc64_route_open_count;
        SC64_ROUTE_PF(slot) = 0;
        SC64_ROUTE_KEEP_COUNT(FILL);
        return 0;
    }
#endif
    const int stride = cols + SC64_LINEAR_PAD;
    const int cap = sc64_route_linear_queue_cap(cols, rows);
    u8 *field = sc64_route_field[slot];
    u8 *dense = sc64_route_linear_grid(cols, rows, stride, cap);
    int goal_row = sc64_cell_row(goal);
    int goal_start = goal + goal_row * SC64_LINEAR_PAD + stride + 4;
    int origin_start = origin + sc64_cell_row(origin) * SC64_LINEAR_PAD + stride + 4;
    int head = 0, tail = sc64_route_linear_seed(dense, goal_start, stride);
    tail = sc64_point_prefix_expand(dense, origin_start, stride, cap, &head, tail);
    int pending = tail - head;
    if (pending > 0 && pending <= SC64_POINT_PREFIX_PENDING && dense[origin_start] != 7u) {
        sc64_route_field_clear(field);
        for (int i = 0; i < tail; ++i) {
            int at = bfs_queue[i];
            /* A queued interior cell has coordinates row 1..128 and
             * column 4..131 at stride 136. Since 136*241 = 32768+8 and
             * 8*128+241*131 < 32768, this recovers the exact row. */
            int dense_row = (int)(((unsigned)at * 241u) >> 15);
            int cell = at - dense_row * SC64_LINEAR_PAD - cols - 4;
            sc64_route_put_new(field, cell, dense[at]);
        }
        sc64_point_prefix_discard(prefix);
        prefix->goal = (u16)goal;
        prefix->slot = (u8)slot;
#ifdef SC64_ROUTE_POINT_SECOND_EXACT_HIT
        prefix->second_cutoff = 0;
#endif
        /* Each discovered cell lies at most head edges from a seed. The
         * blocked-goal seeds can start one row away from the goal. Include
         * those rows without bookkeeping in the pop or packing loops. */
        int radius = head + (dense[goal_start] != 0);
        prefix->restore_first_row = (u8)limit(goal_row + 1 - radius, 1, rows);
        prefix->restore_last_row = (u8)limit(goal_row + 1 + radius, 1, rows);
        memcpy(prefix->queue, bfs_queue + head, (size_t)pending * sizeof(*bfs_queue));
        prefix->count = (u8)pending;
        sc64_point_prefix_index[slot] = (u8)(prefix - sc64_point_prefixes + 1);
    } else {
        tail = sc64_route_expand_linear_from(dense, tail, stride, cap, head);
        if (tail < 0) return sc64_route_fill(slot, goal);
#ifdef SC64_ROUTE_POINT_PREFIX_SPARSE_PACK_PROBE
        sc64_route_pack_linear_complete(field, dense, cols, rows, stride, cap, tail);
#else
        sc64_route_pack_linear(field, dense, cols, rows, stride);
#endif
    }
    SC64_ROUTE_LOGN(slot) = sc64_route_open_count;
    SC64_ROUTE_PF(slot) = 0;
    SC64_ROUTE_KEEP_COUNT(FILL);
    return tail;
}

/* Once a reader requests undiscovered data, restore the directions already
 * assigned and resume the exact pending FIFO in the original order. */
#ifdef SC64_ROUTE_POINT_SECOND_EXACT_HIT
static int __attribute__((noinline)) sc64_point_prefix_resume(int slot, int cell, int near_reader)
#else
static int __attribute__((noinline)) sc64_point_prefix_complete(int slot)
#endif
{
    Sc64PointPrefix *prefix = sc64_point_prefix_find(slot);
    if (!prefix) return 0;
#ifdef SC64_ROUTE_POINT_COLD_CERTIFICATE
    if (prefix->count == SC64_POINT_CERTIFIED_ONLY) {
        (void)sc64_route_fill(slot, sc64_route_goal[slot]);
        return 1;
    }
#endif
#ifdef SC64_ROUTE_POINT_NEAR_RESERVE_PROBE
    Sc64PointPrefix *reserve = &sc64_point_prefixes[SC64_POINT_PREFIX_SLOTS];
    if (near_reader && cell >= 0 && reserve->count &&
        sc64_point_prefix_find(reserve->slot) == reserve)
        cell = -1;
#elif defined(SC64_ROUTE_POINT_SECOND_EXACT_HIT)
    (void)near_reader;
#endif
    const int cols = MAP_COLS, rows = MAP_ROWS, stride = cols + SC64_LINEAR_PAD;
    const int cap = sc64_route_linear_queue_cap(cols, rows);
    u8 *field = sc64_route_field[slot];
    u8 *dense = sc64_route_linear_grid(cols, rows, stride, cap);
    int first_row = 1, last_row = rows;
    if (prefix->restore_first_row >= 1 &&
        prefix->restore_first_row <= prefix->restore_last_row &&
        prefix->restore_last_row <= rows) {
        first_row = prefix->restore_first_row;
        last_row = prefix->restore_last_row;
    }
    /* The initial clear left all other rows at the undiscovered code. */
    const u8 *in = field + (first_row - 1) * (cols / 8) * 3;
    for (int row = first_row; row <= last_row; ++row) {
        u8 *out = dense + row * stride + 4;
        for (int col = 0; col < cols; col += 8) {
            unsigned packed = in[0] | (unsigned)in[1] << 8 | (unsigned)in[2] << 16;
            in += 3;
            if (packed == 0xffffffu) continue;
            for (int i = 0; i < 8; ++i, packed >>= 3) {
                unsigned direction = packed & 7u;
                if (direction != SC64_ROUTE_UNREACHABLE) out[col + i] = (u8)direction;
            }
        }
    }
    int pending = prefix->count;
#ifndef SC64_ROUTE_POINT_SECOND_EXACT_HIT
    sc64_point_prefix_discard(prefix);
#endif
    if (pending <= cap) {
        memcpy(bfs_queue, prefix->queue, (size_t)pending * sizeof(*bfs_queue));
#ifdef SC64_ROUTE_POINT_SECOND_EXACT_HIT
        int head = 0, tail = pending;
        if (cell >= 0 && !prefix->second_cutoff) {
            int start = cell + sc64_cell_row((unsigned)cell) * SC64_LINEAR_PAD + stride + 4;
            tail = sc64_point_prefix_expand(dense, start, stride, cap, &head, tail);
            int remaining = tail - head;
            if (remaining > 0 && remaining <= SC64_POINT_PREFIX_PENDING && dense[start] != 7u) {
                /* The bounded loop does not compact. Only appended cells
                 * need to be published, and each expands the restore span. */
                for (int i = pending; i < tail; ++i) {
                    int at = bfs_queue[i];
                    int row = (int)(((unsigned)at * 241u) >> 15);
                    int index = at - row * SC64_LINEAR_PAD - cols - 4;
                    sc64_route_put_new(field, index, dense[at]);
                    if (row < first_row) first_row = row;
                    if (row > last_row) last_row = row;
                }
                Sc64PointPrefix *parked = prefix;
#ifdef SC64_ROUTE_POINT_NEAR_RESERVE_PROBE
                if (near_reader) {
                    sc64_point_prefix_discard(reserve);
                    parked = reserve;
                    parked->slot = (u8)slot;
                    parked->goal = prefix->goal;
                }
#endif
                parked->restore_first_row = (u8)first_row;
                parked->restore_last_row = (u8)last_row;
                memcpy(parked->queue, bfs_queue + head, (size_t)remaining * sizeof(*bfs_queue));
                parked->count = (u8)remaining;
                parked->second_cutoff = 1;
#ifdef SC64_ROUTE_POINT_NEAR_RESERVE_PROBE
                /* Publish the new owner's completed frontier before freeing
                 * the cold row. The logical route slot and field stay put. */
                if (near_reader) {
                    sc64_point_prefix_index[slot] = SC64_POINT_PREFIX_SLOTS + 1;
                    prefix->count = 0;
                }
#endif
                return 1;
            }
        }
        if (sc64_route_expand_linear_from(dense, tail, stride, cap, head) >= 0) {
            sc64_point_prefix_discard(prefix);
#else
        if (sc64_route_expand_linear(dense, pending, stride, cap) >= 0) {
#endif
            sc64_route_pack_linear(field, dense, cols, rows, stride);
            return 1;
        }
    }
#ifdef SC64_ROUTE_POINT_SECOND_EXACT_HIT
    sc64_point_prefix_discard(prefix);
#endif
    (void)sc64_route_fill(slot, sc64_route_goal[slot]);
    return 1;
}

#ifdef SC64_ROUTE_POINT_COLD_CERTIFICATE
static int sc64_point_certificate_require(int slot, int cell)
{
    Sc64PointPrefix *prefix = sc64_point_prefix_find(slot);
    if (!prefix || prefix->count != SC64_POINT_CERTIFIED_ONLY) return 0;
    if ((unsigned)cell >= (unsigned)(MAP_COLS * MAP_ROWS) ||
        (sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE &&
         !sc64_point_certificate(slot, cell, prefix->goal, 0)))
        (void)sc64_route_fill(slot, prefix->goal);
    return 1;
}
#endif

#ifdef SC64_ROUTE_POINT_SECOND_EXACT_HIT
static int __attribute__((noinline)) sc64_point_prefix_complete(int slot)
{
    return sc64_point_prefix_resume(slot, -1, 0);
}

/* An exact point hit serves this open origin immediately. Other readers
 * still require the complete field, and a second miss cannot repark. */
static void sc64_point_prefix_require_exact_hit(int slot, int origin, int near_reader)
{
#ifdef SC64_ROUTE_POINT_COLD_CERTIFICATE
    if (sc64_point_certificate_require(slot, origin)) return;
#endif
    if (sc64_point_prefix_find(slot) &&
        ((unsigned)origin >= (unsigned)(MAP_COLS * MAP_ROWS) ||
         sc64_route_get(slot, origin) == SC64_ROUTE_UNREACHABLE)) {
        int demand = (unsigned)origin < (unsigned)(MAP_COLS * MAP_ROWS) &&
                     !SC64_ROUTE_LAZY(slot) && SC64_GRID_GET(path_walkable, origin);
        (void)sc64_point_prefix_resume(slot, demand ? origin : -1, near_reader);
    }
}
#endif

static void sc64_point_prefix_require(int slot, int cell)
{
#ifdef SC64_ROUTE_POINT_COLD_CERTIFICATE
    if (sc64_point_certificate_require(slot, cell)) return;
#endif
    if (sc64_point_prefix_find(slot) &&
        (cell < 0 || sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE))
        (void)sc64_point_prefix_complete(slot);
}

#ifdef SC64_ROUTE_POINT_COMPONENT_PROBE
/* A complete field seeded at one open tile describes exactly one connected
 * component. If it includes either endpoint, it decides their connectivity
 * without consuming that field or completing the requested one. */
static int __attribute__((noinline)) sc64_point_prefix_component_reaches(int slot, int cell)
{
    enum { MAX_WITNESSES = 4 };
    const unsigned cells = (unsigned)MAP_COLS * (unsigned)MAP_ROWS;
    const unsigned root = sc64_route_goal[slot];
    if (root >= cells || !SC64_GRID_GET(path_walkable, root)) return -1;
    const int active_slots = SC64_ROUTE_ACTIVE_SLOTS;
    unsigned checked = 0;
    for (int witness = 0; witness < active_slots && checked < MAX_WITNESSES; ++witness) {
        if (witness == slot || sc64_route_valid[witness] != 1 || SC64_ROUTE_LAZY(witness) ||
            sc64_point_prefix_find(witness)) continue;
        const unsigned seed = sc64_route_goal[witness];
        if (seed >= cells || !SC64_GRID_GET(path_walkable, seed)) continue;
        ++checked;
        int contains_root = sc64_route_get(witness, (int)root) != SC64_ROUTE_UNREACHABLE;
        int contains_cell = sc64_route_get(witness, cell) != SC64_ROUTE_UNREACHABLE;
        if (contains_root || contains_cell) return contains_root && contains_cell;
    }
    return -1;
}
#endif

#ifdef SC64_ROUTE_POINT_NEAR_DISTANCE_PROBE
static int sc64_point_near_neighbor(int a, int b)
{
    const int cols = MAP_COLS;
    int delta = a - b;
    return delta == cols || delta == -cols ||
        ((delta == 1 || delta == -1) &&
         sc64_cell_row((unsigned)a) == sc64_cell_row((unsigned)b));
}

static int sc64_point_witness_distance(int slot, int origin, int seed)
{
    const int delta[] = {0, 1, -1, MAP_COLS, -MAP_COLS};
    int distance = 0;
    while (origin != seed && distance < 128) {
        unsigned direction = (unsigned)sc64_route_get(slot, origin);
        if (direction < 1 || direction > 4) return -1;
        origin += delta[direction];
        ++distance;
    }
    return origin == seed ? distance : -1;
}

/* An open neighbor of the root gives an upper bound via its exact field.
 * An open neighbor of the queried goal gives a lower bound by the reverse
 * triangle inequality. Reject only when the bounds rule out every shortest
 * path through the goal. A tie still needs the canonical gradient. */
static int __attribute__((noinline))
sc64_point_prefix_near_distance_miss(int slot, int origin, int goal, int root)
{
    enum { MAX_WITNESSES = 4, NO_UPPER_BOUND = 32767 };
    const int cols = MAP_COLS, active_slots = SC64_ROUTE_ACTIVE_SLOTS;
    int root_row = sc64_cell_row((unsigned)root), goal_row = sc64_cell_row((unsigned)goal);
    int dx = root - root_row * cols - (goal - goal_row * cols);
    int dy = root_row - goal_row;
    int via_gap = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
    int upper = NO_UPPER_BOUND, lower = 0, checked = 0;
    for (int witness = 0; witness < active_slots && checked < MAX_WITNESSES; ++witness) {
        if (witness == slot || sc64_route_valid[witness] != 1 ||
            SC64_ROUTE_LAZY(witness)) continue;
        int seed = sc64_route_goal[witness];
        if ((unsigned)seed >= (unsigned)(cols * MAP_ROWS)) continue;
        int by_root = sc64_point_near_neighbor(seed, root);
        int by_goal = sc64_point_near_neighbor(seed, goal);
        if ((!by_root && !by_goal) || !SC64_GRID_GET(path_walkable, seed) ||
            sc64_point_prefix_find(witness)) continue;
        ++checked;
        int distance = sc64_point_witness_distance(witness, origin, seed);
        if (distance < 0) continue;
        if (by_root && distance + 1 < upper) upper = distance + 1;
        if (by_goal && distance - 1 > lower) lower = distance - 1;
        if (lower + via_gap > upper) return 1;
    }
    return 0;
}
#endif

#endif
