/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
#define GBA_HOT
#define GBA_HOST
#define SC64_RACES
#define SC64_ZP_CAMPAIGN_ACTORS
#define SC64_ROUTE_GRID_CELLS_PER_BYTE 8
static int fixture_cols, fixture_rows, path_grid_valid = 1;
#define MAP_COLS fixture_cols
#define MAP_ROWS fixture_rows
enum { MAP_CELLS = 128 * 128 };
static u8 path_walkable[MAP_CELLS / 8];
static u16 bfs_queue[MAP_CELLS];
static u32 frame;
static unsigned fallbacks;
#define SC64_ROUTE_FACILITY_FIFO_FALLBACK() (++fallbacks)
#define SC64_GRID_GET(grid, c) (((grid)[(c) >> 3] >> ((c) & 7)) & 1u)
#define SC64_GRID_SET(grid, c, v) do { \
    if (v) (grid)[(c) >> 3] |= (u8)(1u << ((c) & 7)); \
    else (grid)[(c) >> 3] &= (u8)~(1u << ((c) & 7)); \
} while (0)
static int sc64_cell_col(unsigned c) { return c % (unsigned)fixture_cols; }
static int sc64_cell_row(unsigned c) { return c / (unsigned)fixture_cols; }
static int limit(int n, int lo, int hi) { return n < lo ? lo : n > hi ? hi : n; }
static void build_path_grid(void) { path_grid_valid = 1; }
#include "../starcraft/sc64_routes.h"

static u8 direction[MAP_CELLS], packed[MAP_CELLS * 3 / 8];
static u16 oracle_queue[MAP_CELLS];
static u32 rng = 0x712f641bu;
static unsigned random_word(void) { rng = rng * 1664525u + 1013904223u; return rng; }

/* Independent geometry for the union of inclusive tile pixels with the depot
 * circle, and a scalar BFS that checks the bounded stop between pops. */
static int reference(int cx, int cy, int origin, const int *edges, int *valid)
{
    int head = 0, tail = 0;
    *valid = 1;
    memset(direction, 7, sizeof(direction));
    if (origin >= 0 && !SC64_GRID_GET(path_walkable, origin)) origin = -1;
    for (int y = 0; y < fixture_rows; ++y) for (int x = 0; x < fixture_cols; ++x) {
        int c = x + y * fixture_cols, seed;
        if (!SC64_GRID_GET(path_walkable, c)) continue;
        if (edges) {
            int l = edges[0] / 16 - 1, r = edges[2] / 16;
            int t = edges[1] / 16 - 1, b = edges[3] / 16;
            seed = ((x == l || x == r) && y > t && y < b) ||
                   ((y == t || y == b) && x > l && x < r);
        } else {
            int dx = cx - limit(cx, x * 16, x * 16 + 15);
            int dy = cy - limit(cy, y * 16, y * 16 + 15);
            seed = dx * dx + dy * dy <= 1024;
        }
        if (seed) { direction[c] = 0; oracle_queue[tail++] = (u16)c; }
    }
    while (head < tail) {
        if (origin >= 0 && direction[origin] != 7) { *valid = 2; break; }
        int c = oracle_queue[head++], x = c % fixture_cols, y = c / fixture_cols;
        int nb[] = { x ? c - 1 : -1, x + 1 < fixture_cols ? c + 1 : -1,
                     y ? c - fixture_cols : -1, y + 1 < fixture_rows ? c + fixture_cols : -1 };
        for (int i = 0; i < 4; ++i) if (nb[i] >= 0 && direction[nb[i]] == 7 &&
            SC64_GRID_GET(path_walkable, nb[i])) {
            direction[nb[i]] = (u8)(i + 1);
            oracle_queue[tail++] = (u16)nb[i];
        }
    }
    memset(packed, 255, sizeof(packed));
    for (int c = 0; c < fixture_cols * fixture_rows; ++c) for (int b = 0; b < 3; ++b)
        if (!(direction[c] & (1 << b))) packed[(3 * c + b) / 8] &= (u8)~(1u << ((3 * c + b) % 8));
    return tail;
}

static int query(int id, int cx, int cy, int origin, const int *edges,
                 int prior_slot, int hit)
{
    u8 before[sizeof(path_walkable)], old[sizeof(path_walkable)];
    unsigned builds = sc64_route_builds, hits = sc64_route_hits;
    unsigned prior_fallbacks = fallbacks;
    unsigned nodes = sc64_route_probe_diag.current_nodes;
    unsigned floods = sc64_route_probe_diag.current_floods;
    memcpy(before, path_walkable, sizeof(before));
    memcpy(old, SC64_ROUTE_OLD_GRID, sizeof(old));
    int slot = sc64_route_facility_slot_for_origin(id, cx, cy, origin, edges);
    assert(slot >= 0 && (prior_slot < 0 || slot == prior_slot));
    assert(sc64_route_goal[slot] == (SC64_ROUTE_DEPOT_TAG | (edges ? SC64_ROUTE_MELEE_ID_TAG : 0) | (unsigned)id));
    assert(sc64_route_builds == builds + !hit && sc64_route_hits == hits + hit);
    assert(!memcmp(before, path_walkable, sizeof(before)));

    int expected_nodes = 0;
    if (!hit) {
        int valid;
        expected_nodes = reference(cx, cy, origin, edges, &valid);
        assert(sc64_route_valid[slot] == valid);
        assert(sc64_route_probe_diag.current_nodes == nodes + (unsigned)expected_nodes);
        assert(!memcmp(sc64_route_field[slot], packed, sc64_route_active_bytes));
    } else assert(!memcmp(sc64_route_field[slot], packed, sc64_route_active_bytes));
    /* The legacy queue can use all MAP_CELLS entries; its very largest
     * fills can reach the scratch-only retained grid. The FIFO cannot. */
    int legacy = !(fixture_cols == 64 || fixture_cols == 96 || fixture_cols == 128) ||
                 fallbacks != prior_fallbacks;
#ifndef SC64_ROUTE_FACILITY_FIFO_PROBE
    legacy = 1;
#endif
    if (!legacy || (unsigned)expected_nodes <= MAP_CELLS - sizeof(old) / sizeof(u16))
        assert(!memcmp(old, SC64_ROUTE_OLD_GRID, sizeof(old)));
    assert(sc64_route_probe_diag.current_floods == floods + !hit);
    return slot;
}

static void run_map(int w, int h)
{
    fixture_cols = w; fixture_rows = h;
    const int cx = w / 2 * 16 + 8, cy = h / 2 * 16 + 8;
    const int center = w / 2 + h / 2 * w;
    const int edge[] = { cx - 24, cy - 24, cx + 24, cy + 24 };
    const int border[] = { 0, 0, 48, 32 };
    unsigned before = fallbacks;
    sc64_routes_reset();
    assert(sc64_route_configure());
    memset(SC64_ROUTE_OLD_GRID, 0xa5, sizeof(path_walkable));
    memset(path_walkable, 255, sizeof(path_walkable));
    int slot = query(1, cx, cy, center, 0, -1, 0);
    assert(sc64_route_valid[slot] == 2);
    query(1, cx, cy, center, 0, slot, 1);
    query(1, cx, cy, 0, 0, slot, 0);
    query(1, cx, cy, -1, 0, slot, 0);
    assert(sc64_route_valid[slot] == 1);
    query(1, cx, cy, 0, 0, slot, 1);
    const int edge_seed = (edge[0] / 16 - 1) + (edge[1] / 16) * w;
    slot = query(1, cx, cy, edge_seed, edge, -1, 0);
    assert(sc64_route_valid[slot] == 2);
    query(1, cx, cy, edge_seed, edge, slot, 1);
    query(1, cx, cy, 0, edge, slot, 0);
    query(2, 8, 8, w - 1, border, -1, 0);
    SC64_GRID_SET(path_walkable, 0, 0);
    slot = query(3, cx, cy, 0, 0, -1, 0);
    assert(sc64_route_valid[slot] == 1); /* blocked origin means unbounded */
    for (int y = 0; y < h; ++y) SC64_GRID_SET(path_walkable, y * w + w / 4, 0);
    slot = query(4, cx, cy, w, edge, -1, 0);
    assert(sc64_route_valid[slot] == 1 && sc64_route_get(slot, w) == 7);
    memset(path_walkable, 0, sizeof(path_walkable));
    SC64_GRID_SET(path_walkable, w, 1);
    query(5, cx, cy, w, 0, -1, 0); /* no seeds */
    for (int i = 0; i < 40; ++i) {
        for (unsigned j = 0; j < sizeof(path_walkable); ++j)
            path_walkable[j] = (u8)((random_word() >> 24) | (random_word() >> 24));
        const int x = (int)(random_word() % (unsigned)(w * 16));
        const int y = (int)(random_word() % (unsigned)(h * 16));
        const int origin = (int)(random_word() % (unsigned)(w * h));
        query(10 + i, x, y, origin, i & 1 ? edge : 0, -1, 0);
    }
#if defined(TEST_FACILITY_OVERFLOW)
    assert((fallbacks > before) == (w == 64 || w == 96 || w == 128));
#else
    assert(fallbacks == before);
#endif
}

int main(void)
{
    run_map(64, 64);
    run_map(96, 73);
    run_map(128, 96);
    run_map(128, 128);
    run_map(80, 48);
    puts("facility FIFO matched packed bytes, validity, cache, grid, and diagnostics");
    return 0;
}
