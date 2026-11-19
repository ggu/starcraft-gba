/* SPDX-License-Identifier: GPL-3.0-or-later */
/* One side of the SC64_TERRAIN_FAST_VALIDATE differential test: a small
 * world with the game's static-object and terrain-cache headers. The make
 * rule compiles this file once as the reference (FV_SIDE_FAST undefined, flag
 * off) and once as the fast side (FV_SIDE_FAST and the flag defined). The
 * tile loop mirrors draw_map() in starcraft/sc64_draw_map.h for each side. */
#include <assert.h>
#include <string.h>
#include "../common/gba.h"
#include "sc64_terrain_fast_validate.h"

#if defined(FV_SIDE_FAST) != defined(SC64_TERRAIN_FAST_VALIDATE)
#error "the fast side, and only it, is built with SC64_TERRAIN_FAST_VALIDATE"
#endif

#define MAX_MINERALS FV_MAX_MINERALS
#define MAX_RACE_GEYSERS FV_MAX_GEYSERS
#define MAX_BUILDINGS FV_MAX_BUILDINGS
#define SIDE 0
static int race_cols = 64, race_rows = 64;
#define MAP_COLS race_cols
#define MAP_ROWS race_rows
#define MAP_CELLS (FV_MAP_MAX * FV_MAP_MAX)
#define WORLD_W (MAP_COLS * 16)
#define WORLD_H (MAP_ROWS * 16)

#ifdef FV_PACKED_GRID
static u8 visible[(MAP_CELLS + 7) / 8], explored[(MAP_CELLS + 7) / 8];
#define SC64_GRID_GET(grid, cell) \
    (((grid)[(unsigned)(cell) >> 3] >> ((unsigned)(cell) & 7u)) & 1u)
#else
static u8 visible[MAP_CELLS], explored[MAP_CELLS];
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#endif

typedef struct { int x, y, amount, style; } Mineral;
typedef struct { int x, y; } Geyser;
typedef struct { int x, y, type, owner, alive, under_construction, raw_type; } Building;
static Mineral m[MAX_MINERALS];
static Geyser geysers[MAX_RACE_GEYSERS];
static int geyser_count;
static Building b[MAX_BUILDINGS];
static int camx, camy;
#ifdef SC64_RACES
static int race_mode, race_selected_mission;
#else
#define SC64_TERRAN_CAMPAIGN 1
static int campaign_mission;
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
static struct { int doodad_slot[32], doodad_enabled[32]; } race_t4;
#endif
static int fv_doodad[FV_DOODADS][2];
#ifdef SC64_Z5_RUNTIME
#define SC64_Z5_UNIT_BACKED_DOODADS 7
static int race_z5_prop_cache_field(unsigned i, int field)
{ return fv_doodad[i][field]; }
#endif
#ifdef SC64_P6_RUNTIME
#define SC64_P6_DOODAD_SOURCE_COUNT 5u
static unsigned race_p6_prop_cache_field(unsigned row, int enabled)
{ return (unsigned)fv_doodad[16 + row][enabled]; }
#endif

/* Copies of the game.c helpers the static key reads. */
static int
currently_visible(int x, int y)
{
    int col = x / 16, row = y / 16;
    if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS)
        return 0;
    return SC64_GRID_GET(visible, col + row * MAP_COLS);
}

static int
geyser_in_view(int x, int y)
{
    int left = x - 32 + 3, top = y - 24 + 2;
    return left < 240 && left + 58 > 0 && top < 120 && top + 40 > 12;
}

static int
building_art_in_view(const Building *q, int x, int y)
{
#ifdef SC64_Z9_RUNTIME
    if (q->raw_type == 173 || q->raw_type == 174)
        return x > -56 && x < 296 && y > -28 && y < 160;
#else
    (void)q;
#endif
    return x >= -20 && x <= 260 && y >= 2 && y <= 130;
}

#ifdef FV_SIDE_FAST
static FvModes *fv_modes;
#endif
#include "../starcraft/sc64_static_key.h"
#ifndef SC64_RENDER_KEY_ONCE
#define SC64_TERRAIN_STATIC_KEY sc64_static_objects_key()
#endif
#include "../starcraft/sc64_terrain_cache.h"

static int fv_limit(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static void
fv_load(const FvWorld *w)
{
    int cell;
    race_cols = w->cols;
    race_rows = w->rows;
#ifdef SC64_RACES
    race_mode = w->race_mode;
    race_selected_mission = w->mission;
#else
    campaign_mission = w->mission;
#endif
    camx = w->camx;
    camy = w->camy;
    memset(visible, 0, sizeof(visible));
    memset(explored, 0, sizeof(explored));
    for (cell = 0; cell < w->cols * w->rows; ++cell) {
#ifdef FV_PACKED_GRID
        if (w->fog[cell] & 1) visible[cell >> 3] |= (u8)(1u << (cell & 7));
        if (w->fog[cell] & 2) explored[cell >> 3] |= (u8)(1u << (cell & 7));
#else
        visible[cell] = (u8)(w->fog[cell] & 1);
        explored[cell] = (u8)((w->fog[cell] >> 1) & 1);
#endif
    }
    for (int i = 0; i < MAX_MINERALS; ++i) {
        m[i].x = w->mineral_x[i];
        m[i].y = w->mineral_y[i];
        m[i].amount = w->mineral_amount[i];
        m[i].style = w->mineral_style[i];
    }
    for (int i = 0; i < MAX_RACE_GEYSERS; ++i) {
        geysers[i].x = w->geyser_x[i];
        geysers[i].y = w->geyser_y[i];
    }
    geyser_count = w->geyser_count;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        b[i].x = w->building_x[i];
        b[i].y = w->building_y[i];
        b[i].type = w->building_type[i];
        b[i].owner = w->building_owner[i];
        b[i].alive = w->building_alive[i];
        b[i].under_construction = w->building_construction[i];
        b[i].raw_type = w->building_raw[i];
    }
    memcpy(fv_doodad, w->doodad, sizeof(fv_doodad));
#ifdef SC64_T4_SCENARIO_RUNTIME
    for (int i = 0; i < 32; ++i) {
        race_t4.doodad_slot[i] = w->doodad[i][0];
        race_t4.doodad_enabled[i] = w->doodad[i][1];
    }
#endif
}

#ifdef FV_SIDE_FAST
static void
fv_count_mode(const Sc64TerrainTiles *t, int probe)
{
    if (!fv_modes) return;
    if (t->clean_mode == SC64_TERRAIN_TILES_INDEXED) ++*(probe ? &fv_modes->probe_indexed : &fv_modes->indexed);
    else if (t->clean_mode == SC64_TERRAIN_TILES_CALL) ++*(probe ? &fv_modes->probe_call : &fv_modes->call);
    else ++*(probe ? &fv_modes->probe_none : &fv_modes->none);
}
#endif

/* The tile answers draw_map() uses, for one window. */
static void
fv_tiles(int col0, int row0, int col1, int row1, u8 *clean, u8 *part, u16 *state, int probe)
{
    int k = 0;
#ifdef FV_SIDE_FAST
    Sc64TerrainTiles tiles;
    int tile = -1;
    sc64_terrain_tiles_begin(&tiles, col0, row0, col1, row1);
    fv_count_mode(&tiles, probe);
#else
    (void)probe;
#endif
    for (int row = row0; row < row1; ++row)
        for (int col = col0; col < col1; ++col, ++k) {
            int is_clean;
            if (k >= FV_CELLS) return;
#ifdef FV_SIDE_FAST
            ++tile;
            is_clean = sc64_terrain_tiles_clean(&tiles, tile, col, row);
#else
            is_clean = sc64_terrain_tile_clean(col, row);
#endif
            clean[k] = (u8)(1 + is_clean);
            if (is_clean) continue;
#ifdef SC64_SOFT_FOG
#ifdef FV_SIDE_FAST
            part[k] = (u8)sc64_terrain_tiles_partial(&tiles, tile, col, row);
            state[k] = sc64_terrain_tiles_state(&tiles, tile, col, row);
#else
            part[k] = (u8)sc64_terrain_tile_partial(col, row);
            state[k] = sc64_terrain_state_view(col, row);
#endif
#else
            (void)part; (void)state;
#endif
        }
}

static void
fv_frame(const FvWorld *w, FvRecord *r)
{
    int col0, row0, col1, row1;
    unsigned key;
    memset(r, 0, sizeof(*r));
    fv_load(w);
    gfx_begin(0);
    key = sc64_static_objects_key();
    r->key = key;
    col0 = fv_limit(camx / 16, 0, MAP_COLS - 1);
    col1 = fv_limit((camx + 240) / 16 + 1, 0, MAP_COLS);
    row0 = fv_limit(camy / 16, 0, MAP_ROWS - 1);
    row1 = fv_limit((camy + 108) / 16 + 1, 0, MAP_ROWS);
    r->window_col0 = col0; r->window_row0 = row0;
    r->window_col1 = col1; r->window_row1 = row1;
#ifdef SC64_RENDER_KEY_ONCE
    r->hit = sc64_terrain_cached_key(camx, camy, key);
#else
    /* The key-per-call API recomputes the key: a second call with no world
     * change, as in the game. */
    r->hit = sc64_terrain_cached(camx, camy);
#endif
    r->reused = sc64_terrain_cache_reused_base;
    if (!r->hit) {
#ifdef SC64_SOFT_FOG
        sc64_terrain_prepare(camx, camy);
#endif
        fv_tiles(col0, row0, col1, row1, r->clean, r->part, r->state, 0);
        fv_tiles(w->probe_col0, w->probe_row0, w->probe_col1, w->probe_row1,
                 r->probe_clean, r->probe_part, r->probe_state, 1);
        if (r->reused) {
            memcpy(r->dirty, sc64_terrain_cache_dirty, sizeof(r->dirty));
#ifdef SC64_SOFT_FOG
            if (sc64_terrain_part_active)
                memcpy(r->part_cells, sc64_terrain_part, sizeof(r->part_cells));
#endif
        }
        /* Cells of the band change only where tiles are drawn; that does not
         * affect any decision, so no pixels are written here. */
#ifdef SC64_RENDER_KEY_ONCE
        r->stored = sc64_terrain_store_key(camx, camy, key);
#else
        r->stored = sc64_terrain_store(camx, camy);
#endif
    }
    r->valid = sc64_terrain_cache_valid;
    r->mask_count = sc64_terrain_cache_mask_count;
    r->cache_camx = sc64_terrain_cache_camx;
    r->cache_camy = sc64_terrain_cache_camy;
    if (r->valid)
        for (int i = 0; i < r->mask_count && i < FV_CELLS; ++i) r->mask[i] = sc64_terrain_cache_mask[i];
    r->requests = sc64_terrain_requests;
    r->hits = sc64_terrain_hits;
    r->partial = sc64_terrain_partial;
#ifdef SC64_SOFT_FOG
    r->now_valid = sc64_terrain_now_valid;
    r->now_col0 = sc64_terrain_now_col0;
    r->now_row0 = sc64_terrain_now_row0;
    r->now_cols = sc64_terrain_now_cols;
    r->now_rows = sc64_terrain_now_rows;
    memcpy(r->now, sc64_terrain_now, sizeof(r->now));
    for (int i = 0; i < SC64_TERRAIN_FOG_ROWS + 2; ++i) {
        r->fp[0][i] = sc64_terrain_fp_known[i];
        r->fp[1][i] = sc64_terrain_fp_seen[i];
    }
#endif
#ifdef SC64_DIAGNOSTIC_TERRAIN
    r->diag[0] = sc64_terrain_diag_invalid_cache;
    r->diag[1] = sc64_terrain_diag_mission_change;
    r->diag[2] = sc64_terrain_diag_static_key_change;
    r->diag[3] = sc64_terrain_diag_camera_oversize;
    r->diag[4] = sc64_terrain_diag_camera_change;
    r->diag[5] = sc64_terrain_diag_invalid_view;
    r->diag[6] = sc64_terrain_diag_invalid_mask;
    r->diag[7] = sc64_terrain_diag_blit_failure;
    r->diag[8] = sc64_terrain_diag_capture_failure;
    r->diag[9] = sc64_terrain_diag_reused_pixels;
    r->diag[10] = sc64_terrain_diag_captured_pixels;
    r->diag[11] = sc64_terrain_diag_captured_tiles;
    r->diag[12] = sc64_terrain_diag_tiles_checked;
    r->diag[13] = sc64_terrain_diag_dirty_tiles;
#endif
}

#ifdef FV_SIDE_FAST
void
fv_fast_frame(const FvWorld *w, FvRecord *r, FvModes *modes)
{
#if defined(SC64_SOFT_FOG) && !defined(SC64_SOFT_FOG_SHADE_ONLY)
    /* The two-column table is the single-column table twice over. */
    for (unsigned i = 0; i < 256; ++i) {
        unsigned k = i & 15u, v = i >> 4;
        unsigned lo = sc64_terrain_fog_triple[(k & 7u) | ((v & 7u) << 3)];
        unsigned hi = sc64_terrain_fog_triple[((k >> 1) & 7u) | (((v >> 1) & 7u) << 3)];
        const u8 *bytes = (const u8 *)&sc64_terrain_fog_triple2[i];
        assert(sc64_terrain_fog_triple2[i] == (lo | hi << 8));
        assert(bytes[0] == lo && bytes[1] == hi);
    }
#endif
    fv_modes = modes;
    fv_frame(w, r);
    fv_modes = 0;
}
#else
void
fv_ref_frame(const FvWorld *w, FvRecord *r)
{
    fv_frame(w, r);
}
#endif
