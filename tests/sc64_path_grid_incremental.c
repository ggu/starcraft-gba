/* Asset-free randomized differential test of the incremental walkability
 * update in starcraft/path_grid_build.h.
 *
 * The game's build_path_grid runs on stand-ins for the game data (buildings,
 * terrain, the per-profile collision predicates and footprint tables) and its
 * grid is compared, cell by cell and after every rebuild, with reference_full
 * below: a verbatim copy of the whole-grid rebuild as it was before the
 * incremental update existed, writing to its own grid. Scenarios change the map
 * (dimensions and terrain identity), place, kill, move and retype structures,
 * flip the collidable, airborne and door predicates, rebuild with nothing
 * changed, change many structures at once (more than the dirty list holds) and
 * place structures partly or wholly outside the map. The build also reports how
 * many rebuilds took each path so the test cannot pass without using the
 * incremental one.
 *
 * Variants (selected by -D, see mk/combat-perf/Makefile):
 *   VARIANT_TERRAN   T4 runtime + airborne battlecruiser facilities, packed grid
 *   VARIANT_ZERG     Z5 doors + source-bounds footprints, bulk terrain, byte grid
 *   VARIANT_SCENARIO T4 scenario doors + Z5 doors + source bounds, packed grid
 *   VARIANT_PLAIN    race maps only (circle footprints), byte grid
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
#define GBA_HOT

#define SC64_RACES
#define SC64_IMPORTED_SKIRMISH
#define SC64_PATH_GRID_STATS
#ifndef NO_INCREMENTAL
#define SC64_PATH_GRID_INCREMENTAL
#endif
#ifdef VARIANT_TERRAN
#define SC64_T4_RUNTIME
#define SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
#define SC64_SHOWCASE_ENABLED
#define TEST_PACKED_GRID
#endif
#ifdef VARIANT_ZERG
#define SC64_Z5_RUNTIME
#define SC64_ZP_CAMPAIGN_ACTORS
#define SC64_ZP_CAMPAIGN_STORAGE
#define SC64_IMPORTED_MISSION
#define SC64_LARGE_MAPS
#define SC64_BULK_TERRAIN_GRID
#endif
#ifdef VARIANT_SCENARIO
#define SC64_T4_RUNTIME
#define SC64_T4_SCENARIO_RUNTIME
#define SC64_Z5_RUNTIME
#define SC64_ZP_CAMPAIGN_ACTORS
#define SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
#define TEST_PACKED_GRID
#endif

#define MAX_BUILDINGS 96
enum { MAP_CELLS_MAX = 128 * 128, B_ZERG_BEACON = 7 };
static int race_cols = 64, race_rows = 64;
#define MAP_COLS race_cols
#define MAP_ROWS race_rows
#ifdef TEST_PACKED_GRID
#define SC64_GRID_STORAGE_BYTES ((MAP_CELLS_MAX + 7) / 8)
#define SC64_GRID_GET(grid, cell) (((grid)[(cell) >> 3] >> ((cell) & 7u)) & 1u)
#define SC64_GRID_SET(grid, cell, value) do { \
    u8 mask_ = (u8)(1u << ((cell) & 7u)); \
    if (value) (grid)[(cell) >> 3] |= mask_; \
    else (grid)[(cell) >> 3] &= (u8)~mask_; \
} while (0)
#else
#define SC64_GRID_STORAGE_BYTES MAP_CELLS_MAX
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_GRID_SET(grid, cell, value) ((grid)[(cell)] = (u8)((value) != 0))
#endif

typedef struct { int x, y, type, alive; } Building;
static Building b[MAX_BUILDINGS];
static u8 path_walkable[SC64_GRID_STORAGE_BYTES];
static int path_grid_valid;
static int race_mode, race_selected_mission, race_skirmish_map, sc64_showcase_scene = -1;

static int limit(int n, int lo, int hi) { return n < lo ? lo : (n > hi ? hi : n); }
static int dist2(int x, int y, int a, int c) { int dx = x - a, dy = y - c; return dx * dx + dy * dy; }

/* ---- stand-in game state ------------------------------------------------ */
static u8 collid_off[MAX_BUILDINGS], airborne[MAX_BUILDINGS], door_closed[MAX_BUILDINGS];
static int t4_active, z5_active;
static u8 mock_blocked[MAP_CELLS_MAX];
static u32 terrain_salt;

static u32 hash3(u32 a, u32 c, u32 d)
{
    u32 h = a * 0x9e3779b1u ^ c * 0x85ebca6bu ^ d * 0xc2b2ae35u;
    h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12; h *= 0x297a2d39u; h ^= h >> 15;
    return h;
}
static void mock_terrain_fill(void)
{
    int c, r;
    /* Terrain depends on the scene only where the game's terrain_blocked does. */
    terrain_salt = (u32)(race_mode * 7 + race_selected_mission * 131 + race_skirmish_map * 17 +
                         race_cols * 3 + race_rows);
#ifdef SC64_SHOWCASE_ENABLED
    terrain_salt += (u32)((sc64_showcase_scene + 1) * 29);
#endif
    for (r = 0; r < race_rows; ++r)
        for (c = 0; c < race_cols; ++c)
            mock_blocked[c + r * race_cols] = (u8)(hash3(terrain_salt, (u32)c, (u32)r) % 9 == 0);
}
static int terrain_blocked(int col, int row)
{
    if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) return 1;
    return mock_blocked[col + row * MAP_COLS] & 1;
}
#ifdef SC64_BULK_TERRAIN_GRID
static const u8 *sc64_base_blocked_map(void) { return mock_blocked; }
static void sc64_terrain_grid_pack(u8 *grid, const u8 *blocked, unsigned cells)
{
    unsigned i;
    for (i = 0; i < cells; ++i) SC64_GRID_SET(grid, i, !(blocked[i] & 1));
}
#endif

/* Footprint tables: pure functions of the type, as in the game. */
static int bounds_for(int type, int *l, int *u, int *r, int *d)
{
    if (type % 5 == 4) return 0;
    *l = 24 + 8 * (type % 4); *r = 32 + 16 * (type % 3);
    *u = 16 + 8 * (type % 5); *d = 40 + 8 * (type % 2);
    return 1;
}
#define sc64_game_building_bounds_source bounds_for
#define sc64_race_t4_building_bounds_source(t, l, u, r, d) bounds_for((t) + 3, l, u, r, d)
static int sc64_t4_building_collidable(int slot) { return !collid_off[slot]; }
static int race_bc_facility_airborne(int slot) { return airborne[slot]; }
static int race_t4_active(void) { return t4_active; }
static int sc64_race_t4_is_door(int type) { return type == 12 || type == 13; }
static int race_z5_active(void) { return z5_active; }
static int race_z5_is_door_type(int type) { return type == 10 || type == 11; }
static int race_z5_building_collidable(int slot) { return door_closed[slot]; }
#ifdef SC64_T4_SCENARIO_RUNTIME
static void race_t4_block_door_grid(int slot)
{
    int left, up, right, down;
    const Building *door = &b[slot];
    if (!sc64_race_t4_building_bounds_source(door->type, &left, &up, &right, &down)) return;
    int min_col = limit((door->x * 2 - left) / 32, 0, MAP_COLS - 1);
    int max_col = limit((door->x * 2 + right) / 32, 0, MAP_COLS - 1);
    int min_row = limit((door->y * 2 - up) / 32, 0, MAP_ROWS - 1);
    int max_row = limit((door->y * 2 + down) / 32, 0, MAP_ROWS - 1);
    for (int row = min_row; row <= max_row; ++row)
        for (int col = min_col; col <= max_col; ++col)
            SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 0);
}
#endif

/* ---- the reference: the whole-grid rebuild as it was, on its own grid ---- */
static u8 ref_grid[SC64_GRID_STORAGE_BYTES];
static void reference_full(void)
{
    int col, row, i;
    memset(ref_grid, 0, sizeof ref_grid);
    for (row = 0; row < MAP_ROWS; row++)
        for (col = 0; col < MAP_COLS; col++) {
            int cell = col + row * MAP_COLS;
            SC64_GRID_SET(ref_grid, cell, !terrain_blocked(col, row));
        }
    for (i = 0; i < MAX_BUILDINGS; i++) if (b[i].alive) {
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
        if (race_bc_facility_airborne(i)) continue;
#endif
        if (b[i].type == B_ZERG_BEACON) continue;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (!sc64_t4_building_collidable(i)) continue;
#ifdef SC64_T4_RUNTIME
#ifdef SC64_T4_SCENARIO_RUNTIME
        if (race_t4_active() && sc64_race_t4_is_door(b[i].type)) {
            int left, up, right, down;
            if (sc64_race_t4_building_bounds_source(b[i].type, &left, &up, &right, &down)) {
                int c0 = limit((b[i].x * 2 - left) / 32, 0, MAP_COLS - 1);
                int c1 = limit((b[i].x * 2 + right) / 32, 0, MAP_COLS - 1);
                int r0 = limit((b[i].y * 2 - up) / 32, 0, MAP_ROWS - 1);
                int r1 = limit((b[i].y * 2 + down) / 32, 0, MAP_ROWS - 1);
                for (row = r0; row <= r1; row++) for (col = c0; col <= c1; col++)
                    SC64_GRID_SET(ref_grid, col + row * MAP_COLS, 0);
            }
            continue;
        }
#endif
#endif
#ifdef SC64_Z5_RUNTIME
        if (race_z5_active() && race_z5_is_door_type(b[i].type)) {
            if (race_z5_building_collidable(i)) {
                int left, up, right, down;
                if (sc64_game_building_bounds_source(b[i].type, &left, &up, &right, &down)) {
                    int c0 = limit((b[i].x * 2 - left) / 32, 0, MAP_COLS - 1);
                    int c1 = limit((b[i].x * 2 + right) / 32, 0, MAP_COLS - 1);
                    int r0 = limit((b[i].y * 2 - up) / 32, 0, MAP_ROWS - 1);
                    int r1 = limit((b[i].y * 2 + down) / 32, 0, MAP_ROWS - 1);
                    for (row = r0; row <= r1; row++) for (col = c0; col <= c1; col++)
                        SC64_GRID_SET(ref_grid, col + row * MAP_COLS, 0);
                }
            }
            continue;
        }
#endif
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        {
            int left, up, right, down;
            if (sc64_game_building_bounds_source(b[i].type, &left, &up, &right, &down)) {
                int x0 = b[i].x * 2 - left, x1 = b[i].x * 2 + right;
                int y0 = b[i].y * 2 - up, y1 = b[i].y * 2 + down;
                int c0 = limit(x0 / 32, 0, MAP_COLS - 1);
                int c1 = limit(x1 / 32, 0, MAP_COLS - 1);
                int r0 = limit(y0 / 32, 0, MAP_ROWS - 1);
                int r1 = limit(y1 / 32, 0, MAP_ROWS - 1);
                for (row = r0; row <= r1; ++row)
                    for (col = c0; col <= c1; ++col) {
                        int cell_x0 = col * 32, cell_y0 = row * 32;
                        int cell_x1 = cell_x0 + 32, cell_y1 = cell_y0 + 32;
                        if (cell_x0 <= x1 && cell_x1 >= x0 && cell_y0 <= y1 && cell_y1 >= y0)
                            SC64_GRID_SET(ref_grid, col + row * MAP_COLS, 0);
                    }
                continue;
            }
        }
#endif
        {
            int c = b[i].x / 16, r = b[i].y / 16;
            for (row = limit(r - 2, 0, MAP_ROWS - 1); row <= limit(r + 2, 0, MAP_ROWS - 1); row++)
                for (col = limit(c - 2, 0, MAP_COLS - 1); col <= limit(c + 2, 0, MAP_COLS - 1); col++)
                    if (dist2(col * 16 + 8, row * 16 + 8, b[i].x, b[i].y) < 18 * 18)
                        SC64_GRID_SET(ref_grid, col + row * MAP_COLS, 0);
        }
    }
}

#include "../starcraft/path_grid_build.h"

/* ---- scenarios ----------------------------------------------------------- */
static u32 rng_state = 0x6d2b79f5u;
static u32 rnd(void)
{
    rng_state ^= rng_state << 13; rng_state ^= rng_state >> 17; rng_state ^= rng_state << 5;
    return rng_state;
}
static int rn(int n) { return (int)(rnd() % (u32)n); }

static long checks, mismatches;
static void compare(const char *what, long scenario, long step)
{
    int cells = race_cols * race_rows, i;
    ++checks;
    for (i = 0; i < cells; ++i)
        if (SC64_GRID_GET(path_walkable, i) != SC64_GRID_GET(ref_grid, i)) {
            if (mismatches < 5)
                fprintf(stderr, "MISMATCH %s scenario %ld step %ld cell (%d,%d) got %d want %d\n",
                        what, scenario, step, i % race_cols, i / race_cols,
                        (int)SC64_GRID_GET(path_walkable, i), (int)SC64_GRID_GET(ref_grid, i));
            ++mismatches;
            return;
        }
}

static int rand_coord(int extent)
{
    int k = rn(40);
    if (k == 0) return -rn(60);
    if (k == 1) return extent + rn(60);
    if (k == 2) return rn(8) * 16;            /* on a cell border */
    if (k == 3) return extent - 1 - rn(4);
    return rn(extent);
}
static void place(int i)
{
    b[i].alive = 1;
    b[i].type = rn(16);
    b[i].x = rand_coord(race_cols * 16);
    b[i].y = rand_coord(race_rows * 16);
    collid_off[i] = (u8)(rn(8) == 0);
    airborne[i] = (u8)(rn(10) == 0);
    door_closed[i] = (u8)rn(2);
}
static void mutate(int step, int burst)
{
    int k, n = burst ? 20 + rn(40) : 1 + rn(3);
    (void)step;
    for (k = 0; k < n; ++k) {
        int i = rn(MAX_BUILDINGS);
        switch (rn(9)) {
        case 0: case 1: place(i); break;
        case 2: b[i].alive = 0; break;
        case 3: if (b[i].alive) { b[i].x += rn(48) - 24; b[i].y += rn(48) - 24; } break;
        case 4: if (b[i].alive) b[i].type = rn(16); break;
        case 5: collid_off[i] ^= 1; break;
        case 6: airborne[i] ^= 1; break;
        case 7: door_closed[i] ^= 1; break;
        default:
            if (rn(6) == 0) t4_active ^= 1; else if (rn(6) == 0) z5_active ^= 1;
            else if (b[i].alive) { b[i].x = rn(64) * 16 + rn(16); b[i].y = rn(64) * 16 + rn(16); }
            break;
        }
    }
    if (rn(400) == 0) { int i = rn(MAX_BUILDINGS); place(i); b[i].x = 40000 + rn(100); }
    if (rn(400) == 0) { int i = rn(MAX_BUILDINGS); place(i); b[i].y = -40000 - rn(100); }
    if (rn(300) == 0) { int i = rn(MAX_BUILDINGS); place(i); b[i].type = 256 + rn(8); }
}

int main(void)
{
    static const int dims[][2] = { {64, 64}, {128, 128}, {96, 80}, {32, 48}, {100, 120}, {40, 40}, {128, 64} };
    long scenario, step, steps = 0;
    unsigned seeds = 400;
    for (scenario = 0; scenario < (long)seeds; ++scenario) {
        int d = rn((int)(sizeof dims / sizeof dims[0]));
        memset(b, 0, sizeof b); memset(collid_off, 0, sizeof collid_off);
        memset(airborne, 0, sizeof airborne); memset(door_closed, 0, sizeof door_closed);
        /* The grid is clobbered between games, as a map load would; the game
         * never reuses a grid across that, so the recorded state goes too. */
        memset(path_walkable, 0xA5, sizeof path_walkable);
#ifdef SC64_PATH_GRID_INCREMENTAL
        path_fp_valid = 0;
#endif
        race_cols = dims[d][0]; race_rows = dims[d][1];
        race_mode = rn(2); race_selected_mission = rn(10); race_skirmish_map = rn(3);
        sc64_showcase_scene = rn(5) == 0 ? rn(3) : -1;
        t4_active = rn(2); z5_active = rn(2);
        mock_terrain_fill();
        { int k, n = rn(MAX_BUILDINGS); for (k = 0; k < n; ++k) place(rn(MAX_BUILDINGS)); }
        for (step = 0; step < 120; ++step, ++steps) {
            int what = rn(100);
            if (what < 4) {            /* new map or mission: terrain identity changes */
                int d2 = rn((int)(sizeof dims / sizeof dims[0]));
                race_cols = dims[d2][0]; race_rows = dims[d2][1];
                race_mode = rn(2); race_selected_mission = rn(10); race_skirmish_map = rn(3);
                sc64_showcase_scene = rn(4) == 0 ? rn(3) : -1;
                mock_terrain_fill();
            } else if (what < 8) {     /* many changes at once */
                mutate((int)step, 1);
            } else if (what < 20) {    /* nothing changed */
            } else {
                mutate((int)step, 0);
            }
            path_grid_valid = 0;
            build_path_grid();
            assert(path_grid_valid);
            reference_full();
            compare("rebuild", scenario, step);
        }
    }
    printf("path grid: %ld rebuilds checked, %ld mismatches; full %u, incremental %u, unchanged %u\n",
           checks, mismatches, path_grid_stat_full, path_grid_stat_inc, path_grid_stat_same);
#ifdef SC64_PATH_GRID_INCREMENTAL
    assert(path_grid_stat_inc > 1000 && path_grid_stat_same > 100 && path_grid_stat_full > 100);
#else
    assert(path_grid_stat_inc == 0);
#endif
    return mismatches ? 1 : 0;
}
