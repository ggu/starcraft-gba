/* Asset-free pixel and fallback oracle for indexed command icons.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "../common/gba.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { unsigned int offset; unsigned char width, height; } Sc64IconFrame;
enum { SC64_ICON_COUNT = 2, SOURCE_MAX = 255 };
static Sc64IconFrame sc64_icons[SC64_ICON_COUNT];
static u8 sc64_icon_pixels[SOURCE_MAX * SOURCE_MAX + 8];
#define SC64_ICON_TABLE_PRESENT 1
#include "../starcraft/sc64_icons.h"

static struct { int x, y; u8 color; } events[SOURCE_MAX * SOURCE_MAX];
static u8 reference_page[240 * 160], actual_page[240 * 160];
static unsigned event_count, observed_count;
static int clip_top, clip_bottom;

static int visible(int x, int y)
{
    return (unsigned)x < 240u && y >= clip_top && y < clip_bottom;
}

void gfx_pixel(int x, int y, u8 color)
{
    assert(observed_count < event_count);
    assert(events[observed_count].x == x && events[observed_count].y == y &&
           events[observed_count].color == color);
    ++observed_count;
    if (visible(x, y)) actual_page[y * 240 + x] = color;
}

/* Keep the scalar definition of each sample independent of the stepper. */
static int reference(unsigned id, int cx, int cy, int size)
{
    const Sc64IconFrame *icon;
    unsigned w, h;
    if (id >= SC64_ICON_COUNT || size <= 0) return 0;
    icon = &sc64_icons[id];
    w = icon->width;
    h = icon->height;
    if (!w || !h || icon->offset > sizeof(sc64_icon_pixels) ||
        w * h > sizeof(sc64_icon_pixels) - icon->offset) return 0;
    if (w > (unsigned)size || h > (unsigned)size) {
        unsigned large = w > h ? w : h;
        w = (w * (unsigned)size + large / 2) / large;
        h = (h * (unsigned)size + large / 2) / large;
        if (!w) w = 1;
        if (!h) h = 1;
    }
    for (unsigned y = 0; y < h; ++y) {
        for (unsigned x = 0; x < w; ++x) {
            const unsigned at = icon->offset + y * icon->height / h * icon->width +
                x * icon->width / w;
            const u8 color = sc64_icon_pixels[at];
            if (color) {
                const int dx = cx - (int)w / 2 + (int)x;
                const int dy = cy - (int)h / 2 + (int)y;
                assert(event_count < SOURCE_MAX * SOURCE_MAX);
                events[event_count].x = dx;
                events[event_count].y = dy;
                events[event_count++].color = color;
            }
        }
    }
    if (event_count < 4) {
        event_count = 0;
        return 0;
    }
    for (unsigned i = 0; i < event_count; ++i)
        if (visible(events[i].x, events[i].y))
            reference_page[events[i].y * 240 + events[i].x] = events[i].color;
    return 1;
}

static unsigned cases, draws, fallbacks;
static void check(unsigned id, int x, int y, int size)
{
    int expected;
    memset(reference_page, 71, sizeof(reference_page));
    memset(actual_page, 71, sizeof(actual_page));
    event_count = observed_count = 0;
    expected = reference(id, x, y, size);
    assert(sc64_icon_draw_centered(id, x, y, size) == expected);
    assert(observed_count == event_count);
    assert(memcmp(actual_page, reference_page, sizeof(actual_page)) == 0);
    ++cases;
    if (expected) ++draws;
    else ++fallbacks;
}

int main(void)
{
    static const int centers[][2] = {{120,80},{-2,12},{241,157},{2,-3},{239,161}};
    static const unsigned large_shapes[][2] = {{21,37},{254,255},{255,1},{1,255}};
    sc64_icons[0].offset = 3; /* unaligned atlas offset */
    for (unsigned w = 1; w <= 20; ++w) for (unsigned h = 1; h <= 20; ++h) {
        sc64_icons[0].width = (u8)w;
        sc64_icons[0].height = (u8)h;
        for (unsigned i = 0; i < w * h; ++i) {
            unsigned p = (i * 73 + w * 23 + h * 11) % 256;
            sc64_icon_pixels[i + 3] = p % 7 ? (u8)p : 0;
        }
        for (int size = 1; size <= 23; ++size) {
            unsigned c = (unsigned)(size + w + h) % (sizeof(centers) / sizeof(centers[0]));
            clip_top = c & 1u ? 14 : 0;
            clip_bottom = c & 1u ? 110 : 160;
            check(0, centers[c][0], centers[c][1], size);
        }
    }

    clip_top = 14;
    clip_bottom = 110;
    memset(sc64_icon_pixels, 0, sizeof(sc64_icon_pixels));
    sc64_icons[0] = (Sc64IconFrame){0,20,20};
    for (unsigned i = 0; i < 3; ++i) sc64_icon_pixels[399 - i] = (u8)(42 + i);
    check(0, -20, 130, 20); /* three opaque pixels request the caller's fallback */
    sc64_icon_pixels[396] = 45;
    check(0, -20, 130, 20); /* four opaque pixels count even when all are clipped */
    check(0, 7, 15, 18);    /* scaling can drop otherwise opaque pixels */
    memset(sc64_icon_pixels, 1, sizeof(sc64_icon_pixels));
    for (unsigned i = 0; i < sizeof(large_shapes) / sizeof(large_shapes[0]); ++i) {
        sc64_icons[0] = (Sc64IconFrame){3,(u8)large_shapes[i][0],(u8)large_shapes[i][1]};
        check(0, -2, 12, 18);  /* small output with a large source step */
        check(0, 242, 112, 255); /* valid oversized scalar fallback */
    }
    check(SC64_ICON_COUNT, 7, 15, 20);
    check(0, 7, 15, 0);
    check(1, 7, 15, 20); /* empty source frame */
    sc64_icons[0] = (Sc64IconFrame){sizeof(sc64_icon_pixels) + 1,1,1};
    check(0, 7, 15, 20);
    sc64_icons[0].offset = sizeof(sc64_icon_pixels);
    check(0, 7, 15, 20);
    assert(draws > 8000 && fallbacks > 500);
    printf("command icon scaling: %u pixel-exact cases, %u draws and %u fallbacks\n",
           cases, draws, fallbacks);
    return 0;
}
