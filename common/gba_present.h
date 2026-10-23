#ifndef GBA_PRESENT_H
#define GBA_PRESENT_H

#include "gba.h"

enum {
    GBA_PRESENT_CYCLES_PER_FRAME = 280896u,
    GBA_PRESENT_MIN_FRAME_CYCLES = 2u * GBA_PRESENT_CYCLES_PER_FRAME,
    GBA_PRESENT_FIRST_VBLANK_LINE = 160,
    GBA_PRESENT_LAST_SAFE_VBLANK_LINE = 226
};

/* Gate page flips to VBlank and at least two hardware frames after the last
 * flip. The final VBlank line is excluded to retain one scanline of margin. */
static inline int
gba_present_ready(u32 elapsed_cycles, u16 vcount)
{
    return elapsed_cycles >= GBA_PRESENT_MIN_FRAME_CYCLES &&
           vcount >= GBA_PRESENT_FIRST_VBLANK_LINE &&
           vcount <= GBA_PRESENT_LAST_SAFE_VBLANK_LINE;
}

#endif
