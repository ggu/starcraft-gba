/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_WEAPON_SOURCE_RUNTIME_H
#define SC64_WEAPON_SOURCE_RUNTIME_H

#include "sc64_weapon_source_vfx.h"
#ifndef SC64_WEAPON_SOURCE_BANK_HEADER
#define SC64_WEAPON_SOURCE_BANK_HEADER "sc64_weapon_source_generated.h"
#endif
#include SC64_WEAPON_SOURCE_BANK_HEADER

#if !defined(SC64_WEAPON_SOURCE_BANK_VERSION) || SC64_WEAPON_SOURCE_BANK_VERSION != 1
#error "Regenerate the versioned source weapon VFX bank"
#endif

#define SC64_SOURCE_PROFILE_HAS_PROJECTILE 0x01u
#define SC64_SOURCE_BULLET_FLY 0u
#define SC64_SOURCE_BULLET_FOLLOW 1u
#define SC64_SOURCE_BULLET_APPEAR_TARGET_UNIT 2u
#define SC64_SOURCE_BULLET_PERSIST_TARGET_POS 3u
#define SC64_SOURCE_BULLET_APPEAR_TARGET_POS 4u
#define SC64_SOURCE_BULLET_APPEAR_SOURCE 5u
#define SC64_SOURCE_BULLET_SELF_DESTRUCT 6u
#define SC64_SOURCE_BULLET_BOUNCE 7u
#define SC64_SOURCE_BULLET_ATTACK_TARGET_POS 8u
#define SC64_SOURCE_BULLET_EXTEND_MAX_RANGE 9u

static inline const Sc64WeaponSourceAttackProfile *
sc64_weapon_source_trace_profile(uint32_t packed)
{
    Sc64WeaponSourceVfxKey key;
    if (!sc64_weapon_source_vfx_unpack(packed, &key)) return 0;
    return sc64_weapon_source_profile_lookup(key.actor_raw, key.weapon_id,
                                              key.target_air, key.variant);
}

static inline uint32_t
sc64_weapon_source_trace_sqroot(uint64_t n)
{
    uint64_t bit = UINT64_C(1) << 62;
    uint64_t root = 0;
    while (bit > n) bit >>= 2;
    while (bit) {
        if (n >= root + bit) {
            n -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root > UINT32_MAX ? UINT32_MAX : (uint32_t)root;
}

static inline uint64_t
sc64_weapon_source_trace_distance_raw(int x0, int y0, int x1, int y1)
{
    int64_t dx = (int64_t)x1 - x0, dy = (int64_t)y1 - y0;
    uint64_t ax = (uint64_t)(dx < 0 ? -dx : dx);
    uint64_t ay = (uint64_t)(dy < 0 ? -dy : dy);
    uint64_t xx = ax * ax, yy = ay * ay;
    uint64_t squared = UINT64_MAX - xx < yy ? UINT64_MAX : xx + yy;
    return (uint64_t)sc64_weapon_source_trace_sqroot(squared) * 512u;
}

static const int16_t sc64_weapon_source_direction_q8[32][2] = {
    {0,-256},{50,-251},{98,-237},{142,-213},{181,-181},{213,-142},{237,-98},{251,-50},
    {256,0},{251,50},{237,98},{213,142},{181,181},{142,213},{98,237},{50,251},
    {0,256},{-50,251},{-98,237},{-142,213},{-181,181},{-213,142},{-237,98},{-251,50},
    {-256,0},{-251,-50},{-237,-98},{-213,-142},{-181,-181},{-142,-213},{-98,-237},{-50,-251}
};

static inline void
sc64_weapon_source_trace_launch_origin(const Sc64WeaponSourceAttackProfile *p,
                                       uint8_t direction, int x, int y,
                                       int *launch_x, int *launch_y)
{
    if (!launch_x || !launch_y) return;
    if (!p || direction > 31) { *launch_x = x; *launch_y = y; return; }
    const int16_t *v = sc64_weapon_source_direction_q8[direction];
    *launch_x = x + ((int)p->forward_offset * v[0]) / 512;
    *launch_y = y + ((int)p->forward_offset * v[1]) / 512 -
                (int)p->upward_offset / 2;
}

/* Distance-time preview for straight source flingies. World positions use
 * GBA pixels; source coordinates are twice that scale. Speed and acceleration
 * are OpenBW fp8 raw values. This predicts arrival duration but does not
 * simulate source turning, homing corrections, collision, or bounces. */
static inline int sc64_weapon_source_bullet_is_stationary(uint8_t bullet_type);

static inline uint32_t
sc64_weapon_source_trace_flight_ticks(const Sc64WeaponSourceAttackProfile *p,
                                      int x0, int y0, int x1, int y1)
{
    uint64_t remaining = sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1);
    int64_t top = p->speed_raw;
    int64_t accel = p->accel_raw;
    uint32_t limit = p->projectile_lifetime ? p->projectile_lifetime : 65535u;
    int64_t speed = 0;
    if (!remaining || sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type))
        return 0;
    if (top <= 0) return 0;
    if (accel <= 0) speed = top;
    for (uint32_t tick = 1; tick <= limit; ++tick) {
        if (accel > 0) {
            speed += accel;
            if (speed > top) speed = top;
        }
        if (speed <= 0) speed = top;
        if ((uint64_t)speed >= remaining) return tick;
        remaining -= (uint64_t)speed;
    }
    return limit;
}

static inline int
sc64_weapon_source_bullet_is_stationary(uint8_t bullet_type)
{
    /* OpenBW weapon_type_t order: target-unit, persist-target-pos,
     * target-pos, source, and self-destruct all stay at one point. */
    return bullet_type >= SC64_SOURCE_BULLET_APPEAR_TARGET_UNIT &&
           bullet_type <= SC64_SOURCE_BULLET_SELF_DESTRUCT;
}

static inline uint32_t
sc64_weapon_source_trace_startup_ticks(const Sc64WeaponSourceAttackProfile *p)
{
    const Sc64WeaponSourceAnimation *init;
    if (!p || !p->projectile_image) return 0;
    init = sc64_weapon_source_animation_lookup(p->projectile_image, 0u);
    if (!init || init->event_first > SC64_WEAPON_SOURCE_EVENT_COUNT ||
        init->event_count > SC64_WEAPON_SOURCE_EVENT_COUNT - init->event_first)
        return 0;
    for (uint16_t i = 0; i < init->event_count; ++i) {
        const Sc64WeaponSourceEvent *event =
            &sc64_weapon_source_events[init->event_first + i];
        /* IScript setspawnframe sends order_signal=1, transitioning an
         * initialized bullet into its GndAttkInit state. */
        if (event->opcode == 36u) return event->source_tick;
    }
    return 0;
}

static inline uint32_t
sc64_weapon_source_trace_release_ticks(const Sc64WeaponSourceAttackProfile *p)
{
    return !p || p->attackwith_tick == UINT16_MAX ? 0u : p->attackwith_tick;
}

static inline uint32_t
sc64_weapon_source_animation_visual_ticks(const Sc64WeaponSourceAnimation *anim)
{
    uint64_t end = anim ? anim->duration_ticks : 0;
    if (!anim || anim->overlay_first > SC64_WEAPON_SOURCE_OVERLAY_COUNT ||
        anim->overlay_count > SC64_WEAPON_SOURCE_OVERLAY_COUNT - anim->overlay_first)
        return (uint32_t)end;
    for (uint16_t i = 0; i < anim->overlay_count; ++i) {
        const Sc64WeaponSourceOverlay *overlay =
            &sc64_weapon_source_overlays[anim->overlay_first + i];
        const Sc64WeaponSourceAnimation *init =
            sc64_weapon_source_animation_lookup(overlay->image_id, 0u);
        if (init && init->loop_pc != UINT32_MAX) continue;
        uint64_t child = init ? init->duration_ticks : 1u;
        uint64_t overlay_end = (uint64_t)overlay->source_tick + child;
        if (overlay_end > end) end = overlay_end;
    }
    return end > UINT32_MAX ? UINT32_MAX : (uint32_t)end;
}

static inline int
sc64_weapon_source_trace_position(const Sc64WeaponSourceAttackProfile *p,
                                  int x0, int y0, int x1, int y1,
                                  uint32_t source_age, int *x, int *y)
{
    if (!p || !x || !y) return 0;
    if (p->weapon_bullet_type == SC64_SOURCE_BULLET_APPEAR_SOURCE ||
        p->weapon_bullet_type == SC64_SOURCE_BULLET_SELF_DESTRUCT) {
        *x = x0;
        *y = y0;
        return 1;
    }
    if (p->weapon_bullet_type == SC64_SOURCE_BULLET_APPEAR_TARGET_UNIT ||
        p->weapon_bullet_type == SC64_SOURCE_BULLET_PERSIST_TARGET_POS ||
        p->weapon_bullet_type == SC64_SOURCE_BULLET_APPEAR_TARGET_POS) {
        *x = x1;
        *y = y1;
        return 1;
    }
    int64_t dx = (int64_t)x1 - x0, dy = (int64_t)y1 - y0;
    uint64_t distance = sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1);
    uint32_t flight = sc64_weapon_source_trace_flight_ticks(p, x0, y0, x1, y1);
    if (!flight) {
        if (!sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1)) {
            *x = x1;
            *y = y1;
            return 1;
        }
        return 0;
    }
    if (source_age >= flight || !distance) {
        *x = x1;
        *y = y1;
        return 1;
    }
    int64_t speed = 0, top = p->speed_raw;
    int64_t accel = p->accel_raw;
    uint64_t travelled = 0;
    for (uint32_t tick = 0; tick < source_age; ++tick) {
        if (accel > 0) {
            speed += accel;
            if (speed > top) speed = top;
        } else {
            speed = top;
        }
        if (speed <= 0) speed = top;
        travelled += (uint64_t)speed;
        if (travelled >= distance) { travelled = distance; break; }
    }
    *x = (int)((int64_t)x0 + dx * (int64_t)travelled / (int64_t)distance);
    *y = (int)((int64_t)y0 + dy * (int64_t)travelled / (int64_t)distance);
    return 1;
}

static inline uint32_t
sc64_weapon_source_trace_duration(const Sc64WeaponSourceAttackProfile *p,
                                  int x0, int y0, int x1, int y1)
{
    uint32_t flight, startup_ticks, death_ticks = 0;
    uint32_t source_ticks, actor_ticks = 0, release_ticks;
    const Sc64WeaponSourceAnimation *anim;
    if (!p || !p->projectile_image ||
        !sc64_weapon_source_image_lookup(p->projectile_image)) return 0;
    flight = sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type) ? 0 :
             sc64_weapon_source_trace_flight_ticks(p, x0, y0, x1, y1);
    if (!sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type) &&
        sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1) && !flight)
        return 0;
    startup_ticks = sc64_weapon_source_trace_startup_ticks(p);
    anim = sc64_weapon_source_animation_lookup(p->projectile_image, 1u);
    if (anim) death_ticks = sc64_weapon_source_animation_visual_ticks(anim);
    /* The order signal enters GndAttkInit. For moving bullets the death script
     * starts at distance-derived arrival/expiry; stationary bullets transition
     * to Death immediately after the authored startup event. */
    uint32_t active_ticks = flight;
    release_ticks = sc64_weapon_source_trace_release_ticks(p);
    source_ticks = release_ticks + startup_ticks + active_ticks + death_ticks;
    if (!source_ticks) source_ticks = p->projectile_frame_count ? 1u : 0u;
    anim = sc64_weapon_source_animation_lookup(p->attack_image, p->attack_anim);
    if (anim && anim->loop_pc == UINT32_MAX) {
        uint32_t visual = sc64_weapon_source_animation_visual_ticks(anim);
        actor_ticks = visual;
    }
    if (actor_ticks > source_ticks) source_ticks = actor_ticks;
    return sc64_weapon_source_vfx_gba_duration(source_ticks);
}

static inline uint32_t
sc64_weapon_source_trace_duration_heading(const Sc64WeaponSourceAttackProfile *p,
                                          uint8_t direction,
                                          int x0, int y0, int x1, int y1)
{
    int launch_x, launch_y;
    sc64_weapon_source_trace_launch_origin(p, direction, x0, y0,
                                          &launch_x, &launch_y);
    return sc64_weapon_source_trace_duration(p, launch_x, launch_y, x1, y1);
}

#endif
