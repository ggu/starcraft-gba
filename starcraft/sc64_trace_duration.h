/* Weapon tracer lifetime and slot bookkeeping. This is part of game.c and is
 * included there once; it lives in its own file to keep game.c under the
 * source-tree size guard.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_TRACE_DURATION_H
#define SC64_TRACE_DURATION_H

/* Reconstruct presentation age from existing endpoints; no new saved state. */
#if defined(SC64_TRACE_PROFILE_INDEX_CACHE) && defined(SC64_TRACE_DURATION_CACHE) && defined(SC64_WEAPON_SOURCE_VFX)
#include "sc64_trace_profile_cache.h"
#endif
/* `out` (nullable) receives the duration and, for source traces, the motion
 * values derived while computing it. */
static int __attribute__((noinline))
sc64_trace_duration_ex(int packed, int x0, int y0, int x1, int y1,
                       Sc64TraceSlot *out)
{
    int result;
    if (out) memset(out, 0, sizeof(*out));
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
    if (sc64_weapon_vfx_style(packed)==SC64_YAMATO_TRAIL_STYLE) { result=9; goto done; }
    if (sc64_weapon_vfx_style(packed)==SC64_YAMATO_IMPACT_STYLE) { result=36; goto done; }
#endif
#ifdef SC64_WEAPON_SOURCE_VFX
    if (sc64_weapon_source_vfx_is_tagged((uint32_t)packed)) {
        Sc64WeaponSourceVfxKey key;
        const Sc64WeaponSourceAttackProfile *profile =
            sc64_weapon_source_trace_profile((uint32_t)packed);
        if (!profile || !sc64_weapon_source_vfx_unpack((uint32_t)packed, &key)) return 0;
        int launch_x, launch_y;
        sc64_weapon_source_trace_launch_origin(profile, key.direction, x0, y0,
                                              &launch_x, &launch_y);
        Sc64WeaponSourceMotion motion =
            sc64_weapon_source_motion_prepare(profile, launch_x, launch_y, x1, y1);
        unsigned duration = sc64_weapon_source_trace_duration_motion(
            profile, launch_x, launch_y, x1, y1, &motion);
#ifdef SC64_UNIT_ATTACK_POSES_ENABLED
        if (key.actor_slot_plus1) {
            unsigned pose = sc64_weapon_source_vfx_gba_duration(
                sc64_unit_attack_source_full_duration(key.actor_raw,key.target_air));
            if (pose > duration) duration = pose;
        }
#endif
        if (out) {
            int index = sc64_weapon_source_profile_bank_index(profile);
            if (index >= 0 && index < 255 && (motion.distance >> 9) <= 0xffffu &&
                motion.flight <= 0xffffu) {
                out->root = (u16)(motion.distance >> 9);
                out->flight = (u16)motion.flight;
                out->profile_plus1 = (u8)(index + 1);
                out->motion_valid = 1;
            }
            out->duration = duration > 0 && duration <= 0xffffu ? (u16)duration : 0;
        }
        return (int)duration;
    }
#else
    (void)x0; (void)y0; (void)x1; (void)y1;
#endif
    result = sc64_weapon_vfx_lifetime(sc64_weapon_vfx_style(packed));
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
done:
#endif
#ifdef SC64_WEAPON_SOURCE_VFX
    if (out) out->duration = result > 0 && result <= 0xffff ? (u16)result : 0;
#endif
    return result;
}

static inline int
sc64_trace_duration(int packed, int x0, int y0, int x1, int y1)
{
    return sc64_trace_duration_ex(packed, x0, y0, x1, y1, 0);
}

static inline int
sc64_tracer_duration(int slot)
{
#ifdef SC64_WEAPON_SOURCE_VFX
    if (sc64_trace_slots[slot].duration) return sc64_trace_slots[slot].duration;
#endif
#ifdef SC64_TRACE_DURATION_CACHE
#ifdef SC64_TRACE_PROFILE_INDEX_CACHE
    if (sc64_trace_durations[slot] & 255u) return sc64_trace_durations[slot] & 255u;
#else
    if (sc64_trace_durations[slot]) return sc64_trace_durations[slot];
#endif
#endif
    const Tracer *t = &tracers[slot];
#ifdef SC64_WEAPON_SOURCE_VFX
    Sc64TraceSlot info;
    int duration = sc64_trace_duration_ex(t->color,t->x0,t->y0,t->x1,t->y1,&info);
    sc64_trace_slots[slot] = info; /* derived from the stored tracer itself */
#else
    int duration = sc64_trace_duration(t->color,t->x0,t->y0,t->x1,t->y1);
#endif
    sc64_trace_duration_cache_store(slot, duration);
    return duration;
}

#ifdef SC64_WEAPON_SOURCE_VFX
/* Fill a slot's profile and motion from the stored tracer. */
static void __attribute__((noinline))
sc64_trace_slot_prepare(int slot, Sc64WeaponSourceVfxKey key)
{
    const Tracer *t = &tracers[slot];
    Sc64TraceSlot *out = &sc64_trace_slots[slot];
    const Sc64WeaponSourceAttackProfile *profile = sc64_weapon_source_profile_lookup(
        key.actor_raw,key.weapon_id,key.target_air,key.variant);
    int index = profile ? sc64_weapon_source_profile_bank_index(profile) : -1;
    if (index < 0 || index >= 255) return;
    int launch_x, launch_y;
    sc64_weapon_source_trace_launch_origin(profile, key.direction, t->x0, t->y0,
                                          &launch_x, &launch_y);
    Sc64WeaponSourceMotion motion = sc64_weapon_source_motion_prepare(
        profile, launch_x, launch_y, t->x1, t->y1);
    if ((motion.distance >> 9) > 0xffffu || motion.flight > 0xffffu) return;
    out->root = (u16)(motion.distance >> 9);
    out->flight = (u16)motion.flight;
    out->profile_plus1 = (u8)(index + 1);
    out->motion_valid = 1;
}
#endif

static void __attribute__((noinline))
add_tracer(int x0, int y0, int x1, int y1, int color)
{
    int slot=0,oldest=-1;
    if (!color) return; /* Non-combat actor: no invented weapon. */
    if (!sc64_weapon_vfx_style(color))
        color=sc64_weapon_vfx_pack(SC64_WEAPON_VFX_STYLE_MUZZLE,color);
#ifdef SC64_WEAPON_SOURCE_VFX
    Sc64TraceSlot info;
    int duration=sc64_trace_duration_ex(color,x0,y0,x1,y1,&info);
#else
    int duration=sc64_trace_duration(color,x0,y0,x1,y1);
#endif
    if (duration<=0) return;
    for (int i=0;i<MAX_TRACERS;++i) {
        if (!tracers[i].life) { slot=i; break; }
        int age=sc64_tracer_duration(i)+1-tracers[i].life;
        if (age>oldest) { oldest=age; slot=i; }
    }
    /* Cosmetic overflow replaces the oldest effect, never a damage event.
     * Fresh shots cannot be starved indefinitely by an earlier volley. */
    tracers[slot].x0=x0; tracers[slot].y0=y0;
    tracers[slot].x1=x1; tracers[slot].y1=y1;
    tracers[slot].color=color;
#ifdef SC64_WEAPON_SOURCE_VFX
    /* The values above came from the arguments; they describe the slot only
     * if storing the endpoints did not narrow them. */
    if (tracers[slot].x0==x0 && tracers[slot].y0==y0 &&
        tracers[slot].x1==x1 && tracers[slot].y1==y1)
        sc64_trace_slots[slot]=info;
    else
        memset(&sc64_trace_slots[slot],0,sizeof(sc64_trace_slots[slot]));
#endif
    sc64_trace_duration_cache_store(slot, duration);
#if defined(SC64_TRACE_PROFILE_INDEX_CACHE) && defined(SC64_TRACE_DURATION_CACHE) && defined(SC64_WEAPON_SOURCE_VFX)
    if (duration <= 255 && sc64_weapon_source_vfx_is_tagged((uint32_t)color)) {
        Sc64WeaponSourceVfxKey key;
        if (sc64_weapon_source_vfx_unpack((uint32_t)color, &key)) {
            const Sc64WeaponSourceAttackProfile *p=sc64_weapon_source_profile_lookup(key.actor_raw,key.weapon_id,key.target_air,key.variant);
            sc64_trace_durations[slot] |= (u16)(sc64_trace_profile_slot_index(p,key)<<8);
        }
    }
#endif
    /* Combat emits before end-of-update aging, including the first draw. */
    tracers[slot].life=duration+1;
}

#endif
