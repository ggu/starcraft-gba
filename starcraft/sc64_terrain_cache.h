#ifndef SC64_TERRAIN_CACHE_INCLUDED
#define SC64_TERRAIN_CACHE_INCLUDED

#if defined(SC64_RENDER_KEY_ONCE) && !defined(SC64_IMPORTED_MISSION)
#error "SC64_RENDER_KEY_ONCE requires SC64_IMPORTED_MISSION"
#endif

#ifndef SC64_GRID_GET
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_TERRAIN_LOCAL_GRID_GET
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
static u8 sc64_terrain_cache_mask[SC64_TERRAIN_CACHE_CELLS];
static u8 sc64_terrain_cache_dirty[SC64_TERRAIN_CACHE_CELLS];
#if defined(SC64_TERRAIN_ROW_MASKS) && \
    (defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE))
/* Packed-grid profiles can compare a whole cached row of fog state at once.
 * Each entry holds the visible or explored bits of one view row's columns
 * [col0, col1), at most 17 bits, read straight from the bit grids. The
 * per-tile mask stays authoritative for scrolled reuse and tile redraws. */
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
    return (bits >> (base & 7u)) & ((1u << width) - 1u);
}
#endif
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

#ifdef SC64_TERRAIN_SCROLL
/* Scrolled restore: rows outside each segment are skipped by the window
 * helper, so the two segment calls together cover exactly the old band. */
static int
sc64_terrain_cache_restore_window(int dx, int dy)
{
#ifdef SC64_TERRAIN_CACHE_VRAM
    return gfx_blit_window(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                           sc64_terrain_cache_pixels_vram, 240,
                           SC64_TERRAIN_CACHE_VRAM_ROWS, dx, dy) &&
           gfx_blit_window(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                           sc64_terrain_cache_pixels_ram, 240,
                           SC64_TERRAIN_CACHE_RAM_ROWS, dx,
                           dy - SC64_TERRAIN_CACHE_VRAM_ROWS);
#else
    return gfx_blit_window(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
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
            for (r = 0; r < rows; r++)
                if (sc64_terrain_grid_run(visible, old_row0 + r, old_col0, width) !=
                        sc64_terrain_row_visible[r] ||
                    sc64_terrain_grid_run(explored, old_row0 + r, old_col0, width) !=
                        sc64_terrain_row_explored[r])
                    changed_rows |= 1u << r;
            /* Nothing reads the per-tile dirty flags after a complete hit. */
            if (!changed_rows) {
#ifdef SC64_DIAGNOSTIC_TERRAIN
                SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_tiles_checked, width * rows);
#endif
                sc64_terrain_hits++;
                return 1;
            }
            for (r = 0; r < rows; r++) {
                if (!(changed_rows & (1u << r))) {
                    memset(sc64_terrain_cache_dirty + i, 0, (size_t)width);
#ifdef SC64_DIAGNOSTIC_TERRAIN
                    SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_tiles_checked, width);
#endif
                    i += width;
                    continue;
                }
                row = old_row0 + r;
                for (col = old_col0; col < old_col1; col++) {
                    int cell = col + row * MAP_COLS;
                    u8 state = (SC64_GRID_GET(visible, cell) ? 1 : 0) |
                               (SC64_GRID_GET(explored, cell) ? 2 : 0);
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
        for (row = old_row0; row < old_row1; row++)
            for (col = old_col0; col < old_col1; col++) {
                int cell = col + row * MAP_COLS;
                u8 state = (SC64_GRID_GET(visible, cell) ? 1 : 0) |
                           (SC64_GRID_GET(explored, cell) ? 2 : 0);
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
    for (row = row0; row < row1; row++)
        for (col = col0; col < col1; col++) {
            int cell = col + row * MAP_COLS;
            int left = col * 16, top = row * 16;
            int right = left + 16, bottom = top + 16;
            int old_index = -1;
            u8 state = (SC64_GRID_GET(visible, cell) ? 1 : 0) |
                       (SC64_GRID_GET(explored, cell) ? 2 : 0);
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
    for (row = row0; row < row1; row++)
        for (col = col0; col < col1; col++) {
            int cell = col + row * MAP_COLS;
            u8 state = (SC64_GRID_GET(visible, cell) ? 1 : 0) |
                       (SC64_GRID_GET(explored, cell) ? 2 : 0);
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
    if (!sc64_terrain_cache_reused_base ||
        !sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1) ||
#else
    if (!sc64_terrain_cache_reused_base ||
        !sc64_terrain_cache_view(sc64_terrain_cache_camx, sc64_terrain_cache_camy,
                                 &col0, &row0, &col1, &row1) ||
#endif
        col < col0 || col >= col1 || row < row0 || row >= row1)
        return 0;
    i = (row - row0) * (col1 - col0) + col - col0;
    return !sc64_terrain_cache_dirty[i];
}

#if defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)
#define SC64_TERRAIN_CACHE_KEY static_key
static int
sc64_terrain_store_key(int x, int y, unsigned int static_key)
#else
#define SC64_TERRAIN_CACHE_KEY (unsigned int)(SC64_TERRAIN_STATIC_KEY)
static int
sc64_terrain_store(int x, int y)
#endif
{
    int col0, row0, col1, row1, col, row, i = 0;
    if (!sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1) ||
        !sc64_terrain_cache_capture()) {
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
    for (row = row0; row < row1; row++)
        for (col = col0; col < col1; col++) {
            int cell = col + row * MAP_COLS;
            sc64_terrain_cache_mask[i++] =
                (SC64_GRID_GET(visible, cell) ? 1 : 0) |
                (SC64_GRID_GET(explored, cell) ? 2 : 0);
        }
    sc64_terrain_cache_mask_count = i;
#ifdef SC64_TERRAIN_ROW_MASKS_ACTIVE
    if (row1 - row0 > SC64_TERRAIN_CACHE_MAX_ROWS) {
        sc64_terrain_cache_valid = 0;
        sc64_terrain_cache_reused_base = 0;
        return 0;
    }
    for (row = row0; row < row1; row++) {
        sc64_terrain_row_visible[row - row0] =
            sc64_terrain_grid_run(visible, row, col0, col1 - col0);
        sc64_terrain_row_explored[row - row0] =
            sc64_terrain_grid_run(explored, row, col0, col1 - col0);
    }
#endif
#ifdef SC64_DIAGNOSTIC_TERRAIN
    SC64_TERRAIN_DIAG_ADD(sc64_terrain_diag_captured_pixels,
                          240 * SC64_TERRAIN_CACHE_HEIGHT);
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
