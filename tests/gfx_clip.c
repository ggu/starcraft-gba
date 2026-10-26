/* Pixel oracle for the optional scene-wide vertical clip. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.h"

#ifndef GBA_SCENE_CLIP
#error "Build this fixture with GBA_SCENE_CLIP enabled"
#endif

enum { WIDTH = 240, HEIGHT = 160, PIXELS = WIDTH * HEIGHT };
static _Alignas(4) u8 baseline[PIXELS];
static _Alignas(4) u8 background[PIXELS];
static _Alignas(4) u8 clipped[PIXELS];
static u8 src16[16 * 16 + 4];
static u8 src_sprite[255 * 4 + 4];
#if defined(D2_DEN_OF_EVIL) || defined(GBA_HOST)
static u8 src_2x[17 * 9 + 4];
#endif

static void
capture(u8 *out)
{
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            out[y * WIDTH + x] = gfx_test_pixel(x, y);
}

static u8
pattern(int x, int y)
{
    return (u8)(1 + ((x * 13 + y * 7 + (x ^ y)) % 31));
}

static void
draw_pattern(void)
{
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            gfx_pixel(x, y, pattern(x, y));
}

static void
init_sources(void)
{
    for (int i = 0; i < 16 * 16; ++i)
        src16[i + 1] = (u8)(i % 5 ? (1 + (i * 11) % 31) : 0);
    for (int i = 0; i < 255 * 4; ++i)
        src_sprite[i + 1] = (u8)(i % 7 ? (1 + (i * 17 + 3) % 31) : 0);
#if defined(D2_DEN_OF_EVIL) || defined(GBA_HOST)
    for (int i = 0; i < 17 * 9; ++i)
        src_2x[i + 1] = (u8)(i % 4 ? (1 + (i * 9) % 31) : 0);
#endif
}

static void
draw_operations(void)
{
    gfx_pixel(0, 0, 31);
    gfx_pixel(239, 159, 30);
    gfx_rect(-5, 3, 27, 19, 4);
    gfx_rect(224, 145, 28, 22, 5);
    gfx_frame(4, 116, 232, 49, 6);
    gfx_line(-8, 9, 245, 148, 7);
    gfx_line(239, 2, 3, 159, 8);
    gfx_circle(17, 3, 12, 9);
    gfx_circle(219, 150, 11, 10);
    gfx_diamond(115, 12, 31, 13, 11);
    gfx_diamond(125, 132, 27, 19, 12);
    gfx_text(3, 1, "CLIP TEST 0123", 13);
    gfx_text(192, 151, "EDGE", 14);
    gfx_num(204, 4, 12345, 15);
    gfx_bar(229, 15, 9, 100, 3, 9, 16, 17);
    gfx_blit16(-7, 7, src16 + 1);
    gfx_blit16(229, 112, src16 + 1);
    gfx_blit_sprite(-121, 11, 255, 3, src_sprite + 1);
    gfx_blit_sprite(237, 116, 7, 31, src_sprite + 1);
    gfx_blit_sprite(31, 78, 63, 4, src_sprite + 1);
#if defined(D2_DEN_OF_EVIL) || defined(GBA_HOST)
    gfx_blit_sprite_2x(225, 117, 17, 9, src_2x + 1);
#endif
}

static void
render_background(u8 *out)
{
    gfx_begin(0);
    draw_pattern();
    capture(out);
}

static void
render_baseline(void)
{
    gfx_begin(0);
    draw_pattern();
    draw_operations();
    capture(baseline);
}

static void
check_clip_range(int top, int bottom)
{
    render_background(background);
    render_baseline();

    gfx_begin(0);
    draw_pattern();
    gfx_set_clip_y(top, bottom);
    assert(gfx_get_clip_y() == (u16)(top | (bottom << 8)));
    draw_operations();
    capture(clipped);

    for (int y = 0; y < HEIGHT; ++y) {
        const u8 *expected = y >= top && y < bottom ? baseline : background;
        for (int x = 0; x < WIDTH; ++x) {
            int i = y * WIDTH + x;
            if (clipped[i] != expected[i]) {
                fprintf(stderr,
                        "clip [%d,%d) mismatch at (%d,%d): got %u expected %u\n",
                        top, bottom, x, y, clipped[i], expected[i]);
                assert(0);
            }
        }
    }
}

static void
test_invalid_ranges_are_ignored(void)
{
    render_background(background);
    gfx_begin(0);
    draw_pattern();
    gfx_set_clip_y(20, 40);
    gfx_set_clip_y(-1, 40);
    gfx_set_clip_y(41, 40);
    gfx_set_clip_y(0, 161);
    assert(gfx_get_clip_y() == (u16)(20 | (40 << 8)));
    gfx_pixel(1, 19, 31);
    gfx_pixel(1, 20, 30);
    gfx_pixel(1, 39, 29);
    gfx_pixel(1, 40, 28);
    assert(gfx_test_pixel(1, 19) == background[19 * WIDTH + 1]);
    assert(gfx_test_pixel(1, 20) == 30);
    assert(gfx_test_pixel(1, 39) == 29);
    assert(gfx_test_pixel(1, 40) == background[40 * WIDTH + 1]);
}

static void
test_begin_resets_clip(void)
{
    gfx_begin(0);
    gfx_set_clip_y(12, 120);
    gfx_pixel(4, 4, 31);
    assert(gfx_test_pixel(4, 4) == 0);

    /* Both begin variants establish a new scene with the full-screen clip. */
    gfx_begin(0);
    assert(gfx_get_clip_y() == (u16)(160 << 8));
    gfx_pixel(4, 4, 31);
    assert(gfx_test_pixel(4, 4) == 31);
    gfx_set_clip_y(12, 120);
    gfx_begin_preserve();
    assert(gfx_get_clip_y() == (u16)(160 << 8));
    gfx_pixel(4, 4, 30);
    assert(gfx_test_pixel(4, 4) == 30);
}

int
main(void)
{
    static const int ranges[][2] = {
        {0, 160}, {0, 0}, {160, 160}, {12, 120},
        {0, 1}, {1, 159}, {159, 160}, {5, 6}, {119, 120}
    };
    gba_init();
    init_sources();
    test_begin_resets_clip();
    for (unsigned i = 0; i < sizeof(ranges) / sizeof(ranges[0]); ++i)
        check_clip_range(ranges[i][0], ranges[i][1]);
    test_invalid_ranges_are_ignored();
    return 0;
}
