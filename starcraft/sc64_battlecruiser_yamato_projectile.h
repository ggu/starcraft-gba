/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BATTLECRUISER_YAMATO_PROJECTILE_H
#define SC64_BATTLECRUISER_YAMATO_PROJECTILE_H
#include "sc64_battlecruiser_yamato_motion.h"
#include "sc64_battlecruiser_yamato_launch_rules.h"
#include "sc64_world_dark_swarm_query.h"
/* Private integration seam: heading and miss are supplied by a future
 * authenticated launch/RNG adapter. No UI or production caller uses this. */
static int race_bc_yamato_launch_resolved(int index,unsigned heading,int missed,
                                          int destination_x,int destination_y)
{
    Sc64BcYamatoShot *shot;
    Sc64BcMotionFields m={0}; Unit *caster;
    if (index<0 || index>=SC64_BC_SHOT_LIMIT || heading>255 || (missed!=0 && missed!=1) ||
        destination_x< -32 || destination_x>4159 || destination_y< -32 || destination_y>4159) return 0;
    shot=&sc64_bc_shots[index];
    if (shot->state!=SC64_BC_SHOT_CHARGING || shot->age!=SC64_BC_YAMATO_RELEASE_SOURCE_TICK ||
        shot->caster>=MAX_UNITS) return 0;
    caster=&u[shot->caster];
    if (!caster->alive || !race_bc_yamato_target_valid(shot->target,caster,0)) return 0;
    m.x_q8=(unsigned)caster->x*512u; m.y_q8=(unsigned)caster->y*512u;
    m.destination_x=(int16_t)destination_x;
    m.destination_y=(int16_t)destination_y;
    m.heading=m.velocity_direction=(uint8_t)heading;
    m.turn_rate=127; m.lifetime=255; m.turning=1;
#ifdef SC64_SHARED_PROJECTILE_STAGE
    /* Charging does not consume a projectile slot; acquire at release. */
    u32 shared_handle=sc64_shared_projectile_acquire(&sc64_shared_projectiles,
        SC64_SHARED_PROJECTILE_YAMATO,(unsigned)index);
    if(!shared_handle) return 0;
#endif
    shot->state=SC64_BC_SHOT_FLIGHT;
    shot->phase=(unsigned char)((shot->phase&63u)|(missed?64u:0u));
    if (!sc64_bc_motion_pack(shot,&m)) {
#ifdef SC64_SHARED_PROJECTILE_STAGE
        (void)sc64_shared_projectile_release(&sc64_shared_projectiles,shared_handle,
            SC64_SHARED_PROJECTILE_YAMATO,(unsigned)index);
#endif
        sc64_bc_shot_clear(shot); return 0;
    }
#ifdef SC64_SHARED_PROJECTILE_STAGE
    sc64_shared_yamato_handles[index]=shared_handle;
#endif
    /* Source CastSpell continues until its order signal at tick 50. */
    caster->work_timer=(caster->work_timer&~7)|6;
    return 1;
}
/* Source-policy launcher. Caller supplies the current source heading and
 * actual terrain/dark-swarm conditions; no default environment is invented. */
static int race_bc_yamato_launch_source(int index,unsigned heading,int terrain_cover,int dark_swarm)
{
    Sc64BcYamatoShot *shot; Sc64BcLaunchResolution r; Unit *caster;
    int flying=0,target;
    if (index<0 || index>=SC64_BC_SHOT_LIMIT || heading>255 ||
        (unsigned)terrain_cover>1 || (unsigned)dark_swarm>1) return 0;
    shot=&sc64_bc_shots[index];
    if (shot->state!=SC64_BC_SHOT_CHARGING || shot->age!=SC64_BC_YAMATO_RELEASE_SOURCE_TICK ||
        shot->caster>=MAX_UNITS) return 0;
    caster=&u[shot->caster]; target=shot->target;
    if (!caster->alive || !race_bc_yamato_target_valid(target,caster,0)) return 0;
    if (target<MAX_UNITS) flying=sc64_race_unit_flying(u[target].type);
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    else if (race_bc_facility_airborne(target-MAX_UNITS)) flying=1;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (target>=MAX_UNITS && race_late_airborne(target-MAX_UNITS)) flying=1;
#endif
#ifdef SC64_SHARED_PROJECTILE_STAGE
    /* Source obtains a bullet slot before initialization consumes the miss
     * roll. A full arena must leave the existing spell RNG unchanged. */
    if(!sc64_shared_projectile_has_capacity(&sc64_shared_projectiles)) return 0;
#endif
    if (!sc64_bc_yamato_resolve_launch(heading,target_x(target)*2,target_y(target)*2,
            !!flying,terrain_cover,dark_swarm,sc64_bc_spell_random()&255u,&r)) return 0;
    return race_bc_yamato_launch_resolved(index,r.heading,r.missed,r.x,r.y);
}
#ifdef SC64_CV5_COVER
/* Cover data must come from an explicitly cover-encoded map asset. The caller
 * still supplies source heading and actual Dark Swarm state. */
static int race_bc_yamato_launch_world_cover(int index,unsigned heading,int dark_swarm)
{
    if (index<0 || index>=SC64_BC_SHOT_LIMIT ||
        sc64_bc_shots[index].target>=MAX_UNITS+MAX_BUILDINGS) return 0;
    int target=sc64_bc_shots[index].target;
    return race_bc_yamato_launch_source(index,heading,
               terrain_provides_cover(target_x(target),target_y(target)),dark_swarm);
}
static int race_bc_yamato_launch_world(int index,unsigned heading)
{
    if (index<0 || index>=SC64_BC_SHOT_LIMIT ||
        sc64_bc_shots[index].target>=MAX_UNITS+MAX_BUILDINGS) return 0;
    return race_bc_yamato_launch_world_cover(index,heading,
               sc64_world_dark_swarm_affects_target(sc64_bc_shots[index].target));
}
#endif
/* A single authenticated source movement step. A missing target remains a
 * visible flight but cannot damage a recycled actor. */
static int race_bc_yamato_projectile_step(int index)
{
    Sc64BcYamatoShot *shot; Sc64BcMotionFields m; int result;
    if (index<0 || index>=SC64_BC_SHOT_LIMIT) return 0;
    shot=&sc64_bc_shots[index];
    if (shot->state!=SC64_BC_SHOT_FLIGHT || !sc64_bc_motion_unpack(shot,&m)) return 0;
    result=sc64_yamato_motion_step(&m,(unsigned)WORLD_W*2u,(unsigned)WORLD_H*2u);
    if (result<0) return 0;
    if (!result) {
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
        unsigned age=255u-(unsigned)m.lifetime;
        if (age>=5u && (age-5u)%3u==0)
            add_tracer((int)(m.x_q8/512u),(int)(m.y_q8/512u),
                       (int)(m.x_q8/512u),(int)(m.y_q8/512u),
                       sc64_bc_yamato_visual_tag(SC64_YAMATO_TRAIL_STYLE,(((unsigned)m.heading+4u)>>3)&31u));
#endif
        return sc64_bc_motion_pack(shot,&m);
    }
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
    add_tracer((int)(m.x_q8/512u),(int)(m.y_q8/512u),
               (int)(m.x_q8/512u),(int)(m.y_q8/512u),
               sc64_bc_yamato_visual_tag(SC64_YAMATO_IMPACT_STYLE,0));
#endif
    int target=shot->target, owner=shot->phase&7, missed=shot->phase&64;
#ifdef SC64_SHARED_PROJECTILE_STAGE
    if(!sc64_shared_projectile_release(&sc64_shared_projectiles,
        sc64_shared_yamato_handles[index],SC64_SHARED_PROJECTILE_YAMATO,(unsigned)index)) return 0;
    sc64_shared_yamato_handles[index]=0;
#endif
    sc64_bc_shot_clear(shot);
    if (!missed && target!=SC64_BC_SHOT_NO_TARGET && target_alive(target))
        hit_target_impl(target,260,sc64_race_unit_from_raw(12),owner,SC64_STOCK_DMG_EXPLOSIVE);
    return 1;
}
static int __attribute__((noinline)) race_bc_yamato_recovery_tick(Unit *q)
{
    unsigned recovery;
    if (!q || !q->alive || !sc64_race_is_battlecruiser(q->type)) return 0;
    recovery=(unsigned)q->work_timer&7u;
    if (!recovery) return 0;
    if (recovery>6 || q->order!=ORDER_ATTACK_TARGET) { q->work_timer&=~7; return 0; }
    if (sc64_battlecruiser_source_tick_due()) {
        --recovery; q->work_timer=(q->work_timer&~7)|(int)recovery;
        if (!recovery) { q->order=ORDER_IDLE; q->target=-1; }
    }
    return 1;
}
#endif
