#include <assert.h>
#include <stdint.h>
#include <string.h>

#ifdef SC64_TERRAIN_CACHE_REAL_GFX
#include "../common/gba.h"
#ifdef SC64_TERRAN_CAMPAIGN
static int campaign_mission;
#endif
#else
typedef uint8_t u8;
typedef uint32_t u32;
#endif
enum { MAP_COLS = 64, MAP_ROWS = 64, MAP_CELLS = MAP_COLS * MAP_ROWS };
#define WORLD_W (MAP_COLS * 16)
#define WORLD_H (MAP_ROWS * 16)

#ifdef SC64_FOG_TEST_PACKED
static u8 visible[(MAP_CELLS + 7) / 8], explored[(MAP_CELLS + 7) / 8];
#define SC64_GRID_GET(grid, cell) \
    (((grid)[(unsigned)(cell) >> 3] >> ((unsigned)(cell) & 7u)) & 1u)
#define FOG_TEST_SET(grid, cell, value) do { \
    u8 mask_ = (u8)(1u << ((unsigned)(cell) & 7u)); \
    if (value) (grid)[(unsigned)(cell) >> 3] |= mask_; \
    else (grid)[(unsigned)(cell) >> 3] &= (u8)~mask_; \
} while (0)
#define FOG_TEST_GET(grid, cell) SC64_GRID_GET(grid, cell)
#else
static u8 visible[MAP_CELLS], explored[MAP_CELLS];
#define FOG_TEST_SET(grid, cell, value) ((grid)[cell] = (u8)((value) != 0))
#define FOG_TEST_GET(grid, cell) ((grid)[cell] != 0)
#endif
static void fog_test_fill(int visible_value, int explored_value)
{
    for (int cell = 0; cell < MAP_CELLS; ++cell) {
        FOG_TEST_SET(visible, cell, visible_value);
        FOG_TEST_SET(explored, cell, explored_value);
    }
}
#ifdef SC64_SOFT_FOG
/* The cache oracle uses terrain only for the tile being painted. Fog masks
 * may inspect neighboring fog state, but must never sample hidden terrain. */
static u8 terrain[MAP_CELLS];
#ifdef SC64_FOG_TEST_REAL_RASTER
static u8 raster_tiles[MAP_CELLS][256];
static u8 raster_shade_lut[256];
#endif
#endif
static unsigned int test_static_key;
#define SC64_TERRAIN_STATIC_KEY test_static_key
#ifndef SC64_TERRAIN_CACHE_REAL_GFX
static u8 pixels[160 * 240];
#ifdef SC64_TERRAN_CAMPAIGN
static int campaign_mission;
#endif
static int read_calls, blit_calls;
static int read_ok = 1;

static int
gfx_read_band(int y, int height, u8 *dst)
{
    read_calls++;
    if (!read_ok) return 0;
    memcpy(dst, pixels + y * 240, (size_t)height * 240);
    return 1;
}

static int
gfx_blit_band(int y, int height, const u8 *src)
{
    blit_calls++;
    memcpy(pixels + y * 240, src, (size_t)height * 240);
    return 1;
}

#ifdef SC64_TERRAIN_SCROLL
static int
gfx_blit_window(int y, int height, const u8 *src, int stride, int rows,
                int left, int top)
{
    for (int row = 0; row < height; ++row) {
        int source_row = top + row;
        if (source_row < 0 || source_row >= rows) continue;
        for (int col = 0; col < 240; ++col) {
            int source_col = left + col;
            if (source_col >= 0 && source_col < stride)
                pixels[(y + row) * 240 + col] = src[source_row * stride + source_col];
        }
    }
    return 1;
}
#endif
#endif

#include "../starcraft/sc64_terrain_cache.h"
#ifdef SC64_SOFT_FOG
#include "../starcraft/sc64_fog.h"
#endif
#if defined(SC64_TERRAIN_CACHE_VRAM) && defined(SC64_SOFT_FOG)
static void fog_vram_canary_init(void)
{
    volatile u8 *bytes = (volatile u8 *)(void *)SC64_OBJ_VRAM_FOG;
    for (unsigned i = 0; i < SC64_OBJ_VRAM_FOG_BYTES; ++i) bytes[i] = 0xa5u;
}
static void fog_vram_canary_check(void)
{
    volatile const u8 *bytes = (volatile const u8 *)(const void *)SC64_OBJ_VRAM_FOG;
    for (unsigned i = 0; i < SC64_OBJ_VRAM_FOG_BYTES; ++i) assert(bytes[i] == 0xa5u);
}
#else
#define fog_vram_canary_init() ((void)0)
#define fog_vram_canary_check() ((void)0)
#endif

#ifdef SC64_RENDER_KEY_ONCE
/* Exercise the same scenarios through the caller-supplied key API. */
#define sc64_terrain_cached(x, y) \
    sc64_terrain_cached_key((x), (y), test_static_key)
#define sc64_terrain_store(x, y) \
    sc64_terrain_store_key((x), (y), test_static_key)
#endif

#ifndef SC64_TERRAIN_CACHE_REAL_GFX
static void
fill_band(u8 value)
{
    memset(pixels + SC64_TERRAIN_CACHE_Y * 240, value,
           SC64_TERRAIN_CACHE_HEIGHT * 240);
}

#ifdef SC64_DIAGNOSTIC_TERRAIN
static void
test_diagnostic_counters(void)
{
    fog_test_fill(0, 0);
    sc64_terrain_cache_valid = 0;
    sc64_terrain_cache_reused_base = 0;
    sc64_terrain_requests = sc64_terrain_hits = sc64_terrain_partial = 0;
    sc64_terrain_diag_invalid_cache = 0;
    sc64_terrain_diag_mission_change = 0;
    sc64_terrain_diag_static_key_change = 0;
    sc64_terrain_diag_camera_oversize = 0;
    sc64_terrain_diag_camera_change = 0;
    sc64_terrain_diag_invalid_view = 0;
    sc64_terrain_diag_invalid_mask = 0;
    sc64_terrain_diag_blit_failure = 0;
    sc64_terrain_diag_capture_failure = 0;
    sc64_terrain_diag_reused_pixels = 0;
    sc64_terrain_diag_captured_pixels = 0;
    sc64_terrain_diag_captured_tiles = 0;
    sc64_terrain_diag_tiles_checked = 0;
    sc64_terrain_diag_dirty_tiles = 0;
    test_static_key = 3;
#ifdef SC64_TERRAN_CAMPAIGN
    campaign_mission = 0;
#endif

    assert(!sc64_terrain_cached(0, 0));
    assert(sc64_terrain_diag_invalid_cache == 1);
    fill_band(0x41);
    assert(sc64_terrain_store(0, 0));
    assert(sc64_terrain_diag_captured_pixels == 240u * SC64_TERRAIN_CACHE_HEIGHT);
    assert(sc64_terrain_diag_captured_tiles == (u32)sc64_terrain_cache_mask_count);
    assert(sc64_terrain_cached(0, 0));
    assert(sc64_terrain_diag_reused_pixels == 240u * SC64_TERRAIN_CACHE_HEIGHT);
    assert(sc64_terrain_diag_tiles_checked == (u32)sc64_terrain_cache_mask_count);
    assert(sc64_terrain_diag_dirty_tiles == 0);

    FOG_TEST_SET(visible, 0, 1);
    assert(!sc64_terrain_cached(0, 0));
    assert(sc64_terrain_diag_dirty_tiles == 1);
    assert(sc64_terrain_diag_tiles_checked == 2u * (u32)sc64_terrain_cache_mask_count);

    test_static_key++;
    assert(!sc64_terrain_cached(0, 0));
    assert(sc64_terrain_diag_static_key_change == 1);
    assert(sc64_terrain_store(0, 0));
#ifdef SC64_TERRAIN_SCROLL
    assert(!sc64_terrain_cached(240, 0));
    assert(sc64_terrain_diag_camera_oversize == 1);
    assert(sc64_terrain_store(0, 0));
    sc64_terrain_cache_camx = -1;
    assert(!sc64_terrain_cached(1, 0));
    assert(sc64_terrain_diag_invalid_view == 1);
#else
    assert(!sc64_terrain_cached(1, 0));
    assert(sc64_terrain_diag_camera_change == 1);
#endif

    assert(sc64_terrain_store(0, 0));
    ++sc64_terrain_cache_mask_count;
    assert(!sc64_terrain_cached(0, 0));
    assert(sc64_terrain_diag_invalid_mask == 1);
#ifdef SC64_TERRAN_CAMPAIGN
    assert(sc64_terrain_store(0, 0));
    campaign_mission++;
    assert(!sc64_terrain_cached(0, 0));
    assert(sc64_terrain_diag_mission_change == 1);
    campaign_mission = 0;
#endif
    read_calls = blit_calls = 0;
}
#endif

static void
test_restore_preserves_surrounding_pixels(void)
{
    u8 top[SC64_TERRAIN_CACHE_Y * 240];
    u8 bottom[(160 - SC64_TERRAIN_CACHE_Y - SC64_TERRAIN_CACHE_HEIGHT) * 240];
    memset(pixels, 0x17, sizeof(pixels));
    memset(top, 0x17, sizeof(top));
    memset(bottom, 0x17, sizeof(bottom));
    FOG_TEST_SET(visible, 10 + 5 * MAP_COLS, 1);
    fill_band(0x5a);
    assert(sc64_terrain_store(0, 0));
    assert(read_calls == 1);
    fill_band(0xc3); /* stand in for objects drawn after the terrain pass */
    assert(sc64_terrain_cached(0, 0));
    assert(blit_calls == 1);
    assert(sc64_terrain_tile_clean(10, 5));
    for (int i = 0; i < SC64_TERRAIN_CACHE_HEIGHT * 240; i++)
        assert(pixels[SC64_TERRAIN_CACHE_Y * 240 + i] == 0x5a);
    assert(memcmp(pixels, top, sizeof(top)) == 0);
    assert(memcmp(pixels + (SC64_TERRAIN_CACHE_Y + SC64_TERRAIN_CACHE_HEIGHT) * 240,
                  bottom, sizeof(bottom)) == 0);
}

static void
test_camera_and_fog_invalidation(void)
{
    fog_test_fill(0, 0);
    fill_band(0x31);
    assert(sc64_terrain_store(0, 0));
    assert(!sc64_terrain_cached(16, 0));

    /* A changed fog cell outside the viewport does not dirty this band. */
    FOG_TEST_SET(visible, 63 + 63 * MAP_COLS, 1);
    assert(sc64_terrain_cached(0, 0));
    assert(sc64_terrain_tile_clean(0, 0));
    /* At x=1, column 15 is visible by one pixel at the right edge. */
    assert(sc64_terrain_store(1, 0));
    FOG_TEST_SET(visible, 15, 1);
    assert(!sc64_terrain_cached(1, 0));
    assert(!sc64_terrain_tile_clean(15, 0));
    assert(sc64_terrain_tile_clean(14, 0));
    assert(sc64_terrain_store(1, 0));
    FOG_TEST_SET(visible, 15, 0);
    FOG_TEST_SET(explored, 15, 1);
    assert(!sc64_terrain_cached(1, 0));
    assert(!sc64_terrain_tile_clean(15, 0));
    assert(sc64_terrain_tile_clean(15, 1));
}

static void
test_edge_viewport_and_campaign_mission_key(void)
{
    fog_test_fill(0, 0);
    fill_band(0x49);
    assert(sc64_terrain_store(WORLD_W - 240, WORLD_H - SC64_TERRAIN_CACHE_HEIGHT));
    assert(sc64_terrain_cached(WORLD_W - 240, WORLD_H - SC64_TERRAIN_CACHE_HEIGHT));
    assert(!sc64_terrain_cached(WORLD_W - 241, WORLD_H - SC64_TERRAIN_CACHE_HEIGHT));
    assert(!sc64_terrain_tile_clean(49, 57));
#ifdef SC64_TERRAN_CAMPAIGN
    campaign_mission = 1;
    assert(!sc64_terrain_cached(WORLD_W - 240, WORLD_H - SC64_TERRAIN_CACHE_HEIGHT));
#endif
}

static void
test_failed_capture_does_not_validate_cache(void)
{
    read_ok = 0;
    assert(!sc64_terrain_store(32, 32));
    assert(!sc64_terrain_cached(32, 32));
    read_ok = 1;
}

static void
test_static_object_key_invalidates_full_base(void)
{
    fog_test_fill(0, 0);
    test_static_key = 17;
    fill_band(0x72);
    assert(sc64_terrain_store(0, 0));
    test_static_key++;
    assert(!sc64_terrain_cached(0, 0));
    assert(!sc64_terrain_tile_clean(0, 0));
}

static void
test_validated_tile_bounds(void)
{
    const int xs[] = {0, 1, 15, 16, WORLD_W - 256, WORLD_W - 241, WORLD_W - 240};
    const int ys[] = {0, 1, 15, 16, WORLD_H - 124, WORLD_H - 109, WORLD_H - 108};
    for (unsigned xi = 0; xi < sizeof(xs) / sizeof(xs[0]); ++xi)
        for (unsigned yi = 0; yi < sizeof(ys) / sizeof(ys[0]); ++yi) {
            int col0, row0, col1, row1;
            int x = xs[xi], y = ys[yi];
            assert(sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1));
            memset(visible, 0, sizeof(visible));
            memset(explored, 0, sizeof(explored));
            assert(sc64_terrain_store(x, y));
            visible[col0 + row0 * MAP_COLS] = 1;
            assert(!sc64_terrain_cached(x, y));
            assert(sc64_terrain_cache_reused_base);
            for (int row = row0 - 1; row <= row1; ++row)
                for (int col = col0 - 1; col <= col1; ++col) {
                    int clean = col >= col0 && col < col1 && row >= row0 && row < row1 &&
                                !(col == col0 && row == row0);
                    assert(sc64_terrain_tile_clean(col, row) == clean);
                }
            /* A failed reuse must invalidate the fast-path precondition. */
            assert(!sc64_terrain_cached(-1, y));
            assert(!sc64_terrain_cache_reused_base);
            assert(!sc64_terrain_tile_clean(col0, row0));
        }
}

int
main(void)
{
#ifdef SC64_DIAGNOSTIC_TERRAIN
    test_diagnostic_counters();
#endif
    test_restore_preserves_surrounding_pixels();
    test_camera_and_fog_invalidation();
    test_edge_viewport_and_campaign_mission_key();
    test_failed_capture_does_not_validate_cache();
    test_static_object_key_invalidates_full_base();
    test_validated_tile_bounds();
    return 0;
}
#else
#ifdef SC64_TERRAIN_CACHE_VRAM
static u8 obj_guard(unsigned offset)
{ return (u8)(0xa5u ^ (offset * 37u)); }

static void seed_obj_guards(void)
{
    for (unsigned i = 0; i < GBA_VRAM_SCRATCH_BYTES; ++i)
        GBA_VRAM_SCRATCH[i] = obj_guard(i);
}

static void verify_obj_guards(void)
{
    unsigned lo = SC64_OBJ_VRAM_CACHE_OFFSET;
    unsigned hi = lo + 240u * SC64_TERRAIN_CACHE_VRAM_ROWS;
    unsigned changed = 0;
    for (unsigned i = 0; i < GBA_VRAM_SCRATCH_BYTES; ++i) {
        if (i >= SC64_OBJ_VRAM_FOG_OFFSET &&
            i < SC64_OBJ_VRAM_FOG_OFFSET + SC64_OBJ_VRAM_FOG_BYTES)
            continue; /* the fog shade map has its own canary */
        if (i < lo || i >= hi) assert(GBA_VRAM_SCRATCH[i] == obj_guard(i));
        else changed += GBA_VRAM_SCRATCH[i] != obj_guard(i);
    }
    assert(changed > 0); /* The actual cache must have used its VRAM rows. */
}
#endif

#if defined(SC64_SOFT_FOG) && defined(SC64_FOG_TEST_REAL_RASTER)
static unsigned partial_cells_drawn, full_cells_drawn;
#endif
static void
render_cell(int col, int row, int camx, int camy, int partial)
{
    (void)partial;
    int cell = col + row * MAP_COLS;
    int x = col * 16 - camx;
    int y = 12 + row * 16 - camy;
#ifdef SC64_SOFT_FOG
#ifdef SC64_FOG_TEST_REAL_RASTER
    const u16 signature = sc64_fog_signature(visible, explored, MAP_COLS,
                                             MAP_ROWS, col, row);
    const unsigned center = sc64_fog_center_state(signature);
    /* Unknown cells pass no terrain pointer. The production raster must paint
     * opaque black without observing terrain for hidden cells. */
    if (partial) ++partial_cells_drawn; else ++full_cells_drawn;
    sc64_terrain_fog_cell(partial, x, y, signature,
                          center == SC64_FOG_UNKNOWN ? 0 : raster_tiles[cell],
                          raster_shade_lut);
#else
    unsigned signature = sc64_fog_signature(visible, explored, MAP_COLS,
                                            MAP_ROWS, col, row);
    unsigned digits[9];
    (void)partial;
    /* The helper encodes the row-major 3x3 neighborhood as a base-3 number.
     * Paint nine tiny state swatches so every signature digit affects pixels. */
    for (int i = 8; i >= 0; --i) {
        digits[i] = signature % 3u;
        signature /= 3u;
    }
    u8 color = FOG_TEST_GET(visible, cell) ? (u8)(32 + terrain[cell] % 192u) :
               (FOG_TEST_GET(explored, cell) ? 8 : 0);
    gfx_rect(x, y, 16, 16, color);
    for (int i = 0; i < 9; ++i) {
        int px = x + 1 + (i % 3) * 5;
        int py = y + 1 + (i / 3) * 5;
        gfx_rect(px, py, 4, 4, (u8)(12u + digits[i]));
    }
#endif
#else
    u8 color = FOG_TEST_GET(visible, cell) ? (u8)(32 + (col * 3 + row) % 224) :
               (FOG_TEST_GET(explored, cell) ? 8 : 0);
    /* Draw black too: a newly unknown cell must erase old terrain. */
    gfx_rect(x, y, 16, 16, color);
#endif
}

static void
render_fixture(int camx, int camy, int incremental)
{
    gfx_begin(0);
    if (incremental) {
        /* Prove the scroll path overwrites every destination pixel, including
         * newly exposed strips and world-edge margins. */
        gfx_rect(0, SC64_TERRAIN_CACHE_Y, 240, SC64_TERRAIN_CACHE_HEIGHT, 255);
        (void)sc64_terrain_cached(camx, camy);
#ifdef SC64_SOFT_FOG
        /* Whatever the lookup left behind must equal the from-scratch
         * signature of every cell of the view, every frame. */
        if (sc64_terrain_now_valid)
            for (int row = sc64_terrain_now_row0; row < sc64_terrain_now_row0 + sc64_terrain_now_rows; ++row)
                for (int col = sc64_terrain_now_col0; col < sc64_terrain_now_col0 + sc64_terrain_now_cols; ++col)
                    assert(sc64_terrain_now[(row - sc64_terrain_now_row0) * sc64_terrain_now_cols +
                                            (col - sc64_terrain_now_col0)] ==
                           sc64_terrain_state(col, row));
#endif
    }
    for (int row = camy / 16; row <= (camy + SC64_TERRAIN_CACHE_HEIGHT) / 16; row++)
        for (int col = camx / 16; col <= (camx + 240) / 16; col++) {
            if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) continue;
            if (incremental && sc64_terrain_tile_clean(col, row)) continue;
#ifdef SC64_SOFT_FOG
            render_cell(col, row, camx, camy,
                        incremental && sc64_terrain_tile_partial(col, row));
#else
            render_cell(col, row, camx, camy, 0);
#endif
        }
}

static void
compare_incremental_with_fresh(int camx, int camy)
{
    _Alignas(4) u8 incremental[240 * SC64_TERRAIN_CACHE_HEIGHT];
    _Alignas(4) u8 fresh[240 * SC64_TERRAIN_CACHE_HEIGHT];
    render_fixture(camx, camy, 1);
    assert(gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT, incremental));
    assert(sc64_terrain_store(camx, camy));
    render_fixture(camx, camy, 0);
    assert(gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT, fresh));
    assert(memcmp(incremental, fresh, sizeof(fresh)) == 0);
    render_fixture(camx, camy, 0);
    assert(sc64_terrain_store(camx, camy));
    fog_vram_canary_check();
}

#ifdef SC64_TERRAIN_SCROLL
static void
test_scroll_differential(void)
{
    static const int cameras[][2] = {
        /* Consecutive 1/2/3-pixel shifts exercise byte alignment variants
         * in gfx_blit_window. */
        {0, 0}, {1, 1}, {3, 3}, {6, 6}, {2, 3}, {7, 8},
        {15, 15}, {16, 16}, {17, 31}, {31, 17}, {63, 79}, {64, 80}, {80, 64},
        {48, 48}, {239, 107}, {478, 213}, {238, 106},
        {WORLD_W - 240, WORLD_H - SC64_TERRAIN_CACHE_HEIGHT},
        {WORLD_W - 241, WORLD_H - SC64_TERRAIN_CACHE_HEIGHT - 1},
        {WORLD_W - 480, WORLD_H - 216}, {0, 0},
        {240, 108}, {480, 216}, {WORLD_W - 240, WORLD_H - 108}
    };
    fog_test_fill(0, 0);
    for (int row = 0; row < MAP_ROWS; ++row)
        for (int col = 0; col < MAP_COLS; ++col) {
            int cell = col + row * MAP_COLS;
            FOG_TEST_SET(visible, cell, ((col * 7 + row * 3) % 5) != 0);
            FOG_TEST_SET(explored, cell, ((col + row) % 7) != 0);
        }
    test_static_key = 71;
    for (unsigned i = 0; i < sizeof(cameras) / sizeof(cameras[0]); ++i) {
        if (i == 7) {
            int col = cameras[i][0] / 16 + 4;
            int row = cameras[i][1] / 16 + 3;
            int cell = col + row * MAP_COLS;
            FOG_TEST_SET(visible, cell, !FOG_TEST_GET(visible, cell));
            FOG_TEST_SET(explored, cell, 1);
        }
        compare_incremental_with_fresh(cameras[i][0], cameras[i][1]);
    }
#if defined(SC64_SOFT_FOG) && defined(SC64_FOG_TEST_REAL_RASTER)
    /* The comparisons above must have exercised strip-only redraws. */
    assert(partial_cells_drawn > 0 && full_cells_drawn > 0);
#endif
    /* These jumps have no overlap and must use a complete repaint. */
    compare_incremental_with_fresh(100, 100);
    compare_incremental_with_fresh(400, 400);
    /* One-pixel overlap in both axes exercises clipped source edges at the
     * largest reusable offsets in each direction under ASan. */
    compare_incremental_with_fresh(100, 100);
    compare_incremental_with_fresh(339, 207);
    compare_incremental_with_fresh(100, 100);
    test_static_key++;
    compare_incremental_with_fresh(101, 101);
    compare_incremental_with_fresh(100, 100);
    ++sc64_terrain_cache_mask_count;
    gfx_begin(0);
    assert(!sc64_terrain_cached(101, 100));
    assert(!sc64_terrain_cache_valid);
    compare_incremental_with_fresh(101, 100);
}
#endif

#ifdef SC64_SOFT_FOG
/* The per-frame signature array must equal the nine-read definition for every
 * cell of every view, including views on each map edge and fogs that mix all
 * three classes, with and without a stale explored bit under a visible one. */
static void
test_signature_array(void)
{
    static const int views[][4] = {
        {0, 0, 16, 8}, {0, 0, 17, 9}, {1, 1, 17, 9}, {10, 5, 27, 13},
        {MAP_COLS - 17, MAP_ROWS - 9, MAP_COLS, MAP_ROWS},
        {MAP_COLS - 16, 0, MAP_COLS, 8}, {0, MAP_ROWS - 8, 16, MAP_ROWS},
        {31, 20, 32, 21}, {2, 0, 3, 9}, {0, 3, 17, 4},
    };
    unsigned seed = 12345u;
    for (int round = 0; round < 40; ++round) {
        for (int cell = 0; cell < MAP_CELLS; ++cell) {
            seed = seed * 1664525u + 1013904223u;
            unsigned r = (seed >> 16) % 7u;
            /* Round 0 is empty; rounds >= 1 use a different class mix each. */
            int vis = round && r < (unsigned)(1 + round % 4);
            int exp = round && (r % 3u == 0u || (round % 5 == 0 && r > 3u));
            FOG_TEST_SET(visible, cell, vis);
            FOG_TEST_SET(explored, cell, exp);
        }
        for (unsigned v = 0; v < sizeof(views) / sizeof(views[0]); ++v) {
            int col0 = views[v][0], row0 = views[v][1], col1 = views[v][2], row1 = views[v][3];
            sc64_terrain_fill_now(col0, row0, col1, row1);
            assert(sc64_terrain_now_valid);
            for (int row = row0; row < row1; ++row)
                for (int col = col0; col < col1; ++col) {
                    int index = (row - row0) * (col1 - col0) + (col - col0);
                    assert(sc64_terrain_now[index] == sc64_terrain_state(col, row));
                    assert(sc64_terrain_state_view(col, row) == sc64_terrain_state(col, row));
                }
        }
    }
    /* Oversize views are refused rather than overrunning the array. */
    sc64_terrain_fill_now(0, 0, 18, 9);
    assert(!sc64_terrain_now_valid);
    sc64_terrain_fill_now(0, 0, 17, 10);
    assert(!sc64_terrain_now_valid);
    sc64_terrain_now_valid = 0;
}

static void
test_soft_fog_neighbor_cache(void)
{
    test_signature_array();
    fog_test_fill(1, 1);
    for (int i = 0; i < MAP_CELLS; ++i) terrain[i] = (u8)(i * 13u + 7u);
#ifdef SC64_FOG_TEST_REAL_RASTER
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        for (int p = 0; p < 256; ++p)
            raster_tiles[cell][p] = (u8)(cell * 13u + p * 7u);
    for (int p = 0; p < 256; ++p) raster_shade_lut[p] = (u8)(p / 2u);
#endif
    test_static_key = 211;
    compare_incremental_with_fresh(100, 100);

    /* An interior fog change changes its own mask and masks of its neighbors. */
    int col = 12, row = 10, cell = col + row * MAP_COLS;
    FOG_TEST_SET(visible, cell, 0);
    assert(!sc64_terrain_cached(100, 100));
    assert(!sc64_terrain_tile_clean(col, row));
    assert(!sc64_terrain_tile_clean(col - 1, row));
    assert(!sc64_terrain_tile_clean(col + 1, row));
    assert(!sc64_terrain_tile_clean(col, row - 1));
    assert(!sc64_terrain_tile_clean(col, row + 1));
    compare_incremental_with_fresh(100, 100);

    /* Each outside-view edge/corner neighbor participates in a visible tile's
     * dependency signature even though that neighbor's pixels are offscreen. */
    static const int outside[][2] = {
        {5, 10}, {22, 10}, {12, 5}, {12, 14},
        {5, 5}, {22, 5}, {5, 14}, {22, 14}
    };
    static const int inside[][2] = {
        {6, 10}, {21, 10}, {12, 6}, {12, 13},
        {6, 6}, {21, 6}, {6, 13}, {21, 13}
    };
    for (unsigned i = 0; i < sizeof(outside) / sizeof(outside[0]); ++i) {
        compare_incremental_with_fresh(100, 100);
        int oc = outside[i][0], orow = outside[i][1];
        int ic = inside[i][0], irow = inside[i][1];
        FOG_TEST_SET(visible, oc + orow * MAP_COLS, 0);
        assert(!sc64_terrain_cached(100, 100));
        assert(!sc64_terrain_tile_clean(ic, irow));
        compare_incremental_with_fresh(100, 100);
    }

    /* Unrelated offscreen fog is not a dependency. Hidden terrain is not read
     * by the current tile or by any neighboring mask. */
    FOG_TEST_SET(visible, 50 + 50 * MAP_COLS, 0);
    assert(sc64_terrain_cached(100, 100));
    FOG_TEST_SET(explored, cell, 0); /* Make the poison target truly unknown. */
    assert(!sc64_terrain_cached(100, 100));
    _Alignas(4) u8 before_hidden_terrain[240 * SC64_TERRAIN_CACHE_HEIGHT];
    _Alignas(4) u8 after_hidden_terrain[240 * SC64_TERRAIN_CACHE_HEIGHT];
    render_fixture(100, 100, 0);
    assert(gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                         before_hidden_terrain));
#ifdef SC64_FOG_TEST_REAL_RASTER
    raster_tiles[cell][113] ^= 0x5au;
#else
    terrain[cell] ^= 0x5au;
#endif
    /* `cell` is now unknown and must paint the same black without its source. */
    render_fixture(100, 100, 0);
    assert(gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                         after_hidden_terrain));
    assert(memcmp(before_hidden_terrain, after_hidden_terrain,
                  sizeof(before_hidden_terrain)) == 0);

    /* Whole-grid load/reset-style replacement at a fixed camera is discovered
     * from the saved dependency signatures without an explicit cache epoch. */
    fog_test_fill(0, 0);
    assert(!sc64_terrain_cached(100, 100));
    compare_incremental_with_fresh(100, 100);

    /* Subtile and tile-crossing scrolls compare the new world signatures with
     * their old overlapping entries; exposed pixels still receive full paint. */
    compare_incremental_with_fresh(101, 101);
    FOG_TEST_SET(explored, 16 + 7 * MAP_COLS, 1);
    compare_incremental_with_fresh(102, 102);
    compare_incremental_with_fresh(116, 116);
    compare_incremental_with_fresh(0, 0);
    compare_incremental_with_fresh(WORLD_W - 240,
                                   WORLD_H - SC64_TERRAIN_CACHE_HEIGHT);
#if defined(SC64_TERRAIN_ROW_MASKS_ACTIVE) && defined(SC64_FOG_TEST_PACKED)
    /* At (128,128), the clipped 18-column halo begins at packed bit 7 and
     * spans through bit 24. This requires the fourth source byte. Column 24
     * and row 15 lie outside the view but affect its right/bottom corner. */
    fog_test_fill(1, 1);
    compare_incremental_with_fresh(128, 128);
    FOG_TEST_SET(visible, 24 + 10 * MAP_COLS, 0);
    assert(!sc64_terrain_cached(128, 128));
    assert(!sc64_terrain_tile_clean(23, 10));
    compare_incremental_with_fresh(128, 128);
    FOG_TEST_SET(visible, 24 + 15 * MAP_COLS, 0);
    assert(!sc64_terrain_cached(128, 128));
    assert(!sc64_terrain_tile_clean(23, 14));
    compare_incremental_with_fresh(128, 128);
#endif
}
#endif

int
main(void)
{
    _Alignas(4) u8 expected[240 * SC64_TERRAIN_CACHE_HEIGHT];
    _Alignas(4) u8 observed[240 * SC64_TERRAIN_CACHE_HEIGHT];
    gba_init();
#ifdef SC64_TERRAIN_CACHE_VRAM
    seed_obj_guards();
#endif
    fog_vram_canary_init(); /* after the guards: the fog map has its own pattern */
    test_static_key = 17;
    fog_test_fill(0, 0);
    for (int row = 0; row < 9; row++)
        for (int col = 0; col < 16; col++) {
            int cell = col + row * MAP_COLS;
            FOG_TEST_SET(visible, cell, ((col + row) % 3) != 0);
            FOG_TEST_SET(explored, cell, 1);
        }
    render_fixture(1, 3, 0);
    assert(gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT, expected));
    assert(sc64_terrain_store(1, 3));
    gfx_rect(24, 32, 80, 35, 255); /* dynamic units/structures over terrain */
    assert(sc64_terrain_cached(1, 3));
    assert(gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT, observed));
    assert(memcmp(expected, observed, sizeof(expected)) == 0);
    fog_vram_canary_check();
    render_fixture(1, 3, 0); /* fresh uncached draw through the real GBA_HOST renderer */
    assert(gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT, observed));
    assert(memcmp(expected, observed, sizeof(expected)) == 0);

    /* Toggle sight, reveal previously hidden cells, then reset exploration.
       Each incremental redraw must equal a clean full render byte for byte. */
    FOG_TEST_SET(visible, 3 + 2 * MAP_COLS, 0);
    FOG_TEST_SET(explored, 3 + 2 * MAP_COLS, 0);
    FOG_TEST_SET(visible, 7 + 4 * MAP_COLS, 1);
    FOG_TEST_SET(explored, 7 + 4 * MAP_COLS, 1);
    FOG_TEST_SET(visible, 10 + 1 * MAP_COLS, 0);
    FOG_TEST_SET(explored, 10 + 1 * MAP_COLS, 1);
    render_fixture(1, 3, 1);
    compare_incremental_with_fresh(1, 3);

    fog_test_fill(0, 0);
    render_fixture(1, 3, 1); /* changed visible cells are repainted black */
    compare_incremental_with_fresh(1, 3);

    render_fixture(17, 11, 1);
    compare_incremental_with_fresh(17, 11);
    test_static_key++;
    gfx_begin(0);
    assert(!sc64_terrain_cached(17, 11));
    render_fixture(17, 11, 1);
    compare_incremental_with_fresh(17, 11);
#ifdef SC64_TERRAIN_SCROLL
    test_scroll_differential();
#endif
#ifdef SC64_TERRAIN_CACHE_VRAM
    verify_obj_guards();
#endif
#ifdef SC64_SOFT_FOG
    test_soft_fog_neighbor_cache();
#endif
    return 0;
}
#endif
