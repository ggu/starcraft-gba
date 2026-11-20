/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Tracer drawing for the shared game renderer. Included by game.c. */
#define SC64_WEAPON_VFX_VISIBLE(x,y) currently_visible((x)+camx,(y)+camy-12)
#include "sc64_weapon_vfx_render.h"
#undef SC64_WEAPON_VFX_VISIBLE
#ifdef SC64_TANK_SOURCE_VFX
#include "sc64_tank_source_vfx_render.h"
#endif

#ifdef SC64_WEAPON_SOURCE_VFX
#include "sc64_weapon_source_vfx_render.h"
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
#include "sc64_yamato_raster_adapter.h"
#endif
#endif


static void __attribute__((noinline))
draw_tracer(int i, unsigned parts)
{
#ifndef SC64_WEAPON_SOURCE_VFX
    (void)parts;
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
    int ys=sc64_weapon_vfx_style(tracers[i].color);
    if (ys==SC64_YAMATO_TRAIL_STYLE || ys==SC64_YAMATO_IMPACT_STYLE) {
        int age=sc64_tracer_duration(i)-tracers[i].life;
        unsigned p=(unsigned)tracers[i].color&7u;
        unsigned source_age=(unsigned)(age<=0?0:age-1);
        source_age-= (source_age+p)/5u;
        if (ys==SC64_YAMATO_TRAIL_STYLE && source_age<6u)
            sc64_yamato_raster_draw(542,(source_age/2u)*17u,
                                     ((unsigned)tracers[i].color>>3)&31u,tracers[i].x0,tracers[i].y0);
        if (ys==SC64_YAMATO_IMPACT_STYLE && source_age<28u)
            sc64_yamato_raster_draw(544,source_age/2u,0,tracers[i].x0,tracers[i].y0);
        return;
    }
#endif
#ifdef SC64_WEAPON_SOURCE_VFX
    if(sc64_weapon_source_vfx_is_tagged((uint32_t)tracers[i].color)) {
        uint32_t packed=(uint32_t)tracers[i].color;
        Sc64WeaponSourceVfxKey key;
        if (!sc64_weapon_source_vfx_unpack(packed,&key)) return;
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
        u32 dt=gba_clock();
#endif
        const Sc64WeaponSourceAttackProfile *profile=0;
        Sc64WeaponSourceMotion slot_motion;
        const Sc64WeaponSourceMotion *prepared=0;
        if (!sc64_trace_slots[i].profile_plus1) sc64_trace_slot_prepare(i,key);
        if (sc64_trace_slots[i].profile_plus1) {
            const Sc64TraceSlot *ts=&sc64_trace_slots[i];
            profile=&sc64_weapon_source_profiles[ts->profile_plus1-1u];
            slot_motion.distance=(uint64_t)ts->root<<9;
            slot_motion.flight=ts->flight;
            prepared=&slot_motion;
        }
#if defined(SC64_TRACE_PROFILE_INDEX_CACHE) && defined(SC64_TRACE_DURATION_CACHE)
        if(!profile) {
            unsigned cached=sc64_trace_durations[i]>>8;
            profile=sc64_trace_cached_profile(cached,key);
        }
#endif
        if(!profile) profile=sc64_weapon_source_profile_lookup(
            key.actor_raw,key.weapon_id,key.target_air,key.variant);
        if (parts == SC64_TRACE_ATTACKER && (!profile || !profile->overlay_count)) return;
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
        diag_lookup+=gba_clock()-dt; dt=gba_clock();
#endif
#ifdef SC64_WEAPON_SOURCE_VFX_OFFSCREEN_CULL
        if (!sc64_weapon_source_vfx_anchor_may_be_visible(
                tracers[i].x0, tracers[i].y0, camx, camy) &&
            !sc64_weapon_source_vfx_anchor_may_be_visible(
                tracers[i].x1, tracers[i].y1, camx, camy) &&
            sc64_weapon_source_vfx_trace_provably_offscreen_resolved(
                key,profile,
                tracers[i].x0, tracers[i].y0, tracers[i].x1, tracers[i].y1,
                camx, camy)) {
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
            diag_cull+=gba_clock()-dt;
#endif
            return;
        }
#endif
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
        diag_cull+=gba_clock()-dt; dt=gba_clock();
#endif
        int age=sc64_tracer_duration(i)-tracers[i].life;
        sc64_weapon_source_vfx_draw_trace_parts(key,profile,
            tracers[i].x0,tracers[i].y0,tracers[i].x1,tracers[i].y1,
            tracers[i].life,(unsigned)(age<0?0:age),prepared,parts);
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
        diag_raster+=gba_clock()-dt;
#endif
        return;
    }
#endif
    int style=sc64_weapon_vfx_style(tracers[i].color);
    /* Legacy original-art saves may contain an untagged tracer color. */
    if (!style) style=SC64_WEAPON_VFX_STYLE_MUZZLE;
#ifdef SC64_TANK_SOURCE_VFX
    if (style==SC64_TANK_SOURCE_TANK || style==SC64_TANK_SOURCE_SIEGE) {
        sc64_private_tank_vfx_draw(style,sc64_weapon_vfx_color(tracers[i].color),
            tracers[i].x0,tracers[i].y0,tracers[i].x1,tracers[i].y1,tracers[i].life);
        return;
    }
#endif
    sc64_weapon_vfx_draw(style,sc64_weapon_vfx_color(tracers[i].color),
        tracers[i].x0-camx,tracers[i].y0-camy+12,
        tracers[i].x1-camx,tracers[i].y1-camy+12,tracers[i].life);
}

#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
static void
sc64_tracer_profile_reset(void)
{
    diag_tracer_cycles=0;
    diag_lookup=diag_cull=diag_raster=diag_motion=diag_actor=diag_projectile=diag_children=0;
}
#endif

static void
draw_tracers_pass(int after_world)
{
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
    if (!after_world) sc64_tracer_profile_reset();
    u32 diag_start=gba_clock();
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
    for (int i=0;i<SC64_BC_SHOT_LIMIT;++i) {
        const Sc64BcYamatoShot *shot=&sc64_bc_shots[i];
        if (shot->state==SC64_BC_SHOT_CHARGING && shot->caster<MAX_UNITS && u[shot->caster].alive) {
            int f=sc64_yamato_raster_charge_frame(shot->age);
            if (f>=0) sc64_yamato_raster_draw(543,(unsigned)f,0,u[shot->caster].x,u[shot->caster].y);
        } else if (shot->state==SC64_BC_SHOT_FLIGHT) {
            Sc64BcMotionFields m;
            if (sc64_bc_motion_unpack(shot,&m))
                sc64_yamato_raster_draw(541,0,(((unsigned)m.heading+4u)>>3)&31u,(int)(m.x_q8/512u),(int)(m.y_q8/512u));
        }
    }
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && sc64_race_is_battlecruiser(u[i].type) &&
        u[i].order==ORDER_ATTACK_TARGET && (u[i].work_timer&7)>0 && (u[i].work_timer&7)<=6) {
        int f=sc64_yamato_raster_charge_frame(50u-((unsigned)u[i].work_timer&7u));
        if (f>=0) sc64_yamato_raster_draw(543,(unsigned)f,0,u[i].x,u[i].y);
    }
#endif
#ifndef SC64_PROJECTILE_DEPTH
    (void)after_world;
#endif
    for (int i = 0; i < MAX_TRACERS; ++i) {
        if (!tracers[i].life) continue;
#ifdef SC64_PROJECTILE_DEPTH
        if (after_world && sc64_trace_slots[i].bullet_layer) {
            draw_tracer(i, SC64_TRACE_ATTACKER);
            continue;
        }
#endif
        draw_tracer(i, SC64_TRACE_ALL);
    }
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
    diag_tracer_cycles+=gba_clock()-diag_start;
#endif
}

static void __attribute__((unused))
draw_tracers(void)
{
    draw_tracers_pass(0);
}
