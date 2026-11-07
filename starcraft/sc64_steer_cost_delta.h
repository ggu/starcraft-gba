/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STEER_COST_DELTA_H
#define SC64_STEER_COST_DELTA_H
/* The common squared-distance term cancels when comparing eight candidates.
 * Restrict the fast arithmetic to a range in which both forms fit int;
 * callers retain the scalar calculation for every other state. */
typedef struct { int x, y, square; } Sc64SteerCostDelta;
static inline int sc64_steer_cost_delta_prepare(Sc64SteerCostDelta *out,
                                                int x,int y,int tx,int ty,int speed)
{
    if (!out || (unsigned)x>8191u || (unsigned)y>8191u ||
        (unsigned)tx>8191u || (unsigned)ty>8191u || (unsigned)speed>16u) return 0;
    out->x=2*speed*(x-tx); out->y=2*speed*(y-ty); out->square=speed*speed;
    return 1;
}
static inline int sc64_steer_cost_delta(const Sc64SteerCostDelta *d,int dx,int dy)
{
    int x=dx>0?d->x:dx<0?-d->x:0;
    int y=dy>0?d->y:dy<0?-d->y:0;
    return x+y+d->square*(1+(dx!=0&&dy!=0));
}
#endif
