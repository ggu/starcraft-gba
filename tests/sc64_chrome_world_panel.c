/* Asset-free scalar framebuffer oracle for the shared world panel.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
enum { C_GOLD, C_SILVER, C_DIM };
static u8 actual[240 * 160], expected[240 * 160];
static void gfx_pixel(int x, int y, u8 color) { assert((unsigned)x < 240 && (unsigned)y < 160); actual[y * 240 + x] = color; }
static void gfx_blit_sprite(int x, int y, int w, int h, const u8 *p) { (void)x; (void)y; (void)w; (void)h; (void)p; }
static int sc64_font_width(const char *s) { (void)s; return 0; }
static void sc64_font_text(int x, int y, int w, const char *s, int c) { (void)x; (void)y; (void)w; (void)s; (void)c; }
static void sc64_font_ellipsis(const char *s, int w, char *out, unsigned n) { (void)s; (void)w; if (n) *out = 0; }
#include "../starcraft/sc64_chrome.h"

/* Original scalar formula, independent of the optimized implementation. */
static void reference(int x, int y, int width, int height)
{
    enum { BX = 6, BY = 5, SW = 208, SH = 58 };
    if (width <= 2 * BX + 1 || height <= 2 * BY + 1 || x >= 240 || y >= 160 ||
        x + width <= 0 || y + height <= 0) return;
    for (int dy = 0; dy < height; ++dy) {
        int sy = dy < BY ? dy : dy >= height - BY ? SH - (height - dy) :
            BY + (dy - BY) * (SH - 2 * BY - 1) / (height - 2 * BY - 1);
        int py = y + dy;
        if (py < 0 || py >= 160) continue;
        for (int dx = 0; dx < width; ++dx) {
            if (dx >= BX && dx < width - BX && dy >= BY && dy < height - BY) continue;
            int sx = dx < BX ? dx : dx >= width - BX ? SW - (width - dx) :
                BX + (dx - BX) * (SW - 2 * BX - 1) / (width - 2 * BX - 1);
            int px = x + dx;
            u8 strength = sc64_chrome_world_panel_pixels[sy * SW + sx];
            if (strength && px >= 0 && px < 240)
                expected[py * 240 + px] = sc64_chrome_world_panel_colors[strength - 1];
        }
    }
}
int main(void)
{
    const int widths[] = {-1, 0, 1, 12, 13, 14, 138, 184, 192};
    const int heights[] = {-1, 0, 1, 10, 11, 12, 26, 48, 70, 86, 92, 132};
    const int xs[] = {-240, -192, -137, -7, -1, 0, 24, 100, 239, 240};
    const int ys[] = {-160, -132, -91, -6, -1, 0, 14, 30, 159, 160};
    unsigned cases = 0;
    for (int y = 0; y < 58; ++y) for (int x = 0; x < 208; ++x)
        sc64_chrome_world_panel_pixels[y * 208 + x] = (u8)((x * 13 + y * 7 + x * y) % 5);
    for (unsigned a = 0; a < sizeof widths / sizeof *widths; ++a)
    for (unsigned b = 0; b < sizeof heights / sizeof *heights; ++b)
    for (unsigned c = 0; c < sizeof xs / sizeof *xs; ++c)
    for (unsigned d = 0; d < sizeof ys / sizeof *ys; ++d) {
        for (unsigned i = 0; i < sizeof actual; ++i) actual[i] = expected[i] = (u8)(i * 37u + 11u);
        reference(xs[c], ys[d], widths[a], heights[b]);
        sc64_chrome_panel_world(xs[c], ys[d], widths[a], heights[b]);
        if (memcmp(actual, expected, sizeof actual)) {
            fprintf(stderr, "mismatch x=%d y=%d w=%d h=%d\n", xs[c], ys[d], widths[a], heights[b]); return 1;
        }
        ++cases;
    }
    printf("world panel: %u scalar comparisons passed\n", cases);
    return 0;
}
