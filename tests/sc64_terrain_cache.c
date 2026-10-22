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
#endif
enum { MAP_COLS = 64, MAP_ROWS = 64, MAP_CELLS = MAP_COLS * MAP_ROWS };
#define WORLD_W (MAP_COLS * 16)
#define WORLD_H (MAP_ROWS * 16)

static u8 visible[MAP_CELLS], explored[MAP_CELLS];
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

#ifndef SC64_TERRAIN_CACHE_REAL_GFX
static void
fill_band(u8 value)
{
    memset(pixels + SC64_TERRAIN_CACHE_Y * 240, value,
           SC64_TERRAIN_CACHE_HEIGHT * 240);
}

static void
test_restore_preserves_surrounding_pixels(void)
{
    u8 top[SC64_TERRAIN_CACHE_Y * 240];
    u8 bottom[(160 - SC64_TERRAIN_CACHE_Y - SC64_TERRAIN_CACHE_HEIGHT) * 240];
    memset(pixels, 0x17, sizeof(pixels));
    memset(top, 0x17, sizeof(top));
    memset(bottom, 0x17, sizeof(bottom));
    visible[10 + 5 * MAP_COLS] = 1;
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
    memset(visible, 0, sizeof(visible));
    memset(explored, 0, sizeof(explored));
    fill_band(0x31);
    assert(sc64_terrain_store(0, 0));
    assert(!sc64_terrain_cached(16, 0));

    /* A changed fog cell outside the viewport does not dirty this band. */
    visible[63 + 63 * MAP_COLS] = 1;
    assert(sc64_terrain_cached(0, 0));
    assert(sc64_terrain_tile_clean(0, 0));
    /* At x=1, column 15 is visible by one pixel at the right edge. */
    assert(sc64_terrain_store(1, 0));
    visible[15] = 1;
    assert(!sc64_terrain_cached(1, 0));
    assert(!sc64_terrain_tile_clean(15, 0));
    assert(sc64_terrain_tile_clean(14, 0));
    assert(sc64_terrain_store(1, 0));
    visible[15] = 0;
    explored[15] = 1;
    assert(!sc64_terrain_cached(1, 0));
    assert(!sc64_terrain_tile_clean(15, 0));
    assert(sc64_terrain_tile_clean(15, 1));
}

static void
test_edge_viewport_and_campaign_mission_key(void)
{
    memset(visible, 0, sizeof(visible));
    memset(explored, 0, sizeof(explored));
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
    memset(visible, 0, sizeof(visible));
    memset(explored, 0, sizeof(explored));
    test_static_key = 17;
    fill_band(0x72);
    assert(sc64_terrain_store(0, 0));
    test_static_key++;
    assert(!sc64_terrain_cached(0, 0));
    assert(!sc64_terrain_tile_clean(0, 0));
}

int
main(void)
{
    test_restore_preserves_surrounding_pixels();
    test_camera_and_fog_invalidation();
    test_edge_viewport_and_campaign_mission_key();
    test_failed_capture_does_not_validate_cache();
    test_static_object_key_invalidates_full_base();
    return 0;
}
#else
static void
render_cell(int col, int row, int camx, int camy)
{
    int cell = col + row * MAP_COLS;
    int x = col * 16 - camx;
    int y = 12 + row * 16 - camy;
    u8 color = visible[cell] ? (u8)(32 + (col * 3 + row) % 224) :
               (explored[cell] ? 8 : 0);
    /* Draw black too: a newly unknown cell must erase old terrain. */
    gfx_rect(x, y, 16, 16, color);
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
    }
    for (int row = camy / 16; row <= (camy + SC64_TERRAIN_CACHE_HEIGHT) / 16; row++)
        for (int col = camx / 16; col <= (camx + 240) / 16; col++) {
            if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) continue;
            if (incremental && sc64_terrain_tile_clean(col, row)) continue;
            render_cell(col, row, camx, camy);
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
}

static void
test_scroll_differential(void)
{
    static const int cameras[][2] = {
        {0, 0}, {1, 1}, {2, 3}, {7, 8}, {15, 15}, {16, 16},
        {17, 31}, {31, 17}, {63, 79}, {64, 80}, {80, 64},
        {48, 48}, {239, 107}, {478, 213}, {238, 106},
        {WORLD_W - 240, WORLD_H - SC64_TERRAIN_CACHE_HEIGHT},
        {WORLD_W - 241, WORLD_H - SC64_TERRAIN_CACHE_HEIGHT - 1},
        {WORLD_W - 480, WORLD_H - 216}, {0, 0},
        {240, 108}, {480, 216}, {WORLD_W - 240, WORLD_H - 108}
    };
    memset(visible, 0, sizeof(visible));
    memset(explored, 0, sizeof(explored));
    for (int row = 0; row < MAP_ROWS; ++row)
        for (int col = 0; col < MAP_COLS; ++col) {
            int cell = col + row * MAP_COLS;
            visible[cell] = ((col * 7 + row * 3) % 5) != 0;
            explored[cell] = ((col + row) % 7) != 0;
        }
    test_static_key = 71;
    for (unsigned i = 0; i < sizeof(cameras) / sizeof(cameras[0]); ++i) {
        if (i == 7) {
            int col = cameras[i][0] / 16 + 4;
            int row = cameras[i][1] / 16 + 3;
            int cell = col + row * MAP_COLS;
            visible[cell] = !visible[cell];
            explored[cell] = 1;
        }
        compare_incremental_with_fresh(cameras[i][0], cameras[i][1]);
    }
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

int
main(void)
{
    _Alignas(4) u8 expected[240 * SC64_TERRAIN_CACHE_HEIGHT];
    _Alignas(4) u8 observed[240 * SC64_TERRAIN_CACHE_HEIGHT];
    gba_init();
    test_static_key = 17;
    memset(visible, 0, sizeof(visible));
    memset(explored, 0, sizeof(explored));
    for (int row = 0; row < 9; row++)
        for (int col = 0; col < 16; col++) {
            int cell = col + row * MAP_COLS;
            visible[cell] = ((col + row) % 3) != 0;
            explored[cell] = 1;
        }
    render_fixture(1, 3, 0);
    assert(gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT, expected));
    assert(sc64_terrain_store(1, 3));
    gfx_rect(24, 32, 80, 35, 255); /* dynamic units/structures over terrain */
    assert(sc64_terrain_cached(1, 3));
    assert(gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT, observed));
    assert(memcmp(expected, observed, sizeof(expected)) == 0);
    render_fixture(1, 3, 0); /* fresh uncached draw through the real GBA_HOST renderer */
    assert(gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT, observed));
    assert(memcmp(expected, observed, sizeof(expected)) == 0);

    /* Toggle sight, reveal previously hidden cells, then reset exploration.
       Each incremental redraw must equal a clean full render byte for byte. */
    visible[3 + 2 * MAP_COLS] = 0;
    explored[3 + 2 * MAP_COLS] = 0;
    visible[7 + 4 * MAP_COLS] = 1;
    explored[7 + 4 * MAP_COLS] = 1;
    visible[10 + 1 * MAP_COLS] = 0;
    explored[10 + 1 * MAP_COLS] = 1;
    render_fixture(1, 3, 1);
    compare_incremental_with_fresh(1, 3);

    memset(visible, 0, sizeof(visible));
    memset(explored, 0, sizeof(explored));
    render_fixture(1, 3, 1); /* changed visible cells are repainted black */
    compare_incremental_with_fresh(1, 3);

    render_fixture(17, 11, 1);
    compare_incremental_with_fresh(17, 11);
    test_static_key++;
    gfx_begin(0);
    assert(!sc64_terrain_cached(17, 11));
    render_fixture(17, 11, 1);
    compare_incremental_with_fresh(17, 11);
    test_scroll_differential();
    return 0;
}
#endif
