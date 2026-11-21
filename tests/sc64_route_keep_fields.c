/* Asset-free randomized differential test for SC64_ROUTE_KEEP_FIELDS.
 *
 * A route field survives a walkable-grid change as a "tainted" field and may
 * answer a next-step query only after the gate proves the stored direction at
 * that cell is the one a fresh flood would store. Every query below goes
 * through sc64_route_next and is compared with an independent tie-ordered
 * reverse BFS computed from scratch on the current grid. The same fixed
 * scenario sequence is also run by a build without the macro; the build counts
 * are compared so the test cannot pass without actually skipping floods.
 *
 *   prog                 run the suite (checks the answers)
 *   prog --emit FILE     also write the per-kind field build counts
 *   prog --compare FILE  also require fewer builds than the counts in FILE
 */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
#define GBA_HOT
#define SC64_RACES

static int race_cols = 64, race_rows = 64;
#define MAP_COLS race_cols
#define MAP_ROWS race_rows
enum { MAP_CELLS = 128 * 128 };

#ifdef TEST_PACKED_GRID
static u8 path_walkable[(MAP_CELLS + 7) / 8];
#define SC64_GRID_GET(grid, cell) (((grid)[(cell) >> 3] >> ((cell) & 7u)) & 1u)
#define SC64_GRID_SET(grid, cell, value) do { \
    u8 mask_ = (u8)(1u << ((cell) & 7u)); \
    if (value) (grid)[(cell) >> 3] |= mask_; \
    else (grid)[(cell) >> 3] &= (u8)~mask_; \
} while (0)
#else
static u8 path_walkable[MAP_CELLS];
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_GRID_SET(grid, cell, value) ((grid)[(cell)] = (u8)((value) != 0))
#endif

static u32 frame;
static u16 bfs_queue[MAP_CELLS];
static int path_grid_valid = 1;
static int limit(int n, int lo, int hi) { return n < lo ? lo : (n > hi ? hi : n); }
static int sc64_cell_row(unsigned cell) { return (int)(cell / (unsigned)MAP_COLS); }
static int sc64_cell_col(unsigned cell) { return (int)(cell % (unsigned)MAP_COLS); }

/* Outcome counters filled by the validation code in the header. */
enum {
    KEEP_GATE_CALL, KEEP_GATE_HIT, KEEP_FAIL_BLOCKED, KEEP_FAIL_CODE,
    KEEP_FAIL_LENGTH, KEEP_FAIL_SEEDS, KEEP_FAIL_NEAR_GOAL, KEEP_FAIL_OPENED,
    KEEP_COMMIT_SAME, KEEP_COMMIT_OVERFLOW, KEEP_COMMIT_DROP, KEEP_MATERIALIZE,
    KEEP_FILL, KEEP_ADOPT, KEEP_POINT_POINTER, KEEP_STATS
};
static unsigned keep_stat[KEEP_STATS];
#define SC64_ROUTE_KEEP_COUNT(name) (++keep_stat[KEEP_##name])

#ifdef SC64_ROUTE_KEEP_FIELDS
static void sc64_routes_topology_changed(void);
static void sc64_routes_grid_begin(void);
static void sc64_routes_grid_commit(void);
#endif
static u8 desired[MAP_CELLS]; /* what the game's rebuild would produce */
static void build_path_grid(void)
{
    unsigned cells = (unsigned)MAP_COLS * (unsigned)MAP_ROWS, i;
#ifdef SC64_ROUTE_KEEP_FIELDS
    sc64_routes_grid_begin();
#endif
    memset(path_walkable, 0, sizeof(path_walkable));
    for (i = 0; i < cells; ++i) SC64_GRID_SET(path_walkable, i, desired[i]);
    path_grid_valid = 1;
#ifdef SC64_ROUTE_KEEP_FIELDS
    sc64_routes_grid_commit();
#endif
}

#include "../starcraft/sc64_routes.h"

/* The game's invalidate_path_grid. */
static void invalidate(void)
{
    path_grid_valid = 0;
#ifdef SC64_ROUTE_KEEP_FIELDS
    sc64_routes_topology_changed();
#else
    sc64_routes_reset();
#endif
}

static u32 rng_state = 0x6d2b79f5u;
static u32 rnd(void)
{
    rng_state ^= rng_state << 13; rng_state ^= rng_state >> 17; rng_state ^= rng_state << 5;
    return rng_state;
}
static int rn(int n) { return (int)(rnd() % (u32)n); }
/* The white-box sampler must not disturb the scenario sequence, which has to
 * be identical in the build without the macro. */
static u32 aux_state = 0x2545f491u;
static int aux_rn(int n)
{
    aux_state ^= aux_state << 13; aux_state ^= aux_state >> 17; aux_state ^= aux_state << 5;
    return (int)(aux_state % (u32)n);
}
static int chance(int percent) { return rn(100) < percent; }

/* ---- oracle: independent reverse BFS, expanded left, right, up, down ---- */
enum { POOL = 40 };
typedef struct { int goal, version; short dist[MAP_CELLS]; u8 dir[MAP_CELLS]; } Oracle;
static Oracle oracle[POOL + 1];
static int grid_version = 1;

static void reference_bfs(Oracle *o, int goal)
{
    static int queue[MAP_CELLS];
    int head = 0, tail = 0, cells = MAP_COLS * MAP_ROWS;
    for (int i = 0; i < cells; ++i) { o->dist[i] = -1; o->dir[i] = SC64_ROUTE_UNREACHABLE; }
    o->goal = goal; o->version = grid_version;
    if (desired[goal]) {
        o->dist[goal] = 0; o->dir[goal] = 0; queue[tail++] = goal;
    } else {
        int gx = goal % MAP_COLS, gy = goal / MAP_COLS;
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            int x = gx + dx, y = gy + dy, cell;
            if ((!dx && !dy) || x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS) continue;
            cell = x + y * MAP_COLS;
            if (desired[cell] && o->dist[cell] < 0) {
                o->dist[cell] = 0; o->dir[cell] = 0; queue[tail++] = cell;
            }
        }
    }
    while (head < tail) {
        int cell = queue[head++], x = cell % MAP_COLS, y = cell / MAP_COLS;
        int nb[4] = { x ? cell - 1 : -1, x + 1 < MAP_COLS ? cell + 1 : -1,
                      y ? cell - MAP_COLS : -1, y + 1 < MAP_ROWS ? cell + MAP_COLS : -1 };
        for (int d = 1; d <= 4; ++d) {
            int next = nb[d - 1];
            if (next < 0 || !desired[next] || o->dist[next] >= 0) continue;
            o->dist[next] = (short)(o->dist[cell] + 1);
            o->dir[next] = (u8)d;
            queue[tail++] = next;
        }
    }
}

static const Oracle *oracle_for(int goal)
{
    for (int i = 0; i < POOL; ++i)
        if (oracle[i].goal == goal && oracle[i].version == grid_version) return &oracle[i];
    {
        static int victim;
        Oracle *o = &oracle[victim++ % POOL];
        reference_bfs(o, goal);
        return o;
    }
}

static int expected_next(const Oracle *o, int sx, int sy, int goal, int *next_cell, int *is_goal)
{
    int start = sx + sy * MAP_COLS;
    *is_goal = 0;
    if (desired[start]) {
        if (o->dist[start] < 0) return 0;
        if (start == goal || o->dir[start] == 0) { *next_cell = goal; *is_goal = 1; return 1; }
        switch (o->dir[start]) {
        case 1: *next_cell = start + 1; break;
        case 2: *next_cell = start - 1; break;
        case 3: *next_cell = start + MAP_COLS; break;
        default: *next_cell = start - MAP_COLS; break;
        }
        return 1;
    } else {
        static const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
        int best = INT_MAX, found = 0;
        for (int i = 0; i < 4; ++i) {
            int x = sx + dx[i], y = sy + dy[i], cell;
            if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS) continue;
            cell = x + y * MAP_COLS;
            if (o->dist[cell] < 0) continue;
            if (o->dist[cell] < best) { best = o->dist[cell]; *next_cell = cell; found = 1; }
        }
        return found;
    }
}

static unsigned long validated, gate_checked;

static void check_grid_unchanged(void)
{
    unsigned cells = (unsigned)MAP_COLS * (unsigned)MAP_ROWS;
    for (unsigned i = 0; i < cells; ++i) assert((SC64_GRID_GET(path_walkable, i) != 0) == (desired[i] != 0));
}

/* Everything the game can observe about cache residency: which fields exist,
 * their goals and ages, the clock and the counters. The build without the
 * macro records the same stream; the two must match query for query. */
static u32 *state_hashes;
static unsigned long state_hash_count, state_hash_capacity;
static u32 state_hash(void)
{
    u32 h = 2166136261u;
#define MIX(v) (h = (h ^ (u32)(v)) * 16777619u)
    for (int i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i) {
        MIX(sc64_route_valid[i]);
        if (sc64_route_valid[i]) { MIX(sc64_route_goal[i]); MIX(sc64_route_age[i]); }
    }
    MIX(sc64_route_clock); MIX(sc64_route_hits); MIX(sc64_route_builds);
#undef MIX
    return h;
}
static void record_state(void)
{
    {
        static long dump_at = -2;
        if (dump_at == -2) { const char *e = getenv("DUMP_AT"); dump_at = e ? atol(e) : -1; }
        if ((long)state_hash_count >= dump_at - 1 && (long)state_hash_count <= dump_at + 1 && dump_at >= 0) {
            printf("state %lu clock %u hits %u builds %u:", state_hash_count, sc64_route_clock, sc64_route_hits, sc64_route_builds);
            for (int i = 0; i < SC64_ROUTE_ACTIVE_SLOTS; ++i)
                printf(" [%d v%d g%d a%x]", i, sc64_route_valid[i], sc64_route_goal[i], sc64_route_age[i]);
            printf("\n");
        }
    }
    if (state_hash_count == state_hash_capacity) {
        state_hash_capacity = state_hash_capacity ? state_hash_capacity * 2 : 1u << 16;
        state_hashes = realloc(state_hashes, state_hash_capacity * sizeof(u32));
        assert(state_hashes);
    }
    state_hashes[state_hash_count++] = state_hash();
}

static void query(int sx, int sy, int goal)
{
    const Oracle *o = oracle_for(goal);
    int expected = -1, is_goal, nx = -1234, ny = -2345;
    int gx = goal % MAP_COLS, gy = goal / MAP_COLS;
    int found = expected_next(o, sx, sy, goal, &expected, &is_goal);
    int actual = sc64_route_next(sx * 16 + 8, sy * 16 + 8, gx * 16 + 8, gy * 16 + 8, &nx, &ny);
    ++validated;
    record_state();
    if (actual != found) {
        printf("MISMATCH found: shape %dx%d origin (%d,%d) goal (%d,%d) expected %d got %d\n",
               MAP_COLS, MAP_ROWS, sx, sy, gx, gy, found, actual);
        abort();
    }
    if (!found) return;
    if (is_goal) {
        assert(nx == gx * 16 + 8 && ny == gy * 16 + 8);
    } else if (nx / 16 + (ny / 16) * MAP_COLS != expected) {
        printf("MISMATCH step: shape %dx%d origin (%d,%d) goal (%d,%d) walkable=%d expected cell %d got %d\n",
               MAP_COLS, MAP_ROWS, sx, sy, gx, gy, desired[sx + sy * MAP_COLS],
               expected, nx / 16 + (ny / 16) * MAP_COLS);
        abort();
    }
}

#ifdef SC64_REACHABLE_FORMATION
static int comp_of[MAP_CELLS], comp_version;
static void label_components(void)
{
    static int queue[MAP_CELLS];
    int cells = MAP_COLS * MAP_ROWS, next = 0;
    if (comp_version == grid_version) return;
    comp_version = grid_version;
    for (int i = 0; i < cells; ++i) comp_of[i] = -1;
    for (int seed = 0; seed < cells; ++seed) {
        int head = 0, tail = 0;
        if (!desired[seed] || comp_of[seed] >= 0) continue;
        comp_of[seed] = next; queue[tail++] = seed;
        while (head < tail) {
            int cell = queue[head++], x = cell % MAP_COLS, y = cell / MAP_COLS;
            int nb[4] = { x ? cell - 1 : -1, x + 1 < MAP_COLS ? cell + 1 : -1,
                          y ? cell - MAP_COLS : -1, y + 1 < MAP_ROWS ? cell + MAP_COLS : -1 };
            for (int d = 0; d < 4; ++d)
                if (nb[d] >= 0 && desired[nb[d]] && comp_of[nb[d]] < 0) { comp_of[nb[d]] = next; queue[tail++] = nb[d]; }
        }
        ++next;
    }
}

/* Mirrors the public reachability pair: starts are the cell or its open
 * cardinal neighbours, goals the cell or its open eight neighbours. */
static void reach_query(int sx, int sy, int g1x, int g1y, int g2x, int g2y)
{
    static const int sdx[4] = { 1, -1, 0, 0 }, sdy[4] = { 0, 0, 1, -1 };
    static const int gdx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 }, gdy[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
    int starts[4], goals[2][8], sn = 0, gn[2] = { 0, 0 }, expected = 0;
    int gc[2][2] = { { g1x, g1y }, { g2x, g2y } };
    int actual;
    label_components();
    if (desired[sx + sy * MAP_COLS]) starts[sn++] = sx + sy * MAP_COLS;
    else for (int i = 0; i < 4; ++i) {
        int x = sx + sdx[i], y = sy + sdy[i];
        if (x >= 0 && y >= 0 && x < MAP_COLS && y < MAP_ROWS && desired[x + y * MAP_COLS]) starts[sn++] = x + y * MAP_COLS;
    }
    for (int g = 0; g < 2; ++g) {
        int cell = gc[g][0] + gc[g][1] * MAP_COLS;
        if (desired[cell]) goals[g][gn[g]++] = cell;
        else for (int i = 0; i < 8; ++i) {
            int x = gc[g][0] + gdx[i], y = gc[g][1] + gdy[i];
            if (x >= 0 && y >= 0 && x < MAP_COLS && y < MAP_ROWS && desired[x + y * MAP_COLS])
                goals[g][gn[g]++] = x + y * MAP_COLS;
        }
    }
    if (sn && (gn[0] || gn[1]))
        for (int s = 0; s < sn; ++s) for (int g = 0; g < 2; ++g) for (int j = 0; j < gn[g]; ++j)
            if (comp_of[starts[s]] == comp_of[goals[g][j]]) expected |= 1 << g;
    actual = sc64_route_reachable_pair(sx * 16 + 8, sy * 16 + 8, g1x * 16 + 8, g1y * 16 + 8,
                                       g2x * 16 + 8, g2y * 16 + 8);
    ++validated;
    record_state();
    if (actual != expected) {
        printf("MISMATCH reach: shape %dx%d start (%d,%d) goals (%d,%d) (%d,%d) expected %d got %d\n",
               MAP_COLS, MAP_ROWS, sx, sy, g1x, g1y, g2x, g2y, expected, actual);
        abort();
    }
}
#endif

#ifdef SC64_ROUTE_ACTIVE_STRIDE
static int phys_get(int id, int cell)
{
    int slot = 0, code, ol = sc64_route_lphys[0];
    u8 *of = sc64_route_field[0];
    sc64_route_lphys[0] = (u8)id; sc64_route_field[0] = sc64_route_phys_base(id);
    code = sc64_route_get(slot, cell);
    sc64_route_lphys[0] = (u8)ol; sc64_route_field[0] = of;
    return code;
}

/* A donor flooded on the current grid must hold exactly what a flood stores. */
static unsigned long donors_checked;
static void check_donors(void)
{
    for (int id = 0; id < sc64_route_pcount; ++id) {
        if ((sc64_route_pf_flags[id] & (SC64_ROUTE_FLAG_DONOR | SC64_ROUTE_FLAG_STALE)) != SC64_ROUTE_FLAG_DONOR)
            continue;
        {
            const Oracle *o = oracle_for(sc64_route_pf_goal[id]);
            for (int cell = 0; cell < MAP_COLS * MAP_ROWS; ++cell) {
                int expected = o->dist[cell] < 0 ? SC64_ROUTE_UNREACHABLE : o->dir[cell];
                if (phys_get(id, cell) != expected) {
                    printf("FRESH DONOR WRONG: shape %dx%d phys %d goal %d cell %d\n", MAP_COLS,
                           MAP_ROWS, id, sc64_route_pf_goal[id], cell);
                    abort();
                }
            }
            ++donors_checked;
        }
    }
}
#else
static void check_donors(void) {}
#endif

#ifdef SC64_ROUTE_KEEP_FIELDS
/* White box: whenever the gate accepts an origin for a lazy slot or a stale
 * donor, the stored code must be the code a fresh flood stores there. Tried
 * for sampled origins plus every cell near the latest change. */
static void gate_sweep(int slot, int goal, const int *changed, int changed_count)
{
    const Oracle *o = oracle_for(goal);
    int cells = MAP_COLS * MAP_ROWS;
    for (int n = 0; n < 500 + changed_count * 40; ++n) {
        int cell;
        if (n < 500) cell = aux_rn(cells);
        else {
            int c = changed[(n - 500) / 40];
            int x = c % MAP_COLS + aux_rn(11) - 5, y = c / MAP_COLS + aux_rn(11) - 5;
            if (x < 0 || y < 0 || x >= MAP_COLS || y >= MAP_ROWS) continue;
            cell = x + y * MAP_COLS;
        }
        if (!desired[cell]) continue;
        if (sc64_route_gate(slot, cell, goal)) {
            ++gate_checked;
            if (o->dist[cell] < 0 || sc64_route_get(slot, cell) != o->dir[cell]) {
                printf("GATE ACCEPTED A STALE CODE: shape %dx%d cell (%d,%d) goal (%d,%d) "
                       "stored %d fresh %d (dist %d)\n", MAP_COLS, MAP_ROWS,
                       cell % MAP_COLS, cell / MAP_COLS, goal % MAP_COLS, goal / MAP_COLS,
                       sc64_route_get(slot, cell), o->dir[cell], o->dist[cell]);
                abort();
            }
        }
    }
}

static void check_gate_codes(const int *changed, int changed_count)
{
    for (int slot = 0; slot < SC64_ROUTE_SLOTS; ++slot)
        if (sc64_route_valid[slot] == 1 && SC64_ROUTE_LAZY(slot))
            gate_sweep(slot, sc64_route_goal[slot], changed, changed_count);
    for (int id = 0; id < sc64_route_pcount; ++id)
        if ((sc64_route_pf_flags[id] & (SC64_ROUTE_FLAG_DONOR | SC64_ROUTE_FLAG_STALE)) ==
            (SC64_ROUTE_FLAG_DONOR | SC64_ROUTE_FLAG_STALE)) {
            /* borrow logical slot 0's view for the physical field */
            int ol = sc64_route_lphys[0];
            u8 *of = sc64_route_field[0];
            sc64_route_lphys[0] = (u8)id; sc64_route_field[0] = sc64_route_phys_base(id);
            gate_sweep(0, sc64_route_pf_goal[id], changed, changed_count);
            sc64_route_lphys[0] = (u8)ol; sc64_route_field[0] = of;
        }
}
#else
static void check_gate_codes(const int *changed, int changed_count) { (void)changed; (void)changed_count; }
#endif

/* Every resident field that is not lazy must hold exactly what a flood on the
 * current grid stores. */
static unsigned long exact_fields_checked;
static void check_resident_fields(void)
{
    for (int slot = 0; slot < SC64_ROUTE_ACTIVE_SLOTS; ++slot) {
        if (sc64_route_valid[slot] != 1) continue;
        if (SC64_ROUTE_LAZY(slot)) continue;
        {
            int goal = sc64_route_goal[slot];
            const Oracle *o = oracle_for(goal);
#ifdef SC64_ROUTE_POINT_PREFIX_PROBE
            int parked = sc64_point_prefix_find(slot) != 0;
#endif
            for (int cell = 0; cell < MAP_COLS * MAP_ROWS; ++cell) {
                int expected = o->dist[cell] < 0 ? SC64_ROUTE_UNREACHABLE : o->dir[cell];
#ifdef SC64_ROUTE_POINT_PREFIX_PROBE
                if (parked && sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE) continue;
#endif
                if (sc64_route_get(slot, cell) != expected) {
                    printf("RESIDENT FIELD WRONG: shape %dx%d slot %d goal %d cell %d stored %d fresh %d\n",
                           MAP_COLS, MAP_ROWS, slot, goal, cell, sc64_route_get(slot, cell), expected);
                    abort();
                }
            }
            ++exact_fields_checked;
        }
    }
}

/* ---- terrain ---- */
enum { T_SPARSE, T_MEDIUM, T_DENSE, T_MAZE, T_ROOMS, T_KINDS };

static void make_maze(int loops)
{
    static int stack[MAP_CELLS];
    int sp = 0, cols = MAP_COLS, rows = MAP_ROWS;
    memset(desired, 0, sizeof(desired));
    desired[1 + cols] = 1; stack[sp++] = 1 + cols;
    while (sp) {
        int cell = stack[sp - 1], x = cell % cols, y = cell / cols, opts[4], n = 0;
        if (x + 2 < cols - 1 && !desired[cell + 2]) opts[n++] = 0;
        if (x - 2 > 0 && !desired[cell - 2]) opts[n++] = 1;
        if (y + 2 < rows - 1 && !desired[cell + 2 * cols]) opts[n++] = 2;
        if (y - 2 > 0 && !desired[cell - 2 * cols]) opts[n++] = 3;
        if (!n) { --sp; continue; }
        switch (opts[rn(n)]) {
        case 0: desired[cell + 1] = desired[cell + 2] = 1; stack[sp++] = cell + 2; break;
        case 1: desired[cell - 1] = desired[cell - 2] = 1; stack[sp++] = cell - 2; break;
        case 2: desired[cell + cols] = desired[cell + 2 * cols] = 1; stack[sp++] = cell + 2 * cols; break;
        default: desired[cell - cols] = desired[cell - 2 * cols] = 1; stack[sp++] = cell - 2 * cols; break;
        }
    }
    for (int y = 1; y < rows - 1; ++y) for (int x = 1; x < cols - 1; ++x)
        if (!desired[x + y * cols] && chance(loops)) desired[x + y * cols] = 1;
}

static void make_terrain(int kind)
{
    int cells = MAP_COLS * MAP_ROWS;
    switch (kind) {
    case T_SPARSE: for (int i = 0; i < cells; ++i) desired[i] = !chance(8); break;
    case T_MEDIUM: for (int i = 0; i < cells; ++i) desired[i] = !chance(28); break;
    case T_DENSE: for (int i = 0; i < cells; ++i) desired[i] = !chance(41); break;
    case T_MAZE: make_maze(chance(50) ? 2 : 6); break;
    default: { /* rooms with doors */
        int pitch = 7 + rn(5);
        for (int y = 0; y < MAP_ROWS; ++y) for (int x = 0; x < MAP_COLS; ++x)
            desired[x + y * MAP_COLS] = (x % pitch != 0 && y % pitch != 0) || chance(5);
        break; }
    }
}

/* ---- scenario ---- */
enum { K_BLOCK, K_OPEN, K_MIXED, K_REVERT, K_BURST, K_CYCLE, K_KINDS };
static const char *const kind_name[K_KINDS] = { "block", "open", "mixed", "revert", "burst", "cycle" };
static int cycle_mode;
static unsigned cycle_counter;
static unsigned long kind_builds[K_KINDS];
/* Floods that really ran: every build without the macro, the fills with it. */
static unsigned long floods_now(void)
{
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    return keep_stat[KEEP_FILL];
#else
    return sc64_route_builds;
#endif
}

static int pool_goal[POOL];

static int random_cell_with(int walkable_wanted)
{
    int cells = MAP_COLS * MAP_ROWS;
    for (int tries = 0; tries < 200; ++tries) {
        int c = rn(cells);
        if (walkable_wanted < 0 || (desired[c] != 0) == walkable_wanted) return c;
    }
    return rn(cells);
}

static void pick_pool(void)
{
    for (int i = 0; i < POOL; ++i) {
        int cell;
        if (i < 24) cell = random_cell_with(1);
        else if (i < 34) cell = random_cell_with(0);
        else cell = chance(50) ? rn(MAP_COLS) : (MAP_ROWS - 1) * MAP_COLS + rn(MAP_COLS);
        pool_goal[i] = cell;
    }
}

typedef struct { int cell; u8 old; } Edit;
enum { MAX_EDITS = 16 };
static Edit edits[MAX_EDITS];
static int edit_count;
static int changed_cells[MAX_EDITS];

static int near_cell(int cell, int radius)
{
    int x = cell % MAP_COLS + rn(2 * radius + 1) - radius;
    int y = cell / MAP_COLS + rn(2 * radius + 1) - radius;
    return limit(x, 0, MAP_COLS - 1) + limit(y, 0, MAP_ROWS - 1) * MAP_COLS;
}

/* mode: K_BLOCK blocks walkable cells, K_OPEN opens blocked cells, else flips. */
static void make_change(int mode, int origin_hint)
{
    int count = 1 + rn(12), center, rect = chance(40);
    edit_count = 0;
    if (chance(45)) center = near_cell(pool_goal[rn(POOL)], 3);
    else if (chance(25)) center = near_cell(origin_hint, 6);
    else center = rn(MAP_COLS * MAP_ROWS);
    if (rect) { /* a building footprint */
        int w = 1 + rn(4), h = 1 + rn(3), x0 = center % MAP_COLS, y0 = center / MAP_COLS;
        for (int y = y0; y < y0 + h && edit_count < MAX_EDITS; ++y)
            for (int x = x0; x < x0 + w && edit_count < MAX_EDITS; ++x) {
                int c;
                if (x >= MAP_COLS || y >= MAP_ROWS) continue;
                c = x + y * MAP_COLS;
                if (mode == K_BLOCK && !desired[c]) continue;
                if (mode == K_OPEN && desired[c]) continue;
                edits[edit_count].cell = c; edits[edit_count++].old = desired[c];
            }
    } else {
        for (int n = 0; n < count * 3 && edit_count < count; ++n) {
            int c = near_cell(center, 1 + rn(4)), dup = 0;
            if (mode == K_BLOCK && !desired[c]) continue;
            if (mode == K_OPEN && desired[c]) continue;
            for (int j = 0; j < edit_count; ++j) dup |= edits[j].cell == c;
            if (dup) continue;
            edits[edit_count].cell = c; edits[edit_count++].old = desired[c];
        }
    }
    for (int i = 0; i < edit_count; ++i) {
        desired[edits[i].cell] = (u8)!edits[i].old;
        changed_cells[i] = edits[i].cell;
    }
    ++grid_version;
    invalidate();
}

static void revert_change(void)
{
    for (int i = 0; i < edit_count; ++i) desired[edits[i].cell] = edits[i].old;
    ++grid_version;
    invalidate();
}

static void run_queries(int count, int *last_origin)
{
    int cells = MAP_COLS * MAP_ROWS;
    for (int q = 0; q < count; ++q) {
        int goal = pool_goal[cycle_mode ? cycle_counter++ % POOL : (unsigned)rn(POOL)], origin;
        int sx, sy;
        int pick = rn(100);
        if (pick < 40) origin = random_cell_with(1);
        else if (pick < 55) origin = random_cell_with(-1);
        else if (pick < 80) origin = near_cell(goal, 8);
        else if (edit_count) origin = near_cell(edits[rn(edit_count)].cell, 4);
        else origin = rn(cells);
        sx = origin % MAP_COLS; sy = origin / MAP_COLS;
        query(sx, sy, goal);
#ifdef SC64_REACHABLE_FORMATION
        if (chance(15)) {
            int g1 = pool_goal[rn(POOL)], g2 = pool_goal[rn(POOL)];
            reach_query(sx, sy, g1 % MAP_COLS, g1 / MAP_COLS, g2 % MAP_COLS, g2 / MAP_COLS);
        }
#endif
        *last_origin = origin;
    }
}

static void scenario(int shape_cols, int shape_rows, int terrain, int kind, int steps)
{
    int origin_hint, builds_before;
    race_cols = shape_cols; race_rows = shape_rows;
    sc64_routes_reset();
    path_grid_valid = 0;
#ifdef TEST_POINT_TAIL_DONOR_FREE
    if (shape_cols == 128 && shape_rows == 128) sc64_route_pcount = 0;
#endif
    make_terrain(terrain);
    ++grid_version;
    pick_pool();
    origin_hint = random_cell_with(1);
    builds_before = (int)floods_now();
    run_queries(2 * POOL, &origin_hint); /* populate fields */
    cycle_mode = kind == K_CYCLE;
    for (int step = 0; step < steps; ++step) {
        int mode = kind == K_BLOCK ? K_BLOCK : kind == K_OPEN ? K_OPEN : K_MIXED;
        int changes = (kind == K_BURST) ? 1 + rn(5) : 1;
        if (kind == K_BLOCK && chance(10)) invalidate(), ++grid_version; /* no-op rebuild */
        if (kind == K_CYCLE) { /* recurring goals, more than the cache holds */
            if (step % 4 == 3) make_change(chance(50) ? K_BLOCK : K_MIXED, origin_hint);
            run_queries(80, &origin_hint);
            if (path_grid_valid) { check_donors(); check_gate_codes(changed_cells, edit_count); }
            if (step % 3 == 0 && path_grid_valid) check_resident_fields();
            continue;
        }
        for (int c = 0; c < changes; ++c) {
            if (kind == K_MIXED && chance(60)) mode = chance(50) ? K_BLOCK : K_OPEN;
            if (kind == K_BURST) mode = chance(50) ? K_OPEN : K_MIXED;
            make_change(mode, origin_hint);
            if (kind == K_BURST && chance(50)) run_queries(3, &origin_hint);
        }
        run_queries(40, &origin_hint);
        if (path_grid_valid) { check_donors(); check_gate_codes(changed_cells, edit_count); }
        if (kind == K_REVERT || chance(8)) {
            revert_change();
            run_queries(30, &origin_hint);
        }
        if (chance(10)) { /* replace a goal: slot reuse */
            pool_goal[rn(POOL)] = random_cell_with(chance(70) ? 1 : 0);
        }
        if (step % 8 == 0) check_grid_unchanged();
        if (step % 3 == 0 && path_grid_valid) check_resident_fields();
    }
    kind_builds[kind] += floods_now() - (unsigned long)builds_before;
}

#ifdef SC64_ROUTE_LINEAR_FLOOD_PROBE
static void check_linear_uses_expected_path(int cols)
{
    const int cells = cols * 128, goal = cols / 2 + 64 * cols;
    int filled;
    race_cols = cols;
    race_rows = 128;
    sc64_routes_reset();
    assert(sc64_route_configure());
    memset(desired, 1, (size_t)cells);
    build_path_grid();
    memset(sc64_route_field[0], 0x53, sc64_route_active_bytes);
    memset(SC64_ROUTE_OLD_GRID, 0x5a, sizeof(path_walkable));
    filled = sc64_route_fill_linear(sc64_route_field[0], goal);
    for (unsigned i = 0; i < sizeof(path_walkable); ++i) assert(SC64_ROUTE_OLD_GRID[i] == 0x5a);
#ifdef TEST_LINEAR_OVERFLOW
    assert(filled == -1);
    for (unsigned i = 0; i < sc64_route_active_bytes; ++i)
        assert(sc64_route_field[0][i] == 0x53);
    assert(sc64_route_fill(0, goal) == cells);
#else
    assert(filled == cells); /* must compact: the field is larger than the queue */
#endif
    reference_bfs(&oracle[POOL], goal);
    for (int i = 0; i < cells; ++i) assert(sc64_route_get(0, i) == oracle[POOL].dir[i]);
    check_grid_unchanged();
}
#endif

#ifdef SC64_ROUTE_POINT_POINTER_PROBE
static void check_single_retained_point(void)
{
    const int first = 64 * 128 + 32, count = 128 * 128;
    race_cols = race_rows = 128;
    memset(desired, 1, count);
    sc64_routes_reset();
    build_path_grid();
    assert(sc64_route_configure());
    /* This fixture has no range reservation. Reproduce the pinned stock
     * 128x128 point bank: fixed identity slots and no ordinary donor planes. */
    sc64_route_pcount = 0;
#if defined(SC64_ROUTE_POINT_TAIL_PROBE) && defined(TEST_LINEAR_OVERFLOW)
    int protected_slot = sc64_route_slot(first);
    int clobber_slot = protected_slot == 0 ? 1 : 0;
    assert(protected_slot >= 0 && sc64_route_valid[protected_slot]);
    sc64_route_point_spare = sc64_route_field[protected_slot];
    memcpy(sc64_route_point_plane, sc64_route_point_spare, SC64_ROUTE_POINT_BYTES);
    sc64_route_field[protected_slot] = sc64_route_point_plane;
    assert(sc64_route_fill(clobber_slot, first+1) == count);
    assert(sc64_route_point_spare == sc64_route_point_plane);
    assert(sc64_route_field[protected_slot] != sc64_route_point_plane);
    reference_bfs(&oracle[POOL], first);
    for (int i = 0; i < count; ++i)
        assert(sc64_route_get(protected_slot, i) == oracle[POOL].dir[i]);
    reference_bfs(&oracle[POOL], first+1);
    for (int i = 0; i < count; ++i)
        assert(sc64_route_get(clobber_slot, i) == oracle[POOL].dir[i]);
    check_grid_unchanged();
    puts("forced whole-queue fallback releases the borrowed tail before writing");
    return;
#endif
    for (int i = 0; i <= SC64_ROUTE_SLOTS; ++i)
        assert(sc64_route_slot(first+i) >= 0);
    assert(sc64_route_point_saved == first);
    unsigned old_fills = keep_stat[KEEP_FILL], old_hits = keep_stat[KEEP_POINT_POINTER];
    u32 old_logical = sc64_route_builds;
    u8 *saved = sc64_route_point_spare;
    int slot = sc64_route_slot(first);
    assert(sc64_route_field[slot] == saved && sc64_route_point_spare != saved);
    assert(slot >= 0 && sc64_route_goal[slot] == first && sc64_route_valid[slot] == 1);
    assert(sc64_route_point_saved == first+1);
    assert(keep_stat[KEEP_FILL] == old_fills && keep_stat[KEEP_POINT_POINTER] == old_hits+1);
    assert(sc64_route_builds == old_logical+1);
    reference_bfs(&oracle[POOL], first);
    for (int i = 0; i < count; ++i) assert(sc64_route_get(slot, i) == oracle[POOL].dir[i]);
    check_grid_unchanged();
#ifdef SC64_ROUTE_POINT_TAIL_PROBE
    /* Release while the scratch address is still logically occupied, then
     * clobber every borrowed byte as a save or full queue would. */
    int bound = -1;
    for (int i = 0; i < SC64_ROUTE_SLOTS; ++i)
        if (sc64_route_field[i] == sc64_route_point_plane) bound = i;
    assert(bound >= 0 && sc64_route_valid[bound]);
    int bound_goal = sc64_route_goal[bound];
    u8 *pool_address = sc64_route_point_spare;
    reference_bfs(&oracle[POOL], bound_goal);
    SC64_ROUTE_POINT_TAIL_RELEASE();
    memset(sc64_route_point_plane, 0x53, SC64_ROUTE_POINT_BYTES);
    assert(sc64_route_point_spare == sc64_route_point_plane && sc64_route_point_saved == -1);
    assert(sc64_route_field[bound] == pool_address && sc64_route_goal[bound] == bound_goal);
    for (int i = 0; i < count; ++i)
        assert(sc64_route_get(bound, i) == oracle[POOL].dir[i]);
#endif
    sc64_routes_topology_changed();
    assert(sc64_route_point_saved == -1);
    assert(sc64_route_slot(first) >= 0 && keep_stat[KEEP_FILL] == old_fills+1);
    sc64_routes_reset();
    assert(sc64_route_point_saved == -1);
    race_cols = race_rows = 64;
    sc64_routes_reset();
    assert(sc64_route_field[0] == sc64_route_pool &&
           sc64_route_point_spare == sc64_route_point_plane);
    race_cols = race_rows = 128;
    sc64_routes_reset();
    assert(sc64_route_field[SC64_ROUTE_SLOTS - 1] == sc64_route_pool +
           (SC64_ROUTE_SLOTS - 1u) * SC64_ROUTE_POINT_BYTES);
    puts("point pointer retention matches fresh BFS, logical miss and topology invalidation");
}
#endif

#ifdef SC64_ROUTE_POINT_PREFIX_PROBE
static void check_point_prefix_resume(void)
{
    const int first = 32 + 32 * 128, distant = 100 + 100 * 128;
    int slots[SC64_POINT_PREFIX_SLOTS + 1];
    race_cols = race_rows = 128;
    memset(desired, 1, sizeof(desired));
    sc64_routes_reset();
    sc64_route_pcount = 0;
    build_path_grid();
    u32 before = sc64_route_builds;

    for (int i = 0; i <= SC64_POINT_PREFIX_SLOTS; ++i) {
        int root = first + 10 * i;
        int origin = root + 1;
        int slot = sc64_route_slot_for(root, origin);
        assert(slot >= 0 && sc64_route_valid[slot] == 1 && sc64_route_goal[slot] == root);
        assert(sc64_route_get(slot, origin) == 2);
        slots[i] = slot;
        if (i < SC64_POINT_PREFIX_SLOTS) {
            assert(sc64_point_prefix_find(slot));
            assert(sc64_route_get(slot, distant) == SC64_ROUTE_UNREACHABLE);
        } else {
            assert(!sc64_point_prefix_find(slot));
            assert(sc64_route_get(slot, distant) != SC64_ROUTE_UNREACHABLE);
        }
    }
    assert(sc64_route_builds == before + SC64_POINT_PREFIX_SLOTS + 1);
    u32 metadata = state_hash();
    for (int i = 0; i < SC64_POINT_PREFIX_SLOTS; ++i) {
        assert(SC64_ROUTE_REACHES(slots[i], distant));
#ifdef SC64_ROUTE_POINT_COMPONENT_PROBE
        SC64_ROUTE_EXACT(slots[i]);
#endif
        assert(!sc64_point_prefix_find(slots[i]));
        reference_bfs(&oracle[POOL], first + 10 * i);
        for (int cell = 0; cell < MAP_CELLS; ++cell)
            assert(sc64_route_get(slots[i], cell) == oracle[POOL].dir[cell]);
    }
    assert(metadata == state_hash());
    check_grid_unchanged();

    int blocked_goal = first + 2 * MAP_COLS;
    desired[blocked_goal] = 0;
    invalidate();
    build_path_grid();
    int slot = sc64_route_slot_for(blocked_goal, blocked_goal + 1);
    assert(slot >= 0 && sc64_point_prefix_find(slot));
    assert(sc64_route_get(slot, blocked_goal + 1) == 0);
    assert(sc64_route_slot_for(blocked_goal, distant) == slot);
    assert(!sc64_point_prefix_find(slot));
    reference_bfs(&oracle[POOL], blocked_goal);
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        assert(sc64_route_get(slot, cell) == oracle[POOL].dir[cell]);
    check_grid_unchanged();

    slot = sc64_route_slot_for(first, first + 1);
    assert(slot >= 0 && sc64_point_prefix_find(slot));
    invalidate();
    for (unsigned i = 0; i < SC64_POINT_PREFIX_SLOTS; ++i)
        assert(!sc64_point_prefixes[i].count);
    puts("point prefixes preserve exact FIFO, capacity fallback, logical residency and topology reset");
}

#ifdef SC64_ROUTE_POINT_SECOND_EXACT_HIT
static void check_point_prefix_exact_hit(void)
{
    const int root = 40 + 40 * 128;
    const int origin = root + 12 * 128;
    const int distant = root + 30 * 128;
    race_cols = race_rows = 128;
    memset(desired, 1, sizeof(desired));
    sc64_routes_reset();
    sc64_route_pcount = 0;
    build_path_grid();

    int slot = sc64_route_slot_for(root, root + 1);
    assert(slot >= 0 && sc64_point_prefix_find(slot));
    assert(sc64_route_get(slot, origin) == SC64_ROUTE_UNREACHABLE);
    u32 builds = sc64_route_builds, hits = sc64_route_hits;
    assert(sc64_route_slot_for(root, origin) == slot);
    Sc64PointPrefix *prefix = sc64_point_prefix_find(slot);
    assert(prefix && prefix->second_cutoff && prefix->restore_last_row >= 53);
    assert(sc64_route_builds == builds && sc64_route_hits == hits + 1);
    reference_bfs(&oracle[POOL], root);
    for (int cell = 0; cell < MAP_CELLS; ++cell) {
        int direction = sc64_route_get(slot, cell);
        if (direction != SC64_ROUTE_UNREACHABLE)
            assert(direction == oracle[POOL].dir[cell]);
    }
    assert(sc64_route_get(slot, origin) == oracle[POOL].dir[origin]);
    assert(sc64_route_get(slot, distant) == SC64_ROUTE_UNREACHABLE);

    assert(sc64_route_slot_for(root, distant) == slot);
    assert(!sc64_point_prefix_find(slot));
    assert(sc64_route_builds == builds && sc64_route_hits == hits + 2);
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        assert(sc64_route_get(slot, cell) == oracle[POOL].dir[cell]);
    check_grid_unchanged();

    sc64_routes_reset();
    sc64_route_pcount = 0;
    build_path_grid();
    slot = sc64_route_slot_for(root, root + 1);
    assert(slot >= 0 && sc64_point_prefix_find(slot));
    assert(sc64_route_slot(root) == slot && !sc64_point_prefix_find(slot));
    puts("one open exact hit reparks; repeat miss and complete lookup retain the full route");
}
#endif

static void check_point_prefix_row_span(void)
{
    const int distant = 100 + 64 * 128;
    race_cols = race_rows = 128;
    sc64_routes_reset();
    sc64_route_pcount = 0;
    for (int row = 0; row < 128; row += 127) {
        int root = 32 + row * 128;
        for (int blocked = 0; blocked <= 1; ++blocked) {
            memset(desired, 1, sizeof(desired));
            desired[root] = !blocked;
            invalidate();
            build_path_grid();
            int slot = sc64_route_slot_for(root, root + 1);
            assert(slot >= 0);
            const Sc64PointPrefix *prefix = sc64_point_prefix_find(slot);
            assert(prefix);
            assert(prefix->restore_first_row == (row ? 127 : 1));
            assert(prefix->restore_last_row == (row ? 128 : 2));
            assert(sc64_route_get(slot, distant) == SC64_ROUTE_UNREACHABLE);
            u32 metadata = state_hash();
            SC64_ROUTE_EXACT(slot);
            assert(!sc64_point_prefix_find(slot) && state_hash() == metadata);
            reference_bfs(&oracle[POOL], root);
            for (int cell = 0; cell < MAP_CELLS; ++cell)
                assert(sc64_route_get(slot, cell) == oracle[POOL].dir[cell]);
            check_grid_unchanged();
        }
    }
    puts("point prefix row bounds preserve exact directions at both map edges");
}

#ifdef SC64_ROUTE_POINT_TAIL_PROBE
static void check_point_prefix_tail_exchange(void)
{
    const int first = 32 + 32 * 128;
    const int other = 64 + 64 * 128;
    race_cols = race_rows = 128;
    memset(desired, 1, sizeof(desired));
    sc64_routes_reset();
    sc64_route_pcount = 0;
    build_path_grid();
    int slot = sc64_route_slot(first);
    assert(slot >= 0);
    for (int i = 1; i < SC64_ROUTE_SLOTS; ++i)
        assert(sc64_route_slot(first + i) >= 0);

    /* Preserve a completed victim and park the incoming prefix in the
     * borrowed plane. A hit swaps the full field back into the slot. */
    sc64_route_age[slot] = 0;
    assert(sc64_route_slot_for(other, other + 1) == slot);
    assert(sc64_route_point_saved == first && sc64_point_prefix_find(slot));
    assert(sc64_route_field[slot] == sc64_route_point_plane);
    u8 *saved = sc64_route_point_spare;
    unsigned fills = keep_stat[KEEP_FILL], hits = keep_stat[KEEP_POINT_POINTER];
    sc64_route_age[slot] = 0;
    assert(sc64_route_slot(first) == slot);
    assert(!sc64_point_prefix_find(slot) && sc64_route_point_saved < 0);
    assert(sc64_route_field[slot] == saved && sc64_route_point_spare == sc64_route_point_plane);
    assert(keep_stat[KEEP_FILL] == fills && keep_stat[KEEP_POINT_POINTER] == hits + 1);
    reference_bfs(&oracle[POOL], first);
    for (int i = 0; i < MAP_CELLS; ++i)
        assert(sc64_route_get(slot, i) == oracle[POOL].dir[i]);

    /* A miss can evict a parked prefix and preserve the complete tail.
     * A full-queue user can then release the bound prefix's backing. */
    sc64_route_age[slot] = 0;
    assert(sc64_route_slot_for(other, other + 1) == slot);
    assert(sc64_route_point_saved == first && sc64_point_prefix_find(slot));
    assert(sc64_route_point_spare == saved);
    sc64_route_age[slot] = 0;
    assert(sc64_route_slot_for(other + 2, other + 3) == slot);
    assert(sc64_route_point_saved == first && sc64_route_point_spare == saved);
    assert(sc64_point_prefix_find(slot) && sc64_route_field[slot] == sc64_route_point_plane);
    SC64_ROUTE_POINT_TAIL_RELEASE();
    memset(sc64_route_point_plane, 0x53, SC64_ROUTE_POINT_BYTES);
    assert(sc64_route_point_saved < 0 && sc64_route_field[slot] == saved);
    assert(sc64_point_prefix_find(slot));
    assert(SC64_ROUTE_REACHES(slot, MAP_CELLS - 1));
    SC64_ROUTE_EXACT(slot);
    assert(!sc64_point_prefix_find(slot));
    reference_bfs(&oracle[POOL], other + 2);
    for (int i = 0; i < MAP_CELLS; ++i)
        assert(sc64_route_get(slot, i) == oracle[POOL].dir[i]);
    check_grid_unchanged();
    puts("completed tail exchange excludes parked prefix victims");
}
#endif
#endif

#ifdef SC64_ROUTE_POINT_COMPONENT_PROBE
static void point_component_map(void)
{
    race_cols = race_rows = 128;
    memset(desired, 1, sizeof(desired));
    for (int row = 0; row < MAP_ROWS; ++row) {
        desired[40 + row * MAP_COLS] = 0;
        desired[80 + row * MAP_COLS] = 0;
    }
    sc64_routes_reset();
    sc64_route_pcount = 0;
    build_path_grid();
    sc64_route_clock = 0;
    sc64_route_builds = sc64_route_hits = 0;
}

static void check_point_component_witness(void)
{
    const int left = 20 + 32 * 128, left_far = 20 + 100 * 128;
    const int middle = 60 + 32 * 128, middle_far = 60 + 100 * 128;
    const int right = 100 + 32 * 128, wall = 40 + 32 * 128;
    u32 formation_meta[2];

    for (int complete = 0; complete <= 1; ++complete) {
        point_component_map();
        int slot = sc64_route_slot_for(left, left + 1);
        int witness = sc64_route_slot(left_far);
        assert(slot >= 0 && witness >= 0 && sc64_point_prefix_find(slot));
        assert(sc64_route_get(slot, left_far) == SC64_ROUTE_UNREACHABLE);
        u32 before = state_hash();
#ifdef SC64_ROUTE_POINT_NEAR_COMPONENT_PROBE
        assert(!sc64_route_near_root_reaches(slot, middle_far, left + 1, left));
        assert(sc64_point_prefix_find(slot) && state_hash() == before);
#endif
        assert(SC64_ROUTE_REACHES(slot, left_far));
        assert(!SC64_ROUTE_REACHES(slot, middle_far));
        assert(sc64_point_prefix_find(slot) && state_hash() == before);

        if (complete) SC64_ROUTE_EXACT(slot);
        int hints[] = {slot, -1, -1, -1}, roots[] = {left, -1, -1, -1}, next = 0;
        u32 witness_age = sc64_route_age[witness];
        assert(sc64_route_cells_connected(&left_far, 1, &left, 1, &middle_far, 1,
                                           hints, roots, &next, 0) == 1);
        assert(hints[0] == slot && roots[0] == left && next == 1);
        assert(sc64_route_age[witness] == witness_age && sc64_route_hits == 1);
        assert(sc64_route_builds == 2 && !!sc64_point_prefix_find(slot) == !complete);
        formation_meta[complete] = state_hash();
    }
    assert(formation_meta[0] == formation_meta[1]);

    point_component_map();
    int slot = sc64_route_slot_for(left, left + 1);
    assert(sc64_route_slot(middle) >= 0);
    assert(!SC64_ROUTE_REACHES(slot, middle_far) && sc64_point_prefix_find(slot));

    point_component_map();
    slot = sc64_route_slot_for(left, left + 1);
    assert(sc64_route_slot(right) >= 0);
    assert(!SC64_ROUTE_REACHES(slot, middle_far) && !sc64_point_prefix_find(slot));

    point_component_map();
    slot = sc64_route_slot_for(left, left + 1);
    int witness = sc64_route_slot(wall);
    assert(sc64_route_get(witness, left) != SC64_ROUTE_UNREACHABLE);
    assert(sc64_route_get(witness, middle_far) != SC64_ROUTE_UNREACHABLE);
    assert(!SC64_ROUTE_REACHES(slot, middle_far) && !sc64_point_prefix_find(slot));

    point_component_map();
    slot = sc64_route_slot_for(left, left + 1);
    witness = sc64_route_slot_for(left_far, left_far + 1);
    assert(sc64_point_prefix_find(witness));
    assert(sc64_route_get(witness, left) == SC64_ROUTE_UNREACHABLE);
    assert(SC64_ROUTE_REACHES(slot, left_far) && !sc64_point_prefix_find(slot));
    assert(sc64_point_prefix_find(witness));

    point_component_map();
    slot = sc64_route_slot_for(left, left + 1);
    witness = sc64_route_slot(left_far);
    SC64_ROUTE_PF(witness) |= SC64_ROUTE_FLAG_STALE;
    assert(SC64_ROUTE_REACHES(slot, left_far) && !sc64_point_prefix_find(slot));
    check_grid_unchanged();
#ifdef SC64_ROUTE_POINT_NEAR_COMPONENT_PROBE
    point_component_map();
    slot = sc64_route_slot_for(left, left + 1);
    assert(sc64_route_slot(left_far) >= 0);
    u32 before = state_hash();
    assert(sc64_route_near_root_reaches(slot, left_far, left + 128, left));
    assert(!sc64_point_prefix_find(slot) && state_hash() == before);
    check_grid_unchanged();
#endif
    puts("point witnesses preserve yes/no/fallback, selected slot and logical cache metadata");
}
#endif

#ifdef SC64_ROUTE_POINT_NEAR_DISTANCE_PROBE
static void check_point_near_distance(void)
{
    const int width = 128, root = 24 + 24 * width;
    enum { PROOF, TIE, STALE, PARKED, BLOCKED };
    for (int mode = PROOF; mode <= BLOCKED; ++mode) {
        const int tie = mode == TIE;
        int origin = root + 10 - (tie ? 10 * width : 0);
        int goal = root + (tie ? 1 : width);
        int upper_seed = root + (tie ? -width : 1);
        int lower_seed = goal + width;
        race_cols = race_rows = width;
        memset(desired, 1, sizeof(desired));
        if (!tie) {
            for (int y = 22; y <= 30; ++y) desired[29 + y * width] = 0;
        }
        if (mode == BLOCKED) desired[lower_seed] = 0;
        sc64_routes_reset();
        sc64_route_pcount = 0;
        build_path_grid();
        sc64_route_clock = 0;
        int upper_slot = sc64_route_slot(upper_seed);
        int lower_slot = mode == PARKED ? sc64_route_slot_for(lower_seed, lower_seed + 1) :
            sc64_route_slot(lower_seed);
        int slot = sc64_route_slot_for(root, root - width);
        assert(slot >= 0 && upper_slot >= 0 && lower_slot >= 0);
        assert(sc64_point_prefix_find(slot) &&
               sc64_route_get(slot, origin) == SC64_ROUTE_UNREACHABLE);
        assert(!!sc64_point_prefix_find(lower_slot) == (mode == PARKED));
        if (mode == STALE) SC64_ROUTE_PF(lower_slot) |= SC64_ROUTE_FLAG_STALE;
        assert(!sc64_point_near_neighbor(width - 1, width));

        reference_bfs(&oracle[POOL], upper_seed);
        int upper = oracle[POOL].dist[origin] + 1;
        reference_bfs(&oracle[POOL], lower_seed);
        if (tie) assert(oracle[POOL].dist[origin] == upper);
        if (mode == PROOF) assert(oracle[POOL].dist[origin] > upper);
        reference_bfs(&oracle[POOL], root);
        int cursor = origin;
        for (unsigned step = 0; step < 128 && cursor != goal; ++step) {
            int at_root, next;
            if (!expected_next(&oracle[POOL], cursor % width, cursor / width, root,
                               &next, &at_root) || at_root) break;
            cursor = next;
        }
        int hit = cursor == goal;
        assert(hit == tie);

        u32 before = state_hash(), hits = sc64_route_hits, builds = sc64_route_builds;
        assert(sc64_route_near_root_reaches(slot, origin, goal, root) == hit);
        assert(state_hash() == before);
#ifdef SC64_ROUTE_POINT_NEAR_DEMAND_PROBE
        const Sc64PointPrefix *prefix = sc64_point_prefix_find(slot);
        if (mode == PROOF) {
            assert(prefix && sc64_route_get(slot, origin) == SC64_ROUTE_UNREACHABLE);
        } else if (prefix) {
            assert(prefix->count <= SC64_POINT_PREFIX_PENDING && prefix->second_cutoff);
            assert(sc64_route_get(slot, origin) == oracle[POOL].dir[origin]);
        }
#else
        assert(!!sc64_point_prefix_find(slot) == (mode == PROOF));
#endif
        assert((sc64_route_step_slot(origin, goal) == slot) == hit);
        if (hit) {
            assert(sc64_route_builds == builds && sc64_route_hits == hits + 1);
            assert(sc64_route_get(slot, origin) == oracle[POOL].dir[origin]);
        }
        check_grid_unchanged();
    }
    puts("near-root distance witnesses preserve strict misses, ties and invalid witness fallback");
}
#endif

#ifdef SC64_ROUTE_POINT_NEAR_DEMAND_PROBE
static void check_point_near_demand(void)
{
    race_cols = race_rows = 128;
    const int cols = MAP_COLS, root = 24 + 24 * cols;
    const int first = root + 17 * cols, origin = root + 20 * cols;
    const int whole_field_cell = root + 42 * cols;
    for (int follows = 0; follows <= 1; ++follows) {
        const int goal = root + (follows ? cols : 1);
        memset(desired, 1, sizeof(desired));
        sc64_routes_reset();
        sc64_route_pcount = 0;
        build_path_grid();
        reference_bfs(&oracle[POOL], root);
        int slot = sc64_route_slot_for(root, first);
        assert(slot >= 0);
        const Sc64PointPrefix *prefix = sc64_point_prefix_find(slot);
        assert(prefix && prefix->count <= SC64_POINT_PREFIX_PENDING && !prefix->second_cutoff);
        assert(sc64_route_get(slot, origin) == SC64_ROUTE_UNREACHABLE);

        const u32 before = state_hash();
        assert(sc64_route_near_root_reaches(slot, origin, goal, root) == follows);
        assert(state_hash() == before);
#ifdef SC64_ROUTE_POINT_NEAR_RESERVE_PROBE
        assert(!prefix->count);
        prefix = &sc64_point_prefixes[SC64_POINT_PREFIX_SLOTS];
#endif
        assert(sc64_point_prefix_find(slot) == prefix && prefix->second_cutoff);
        assert(sc64_route_get(slot, origin) == oracle[POOL].dir[origin]);
        for (int cell = 0; cell < MAP_CELLS; ++cell) {
            int direction = sc64_route_get(slot, cell);
            if (direction != SC64_ROUTE_UNREACHABLE) assert(direction == oracle[POOL].dir[cell]);
        }

        /* A missed read after the second cutoff still completes the field. */
        assert(sc64_route_get(slot, whole_field_cell) == SC64_ROUTE_UNREACHABLE);
        if (follows)
            assert(sc64_route_near_root_reaches(slot, whole_field_cell, goal, root));
        else
            SC64_ROUTE_EXACT(slot);
        assert(!sc64_point_prefix_find(slot) && state_hash() == before);
        for (int cell = 0; cell < MAP_CELLS; ++cell)
            assert(sc64_route_get(slot, cell) == oracle[POOL].dir[cell]);
        check_grid_unchanged();
    }
    puts("Nearby-root reads preserve canonical hits and misses through bounded and full resumption.");
}
#endif

#ifdef SC64_ROUTE_POINT_NEAR_RESERVE_PROBE
static void check_point_near_reserve(void)
{
    race_cols = race_rows = 128;
    const int cols = MAP_COLS, first_root = 24 + 24 * cols;
    Sc64PointPrefix *reserve = &sc64_point_prefixes[SC64_POINT_PREFIX_SLOTS];
    memset(desired, 1, sizeof(desired));
    sc64_routes_reset();
    sc64_route_pcount = 0;
    build_path_grid();
    int first_slot = sc64_route_slot_for(first_root, first_root + 17 * cols);
    assert(first_slot >= 0);
    const Sc64PointPrefix *cold = sc64_point_prefix_find(first_slot);
    assert(cold && cold != reserve && !reserve->count);
    assert(sc64_route_near_root_reaches(first_slot, first_root + 20 * cols,
                                       first_root + cols, first_root));
    assert(!cold->count && reserve->second_cutoff);
    assert(sc64_point_prefix_find(first_slot) == reserve);
    assert(sc64_point_prefix_index[first_slot] == SC64_POINT_PREFIX_SLOTS + 1);

    int roots[SC64_POINT_PREFIX_SLOTS], slots[SC64_POINT_PREFIX_SLOTS];
    for (int i = 0; i < SC64_POINT_PREFIX_SLOTS; ++i) {
        roots[i] = first_root + 12 + 8 * i;
        slots[i] = sc64_route_slot_for(roots[i], roots[i] + 17 * cols);
        assert(slots[i] >= 0);
        const Sc64PointPrefix *prefix = sc64_point_prefix_find(slots[i]);
        assert(prefix && prefix != reserve && !prefix->second_cutoff);
    }
    assert(!sc64_point_prefix_free());
    const int extra_root = first_root + 56;
    const int extra_slot = sc64_route_slot_for(extra_root, extra_root + 17 * cols);
    assert(extra_slot >= 0 && !sc64_point_prefix_find(extra_slot));
    assert(sc64_point_prefix_find(first_slot) == reserve);

    /* A live reserve prevents a second nearby repark. The exact cold
     * field completes in place and the reserved owner stays valid. */
    assert(sc64_route_near_root_reaches(slots[0], roots[0] + 20 * cols,
                                       roots[0] + cols, roots[0]));
    assert(!sc64_point_prefix_find(slots[0]) && sc64_point_prefix_find(first_slot) == reserve);
    reference_bfs(&oracle[POOL], roots[0]);
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        assert(sc64_route_get(slots[0], cell) == oracle[POOL].dir[cell]);

    /* A repurposed range slot no longer owns the reserve. Reassignment
     * clears that stale index and publishes only the new owner. */
    sc64_route_valid[first_slot] = 2;
    assert(!sc64_point_prefix_find(first_slot));
    assert(sc64_route_near_root_reaches(slots[1], roots[1] + 20 * cols,
                                       roots[1] + cols, roots[1]));
    assert(!sc64_point_prefix_index[first_slot] && sc64_point_prefix_find(slots[1]) == reserve);
    reference_bfs(&oracle[POOL], roots[1]);
    SC64_ROUTE_EXACT(slots[1]);
    assert(!reserve->count && !sc64_point_prefix_index[slots[1]]);
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        assert(sc64_route_get(slots[1], cell) == oracle[POOL].dir[cell]);
    check_grid_unchanged();
    puts("Nearby-root reserve keeps four cold slots, completes when occupied and releases stale owners.");
}
#endif

#ifdef SC64_ROUTE_KEEP_FIELDS
/* Adopt an exact donor as lazy after a harmless closure, then request a
 * nearby goal on that gradient. Closing a tile on the path must instead
 * materialize the field before deciding the requested next step. */
static void check_nearby_lazy_root(void)
{
    int slot, nx, ny;
    race_cols = race_rows = 64;
    int origin = 20 + 4 * MAP_COLS, root = origin + 4;
    memset(desired, 1, sizeof desired);
    sc64_routes_reset();
    build_path_grid();
    assert(sc64_route_slot_for(root, origin) >= 0);
    desired[0] = 0;
    invalidate();
    build_path_grid();
    slot = sc64_route_slot_for(root, origin);
    assert(slot >= 0 && SC64_ROUTE_LAZY(slot));
    assert(sc64_route_next(20 * 16 + 8, 4 * 16 + 8, 23 * 16 + 8, 4 * 16 + 8, &nx, &ny));
    assert(nx == 21 * 16 + 8 && ny == 4 * 16 + 8 && SC64_ROUTE_LAZY(slot));

    desired[origin + 1] = 0;
    invalidate();
    build_path_grid();
    slot = sc64_route_slot_for(root, root + MAP_COLS);
    assert(slot >= 0 && SC64_ROUTE_LAZY(slot));
    assert(sc64_route_next(20 * 16 + 8, 4 * 16 + 8, 23 * 16 + 8, 4 * 16 + 8, &nx, &ny));
    assert(nx == 20 * 16 + 8 && (ny == 3 * 16 + 8 || ny == 5 * 16 + 8));
    assert(!SC64_ROUTE_LAZY(slot));
}
#endif

#ifdef SC64_ROUTE_LINEAR_EMPTY_GOAL_PROBE
static void check_empty_linear_goals(void)
{
    const int widths[] = { 96, 128 };
    for (unsigned i = 0; i < sizeof(widths) / sizeof(widths[0]); ++i) {
        race_cols = widths[i];
        race_rows = 128;
        const int last = MAP_COLS * MAP_ROWS - 1;
        const int goals[] = { 0, MAP_COLS - 1, last - MAP_COLS + 1, last };
        const int wrapping[] = { MAP_COLS - 1, MAP_COLS,
                                 last - MAP_COLS, last - MAP_COLS + 1 };
        const int neighbors[] = { MAP_COLS + 1, 2 * MAP_COLS - 2,
                                  last - 2 * MAP_COLS + 2, last - MAP_COLS - 1 };
        for (unsigned j = 0; j < sizeof(goals) / sizeof(goals[0]); ++j) {
            memset(desired, 0, sizeof desired);
            desired[wrapping[j]] = 1;
            sc64_routes_reset();
            build_path_grid();
            int slot = sc64_route_slot_for(goals[j], -1);
            assert(slot >= 0 && sc64_route_valid[slot] == 1);
            for (int cell = 0; cell <= last; ++cell)
                assert(sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE);

#ifdef SC64_ROUTE_POINT_PREFIX_PROBE
            if (MAP_COLS == 128) {
                sc64_routes_reset();
                sc64_route_pcount = 0;
                build_path_grid();
                memset(bfs_queue, 0xa5, sizeof(bfs_queue));
                const u32 fills = sc64_route_builds;
                slot = sc64_route_slot_for(goals[j], wrapping[j]);
                assert(slot >= 0 && sc64_route_valid[slot] == 1 &&
                       sc64_route_goal[slot] == goals[j] && sc64_route_builds == fills + 1);
                assert(!sc64_point_prefix_find(slot));
                for (int cell = 0; cell <= last; ++cell)
                    assert(sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE);
                for (unsigned k = 0; k < sizeof(bfs_queue) / sizeof(*bfs_queue); ++k)
                    assert(bfs_queue[k] == 0xa5a5u);
            }
#endif

            desired[neighbors[j]] = 1;
            sc64_routes_reset();
            build_path_grid();
            slot = sc64_route_slot_for(goals[j], -1);
            assert(slot >= 0 && sc64_route_get(slot, neighbors[j]) == 0);
        }
    }
    puts("Empty point fields reject wrapped seeds and preserve diagonal seeds.");
}
#endif

#ifdef SC64_ROUTE_POINT_COLD_CERTIFICATE
static void check_point_certificate_wider_retry(void)
{
    race_cols = race_rows = 128;
    const int cols = MAP_COLS;
    const int goal = 32 + 32 * cols, near = goal + 2 * cols - 1;
    const int closer_seed = goal + 1, connected_seed = goal + cols - 1;
    memset(desired, 1, sizeof(desired));
    for (int row = -1; row <= 1; ++row)
        for (int col = -1; col <= 1; ++col)
            desired[goal + row * cols + col] = 0;
    desired[goal + 2] = 0;
    /* The nearer seed is isolated. The canonical chain uses the other seed. */
    desired[closer_seed] = desired[connected_seed] = 1;
    for (int gap = 17; gap <= 18; ++gap) {
        const int origin = goal + 13 * cols + gap - 13;
        sc64_routes_reset();
        sc64_route_pcount = 0;
        build_path_grid();
        reference_bfs(&oracle[POOL], goal);
        int slot = sc64_route_slot_for(goal, near);
        Sc64PointPrefix *prefix = sc64_point_prefix_find(slot);
        assert(prefix && prefix->count == SC64_POINT_CERTIFIED_ONLY);
        u8 *field = sc64_route_field[slot];
        assert(sc64_route_get(slot, origin) == SC64_ROUTE_UNREACHABLE);
        assert(!sc64_point_certificate(slot, origin, goal, 1));
        assert(!sc64_point_certificate(slot, goal + 13 * cols + 6, goal, 0));
        assert(sc64_route_get(slot, origin) == SC64_ROUTE_UNREACHABLE);
        const unsigned builds = sc64_route_builds, hits = sc64_route_hits;
        assert(sc64_route_slot_for(goal, origin) == slot);
        assert(sc64_route_builds == builds && sc64_route_hits == hits + 1);
        assert(sc64_route_field[slot] == field && sc64_point_prefix_find(slot) == prefix &&
               prefix->count == SC64_POINT_CERTIFIED_ONLY);
        const int step[] = {0, 1, -1, cols, -cols};
        int cell = origin;
        for (int n = 0; n < gap; ++n) {
            int direction = sc64_route_get(slot, cell);
            assert(direction > 0 && direction <= 4 && direction == oracle[POOL].dir[cell]);
            cell += step[direction];
        }
        assert(cell == connected_seed && sc64_route_get(slot, cell) == 0);
        const u32 before = state_hash();
        SC64_ROUTE_EXACT(slot);
        assert(!sc64_point_prefix_find(slot) && state_hash() == before);
        for (int i = 0; i < MAP_CELLS; ++i)
            assert(sc64_route_get(slot, i) == oracle[POOL].dir[i]);
        check_grid_unchanged();
    }
    puts("Exact cache hits certify 17–18-gap retries without materializing the blocked-goal field.");
}

#ifdef SC64_ROUTE_POINT_PREFIX_SPARSE_PACK_PROBE
static void check_point_prefix_sparse_completion(void)
{
    const int sizes[] = { 2, 384, 385 };
    race_cols = race_rows = 128;
    const int cols = MAP_COLS, goal = 18 + 19 * cols, origin = 70 + 66 * cols;
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(*sizes); ++i) {
        memset(desired, 0, sizeof(desired));
        for (int n = 0; n < sizes[i]; ++n)
            desired[goal + (n / 32) * cols + n % 32] = 1;
        desired[origin] = 1;
        sc64_routes_reset();
        sc64_route_pcount = 0;
        build_path_grid();
        reference_bfs(&oracle[POOL], goal);
        const unsigned builds = sc64_route_builds;
        int slot = sc64_route_slot_for(goal, origin);
        assert(slot >= 0 && sc64_route_builds == builds + 1);
        assert(!sc64_point_prefix_find(slot));
        assert(sc64_route_get(slot, origin) == SC64_ROUTE_UNREACHABLE);
        for (int cell = 0; cell < MAP_CELLS; ++cell)
            assert(sc64_route_get(slot, cell) == oracle[POOL].dir[cell]);
        check_grid_unchanged();
    }
    puts("Exhausted cold prefixes match full fields at 2, 384 and 385 visited cells.");
}
#endif

static void check_point_cold_certificate(void)
{
    int certified = 0, declined = 0, blocked_goals = 0;
    race_cols = race_rows = 128;
    for (int i = 0; i < 256; ++i) {
        for (int cell = 0; cell < MAP_CELLS; ++cell)
            desired[cell] = (u8)(aux_rn(100) < 80);
        int gx = aux_rn(128), gy = aux_rn(128);
        int ox = limit(gx + aux_rn(17) - 8, 0, 127);
        int oy = limit(gy + aux_rn(17) - 8, 0, 127);
        int goal = gy * MAP_COLS + gx, origin = oy * MAP_COLS + ox;
        sc64_routes_reset();
        sc64_route_pcount = 0;
        build_path_grid();
        int slot = sc64_route_slot_for(goal, origin);
        assert(slot >= 0);
        Sc64PointPrefix *prefix = sc64_point_prefix_find(slot);
        if (!prefix || prefix->count != SC64_POINT_CERTIFIED_ONLY) {
            ++declined;
            continue;
        }
        ++certified;
        blocked_goals += !desired[goal];
        reference_bfs(&oracle[POOL], goal);
        assert(sc64_route_get(slot, origin) == oracle[POOL].dir[origin]);
        for (int cell = 0; cell < MAP_CELLS; ++cell) {
            int direction = sc64_route_get(slot, cell);
            if (direction != SC64_ROUTE_UNREACHABLE) {
                assert(direction == oracle[POOL].dir[cell]);
                if (direction) {
                    const int step[] = {0, 1, -1, MAP_COLS, -MAP_COLS};
                    assert(sc64_route_get(slot, cell + step[direction]) != SC64_ROUTE_UNREACHABLE);
                }
            }
        }
        u32 before = state_hash();
        SC64_ROUTE_EXACT(slot);
        assert(!sc64_point_prefix_find(slot) && before == state_hash());
        for (int cell = 0; cell < MAP_CELLS; ++cell)
            assert(sc64_route_get(slot, cell) == oracle[POOL].dir[cell]);
        check_grid_unchanged();
    }
    assert(certified > 30 && declined > 30 && blocked_goals > 5);

    memset(desired, 1, sizeof(desired));
    sc64_routes_reset();
    sc64_route_pcount = 0;
    build_path_grid();
    const int goal = 32 + 32 * MAP_COLS;
    const int slot = sc64_route_slot_for(goal, goal + 1);
    assert(sc64_point_prefix_find(slot)->count == SC64_POINT_CERTIFIED_ONLY);
    const u32 before = state_hash();
    sc64_point_prefix_require(slot, goal + 2 * MAP_COLS);
    assert(sc64_point_prefix_find(slot)->count == SC64_POINT_CERTIFIED_ONLY);
    assert(sc64_route_near_root_reaches(slot, goal + 2 * MAP_COLS, goal + MAP_COLS, goal));
    assert(sc64_route_get(slot, goal + 2 * MAP_COLS) == 4 && before == state_hash());
    sc64_point_prefix_require(slot, goal + 30 * MAP_COLS);
    assert(!sc64_point_prefix_find(slot) && sc64_route_get(slot, goal + 30 * MAP_COLS) == 4);
    assert(before == state_hash());
    printf("Cold certificate covers %d shortest paths, %d blocked goals and %d declines.\n",
           certified, blocked_goals, declined);
}
#endif

#ifdef SC64_ROUTE_LINEAR_SPARSE_PACK_PROBE
static void check_sparse_linear_pack(void)
{
    const int sizes[] = { 1, 361, 384, 385, MAP_CELLS };
    const int goal = MAP_CELLS - 1;
    race_cols = race_rows = 128;
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(*sizes); ++i) {
        const int size = sizes[i];
        memset(desired, 0, sizeof(desired));
        for (int j = 0; j < size; ++j) {
            int cell = size == 361 ? goal - j % 19 - j / 19 * 128 : goal - j;
            desired[cell] = 1;
        }
        if (size != MAP_CELLS) desired[0] = 1;
        sc64_routes_reset();
        build_path_grid();
        int slot = sc64_route_slot_for(goal, -1);
        assert(slot >= 0);
        assert(sc64_route_fill_linear(sc64_route_field[slot], goal) == size);
        reference_bfs(&oracle[POOL], goal);
        for (int cell = 0; cell < MAP_CELLS; ++cell)
            assert(sc64_route_get(slot, cell) == oracle[POOL].dir[cell]);
        check_grid_unchanged();
    }

    /* A blocked goal may seed disjoint components, including diagonal cells. */
    memset(desired, 0, sizeof(desired));
    int blocked = 40 + 40 * 128;
    desired[blocked - 129] = desired[blocked + 129] = 1;
    sc64_routes_reset();
    build_path_grid();
    int slot = sc64_route_slot_for(blocked, -1);
    assert(slot >= 0);
    assert(sc64_route_fill_linear(sc64_route_field[slot], blocked) == 2);
    reference_bfs(&oracle[POOL], blocked);
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        assert(sc64_route_get(slot, cell) == oracle[POOL].dir[cell]);
    check_grid_unchanged();
    puts("Sparse linear packing matches canonical BFS at map edges and across the cutoff.");
}
#endif

int main(int argc, char **argv)
{
    static const int shapes[][2] = { { 64, 64 }, { 128, 96 }, { 128, 128 }, { 100, 70 }, { 37, 53 },
#ifdef TEST_LINEAR_FLOOD
                                    { 96, 128 },
#endif
    };
    const char *emit = 0, *compare = 0;
#ifdef SC64_ROUTE_LINEAR_SPARSE_PACK_PROBE
    if (argc == 2 && !strcmp(argv[1], "--sparse-linear-only")) {
        check_sparse_linear_pack();
        return 0;
    }
#endif
#ifdef SC64_ROUTE_POINT_COLD_CERTIFICATE
    if (argc == 2 && !strcmp(argv[1], "--point-certificate-only")) {
        check_point_cold_certificate();
        check_point_certificate_wider_retry();
#ifdef SC64_ROUTE_POINT_PREFIX_SPARSE_PACK_PROBE
        check_point_prefix_sparse_completion();
#endif
        return 0;
    }
#endif
#ifdef SC64_ROUTE_POINT_NEAR_DISTANCE_PROBE
    if (argc == 2 && !strcmp(argv[1], "--point-near-distance-only")) {
        check_point_near_distance();
        return 0;
    }
#endif
#ifdef SC64_ROUTE_POINT_NEAR_DEMAND_PROBE
    if (argc == 2 && !strcmp(argv[1], "--point-near-demand-only")) {
        check_point_near_demand();
#ifdef SC64_ROUTE_POINT_NEAR_RESERVE_PROBE
        check_point_near_reserve();
#endif
        return 0;
    }
#endif
#ifdef SC64_ROUTE_POINT_COMPONENT_PROBE
    if (argc == 2 && !strcmp(argv[1], "--point-component-only")) {
        check_point_component_witness();
        return 0;
    }
#endif
#ifdef SC64_ROUTE_POINT_PREFIX_PROBE
    if (argc == 2 && !strcmp(argv[1], "--point-prefix-only")) {
        check_point_prefix_resume();
        check_point_prefix_row_span();
#ifdef SC64_ROUTE_POINT_SECOND_EXACT_HIT
        check_point_prefix_exact_hit();
#endif
#ifdef SC64_ROUTE_POINT_TAIL_PROBE
        check_point_prefix_tail_exchange();
#endif
        return 0;
    }
#endif
#ifdef SC64_ROUTE_POINT_POINTER_PROBE
    if (argc == 2 && !strcmp(argv[1], "--point-pointer-only")) {
        check_single_retained_point();
        return 0;
    }
#endif
    for (int i = 1; i + 1 < argc; ++i) {
        if (!strcmp(argv[i], "--emit")) emit = argv[i + 1];
        if (!strcmp(argv[i], "--compare")) compare = argv[i + 1];
    }
#ifdef SC64_ROUTE_KEEP_FIELDS
    if (argc == 2 && !strcmp(argv[1], "--nearby-lazy-only")) {
        check_nearby_lazy_root();
        return 0;
    }
#endif
    /* The same scenarios run in the same order in every configuration. */
    for (int round = 0; round < 3; ++round)
    for (unsigned s = 0; s < sizeof(shapes) / sizeof(shapes[0]); ++s)
        for (int terrain = 0; terrain < T_KINDS; ++terrain)
            for (int kind = 0; kind < K_KINDS; ++kind)
                scenario(shapes[s][0], shapes[s][1], terrain, kind, 10);

#ifdef SC64_ROUTE_LINEAR_EMPTY_GOAL_PROBE
    check_empty_linear_goals();
#endif

    printf("validated queries: %lu\n", validated);
    for (int k = 0; k < K_KINDS; ++k) printf("builds %-7s %lu\n", kind_name[k], kind_builds[k]);
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    printf("eviction/reset donors: adopted %u of %u floods needed; fresh donors checked %lu\n",
           keep_stat[KEEP_ADOPT], keep_stat[KEEP_ADOPT] + keep_stat[KEEP_FILL], donors_checked);
    assert(keep_stat[KEEP_ADOPT] > 1000 && donors_checked > 1000);
#endif
#ifdef SC64_ROUTE_KEEP_FIELDS
    printf("gate: calls %u hits %u | fail blocked %u code %u length %u seeds %u near-goal %u opened %u\n",
           keep_stat[KEEP_GATE_CALL], keep_stat[KEEP_GATE_HIT], keep_stat[KEEP_FAIL_BLOCKED],
           keep_stat[KEEP_FAIL_CODE], keep_stat[KEEP_FAIL_LENGTH], keep_stat[KEEP_FAIL_SEEDS],
           keep_stat[KEEP_FAIL_NEAR_GOAL], keep_stat[KEEP_FAIL_OPENED]);
    printf("commit: same %u overflow %u dropped %u | fills %u materialized %u | gate codes checked %lu\n",
           keep_stat[KEEP_COMMIT_SAME], keep_stat[KEEP_COMMIT_OVERFLOW],
           keep_stat[KEEP_COMMIT_DROP], keep_stat[KEEP_FILL], keep_stat[KEEP_MATERIALIZE],
           gate_checked);
    assert(keep_stat[KEEP_GATE_HIT] > 2000);
    assert(keep_stat[KEEP_FAIL_BLOCKED] && keep_stat[KEEP_FAIL_CODE] &&
           keep_stat[KEEP_FAIL_LENGTH] && keep_stat[KEEP_FAIL_SEEDS] &&
           keep_stat[KEEP_FAIL_NEAR_GOAL] && keep_stat[KEEP_FAIL_OPENED]);
    assert(keep_stat[KEEP_COMMIT_SAME] && keep_stat[KEEP_COMMIT_OVERFLOW] &&
           keep_stat[KEEP_COMMIT_DROP]);
    assert(gate_checked > 20000);
#endif
    printf("resident fields checked against a fresh flood: %lu\n", exact_fields_checked);
    assert(validated >= 200000 && exact_fields_checked > 1000);
    if (emit) {
        char path[512];
        FILE *f = fopen(emit, "w");
        assert(f);
        for (int k = 0; k < K_KINDS; ++k) fprintf(f, "%s %lu\n", kind_name[k], kind_builds[k]);
        fclose(f);
        snprintf(path, sizeof(path), "%s.state", emit);
        f = fopen(path, "wb");
        assert(f && fwrite(state_hashes, sizeof(u32), state_hash_count, f) == state_hash_count);
        fclose(f);
    }
    if (compare) {
        FILE *f = fopen(compare, "r");
        char name[32], path[512];
        unsigned long base, total_base = 0, total_keep = 0, n = 0;
        u32 reference;
        assert(f);
        {   /* cache residency, query by query, must match the build without the macro */
            FILE *g;
            snprintf(path, sizeof(path), "%s.state", compare);
            g = fopen(path, "rb");
            assert(g);
            while (fread(&reference, sizeof(reference), 1, g) == 1) {
                if (n >= state_hash_count || state_hashes[n] != reference) {
                    printf("CACHE RESIDENCY DIFFERS from the build without the macro at query %lu\n", n);
                    abort();
                }
                ++n;
            }
            assert(n == state_hash_count);
            fclose(g);
            printf("cache residency identical after all %lu queries\n", n);
        }
        for (int k = 0; k < K_KINDS; ++k) {
            assert(fscanf(f, "%31s %lu", name, &base) == 2 && !strcmp(name, kind_name[k]));
            printf("kind %-7s baseline builds %lu, keep builds %lu\n", name, base, kind_builds[k]);
            assert(kind_builds[k] <= base);
            if (k == K_BLOCK || k == K_CYCLE) assert(kind_builds[k] < base);
            if (k == K_CYCLE) assert(kind_builds[k] * 10 < base * 7);
            total_base += base; total_keep += kind_builds[k];
        }
        fclose(f);
        assert(total_keep < total_base);
        printf("total baseline builds %lu, keep builds %lu (%.1f%%)\n", total_base, total_keep,
               100.0 * (double)total_keep / (double)total_base);
    }
#ifdef SC64_ROUTE_LINEAR_FLOOD_PROBE
    check_linear_uses_expected_path(96);
    check_linear_uses_expected_path(128);
#endif
#ifdef SC64_ROUTE_KEEP_FIELDS
    check_nearby_lazy_root();
#endif
#ifdef SC64_ROUTE_POINT_NEAR_DISTANCE_PROBE
    check_point_near_distance();
#endif
    puts("sc64 route keep-fields: ok");
    return 0;
}
