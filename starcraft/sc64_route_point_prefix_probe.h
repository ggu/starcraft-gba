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

#ifdef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
static inline void sc64_point_prefix_park(Sc64PointPrefix *prefix, int slot, int goal,
                                          const u16 *pending, int count)
{
    sc64_point_prefix_discard(prefix);
    prefix->goal = (u16)goal;
    prefix->slot = (u8)slot;
    memcpy(prefix->queue, pending, (size_t)count * sizeof(*pending));
    prefix->count = (u8)count;
    sc64_point_prefix_index[slot] = (u8)(prefix - sc64_point_prefixes + 1);
}

static int sc64_point_component_reaches(int slot, unsigned root, int cell, unsigned limit);

static inline int sc64_point_prefix_padded_seed(int root)
{
    return root + (root / SC64_LINEAR_WIDE_COLS + 1) * SC64_LINEAR_PAD +
           SC64_LINEAR_WIDE_COLS + 4;
}

/* An open seed enters the FIFO once. If it is still the only pending cell,
 * no expansion has run and a full reader can start the ordinary fill. */
static inline int sc64_point_prefix_seed_only(const Sc64PointPrefix *prefix)
{
    return prefix->count == 1 &&
           prefix->queue[0] == sc64_point_prefix_padded_seed(prefix->goal);
}
#endif

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
#ifndef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
    u8 *dense = sc64_route_linear_grid(cols, rows, stride, cap);
#endif
    int goal_start = goal + sc64_cell_row(goal) * SC64_LINEAR_PAD + stride + 4;
    int origin_start = origin + sc64_cell_row(origin) * SC64_LINEAR_PAD + stride + 4;
#ifdef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
    int head = 0, tail;
    /* A negative witness can answer this open origin immediately. Keep the
     * ordinary logical insertion and a real FIFO seed for future readers. */
    if (sc64_point_component_reaches(slot, (unsigned)goal, origin, 1) == 0) {
        const u16 seed = (u16)goal_start;
        sc64_route_field_clear(field);
        sc64_route_put_new(field, goal, 0);
        sc64_point_prefix_park(prefix, slot, goal, &seed, 1);
        tail = 1;
        goto filled;
    }
    u8 *dense = sc64_route_linear_grid(cols, rows, stride, cap);
    tail = sc64_route_linear_seed(dense, goal_start, stride);
#else
    int head = 0, tail = sc64_route_linear_seed(dense, goal_start, stride);
#endif
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
#ifdef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
        sc64_point_prefix_park(prefix, slot, goal, bfs_queue + head, pending);
#else
        sc64_point_prefix_discard(prefix);
        prefix->goal = (u16)goal;
        prefix->slot = (u8)slot;
        memcpy(prefix->queue, bfs_queue + head, (size_t)pending * sizeof(*bfs_queue));
        prefix->count = (u8)pending;
        sc64_point_prefix_index[slot] = (u8)(prefix - sc64_point_prefixes + 1);
#endif
    } else {
        tail = sc64_route_expand_linear_from(dense, tail, stride, cap, head);
        if (tail < 0) return sc64_route_fill(slot, goal);
        sc64_route_pack_linear(field, dense, cols, rows, stride);
    }
#ifdef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
filled:
#endif
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
#ifdef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
    if (sc64_point_prefix_seed_only(prefix)) {
        (void)sc64_route_fill(slot, sc64_route_goal[slot]);
        return 1;
    }
#endif
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

#ifdef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
static void sc64_point_prefix_require_mode(int slot, int cell, int ordinary_move)
{
    Sc64PointPrefix *prefix = sc64_point_prefix_find(slot);
    if (!prefix || (cell >= 0 && sc64_route_get(slot, cell) != SC64_ROUTE_UNREACHABLE)) return;
    if (ordinary_move && cell >= 0 && SC64_GRID_GET(path_walkable, cell) &&
        sc64_point_prefix_seed_only(prefix) &&
        sc64_point_component_reaches(slot, prefix->goal, cell, 1) == 0)
        return;
    (void)sc64_point_prefix_complete(slot);
}

static void sc64_point_prefix_require(int slot, int cell)
{
    sc64_point_prefix_require_mode(slot, cell, 0);
}
#else
static void sc64_point_prefix_require(int slot, int cell)
{
    if (sc64_point_prefix_find(slot) &&
        (cell < 0 || sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE))
        (void)sc64_point_prefix_complete(slot);
}
#endif

#ifdef SC64_ROUTE_POINT_COMPONENT_PROBE
/* A complete field seeded at one open tile describes exactly one connected
 * component. If it includes either endpoint, it decides their connectivity
 * without consuming that field or completing the requested one. */
static int __attribute__((noinline))
#ifdef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
sc64_point_component_reaches(int slot, unsigned root, int cell, unsigned limit)
#else
sc64_point_prefix_component_reaches(int slot, int cell)
#endif
{
#ifndef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
    enum { MAX_WITNESSES = 4 };
#endif
    const unsigned cells = (unsigned)MAP_COLS * (unsigned)MAP_ROWS;
#ifndef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
    const unsigned root = sc64_route_goal[slot];
#endif
    if (root >= cells || !SC64_GRID_GET(path_walkable, root)) return -1;
    const int active_slots = SC64_ROUTE_ACTIVE_SLOTS;
    unsigned checked = 0;
#ifdef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
    for (int witness = 0; witness < active_slots && checked < limit; ++witness) {
#else
    for (int witness = 0; witness < active_slots && checked < MAX_WITNESSES; ++witness) {
#endif
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
#ifdef SC64_ROUTE_POINT_SEED_COMPONENT_PROBE
static int sc64_point_prefix_component_reaches(int slot, int cell)
{
    return sc64_point_component_reaches(slot, sc64_route_goal[slot], cell, 4);
}
#endif
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
