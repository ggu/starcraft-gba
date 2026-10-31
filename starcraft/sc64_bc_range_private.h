/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BC_RANGE_PRIVATE_H
#define SC64_BC_RANGE_PRIVATE_H
#include "sc64_siege_source_bounds.h"
/* Stationary source-unit rectangle distance from pinned BWAPI UnitType
 * extents and OpenBW's xy_length approximation. Dynamic movement/halt
 * allowances are separate caller inputs and have not yet been bound. */
static int sc64_bc_source_range_distance(unsigned raw_a,int ax,int ay,
                                         unsigned raw_b,int bx,int by)
{
    int al,at,ar,ab,bl,bt,br,bb,dx,dy,hi,lo;
    if (raw_a>=228 || raw_b>=228 || ax<0 || ay<0 || bx<0 || by<0 ||
        ax>=2048 || ay>=2048 || bx>=2048 || by>=2048) return -1;
    const short *a=siege_bounds[raw_a],*b=siege_bounds[raw_b];
    al=ax*2-a[2];at=ay*2-a[3];ar=ax*2+a[4]+1;ab=ay*2+a[5]+1;
    bl=bx*2-b[2];bt=by*2-b[3];br=bx*2+b[4]+1;bb=by*2+b[5]+1;
    dx=al>br?al-br:bl>ar?bl-ar:0; dy=at>bb?at-bb:bt>ab?bt-ab:0;
    hi=dx>dy?dx:dy;lo=dx>dy?dy:dx;
    if (hi/4<lo) hi=hi-hi/16+lo*3/8-hi/64+lo*3/256;
    return hi;
}
#endif
