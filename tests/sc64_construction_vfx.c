#define GBA_HOST 1
#include <assert.h>
#include <string.h>
#include "../common/gba.h"
#include "../starcraft/sc64_construction_vfx.h"

enum { FB_SIZE = 240 * 160 };
static u8 pixels[FB_SIZE];

static void
capture(u8 *out)
{
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            out[y * 240 + x] = gfx_test_pixel(x, y);
}

static int
different_pixels(const u8 *a, const u8 *b)
{
    int n = 0;
    for (int i = 0; i < FB_SIZE; ++i) n += a[i] != b[i];
    return n;
}

static int
nonblack_pixels(const u8 *a)
{
    int n = 0;
    for (int i = 0; i < FB_SIZE; ++i) n += a[i] != C_BLACK;
    return n;
}

static void
render(int race, int x, int y, int width, int height,
       int progress, int duration, u32 clock, u8 *out)
{
    gfx_begin(C_BLACK);
    assert(sc64_construction_vfx_draw(race, x, y, width, height,
                                      progress, duration, clock));
    capture(out);
}

static void
test_races_have_distinct_visible_art(void)
{
    u8 terran[FB_SIZE], zerg[FB_SIZE], protoss[FB_SIZE];
    render(SC64_CONSTRUCTION_RACE_TERRAN, 100, 75, 40, 32, 1, 12, 8, terran);
    render(SC64_CONSTRUCTION_RACE_ZERG, 100, 75, 40, 32, 1, 12, 8, zerg);
    render(SC64_CONSTRUCTION_RACE_PROTOSS, 100, 75, 40, 32, 1, 12, 8, protoss);
    assert(different_pixels(terran, zerg) > 20);
    assert(different_pixels(terran, protoss) > 20);
    assert(different_pixels(zerg, protoss) > 20);
}

static void
test_progress_and_clock_animate_active_effect(void)
{
    u8 first[FB_SIZE], next_clock[FB_SIZE], next_progress[FB_SIZE];
    render(SC64_CONSTRUCTION_RACE_PROTOSS, 100, 75, 36, 28, 1, 12, 0, first);
    render(SC64_CONSTRUCTION_RACE_PROTOSS, 100, 75, 36, 28, 1, 12, 4, next_clock);
    render(SC64_CONSTRUCTION_RACE_PROTOSS, 100, 75, 36, 28, 9, 12, 0, next_progress);
    assert(different_pixels(first, next_clock) > 0);
    assert(different_pixels(first, next_progress) > 0);
}

static void
test_waiting_marker_is_static_and_distinct(void)
{
    u8 waiting_a[FB_SIZE], waiting_b[FB_SIZE], active[FB_SIZE];
    render(SC64_CONSTRUCTION_RACE_TERRAN, 100, 75, 40, 32, -1, 12, 0, waiting_a);
    render(SC64_CONSTRUCTION_RACE_TERRAN, 100, 75, 40, 32, -1, 12, 1000, waiting_b);
    render(SC64_CONSTRUCTION_RACE_TERRAN, 100, 75, 40, 32, 0, 12, 0, active);
    assert(different_pixels(waiting_a, waiting_b) == 0);
    assert(different_pixels(waiting_a, active) > 0);
}

static void
test_invalid_inputs_do_not_draw(void)
{
    gfx_begin(C_BLACK);
    capture(pixels);
    assert(!sc64_construction_vfx_draw(0, 100, 75, 32, 32, 0, 10, 0));
    assert(!sc64_construction_vfx_draw(99, 100, 75, 32, 32, 0, 10, 0));
    assert(!sc64_construction_vfx_draw(SC64_CONSTRUCTION_RACE_ZERG,
                                      100, 75, 32, 32, 0, 0, 0));
    u8 after[FB_SIZE];
    capture(after);
    assert(different_pixels(pixels, after) == 0);
    assert(sc64_construction_vfx_phase(-1, 10, 100) == -1);
    assert(sc64_construction_vfx_phase(5, 0, 100) == -1);
}

static void
test_dimensions_are_clamped_and_screen_clips(void)
{
    u8 tiny[FB_SIZE], huge[FB_SIZE];
    render(SC64_CONSTRUCTION_RACE_ZERG, 0, 0, -100, -100, 2, 10, 4, tiny);
    render(SC64_CONSTRUCTION_RACE_ZERG, 239, 159, 10000, 10000,
           2, 10, 4, huge);
    assert(nonblack_pixels(tiny) > 0);
    assert(nonblack_pixels(huge) > 0);
}

int
main(void)
{
    test_races_have_distinct_visible_art();
    test_progress_and_clock_animate_active_effect();
    test_waiting_marker_is_static_and_distinct();
    test_invalid_inputs_do_not_draw();
    test_dimensions_are_clamped_and_screen_clips();
    return 0;
}
