/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exact host framebuffer oracle for the optional phase-packed sprite blitter. */
#include "../common/gba.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define FB_WIDTH 240
#define FB_HEIGHT 160
#define FB_PIXELS (FB_WIDTH * FB_HEIGHT)

typedef struct {
    GbaSpriteWordFrame frame;
    u32 *storage[4];
} PackedAllocation;

static void set_clip(int top, int bottom)
{
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(top, bottom);
#else
    (void)top; (void)bottom;
#endif
}

static u32 rng = 0x92b3184du;
static u32 next_random(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static void pack_frame(PackedAllocation *packed, int width, int height,
                       const u8 *pixels)
{
    packed->frame.width = (u16)width;
    packed->frame.height = (u16)height;
    for (int phase = 0; phase < 4; ++phase) {
        int groups = (phase + width + 3) / 4;
        size_t words = (size_t)height * (size_t)groups * 2u;
        packed->storage[phase] = (u32 *)calloc(words, sizeof(u32));
        if (!packed->storage[phase]) exit(2);
        packed->frame.phase_words[phase] = packed->storage[phase];
        for (int y = 0; y < height; ++y) {
            for (int group = 0; group < groups; ++group) {
                u32 color = 0, mask = 0;
                for (int lane = 0; lane < 4; ++lane) {
                    int sx = group * 4 + lane - phase;
                    u8 pixel = sx >= 0 && sx < width ? pixels[(size_t)y * width + sx] : 0;
                    color |= (u32)pixel << (lane * 8);
                    if (pixel) mask |= 0xffu << (lane * 8);
                }
                size_t offset = ((size_t)y * groups + group) * 2u;
                packed->storage[phase][offset] = color;
                packed->storage[phase][offset + 1] = mask;
            }
        }
    }
}

static void free_frame(PackedAllocation *packed)
{
    for (int phase = 0; phase < 4; ++phase) free(packed->storage[phase]);
}

static void patterned_frame(void)
{
    gfx_begin(0x53);
    for (int y = 0; y < FB_HEIGHT; ++y)
        for (int x = 0; x < FB_WIDTH; ++x)
            gfx_pixel(x, y, (u8)((x * 13 + y * 29 + (x ^ y) * 7) & 255));
}

static void compare_case(int width, int height, int x, int y, int clip_top,
                         int clip_bottom, unsigned pattern)
{
    size_t pixels_count = (size_t)width * (size_t)height;
    u8 *pixels = (u8 *)malloc(pixels_count);
    u8 *expected = (u8 *)malloc(FB_PIXELS);
    if (!pixels || !expected) exit(2);
    for (size_t i = 0; i < pixels_count; ++i) {
        switch (pattern) {
        case 0: pixels[i] = 0; break;
        case 1: pixels[i] = (u8)(1u + (i % 255u)); break;
        case 2: pixels[i] = (i & 3u) == 0 ? 0 : (u8)next_random(); break;
        default: pixels[i] = (next_random() & 1u) ? (u8)next_random() : 0; break;
        }
    }
    PackedAllocation packed = {0};
    pack_frame(&packed, width, height, pixels);

    patterned_frame();
    set_clip(clip_top, clip_bottom);
    gfx_blit_sprite(x, y, width, height, pixels);
    for (int py = 0; py < FB_HEIGHT; ++py)
        for (int px = 0; px < FB_WIDTH; ++px)
            expected[py * FB_WIDTH + px] = gfx_test_pixel(px, py);

    patterned_frame();
    set_clip(clip_top, clip_bottom);
    gfx_blit_sprite_words(x, y, &packed.frame);
    for (int py = 0; py < FB_HEIGHT; ++py) {
        for (int px = 0; px < FB_WIDTH; ++px) {
            u8 actual = gfx_test_pixel(px, py);
            u8 wanted = expected[py * FB_WIDTH + px];
            if (actual != wanted) {
                fprintf(stderr,
                        "packed mismatch at (%d,%d), sprite=(%d,%d %dx%d), "
                        "clip=[%d,%d), pattern=%u: expected=%u actual=%u\n",
                        px, py, x, y, width, height, clip_top, clip_bottom,
                        pattern, wanted, actual);
                exit(1);
            }
        }
    }
    free_frame(&packed);
    free(expected);
    free(pixels);
}

static void invalid_cases(void)
{
    GbaSpriteWordFrame frame = {1, 1, {0, 0, 0, 0}};
    static const u32 pixel[2] = {0x0000007fu, 0x000000ffu};
    for (int i = 0; i < 4; ++i) frame.phase_words[i] = pixel;
    set_clip(0, 160);
    patterned_frame();
    gfx_blit_sprite_words(INT_MIN, 0, &frame);
    gfx_blit_sprite_words(INT_MAX, 0, &frame);
    gfx_blit_sprite_words(0, INT_MIN, &frame);
    gfx_blit_sprite_words(0, INT_MAX, &frame);
    for (int i = 0; i < FB_HEIGHT; ++i)
        for (int x = 0; x < FB_WIDTH; ++x)
            if (gfx_test_pixel(x, i) != (u8)((x * 13 + i * 29 + (x ^ i) * 7) & 255))
                exit(1);
    frame.phase_words[2] = NULL;
    gfx_blit_sprite_words(0, 0, &frame);
}

int main(void)
{
    static const int sizes[] = {1, 2, 3, 4, 7, 16, 31, 32, 63, 64, 101, 255, 256};
    static const int clips[][2] = {{0, 160}, {12, 120}, {30, 85}};
    gba_init();
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        int w = sizes[i], h = sizes[(i * 7u + 3u) % (sizeof(sizes) / sizeof(sizes[0]))];
        int positions[][2] = {
            {0, 0}, {1, 2}, {2, 1}, {-1, 3}, {5, -1},
            {239 - w + 1, 7}, {8, 159 - h + 1},
            {-w + 1, 0}, {0, -h + 1}, {239, 159},
            {-w + 1, -h + 1}, {239 - w + 1, -h + 1}
        };
        for (unsigned p = 0; p < sizeof(positions) / sizeof(positions[0]); ++p)
            for (unsigned c = 0; c < sizeof(clips) / sizeof(clips[0]); ++c)
                for (unsigned pattern = 0; pattern < 4; ++pattern)
                    compare_case(w, h, positions[p][0], positions[p][1],
                                 clips[c][0], clips[c][1], pattern);
    }
    for (unsigned i = 0; i < 500; ++i) {
        int w = 1 + (int)(next_random() % 128u);
        int h = 1 + (int)(next_random() % 96u);
        int x = (int)(next_random() % (unsigned)(FB_WIDTH + w + 1)) - w;
        int y = (int)(next_random() % (unsigned)(FB_HEIGHT + h + 1)) - h;
        compare_case(w, h, x, y, clips[i % 3][0], clips[i % 3][1], i % 4);
    }
    invalid_cases();
    puts("gfx_blit_sprite_words exact-pixel differential tests passed");
    return 0;
}
