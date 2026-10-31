/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_WORLD_DARK_SWARM_QUERY_H
#define SC64_WORLD_DARK_SWARM_QUERY_H
/* Full shared capability remains disabled pending all projectile families,
 * geometry profiles, saves, art and runtime qualification. */
#ifdef SC64_SHARED_DARK_SWARM
#error "Bind sc64_world_dark_swarm_affects_target to the shared effect state"
#endif
#ifdef SC64_SHARED_DARK_SWARM_PROBE
#if !defined(SC64_SHARED_DEFILER_EXECUTOR) || !defined(SC64_BATTLECRUISER_YAMATO_EXPERIMENT)
#error "Dark Swarm query probe requires staged fields and the staged projectile header"
#endif
#ifdef SC64_SHARED_DARK_SWARM_SOURCE_RECTS
#include "sc64_dark_swarm_rect_lookup.h"
#else
#ifndef SC64_P7_ACTORS
#error "Dark Swarm query requires source rectangles or P7 actor facts"
#endif
#include "sc64_p7_geometry.h"
#endif
/* Returns -1 when a live field exists but target geometry is unknown; callers
 * must fail closed rather than silently launch through an unqualified field. */
static int sc64_world_dark_swarm_affects_target(int target)
{
    if(target<0 || target>=MAX_UNITS || !u[target].alive || sc64_race_unit_flying(u[target].type))return 0;
    int active=0;
    for(int i=0;i<MAX_UNITS;i++)
        if(sc64_defiler_field_get(i).remaining && sc64_defiler_field_get(i).remaining<=900) {active=1;break;}
    if(!active)return 0;
    Unit *q=&u[target];int raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
    int left,top,right,bottom;
#ifdef SC64_SHARED_DARK_SWARM_SOURCE_RECTS
    if(!sc64_dark_swarm_source_rect(raw,q->x,q->y,WORLD_W,WORLD_H,
                                    &left,&top,&right,&bottom))return -1;
#else
    if(raw<0 || !sc64_p7_actor_rect_source((unsigned)raw,q->x,q->y,0,&left,&top,&right,&bottom))return -1;
#endif
    for(int i=0;i<MAX_UNITS;i++) {
        Sc64DarkSwarmEffect field=sc64_defiler_field_get(i), *f=&field;
        if(!f->remaining || f->remaining>900)continue;
        if(left<f->x+80 && right>f->x-80 && top<f->y+80 && bottom>f->y-80)return 1;
    }
    return 0;
}
#else
static int sc64_world_dark_swarm_affects_target(int target)
{ (void)target; return 0; }
#endif
#endif
