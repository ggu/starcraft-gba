/* Asset-free framebuffer oracle for terrain-only overdraw and clip fallback. */
#include "common/gba.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static u8 source[108 * 240] __attribute__((aligned(4)));
static u8 reference[160 * 240], candidate[160 * 240];

static void capture(u8 *out)
{
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            out[y * 240 + x] = gfx_test_pixel(x, y);
}

static void redraw_exposed_edge(int rows, int top, int dx)
{
    for (int y = 0; y < 108; ++y) {
        if (top + y < 0 || top + y >= rows) continue;
        int x = dx > 0 ? 240 - dx : 0;
        int width = dx > 0 ? dx : -dx;
        gfx_rect(x, y + 12, width, 1, 99);
    }
}

int main(void)
{
    static const int clips[][2] = {
        {0, 0}, {0, 12}, {12, 13}, {12, 119}, {13, 120},
        {119, 120}, {120, 160}, {12, 120}, {0, 160}
    };
    static const int tops[] = {
        -107, -68, -20, -1, 0, 1, 20, 47, 48, 67, 68, 106, 107, 108
    };
    static const int lengths[] = {1, 2, 3, 40, 48, 68, 108};
    unsigned cases = 0;
    for (unsigned i = 0; i < sizeof(source); ++i)
        source[i] = (u8)(i * 37u + (i >> 8));
    for (unsigned c = 0; c < sizeof(clips) / sizeof(*clips); ++c)
        for (unsigned n = 0; n < sizeof(lengths) / sizeof(*lengths); ++n)
            for (unsigned t = 0; t < sizeof(tops) / sizeof(*tops); ++t)
                for (int dx = -12; dx <= 12; dx += 4) {
                    if (!dx) continue;
                    int rows = lengths[n], top = tops[t];
                    gfx_begin(37);
                    gfx_set_clip_y(clips[c][0], clips[c][1]);
                    assert(gfx_blit_window(12, 108, source, 240, rows, dx, top));
                    redraw_exposed_edge(rows, top, dx);
                    capture(reference);
                    gfx_begin(37);
                    gfx_set_clip_y(clips[c][0], clips[c][1]);
                    assert(gfx_blit_terrain_window(12, 108, source, 240, rows, dx, top));
                    redraw_exposed_edge(rows, top, dx);
                    capture(candidate);
                    if (memcmp(reference, candidate, sizeof(reference))) {
                        fprintf(stderr, "clip %d,%d rows %d top %d dx %d\n",
                                clips[c][0], clips[c][1], rows, top, dx);
                        assert(0);
                    }
                    ++cases;
                }
    printf("terrain window and clipped redraw: %u cases passed\n", cases);
    return 0;
}
