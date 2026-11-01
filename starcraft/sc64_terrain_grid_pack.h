/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_TERRAIN_GRID_PACK_H
#define SC64_TERRAIN_GRID_PACK_H
/* Pack the low-bit blocked property into the existing walkable bit grid.
 * Bits outside the active cell count retain their previous values. */
static void sc64_terrain_grid_pack(u8 *dst,const u8 *blocked,unsigned count)
{
    unsigned i=0;
    for (; i+8u<=count; i+=8u) {
        unsigned v=0;
        for (unsigned j=0;j<8;j++) v|=((blocked[i+j]&1u)^1u)<<j;
        dst[i>>3]=(u8)v;
    }
    if (i<count) {
        unsigned v=0,n=count-i,mask=(1u<<n)-1u;
        for (unsigned j=0;j<n;j++) v|=((blocked[i+j]&1u)^1u)<<j;
        dst[i>>3]=(u8)((dst[i>>3]&~mask)|v);
    }
}
#endif
