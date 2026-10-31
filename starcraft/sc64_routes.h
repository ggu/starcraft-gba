#ifndef SC64_ROUTES_H
#define SC64_ROUTES_H

#include "sc64_depot_goal.h"

/* Point goals are cell indices; the upper bit identifies depot slots. */
#define SC64_ROUTE_DEPOT_TAG 0x8000u
_Static_assert(MAP_CELLS <= SC64_ROUTE_DEPOT_TAG, "route point goals overlap depot keys");

/* Standalone route fixtures still use byte grids; game.c supplies the packed
 * accessor for SC64_LARGE_MAPS builds. */
#ifndef SC64_GRID_GET
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_ROUTES_LOCAL_GRID_GET
#if defined(SC64_RACES) && !defined(SC64_GRID_SET)
#define SC64_GRID_SET(grid, cell, value) ((grid)[(cell)] = (u8)((value) != 0))
#define SC64_ROUTES_LOCAL_GRID_SET
#endif
#endif

#if defined(SC64_ROUTE_MONOTONE_PROBE) && defined(SC64_RACES) && \
    !defined(SC64_ROUTE_PARTIAL_FLOOD)
#define SC64_ROUTE_MONOTONE_ENABLED 1
#include "sc64_route_monotone_probe.h"
#endif
#if defined(GBA_HOST) && defined(SC64_ROUTE_MONOTONE_ENABLED) && \
    defined(SC64_ROUTE_MONOTONE_DIAGNOSTICS)
#define SC64_ROUTE_MONOTONE_DIAG_ENABLED 1
#endif

#if defined(SC64_DEPOT_BOUNDED) && !defined(SC64_RACES)
#error "SC64_DEPOT_BOUNDED requires SC64_RACES"
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
#ifdef SC64_ROUTE_NEW_CELL
#if !defined(SC64_RACES) || !defined(SC64_ROUTE_PACK3) || \
    !defined(SC64_ROUTE_VISIT_GRID) || !defined(SC64_ROUTE_FLOOD_SPLIT) || \
    defined(SC64_ROUTE_PARTIAL_FLOOD)
#error "SC64_ROUTE_NEW_CELL requires a full packed visited-grid split flood"
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
    if (slots > SC64_ROUTE_ACTIVE_MAX_SLOTS) slots = SC64_ROUTE_ACTIVE_MAX_SLOTS;
    if (slots == 0) return 0;
    if (sc64_route_config_cols == cols && sc64_route_config_rows == rows &&
        sc64_route_active_bytes == bytes && sc64_route_active_slots == slots) return 1;
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
#define SC64_ROUTE_ACTIVE_SLOTS SC64_ROUTE_SLOTS
#endif
volatile u32 sc64_route_hits, sc64_route_builds;

#ifdef SC64_ROUTE_MONOTONE_DIAG_ENABLED
typedef struct {
    unsigned attempts, hits, fails, budget, nodes, candidates, memo_checks, memo_clear_bytes;
    unsigned have_frame, current_frame, current_floods, current_nodes;
    unsigned max_flood_frame, max_floods, max_node_frame, max_frame_nodes;
} sc64_route_probe_host_diag;
static sc64_route_probe_host_diag sc64_route_probe_diag;

static void sc64_route_probe_host_finish_frame(void)
{
    if (!sc64_route_probe_diag.have_frame) return;
    if (sc64_route_probe_diag.current_floods > sc64_route_probe_diag.max_floods) {
        sc64_route_probe_diag.max_floods = sc64_route_probe_diag.current_floods;
        sc64_route_probe_diag.max_flood_frame = sc64_route_probe_diag.current_frame;
    }
    if (sc64_route_probe_diag.current_nodes > sc64_route_probe_diag.max_frame_nodes) {
        sc64_route_probe_diag.max_frame_nodes = sc64_route_probe_diag.current_nodes;
        sc64_route_probe_diag.max_node_frame = sc64_route_probe_diag.current_frame;
    }
}

static void sc64_route_probe_host_flood(unsigned nodes)
{
    unsigned now = (unsigned)frame;
    if (sc64_route_probe_diag.have_frame && sc64_route_probe_diag.current_frame != now) {
        sc64_route_probe_host_finish_frame();
        sc64_route_probe_diag.current_floods = 0;
        sc64_route_probe_diag.current_nodes = 0;
    }
    sc64_route_probe_diag.have_frame = 1;
    sc64_route_probe_diag.current_frame = now;
    sc64_route_probe_diag.current_floods++;
    sc64_route_probe_diag.current_nodes += nodes;
}
#endif

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

#ifdef SC64_ROUTE_NEW_CELL
static void __attribute__((noinline)) sc64_route_put(int slot, int cell, int direction)
#else
static void GBA_HOT sc64_route_put(int slot, int cell, int direction)
#endif
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
#ifdef SC64_ROUTE_DECAY_FREQ
    memset(sc64_route_age, 0, sizeof(sc64_route_age));
    sc64_route_clock = 0;
#endif
}

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

#ifdef SC64_ROUTE_NEW_CELL
/* Only a fresh full flood may call this: each destination's three bits are
 * still 111, and the borrowed visited grid prevents a second write. */
static void GBA_HOT __attribute__((noinline))
sc64_route_put_new(u8 *field, int cell, int direction)
{
    u32 bit=(u32)cell*3u;
    unsigned byte=bit>>3, shift=bit&7u;
    unsigned clear=(7u^(unsigned)direction)<<shift;
    field[byte]&=(u8)~clear;
    if(shift>5u)field[byte+1]&=(u8)~(clear>>8);
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
#ifdef SC64_ROUTE_NEW_CELL
    u8 *new_field = sc64_route_field[slot];
#endif
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
#ifdef SC64_ROUTE_NEW_CELL
#define SC64_ROUTE_FLOOD_PUT(next, direction) sc64_route_put_new(new_field, (next), (direction))
#else
#define SC64_ROUTE_FLOOD_PUT(next, direction) sc64_route_put(slot, (next), (direction))
#endif
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next)) { \
                SC64_ROUTE_FLOOD_PUT(next, direction); bfs_queue[qtail++] = (u16)next; \
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
#ifdef SC64_ROUTE_FLOOD_PUT
#undef SC64_ROUTE_FLOOD_PUT
#endif
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
        if (sc64_route_valid[i] && sc64_route_goal[i] == goal) {
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
#ifdef SC64_ROUTE_MONOTONE_DIAG_ENABLED
    sc64_route_probe_host_flood((unsigned)tail);
#endif
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

#if defined(SC64_RACES) && !defined(SC64_ROUTE_PARTIAL_FLOOD)
#ifdef SC64_ROUTE_MONOTONE_ENABLED
static int __attribute__((noinline))
sc64_route_step_slot_mode(int origin, int goal, int allow_build)
{
    int i, used = 0;
    if (origin == goal || !SC64_GRID_GET(path_walkable, origin) ||
        !SC64_GRID_GET(path_walkable, goal)) return allow_build ? sc64_route_slot(goal) : -1;
    for (i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i)
        if (sc64_route_valid[i] && sc64_route_goal[i] == (u16)goal) return sc64_route_slot(goal);
#ifdef SC64_ROUTE_DECAY_FREQ
    if ((sc64_route_clock & SC64_ROUTE_RECENCY_MASK) == SC64_ROUTE_RECENCY_MASK)
        return allow_build ? sc64_route_slot(goal) : -1;
#endif
    for (i = 0; i < SC64_ROUTE_ACTIVE_SLOTS && used < 2; ++i) {
        int root, rx, ry, gx, gy, cur, steps;
        if (sc64_route_valid[i] != 1 || (sc64_route_goal[i] & SC64_ROUTE_DEPOT_TAG)) continue;
        root = (int)sc64_route_goal[i];
        if (!SC64_GRID_GET(path_walkable, root)) continue;
        rx = root % MAP_COLS; ry = root / MAP_COLS;
        gx = goal % MAP_COLS; gy = goal / MAP_COLS;
        if ((rx > gx ? rx-gx : gx-rx) + (ry > gy ? ry-gy : gy-ry) > 4) continue;
        ++used;
        cur = origin;
        for (steps = 0; steps < 128 && cur != goal; ++steps) {
            int d, x, y;
            if (!SC64_GRID_GET(path_walkable, cur)) break;
            d = sc64_route_get(i, cur);
            if (d < 1 || d > 4) break;
            x = cur % MAP_COLS; y = cur / MAP_COLS;
            if (d == 1) ++x; else if (d == 2) --x;
            else if (d == 3) ++y; else --y;
            if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS) break;
            cur = x + y * MAP_COLS;
            if (!SC64_GRID_GET(path_walkable, cur)) break;
        }
        if (cur != goal) continue;
#ifdef SC64_ROUTE_DECAY_FREQ
        sc64_route_age_request();
        sc64_route_age_touch(i, 1);
#else
        sc64_route_age[i] = ++sc64_route_clock;
#endif
        sc64_route_hits++;
        return i;
    }
    return allow_build ? sc64_route_slot(goal) : -1;
}

static int __attribute__((noinline)) sc64_route_step_slot(int origin, int goal)
{
    return sc64_route_step_slot_mode(origin, goal, 1);
}
#else
/* FIFO L/R/U/D discovery selects the lexicographically first shortest path
 * from the root. Every suffix of that path is the same preferred shortest
 * path from its own root, so a new goal on the cached gradient preserves the
 * exact next step, including ties. Blocked endpoints and partial fields use
 * the ordinary lookup instead.
 *
 * Try at most two nearby-root fields and 128 steps each, then fall back. Keep
 * the cache search and fallback dispatch in ROM, with no additional state.
 * Only the next-step caller may consume a field with a different root. */
static int __attribute__((noinline))
sc64_route_step_slot(int origin, int goal)
{
    int i, used = 0;
    if (origin == goal || !SC64_GRID_GET(path_walkable, origin) ||
        !SC64_GRID_GET(path_walkable, goal)) return sc64_route_slot(goal);
    for (i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i)
        if (sc64_route_valid[i] && sc64_route_goal[i] == (u16)goal) return sc64_route_slot(goal);
#ifdef SC64_ROUTE_DECAY_FREQ
    if ((sc64_route_clock & SC64_ROUTE_RECENCY_MASK) == SC64_ROUTE_RECENCY_MASK) return sc64_route_slot(goal);
#endif
    for (i = 0; i < SC64_ROUTE_ACTIVE_SLOTS && used < 2; ++i) {
        int root, rx, ry, gx, gy, cur, steps;
        if (sc64_route_valid[i] != 1 || (sc64_route_goal[i] & SC64_ROUTE_DEPOT_TAG)) continue;
        root = (int)sc64_route_goal[i];
        if (!SC64_GRID_GET(path_walkable, root)) continue;
        rx = root % MAP_COLS; ry = root / MAP_COLS;
        gx = goal % MAP_COLS; gy = goal / MAP_COLS;
        if ((rx > gx ? rx-gx : gx-rx) + (ry > gy ? ry-gy : gy-ry) > 4) continue;
        ++used;
        cur = origin;
        for (steps = 0; steps < 128 && cur != goal; ++steps) {
            int d, x, y;
            if (!SC64_GRID_GET(path_walkable, cur)) break;
            d = sc64_route_get(i, cur);
            if (d < 1 || d > 4) break;
            x = cur % MAP_COLS; y = cur / MAP_COLS;
            if (d == 1) ++x; else if (d == 2) --x;
            else if (d == 3) ++y; else --y;
            if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS) break;
            cur = x + y * MAP_COLS;
            if (!SC64_GRID_GET(path_walkable, cur)) break;
        }
        if (cur != goal) continue;
#ifdef SC64_ROUTE_DECAY_FREQ
        sc64_route_age_request();
        sc64_route_age_touch(i, 1);
#else
        sc64_route_age[i] = ++sc64_route_clock;
#endif
        sc64_route_hits++;
        return i;
    }
    return sc64_route_slot(goal);
}
#endif
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

#ifdef SC64_ROUTE_MONOTONE_ENABLED
static int __attribute__((noinline))
sc64_route_monotone_next_step(int origin, int goal, int tx, int ty,
                              int *next_x, int *next_y)
{
    int i, predecessor = -1, status, cols = MAP_COLS, rows = MAP_ROWS;
    sc64_route_probe_stats stats;
    int active_cells;
    if (cols <= 0 || rows <= 0 || cols > 128 || rows > 128 ||
        cols * rows > MAP_CELLS) return 0;
    active_cells = cols * rows;
    if (origin < 0 || origin >= active_cells || goal < 0 || goal >= active_cells ||
        !SC64_GRID_GET(path_walkable, origin)) return 0;
    for (i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i)
        if (sc64_route_valid[i] == 1 && sc64_route_goal[i] == (u16)goal) return 0;
#ifdef SC64_ROUTE_DECAY_FREQ
    if ((sc64_route_clock & SC64_ROUTE_RECENCY_MASK) == SC64_ROUTE_RECENCY_MASK) return 0;
#endif
    status = sc64_route_monotone_probe(path_walkable, cols, rows, active_cells,
                                       bfs_queue, sizeof(bfs_queue), origin, goal,
                                       512u, &predecessor, &stats);
#ifdef SC64_ROUTE_MONOTONE_DIAG_ENABLED
    sc64_route_probe_diag.attempts++;
    sc64_route_probe_diag.nodes += stats.nodes;
    sc64_route_probe_diag.candidates += stats.candidates;
    sc64_route_probe_diag.memo_checks += stats.memo_checks;
    sc64_route_probe_diag.memo_clear_bytes += stats.memo_clear_bytes;
    if (status == 1) sc64_route_probe_diag.hits++;
    else {
        sc64_route_probe_diag.fails++;
        if (status == SC64_ROUTE_PROBE_BUDGET) sc64_route_probe_diag.budget++;
    }
#endif
    if (status != 1) return 0;
#ifdef SC64_ROUTE_DECAY_FREQ
    sc64_route_age_request();
#endif
    if (predecessor == origin) { *next_x = tx; *next_y = ty; }
    else {
        *next_x = (predecessor % cols) * 16 + 8;
        *next_y = (predecessor / cols) * 16 + 8;
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
#if defined(SC64_RACES) && !defined(SC64_ROUTE_PARTIAL_FLOOD)
#ifdef SC64_ROUTE_MONOTONE_ENABLED
    slot = sc64_route_step_slot_mode(cell, goal, 0);
    if (slot < 0) {
        if (sc64_route_monotone_next_step(cell, goal, tx, ty, next_x, next_y)) return 1;
        slot = sc64_route_slot(goal);
    }
#else
    slot = sc64_route_step_slot(cell, goal);
#endif
#elif defined(SC64_ROUTE_PARTIAL_FLOOD)
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

/* Depot fields use a separate key namespace and all reachable deposit cells
 * as roots. A depot moving, being removed or reusing its slot must invalidate
 * the route cache. The existing topology/load invalidation owns that contract.
 * Full depot fields serve all workers. With the bounded capability, incomplete
 * fields serve only already-discovered origins and are never component proofs.
 * A paired grid accessor may temporarily borrow and restore walkability bits. */
static int __attribute__((noinline))
sc64_route_depot_slot_for_origin(int depot_id, int center_x, int center_y, int origin)
{
    int i, slot = -1, head = 0, tail = 0;
    int x0, x1, y0, y1, col, row;
    int partial_slot=-1, complete=1;
    u16 key;
#ifndef SC64_RACES
    u16 *field;
#endif
    if (depot_id < 0 || depot_id >= (int)SC64_ROUTE_DEPOT_TAG ||
        MAP_COLS <= 0 || MAP_ROWS <= 0 ||
        (unsigned)MAP_COLS > (unsigned)MAP_CELLS / (unsigned)MAP_ROWS ||
        center_x < 0 || center_y < 0 || center_x / 16 >= MAP_COLS ||
        center_y / 16 >= MAP_ROWS || origin < -1 ||
        (origin >= 0 && (unsigned)origin >= (unsigned)MAP_COLS * (unsigned)MAP_ROWS)) return -1;
#ifndef SC64_DEPOT_BOUNDED
    (void)origin;
#endif
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    if (!sc64_route_configure()) return -1;
#endif
    key = (u16)(SC64_ROUTE_DEPOT_TAG | (unsigned)depot_id);
    if (!path_grid_valid) build_path_grid();
#ifdef SC64_DEPOT_BOUNDED
    if (origin>=0 && !SC64_GRID_GET(path_walkable,origin)) origin=-1;
#endif
#ifdef SC64_ROUTE_DECAY_FREQ
    sc64_route_age_request();
#endif
    for (i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i) {
        if (sc64_route_valid[i] && sc64_route_goal[i] == key) {
#ifdef SC64_DEPOT_BOUNDED
            if (sc64_route_valid[i] != 1 &&
                (origin < 0 || sc64_route_get(i,origin)==SC64_ROUTE_UNREACHABLE)) {
                partial_slot=i; continue;
            }
#else
            if (sc64_route_valid[i] != 1) continue;
#endif
#ifdef SC64_ROUTE_DECAY_FREQ
            sc64_route_age_touch(i, 1);
#else
            sc64_route_age[i] = ++sc64_route_clock;
#endif
            sc64_route_hits++;
            return i;
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
    if (partial_slot>=0) slot=partial_slot;
    if (slot < 0) return -1;
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
    x0 = center_x >= 32 ? (center_x - 32) / 16 : 0;
    y0 = center_y >= 32 ? (center_y - 32) / 16 : 0;
    x1 = (center_x + 32) / 16; if (x1 >= MAP_COLS) x1 = MAP_COLS - 1;
    y1 = (center_y + 32) / 16; if (y1 >= MAP_ROWS) y1 = MAP_ROWS - 1;
    for (row = y0; row <= y1; ++row) for (col = x0; col <= x1; ++col) {
        int px, py, cell = col + row * MAP_COLS;
        if (!SC64_GRID_GET(path_walkable, cell) ||
            !sc64_depot_cell_point(center_x, center_y, col, row, &px, &py)) continue;
#ifdef SC64_RACES
        sc64_route_put(slot, cell, 0);
#ifdef SC64_GRID_SET
        SC64_GRID_SET(path_walkable, cell, 0);
#endif
#else
        field[cell] = 0;
#endif
        bfs_queue[tail++] = (u16)cell;
    }
    while (head < tail) {
#ifdef SC64_DEPOT_BOUNDED
        if (origin>=0 && sc64_route_get(slot,origin)!=SC64_ROUTE_UNREACHABLE) {
            complete=0; break;
        }
#endif
        int cell = bfs_queue[head++], next;
#ifdef SC64_RACES
        int x = sc64_cell_col(cell), y = sc64_cell_row(cell);
#else
        int x = cell % MAP_COLS, y = cell / MAP_COLS;
#endif
#ifdef SC64_RACES
#ifdef SC64_GRID_SET
/* The game supplies paired accessors. Borrow its walkable bit as visited
 * during this synchronous flood, then restore every queued cell below.
 * Read-only custom accessors retain the field-based visited test. */
#define SC64_DEPOT_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next)) { \
                sc64_route_put(slot, next, (direction)); bfs_queue[tail++] = (u16)next; \
                SC64_GRID_SET(path_walkable, next, 0); } }
#else
#define SC64_DEPOT_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next) && sc64_route_get(slot, next) == SC64_ROUTE_UNREACHABLE) { \
                sc64_route_put(slot, next, (direction)); bfs_queue[tail++] = (u16)next; } }
#endif
#else
#define SC64_DEPOT_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next) && field[next] == 0xFFFF) { \
                field[next] = (u16)(((field[cell] & ~7u) + 8u) | (direction)); bfs_queue[tail++] = (u16)next; } }
#endif
        SC64_DEPOT_FLOOD(x > 0, cell - 1, 1);
        SC64_DEPOT_FLOOD(x + 1 < MAP_COLS, cell + 1, 2);
        SC64_DEPOT_FLOOD(y > 0, cell - MAP_COLS, 3);
        SC64_DEPOT_FLOOD(y + 1 < MAP_ROWS, cell + MAP_COLS, 4);
#undef SC64_DEPOT_FLOOD
    }
#if defined(SC64_RACES) && defined(SC64_GRID_SET)
    for (i=0;i<tail;++i) SC64_GRID_SET(path_walkable,bfs_queue[i],1);
#endif
    sc64_route_goal[slot] = key;
#ifdef SC64_ROUTE_DECAY_FREQ
    sc64_route_age[slot] = sc64_route_age_pack(1u, sc64_route_clock);
#else
    sc64_route_age[slot] = ++sc64_route_clock;
#endif
    sc64_route_valid[slot] = complete ? 1 : 2;
    sc64_route_builds++;
#ifdef SC64_ROUTE_MONOTONE_DIAG_ENABLED
    sc64_route_probe_host_flood((unsigned)tail);
#endif
    return slot;
}

static int __attribute__((unused)) sc64_route_depot_slot(int depot_id,int center_x,int center_y)
{
    return sc64_route_depot_slot_for_origin(depot_id,center_x,center_y,-1);
}

static int
sc64_route_depot_direction(int slot, int cell)
{
#ifdef SC64_RACES
    int d = sc64_route_get(slot, cell);
    return d == SC64_ROUTE_UNREACHABLE ? -1 : d;
#else
    u16 d = sc64_route_field[slot][cell];
    return d == 0xFFFF ? -1 : d & 7;
#endif
}

static int __attribute__((noinline, unused))
sc64_route_depot_next(int x, int y, int depot_id, int center_x, int center_y,
                      int *next_x, int *next_y)
{
    static const signed char dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
    int slot, col, row, cell, direction;
    if (!next_x || !next_y || x < 0 || y < 0 ||
        MAP_COLS <= 0 || MAP_ROWS <= 0 || x / 16 >= MAP_COLS || y / 16 >= MAP_ROWS) return 0;
    col = x / 16; row = y / 16; cell = col + row * MAP_COLS;
#ifdef SC64_DEPOT_BOUNDED
    slot = sc64_route_depot_slot_for_origin(depot_id,center_x,center_y,cell);
#else
    slot = sc64_route_depot_slot(depot_id, center_x, center_y);
#endif
    if (slot < 0) return 0;
    direction = sc64_route_depot_direction(slot, cell);
    if (direction < 0 && !SC64_GRID_GET(path_walkable, cell)) {
        int i, best = -1, cost = MAP_CELLS + 1;
        for (i = 0; i < 4; ++i) {
            int nx = col + dx[i], ny = row + dy[i], current, distance = 0, d;
            if (nx < 0 || ny < 0 || nx >= MAP_COLS || ny >= MAP_ROWS) continue;
            current = nx + ny * MAP_COLS;
            d = sc64_route_depot_direction(slot, current);
            while (d > 0 && d <= 4 && distance < MAP_CELLS) {
                if (d == 1) ++current; else if (d == 2) --current;
                else if (d == 3) current += MAP_COLS; else current -= MAP_COLS;
                ++distance;
                d = sc64_route_depot_direction(slot, current);
            }
            if (d == 0 && distance < cost) { cost = distance; best = i; }
        }
        if (best < 0) return 0;
        col += dx[best]; row += dy[best];
    } else if (direction < 0) return 0;
    else if (direction == 0)
        return sc64_depot_cell_point(center_x, center_y, col, row, next_x, next_y);
    else if (direction == 1) ++col;
    else if (direction == 2) --col;
    else if (direction == 3) ++row;
    else if (direction == 4) --row;
    else return 0;
    *next_x = col * 16 + 8; *next_y = row * 16 + 8;
    return 1;
}

/* Group formation only needs a reachability decision, not a next step. On
 * complete imported-race fields, any walkable-rooted reverse field is an exact
 * connected-component label: membership in the same field is equivalent to
 * four-neighbour reachability. Blocked starts/targets expand to the same
 * cardinal/diagonal open-cell sets as sc64_route_next. Partial floods retain
 * the path-query implementation as their oracle; visit-grid floods restore all
 * borrowed walkability bits before exposing a complete field (valid == 1). */
#ifdef SC64_REACHABLE_FORMATION
#if defined(SC64_RACES) && defined(SC64_IMPORTED_MISSION) && \
    !defined(SC64_ROUTE_PARTIAL_FLOOD)
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
