#ifndef SC64_P7_POWER_GEOMETRY_H
#define SC64_P7_POWER_GEOMETRY_H

/* OpenBW's psi_field_mask and is_in_psionic_matrix_range, pinned tree
 * 4b046d5f65302b10cb0a745f0fecd37ec85b20a8. Actor deltas use half-resolution
 * runtime pixels. Preserve the source's outer rejection before its negative
 * edge adjustment; the field is asymmetric at the interior cell boundaries. */
static const unsigned char sc64_p7_psi_field_mask[5][8] = {
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,0},
    {1,1,1,1,1,1,0,0},
    {1,1,1,0,0,0,0,0}
};

static int sc64_p7_in_psi_range(int dx, int dy)
{
    /* Reject in runtime coordinates first, also bounding the absolute value
     * conversion for every int input. The source rejects |x|>=256, |y|>=160. */
    if (dx<=-128 || dx>=128 || dy<=-80 || dy>=80) return 0;
    unsigned x=(unsigned)(dx<0?-dx:dx)*2u;
    unsigned y=(unsigned)(dy<0?-dy:dy)*2u;
    if (dx<0) --x;
    if (dy<0) --y;
    return sc64_p7_psi_field_mask[y/32u][x/32u]!=0;
}

#endif
