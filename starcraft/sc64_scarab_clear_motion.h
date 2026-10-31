/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_CLEAR_MOTION_H
#define SC64_SCARAB_CLEAR_MOTION_H
#include <stdint.h>
/* Isolated clear, straight, already-aligned motion slice, in source Q8.
 * `age` is elapsed source ticks after state-0 initialization. This is not
 * pathfinding, turning, near-target arrival or a complete IScript runner. */
static inline int sc64_scarab_clear_motion(int32_t *x,int32_t *y,int32_t tx,int32_t ty,
                                          unsigned heading,unsigned age,int clear)
{
    int32_t dx,dy,nx,ny;
    if (!x || !y || x==y || !clear || *x<0 || *y<0 || tx<0 || ty<0 ||
        *x>0x7fffff || *y>0x7fffff || tx>0x7fffff || ty>0x7fffff || age>89) return 0;
    dx=tx-*x;dy=ty-*y;
    /* Reject turning and the unqualified near-target/clamping zone. */
    if (!((heading==64 && dx>=64*256 && dy==0) ||
          (heading==128 && dy>=64*256 && dx==0))) return 0;
    nx=*x;ny=*y;
    if (age>=3) { if (heading==64) nx+=16*256; else ny+=16*256; }
    *x=nx;*y=ny;return 1;
}
#endif
