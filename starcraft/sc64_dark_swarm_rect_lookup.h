/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_DARK_SWARM_RECT_LOOKUP_H
#define SC64_DARK_SWARM_RECT_LOOKUP_H
#include <limits.h>
#include <stdint.h>
#include "dark_swarm_rects_generated.h"
#if SC64_DARK_SWARM_RECTS_FORMAT != 1 || SC64_DARK_SWARM_RECTS_COUNT != 228
#error "Unsupported Dark Swarm source rectangle table"
#endif
_Static_assert(sizeof(sc64_dark_swarm_rects)==228u*4u,
               "Dark Swarm source rectangle table has the wrong size");
/* Input coordinates are GBA world pixels; output is a half-open source box.
 * Validate both the actual world and arithmetic before doubling noncompact
 * int coordinates. Failure means unknown geometry, never no field. */
static inline int sc64_dark_swarm_source_rect(int raw,int x,int y,int width,int height,
                                               int *left,int *top,int *right,int *bottom)
{
    if(raw<0 || raw>=SC64_DARK_SWARM_RECTS_COUNT || x<0 || y<0 ||
       width<=0 || height<=0 || x>=width || y>=height ||
       x>(INT_MAX-256)/2 || y>(INT_MAX-256)/2 || !left || !top || !right || !bottom)return 0;
    const uint8_t *r=sc64_dark_swarm_rects[raw];
    *left=x*2-r[0];*top=y*2-r[1];
    *right=x*2+r[2]+1;*bottom=y*2+r[3]+1;
    return 1;
}
#endif
