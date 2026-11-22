/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_FACING_DIRECTION_H
#define SC64_FACING_DIRECTION_H

/* The products below fit int for map-coordinate deltas through +/-65535. */
static int
sc64_facing_direction(int dx, int dy)
{
    int first, along, across;
    if (!dx && !dy) return 0;
    if (dy < 0) {
        if (dx < 0) {
            first = 24;
            along = -dx;
            across = -dy;
        } else {
            first = 0;
            along = -dy;
            across = dx;
        }
    } else {
        if (dx < 0) {
            first = 16;
            along = dy;
            across = -dx;
        } else {
            first = 8;
            along = dx;
            across = dy;
        }
    }
    /* Adjacent dot products cross at increasing across/along ratios.
     * Search the exact differences between the rounded direction vectors. */
    if (across * 39 <= along * 32) {
        if (across * 48 <= along * 14)
            return first + (across * 50 > along * 5);
        return first + 2 + (across * 44 > along * 24);
    }
    if (across * 24 <= along * 44)
        return first + 4 + (across * 32 > along * 39);
    if (across * 14 <= along * 48) return first + 6;
    /* At the wrapping boundary, direction zero wins an equal dot product. */
    if (first == 24) return across * 5 >= along * 50 ? 0 : 31;
    return first + 7 + (across * 5 > along * 50);
}

#endif
