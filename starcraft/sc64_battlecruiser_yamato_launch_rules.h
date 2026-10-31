/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BATTLECRUISER_YAMATO_LAUNCH_RULES_H
#define SC64_BATTLECRUISER_YAMATO_LAUNCH_RULES_H
#include <stdint.h>
#include "sc64_interceptor_direction_tables.h"
/* Pure launch policy for standard flying Battlecruisers. `sample` is the
 * low byte of the source RNG draw; the caller owns RNG stream and target
 * visibility/cover/dark-swarm inputs.  The source compares <=, including a
 * sample of zero even when the raw miss threshold is zero. */
typedef struct { int x,y; uint8_t heading,missed,threshold; } Sc64BcLaunchResolution;
static int sc64_bc_launch_floor_q8(int n)
{ return n>=0?n/256:-(((-n)+255)/256); }
static int sc64_bc_yamato_resolve_launch(unsigned heading,int x,int y,
                                          int target_flying,int terrain_cover,
                                          int dark_swarm,unsigned sample,
                                          Sc64BcLaunchResolution *out)
{
    unsigned threshold;
    if (!out || heading>255 || x<0 || x>=4096 || y<0 || y>=4096 ||
        (unsigned)target_flying>1 || (unsigned)terrain_cover>1 ||
        (unsigned)dark_swarm>1 || sample>255) return 0;
    threshold=target_flying?0u:dark_swarm?255u:terrain_cover?119u:0u;
    out->heading=(uint8_t)heading; out->threshold=(uint8_t)threshold;
    out->missed=(uint8_t)(sample<=threshold); out->x=x; out->y=y;
    if (out->missed) {
        Sc64InterceptorDirectionVector v=sc64_interceptor_direction_vector[heading];
        out->x-=sc64_bc_launch_floor_q8((int)v.x*30);
        out->y-=sc64_bc_launch_floor_q8((int)v.y*30);
    }
    return 1;
}
#endif
