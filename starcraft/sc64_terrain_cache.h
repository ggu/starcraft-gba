#ifndef SC64_TERRAIN_CACHE_INCLUDED
#define SC64_TERRAIN_CACHE_INCLUDED

#if defined(SC64_RENDER_KEY_ONCE) && !defined(SC64_IMPORTED_MISSION)
#error "SC64_RENDER_KEY_ONCE requires SC64_IMPORTED_MISSION"
#endif

#ifndef SC64_GRID_GET
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_TERRAIN_LOCAL_GRID_GET
#endif

#if defined(SC64_TERRAIN_FAST_VALIDATE) && defined(SC64_TILE_RENDERER)
#error "SC64_TERRAIN_FAST_VALIDATE covers the mode-4 terrain cache, not SC64_TILE_RENDERER"
#endif

#ifdef SC64_TILE_RENDERER
/* The tile renderer keeps terrain in hardware layers. Its mode-4 fallback
 * (skirmish maps) redraws every frame, so no cache storage is reserved. */
#ifndef SC64_TERRAIN_STATIC_KEY
#define SC64_TERRAIN_STATIC_KEY 0u
#endif
static int sc64_terrain_cache_valid, sc64_terrain_cache_reused_base;
static int sc64_terrain_cached(int x, int y) { (void)x; (void)y; return 0; }
static int sc64_terrain_cached_key(int x, int y, unsigned int k) { (void)x; (void)y; (void)k; return 0; }
static int sc64_terrain_tile_clean(int col, int row) { (void)col; (void)row; return 0; }
static int sc64_terrain_store(int x, int y) { (void)x; (void)y; return 0; }
static int sc64_terrain_store_key(int x, int y, unsigned int k) { (void)x; (void)y; (void)k; return 0; }
#else

/* Cache only the static terrain and fog pixels. Dynamic objects are drawn
 * after this band is restored each frame. Keep the buffer word-aligned for
 * the Mode 4 DMA helpers. */
#define SC64_TERRAIN_CACHE_Y 12
#define SC64_TERRAIN_CACHE_HEIGHT 108
#define SC64_TERRAIN_CACHE_CELLS (17 * 9)
#ifdef SC64_TERRAIN_FAST_VALIDATE
/* A cache view starts at x/16 for 0 <= x, so it spans at most 16 columns. */
#define SC64_TERRAIN_FAST_COLS 17
#endif
#ifndef SC64_TERRAIN_STATIC_KEY
#define SC64_TERRAIN_STATIC_KEY 0u
#endif

#ifdef SC64_TERRAIN_CACHE_VRAM
/* The first rows of the saved band live in the OBJ tile VRAM range that
 * sc64_obj_vram.h assigns to the cache: the whole 16 KiB (68 rows) in most
 * products, the part after the P7 projectile arena (48 rows) when that arena
 * is enabled. DMA moves them at VRAM speed; only the remainder occupies
 * EWRAM. */
#include "sc64_obj_vram.h"
#define SC64_TERRAIN_CACHE_VRAM_ROWS ((int)(SC64_OBJ_VRAM_CACHE_BYTES / 240u))
#define SC64_TERRAIN_CACHE_RAM_ROWS \
    (SC64_TERRAIN_CACHE_HEIGHT - SC64_TERRAIN_CACHE_VRAM_ROWS)
_Static_assert(SC64_TERRAIN_CACHE_VRAM_ROWS > 0 &&
               SC64_TERRAIN_CACHE_VRAM_ROWS < SC64_TERRAIN_CACHE_HEIGHT,
               "the VRAM terrain cache needs part, not all, of the band in VRAM");
_Static_assert(SC64_OBJ_VRAM_CACHE_OFFSET + 240u * SC64_TERRAIN_CACHE_VRAM_ROWS <=
               GBA_VRAM_SCRATCH_BYTES, "terrain cache rows exceed the OBJ tile VRAM");
#define sc64_terrain_cache_pixels_vram ((u8 *)SC64_OBJ_VRAM_CACHE)
static u8 sc64_terrain_cache_pixels_ram[240 * SC64_TERRAIN_CACHE_RAM_ROWS]
    __attribute__((aligned(4)));
#else
static u8 sc64_terrain_cache_pixels[240 * SC64_TERRAIN_CACHE_HEIGHT]
    __attribute__((aligned(4)));
#endif
#ifdef SC64_SOFT_FOG
#include "sc64_fog.h"
typedef u16 Sc64TerrainState;
#else
typedef u8 Sc64TerrainState;
#endif
static Sc64TerrainState sc64_terrain_cache_mask[SC64_TERRAIN_CACHE_CELLS];
static u8 sc64_terrain_cache_dirty[SC64_TERRAIN_CACHE_CELLS];

/* A soft edge reads the surrounding fog classes, including cells just
 * outside the viewport. Nine ternary classes fit exactly in 16 bits. */
static inline Sc64TerrainState
sc64_terrain_state(int col, int row)
{
#ifdef SC64_SOFT_FOG
#ifdef SC64_SOFT_FOG_SHADE_ONLY
    return sc64_fog_shade_only(sc64_fog_center_state(
        sc64_fog_signature(visible, explored, MAP_COLS, MAP_ROWS, col, row)));
#else
    return sc64_fog_signature(visible, explored, MAP_COLS, MAP_ROWS, col, row);
#endif
#else
    int cell = col + row * MAP_COLS;
    return (SC64_GRID_GET(visible, cell) ? 1 : 0) |
           (SC64_GRID_GET(explored, cell) ? 2 : 0);
#endif
}
#ifdef SC64_SOFT_FOG
/* The signatures of the view being rendered are computed once per frame, by
 * the cache lookup (or by the draw pass when the lookup did not get that
 * far), and read by the dirty test, the draw pass and the capture. A
 * signature costs nine grid reads, so computing it three times per cell per
 * frame was the dominant cost of the first draft. Here each map row is read
 * as two bit runs and the rows are combined with three table lookups per
 * cell. The array describes the view named by the four fields below. */
#define SC64_TERRAIN_FOG_COLS 17
#define SC64_TERRAIN_FOG_ROWS 9
static u16 sc64_terrain_now[SC64_TERRAIN_CACHE_CELLS];
/* After a scrolled lookup, a dirty cell whose fog is unchanged but which the
 * shifted pixels do not fully cover is redrawn only inside the exposed strips:
 * x in [strip_x0, strip_x1) and y in [strip_y0, strip_y1) on screen. */
static u8 sc64_terrain_part[SC64_TERRAIN_CACHE_CELLS];
static int sc64_terrain_part_active;
static int sc64_terrain_strip_x0, sc64_terrain_strip_x1;
static int sc64_terrain_strip_y0, sc64_terrain_strip_y1;
static int sc64_terrain_now_valid;
static int sc64_terrain_now_col0, sc64_terrain_now_row0;
static int sc64_terrain_now_cols, sc64_terrain_now_rows;

#if defined(SC64_LARGE_MAPS) || defined(SC64_ZP_CAMPAIGN_STORAGE) || \
    defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_FOG_TEST_PACKED)
#define SC64_TERRAIN_PACKED_FOG_GRID 1
#endif

/* Bit i of the result is cell col0+i of the row, for i < width <= 19. */
static unsigned
sc64_terrain_fog_run(const u8 *grid, int row, int col0, int width)
{
#ifdef SC64_TERRAIN_PACKED_FOG_GRID
    unsigned base = (unsigned)(row * MAP_COLS + col0);
    unsigned first = base >> 3;
    unsigned bits;
    if (first + 3u < (unsigned)sizeof(visible)) {
        /* Four bytes cover any run of up to 25 bits; the grids are the same
         * size, and bits past the run are masked off. */
        bits = (unsigned)grid[first] | ((unsigned)grid[first + 1] << 8) |
               ((unsigned)grid[first + 2] << 16) | ((unsigned)grid[first + 3] << 24);
    } else {
        unsigned last = (base + (unsigned)width - 1u) >> 3;
        bits = grid[first];
        if (last > first) bits |= (unsigned)grid[first + 1] << 8;
        if (last > first + 1) bits |= (unsigned)grid[first + 2] << 16;
        if (last > first + 2) bits |= (unsigned)grid[first + 3] << 24;
    }
    return (bits >> (base & 7u)) & ((1u << width) - 1u);
#else
    unsigned bits = 0;
    for (int i = 0; i < width; ++i)
        if (SC64_GRID_GET(grid, row * MAP_COLS + col0 + i)) bits |= 1u << i;
    return bits;
#endif
}

/* One neighbor row's contribution to a signature: the three cells left,
 * centre and right of a column, as base-three digits 1, 3 and 9. The index is
 * the three known bits (bits 0-2) and the three visible bits (bits 3-5);
 * a known cell counts one, a visible one a second. */
static const u8 sc64_terrain_fog_triple[64] = {
     0,  1,  3,  4,  9, 10, 12, 13,  1,  2,  4,  5, 10, 11, 13, 14,
     3,  4,  6,  7, 12, 13, 15, 16,  4,  5,  7,  8, 13, 14, 16, 17,
     9, 10, 12, 13, 18, 19, 21, 22, 10, 11, 13, 14, 19, 20, 22, 23,
    12, 13, 15, 16, 21, 22, 24, 25, 13, 14, 16, 17, 22, 23, 25, 26,
};

/* The fog bits the array was built from: the known and visible words of every
 * halo row of its window. While the window and these words are unchanged the
 * array is still exact, so a scroll inside one cell window, or a frame in which
 * no fog bit changed, skips the combine pass entirely. */
#if defined(SC64_TERRAIN_FAST_VALIDATE) && !defined(SC64_SOFT_FOG_SHADE_ONLY)
/* sc64_terrain_fog_triple for two adjacent columns per lookup. Index bits 0-3
 * are the known bits of four cells, bits 4-7 their visible bits; the low byte
 * is the triple of cells 0-2, the high byte that of cells 1-3. A triple is
 * the sum over its three cells of (known + visible) * 3^position, exactly the
 * single-column table above (the differential test compares every entry).
 * fill_now reads the pairs back as bytes: low byte first (little-endian). */
#define SC64_TERRAIN_T3(k, v) \
    (((k) & 1) + ((v) & 1) + 3 * ((((k) >> 1) & 1) + (((v) >> 1) & 1)) + \
     9 * ((((k) >> 2) & 1) + (((v) >> 2) & 1)))
#define SC64_TERRAIN_T2(i) \
    (SC64_TERRAIN_T3((i) & 15, ((i) >> 4) & 15) | \
     (SC64_TERRAIN_T3(((i) & 15) >> 1, ((i) >> 5) & 7) << 8))
#define SC64_TERRAIN_T2_4(i) SC64_TERRAIN_T2(i), SC64_TERRAIN_T2((i) + 1), \
    SC64_TERRAIN_T2((i) + 2), SC64_TERRAIN_T2((i) + 3)
#define SC64_TERRAIN_T2_16(i) SC64_TERRAIN_T2_4(i), SC64_TERRAIN_T2_4((i) + 4), \
    SC64_TERRAIN_T2_4((i) + 8), SC64_TERRAIN_T2_4((i) + 12)
#define SC64_TERRAIN_T2_64(i) SC64_TERRAIN_T2_16(i), SC64_TERRAIN_T2_16((i) + 16), \
    SC64_TERRAIN_T2_16((i) + 32), SC64_TERRAIN_T2_16((i) + 48)
static const u16 sc64_terrain_fog_triple2[256] = {
    SC64_TERRAIN_T2_64(0), SC64_TERRAIN_T2_64(64),
    SC64_TERRAIN_T2_64(128), SC64_TERRAIN_T2_64(192),
};
#undef SC64_TERRAIN_T2_64
#undef SC64_TERRAIN_T2_16
#undef SC64_TERRAIN_T2_4
#undef SC64_TERRAIN_T2
#undef SC64_TERRAIN_T3
#endif

static u32 sc64_terrain_fp_known[SC64_TERRAIN_FOG_ROWS + 2];
static u32 sc64_terrain_fp_seen[SC64_TERRAIN_FOG_ROWS + 2];
static int sc64_terrain_fp_valid;

static void
sc64_terrain_fill_now(int col0, int row0, int col1, int row1)
{
    u8 triple[SC64_TERRAIN_FOG_ROWS + 2][SC64_TERRAIN_FOG_COLS];
    u32 known_row[SC64_TERRAIN_FOG_ROWS + 2], seen_row[SC64_TERRAIN_FOG_ROWS + 2];
    int cols = col1 - col0, rows = row1 - row0, hr, c, r;
    int same;
    sc64_terrain_now_valid = 0;
    if (cols <= 0 || rows <= 0 || cols > SC64_TERRAIN_FOG_COLS ||
        rows > SC64_TERRAIN_FOG_ROWS) {
        sc64_terrain_fp_valid = 0;
        return;
    }
    same = sc64_terrain_fp_valid && sc64_terrain_now_col0 == col0 &&
           sc64_terrain_now_row0 == row0 && sc64_terrain_now_cols == cols &&
           sc64_terrain_now_rows == rows;
    for (hr = 0; hr < rows + 2; ++hr) {
        int row = row0 - 1 + hr;
        unsigned known = 0, seen = 0;
        if (row >= 0 && row < MAP_ROWS) {
            /* Bit j is column col0 - 1 + j; columns outside the map stay
             * unknown (zero). */
            int first = col0 > 0 ? col0 - 1 : 0;
            int last = col1 < MAP_COLS ? col1 : MAP_COLS - 1;
            unsigned shift = (unsigned)(first - (col0 - 1));
            unsigned v = sc64_terrain_fog_run(visible, row, first, last - first + 1);
            unsigned e = sc64_terrain_fog_run(explored, row, first, last - first + 1);
            known = (v | e) << shift;
            seen = v << shift;
        }
        known_row[hr] = known;
        seen_row[hr] = seen;
        if (same && (known != sc64_terrain_fp_known[hr] || seen != sc64_terrain_fp_seen[hr]))
            same = 0;
    }
    if (same) {
        sc64_terrain_now_valid = 1;
        return;
    }
#if defined(SC64_TERRAIN_FAST_VALIDATE) && !defined(SC64_SOFT_FOG_SHADE_ONLY)
    /* The same triples, two columns per lookup, and the same sums with row
     * pointers. Rows hold 18 bytes so the pair stores stay aligned; an odd
     * last pair also writes the unused triple just past the view. */
    u16 triple_pairs[SC64_TERRAIN_FOG_ROWS + 2][(SC64_TERRAIN_FOG_COLS + 1) / 2];
    (void)triple;
    for (hr = 0; hr < rows + 2; ++hr) {
        unsigned known = known_row[hr], seen = seen_row[hr] << 4;
        u16 *out = triple_pairs[hr];
        sc64_terrain_fp_known[hr] = known_row[hr];
        sc64_terrain_fp_seen[hr] = seen_row[hr];
        for (c = 0; c < cols; c += 2, known >>= 2, seen >>= 2)
            *out++ = sc64_terrain_fog_triple2[(known & 15u) | (seen & 0xf0u)];
    }
    {
        u16 *out = sc64_terrain_now;
        for (r = 0; r < rows; ++r) {
            const u8 *top = (const u8 *)triple_pairs[r];
            const u8 *mid = (const u8 *)triple_pairs[r + 1];
            const u8 *low = (const u8 *)triple_pairs[r + 2];
            for (c = 0; c < cols; ++c)
                *out++ = (u16)(top[c] + 27u * mid[c] + 729u * low[c]);
        }
    }
#else
    for (hr = 0; hr < rows + 2; ++hr) {
        unsigned known = known_row[hr], seen = seen_row[hr];
        sc64_terrain_fp_known[hr] = known;
        sc64_terrain_fp_seen[hr] = seen;
        for (c = 0; c < cols; ++c)
            triple[hr][c] = sc64_terrain_fog_triple[((known >> c) & 7u) |
                                                    (((seen >> c) & 7u) << 3)];
    }
    for (r = 0; r < rows; ++r)
        for (c = 0; c < cols; ++c)
#ifdef SC64_SOFT_FOG_SHADE_ONLY
            sc64_terrain_now[r * cols + c] = sc64_fog_shade_only(
                ((sc64_terrain_fp_known[r + 1] >> (c + 1)) & 1u) +
                ((sc64_terrain_fp_seen[r + 1] >> (c + 1)) & 1u));
#else
            sc64_terrain_now[r * cols + c] =
                (u16)(triple[r][c] + 27u * triple[r + 1][c] + 729u * triple[r + 2][c]);
#endif
#endif
    /* The combine above reads cells beyond the map as unknown; the
     * definition says they take the center's class. Fix the border cells. */
    if (col0 == 0 || col1 == MAP_COLS || row0 == 0 || row1 == MAP_ROWS)
        for (r = 0; r < rows; ++r)
            for (c = 0; c < cols; ++c) {
                int col = col0 + c, row = row0 + r;
                if (col == 0 || col == MAP_COLS - 1 || row == 0 || row == MAP_ROWS - 1)
                    sc64_terrain_now[r * cols + c] = sc64_terrain_state(col, row);
            }
    sc64_terrain_now_col0 = col0;
    sc64_terrain_now_row0 = row0;
    sc64_terrain_now_cols = cols;
    sc64_terrain_now_rows = rows;
    sc64_terrain_now_valid = 1;
    sc64_terrain_fp_valid = 1;
}

#define SC64_TERRAIN_FILL(c0, r0, c1, r1) sc64_terrain_fill_now((c0), (r0), (c1), (r1))
#define SC64_TERRAIN_STATE_AT(i, col, row) ((Sc64TerrainState)sc64_terrain_now[i])
#else
#define SC64_TERRAIN_FILL(c0, r0, c1, r1) ((void)0)
#define SC64_TERRAIN_STATE_AT(i, col, row) sc64_terrain_state((col), (row))
#endif
#if defined(SC64_TERRAIN_ROW_MASKS) && \
    (defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE))
/* Packed-grid profiles can compare a whole cached row of fog state at once.
 * Soft-fog entries hold a clipped one-cell halo (at most 18 bits); ordinary
 * entries hold the view row (at most 17). The per-tile mask remains
 * authoritative for scroll reuse and dirty-cell selection. */
#define SC64_TERRAIN_ROW_MASKS_ACTIVE 1
#define SC64_TERRAIN_CACHE_MAX_ROWS 10
static u32 sc64_terrain_row_visible[SC64_TERRAIN_CACHE_MAX_ROWS];
static u32 sc64_terrain_row_explored[SC64_TERRAIN_CACHE_MAX_ROWS];

static unsigned
sc64_terrain_grid_run(const u8 *grid, int row, int col0, int width)
{
    unsigned base = (unsigned)(row * MAP_COLS + col0);
    unsigned first = base >> 3, last = (base + (unsigned)width - 1u) >> 3;
    unsigned bits = grid[first];
    if (last > first) bits |= (unsigned)grid[first + 1] << 8;
    if (last > first + 1) bits |= (unsigned)grid[first + 2] << 16;
#ifdef SC64_SOFT_FOG
    if (last > first + 2) bits |= (unsigned)grid[first + 3] << 24;
#endif
    return (bits >> (base & 7u)) & ((1u << width) - 1u);
}

#ifdef SC64_SOFT_FOG
static void
sc64_terrain_halo(int col0, int row0, int col1, int row1,
                  int *halo_col0, int *halo_row0,
                  int *halo_col1, int *halo_row1)
{
    *halo_col0 = col0 > 0 ? col0 - 1 : 0;
    *halo_row0 = row0 > 0 ? row0 - 1 : 0;
    *halo_col1 = col1 < MAP_COLS ? col1 + 1 : MAP_COLS;
    *halo_row1 = row1 < MAP_ROWS ? row1 + 1 : MAP_ROWS;
}
#endif
#endif
/* The signature of a cell of the view being drawn. */
static inline Sc64TerrainState
sc64_terrain_state_view(int col, int row)
{
#ifdef SC64_SOFT_FOG
    if (sc64_terrain_now_valid &&
        col >= sc64_terrain_now_col0 && col < sc64_terrain_now_col0 + sc64_terrain_now_cols &&
        row >= sc64_terrain_now_row0 && row < sc64_terrain_now_row0 + sc64_terrain_now_rows)
        return sc64_terrain_now[(row - sc64_terrain_now_row0) * sc64_terrain_now_cols +
                                (col - sc64_terrain_now_col0)];
#endif
    return sc64_terrain_state(col, row);
}
static int sc64_terrain_cache_valid;
static volatile unsigned sc64_terrain_requests, sc64_terrain_hits, sc64_terrain_partial;
static int sc64_terrain_cache_reused_base;
static int sc64_terrain_cache_camx, sc64_terrain_cache_camy;
static int sc64_terrain_cache_mission;
static unsigned int sc64_terrain_cache_static_key;
static int sc64_terrain_cache_mask_count;
#ifdef SC64_DIAGNOSTIC_TERRAIN
/* Cumulative counters for one diagnostic boot/replay. Pixel counts describe
 * copies from the saved terrain buffer; tile counts describe cache-mask cells
 * checked and marked dirty. These objects and increments disappear otherwise. */
static volatile u32 sc64_terrain_diag_invalid_cache;
static volatile u32 sc64_terrain_diag_mission_change;
static volatile u32 sc64_terrain_diag_static_key_change;
static volatile u32 sc64_terrain_diag_camera_oversize;
static volatile u32 sc64_terrain_diag_camera_change;
static volatile u32 sc64_terrain_diag_invalid_view;
static volatile u32 sc64_terrain_diag_invalid_mask;
static volatile u32 sc64_terrain_diag_blit_failure;
static volatile u32 sc64_terrain_diag_capture_failure;
static volatile u32 sc64_terrain_diag_reused_pixels;
static volatile u32 sc64_terrain_diag_captured_pixels;
static volatile u32 sc64_terrain_diag_captured_tiles;
static volatile u32 sc64_terrain_diag_tiles_checked;
static volatile u32 sc64_terrain_diag_dirty_tiles;
#define SC64_TERRAIN_DIAG_ADD(counter, amount) ((counter) += (u32)(amount))
#define SC64_TERRAIN_DIAG_REJECT(key) do { \
    if (!sc64_terrain_cache_valid) SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_invalid_cache, 1); \
    else if (sc64_terrain_cache_mission != sc64_terrain_cache_mission_key()) \
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_mission_change, 1); \
    else if (sc64_terrain_cache_static_key != (unsigned int)(key)) \
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_static_key_change, 1); \
} while (0)
#define SC64_TERRAIN_DIAG_REJECT_AT(x, y, key) do { \
    if (!sc64_terrain_cache_valid) SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_invalid_cache, 1); \
    else if (sc64_terrain_cache_mission != sc64_terrain_cache_mission_key()) \
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_mission_change, 1); \
    else if (sc64_terrain_cache_static_key != (unsigned int)(key)) \
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_static_key_change, 1); \
    else if ((x) != sc64_terrain_cache_camx || (y) != sc64_terrain_cache_camy) \
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_camera_change, 1); \
} while (0)
#endif
#ifdef SC64_TERRAIN_SCROLL
/* dirty[] is indexed for the camera currently being rendered; mask[] and
 * camx/camy continue to describe the last captured framebuffer until store. */
static int sc64_terrain_cache_view_camx, sc64_terrain_cache_view_camy;
#endif

/* Whole-band transfers between the framebuffer and the saved pixels. With
 * the VRAM split, each is two transfers over the same screen rows. */
static int
sc64_terrain_cache_restore(void)
{
#ifdef SC64_TERRAIN_CACHE_VRAM
    return gfx_blit_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_VRAM_ROWS,
                         sc64_terrain_cache_pixels_vram) &&
           gfx_blit_band(SC64_TERRAIN_CACHE_Y + SC64_TERRAIN_CACHE_VRAM_ROWS,
                         SC64_TERRAIN_CACHE_RAM_ROWS, sc64_terrain_cache_pixels_ram);
#else
    return gfx_blit_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                         sc64_terrain_cache_pixels);
#endif
}

static int
sc64_terrain_cache_capture(void)
{
#ifdef SC64_TERRAIN_CACHE_VRAM
    return gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_VRAM_ROWS,
                         sc64_terrain_cache_pixels_vram) &&
           gfx_read_band(SC64_TERRAIN_CACHE_Y + SC64_TERRAIN_CACHE_VRAM_ROWS,
                         SC64_TERRAIN_CACHE_RAM_ROWS, sc64_terrain_cache_pixels_ram);
#else
    return gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                         sc64_terrain_cache_pixels);
#endif
}

#if defined(SC64_TERRAIN_DIRTY_CAPTURE) && defined(SC64_TERRAIN_CACHE_VRAM)
#include "sc64_static_dirty_rows.h"
#define SC64_TERRAIN_DIRTY_CAPTURE_ACTIVE 1
static int sc64_terrain_cache_mission_key(void);
typedef struct {
    u32 generation;
    u8 page;
} Sc64TerrainCapturePage;

static inline Sc64TerrainCapturePage sc64_terrain_capture_page(void)
{
    u8 page = gfx_backbuffer_index();
    return (Sc64TerrainCapturePage){gfx_clear_generation(page), page};
}

static int __attribute__((noinline))
sc64_terrain_capture_dirty(const Sc64StaticDirtyRowsPlan *plan, Sc64TerrainCapturePage base,
                           int x, int y, unsigned static_key)
{
    const int first = SC64_TERRAIN_CACHE_Y;
    const int split = first + SC64_TERRAIN_CACHE_VRAM_ROWS;
    const int end = first + SC64_TERRAIN_CACHE_HEIGHT;
    int previous = first, total = 0;
    if (!plan || !plan->count || plan->count > SC64_STATIC_DIRTY_ROWS_MAX_BANDS ||
        !sc64_terrain_cache_valid || !sc64_terrain_cache_reused_base ||
        sc64_terrain_cache_camx != x || sc64_terrain_cache_camy != y ||
        sc64_terrain_cache_static_key != static_key ||
        sc64_terrain_cache_mission != sc64_terrain_cache_mission_key() ||
        base.page != gfx_backbuffer_index() || base.generation != gfx_clear_generation(base.page)) return 0;
    for (unsigned i = 0; i < plan->count; ++i) {
        int top = plan->bands[i].top, bottom = plan->bands[i].bottom;
        if (top < previous || bottom <= top || bottom > end) return 0;
        total += bottom - top;
        previous = bottom;
    }
    if (total >= SC64_TERRAIN_CACHE_HEIGHT) return 0;
    for (unsigned i = 0; i < plan->count; ++i) {
        int top = plan->bands[i].top, bottom = plan->bands[i].bottom;
        if (top < split) {
            int last = bottom < split ? bottom : split;
            if (!gfx_read_band(top, last - top, sc64_terrain_cache_pixels_vram + (top - first) * 240)) return 0;
            top = last;
        }
        if (top < bottom &&
            !gfx_read_band(top, bottom - top, sc64_terrain_cache_pixels_ram + (top - split) * 240)) return 0;
    }
    return total;
}
#endif

#ifdef SC64_TERRAIN_SCROLL
/* Scrolled restore: rows outside each segment are skipped by the window
 * helper, so the two segment calls together cover exactly the old band. */
static int
sc64_terrain_cache_restore_window(int dx, int dy)
{
#ifdef SC64_TERRAIN_CACHE_VRAM
    return gfx_blit_terrain_window(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                           sc64_terrain_cache_pixels_vram, 240,
                           SC64_TERRAIN_CACHE_VRAM_ROWS, dx, dy) &&
           gfx_blit_terrain_window(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                           sc64_terrain_cache_pixels_ram, 240,
                           SC64_TERRAIN_CACHE_RAM_ROWS, dx,
                           dy - SC64_TERRAIN_CACHE_VRAM_ROWS);
#else
    return gfx_blit_terrain_window(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                           sc64_terrain_cache_pixels, 240,
                           SC64_TERRAIN_CACHE_HEIGHT, dx, dy);
#endif
}
#endif

static int
sc64_terrain_cache_mission_key(void)
{
#ifdef SC64_SHOWCASE_ENABLED
    if(sc64_showcase_scene>=0)return 200+sc64_showcase_scene;
#endif
#ifdef SC64_RACES
    return race_mode?
#ifdef SC64_IMPORTED_SKIRMISH
        100+race_skirmish_map
#else
        100
#endif
        :race_selected_mission;
#elif defined(SC64_TERRAN_CAMPAIGN)
    return campaign_mission;
#else
    return 0;
#endif
}

static int
sc64_terrain_cache_view(int x, int y, int *col0, int *row0,
                        int *col1, int *row1)
{
    if (x < 0 || y < 0 || x > WORLD_W - 240 || y > WORLD_H - SC64_TERRAIN_CACHE_HEIGHT)
        return 0;
    *col0 = x / 16;
    *col1 = (x + 240) / 16 + 1;
    *row0 = y / 16;
    *row1 = (y + SC64_TERRAIN_CACHE_HEIGHT) / 16 + 1;
    if (*col1 > MAP_COLS) *col1 = MAP_COLS;
    if (*row1 > MAP_ROWS) *row1 = MAP_ROWS;
    return (*col1 > *col0 && *row1 > *row0 &&
            (*col1 - *col0) * (*row1 - *row0) <= SC64_TERRAIN_CACHE_CELLS);
}

#ifdef SC64_SOFT_FOG
/* Compute the signatures of the view at (x, y) unless a lookup already has. */
static void __attribute__((unused))
sc64_terrain_prepare(int x, int y)
{
    int col0, row0, col1, row1;
    if (!sc64_terrain_now_valid && sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1))
        sc64_terrain_fill_now(col0, row0, col1, row1);
}
#endif

#if defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)
#define SC64_TERRAIN_CACHE_KEY static_key
static int
sc64_terrain_cached_key(int x, int y, unsigned int static_key)
#else
#define SC64_TERRAIN_CACHE_KEY (unsigned int)(SC64_TERRAIN_STATIC_KEY)
static int
sc64_terrain_cached(int x, int y)
#endif
{
#ifdef SC64_TERRAIN_SCROLL
    int old_col0, old_row0, old_col1, old_row1;
    int col0, row0, col1, row1, col, row, i = 0, unchanged = 1;
    int dx, dy;
    sc64_terrain_requests++;
    sc64_terrain_cache_reused_base = 0;
#ifdef SC64_SOFT_FOG
    sc64_terrain_now_valid = 0;
    sc64_terrain_part_active = 0;
#endif
    if (!sc64_terrain_cache_valid ||
        sc64_terrain_cache_mission != sc64_terrain_cache_mission_key() ||
        sc64_terrain_cache_static_key != SC64_TERRAIN_CACHE_KEY) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
        SC64_TERRAIN_DIAG_REJECT(SC64_TERRAIN_CACHE_KEY);
#endif
        return 0;
    }
    dx = x - sc64_terrain_cache_camx;
    dy = y - sc64_terrain_cache_camy;
    if ((dx != 0 || dy != 0) &&
        ((dx < 0 ? -dx : dx) >= 240 ||
         (dy < 0 ? -dy : dy) >= SC64_TERRAIN_CACHE_HEIGHT)) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_camera_oversize, 1);
#endif
        return 0;
    }
    if (!sc64_terrain_cache_view(sc64_terrain_cache_camx, sc64_terrain_cache_camy,
                                 &old_col0, &old_row0, &old_col1, &old_row1)) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_invalid_view, 1);
#endif
        return 0;
    }
    if (dx == 0 && dy == 0) {
        if ((old_col1 - old_col0) * (old_row1 - old_row0) !=
            sc64_terrain_cache_mask_count) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
            SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_invalid_mask, 1);
#endif
            sc64_terrain_cache_valid = 0;
            return 0;
        }
        if (!sc64_terrain_cache_restore()) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
            SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_blit_failure, 1);
#endif
            sc64_terrain_cache_valid = 0;
            return 0;
        }
#ifdef SC64_DIAGNOSTIC_TERRAIN
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_reused_pixels,
                              240 * SC64_TERRAIN_CACHE_HEIGHT);
#endif
        sc64_terrain_cache_view_camx = x;
        sc64_terrain_cache_view_camy = y;
        sc64_terrain_cache_reused_base = 1;
#ifdef SC64_TERRAIN_ROW_MASKS_ACTIVE
        {
            int width = old_col1 - old_col0, rows = old_row1 - old_row0, r;
            unsigned changed_rows = 0;
#ifdef SC64_SOFT_FOG
            int hcol0, hrow0, hcol1, hrow1;
            sc64_terrain_halo(old_col0, old_row0, old_col1, old_row1,
                              &hcol0, &hrow0, &hcol1, &hrow1);
            if (hcol1 - hcol0 >= 32 || hrow1 - hrow0 > SC64_TERRAIN_CACHE_MAX_ROWS) {
                sc64_terrain_cache_valid = 0;
                return 0;
            }
            for (r = hrow0; r < hrow1; ++r)
                if (sc64_terrain_grid_run(visible, r, hcol0, hcol1 - hcol0) !=
                        sc64_terrain_row_visible[r - hrow0] ||
                    sc64_terrain_grid_run(explored, r, hcol0, hcol1 - hcol0) !=
                        sc64_terrain_row_explored[r - hrow0]) {
                    changed_rows = 1u;
                    break;
                }
#else
            for (r = 0; r < rows; r++)
                if (sc64_terrain_grid_run(visible, old_row0 + r, old_col0, width) !=
                        sc64_terrain_row_visible[r] ||
                    sc64_terrain_grid_run(explored, old_row0 + r, old_col0, width) !=
                        sc64_terrain_row_explored[r])
                    changed_rows |= 1u << r;
#endif
            /* On soft-fog profiles, the saved halo row runs already cover all
             * off-view neighbors that can affect any visible signature. */
            /* Nothing reads the per-tile dirty flags after a complete hit. */
            if (!changed_rows) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
                SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_tiles_checked, width * rows);
#endif
                sc64_terrain_hits++;
                return 1;
            }
            SC64_TERRAIN_FILL(old_col0, old_row0, old_col1, old_row1);
            for (r = 0; r < rows; r++) {
#ifndef SC64_SOFT_FOG
                if (!(changed_rows & (1u << r))) {
                    memset(sc64_terrain_cache_dirty + i, 0, (size_t)width);
#ifdef SC64_DIAGNOSTIC_TERRAIN
                    SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_tiles_checked, width);
#endif
                    i += width;
                    continue;
                }
#endif
                row = old_row0 + r;
                for (col = old_col0; col < old_col1; col++) {
                    Sc64TerrainState state = SC64_TERRAIN_STATE_AT(i, col, row);
                    sc64_terrain_cache_dirty[i] = sc64_terrain_cache_mask[i] != state;
#ifdef SC64_DIAGNOSTIC_TERRAIN
                    SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_tiles_checked, 1);
                    if (sc64_terrain_cache_dirty[i])
                        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_dirty_tiles, 1);
#endif
                    i++;
                }
            }
            sc64_terrain_partial++;
            return 0;
        }
#else
        SC64_TERRAIN_FILL(old_col0, old_row0, old_col1, old_row1);
#if defined(SC64_TERRAIN_FAST_VALIDATE) && defined(SC64_SOFT_FOG) && \
    !defined(SC64_DIAGNOSTIC_TERRAIN)
        /* Same camera: tile i of the view is cache-mask cell i. The soft-fog
         * state of cell i is sc64_terrain_now[i], so the nested loop below
         * reduces to one pass over the view's cells. */
        {
            const Sc64TerrainState *mask = sc64_terrain_cache_mask;
            const u16 *now = sc64_terrain_now;
            u8 *dirty = sc64_terrain_cache_dirty;
            const int count = (old_col1 - old_col0) * (old_row1 - old_row0);
            int any = 0;
            for (i = 0; i < count; i++) {
                int d = mask[i] != (Sc64TerrainState)now[i];
                dirty[i] = (u8)d;
                any |= d;
            }
            unchanged = !any;
        }
        if (0)
#endif
        for (row = old_row0; row < old_row1; row++)
            for (col = old_col0; col < old_col1; col++) {
                Sc64TerrainState state = SC64_TERRAIN_STATE_AT(i, col, row);
                sc64_terrain_cache_dirty[i] = sc64_terrain_cache_mask[i] != state;
#ifdef SC64_DIAGNOSTIC_TERRAIN
                SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_tiles_checked, 1);
                if (sc64_terrain_cache_dirty[i])
                    SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_dirty_tiles, 1);
#endif
                if (sc64_terrain_cache_dirty[i]) unchanged = 0;
                i++;
            }
        if (unchanged) sc64_terrain_hits++;
        else sc64_terrain_partial++;
        return unchanged;
#endif
    }
    if (!sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1)) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_invalid_view, 1);
#endif
        return 0;
    }
    if ((old_col1 - old_col0) * (old_row1 - old_row0) !=
        sc64_terrain_cache_mask_count) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_invalid_mask, 1);
#endif
        sc64_terrain_cache_valid = 0;
        return 0;
    }
    if (!sc64_terrain_cache_restore_window(dx, dy)) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_blit_failure, 1);
#endif
        sc64_terrain_cache_valid = 0;
        return 0;
    }
#ifdef SC64_DIAGNOSTIC_TERRAIN
    SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_reused_pixels,
        (240 - (dx < 0 ? -dx : dx)) *
        (SC64_TERRAIN_CACHE_HEIGHT - (dy < 0 ? -dy : dy)));
#endif
    sc64_terrain_cache_view_camx = x;
    sc64_terrain_cache_view_camy = y;
    sc64_terrain_cache_reused_base = 1;
    SC64_TERRAIN_FILL(col0, row0, col1, row1);
#ifdef SC64_SOFT_FOG
    /* The shifted old pixels cover screen x in [-dx, 240-dx) and band rows
     * [-dy, H-dy); the rest of the band is the exposed strips. */
    sc64_terrain_part_active = 1;
    sc64_terrain_strip_x0 = dx > 0 ? 240 - dx : 0;
    sc64_terrain_strip_x1 = dx > 0 ? 240 : (dx < 0 ? -dx : 0);
    sc64_terrain_strip_y0 = SC64_TERRAIN_CACHE_Y +
        (dy > 0 ? SC64_TERRAIN_CACHE_HEIGHT - dy : 0);
    sc64_terrain_strip_y1 = SC64_TERRAIN_CACHE_Y +
        (dy > 0 ? SC64_TERRAIN_CACHE_HEIGHT : (dy < 0 ? -dy : 0));
#endif
#ifdef SC64_TERRAIN_FAST_VALIDATE
    /* The test below splits into a row half and a column half: whether the
     * tile is in the old cell window and whether its clipped rectangle lies
     * inside the old camera rectangle. Compute each half once per row and
     * once per column; the per-tile work is the mask compare. The view is
     * at most 16 columns wide; the general loop stays as the fallback. */
    if (col1 - col0 <= SC64_TERRAIN_FAST_COLS) {
        const int old_x = sc64_terrain_cache_camx, old_y = sc64_terrain_cache_camy;
        const int old_width = old_col1 - old_col0, cols = col1 - col0;
        /* Per column: the offset into an old mask row (-1 outside the old
         * window) and whether the clipped tile leaves the old camera. */
        signed char col_offset[SC64_TERRAIN_FAST_COLS], no_offset[SC64_TERRAIN_FAST_COLS];
        u8 col_edge[SC64_TERRAIN_FAST_COLS];
        int c, any_dirty = 0;
        for (c = 0; c < cols; c++) {
            int left = (col0 + c) * 16, right = left + 16;
            if (left < x) left = x;
            if (right > x + 240) right = x + 240;
            col_offset[c] = (signed char)(col0 + c >= old_col0 && col0 + c < old_col1 ?
                                          col0 + c - old_col0 : -1);
            no_offset[c] = -1;
            col_edge[c] = (u8)(left < old_x || right > old_x + 240);
        }
        for (row = row0; row < row1; row++) {
            int top = row * 16, bottom = top + 16;
            int row_in = row >= old_row0 && row < old_row1;
            const signed char *offset = row_in ? col_offset : no_offset;
            const Sc64TerrainState *old_row_mask = sc64_terrain_cache_mask +
                (row_in ? (row - old_row0) * old_width : 0);
            unsigned row_edge;
            if (top < y) top = y;
            if (bottom > y + SC64_TERRAIN_CACHE_HEIGHT)
                bottom = y + SC64_TERRAIN_CACHE_HEIGHT;
            row_edge = top < old_y || bottom > old_y + SC64_TERRAIN_CACHE_HEIGHT;
            for (c = 0; c < cols; c++, i++) {
                Sc64TerrainState state = SC64_TERRAIN_STATE_AT(i, col0 + c, row);
                int off = offset[c];
                unsigned dirty = 1;
#ifdef SC64_SOFT_FOG
                unsigned part = 1;
#endif
                if (off >= 0) {
                    /* In the old window: dirty unless inside the old camera
                     * with unchanged fog; partial when only the camera moved. */
                    unsigned same = old_row_mask[off] == state;
                    unsigned edge = row_edge | col_edge[c];
                    dirty = edge | (same ^ 1u);
#ifdef SC64_SOFT_FOG
                    part = edge & same;
#endif
                }
                sc64_terrain_cache_dirty[i] = (u8)dirty;
#ifdef SC64_SOFT_FOG
                sc64_terrain_part[i] = (u8)part;
#endif
#ifdef SC64_DIAGNOSTIC_TERRAIN
                SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_tiles_checked, 1);
                if (dirty) SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_dirty_tiles, 1);
#endif
                any_dirty |= (int)dirty;
            }
        }
        if (any_dirty) unchanged = 0;
    } else
#endif
    for (row = row0; row < row1; row++)
        for (col = col0; col < col1; col++) {
            int left = col * 16, top = row * 16;
            int right = left + 16, bottom = top + 16;
            int old_index = -1;
            Sc64TerrainState state = SC64_TERRAIN_STATE_AT(i, col, row);
            if (left < x) left = x;
            if (top < y) top = y;
            if (right > x + 240) right = x + 240;
            if (bottom > y + SC64_TERRAIN_CACHE_HEIGHT)
                bottom = y + SC64_TERRAIN_CACHE_HEIGHT;
            if (col >= old_col0 && col < old_col1 &&
                row >= old_row0 && row < old_row1)
                old_index = (row - old_row0) * (old_col1 - old_col0) + col - old_col0;
            sc64_terrain_cache_dirty[i] =
                old_index < 0 ||
                left < sc64_terrain_cache_camx ||
                right > sc64_terrain_cache_camx + 240 ||
                top < sc64_terrain_cache_camy ||
                bottom > sc64_terrain_cache_camy + SC64_TERRAIN_CACHE_HEIGHT ||
                sc64_terrain_cache_mask[old_index < 0 ? 0 : old_index] != state;
#ifdef SC64_SOFT_FOG
            /* A cell the scroll exposed with unchanged fog, or one that lies
             * wholly beyond the old view: either way, only the exposed
             * strips of it are on screen and new. */
            sc64_terrain_part[i] = sc64_terrain_cache_dirty[i] &&
                (old_index < 0 || sc64_terrain_cache_mask[old_index] == state);
#endif
#ifdef SC64_DIAGNOSTIC_TERRAIN
            SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_tiles_checked, 1);
            if (sc64_terrain_cache_dirty[i])
                SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_dirty_tiles, 1);
#endif
            if (sc64_terrain_cache_dirty[i]) unchanged = 0;
            i++;
        }
    if (unchanged) sc64_terrain_hits++;
    else sc64_terrain_partial++;
    return unchanged;
#else
    int col0, row0, col1, row1, col, row, i = 0, unchanged = 1;
    sc64_terrain_requests++;
    sc64_terrain_cache_reused_base = 0;
#ifdef SC64_SOFT_FOG
    sc64_terrain_now_valid = 0;
    sc64_terrain_part_active = 0;
#endif
    if (!sc64_terrain_cache_valid ||
        x != sc64_terrain_cache_camx || y != sc64_terrain_cache_camy ||
        sc64_terrain_cache_mission != sc64_terrain_cache_mission_key() ||
        sc64_terrain_cache_static_key != SC64_TERRAIN_CACHE_KEY) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
        SC64_TERRAIN_DIAG_REJECT_AT(x, y, SC64_TERRAIN_CACHE_KEY);
#endif
        return 0;
    }
    if (!sc64_terrain_cache_restore()) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
        SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_blit_failure, 1);
#endif
        sc64_terrain_cache_valid = 0;
        return 0;
    }
#ifdef SC64_DIAGNOSTIC_TERRAIN
    SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_reused_pixels,
                          240 * SC64_TERRAIN_CACHE_HEIGHT);
#endif
#ifdef SC64_DIAGNOSTIC_TERRAIN
    {
        int view_ok = sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1);
        if (!view_ok ||
            (col1 - col0) * (row1 - row0) != sc64_terrain_cache_mask_count) {
            if (!view_ok)
                SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_invalid_view, 1);
            else
                SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_invalid_mask, 1);
            sc64_terrain_cache_valid = 0;
            return 0;
        }
    }
#else
    if (!sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1) ||
        (col1 - col0) * (row1 - row0) != sc64_terrain_cache_mask_count) {
        sc64_terrain_cache_valid = 0;
        return 0;
    }
#endif
    sc64_terrain_cache_reused_base = 1;
    SC64_TERRAIN_FILL(col0, row0, col1, row1);
    for (row = row0; row < row1; row++)
        for (col = col0; col < col1; col++) {
            Sc64TerrainState state = SC64_TERRAIN_STATE_AT(i, col, row);
            sc64_terrain_cache_dirty[i] = sc64_terrain_cache_mask[i] != state;
#ifdef SC64_DIAGNOSTIC_TERRAIN
            SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_tiles_checked, 1);
            if (sc64_terrain_cache_dirty[i])
                SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_dirty_tiles, 1);
#endif
            if (sc64_terrain_cache_dirty[i]) unchanged = 0;
            i++;
        }
    if (unchanged) sc64_terrain_hits++;
    else sc64_terrain_partial++;
    return unchanged;
#endif
}
#undef SC64_TERRAIN_CACHE_KEY

static int
sc64_terrain_tile_clean(int col, int row)
{
    int col0, row0, col1, row1, i;
#ifdef SC64_TERRAIN_SCROLL
    int x = sc64_terrain_cache_view_camx, y = sc64_terrain_cache_view_camy;
#else
    int x = sc64_terrain_cache_camx, y = sc64_terrain_cache_camy;
#endif
    if (!sc64_terrain_cache_reused_base) return 0;
    /* Successful reuse already validated this view before filling dirty[].
     * Derive its bounds directly instead of revalidating it for every tile. */
    col0 = x / 16;
    row0 = y / 16;
    col1 = (x + 240) / 16 + 1;
    row1 = (y + SC64_TERRAIN_CACHE_HEIGHT) / 16 + 1;
    if (col1 > MAP_COLS) col1 = MAP_COLS;
    if (row1 > MAP_ROWS) row1 = MAP_ROWS;
    if (col < col0 || col >= col1 || row < row0 || row >= row1)
        return 0;
    i = (row - row0) * (col1 - col0) + col - col0;
    return !sc64_terrain_cache_dirty[i];
}

#ifdef SC64_SOFT_FOG
/* A dirty tile that needs only its exposed strips redrawn (see
 * sc64_terrain_part); false for every tile unless the last lookup scrolled. */
static int
sc64_terrain_tile_partial(int col, int row)
{
#ifdef SC64_TERRAIN_SCROLL
    int x = sc64_terrain_cache_view_camx, y = sc64_terrain_cache_view_camy;
#else
    int x = sc64_terrain_cache_camx, y = sc64_terrain_cache_camy;
#endif
    int col0 = x / 16, row0 = y / 16, col1 = (x + 240) / 16 + 1;
    if (!sc64_terrain_part_active || !sc64_terrain_cache_reused_base) return 0;
    if (col1 > MAP_COLS) col1 = MAP_COLS;
    if (col < col0 || col >= col1 || row < row0) return 0;
    {
        int i = (row - row0) * (col1 - col0) + col - col0;
        if (i >= SC64_TERRAIN_CACHE_CELLS) return 0;
        return sc64_terrain_part[i];
    }
}

/* Draw one tile of fog-composed terrain: whole, or only the exposed strips. */
static void __attribute__((unused))
sc64_terrain_fog_cell(int partial, int sx, int sy, u16 signature,
                      const u8 *terrain, const u8 *lut)
{
    if (signature == SC64_FOG_SIGNATURE_COUNT - 1 && terrain)
        sc64_fog_blit_cell(sx, sy, terrain); /* fully visible: as the base draws it */
    else if (partial)
        sc64_fog_raster_strips(sx, sy, signature, terrain, lut,
                               sc64_terrain_strip_x0, sc64_terrain_strip_x1,
                               sc64_terrain_strip_y0, sc64_terrain_strip_y1);
    else
        sc64_fog_raster_cell(sx, sy, signature, terrain, lut);
}
#endif

#ifdef SC64_TERRAIN_FAST_VALIDATE
/* Answers of sc64_terrain_tile_clean, sc64_terrain_tile_partial and
 * sc64_terrain_state_view for the tiles of one draw window, visited row-major
 * with a running index i. When the window is the cache view that the lookup
 * just validated, tile i of the window is cache cell i, so each answer is
 * one array read; for any other window the per-tile functions answer. */
enum {
    SC64_TERRAIN_TILES_NONE,    /* no reused base: no tile is clean or partial */
    SC64_TERRAIN_TILES_INDEXED, /* the window is the reused cache view */
    SC64_TERRAIN_TILES_CALL     /* another window: ask the per-tile functions */
};
typedef struct {
    int clean_mode;
#ifdef SC64_SOFT_FOG
    int part_active, state_indexed;
#endif
} Sc64TerrainTiles;

static inline void
sc64_terrain_tiles_begin(Sc64TerrainTiles *t, int col0, int row0, int col1, int row1)
{
#ifdef SC64_TERRAIN_SCROLL
    int x = sc64_terrain_cache_view_camx, y = sc64_terrain_cache_view_camy;
#else
    int x = sc64_terrain_cache_camx, y = sc64_terrain_cache_camy;
#endif
    t->clean_mode = SC64_TERRAIN_TILES_NONE;
    if (sc64_terrain_cache_reused_base) {
        /* The bounds sc64_terrain_tile_clean derives for every tile. */
        int view_col1 = (x + 240) / 16 + 1;
        int view_row1 = (y + SC64_TERRAIN_CACHE_HEIGHT) / 16 + 1;
        if (view_col1 > MAP_COLS) view_col1 = MAP_COLS;
        if (view_row1 > MAP_ROWS) view_row1 = MAP_ROWS;
        t->clean_mode = x / 16 == col0 && y / 16 == row0 &&
                        view_col1 == col1 && view_row1 == row1 &&
                        col1 > col0 && row1 > row0 &&
                        (col1 - col0) * (row1 - row0) <= SC64_TERRAIN_CACHE_CELLS ?
                        SC64_TERRAIN_TILES_INDEXED : SC64_TERRAIN_TILES_CALL;
    }
#ifdef SC64_SOFT_FOG
    t->part_active = sc64_terrain_part_active;
    t->state_indexed = sc64_terrain_now_valid &&
        sc64_terrain_now_col0 == col0 && sc64_terrain_now_row0 == row0 &&
        sc64_terrain_now_cols == col1 - col0 && sc64_terrain_now_rows == row1 - row0;
#endif
}

static inline int
sc64_terrain_tiles_clean(const Sc64TerrainTiles *t, int i, int col, int row)
{
    if (t->clean_mode == SC64_TERRAIN_TILES_INDEXED) return !sc64_terrain_cache_dirty[i];
    return t->clean_mode == SC64_TERRAIN_TILES_CALL && sc64_terrain_tile_clean(col, row);
}

#ifdef SC64_SOFT_FOG
static inline int
sc64_terrain_tiles_partial(const Sc64TerrainTiles *t, int i, int col, int row)
{
    if (t->clean_mode == SC64_TERRAIN_TILES_INDEXED)
        return t->part_active ? sc64_terrain_part[i] : 0;
    return t->clean_mode == SC64_TERRAIN_TILES_CALL ? sc64_terrain_tile_partial(col, row) : 0;
}

static inline Sc64TerrainState
sc64_terrain_tiles_state(const Sc64TerrainTiles *t, int i, int col, int row)
{
    return t->state_indexed ? (Sc64TerrainState)sc64_terrain_now[i] :
                              sc64_terrain_state_view(col, row);
}
#endif
#endif

#if defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)
#define SC64_TERRAIN_CACHE_KEY static_key
static int
sc64_terrain_store_key(int x, int y, unsigned int static_key
#ifdef SC64_TERRAIN_DIRTY_CAPTURE_ACTIVE
                       , const Sc64StaticDirtyRowsPlan *plan, Sc64TerrainCapturePage base
#endif
                       )
#else
#define SC64_TERRAIN_CACHE_KEY (unsigned int)(SC64_TERRAIN_STATIC_KEY)
static int
sc64_terrain_store(int x, int y
#ifdef SC64_TERRAIN_DIRTY_CAPTURE_ACTIVE
                   , const Sc64StaticDirtyRowsPlan *plan, Sc64TerrainCapturePage base
#endif
                   )
#endif
{
    int col0, row0, col1, row1, col, row, i = 0;
#ifdef SC64_TERRAIN_DIRTY_CAPTURE_ACTIVE
    int captured_rows = 0;
#endif
    if (!sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1) ||
#ifdef SC64_TERRAIN_DIRTY_CAPTURE_ACTIVE
        (!(captured_rows = sc64_terrain_capture_dirty(plan, base, x, y, SC64_TERRAIN_CACHE_KEY)) &&
         !sc64_terrain_cache_capture())) {
#else
        !sc64_terrain_cache_capture()) {
#endif
#ifdef SC64_DIAGNOSTIC_TERRAIN
        if (!sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1))
            SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_invalid_view, 1);
        else
            SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_capture_failure, 1);
#endif
        sc64_terrain_cache_valid = 0;
        sc64_terrain_cache_reused_base = 0;
        return 0;
    }
#ifdef SC64_SOFT_FOG
    if (!sc64_terrain_now_valid || sc64_terrain_now_col0 != col0 ||
        sc64_terrain_now_row0 != row0 || sc64_terrain_now_cols != col1 - col0 ||
        sc64_terrain_now_rows != row1 - row0)
        sc64_terrain_fill_now(col0, row0, col1, row1);
    if (sc64_terrain_now_valid) {
        for (i = 0; i < sc64_terrain_now_cols * sc64_terrain_now_rows; ++i)
            sc64_terrain_cache_mask[i] = sc64_terrain_now[i];
        sc64_terrain_now_valid = 0;
    } else
#endif
    for (row = row0; row < row1; row++)
        for (col = col0; col < col1; col++) {
            sc64_terrain_cache_mask[i++] = sc64_terrain_state(col, row);
        }
    sc64_terrain_cache_mask_count = i;
#ifdef SC64_TERRAIN_ROW_MASKS_ACTIVE
    {
        int mask_row0 = row0, mask_row1 = row1;
#ifdef SC64_SOFT_FOG
        int halo_col0, halo_row0, halo_col1, halo_row1;
        sc64_terrain_halo(col0, row0, col1, row1,
                          &halo_col0, &halo_row0, &halo_col1, &halo_row1);
        if (halo_col1 - halo_col0 >= 32 ||
            halo_row1 - halo_row0 > SC64_TERRAIN_CACHE_MAX_ROWS) {
            sc64_terrain_cache_valid = 0;
            sc64_terrain_cache_reused_base = 0;
            return 0;
        }
        mask_row0 = halo_row0;
        mask_row1 = halo_row1;
        for (row = halo_row0; row < halo_row1; row++) {
            sc64_terrain_row_visible[row - halo_row0] =
                sc64_terrain_grid_run(visible, row, halo_col0, halo_col1 - halo_col0);
            sc64_terrain_row_explored[row - halo_row0] =
                sc64_terrain_grid_run(explored, row, halo_col0, halo_col1 - halo_col0);
        }
#else
        for (row = row0; row < row1; row++) {
            sc64_terrain_row_visible[row - row0] =
                sc64_terrain_grid_run(visible, row, col0, col1 - col0);
            sc64_terrain_row_explored[row - row0] =
                sc64_terrain_grid_run(explored, row, col0, col1 - col0);
        }
#endif
        if (mask_row1 - mask_row0 > SC64_TERRAIN_CACHE_MAX_ROWS) {
        sc64_terrain_cache_valid = 0;
        sc64_terrain_cache_reused_base = 0;
        return 0;
        }
    }
#endif
#ifdef SC64_DIAGNOSTIC_TERRAIN
    SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_captured_pixels,
                          240 *
#ifdef SC64_TERRAIN_DIRTY_CAPTURE_ACTIVE
                          (captured_rows ? captured_rows : SC64_TERRAIN_CACHE_HEIGHT)
#else
                          SC64_TERRAIN_CACHE_HEIGHT
#endif
                          );
    SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_captured_tiles, i);
#endif
    sc64_terrain_cache_camx = x;
    sc64_terrain_cache_camy = y;
    sc64_terrain_cache_mission = sc64_terrain_cache_mission_key();
    sc64_terrain_cache_static_key = SC64_TERRAIN_CACHE_KEY;
    sc64_terrain_cache_valid = 1;
    sc64_terrain_cache_reused_base = 0;
    return 1;
}
#undef SC64_TERRAIN_CACHE_KEY

#endif /* SC64_TILE_RENDERER */

#ifdef SC64_TERRAIN_LOCAL_GRID_GET
#undef SC64_GRID_GET
#undef SC64_TERRAIN_LOCAL_GRID_GET
#endif
#endif
