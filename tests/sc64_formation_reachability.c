/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independent graph oracle for the private Zerg group-route predicate.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_RACES) || !defined(SC64_IMPORTED_MISSION) || \
    defined(SC64_ROUTE_PARTIAL_FLOOD) || defined(SC64_ROUTE_VISIT_GRID)
#error "Compile the complete imported-race route configuration"
#endif

static unsigned rng_state = 0x8f31a6c5u;
static unsigned random_u32(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static int pixel_cell(int x, int y)
{
    int col = limit(x / 16, 0, MAP_COLS - 1);
    int row = limit(y / 16, 0, MAP_ROWS - 1);
    return col + row * MAP_COLS;
}

/* Independent flood implementing path_next_tile's four-neighbour graph and
 * its eight-neighbour blocked-target seeds/cardinal blocked-start exits. */
static int graph_oracle(int sx, int sy, int tx, int ty)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS];
    static const s8 dx8[8] = {0,1,1,1,0,-1,-1,-1};
    static const s8 dy8[8] = {-1,-1,0,1,1,1,0,-1};
    static const s8 dx4[4] = {1,-1,0,0};
    static const s8 dy4[4] = {0,0,1,-1};
    int sc = pixel_cell(sx, sy), gc = pixel_cell(tx, ty);
    int scol = sc % MAP_COLS, srow = sc / MAP_COLS;
    int gcol = gc % MAP_COLS, grow = gc / MAP_COLS;
    unsigned head = 0, tail = 0;
    memset(seen, 0, sizeof(seen));
    if (SC64_GRID_GET(path_walkable, gc)) {
        seen[gc] = 1; queue[tail++] = (u16)gc;
    } else {
        for (int d = 0; d < 8; ++d) {
            int x = gcol + dx8[d], y = grow + dy8[d];
            int cell;
            if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS) continue;
            cell = x + y * MAP_COLS;
            if (SC64_GRID_GET(path_walkable, cell) && !seen[cell]) {
                seen[cell] = 1; queue[tail++] = (u16)cell;
            }
        }
    }
    while (head < tail) {
        int cell = queue[head++], x = cell % MAP_COLS, y = cell / MAP_COLS;
        for (int d = 0; d < 4; ++d) {
            int nx = x + dx4[d], ny = y + dy4[d], next;
            if (nx < 0 || ny < 0 || nx >= MAP_COLS || ny >= MAP_ROWS) continue;
            next = nx + ny * MAP_COLS;
            if (SC64_GRID_GET(path_walkable, next) && !seen[next]) {
                seen[next] = 1; queue[tail++] = (u16)next;
            }
        }
    }
    if (SC64_GRID_GET(path_walkable, sc)) return seen[sc] != 0;
    for (int d = 0; d < 4; ++d) {
        int x = scol + dx4[d], y = srow + dy4[d];
        if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS) continue;
        if (seen[x + y * MAP_COLS]) return 1;
    }
    return 0;
}

static void clear_graph(void)
{
    SC64_GRID_CLEAR(path_walkable);
    path_grid_valid = 1;
    sc64_routes_reset();
}

static void check_pair(int sx, int sy, int tx, int ty)
{
    int nx, ny;
    int expected = graph_oracle(sx, sy, tx, ty);
    int tx2 = tx + 32, ty2 = ty - 16;
    int expected2 = graph_oracle(sx, sy, tx2, ty2);
    int old_route = path_next_tile(sx, sy, tx, ty, &nx, &ny);
    int fast_route = sc64_route_reachable(sx, sy, tx, ty);
    int pair = sc64_route_reachable_pair(sx, sy, tx, ty, tx2, ty2);
    int hint_slots[4] = {-1, -1, -1, -1};
    int hint_roots[4] = {-1, -1, -1, -1};
    int hint_next = 0;
    int use_cursor = sc64_route_formation_fallback_with_hint(
        sx, sy, tx, ty, tx2, ty2, hint_slots, hint_roots, &hint_next);
    assert(fast_route >= 0);
    assert(pair >= 0);
    assert(old_route == expected);
    assert(fast_route == expected);
    assert(((pair & 1) != 0) == expected);
    assert(((pair & 2) != 0) == expected2);
    assert(use_cursor == (!expected && expected2));
}

static void check_blocked_start_goal_and_diagonal_seed(void)
{
    clear_graph();
    /* A blocked goal seeds diagonally adjacent open cells. */
    SC64_GRID_SET(path_walkable, 50 + 50 * MAP_COLS, 1);
    check_pair(50 * 16 + 8, 50 * 16 + 8,
               50 * 16 + 8, 51 * 16 + 8);
    /* A blocked start exits cardinally into the diagonal target seed. */
    clear_graph();
    SC64_GRID_SET(path_walkable, 41 + 40 * MAP_COLS, 1);
    check_pair(40 * 16 + 8, 40 * 16 + 8,
               42 * 16 + 8, 41 * 16 + 8);
    /* A blocked start with no cardinal exit cannot use a diagonal touch. */
    clear_graph();
    SC64_GRID_SET(path_walkable, 41 + 41 * MAP_COLS, 1);
    check_pair(40 * 16 + 8, 40 * 16 + 8,
               42 * 16 + 8, 42 * 16 + 8);
    /* Negative and oversized pixels use the same truncating division/clamp. */
    clear_graph();
    SC64_GRID_SET(path_walkable, 0, 1);
    SC64_GRID_SET(path_walkable, MAP_COLS - 1 + (MAP_ROWS - 1) * MAP_COLS, 1);
    check_pair(-1, -1, -16, -16);
    check_pair(WORLD_W + 64, WORLD_H + 32, WORLD_W + 300, WORLD_H + 300);
}

static void check_random_graphs_against_oracle(void)
{
    for (unsigned graph = 0; graph < 32; ++graph) {
        clear_graph();
        for (int y = 42; y < 70; ++y)
            for (int x = 45; x < 73; ++x)
                if ((random_u32() & 7u) < 5u)
                    SC64_GRID_SET(path_walkable, x + y * MAP_COLS, 1);
        for (unsigned q = 0; q < 16; ++q) {
            int sx, sy, tx, ty;
            if (q == 0) { sx = -33; sy = 60 * 16 + 8; tx = 9000; ty = 60 * 16 + 8; }
            else {
                sx = (45 + (int)(random_u32() % 28u)) * 16 + 8;
                sy = (42 + (int)(random_u32() % 28u)) * 16 + 8;
                tx = (45 + (int)(random_u32() % 28u)) * 16 + 8;
                ty = (42 + (int)(random_u32() % 28u)) * 16 + 8;
            }
            check_pair(sx, sy, tx, ty);
        }
    }
}

static void check_topology_invalidation(void)
{
    clear_graph();
    SC64_GRID_SET(path_walkable, 61 + 60 * MAP_COLS, 1);
    check_pair(61 * 16 + 8, 60 * 16 + 8, 61 * 16 + 8, 60 * 16 + 8);
    int populated = 0;
    for (int i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i) populated += sc64_route_valid[i] != 0;
    assert(populated != 0);
    invalidate_path_grid();
    for (int i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i) assert(sc64_route_valid[i] == 0);
    clear_graph();
    SC64_GRID_SET(path_walkable, 63 + 60 * MAP_COLS, 1);
    check_pair(61 * 16 + 8, 60 * 16 + 8, 63 * 16 + 8, 60 * 16 + 8);
}

static void island_cell(unsigned i, int shifted, int *col, int *row)
{
    *col = 5 + (int)(i % 6u) * 12 + shifted;
    *row = 5 + (int)(i / 6u) * 12;
}

static void build_island_graph(unsigned count, int shifted)
{
    clear_graph();
    for (unsigned i = 0; i < count; ++i) {
        int col, row;
        island_cell(i, shifted, &col, &row);
        assert(col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS);
        SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 1);
    }
    /* The fixture authors the walk grid directly; invalidate cached route
     * fields while retaining the caller's stack-local hints. */
    invalidate_path_grid();
    path_grid_valid = 1;
}

static void check_retained_hints_across_eviction_and_invalidation(void)
{
    int hint_slots[4] = {-1, -1, -1, -1};
    int hint_roots[4] = {-1, -1, -1, -1};
    int hint_next = 0;
    unsigned active = (unsigned)SC64_ROUTE_ACTIVE_SLOTS;
    unsigned count = active + 8u;
    unsigned order[64];
    u32 builds_before = sc64_route_builds;
    assert(active > 0 && count <= sizeof(order) / sizeof(order[0]));
    build_island_graph(count, 0);

    /* Shuffle a set larger than the complete-field cache, then repeat random
     * components. Keep the same four hints live across every query. */
    for (unsigned i = 0; i < count; ++i) order[i] = i;
    for (unsigned i = count - 1; i > 0; --i) {
        unsigned j = random_u32() % (i + 1u), t = order[i];
        order[i] = order[j]; order[j] = t;
    }
    for (unsigned q = 0; q < count + 32u; ++q) {
        unsigned i = q < count ? order[q] : random_u32() % count;
        unsigned next = (i + 1u) % count;
        int sc, sr, nc, nr, slot_col, slot_row, tx, ty, sx, sy;
        int slot_reachable, cursor_reachable, use_cursor;
        island_cell(i, 0, &sc, &sr);
        island_cell(next, 0, &nc, &nr);
        sx = sc * 16 + 8; sy = sr * 16 + 8;
        if (q % 3u == 0) { slot_col = sc; slot_row = sr; tx = nc; ty = nr; }
        else if (q % 3u == 1) { slot_col = nc; slot_row = nr; tx = sc; ty = sr; }
        else { slot_col = nc; slot_row = nr; tx = nc; ty = nr; }
        slot_reachable = graph_oracle(sx, sy, slot_col * 16 + 8,
                                      slot_row * 16 + 8);
        cursor_reachable = graph_oracle(sx, sy, tx * 16 + 8, ty * 16 + 8);
        use_cursor = sc64_route_formation_fallback_with_hint(
            sx, sy, slot_col * 16 + 8, slot_row * 16 + 8,
            tx * 16 + 8, ty * 16 + 8,
            hint_slots, hint_roots, &hint_next);
        assert(use_cursor == (!slot_reachable && cursor_reachable));
    }
    assert(sc64_route_builds - builds_before > active);
    assert(hint_next > 0);
    {
        int retained = 0;
        for (int i = 0; i < 4; ++i) retained |= hint_slots[i] >= 0;
        assert(retained);
    }

    /* Move all islands by one tile, invalidate route topology but keep the
     * stale slot/root hints, then compare the new decision to the oracle. */
    build_island_graph(count, 1);
    for (unsigned q = 0; q < count; ++q) {
        unsigned i = (q * 7u) % count;
        unsigned next = (i + 1u) % count;
        int sc, sr, nc, nr, sx, sy, tx, ty, expected_slot, expected_cursor;
        int use_cursor;
        island_cell(i, 1, &sc, &sr);
        island_cell(next, 1, &nc, &nr);
        sx = sc * 16 + 8; sy = sr * 16 + 8;
        tx = (q & 1u) ? nc : sc; ty = (q & 1u) ? nr : sr;
        expected_slot = graph_oracle(sx, sy, nc * 16 + 8, nr * 16 + 8);
        expected_cursor = graph_oracle(sx, sy, tx * 16 + 8, ty * 16 + 8);
        use_cursor = sc64_route_formation_fallback_with_hint(
            sx, sy, nc * 16 + 8, nr * 16 + 8,
            tx * 16 + 8, ty * 16 + 8,
            hint_slots, hint_roots, &hint_next);
        assert(use_cursor == (!expected_slot && expected_cursor));
    }
}

int main(void)
{
    game_init();
    race_mode = 0; race_human = R_ZERG; race_selected_mission = SC64_Z7_GLOBAL_ID;
    race_cols = SC64_Z7_MAP_COLS; race_rows = SC64_Z7_MAP_ROWS;
    init_state();
    check_blocked_start_goal_and_diagonal_seed();
    check_random_graphs_against_oracle();
    check_topology_invalidation();
    check_retained_hints_across_eviction_and_invalidation();
    puts("formation reachability matches path and graph oracles");
    return 0;
}
