/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exact host framebuffer oracle for the public indexed sprite blitter. */
#include "../common/gba.h"

#include <stdio.h>
#include <stdlib.h>

enum { FB_WIDTH = 240, FB_HEIGHT = 160, FB_PIXELS = FB_WIDTH * FB_HEIGHT };

static u8 expected[FB_PIXELS];
static u32 random_state = 0x6d2b79f5u;

static u32 next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static u8 patterned_background(int x, int y)
{
    return (u8)((x * 13 + y * 29 + (x ^ y) * 7) & 0xff);
}

static void begin_patterned_frame(void)
{
    gfx_begin(0x5a);
    for (int y = 0; y < FB_HEIGHT; ++y) {
        for (int x = 0; x < FB_WIDTH; ++x) {
            u8 pixel = patterned_background(x, y);
            expected[y * FB_WIDTH + x] = pixel;
            gfx_pixel(x, y, pixel);
        }
    }
}

static u8 pattern_pixel(unsigned pattern, size_t index)
{
    switch (pattern) {
    case 0: return 0; /* fully transparent */
    case 1: return (u8)(1u + (index % 255u)); /* opaque, includes high indices */
    case 2:
        switch (index & 7u) {
        case 0: return 0;
        case 1: return 0x80;
        case 2: return 0xff;
        case 3: return 0x01;
        case 4: return 0;
        case 5: return 0xa5;
        case 6: return 0;
        default: return 0x7f;
        }
    default:
        return (next_random() & 3u) == 0 ? 0 : (u8)next_random();
    }
}

static void apply_oracle(int x, int y, int width, int height,
                         const u8 *source)
{
    for (int sy = 0; sy < height; ++sy) {
        for (int sx = 0; sx < width; ++sx) {
            u8 pixel = source[(size_t)sy * (size_t)width + (size_t)sx];
            int dx = x + sx, dy = y + sy;
            if (pixel && dx >= 0 && dx < FB_WIDTH && dy >= 0 && dy < FB_HEIGHT)
                expected[dy * FB_WIDTH + dx] = pixel;
        }
    }
}

static void check_frame(const char *label, int x, int y,
                        int width, int height, unsigned pattern,
                        unsigned source_offset)
{
    size_t length = (size_t)width * (size_t)height;
    u8 *allocation = (u8 *)malloc(length + source_offset);
    if (!allocation) {
        fprintf(stderr, "allocation failed for %s (%dx%d)\n", label, width, height);
        exit(1);
    }
    u8 *source = allocation + source_offset;
    for (size_t i = 0; i < length; ++i) source[i] = pattern_pixel(pattern, i);

    begin_patterned_frame();
    apply_oracle(x, y, width, height, source);
    gfx_blit_sprite(x, y, width, height, source);
    for (int py = 0; py < FB_HEIGHT; ++py) {
        for (int px = 0; px < FB_WIDTH; ++px) {
            u8 actual = gfx_test_pixel(px, py);
            u8 wanted = expected[py * FB_WIDTH + px];
            if (actual != wanted) {
                fprintf(stderr,
                        "%s: mismatch at (%d,%d), sprite=(%d,%d %dx%d), "
                        "pattern=%u offset=%u: expected=%u actual=%u\n",
                        label, px, py, x, y, width, height, pattern,
                        source_offset, wanted, actual);
                free(allocation);
                exit(1);
            }
        }
    }
    gfx_end();
    free(allocation);
}

static void check_invalid(const char *label, int x, int y, int width, int height,
                          const u8 *source)
{
    begin_patterned_frame();
    gfx_blit_sprite(x, y, width, height, source);
    for (int py = 0; py < FB_HEIGHT; ++py) {
        for (int px = 0; px < FB_WIDTH; ++px) {
            u8 actual = gfx_test_pixel(px, py);
            u8 wanted = expected[py * FB_WIDTH + px];
            if (actual != wanted) {
                fprintf(stderr, "%s modified framebuffer at (%d,%d)\n",
                        label, px, py);
                exit(1);
            }
        }
    }
    gfx_end();
}

static void dimensions_and_offsets(void)
{
    static const int sizes[] = { 1, 2, 3, 4, 7, 16, 31, 32, 63, 64, 255, 256 };
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        int width = sizes[i];
        int height = sizes[(i * 5u + 3u) % (sizeof(sizes) / sizeof(sizes[0]))];
        int positions[][2] = {
            { 0, 0 }, { 1, 2 }, { 2, 1 },
            { -1, 3 }, { 5, -1 },
            { 239 - width + 1, 7 }, { 8, 159 - height + 1 },
            { -width + 1, 0 }, { 0, -height + 1 },
            { 239, 159 }, { -width + 1, -height + 1 },
            { 239 - width + 1, -height + 1 },
            { -width + 1, 159 - height + 1 },
        };
        for (unsigned p = 0; p < sizeof(positions) / sizeof(positions[0]); ++p)
            for (unsigned pattern = 0; pattern < 3; ++pattern)
                for (unsigned offset = 0; offset < 4; ++offset)
                    check_frame("dimension/clip", positions[p][0], positions[p][1],
                                width, height, pattern, offset);
    }

    /* Exercise every listed size in both axes and odd/even pairings, including
     * the widest legal source rows and heights. */
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        check_frame("square dimensions", 17, 19, sizes[i], sizes[i], 2, i & 3u);
        check_frame("cross dimensions", 21, 23, sizes[i], sizes[11u - i], 1,
                    (i + 1u) & 3u);
    }
}

static void random_moderate_cases(void)
{
    for (unsigned i = 0; i < 500; ++i) {
        int width = 1 + (int)(next_random() % 64u);
        int height = 1 + (int)(next_random() % 64u);
        int x = (int)(next_random() % (unsigned)(FB_WIDTH + width + 1)) - width;
        int y = (int)(next_random() % (unsigned)(FB_HEIGHT + height + 1)) - height;
        check_frame("seeded random", x, y, width, height, 3,
                    next_random() & 3u);
    }
}

static void invalid_arguments(void)
{
    static const u8 tiny_source = 0x80;
    check_invalid("null source", 0, 0, 1, 1, NULL);
    check_invalid("zero width", 0, 0, 0, 1, &tiny_source);
    check_invalid("zero height", 0, 0, 1, 0, &tiny_source);
    check_invalid("negative width", 0, 0, -1, 1, &tiny_source);
    check_invalid("negative height", 0, 0, 1, -1, &tiny_source);
    check_invalid("oversized width", 0, 0, 257, 1, &tiny_source);
    check_invalid("oversized height", 0, 0, 1, 257, &tiny_source);
    check_invalid("offscreen right", FB_WIDTH, 20, 1, 1, &tiny_source);
    check_invalid("offscreen bottom", 20, FB_HEIGHT, 1, 1, &tiny_source);
    check_invalid("offscreen left", -1, 20, 1, 1, &tiny_source);
    check_invalid("offscreen top", 20, -1, 1, 1, &tiny_source);
}

int main(void)
{
    gba_init();
    dimensions_and_offsets();
    random_moderate_cases();
    invalid_arguments();
    puts("gfx_blit_sprite exact-pixel differential tests passed");
    return 0;
}
