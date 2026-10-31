/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SIGHT_AXIS_H
#define SC64_SIGHT_AXIS_H

/* Ordered unsigned subtraction gives the exact nonnegative distance even
 * for INT_MIN/INT_MAX endpoints. Do not subtract signed positions or take
 * abs() before rejecting a malformed or distant source. */
static inline int sc64_sight_axis_contains(int position, int center,
                                         unsigned radius)
{
    unsigned distance = position < center ?
        (unsigned)center - (unsigned)position :
        (unsigned)position - (unsigned)center;
    return distance <= radius;
}

#endif
