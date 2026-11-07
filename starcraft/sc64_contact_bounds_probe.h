/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CONTACT_BOUNDS_PROBE_H
#define SC64_CONTACT_BOUNDS_PROBE_H
/* Direction order is E, SE, S, SW, W, NW, N, NE. The scalar fallback
 * handles malformed positions and unusual or negative speeds unchanged. */
static inline int sc64_contact_bounds_fast(int x,int y,int speed,int width,int height,
                                            unsigned *mask)
{
    if (!mask || width<=0 || height<=0 || width>8192 || height>8192 ||
        (unsigned)x>=(unsigned)width ||
        (unsigned)y>=(unsigned)height || (unsigned)speed>16u) return 0;
    unsigned result=0;
    if (x+speed>=width) result|=0x83u;
    if (x-speed<0) result|=0x38u;
    if (y+speed>=height) result|=0x0eu;
    if (y-speed<0) result|=0xe0u;
    *mask=result;
    return 1;
}
#endif
