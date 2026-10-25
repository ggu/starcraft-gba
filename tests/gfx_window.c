/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exact host framebuffer oracle for the public opaque window blitter. */
#include "../common/gba.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

enum { FB_WIDTH = 240, FB_HEIGHT = 160, FB_PIXELS = FB_WIDTH * FB_HEIGHT };

static u8 expected[FB_PIXELS];

static u8 destination_pattern(int x, int y)
{
    return (u8)((x * 31 + y * 17 + (x ^ (y * 3)) * 5) & 0xff);
}

static u8 source_pattern(int x, int y)
{
    switch (((unsigned)x + (unsigned)y * 3u) & 7u) {
    case 0: return 0;
    case 1: return 0x80;
    case 2: return 0xff;
    case 3: return 0x01;
    case 4: return 0;
    case 5: return 0xa5;
    case 6: return 0x7f;
    default: return (u8)(x * 13 + y * 47);
    }
}

static void begin_patterned_frame(void)
{
    gfx_begin(0x53);
    for (int y = 0; y < FB_HEIGHT; ++y) {
        for (int x = 0; x < FB_WIDTH; ++x) {
            u8 pixel = destination_pattern(x, y);
            expected[y * FB_WIDTH + x] = pixel;
            gfx_pixel(x, y, pixel);
        }
    }
}

static void apply_oracle(int y, int height, const u8 *source, int stride,
                         int rows, int left, int top)
{
    /* Avoid overflowing top+dy for extreme offsets whose source rows cannot
       intersect the requested window. */
    if (top >= rows || top <= -height) return;
    for (int dy = 0; dy < height; ++dy) {
        int sy = top + dy;
        if (sy < 0 || sy >= rows) continue;
        for (int dx = 0; dx < FB_WIDTH; ++dx) {
            int sx = left + dx;
            if (sx >= 0 && sx < stride)
                expected[(y + dy) * FB_WIDTH + dx] = source[sy * stride + sx];
        }
    }
}

static int expected_valid(int y, int height, const u8 *source,
                          int stride, int rows)
{
    return source && (((uintptr_t)source & 3u) == 0) && stride > 0 &&
           (stride & 3) == 0 && rows > 0 && y >= 0 && height > 0 &&
           height <= FB_HEIGHT && y <= FB_HEIGHT - height;
}

static void verify_frame(const char *label, int y, int height, int stride,
                         int rows, int left, int top, int expected_result,
                         int actual_result)
{
    if (actual_result != expected_result) {
        fprintf(stderr,
                "%s: return mismatch y=%d height=%d stride=%d rows=%d "
                "left=%d top=%d expected=%d actual=%d\n",
                label, y, height, stride, rows, left, top,
                expected_result, actual_result);
        exit(1);
    }
    for (int py = 0; py < FB_HEIGHT; ++py) {
        for (int px = 0; px < FB_WIDTH; ++px) {
            u8 actual = gfx_test_pixel(px, py);
            u8 wanted = expected[py * FB_WIDTH + px];
            if (actual != wanted) {
                fprintf(stderr,
                        "%s: mismatch at (%d,%d) y=%d height=%d "
                        "stride=%d rows=%d left=%d top=%d "
                        "expected=%u actual=%u\n",
                        label, px, py, y, height, stride, rows, left, top,
                        wanted, actual);
                exit(1);
            }
        }
    }
}

static void run_case(const char *label, int y, int height, int stride,
                     int rows, int left, int top, unsigned source_offset,
                     int provide_source)
{
    size_t length = stride > 0 && rows > 0 ? (size_t)stride * (size_t)rows : 0;
    size_t offset = source_offset;
    u8 *allocation = NULL, *source = NULL;
    if (provide_source) {
        size_t allocation_size = length + offset;
        if (allocation_size == 0) allocation_size = 1;
        allocation = (u8 *)malloc(allocation_size);
        if (!allocation) {
            fprintf(stderr, "allocation failed for %s\n", label);
            exit(1);
        }
        source = allocation + offset;
        for (int sy = 0; sy < rows && stride > 0; ++sy)
            for (int sx = 0; sx < stride; ++sx)
                source[(size_t)sy * (size_t)stride + (size_t)sx] =
                    source_pattern(sx, sy);
    }

    begin_patterned_frame();
    int valid = expected_valid(y, height, source, stride, rows);
    if (valid) apply_oracle(y, height, source, stride, rows, left, top);
    int result = gfx_blit_window(y, height, source, stride, rows, left, top);
    verify_frame(label, y, height, stride, rows, left, top, valid, result);
    gfx_end();
    free(allocation);
}

static void vertical_band_cases(void)
{
    static const int strides[] = { 240, 244, 248, 256, 272 };
    static const int row_counts[] = { 108, 160 };
    static const int tops108[] = { -108, -107, -4, -1, 0, 1, 4, 107, 108, 109 };
    static const int tops160[] = { -160, -159, -4, -1, 0, 1, 4,
                                    107, 108, 159, 160, 161 };
    for (unsigned si = 0; si < sizeof(strides) / sizeof(strides[0]); ++si) {
        for (unsigned ri = 0; ri < sizeof(row_counts) / sizeof(row_counts[0]); ++ri) {
            int rows = row_counts[ri];
            const int *tops = ri == 0 ? tops108 : tops160;
            size_t top_count = ri == 0 ? sizeof(tops108) / sizeof(tops108[0]) :
                                         sizeof(tops160) / sizeof(tops160[0]);
            for (unsigned ti = 0; ti < top_count; ++ti) {
                run_case("full-width vertical bands", 0, rows == 108 ? 108 : 160,
                         strides[si], rows, 0, tops[ti], 0, 1);
                if (rows == 108) {
                    run_case("visible vertical window at y12", 12, 108,
                             strides[si], rows, 0, tops[ti], 0, 1);
                    run_case("visible vertical window at y52", 52, 108,
                             strides[si], rows, 0, tops[ti], 0, 1);
                }
            }
        }
    }

    for (unsigned ri = 0; ri < sizeof(row_counts) / sizeof(row_counts[0]); ++ri) {
        int rows = row_counts[ri];
        int height = rows == 108 ? 108 : 160;
        run_case("top INT_MIN", 0, height, 240, rows, 0, INT_MIN, 0, 1);
        /* Keep the generic path's old top+row arithmetic in range too. */
        run_case("top INT_MAX", 0, 1, 240, rows, 0, INT_MAX, 0, 1);
    }
}

static void horizontal_clip_cases(void)
{
    static const int strides[] = { 240, 244, 248, 256, 272 };
    static const int lefts[] = { -241, -240, -239, -4, -1, 0,
                                  1, 4, 239, 240, 241, 479, 480 };
    static const int tops[] = { -4, -1, 0, 1, 4, 107, 108 };
    for (unsigned si = 0; si < sizeof(strides) / sizeof(strides[0]); ++si)
        for (unsigned xi = 0; xi < sizeof(lefts) / sizeof(lefts[0]); ++xi)
            for (unsigned ti = 0; ti < sizeof(tops) / sizeof(tops[0]); ++ti)
                run_case("horizontal clipping", 12, 108, strides[si], 108,
                         lefts[xi], tops[ti], 0, 1);
}

static void invalid_argument_cases(void)
{
    static u8 unaligned_storage[8] __attribute__((aligned(4)));
    const u8 *unaligned = unaligned_storage + 1;
    run_case("null source", 0, 1, 240, 1, 0, 0, 0, 0);
    for (unsigned offset = 1; offset <= 3; ++offset)
        run_case("unaligned source", 0, 1, 240, 1, 0, 0, offset, 1);
    begin_patterned_frame();
    int result = gfx_blit_window(0, 1, unaligned, 240, 1, 0, 0);
    verify_frame("fixed unaligned source", 0, 1, 240, 1, 0, 0, 0, result);
    gfx_end();

    run_case("zero stride", 0, 1, 0, 1, 0, 0, 0, 1);
    run_case("negative stride", 0, 1, -4, 1, 0, 0, 0, 1);
    run_case("unaligned stride", 0, 1, 242, 1, 0, 0, 0, 1);
    run_case("zero rows", 0, 1, 240, 0, 0, 0, 0, 1);
    run_case("negative rows", 0, 1, 240, -1, 0, 0, 0, 1);
    run_case("negative destination y", -1, 1, 240, 1, 0, 0, 0, 1);
    run_case("zero height", 0, 0, 240, 1, 0, 0, 0, 1);
    run_case("negative height", 0, -1, 240, 1, 0, 0, 0, 1);
    run_case("height past screen", 0, 161, 240, 161, 0, 0, 0, 1);
    run_case("destination overrun", 159, 2, 240, 2, 0, 0, 0, 1);
    run_case("destination at bottom", 160, 1, 240, 1, 0, 0, 0, 1);

    /* Valid arguments with empty source intersection still return success and
       leave the destination pattern untouched. */
    run_case("empty far-left view", 0, 1, 240, 1, 480, 0, 0, 1);
    run_case("empty far-right view", 0, 1, 240, 1, -240, 0, 0, 1);
    run_case("empty above view", 0, 1, 240, 1, 0, -1, 0, 1);
    run_case("empty below view", 0, 1, 240, 1, 0, 1, 0, 1);
}

int main(void)
{
    gba_init();
    vertical_band_cases();
    horizontal_clip_cases();
    invalid_argument_cases();
    puts("gfx_blit_window exact-pixel differential tests passed");
    return 0;
}
