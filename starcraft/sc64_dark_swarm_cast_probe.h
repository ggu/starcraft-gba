/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_DARK_SWARM_CAST_PROBE_H
#define SC64_DARK_SWARM_CAST_PROBE_H
#ifndef SC64_DARK_SWARM_CAST_PROBE
#error "Dark Swarm cast stage is not a complete gameplay capability"
#endif
#include "sc64_dark_swarm_state.h"

/* N64 authenticated techdata.dat row 14, unit row 202. Energy is in source
 * 8.8 fixed-point units. These are probe entry points, not gameplay orders:
 * the eventual caller must establish owner, eligibility, visibility, weapon
 * range, facing, cooldown and the source animation transition first. */
#define SC64_DARK_SWARM_ENERGY_Q8 (100u * 256u)
#define SC64_DARK_SWARM_FROM 80
#define SC64_DARK_SWARM_TO_PLUS_ONE 80

static int sc64_dark_swarm_probe_begin(const Unit *caster, int burrowed,
                                       unsigned *energy_q8)
{
    if (!caster || !caster->alive || sc64_race_raw_unit(caster->type)!=46 ||
        burrowed || !energy_q8 || *energy_q8<SC64_DARK_SWARM_ENERGY_Q8)
        return 0;
    *energy_q8-=SC64_DARK_SWARM_ENERGY_Q8;
    return 1;
}

/* Invoke only on the later source projectile-hit event, and only after a
 * successful effect allocation. Failure there does not refund cast energy.
 * OpenBW clamps the effect's outer unit bounds to map bounds with the bottom
 * 32 source pixels excluded. Caller supplies actual source-pixel map size. */
static int sc64_dark_swarm_probe_place(Sc64DarkSwarmEffect *effect,
                                       int x,int y,int width,int height)
{
    if (!effect || width<160 || height<192) return 0;
    if (x<80) x=80; else if (x+80>=width) x=width-80;
    if (y<80) y=80; else if (y+80>=height-32) y=height-112;
    sc64_dark_swarm_created(effect,x,y);
    return 1;
}
#endif
