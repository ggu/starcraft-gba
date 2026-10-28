#ifndef SC64_ROUTES_H
#define SC64_ROUTES_H

#ifdef SC64_RANGE_WORK
#if !defined(SC64_RACES) || !defined(SC64_ROUTE_PACK3) || \
    !defined(SC64_IMPORTED_MISSION) || !defined(SC64_DIRECT_FIRING_POINTS)
#error "SC64_RANGE_WORK requires packed imported-race routes and direct firing points"
#endif
#include "sc64_range_work.h"
#endif

/* Standalone route fixtures still use byte grids; game.c supplies the packed
 * accessor for SC64_LARGE_MAPS builds. */
#ifndef SC64_GRID_GET
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_ROUTES_LOCAL_GRID_GET
#endif

#ifdef SC64_ROUTE_DECAY_FREQ
#if !defined(SC64_RACES) || !defined(SC64_ROUTE_PACK3)
#error "SC64_ROUTE_DECAY_FREQ requires SC64_RACES and SC64_ROUTE_PACK3"
#endif
#endif
#ifdef SC64_ROUTE_PARTIAL_FLOOD
#if !defined(SC64_RACES) || !defined(SC64_ROUTE_PACK3)
#error "SC64_ROUTE_PARTIAL_FLOOD requires SC64_RACES and SC64_ROUTE_PACK3"
#endif
#endif
#ifdef SC64_ROUTE_FLOOD_SPLIT
#if !defined(SC64_RACES) || !defined(SC64_ROUTE_PACK3)
#error "SC64_ROUTE_FLOOD_SPLIT requires SC64_RACES and SC64_ROUTE_PACK3"
#endif
#endif
#ifdef SC64_ROUTE_VISIT_GRID
#if !defined(SC64_RACES) || !defined(SC64_ROUTE_PACK3)
#error "SC64_ROUTE_VISIT_GRID requires SC64_RACES and SC64_ROUTE_PACK3"
#endif
#ifndef SC64_GRID_SET
#ifdef SC64_ROUTES_LOCAL_GRID_GET
#define SC64_GRID_SET(grid, cell, value) ((grid)[(cell)] = (u8)((value) != 0))
#define SC64_ROUTES_LOCAL_GRID_SET
#else
#error "SC64_ROUTE_VISIT_GRID requires SC64_GRID_SET for a custom grid accessor"
#endif
#endif
#endif
#ifdef SC64_ROUTE_WORD_CLEAR
#if !defined(SC64_RACES) || !defined(SC64_ROUTE_PACK3)
#error "SC64_ROUTE_WORD_CLEAR requires SC64_RACES and SC64_ROUTE_PACK3"
#endif
#endif
#ifdef SC64_ROUTE_L_FAST_PATH
#if !defined(SC64_RACES) || !defined(SC64_ROUTE_PACK3)
#error "SC64_ROUTE_L_FAST_PATH requires SC64_RACES and SC64_ROUTE_PACK3"
#endif
#endif
#ifdef SC64_ROUTE_ACTIVE_STRIDE
#if !defined(SC64_RACES) || !defined(SC64_ROUTE_PACK3) || \
    !defined(SC64_ZP_CAMPAIGN_STORAGE) || !defined(SC64_LARGE_MAPS)
#error "SC64_ROUTE_ACTIVE_STRIDE requires the packed ZP large-map race profile"
#endif
#endif

/* Shared shortest-path fields on the same four-neighbour collision grid.
 * A mineral, base or group destination costs one reverse flood, rather than
 * one flood per moving unit per tile. Dynamic actors still use local steering.
 * Derived caches are discarded after load and whenever a structure changes. */
/* The race game needs fields for four economies and several armies. */
#if defined(SC64_ROUTE_PACK3) && !defined(SC64_RACES)
#error "SC64_ROUTE_PACK3 is only supported by race route fields"
#endif
#if defined(SC64_T4_RUNTIME) && (!defined(SC64_RACES) || !defined(SC64_LARGE_MAPS) || \
                                !defined(SC64_ROUTE_PACK3))
#error "SC64_T4_RUNTIME requires large packed race routes"
#endif
#ifdef SC64_RACES
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_ROUTE_ACTIVE_STRIDE
#define SC64_ROUTE_SLOTS 18
#define SC64_ROUTE_ACTIVE_MAX_SLOTS 18
#else
#define SC64_ROUTE_SLOTS 14
#endif
#elif defined(SC64_LARGE_MAPS)
#ifdef SC64_ROUTE_PACK3
#define SC64_ROUTE_SLOTS 20
#else
#define SC64_ROUTE_SLOTS 15
#endif
#elif defined(SC64_T3_ENABLED)
#ifdef SC64_ROUTE_PACK3
#define SC64_ROUTE_SLOTS 22
#else
#define SC64_ROUTE_SLOTS 17
#endif
#else
#ifdef SC64_ROUTE_PACK3
#define SC64_ROUTE_SLOTS 24
#else
#define SC64_ROUTE_SLOTS 18
#endif
#endif
#ifdef SC64_ROUTE_PACK3
#define SC64_ROUTE_BYTES ((MAP_CELLS * 3 + 7) / 8)
#define SC64_ROUTE_UNREACHABLE 7
#else
#define SC64_ROUTE_BYTES ((MAP_CELLS + 1) / 2)
#define SC64_ROUTE_UNREACHABLE 15
#endif
#elif defined(SC64_SKIRMISH)
#define SC64_ROUTE_SLOTS 16
#else
#define SC64_ROUTE_SLOTS 8
#endif
#ifdef SC64_RANGE_WORK
/* Two of the fourteen fixed maximum-map planes are transient range-query
 * storage. The remaining twelve planes are shared with normal route LRU
 * fields; a retained tagged range snapshot dynamically occupies one slot. */
#define SC64_RANGE_WORK_RESERVED_PLANES 2u
#endif
#ifdef SC64_RACES
/* Default: two cells per byte. Optional PACK3: one 3-bit cell code. In either
 * layout 0 is the goal, 1..4 point toward a shortest-path neighbour, and the
 * all-ones code means unreachable. */
#ifdef SC64_ROUTE_WORD_CLEAR
/* The optional word clear uses an explicitly aligned byte array and GCC's
 * may_alias-qualified word type. Each row may start unaligned when the map's
 * packed byte count is not a multiple of four, so the helper handles a byte
 * prefix and tail around its aligned word stores. */
#ifdef SC64_ROUTE_ACTIVE_STRIDE
#define SC64_ROUTE_POOL_BYTES (14u * SC64_ROUTE_BYTES)
static u8 sc64_route_pool[SC64_ROUTE_POOL_BYTES] __attribute__((aligned(4)));
static u8 *sc64_route_field[SC64_ROUTE_SLOTS];
static unsigned sc64_route_active_bytes, sc64_route_active_slots;
static int sc64_route_config_cols = -1, sc64_route_config_rows = -1;
#else
static u8 sc64_route_field[SC64_ROUTE_SLOTS][SC64_ROUTE_BYTES]
    __attribute__((aligned(4)));
#endif
typedef u32 sc64_route_clear_word __attribute__((__may_alias__));
static void sc64_route_field_clear(u8 *field)
{
    unsigned prefix = (unsigned)(-(uintptr_t)field) & 3u;
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    unsigned remaining = sc64_route_active_bytes;
#else
    unsigned remaining = SC64_ROUTE_BYTES;
#endif
    unsigned words_count, i;
    while (prefix && remaining) {
        *field++ = 0xff;
        --prefix;
        --remaining;
    }
    words_count = remaining / sizeof(sc64_route_clear_word);
    {
        sc64_route_clear_word *words =
            (sc64_route_clear_word *)(void *)field;
        for (i = 0; i + 4 <= words_count; i += 4) {
            words[i] = 0xffffffffu;
            words[i + 1] = 0xffffffffu;
            words[i + 2] = 0xffffffffu;
            words[i + 3] = 0xffffffffu;
        }
        for (; i < words_count; ++i) words[i] = 0xffffffffu;
    }
    field += words_count * sizeof(sc64_route_clear_word);
    remaining -= words_count * sizeof(sc64_route_clear_word);
    while (remaining--) *field++ = 0xff;
}
#else
#ifdef SC64_ROUTE_ACTIVE_STRIDE
#define SC64_ROUTE_POOL_BYTES (14u * SC64_ROUTE_BYTES)
static u8 sc64_route_pool[SC64_ROUTE_POOL_BYTES];
static u8 *sc64_route_field[SC64_ROUTE_SLOTS];
static unsigned sc64_route_active_bytes, sc64_route_active_slots;
static int sc64_route_config_cols = -1, sc64_route_config_rows = -1;
#else
static u8 sc64_route_field[SC64_ROUTE_SLOTS][SC64_ROUTE_BYTES];
#endif
#endif
#else
/* Legacy fields retain distances for the original blocked-start behavior. */
static u16 sc64_route_field[SC64_ROUTE_SLOTS][MAP_CELLS];
#endif
static u8 sc64_route_valid[SC64_ROUTE_SLOTS];
static u16 sc64_route_goal[SC64_ROUTE_SLOTS];
static u32 sc64_route_age[SC64_ROUTE_SLOTS], sc64_route_clock;
#ifdef SC64_RANGE_WORK
static void sc64_range_route_reset(void);
#endif
#ifdef SC64_ROUTE_ACTIVE_STRIDE
/* Rebind field bases after map-shape changes. Return false for invalid or
 * oversized geometry before any route-grid access can use these fields. */
static int sc64_route_configure(void)
{
    int cols = MAP_COLS, rows = MAP_ROWS;
    unsigned cells, bytes, slots, i;
    if (cols <= 0 || rows <= 0 || cols > 128 || rows > 128 ||
        (unsigned)cols > (unsigned)MAP_CELLS / (unsigned)rows) {
        if (sc64_route_config_cols != cols || sc64_route_config_rows != rows ||
            sc64_route_active_slots != 0) {
#ifdef SC64_RANGE_WORK
            sc64_range_route_reset();
#endif
            sc64_route_config_cols = cols;
            sc64_route_config_rows = rows;
            sc64_route_active_bytes = 0;
            sc64_route_active_slots = 0;
            for (i = 0; i < SC64_ROUTE_ACTIVE_MAX_SLOTS; ++i) sc64_route_field[i] = 0;
            memset(sc64_route_valid, 0, sizeof(sc64_route_valid));
            memset(sc64_route_age, 0, sizeof(sc64_route_age));
            sc64_route_clock = 0;
        }
        return 0;
    }
    cells = (unsigned)cols * (unsigned)rows;
    bytes = (cells * 3u + 7u) / 8u;
    if (bytes == 0 || bytes > SC64_ROUTE_BYTES || bytes > SC64_ROUTE_POOL_BYTES)
        return 0;
    slots = SC64_ROUTE_POOL_BYTES / bytes;
#ifdef SC64_RANGE_WORK
    slots = (SC64_ROUTE_POOL_BYTES -
             SC64_RANGE_WORK_RESERVED_PLANES * SC64_ROUTE_BYTES) / bytes;
#endif
    if (slots > SC64_ROUTE_ACTIVE_MAX_SLOTS) slots = SC64_ROUTE_ACTIVE_MAX_SLOTS;
    if (slots == 0) return 0;
    if (sc64_route_config_cols == cols && sc64_route_config_rows == rows &&
        sc64_route_active_bytes == bytes && sc64_route_active_slots == slots) return 1;
#ifdef SC64_RANGE_WORK
    sc64_range_route_reset();
#endif
    sc64_route_config_cols = cols;
    sc64_route_config_rows = rows;
    sc64_route_active_bytes = bytes;
    sc64_route_active_slots = slots;
    for (i = 0; i < slots; ++i) sc64_route_field[i] = sc64_route_pool + i * bytes;
    for (; i < SC64_ROUTE_ACTIVE_MAX_SLOTS; ++i) sc64_route_field[i] = 0;
    memset(sc64_route_valid, 0, sizeof(sc64_route_valid));
    memset(sc64_route_age, 0, sizeof(sc64_route_age));
    sc64_route_clock = 0;
    return 1;
}
#define SC64_ROUTE_ACTIVE_SLOTS ((int)sc64_route_active_slots)
#else
#ifdef SC64_RANGE_WORK
#define SC64_ROUTE_ACTIVE_SLOTS ((int)SC64_ROUTE_SLOTS - (int)SC64_RANGE_WORK_RESERVED_PLANES)
#else
#define SC64_ROUTE_ACTIVE_SLOTS SC64_ROUTE_SLOTS
#endif
#endif
volatile u32 sc64_route_hits, sc64_route_builds;

#ifdef SC64_ROUTE_DECAY_FREQ
#define SC64_ROUTE_FREQ_SHIFT 26u
#define SC64_ROUTE_RECENCY_MASK 0x03FFFFFFu
#define SC64_ROUTE_DECAY_PERIOD 128u
static u32 sc64_route_age_freq(u32 packed) { return packed >> SC64_ROUTE_FREQ_SHIFT; }
static u32 sc64_route_age_stamp(u32 packed) { return packed & SC64_ROUTE_RECENCY_MASK; }
static u32 sc64_route_age_pack(u32 freq, u32 stamp)
{
    return ((freq > 63u ? 63u : freq) << SC64_ROUTE_FREQ_SHIFT) |
           (stamp & SC64_ROUTE_RECENCY_MASK);
}
/* Clear the derived cache before recency wraps, keeping age ordering exact. */
static int sc64_route_age_older(u32 a, u32 b)
{
    return sc64_route_age_stamp(a) < sc64_route_age_stamp(b);
}
static void sc64_route_age_request(void)
{
    int i;
    if ((sc64_route_clock & SC64_ROUTE_RECENCY_MASK) == SC64_ROUTE_RECENCY_MASK) {
        memset(sc64_route_valid, 0, sizeof(sc64_route_valid));
        memset(sc64_route_age, 0, sizeof(sc64_route_age));
        sc64_route_clock = 0;
    }
    ++sc64_route_clock;
    if ((sc64_route_clock & (SC64_ROUTE_DECAY_PERIOD - 1u)) != 0u) return;
    for (i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i) if (sc64_route_valid[i]) {
        u32 packed = sc64_route_age[i];
        sc64_route_age[i] = sc64_route_age_pack(sc64_route_age_freq(packed) >> 1,
                                                sc64_route_age_stamp(packed));
    }
}
static void sc64_route_age_touch(int slot, int increment)
{
    u32 freq = sc64_route_age_freq(sc64_route_age[slot]);
    if (increment && freq < 63u) ++freq;
    sc64_route_age[slot] = sc64_route_age_pack(freq, sc64_route_clock);
}
#endif
#ifdef SC64_RACES
static int GBA_HOT sc64_route_get(int slot, int cell)
{
#ifdef SC64_ROUTE_PACK3
    u32 bit = (u32)cell * 3u;
    unsigned byte = bit >> 3, shift = bit & 7u;
    u16 pair = sc64_route_field[slot][byte];
    if (shift > 5u) pair |= (u16)sc64_route_field[slot][byte + 1] << 8;
    return (pair >> shift) & 7u;
#else
    u8 v = sc64_route_field[slot][cell >> 1];
    return (cell & 1) ? v >> 4 : v & 15;
#endif
}

static void GBA_HOT sc64_route_put(int slot, int cell, int direction)
{
#ifdef SC64_ROUTE_PACK3
    u32 bit = (u32)cell * 3u;
    unsigned byte = bit >> 3, shift = bit & 7u;
    u16 pair = sc64_route_field[slot][byte];
    u16 mask = (u16)(7u << shift);
    if (shift > 5u) pair |= (u16)sc64_route_field[slot][byte + 1] << 8;
    pair = (u16)((pair & ~mask) | (((u16)direction & 7u) << shift));
    sc64_route_field[slot][byte] = (u8)pair;
    if (shift > 5u) sc64_route_field[slot][byte + 1] = (u8)(pair >> 8);
#else
    u8 *p = &sc64_route_field[slot][cell >> 1];
    if (cell & 1) *p = (u8)((*p & 15) | (direction << 4));
    else *p = (u8)((*p & 240) | direction);
#endif
}
#endif

static void
sc64_routes_reset(void)
{
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    (void)sc64_route_configure();
#endif
    memset(sc64_route_valid, 0, sizeof(sc64_route_valid));
#ifdef SC64_RANGE_WORK
    sc64_range_route_reset();
#endif
#ifdef SC64_ROUTE_DECAY_FREQ
    memset(sc64_route_age, 0, sizeof(sc64_route_age));
    sc64_route_clock = 0;
#endif
}

#ifdef SC64_RANGE_WORK
enum {
    SC64_RANGE_ROUTE_ERROR = -1,
    SC64_RANGE_ROUTE_PENDING = 0,
    SC64_RANGE_ROUTE_FOUND = 1,
    SC64_RANGE_ROUTE_NO_PATH = 2,
    SC64_RANGE_ROUTE_CANCELLED = 3,
    SC64_RANGE_ROUTE_STALE_GUIDANCE = 4,
    SC64_RANGE_ROUTE_QUEUE_CAPACITY = 4,
    SC64_RANGE_ROUTE_RESULT_LEASE = 24,
    SC64_RANGE_ROUTE_IDLE_LIMIT = 48
};
#ifndef SC64_RANGE_WORK_BUDGET_PER_TICK
#define SC64_RANGE_WORK_BUDGET_PER_TICK 96u
#endif

typedef struct Sc64RangeRouteJob {
    u16 target, tx, ty, minimum, maximum;
    u16 last_request;
    u8 valid;
} Sc64RangeRouteJob;

static Sc64RangeWork sc64_range_route_work;
static Sc64RangeRouteJob sc64_range_route_active;
static Sc64RangeRouteJob
    sc64_range_route_pending[SC64_RANGE_ROUTE_QUEUE_CAPACITY];
static u32 sc64_range_route_complete_frame;
static u8 sc64_range_route_queue_head, sc64_range_route_queue_count;
static s8 sc64_range_route_snapshot_slot = -1;
static s8 sc64_range_route_copy_slot = -1;
static unsigned short sc64_range_route_copy_cursor;
static unsigned short sc64_range_route_last_units;
static Sc64RangeRouteJob sc64_range_route_snapshot_key;

static int sc64_range_route_same(const Sc64RangeRouteJob *a,
                                const Sc64RangeRouteJob *b)
{
    return a->valid && b->valid && a->target == b->target &&
        a->tx == b->tx && a->ty == b->ty &&
        a->minimum == b->minimum && a->maximum == b->maximum;
}

static int sc64_range_route_same_actor_band(const Sc64RangeRouteJob *a,
                                            const Sc64RangeRouteJob *b)
{
    return a->valid && b->valid && a->target == b->target &&
        a->minimum == b->minimum && a->maximum == b->maximum;
}

static u8 *sc64_range_route_gradient_storage(void)
{
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    return sc64_route_pool + SC64_ROUTE_POOL_BYTES -
        SC64_RANGE_WORK_RESERVED_PLANES * SC64_ROUTE_BYTES;
#else
    return (u8 *)(void *)sc64_route_field[SC64_ROUTE_SLOTS - 2];
#endif
}

static u8 *sc64_range_route_frontier_storage(void)
{
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    return sc64_route_pool + SC64_ROUTE_POOL_BYTES - SC64_ROUTE_BYTES;
#else
    return (u8 *)(void *)sc64_route_field[SC64_ROUTE_SLOTS - 1];
#endif
}

static unsigned sc64_range_route_plane_capacity(void)
{
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    return SC64_ROUTE_BYTES;
#elif defined(SC64_ROUTE_PACK3)
    return SC64_ROUTE_BYTES;
#else
    return sizeof(sc64_route_field[0]);
#endif
}

static void sc64_range_route_snapshot_invalidate(void)
{
    if (sc64_range_route_snapshot_slot >= 0)
        sc64_route_valid[sc64_range_route_snapshot_slot] = 0;
    if (sc64_range_route_copy_slot >= 0)
        sc64_route_valid[sc64_range_route_copy_slot] = 0;
    sc64_range_route_snapshot_slot = -1;
    sc64_range_route_copy_slot = -1;
    sc64_range_route_copy_cursor = 0;
    memset(&sc64_range_route_snapshot_key, 0,
           sizeof(sc64_range_route_snapshot_key));
}

static void sc64_range_route_reset(void)
{
    sc64_range_work_reset(&sc64_range_route_work);
    memset(&sc64_range_route_active, 0, sizeof(sc64_range_route_active));
    memset(sc64_range_route_pending, 0, sizeof(sc64_range_route_pending));
    sc64_range_route_queue_head = sc64_range_route_queue_count = 0;
    sc64_range_route_snapshot_invalidate();
}

static int sc64_range_route_queue_index(unsigned offset)
{
    return (sc64_range_route_queue_head + (u8)offset) %
        SC64_RANGE_ROUTE_QUEUE_CAPACITY;
}

static void sc64_range_route_queue_remove(unsigned offset)
{
    for (unsigned i = offset; i + 1u < sc64_range_route_queue_count; ++i)
        sc64_range_route_pending[sc64_range_route_queue_index(i)] =
            sc64_range_route_pending[sc64_range_route_queue_index(i + 1u)];
    if (sc64_range_route_queue_count) {
        unsigned last = sc64_range_route_queue_index(
            sc64_range_route_queue_count - 1u);
        memset(&sc64_range_route_pending[last], 0,
               sizeof(sc64_range_route_pending[last]));
        --sc64_range_route_queue_count;
    }
    if (!sc64_range_route_queue_count) sc64_range_route_queue_head = 0;
}

static int sc64_range_route_enqueue(const Sc64RangeRouteJob *job)
{
    for (unsigned i = 0; i < sc64_range_route_queue_count; ++i) {
        Sc64RangeRouteJob *queued =
            &sc64_range_route_pending[sc64_range_route_queue_index(i)];
        if (sc64_range_route_same(queued, job)) {
            queued->last_request = (u16)frame;
            return 1;
        }
        if (sc64_range_route_same_actor_band(queued, job)) {
            queued->tx = job->tx; queued->ty = job->ty;
            queued->last_request = (u16)frame;
            return 1;
        }
    }
    if (sc64_range_route_queue_count >= SC64_RANGE_ROUTE_QUEUE_CAPACITY) return 0;
    {
        unsigned tail = sc64_range_route_queue_index(sc64_range_route_queue_count++);
        sc64_range_route_pending[tail] = *job;
        sc64_range_route_pending[tail].last_request = (u16)frame;
        sc64_range_route_pending[tail].valid = 1;
    }
    return 1;
}

static int sc64_range_route_dequeue(Sc64RangeRouteJob *job)
{
    if (!sc64_range_route_queue_count) return 0;
    *job = sc64_range_route_pending[sc64_range_route_queue_head];
    memset(&sc64_range_route_pending[sc64_range_route_queue_head], 0,
           sizeof(sc64_range_route_pending[sc64_range_route_queue_head]));
    sc64_range_route_queue_head =
        (sc64_range_route_queue_head + 1u) % SC64_RANGE_ROUTE_QUEUE_CAPACITY;
    --sc64_range_route_queue_count;
    if (!sc64_range_route_queue_count) sc64_range_route_queue_head = 0;
    return 1;
}

static int sc64_range_route_queue_nonempty(void)
{
    return sc64_range_route_queue_count != 0;
}

static int sc64_range_route_has_replacement(void)
{
    for (unsigned i = 0; i < sc64_range_route_queue_count; ++i)
        if (sc64_range_route_same_actor_band(&sc64_range_route_active,
            &sc64_range_route_pending[sc64_range_route_queue_index(i)]))
            return 1;
    return 0;
}

static int sc64_range_route_snapshot_begin_copy(void)
{
    int slot = sc64_range_route_snapshot_slot;
    if (slot < 0) {
        for (int i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i)
            if (!sc64_route_valid[i]) { slot = i; break; }
        if (slot < 0) {
            slot = 0;
            for (int i = 1; i < SC64_ROUTE_ACTIVE_SLOTS; ++i) {
#ifdef SC64_ROUTE_DECAY_FREQ
                if (sc64_route_age_freq(sc64_route_age[i]) <
                        sc64_route_age_freq(sc64_route_age[slot]) ||
                    (sc64_route_age_freq(sc64_route_age[i]) ==
                        sc64_route_age_freq(sc64_route_age[slot]) &&
                     sc64_route_age_older(sc64_route_age[i],
                                          sc64_route_age[slot]))) slot = i;
#else
                if (sc64_route_age[i] < sc64_route_age[slot]) slot = i;
#endif
            }
        }
    }
    if (slot < 0 || slot >= SC64_ROUTE_ACTIVE_SLOTS) return 0;
    sc64_range_route_snapshot_slot = -1;
    memset(&sc64_range_route_snapshot_key, 0,
           sizeof(sc64_range_route_snapshot_key));
    sc64_range_route_copy_slot = slot;
    sc64_range_route_copy_cursor = 0;
    sc64_route_valid[slot] = 3; /* reserved/tagged; ordinary fields ignore it */
    return 1;
}

/* Copy only the current bounded chunk. Word transfers are used when both
 * addresses can be aligned together; mismatched offsets stay bytewise. */
typedef u32 sc64_range_copy_word __attribute__((__may_alias__));
static void sc64_range_copy_bytes(u8 *dst, const u8 *src, unsigned bytes)
{
    if ((((uintptr_t)dst ^ (uintptr_t)src) & 3u) != 0u) {
        while (bytes--) *dst++ = *src++;
        return;
    }
    while (bytes && ((uintptr_t)dst & 3u)) {
        *dst++ = *src++;
        --bytes;
    }
    while (bytes >= sizeof(sc64_range_copy_word)) {
        *(sc64_range_copy_word *)(void *)dst =
            *(const sc64_range_copy_word *)(const void *)src;
        dst += sizeof(sc64_range_copy_word);
        src += sizeof(sc64_range_copy_word);
        bytes -= sizeof(sc64_range_copy_word);
    }
    while (bytes--) *dst++ = *src++;
}

static int sc64_range_route_snapshot_copy(unsigned budget)
{
    unsigned bytes = sc64_range_route_work.gradient_bytes;
    unsigned used = 0;
    while (budget-- && sc64_range_route_copy_cursor < bytes) {
        unsigned count = bytes - sc64_range_route_copy_cursor;
        if (count > 64u) count = 64u;
        sc64_range_copy_bytes(
            sc64_route_field[sc64_range_route_copy_slot] +
                sc64_range_route_copy_cursor,
            sc64_range_route_work.gradient + sc64_range_route_copy_cursor,
            count);
        sc64_range_route_copy_cursor = (unsigned short)
            (sc64_range_route_copy_cursor + count);
        ++used;
    }
    sc64_range_route_last_units = (unsigned short)used;
    if (sc64_range_route_copy_cursor < bytes) return 0;
    sc64_range_route_snapshot_slot = sc64_range_route_copy_slot;
    sc64_range_route_copy_slot = -1;
    sc64_range_route_snapshot_key = sc64_range_route_active;
    sc64_range_route_snapshot_key.valid = 1;
#ifdef SC64_ROUTE_DECAY_FREQ
    sc64_route_age_touch(sc64_range_route_snapshot_slot, 1);
#else
    sc64_route_age[sc64_range_route_snapshot_slot] = ++sc64_route_clock;
#endif
    return 1;
}

static int sc64_range_route_snapshot_query(int target, int tx, int ty,
                                           int minimum, int maximum,
                                           int col, int row,
                                           int *next_col, int *next_row)
{
    unsigned cell, direction;
    int exact;
    if (sc64_range_route_snapshot_slot < 0 ||
        sc64_range_route_snapshot_slot >= SC64_ROUTE_ACTIVE_SLOTS ||
        sc64_route_valid[sc64_range_route_snapshot_slot] != 3 ||
        !sc64_range_route_snapshot_key.valid || !next_col || !next_row ||
        sc64_range_route_snapshot_key.target != target ||
        sc64_range_route_snapshot_key.minimum != minimum ||
        sc64_range_route_snapshot_key.maximum != maximum ||
        col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS)
        return SC64_RANGE_ROUTE_PENDING;
    exact = sc64_range_route_snapshot_key.tx == tx &&
            sc64_range_route_snapshot_key.ty == ty;
    cell = (unsigned)col + (unsigned)row * (unsigned)MAP_COLS;
    direction = sc64_range_work_get3(
        (const u8 *)(const void *)sc64_route_field[sc64_range_route_snapshot_slot],
        cell);
    if (SC64_GRID_GET(path_walkable, cell)) {
        *next_col = col; *next_row = row;
        if (direction == 1u) ++*next_col;
        else if (direction == 2u) --*next_col;
        else if (direction == 3u) ++*next_row;
        else if (direction == 4u) --*next_row;
        else if (direction != 0u) return exact ? SC64_RANGE_ROUTE_NO_PATH :
                                                   SC64_RANGE_ROUTE_PENDING;
    } else {
        static const s8 dx[4] = {1,0,-1,0};
        static const s8 dy[4] = {0,1,0,-1};
        int found = 0;
        for (int i = 0; i < 4; ++i) {
            int cx = col + dx[i], cy = row + dy[i];
            if (cx >= 0 && cy >= 0 && cx < MAP_COLS && cy < MAP_ROWS &&
                SC64_GRID_GET(path_walkable, cx + cy * MAP_COLS) &&
                sc64_range_work_get3((const u8 *)(const void *)
                    sc64_route_field[sc64_range_route_snapshot_slot],
                    (unsigned)cx + (unsigned)cy * (unsigned)MAP_COLS) != 7u) {
                *next_col = cx; *next_row = cy; found = 1; break;
            }
        }
        if (!found) return exact ? SC64_RANGE_ROUTE_NO_PATH :
                                   SC64_RANGE_ROUTE_PENDING;
    }
    if (*next_col < 0 || *next_row < 0 || *next_col >= MAP_COLS ||
        *next_row >= MAP_ROWS || !SC64_GRID_GET(path_walkable,
            *next_col + *next_row * MAP_COLS))
        return SC64_RANGE_ROUTE_ERROR;
#ifdef SC64_ROUTE_DECAY_FREQ
    sc64_route_age_touch(sc64_range_route_snapshot_slot, 1);
#else
    sc64_route_age[sc64_range_route_snapshot_slot] = ++sc64_route_clock;
#endif
    return exact ? SC64_RANGE_ROUTE_FOUND : SC64_RANGE_ROUTE_STALE_GUIDANCE;
}

static int sc64_range_route_snapshot_matches_active(void)
{
    return sc64_range_route_snapshot_slot >= 0 &&
        sc64_range_route_snapshot_slot < SC64_ROUTE_ACTIVE_SLOTS &&
        sc64_route_valid[sc64_range_route_snapshot_slot] == 3 &&
        sc64_range_route_same(&sc64_range_route_snapshot_key,
                              &sc64_range_route_active);
}

static void sc64_range_route_tick(void)
{
    unsigned used = 0, budget = SC64_RANGE_WORK_BUDGET_PER_TICK;
    int status;
    sc64_range_route_last_units = 0;
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    /* A changed map shape rebinds/clears the route pool and resets range
     * jobs. Do this before inspecting or dequeuing a job so no request can be
     * copied into state that configuration immediately invalidates. */
    if (!sc64_route_configure()) return;
#endif
    for (unsigned i = 0; i < sc64_range_route_queue_count;)
        if ((u16)((u16)frame - sc64_range_route_pending[
                sc64_range_route_queue_index(i)].last_request) >
                SC64_RANGE_ROUTE_IDLE_LIMIT)
            sc64_range_route_queue_remove(i);
        else ++i;
    if (sc64_range_route_active.valid &&
        sc64_range_route_work.phase != SC64_RANGE_WORK_DONE &&
        (u16)((u16)frame - sc64_range_route_active.last_request) >
            SC64_RANGE_ROUTE_IDLE_LIMIT) {
        sc64_range_work_reset(&sc64_range_route_work);
        sc64_range_route_active.valid = 0;
    }
    /* A queued successor may expire while a bounded snapshot copy is in
     * progress (for example with a deliberately tiny per-tick budget). Do
     * not leave its partially copied slot pinned after there is no consumer. */
    if (!sc64_range_route_queue_nonempty() &&
        sc64_range_route_copy_slot >= 0)
        sc64_range_route_snapshot_invalidate();
    /* Route work should not turn an otherwise idle topology invalidation into
     * an eager full-grid rebuild. Shape configuration/reset above is constant
     * time; build the collision grid only when work is actually requested. */
    if ((!sc64_range_route_active.valid ||
         sc64_range_route_work.phase == SC64_RANGE_WORK_DONE) &&
        !sc64_range_route_queue_nonempty()) return;
    if (!path_grid_valid) build_path_grid();
    if (sc64_range_route_active.valid &&
        sc64_range_route_work.phase == SC64_RANGE_WORK_DONE &&
        sc64_range_route_queue_nonempty() &&
        (sc64_range_route_has_replacement() ||
         (u32)frame - sc64_range_route_complete_frame >=
             SC64_RANGE_ROUTE_RESULT_LEASE)) {
        /* Preserve the completed route in a tagged ordinary LRU field before
         * the two scratch planes are reused by the next worker. Copy in
         * bounded 64-byte chunks, charged against this tick's work budget. */
        if (sc64_range_route_snapshot_matches_active()) {
            sc64_range_work_reset(&sc64_range_route_work);
            sc64_range_route_active.valid = 0;
        } else {
            if (sc64_range_route_copy_slot < 0 &&
                !sc64_range_route_snapshot_begin_copy()) {
                sc64_range_work_reset(&sc64_range_route_work);
                sc64_range_route_active.valid = 0;
            } else {
                (void)sc64_range_route_snapshot_copy(budget);
                return;
            }
        }
    }
    if (!sc64_range_route_active.valid) {
        if (!sc64_range_route_dequeue(&sc64_range_route_active)) return;
        status = sc64_range_work_begin(&sc64_range_route_work,
            MAP_COLS, MAP_ROWS, path_walkable, SC64_GRID_STORAGE_BYTES,
            sc64_range_route_gradient_storage(), sc64_range_route_plane_capacity(),
            sc64_range_route_frontier_storage(), sc64_range_route_plane_capacity(),
            sc64_range_route_active.tx, sc64_range_route_active.ty,
            sc64_range_route_active.minimum, sc64_range_route_active.maximum);
        if (status < 0) {
            sc64_range_route_active.valid = 0;
            return;
        }
    }
    if (sc64_range_route_work.phase != SC64_RANGE_WORK_DONE) {
        status = sc64_range_work_pump(&sc64_range_route_work,
            budget, &used);
        sc64_range_route_last_units = (unsigned short)used;
        if (status == SC64_RANGE_WORK_READY)
            sc64_range_route_complete_frame = (u32)frame;
        else if (status < 0) {
            sc64_range_work_reset(&sc64_range_route_work);
            sc64_range_route_active.valid = 0;
        }
    }
}

/* Return FOUND for the current exact key. On target motion, queue the latest
 * position and return STALE_GUIDANCE only from a same-target/range field;
 * current weapon checks still decide whether combat can fire. Pending and
 * no-path never expose a field for a different target or topology. */
static int sc64_range_route_next(int target, int tx, int ty,
                                 int minimum, int maximum,
                                 int col, int row,
                                 int *next_col, int *next_row)
{
    Sc64RangeRouteJob job = {0};
    int status, nx = -1, ny = -1;
    unsigned exits = 0;
    if (!next_col || !next_row || target < 0 || target > 0xffff ||
        tx < 0 || ty < 0 || tx >= MAP_COLS * 16 || ty >= MAP_ROWS * 16 ||
        minimum < 0 || maximum < minimum || maximum > 4096 ||
        col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS)
        return SC64_RANGE_ROUTE_ERROR;
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    /* Queries may run before the frame-end worker tick. Rebind variable-size
     * route slots now so no old-shape gradient is read using new dimensions. */
    if (!sc64_route_configure()) return SC64_RANGE_ROUTE_ERROR;
#endif
    job.target = (u16)target; job.tx = (u16)tx; job.ty = (u16)ty;
    job.minimum = (u16)minimum; job.maximum = (u16)maximum; job.valid = 1;
    if (!path_grid_valid) build_path_grid();
    if (sc64_range_route_active.valid &&
        sc64_range_route_same_actor_band(&sc64_range_route_active, &job) &&
        !sc64_range_route_same(&sc64_range_route_active, &job)) {
        /* Keep bounded work on its original seed snapshot. Repeated target
         * motion coalesces the latest position, while a discovered gradient
         * remains usable as explicitly stale movement guidance. */
        sc64_range_route_active.last_request = (u16)frame;
        (void)sc64_range_route_enqueue(&job);
        status = sc64_range_work_query(&sc64_range_route_work,
                                       col, row, &nx, &ny, &exits);
        if (status == SC64_RANGE_WORK_READY) {
            if (exits) {
                static const s8 dx[4] = {1,0,-1,0};
                static const s8 dy[4] = {0,1,0,-1};
                static const u8 bits[4] = {0,2,1,3};
                for (int i = 0; i < 4; ++i) if (exits & (1u << bits[i])) {
                    nx = col + dx[i]; ny = row + dy[i]; break;
                }
            }
            if (nx >= 0 && ny >= 0 && nx < MAP_COLS && ny < MAP_ROWS &&
                SC64_GRID_GET(path_walkable, nx + ny * MAP_COLS)) {
                *next_col = nx; *next_row = ny;
                return SC64_RANGE_ROUTE_STALE_GUIDANCE;
            }
        }
        status = sc64_range_route_snapshot_query(target, tx, ty, minimum,
            maximum, col, row, next_col, next_row);
        if (status == SC64_RANGE_ROUTE_FOUND ||
            status == SC64_RANGE_ROUTE_STALE_GUIDANCE) return status;
        return SC64_RANGE_ROUTE_CANCELLED;
    }
    if (sc64_range_route_same(&sc64_range_route_active, &job)) {
        sc64_range_route_active.last_request = (u32)frame;
        status = sc64_range_work_query(&sc64_range_route_work,
                                       col, row, &nx, &ny, &exits);
        if (status == SC64_RANGE_WORK_READY) {
            if (exits) {
                static const s8 dx[4] = {1,0,-1,0};
                static const s8 dy[4] = {0,1,0,-1};
                static const u8 bits[4] = {0,2,1,3};
                for (int i = 0; i < 4; ++i) if (exits & (1u << bits[i])) {
                    nx = col + dx[i]; ny = row + dy[i]; break;
                }
            }
            if (nx < 0 || ny < 0 || nx >= MAP_COLS || ny >= MAP_ROWS ||
                !SC64_GRID_GET(path_walkable, nx + ny * MAP_COLS))
                return SC64_RANGE_ROUTE_ERROR;
            *next_col = nx; *next_row = ny;
            return SC64_RANGE_ROUTE_FOUND;
        }
        if (status == SC64_RANGE_WORK_NO_PATH) return SC64_RANGE_ROUTE_NO_PATH;
        if (status == SC64_RANGE_WORK_PENDING) {
            status = sc64_range_route_snapshot_query(target, tx, ty, minimum,
                maximum, col, row, next_col, next_row);
            if (status == SC64_RANGE_ROUTE_FOUND ||
                status == SC64_RANGE_ROUTE_STALE_GUIDANCE) return status;
            return SC64_RANGE_ROUTE_PENDING;
        }
        return SC64_RANGE_ROUTE_ERROR;
    }
    (void)sc64_range_route_enqueue(&job);
    status = sc64_range_route_snapshot_query(target, tx, ty, minimum,
        maximum, col, row, next_col, next_row);
    if (status == SC64_RANGE_ROUTE_FOUND ||
        status == SC64_RANGE_ROUTE_STALE_GUIDANCE) return status;
    return SC64_RANGE_ROUTE_PENDING;
}
#endif

#ifdef SC64_ROUTE_PARTIAL_FLOOD
/* A partial field is usable only when it has discovered the query origin's
 * exact gradient. A blocked origin waits for all walkable cardinal exits. */
static int sc64_route_origin_ready(int slot, int origin, int origin_walkable)
{
    int col, row, cell;
    if (origin < 0) return 0;
    if (origin_walkable)
        return sc64_route_get(slot, origin) != SC64_ROUTE_UNREACHABLE;
    col = sc64_cell_col(origin);
    row = sc64_cell_row(origin);
    if (col > 0) {
        cell = origin - 1;
        if (sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE &&
            SC64_GRID_GET(path_walkable, cell)) return 0;
    }
    if (col + 1 < MAP_COLS) {
        cell = origin + 1;
        if (sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE &&
            SC64_GRID_GET(path_walkable, cell)) return 0;
    }
    if (row > 0) {
        cell = origin - MAP_COLS;
        if (sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE &&
            SC64_GRID_GET(path_walkable, cell)) return 0;
    }
    if (row + 1 < MAP_ROWS) {
        cell = origin + MAP_COLS;
        if (sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE &&
            SC64_GRID_GET(path_walkable, cell)) return 0;
    }
    return 1;
}
#endif

#ifdef SC64_ROUTE_FLOOD_SPLIT
/* Keep lookup/metadata cold while preserving a hot flood loop. The caller
 * passes the existing seed queue and partial-completeness state unchanged. */
static int GBA_HOT __attribute__((noinline))
sc64_route_flood(int slot, int head, int *tail, int origin,
                 int origin_walkable, int complete)
{
    int qtail = *tail;
#ifndef SC64_ROUTE_PARTIAL_FLOOD
    (void)origin;
    (void)origin_walkable;
#endif
#ifdef SC64_ROUTE_PARTIAL_FLOOD
    if (sc64_route_origin_ready(slot, origin, origin_walkable))
        complete = (qtail == 0);
    else
#endif
    while (head < qtail) {
#ifdef SC64_RACES
        int cell = bfs_queue[head++], col = sc64_cell_col(cell), row = sc64_cell_row(cell);
#else
        int cell = bfs_queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
#endif
        int next;
        /* The stored direction takes the neighbour back toward this cell. */
#ifdef SC64_RACES
#ifdef SC64_ROUTE_VISIT_GRID
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next)) { \
                sc64_route_put(slot, next, (direction)); bfs_queue[qtail++] = (u16)next; \
                SC64_GRID_SET(path_walkable, next, 0); } }
#else
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next) && sc64_route_get(slot, next) == SC64_ROUTE_UNREACHABLE) { \
                sc64_route_put(slot, next, (direction)); bfs_queue[qtail++] = (u16)next; } }
#endif
#else
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next) && field[next] == 0xFFFF) { \
                field[next] = (u16)(((field[cell] & ~7u) + 8u) | (direction)); \
                bfs_queue[qtail++] = (u16)next; } }
#endif
        SC64_FLOOD(col > 0, cell - 1, 1);
        SC64_FLOOD(col + 1 < MAP_COLS, cell + 1, 2);
        SC64_FLOOD(row > 0, cell - MAP_COLS, 3);
        SC64_FLOOD(row + 1 < MAP_ROWS, cell + MAP_COLS, 4);
#undef SC64_FLOOD
#ifdef SC64_ROUTE_PARTIAL_FLOOD
        if (sc64_route_origin_ready(slot, origin, origin_walkable)) {
            if (head < qtail) complete = 0;
            break;
        }
#endif
    }
    *tail = qtail;
    return complete;
}
#define SC64_ROUTE_SLOT_ATTR __attribute__((noinline))
#else
#define SC64_ROUTE_SLOT_ATTR GBA_HOT
#endif
static int SC64_ROUTE_SLOT_ATTR
#ifdef SC64_ROUTE_PARTIAL_FLOOD
sc64_route_slot_for_origin(int goal, int origin, int origin_walkable)
#else
sc64_route_slot(int goal)
#endif
{
    int i, slot = -1, head = 0, tail = 0;
#ifdef SC64_ROUTE_PARTIAL_FLOOD
    int partial_slot = -1, complete = 1;
#endif
#ifndef SC64_RACES
    u16 *field;
#endif
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    if (!sc64_route_configure()) return -1;
#endif
#ifdef SC64_ROUTE_DECAY_FREQ
    sc64_route_age_request();
#endif
    for (i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; i++) {
#ifdef SC64_RANGE_WORK
        if (i == sc64_range_route_copy_slot) continue;
        if ((sc64_route_valid[i] == 1 || sc64_route_valid[i] == 2) &&
            sc64_route_goal[i] == goal) {
#else
        if (sc64_route_valid[i] && sc64_route_goal[i] == goal) {
#endif
#ifdef SC64_ROUTE_PARTIAL_FLOOD
            if (sc64_route_valid[i] == 1 ||
                sc64_route_origin_ready(i, origin, origin_walkable)) {
#ifdef SC64_ROUTE_DECAY_FREQ
                sc64_route_age_touch(i, 1);
#else
                sc64_route_age[i] = ++sc64_route_clock;
#endif
                sc64_route_hits++;
                return i;
            }
            partial_slot = i;
#else
#ifdef SC64_ROUTE_DECAY_FREQ
            sc64_route_age_touch(i, 1);
#else
            sc64_route_age[i] = ++sc64_route_clock;
#endif
            sc64_route_hits++;
            return i;
#endif
        }
#ifdef SC64_ROUTE_DECAY_FREQ
        if (slot < 0 || !sc64_route_valid[i] ||
            (sc64_route_valid[slot] &&
             (sc64_route_age_freq(sc64_route_age[i]) < sc64_route_age_freq(sc64_route_age[slot]) ||
              (sc64_route_age_freq(sc64_route_age[i]) == sc64_route_age_freq(sc64_route_age[slot]) &&
               sc64_route_age_older(sc64_route_age[i], sc64_route_age[slot]))))) slot = i;
#else
        if (slot < 0 || !sc64_route_valid[i] ||
            (sc64_route_valid[slot] && sc64_route_age[i] < sc64_route_age[slot])) slot = i;
#endif
    }
#ifdef SC64_ROUTE_PARTIAL_FLOOD
    if (partial_slot >= 0) slot = partial_slot;
#endif
#ifdef SC64_RANGE_WORK
    if (slot == sc64_range_route_snapshot_slot) {
        sc64_range_route_snapshot_slot = -1;
        memset(&sc64_range_route_snapshot_key, 0,
               sizeof(sc64_range_route_snapshot_key));
    }
#endif
#ifdef SC64_RACES
#ifdef SC64_ROUTE_WORD_CLEAR
    sc64_route_field_clear(sc64_route_field[slot]);
#else
    memset(sc64_route_field[slot], 255,
#ifdef SC64_ROUTE_ACTIVE_STRIDE
           sc64_route_active_bytes
#else
           SC64_ROUTE_BYTES
#endif
           );
#endif
#else
    field = sc64_route_field[slot];
    memset(field, 255, MAP_CELLS * sizeof(*field));
#endif
    if (SC64_GRID_GET(path_walkable, goal)) {
#ifdef SC64_RACES
        sc64_route_put(slot, goal, 0);
#else
        field[goal] = 0;
#endif
        bfs_queue[tail++] = (u16)goal;
#ifdef SC64_ROUTE_VISIT_GRID
        SC64_GRID_SET(path_walkable, goal, 0);
#endif
    } else {
#ifdef SC64_RACES
        int x = sc64_cell_col(goal), y = sc64_cell_row(goal), dx, dy;
#else
        int x = goal % MAP_COLS, y = goal / MAP_COLS, dx, dy;
#endif
        for (dy = -1; dy <= 1; dy++) for (dx = -1; dx <= 1; dx++) {
            int col = x + dx, row = y + dy, cell;
            if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) continue;
            cell = col + row * MAP_COLS;
            if (SC64_GRID_GET(path_walkable, cell)) {
#ifdef SC64_RACES
                if (sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE) {
                    sc64_route_put(slot, cell, 0); bfs_queue[tail++] = (u16)cell;
#ifdef SC64_ROUTE_VISIT_GRID
                    SC64_GRID_SET(path_walkable, cell, 0);
#endif
                }
#else
                field[cell] = 0; bfs_queue[tail++] = (u16)cell;
#endif
            }
        }
    }
#ifdef SC64_ROUTE_FLOOD_SPLIT
#ifdef SC64_ROUTE_PARTIAL_FLOOD
    complete = sc64_route_flood(slot, head, &tail,
                                origin, origin_walkable, complete
                                );
#else
    (void)sc64_route_flood(slot, head, &tail, -1, 0, 1);
#endif
#else
#ifdef SC64_ROUTE_PARTIAL_FLOOD
    if (sc64_route_origin_ready(slot, origin, origin_walkable)) complete = (tail == 0);
    else
#endif
    while (head < tail) {
#ifdef SC64_RACES
        int cell = bfs_queue[head++], col = sc64_cell_col(cell), row = sc64_cell_row(cell);
#else
        int cell = bfs_queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
#endif
        int next;
        /* The stored direction takes the neighbour back toward this cell. */
#ifdef SC64_RACES
#ifdef SC64_ROUTE_VISIT_GRID
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next)) { \
                sc64_route_put(slot, next, (direction)); bfs_queue[tail++] = (u16)next; \
                SC64_GRID_SET(path_walkable, next, 0); } }
#else
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next) && sc64_route_get(slot, next) == SC64_ROUTE_UNREACHABLE) { \
                sc64_route_put(slot, next, (direction)); bfs_queue[tail++] = (u16)next; } }
#endif
#else
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next) && field[next] == 0xFFFF) { \
                field[next] = (u16)(((field[cell] & ~7u) + 8u) | (direction)); \
                bfs_queue[tail++] = (u16)next; } }
#endif
        SC64_FLOOD(col > 0, cell - 1, 1);
        SC64_FLOOD(col + 1 < MAP_COLS, cell + 1, 2);
        SC64_FLOOD(row > 0, cell - MAP_COLS, 3);
        SC64_FLOOD(row + 1 < MAP_ROWS, cell + MAP_COLS, 4);
#undef SC64_FLOOD
#ifdef SC64_ROUTE_PARTIAL_FLOOD
        if (sc64_route_origin_ready(slot, origin, origin_walkable)) {
            if (head < tail) complete = 0;
            break;
        }
#endif
    }
#endif
#ifdef SC64_ROUTE_VISIT_GRID
    /* Each enqueued cell was cleared exactly once, so bfs_queue is a compact
     * restoration list for the temporarily borrowed walkability bits. */
    for (i = 0; i < tail; ++i) SC64_GRID_SET(path_walkable, bfs_queue[i], 1);
#endif
    sc64_route_goal[slot] = (u16)goal;
#ifdef SC64_ROUTE_DECAY_FREQ
#ifdef SC64_ROUTE_PARTIAL_FLOOD
    if (partial_slot >= 0) sc64_route_age_touch(slot, 0);
    else sc64_route_age[slot] = sc64_route_age_pack(1u, sc64_route_clock);
#else
    sc64_route_age[slot] = sc64_route_age_pack(1u, sc64_route_clock);
#endif
#else
    sc64_route_age[slot] = ++sc64_route_clock;
#endif
#ifdef SC64_ROUTE_PARTIAL_FLOOD
    sc64_route_valid[slot] = complete ? 1 : 2;
#else
    sc64_route_valid[slot] = 1;
#endif
    sc64_route_builds++;
    return slot;
}

#undef SC64_ROUTE_SLOT_ATTR

#ifdef SC64_ROUTE_PARTIAL_FLOOD
/* Keep the legacy complete-field API for callers that do not have an origin. */
static int GBA_HOT sc64_route_slot(int goal)
{
    return sc64_route_slot_for_origin(goal, -1, 0);
}
#endif

#if defined(SC64_RACES) && defined(SC64_ROUTE_PACK3)
/* Inflated structure cells are rare actor origins. Keep their gradient walk in
 * ROM so the hot route query stays small; selection/ties remain identical. */
static int __attribute__((noinline))
sc64_route_blocked_start(int slot,int col,int row,int goal,int *out_col,int *out_row)
{
    static const s8 dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    int i,best=-1,cost=0x7fffffff;
    for(i=0;i<4;i++) {
        int cx=col+dx[i],cy=row+dy[i],distance,candidate;
        if(cx<0||cy<0||cx>=MAP_COLS||cy>=MAP_ROWS)continue;
        candidate=cx+cy*MAP_COLS;
        if(sc64_route_get(slot,candidate)==SC64_ROUTE_UNREACHABLE)continue;
        /* Follow each adjacent packed gradient to compare actual path lengths.
         * Direction zero (including a blocked-goal seed) stops this walk. */
        distance=0;
        while(candidate!=goal&&distance<MAP_CELLS) {
            int d=sc64_route_get(slot,candidate);
            if(d<1||d>4)break;
            if(d==1)candidate++;
            else if(d==2)candidate--;
            else if(d==3)candidate+=MAP_COLS;
            else candidate-=MAP_COLS;
            ++distance;
        }
        if(distance<cost){cost=distance;best=i;}
    }
    if(best<0)return 0;
    *out_col=col+dx[best];*out_row=row+dy[best];
    return 1;
}
#endif

#ifdef SC64_ROUTE_L_FAST_PATH
/* The reverse BFS expands left, right, up, then down. Its preferred shortest
 * path runs from the goal horizontally, then vertically. If every cell on that
 * L is walkable, no obstacle elsewhere can change the origin's first step. */
static int GBA_HOT
sc64_route_preferred_l_clear(int start_col, int start_row,
                             int goal_col, int goal_row)
{
    int col = goal_col, row = goal_row;
    while (col != start_col) {
        col += start_col > col ? 1 : -1;
        if (!SC64_GRID_GET(path_walkable, col + row * MAP_COLS)) return 0;
    }
    while (row != start_row) {
        row += start_row > row ? 1 : -1;
        if (!SC64_GRID_GET(path_walkable, col + row * MAP_COLS)) return 0;
    }
    return 1;
}
#endif

static int GBA_HOT
sc64_route_next(int x, int y, int tx, int ty, int *next_x, int *next_y)
{
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    int col, row, goal, cell, slot, direction;
#ifdef SC64_ROUTE_L_FAST_PATH
    int goal_col, goal_row;
#endif
#else
    int col = limit(x / 16, 0, MAP_COLS - 1), row = limit(y / 16, 0, MAP_ROWS - 1);
#ifdef SC64_ROUTE_L_FAST_PATH
    int goal_col = limit(tx / 16, 0, MAP_COLS - 1);
    int goal_row = limit(ty / 16, 0, MAP_ROWS - 1);
    int goal = goal_col + goal_row * MAP_COLS;
#else
    int goal = limit(tx / 16, 0, MAP_COLS - 1) + limit(ty / 16, 0, MAP_ROWS - 1) * MAP_COLS;
#endif
    int cell = col + row * MAP_COLS, slot, direction;
#endif
#ifndef SC64_RACES
    u16 *field;
#endif
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    if (!sc64_route_configure()) return 0;
    col = limit(x / 16, 0, MAP_COLS - 1);
    row = limit(y / 16, 0, MAP_ROWS - 1);
#ifdef SC64_ROUTE_L_FAST_PATH
    goal_col = limit(tx / 16, 0, MAP_COLS - 1);
    goal_row = limit(ty / 16, 0, MAP_ROWS - 1);
    goal = goal_col + goal_row * MAP_COLS;
#else
    goal = limit(tx / 16, 0, MAP_COLS - 1) + limit(ty / 16, 0, MAP_ROWS - 1) * MAP_COLS;
#endif
    cell = col + row * MAP_COLS;
#endif
    if (!path_grid_valid) build_path_grid();
#ifdef SC64_ROUTE_L_FAST_PATH
    if (SC64_GRID_GET(path_walkable, cell) && SC64_GRID_GET(path_walkable, goal) &&
        sc64_route_preferred_l_clear(col, row, goal_col, goal_row)) {
        if (cell == goal) {
            *next_x = tx;
            *next_y = ty;
            return 1;
        }
        /* Reverse the goal-rooted horizontal-then-vertical route. */
        if (row != goal_row) row += row < goal_row ? 1 : -1;
        else col += col < goal_col ? 1 : -1;
        *next_x = col * 16 + 8;
        *next_y = row * 16 + 8;
        return 1;
    }
#endif
#ifdef SC64_ROUTE_PARTIAL_FLOOD
    slot = sc64_route_slot_for_origin(goal, cell, SC64_GRID_GET(path_walkable, cell));
#else
    slot = sc64_route_slot(goal);
#endif
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    if (slot < 0) return 0;
#endif
#ifdef SC64_RACES
    direction = sc64_route_get(slot, cell);
    if (direction == SC64_ROUTE_UNREACHABLE) direction = -1;
#else
    field = sc64_route_field[slot];
    direction = field[cell] == 0xFFFF ? -1 : field[cell] & 7;
#endif
#if defined(SC64_RACES) && defined(SC64_ROUTE_PACK3)
    if (direction < 0 && !SC64_GRID_GET(path_walkable, cell)) {
        if(!sc64_route_blocked_start(slot,col,row,goal,&col,&row))return 0;
    } else if(direction<0)return 0;
    else if(direction==0){*next_x=tx;*next_y=ty;return 1;}
    else if(direction==1)col++;
    else if(direction==2)col--;
    else if(direction==3)row++;
    else row--;
#else
    if (direction < 0 && !SC64_GRID_GET(path_walkable, cell)) {
        static const s8 dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
        int i, best = -1, cost = 0x7fffffff;
        /* Imported/produced units may start in an inflated building cell. */
        for (i = 0; i < 4; i++) {
            int cx = col + dx[i], cy = row + dy[i], distance;
            int candidate;
            if (cx < 0 || cy < 0 || cx >= MAP_COLS || cy >= MAP_ROWS) continue;
#ifdef SC64_RACES
            candidate = cx + cy * MAP_COLS;
            if (sc64_route_get(slot, candidate) == SC64_ROUTE_UNREACHABLE) continue;
            /* Rare blocked starts have no distance nibble. Follow each
             * adjacent gradient to compare true path lengths. */
            distance = 0;
            while (candidate != goal && distance < MAP_CELLS) {
                int d = sc64_route_get(slot, candidate);
                if (d < 1 || d > 4) break;
                if (d == 1) candidate++;
                else if (d == 2) candidate--;
                else if (d == 3) candidate += MAP_COLS;
                else candidate -= MAP_COLS;
                ++distance;
            }
#else
            if (field[cx + cy * MAP_COLS] == 0xFFFF) continue;
            distance = field[cx + cy * MAP_COLS] >> 3;
#endif
            if (distance < cost) { cost = distance; best = i; }
        }
        if (best < 0) return 0;
        col += dx[best]; row += dy[best];
    } else if (direction < 0) return 0;
    else if (direction == 0) { *next_x = tx; *next_y = ty; return 1; }
    else if (direction == 1) col++;
    else if (direction == 2) col--;
    else if (direction == 3) row++;
    else row--;
#endif
    *next_x = col * 16 + 8; *next_y = row * 16 + 8;
    return 1;
}

/* Group formation only needs a reachability decision, not a next step. On
 * complete imported-race fields, any walkable-rooted reverse field is an exact
 * connected-component label: membership in the same field is equivalent to
 * four-neighbour reachability. Blocked starts/targets expand to the same
 * cardinal/diagonal open-cell sets as sc64_route_next. Partial floods and
 * visit-grid builds retain the path-query implementation as their oracle. */
#ifdef SC64_REACHABLE_FORMATION
#if defined(SC64_RACES) && defined(SC64_IMPORTED_MISSION) && \
    !defined(SC64_ROUTE_PARTIAL_FLOOD) && !defined(SC64_ROUTE_VISIT_GRID)
#define SC64_ROUTE_COMPONENT_HINTS 4
static int __attribute__((noinline))
sc64_route_cells_connected(const int *starts, int start_count,
                           const int *goals, int goal_count,
                           const int *other_goals, int other_goal_count,
                           int *hint_slots, int *hint_roots, int *hint_next,
                           int stop_if_slot_reachable)
{
    int i, j, k, reached = 0;
    if (start_count <= 0) return 0;
    if (goal_count <= 0 && other_goal_count <= 0) return 0;
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    if (!sc64_route_configure()) return -1;
#endif
    if (SC64_ROUTE_ACTIVE_SLOTS <= 0) return -1;
    if (!path_grid_valid) build_path_grid();
    for (i = 0; i < start_count; ++i) {
        int slot = -1;
        int source = starts[i];
#ifdef SC64_ROUTE_DECAY_FREQ
        sc64_route_age_request();
#endif
        if (hint_slots && hint_roots) for (j = 0; j < SC64_ROUTE_COMPONENT_HINTS; ++j) {
            int hs = hint_slots[j], hr = hint_roots[j];
            if (hs >= 0 && hs < SC64_ROUTE_ACTIVE_SLOTS &&
                sc64_route_valid[hs] == 1 && sc64_route_goal[hs] == (u16)hr &&
                hr >= 0 && hr < MAP_CELLS && SC64_GRID_GET(path_walkable, hr) &&
                sc64_route_get(hs, source) != SC64_ROUTE_UNREACHABLE) {
                slot = hs;
#ifdef SC64_ROUTE_DECAY_FREQ
                sc64_route_age_touch(hs, 1);
#else
                sc64_route_age[hs] = ++sc64_route_clock;
#endif
                sc64_route_hits++;
                break;
            }
        }
        /* Reuse a complete field rooted at any walkable point in this
         * component; field goals seeded from blocked cells are not labels. */
        for (k = 0; slot < 0 && k < SC64_ROUTE_ACTIVE_SLOTS; ++k) {
            int root;
            if (sc64_route_valid[k] != 1) continue;
            root = (int)sc64_route_goal[k];
            if (root < 0 || root >= MAP_CELLS ||
                !SC64_GRID_GET(path_walkable, root) ||
                sc64_route_get(k, source) == SC64_ROUTE_UNREACHABLE)
                continue;
            slot = k;
#ifdef SC64_ROUTE_DECAY_FREQ
            sc64_route_age_touch(k, 1);
#else
            sc64_route_age[k] = ++sc64_route_clock;
#endif
            sc64_route_hits++;
            break;
        }
        /* No cached field covers this component: build one from this open
         * source and inspect every requested destination before any further
         * allocation can evict it. */
        if (slot < 0) slot = sc64_route_slot(source);
        if (slot < 0) return -1;
        if (hint_slots && hint_roots && hint_next) {
            int h = (*hint_next)++ % SC64_ROUTE_COMPONENT_HINTS;
            hint_slots[h] = slot;
            hint_roots[h] = (int)sc64_route_goal[slot];
        }
        for (j = 0; j < goal_count; ++j)
            if (sc64_route_get(slot, goals[j]) != SC64_ROUTE_UNREACHABLE) {
                reached |= 1;
                break;
            }
        if (stop_if_slot_reachable && (reached & 1)) return 0;
        for (j = 0; j < other_goal_count; ++j)
            if (sc64_route_get(slot, other_goals[j]) != SC64_ROUTE_UNREACHABLE) {
                reached |= 2;
                break;
            }
        if (!stop_if_slot_reachable && reached == 3) return reached;
    }
    if (stop_if_slot_reachable) return (reached & 2) != 0;
    return reached;
}

static int __attribute__((noinline))
sc64_route_reachable_pair_with_hint(int sx, int sy,
                                    int tx1, int ty1, int tx2, int ty2,
                                    int *hint_slots, int *hint_roots, int *hint_next,
                                    int stop_if_slot_reachable)
{
    static const s8 start_dx[4] = {1, -1, 0, 0};
    static const s8 start_dy[4] = {0, 0, 1, -1};
    static const s8 goal_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const s8 goal_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    int start_col, start_row, goal1_col, goal1_row, goal2_col, goal2_row, start;
    int starts[4], goals1[8], goals2[8], start_count = 0, goal1_count = 0, goal2_count = 0;
    int i, result;
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    if (!sc64_route_configure()) return -1;
#endif
    if (!path_grid_valid) build_path_grid();
    start_col = limit(sx / 16, 0, MAP_COLS - 1);
    start_row = limit(sy / 16, 0, MAP_ROWS - 1);
    goal1_col = limit(tx1 / 16, 0, MAP_COLS - 1);
    goal1_row = limit(ty1 / 16, 0, MAP_ROWS - 1);
    goal2_col = limit(tx2 / 16, 0, MAP_COLS - 1);
    goal2_row = limit(ty2 / 16, 0, MAP_ROWS - 1);
    start = start_col + start_row * MAP_COLS;
    if (SC64_GRID_GET(path_walkable, start)) starts[start_count++] = start;
    else for (i = 0; i < 4; ++i) {
        int col = start_col + start_dx[i], row = start_row + start_dy[i];
        int cell;
        if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) continue;
        cell = col + row * MAP_COLS;
        if (SC64_GRID_GET(path_walkable, cell)) starts[start_count++] = cell;
    }
    {
        int goal = goal1_col + goal1_row * MAP_COLS;
        if (SC64_GRID_GET(path_walkable, goal)) goals1[goal1_count++] = goal;
        else for (i = 0; i < 8; ++i) {
            int col = goal1_col + goal_dx[i], row = goal1_row + goal_dy[i];
            int cell;
            if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) continue;
            cell = col + row * MAP_COLS;
            if (SC64_GRID_GET(path_walkable, cell)) goals1[goal1_count++] = cell;
        }
    }
    {
        int goal = goal2_col + goal2_row * MAP_COLS;
        if (SC64_GRID_GET(path_walkable, goal)) goals2[goal2_count++] = goal;
        else for (i = 0; i < 8; ++i) {
            int col = goal2_col + goal_dx[i], row = goal2_row + goal_dy[i];
            int cell;
            if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) continue;
            cell = col + row * MAP_COLS;
            if (SC64_GRID_GET(path_walkable, cell)) goals2[goal2_count++] = cell;
        }
    }
    result = sc64_route_cells_connected(starts, start_count,
                                        goals1, goal1_count, goals2, goal2_count,
                                        hint_slots, hint_roots, hint_next,
                                        stop_if_slot_reachable);
    return result;
}

static int __attribute__((noinline))
sc64_route_reachable_with_hint(int sx, int sy, int tx, int ty,
                               int *hint_slots, int *hint_roots, int *hint_next)
{
    int result = sc64_route_reachable_pair_with_hint(sx, sy, tx, ty, tx, ty,
                                                     hint_slots, hint_roots, hint_next, 0);
    return result < 0 ? -1 : (result & 1) != 0;
}

static int __attribute__((noinline))
sc64_route_reachable_pair(int sx, int sy, int tx1, int ty1, int tx2, int ty2)
{
    return sc64_route_reachable_pair_with_hint(sx, sy, tx1, ty1, tx2, ty2, 0, 0, 0, 0);
}
static int __attribute__((noinline))
sc64_route_formation_fallback_with_hint(int sx, int sy,
                                        int slot_x, int slot_y, int tx, int ty,
                                        int *hint_slots, int *hint_roots, int *hint_next)
{
    return sc64_route_reachable_pair_with_hint(sx, sy, slot_x, slot_y, tx, ty,
                                               hint_slots, hint_roots, hint_next, 1);
}
static int __attribute__((noinline))
sc64_route_reachable(int sx, int sy, int tx, int ty)
{
    return sc64_route_reachable_with_hint(sx, sy, tx, ty, 0, 0, 0);
}
#else
static int __attribute__((noinline))
sc64_route_reachable_pair_with_hint(int sx, int sy,
                                    int tx1, int ty1, int tx2, int ty2,
                                    int *hint_slots, int *hint_roots, int *hint_next,
                                    int stop_if_slot_reachable)
{
    (void)sx; (void)sy; (void)tx1; (void)ty1; (void)tx2; (void)ty2;
    (void)hint_slots; (void)hint_roots; (void)hint_next;
    (void)stop_if_slot_reachable;
    return -1;
}
static int __attribute__((noinline))
sc64_route_reachable_with_hint(int sx, int sy, int tx, int ty,
                               int *hint_slots, int *hint_roots, int *hint_next)
{
    int result = sc64_route_reachable_pair_with_hint(sx, sy, tx, ty, tx, ty,
                                                     hint_slots, hint_roots, hint_next, 0);
    return result < 0 ? -1 : (result & 1) != 0;
}
static int __attribute__((noinline))
sc64_route_reachable(int sx, int sy, int tx, int ty)
{
    return sc64_route_reachable_with_hint(sx, sy, tx, ty, 0, 0, 0);
}
static int __attribute__((noinline))
sc64_route_reachable_pair(int sx, int sy, int tx1, int ty1, int tx2, int ty2)
{
    return sc64_route_reachable_pair_with_hint(sx, sy, tx1, ty1, tx2, ty2, 0, 0, 0, 0);
}
static int __attribute__((noinline))
sc64_route_formation_fallback_with_hint(int sx, int sy,
                                        int slot_x, int slot_y, int tx, int ty,
                                        int *hint_slots, int *hint_roots, int *hint_next)
{
    return sc64_route_reachable_pair_with_hint(sx, sy, slot_x, slot_y, tx, ty,
                                               hint_slots, hint_roots, hint_next, 1);
}
#endif
#endif /* SC64_REACHABLE_FORMATION */
#ifdef SC64_ROUTES_LOCAL_GRID_GET
#undef SC64_GRID_GET
#undef SC64_ROUTES_LOCAL_GRID_GET
#endif
#ifdef SC64_ROUTES_LOCAL_GRID_SET
#undef SC64_GRID_SET
#undef SC64_ROUTES_LOCAL_GRID_SET
#endif
#endif
