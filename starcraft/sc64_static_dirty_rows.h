#ifndef SC64_STATIC_DIRTY_ROWS_H
#define SC64_STATIC_DIRTY_ROWS_H

#include <stdint.h>

enum {
    SC64_STATIC_DIRTY_ROWS_MAX_WIDTH = 17,
    SC64_STATIC_DIRTY_ROWS_MAX_ROWS = 9,
    SC64_STATIC_DIRTY_ROWS_MAX_BANDS = 5,
    SC64_STATIC_DIRTY_ROWS_CACHE_TOP = 12,
    SC64_STATIC_DIRTY_ROWS_CACHE_BOTTOM = 120
};

typedef struct {
    int16_t top, bottom;
} Sc64StaticDirtyRowsBand;

typedef struct {
    uint8_t count;
    Sc64StaticDirtyRowsBand bands[SC64_STATIC_DIRTY_ROWS_MAX_BANDS];
} Sc64StaticDirtyRowsPlan;

/* Convert the current-view row-major dirty tile mask into disjoint screen-y
 * bands. The caller owns cache validity/view checks; this helper is pure. */
static inline int
sc64_static_dirty_rows_plan(const uint8_t *dirty, unsigned width,
                            unsigned rows, int first_world_row, int camy,
                            int clip_top, int clip_bottom,
                            Sc64StaticDirtyRowsPlan *plan)
{
    unsigned row = 0;
    int top_limit, bottom_limit, screen_row_base;

    if (!plan) return 0;
    plan->count = 0;
    if (!dirty || width == 0 || width > SC64_STATIC_DIRTY_ROWS_MAX_WIDTH ||
        rows == 0 || rows > SC64_STATIC_DIRTY_ROWS_MAX_ROWS ||
        width * rows > SC64_STATIC_DIRTY_ROWS_MAX_WIDTH *
                       SC64_STATIC_DIRTY_ROWS_MAX_ROWS ||
        first_world_row < 0 || camy < 0 ||
        first_world_row != camy / 16 || clip_top < 0 ||
        clip_top > clip_bottom || clip_bottom > 160)
        return 0;

    top_limit = clip_top > SC64_STATIC_DIRTY_ROWS_CACHE_TOP ?
                clip_top : SC64_STATIC_DIRTY_ROWS_CACHE_TOP;
    bottom_limit = clip_bottom < SC64_STATIC_DIRTY_ROWS_CACHE_BOTTOM ?
                   clip_bottom : SC64_STATIC_DIRTY_ROWS_CACHE_BOTTOM;
    if (top_limit >= bottom_limit) return 0;
    screen_row_base = SC64_STATIC_DIRTY_ROWS_CACHE_TOP - (camy % 16);

    while (row < rows) {
        unsigned first, end, col;
        int top, bottom;

        for (col = 0; col < width && !dirty[row * width + col]; ++col) { }
        if (col == width) {
            ++row;
            continue;
        }

        first = row;
        do {
            ++row;
            if (row == rows) break;
            for (col = 0; col < width && !dirty[row * width + col]; ++col) { }
        } while (col < width);
        end = row;

        top = screen_row_base + (int)first * 16;
        bottom = screen_row_base + (int)end * 16;
        if (top < top_limit) top = top_limit;
        if (bottom > bottom_limit) bottom = bottom_limit;
        if (top >= bottom) continue;
        if (plan->count >= SC64_STATIC_DIRTY_ROWS_MAX_BANDS) {
            plan->count = 0;
            return 0;
        }
        plan->bands[plan->count].top = (int16_t)top;
        plan->bands[plan->count].bottom = (int16_t)bottom;
        ++plan->count;
    }
    return plan->count != 0;
}

#endif
