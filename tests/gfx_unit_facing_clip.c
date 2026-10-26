/* Pixel oracle for horizontal unit-frame flips under the scene clip. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.h"

#ifndef SC64_UNIT_FACING
#error "Build this fixture with SC64_UNIT_FACING enabled"
#endif
#ifndef GBA_SCENE_CLIP
#error "Build this fixture with GBA_SCENE_CLIP enabled"
#endif

enum { W = 240, H = 160, PIXELS = W * H };
static u8 expected[PIXELS];
static u8 background[PIXELS];
static u8 source[19 * 13 + 2];

static u8 bg(int x, int y)
{
    return (u8)(1 + ((x * 17 + y * 31 + (x ^ y)) % 251));
}

static void begin(void)
{
    gfx_begin(0);
    gfx_set_clip_y(0, 160);
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        u8 p = bg(x, y);
        background[y * W + x] = expected[y * W + x] = p;
        gfx_pixel(x, y, p);
    }
}

static void oracle_hflip(int x, int y, int width, int height,
                         const u8 *pixels, int clip_top, int clip_bottom)
{
    for (int sy = 0; sy < height; ++sy) for (int sx = 0; sx < width; ++sx) {
        int dx = x + width - sx - 1, dy = y + sy;
        u8 p = pixels[sy * width + sx];
        if (p && dx >= 0 && dx < W && dy >= clip_top && dy < clip_bottom && dy < H)
            expected[dy * W + dx] = p;
    }
}

static void verify(const char *label, int x, int y, int width, int height,
                   int top, int bottom)
{
    for (int py = 0; py < H; ++py) for (int px = 0; px < W; ++px) {
        u8 actual = gfx_test_pixel(px, py);
        u8 want = expected[py * W + px];
        if (actual != want) {
            fprintf(stderr, "%s mismatch at (%d,%d), sprite(%d,%d %dx%d), clip[%d,%d): got %u expected %u\n",
                    label, px, py, x, y, width, height, top, bottom, actual, want);
            assert(0);
        }
    }
}

int main(void)
{
    static const int cases[][4] = {
        {-4, -2, 19, 13}, {-1, 10, 19, 13}, {1, 11, 19, 13},
        {2, 35, 19, 13}, {221, 77, 19, 13}, {235, 119, 19, 13},
        {7, 154, 19, 13}
    };
    for (size_t i = 0; i < sizeof(source) - 2; ++i)
        source[i + 1] = (u8)(i % 5 ? 1 + ((i * 29 + 7) % 255) : 0);
    source[0] = 0xa7;
    source[sizeof(source) - 1] = 0xb9;
    gba_init();
    for (int range = 0; range < 5; ++range) {
        static const int clips[][2] = {{0,160},{0,0},{12,120},{35,42},{150,160}};
        int top = clips[range][0], bottom = clips[range][1];
        for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
            int x = cases[i][0], y = cases[i][1], w = cases[i][2], h = cases[i][3];
            begin();
            gfx_set_clip_y(top, bottom);
            oracle_hflip(x, y, w, h, source + 1, top, bottom);
            gfx_blit_sprite_hflip(x, y, w, h, source + 1);
            verify("horizontal flip", x, y, w, h, top, bottom);
        }
    }
    /* Invalid clip ranges must leave the last valid clip intact. */
    begin();
    gfx_set_clip_y(8, 40);
    oracle_hflip(3, 5, 19, 13, source + 1, 8, 40);
    gfx_set_clip_y(-1, 12);
    assert(gfx_get_clip_y() == (u16)(8 | (40 << 8)));
    gfx_blit_sprite_hflip(3, 5, 19, 13, source + 1);
    verify("invalid range preserves clip", 3, 5, 19, 13, 8, 40);
    assert(source[0] == 0xa7 && source[sizeof(source) - 1] == 0xb9);
    puts("unit hflip scene clip and halfword edges: ok");
    return 0;
}
