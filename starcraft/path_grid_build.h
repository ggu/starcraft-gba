/* Walkability grid construction, with an incremental update after structure
 * changes. Included by game.c (and by tests/sc64_path_grid_incremental.c with
 * stand-ins for the game's data) at the point where build_path_grid used to be.
 *
 * path_grid_build_full is the original whole-grid rebuild, unchanged: terrain,
 * then the footprint of every collidable building, each footprint only ever
 * clearing cells. A cell is therefore walkable exactly when its terrain is open
 * and no building footprint covers it, independent of building order.
 *
 * With SC64_PATH_GRID_INCREMENTAL the build records, per building slot, the
 * footprint kind it applied and the inputs that determine it (position and
 * type), together with the terrain identity. The next build derives the same
 * description for every slot from the current state. Slots whose description is
 * unchanged cover the same cells as before, and the terrain is unchanged, so
 * only cells in the old or new footprint of a changed slot can differ. Those
 * rectangles are reset to terrain and every footprint is re-applied clipped to
 * them. Anything else (first build, a different map, scene or mission, too
 * many changed slots, a coordinate outside 16 bits) takes the full path. With
 * SC64_PATH_GRID_VERIFY every incremental result is compared with a full
 * rebuild and the program aborts on any difference. */
#ifndef STARCRAFT_PATH_GRID_BUILD_H
#define STARCRAFT_PATH_GRID_BUILD_H

#if defined(SC64_PATH_GRID_INCREMENTAL) && !defined(SC64_RACES)
#undef SC64_PATH_GRID_INCREMENTAL /* terrain identity is only known for race maps */
#endif

#ifdef SC64_PATH_GRID_STATS
static unsigned path_grid_stat_full, path_grid_stat_inc, path_grid_stat_same;
#endif

#ifdef SC64_PATH_GRID_INCREMENTAL
enum { PFP_NONE = 0, PFP_CIRCLE, PFP_T4_DOOR, PFP_DOOR, PFP_SOURCE };
enum { PATH_GRID_MAX_DIRTY = 24 };
typedef struct { s16 x, y; u16 type; u8 kind, pad; } PathFootprint;
typedef struct { int cols, rows, mode, mission, skirmish, scene; } PathTerrainKey;
typedef struct { u8 c0, c1, r0, r1; } PathDirtyRect;
static PathFootprint path_fp[MAX_BUILDINGS];
static PathTerrainKey path_fp_key;
static u8 path_fp_valid;

static void __attribute__((noinline))
path_terrain_key(PathTerrainKey *k)
{
    k->cols = MAP_COLS; k->rows = MAP_ROWS;
    k->mode = race_mode; k->mission = race_selected_mission;
#ifdef SC64_IMPORTED_SKIRMISH
    k->skirmish = race_skirmish_map;
#else
    k->skirmish = 0;
#endif
#ifdef SC64_SHOWCASE_ENABLED
    k->scene = sc64_showcase_scene;
#else
    k->scene = -1;
#endif
}

/* The footprint build_path_grid applies for slot i, in the order of its tests. */
static void __attribute__((noinline))
path_fp_describe(int i, PathFootprint *f)
{
    int left, up, right, down;
    f->kind = PFP_NONE; f->pad = 0;
    f->x = (s16)b[i].x; f->y = (s16)b[i].y; f->type = (u16)b[i].type;
    if (!b[i].alive) return;
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (race_bc_facility_airborne(i)) return;
#endif
    if (b[i].type == B_ZERG_BEACON) return;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (!sc64_t4_building_collidable(i)) return;
#ifdef SC64_T4_RUNTIME
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (race_t4_active() && sc64_race_t4_is_door(b[i].type)) {
        if (sc64_race_t4_building_bounds_source(b[i].type, &left, &up, &right, &down))
            f->kind = PFP_T4_DOOR;
        return;
    }
#endif
#endif
#ifdef SC64_Z5_RUNTIME
    if (race_z5_active() && race_z5_is_door_type(b[i].type)) {
        if (race_z5_building_collidable(i) &&
            sc64_game_building_bounds_source(b[i].type, &left, &up, &right, &down))
            f->kind = PFP_DOOR;
        return;
    }
#endif
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    if (sc64_game_building_bounds_source(b[i].type, &left, &up, &right, &down)) {
        f->kind = PFP_SOURCE;
        return;
    }
#endif
    (void)left; (void)up; (void)right; (void)down;
    f->kind = PFP_CIRCLE;
}

/* Cell rectangle covered by a footprint (the loop bounds of the full build),
 * and for source footprints the world rectangle the cells are tested against. */
static void __attribute__((noinline))
path_fp_rect(const PathFootprint *f, int *c0, int *c1, int *r0, int *r1, int *world)
{
    int left = 0, up = 0, right = 0, down = 0;
    if (f->kind == PFP_CIRCLE) {
        int c = f->x / 16, r = f->y / 16;
        *c0 = limit(c - 2, 0, MAP_COLS - 1); *c1 = limit(c + 2, 0, MAP_COLS - 1);
        *r0 = limit(r - 2, 0, MAP_ROWS - 1); *r1 = limit(r + 2, 0, MAP_ROWS - 1);
        return;
    }
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (f->kind == PFP_T4_DOOR)
        (void)sc64_race_t4_building_bounds_source(f->type, &left, &up, &right, &down);
    else
#endif
#if defined(SC64_Z5_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        (void)sc64_game_building_bounds_source(f->type, &left, &up, &right, &down);
#else
        (void)0;
#endif
    world[0] = f->x * 2 - left; world[1] = f->x * 2 + right;
    world[2] = f->y * 2 - up;   world[3] = f->y * 2 + down;
    *c0 = limit(world[0] / 32, 0, MAP_COLS - 1); *c1 = limit(world[1] / 32, 0, MAP_COLS - 1);
    *r0 = limit(world[2] / 32, 0, MAP_ROWS - 1); *r1 = limit(world[3] / 32, 0, MAP_ROWS - 1);
}

/* Clear this footprint's cells inside the column/row window [x0,x1] x [y0,y1]. */
static void __attribute__((noinline))
path_fp_clear(const PathFootprint *f, int x0, int x1, int y0, int y1)
{
    int c0, c1, r0, r1, row, col, world[4] = { 0, 0, 0, 0 };
    if (f->kind == PFP_NONE) return;
    path_fp_rect(f, &c0, &c1, &r0, &r1, world);
    if (c0 < x0) c0 = x0;
    if (c1 > x1) c1 = x1;
    if (r0 < y0) r0 = y0;
    if (r1 > y1) r1 = y1;
    for (row = r0; row <= r1; ++row)
        for (col = c0; col <= c1; ++col) {
            if (f->kind == PFP_CIRCLE) {
                if (dist2(col * 16 + 8, row * 16 + 8, f->x, f->y) >= 18 * 18) continue;
            } else if (f->kind == PFP_SOURCE) {
                int cx0 = col * 32, cy0 = row * 32;
                if (!(cx0 <= world[1] && cx0 + 32 >= world[0] &&
                      cy0 <= world[3] && cy0 + 32 >= world[2])) continue;
            }
            SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 0);
        }
}

static void __attribute__((noinline))
path_grid_record_state(void)
{
    int i;
    path_terrain_key(&path_fp_key);
    path_fp_valid = 1;
    for (i = 0; i < MAX_BUILDINGS; ++i) {
        path_fp_describe(i, &path_fp[i]);
        /* A coordinate the record cannot hold exactly disables the update. */
        if (b[i].x != (s16)b[i].x || b[i].y != (s16)b[i].y ||
            b[i].type != (int)(u16)b[i].type) path_fp_valid = 0;
    }
}

static int
path_fp_same(const PathFootprint *a, const PathFootprint *c)
{
    if (a->kind != c->kind) return 0;
    if (a->kind == PFP_NONE) return 1;
    return a->x == c->x && a->y == c->y && a->type == c->type;
}

static void __attribute__((noinline))
path_dirty_add(PathDirtyRect *d, int *n, const PathFootprint *f)
{
    int c0, c1, r0, r1, world[4];
    if (f->kind == PFP_NONE) return;
    path_fp_rect(f, &c0, &c1, &r0, &r1, world);
    if (*n < PATH_GRID_MAX_DIRTY) {
        d[*n].c0 = (u8)c0; d[*n].c1 = (u8)c1; d[*n].r0 = (u8)r0; d[*n].r1 = (u8)r1;
    }
    ++*n;
}

/* Returns 1 when path_walkable has been brought up to date incrementally. */
static int __attribute__((noinline))
path_grid_update(void)
{
    PathDirtyRect dirty[PATH_GRID_MAX_DIRTY];
    PathFootprint now;
    PathTerrainKey key;
    int i, k, n = 0, row, col;
    if (!path_fp_valid) return 0;
    path_terrain_key(&key);
    if (memcmp(&key, &path_fp_key, sizeof key) != 0) return 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) {
        if (b[i].x != (s16)b[i].x || b[i].y != (s16)b[i].y ||
            b[i].type != (int)(u16)b[i].type) { path_fp_valid = 0; return 0; }
        path_fp_describe(i, &now);
        if (path_fp_same(&now, &path_fp[i])) continue;
        path_dirty_add(dirty, &n, &path_fp[i]);
        path_dirty_add(dirty, &n, &now);
        path_fp[i] = now;
        if (n > PATH_GRID_MAX_DIRTY) { path_fp_valid = 0; return 0; }
    }
#ifdef SC64_PATH_GRID_STATS
    if (n) ++path_grid_stat_inc; else ++path_grid_stat_same;
#endif
    for (k = 0; k < n; ++k)
        for (row = dirty[k].r0; row <= dirty[k].r1; ++row)
            for (col = dirty[k].c0; col <= dirty[k].c1; ++col)
                SC64_GRID_SET(path_walkable, col + row * MAP_COLS, !terrain_blocked(col, row));
    if (n)
        for (i = 0; i < MAX_BUILDINGS; ++i)
            for (k = 0; k < n; ++k)
                path_fp_clear(&path_fp[i], dirty[k].c0, dirty[k].c1, dirty[k].r0, dirty[k].r1);
    return 1;
}
#endif /* SC64_PATH_GRID_INCREMENTAL */

#if (defined(SC64_T4_RUNTIME) && defined(SC64_VISIBILITY_SPANS)) || defined(SC64_ZP_CAMPAIGN_STORAGE)
/* Rebuilds follow map/door changes. Keep the larger span visibility path in
 * IWRAM and this occasional full-grid rebuild in ROM within the same reserve. */
static void
#else
static void GBA_HOT
#endif
path_grid_build_full(void)
{
    int col, row, i;
#if defined(SC64_BULK_TERRAIN_GRID) && defined(SC64_RACES) && \
    defined(SC64_IMPORTED_MISSION) && \
    (defined(SC64_LARGE_MAPS) || defined(SC64_ZP_CAMPAIGN_STORAGE))
    const u8 *base_blocked = sc64_base_blocked_map();
    if (base_blocked) sc64_terrain_grid_pack(path_walkable, base_blocked, (unsigned)MAP_COLS * (unsigned)MAP_ROWS);
    else
#endif
    for (row = 0; row < MAP_ROWS; row++)
	for (col = 0; col < MAP_COLS; col++) {
	    int cell = col + row * MAP_COLS;
	    SC64_GRID_SET(path_walkable, cell, !terrain_blocked(col, row));
	}
    for (i = 0; i < MAX_BUILDINGS; i++) if (b[i].alive) {
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
        if (race_bc_facility_airborne(i)) continue;
#endif
#ifdef SC64_RACES
        if(b[i].type==B_ZERG_BEACON)continue;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (!sc64_t4_building_collidable(i)) continue;
#ifdef SC64_T4_RUNTIME
#ifdef SC64_T4_SCENARIO_RUNTIME
        if (race_t4_active() && sc64_race_t4_is_door(b[i].type)) {
            /* Conservative tile overlap at the GBA's half scale. Opening
             * restores the retained terrain, rather than carving new floor. */
            race_t4_block_door_grid(i);
            continue;
        }
#endif
#endif
#ifdef SC64_Z5_RUNTIME
        if (race_z5_active() && race_z5_is_door_type(b[i].type)) {
            if (race_z5_building_collidable(i)) {
                int left, up, right, down;
                if (sc64_game_building_bounds_source(b[i].type, &left, &up, &right, &down)) {
                    int c0=limit((b[i].x*2-left)/32,0,MAP_COLS-1);
                    int c1=limit((b[i].x*2+right)/32,0,MAP_COLS-1);
                    int r0=limit((b[i].y*2-up)/32,0,MAP_ROWS-1);
                    int r1=limit((b[i].y*2+down)/32,0,MAP_ROWS-1);
                    for(row=r0;row<=r1;row++) for(col=c0;col<=c1;col++)
                        SC64_GRID_SET(path_walkable,col+row*MAP_COLS,0);
                }
            }
            continue;
        }
#endif
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
                        if (cell_x0 <= x1 && cell_x1 >= x0 &&
                            cell_y0 <= y1 && cell_y1 >= y0)
                            SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 0);
                    }
                continue;
            }
        }
#endif
        int c = b[i].x / 16, r = b[i].y / 16;
        for (row = limit(r - 2, 0, MAP_ROWS - 1); row <= limit(r + 2, 0, MAP_ROWS - 1); row++)
            for (col = limit(c - 2, 0, MAP_COLS - 1); col <= limit(c + 2, 0, MAP_COLS - 1); col++)
                if (dist2(col * 16 + 8, row * 16 + 8, b[i].x, b[i].y) < 18 * 18)
                    SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 0);
    }
#ifdef SC64_PATH_GRID_INCREMENTAL
    path_grid_record_state();
#endif
}

#if defined(SC64_PATH_GRID_VERIFY) && defined(SC64_PATH_GRID_INCREMENTAL)
#include <stdio.h>
static u8 path_grid_verify_copy[SC64_GRID_STORAGE_BYTES], path_grid_verify_prev[SC64_GRID_STORAGE_BYTES];
static int path_grid_verify_have;
static void
path_grid_verify_fail(const PathFootprint *kept)
{
    int i, cells = MAP_COLS * MAP_ROWS;
    for (i = 0; i < cells; ++i)
        if (SC64_GRID_GET(path_grid_verify_copy, i) != SC64_GRID_GET(path_walkable, i)) {
            fprintf(stderr, "path grid verify: cell %d,%d incremental %d full %d\n", i % MAP_COLS,
                    i / MAP_COLS, (int)SC64_GRID_GET(path_grid_verify_copy, i),
                    (int)SC64_GRID_GET(path_walkable, i));
            break;
        }
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (!path_fp_same(&kept[i], &path_fp[i]))
            fprintf(stderr, "path grid verify: slot %d kept kind %d (%d,%d) type %d, full kind %d (%d,%d) type %d\n",
                    i, kept[i].kind, kept[i].x, kept[i].y, kept[i].type,
                    path_fp[i].kind, path_fp[i].x, path_fp[i].y, path_fp[i].type);
    for (i = 0; i < MAX_BUILDINGS; ++i) {
        int c0, c1, r0, r1, w[4] = { 0, 0, 0, 0 };
        if (path_fp[i].kind == PFP_NONE) continue;
        path_fp_rect(&path_fp[i], &c0, &c1, &r0, &r1, w);
        fprintf(stderr, "path grid verify: slot %d kind %d at %d,%d type %d cells %d-%d x %d-%d\n", i,
                path_fp[i].kind, path_fp[i].x, path_fp[i].y, path_fp[i].type, c0, c1, r0, r1);
    }
    __builtin_trap();
}
#endif

/* Not inlined: hot callers keep one cold call, as before the update existed. */
static void __attribute__((noinline))
build_path_grid(void)
{
#ifdef SC64_ROUTE_KEEP_FIELDS
    sc64_routes_grid_begin();
#endif
#ifdef SC64_PATH_GRID_INCREMENTAL
#ifdef SC64_PATH_GRID_VERIFY
    if (path_grid_verify_have && memcmp(path_grid_verify_prev, path_walkable, sizeof path_walkable) != 0) {
        int c;
        for (c = 0; c < MAP_COLS * MAP_ROWS; ++c)
            if (SC64_GRID_GET(path_grid_verify_prev, c) != SC64_GRID_GET(path_walkable, c)) break;
        fprintf(stderr, "path grid verify: grid changed outside a rebuild, first cell %d,%d was %d now %d\n",
                c % MAP_COLS, c / MAP_COLS, (int)SC64_GRID_GET(path_grid_verify_prev, c),
                (int)SC64_GRID_GET(path_walkable, c));
    }
#endif
    if (path_grid_update()) {
#ifdef SC64_PATH_GRID_VERIFY
        PathFootprint kept[MAX_BUILDINGS];
        memcpy(path_grid_verify_copy, path_walkable, sizeof path_walkable);
        memcpy(kept, path_fp, sizeof kept);
        path_grid_build_full();
        {
            int k;
            for (k = 0; k < MAX_BUILDINGS; ++k)
                if (!path_fp_same(&kept[k], &path_fp[k])) path_grid_verify_fail(kept);
        }
        if (memcmp(path_grid_verify_copy, path_walkable, sizeof path_walkable) != 0)
            path_grid_verify_fail(kept);
#endif
    } else
#endif
    {
#ifdef SC64_PATH_GRID_STATS
        ++path_grid_stat_full;
#endif
        path_grid_build_full();
    }
    path_grid_valid = 1;
#ifdef SC64_ROUTE_KEEP_FIELDS
    sc64_routes_grid_commit();
#endif
#if defined(SC64_PATH_GRID_VERIFY) && defined(SC64_PATH_GRID_INCREMENTAL)
    memcpy(path_grid_verify_prev, path_walkable, sizeof path_walkable);
    path_grid_verify_have = 1;
#endif
}

#endif
