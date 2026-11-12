#ifndef SC64_DEPOT_GOAL_H
#define SC64_DEPOT_GOAL_H

#include <limits.h>

/* Closest integer pixel to the depot center inside this 16x16 cell. A cell
 * is a valid deposit goal exactly when that pixel is inside the closed ring. */
static int
sc64_depot_cell_point(int cx, int cy, int col, int row, int *px, int *py)
{
    /* Callers pass cells and centers of the map; there the 32-bit arithmetic
     * below is exact and the range tests of the general path cannot fail. */
    if ((unsigned)cx < 0x4000u && (unsigned)cy < 0x4000u &&
        (unsigned)col < 0x400u && (unsigned)row < 0x400u) {
        int left = col * 16, top = row * 16, x = cx, y = cy, dx, dy;
        if (x < left) x = left;
        else if (x > left + 15) x = left + 15;
        if (y < top) y = top;
        else if (y > top + 15) y = top + 15;
        dx = x - cx; dy = y - cy;
        if (dx < -32 || dx > 32 || dy < -32 || dy > 32 ||
            dx * dx + dy * dy > 32 * 32) return 0;
        *px = x; *py = y;
        return 1;
    }
    long long left = (long long)col * 16, top = (long long)row * 16;
    long long x = cx, y = cy, dx, dy;
    if (x < left) x = left;
    else if (x > left + 15) x = left + 15;
    if (y < top) y = top;
    else if (y > top + 15) y = top + 15;
    dx = x - cx; dy = y - cy;
    if (dx < -32 || dx > 32 || dy < -32 || dy > 32 ||
        dx * dx + dy * dy > 32 * 32 || x < INT_MIN || x > INT_MAX ||
        y < INT_MIN || y > INT_MAX) return 0;
    *px = (int)x; *py = (int)y;
    return 1;
}

#endif
