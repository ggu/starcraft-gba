/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ROUTE_POINT_PREFIX_PROBE_H
#define SC64_ROUTE_POINT_PREFIX_PROBE_H

static int sc64_route_fill(int slot, int goal);

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
    sc64_point_prefix_index[prefix->slot] = 0;
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
    Sc64PointPrefix *prefix = sc64_point_prefix_free();
    if (!prefix) return sc64_route_fill(slot, goal);
    const int stride = cols + SC64_LINEAR_PAD;
    const int cap = sc64_route_linear_queue_cap(cols, rows);
    u8 *field = sc64_route_field[slot];
    u8 *dense = sc64_route_linear_grid(cols, rows, stride, cap);
    int goal_start = goal + sc64_cell_row(goal) * SC64_LINEAR_PAD + stride + 4;
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
        memcpy(prefix->queue, bfs_queue + head, (size_t)pending * sizeof(*bfs_queue));
        prefix->count = (u8)pending;
        sc64_point_prefix_index[slot] = (u8)(prefix - sc64_point_prefixes + 1);
    } else {
        tail = sc64_route_expand_linear_from(dense, tail, stride, cap, head);
        if (tail < 0) return sc64_route_fill(slot, goal);
        sc64_route_pack_linear(field, dense, cols, rows, stride);
    }
    SC64_ROUTE_LOGN(slot) = sc64_route_open_count;
    SC64_ROUTE_PF(slot) = 0;
    SC64_ROUTE_KEEP_COUNT(FILL);
    return tail;
}

/* Once a reader requests undiscovered data, restore the directions already
 * assigned and resume the exact pending FIFO in the original order. */
static int __attribute__((noinline)) sc64_point_prefix_complete(int slot)
{
    Sc64PointPrefix *prefix = sc64_point_prefix_find(slot);
    if (!prefix) return 0;
    const int cols = MAP_COLS, rows = MAP_ROWS, stride = cols + SC64_LINEAR_PAD;
    const int cap = sc64_route_linear_queue_cap(cols, rows);
    u8 *field = sc64_route_field[slot];
    u8 *dense = sc64_route_linear_grid(cols, rows, stride, cap);
    const u8 *in = field;
    for (int row = 1; row <= rows; ++row) {
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
    sc64_point_prefix_discard(prefix);
    if (pending <= cap) {
        memcpy(bfs_queue, prefix->queue, (size_t)pending * sizeof(*bfs_queue));
        if (sc64_route_expand_linear(dense, pending, stride, cap) >= 0) {
            sc64_route_pack_linear(field, dense, cols, rows, stride);
            return 1;
        }
    }
    (void)sc64_route_fill(slot, sc64_route_goal[slot]);
    return 1;
}

static void sc64_point_prefix_require(int slot, int cell)
{
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

#endif
