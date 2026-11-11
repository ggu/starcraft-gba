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

#if !defined(SC64_WEAPON_SOURCE_SPROLUSELO_LO) || SC64_WEAPON_SOURCE_SPROLUSELO_LO != 1
#error "Regenerate the source weapon VFX bank with SPROLUSELO LO offsets"
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
    if (n <= UINT32_MAX) {
        /* Map-sized distances: the same digit-by-digit root in 32 bits. */
        uint32_t m = (uint32_t)n, bit32 = UINT32_C(1) << 30, root32 = 0;
        while (bit32 > m) bit32 >>= 2;
        while (bit32) {
            if (m >= root32 + bit32) {
                m -= root32 + bit32;
                root32 = (root32 >> 1) + bit32;
            } else {
                root32 >>= 1;
            }
            bit32 >>= 2;
        }
        return root32;
    }
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
sc64_weapon_source_trace_flight_ticks_distance(const Sc64WeaponSourceAttackProfile *p,
                                               uint64_t remaining)
{
    int64_t top = p->speed_raw;
    int64_t accel = p->accel_raw;
    uint32_t limit = p->projectile_lifetime ? p->projectile_lifetime : 65535u;
    int64_t speed = 0;
    if (!remaining || sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type))
        return 0;
    if (top <= 0) return 0;
    if (remaining <= INT32_MAX) {
        /* Same arithmetic in 32 bits: top <= INT32_MAX, accel <= INT16_MAX,
         * so speed + accel and remaining + top - 1 cannot wrap. */
        uint32_t rem = (uint32_t)remaining, top32 = (uint32_t)top, speed32 = 0;
        uint32_t tick32 = 1;
        if (accel <= 0) speed32 = top32;
        else {
            uint32_t accel32 = (uint32_t)accel;
            for (; tick32 <= limit && speed32 < top32; ++tick32) {
                speed32 += accel32;
                if (speed32 > top32) speed32 = top32;
                if (speed32 >= rem) return tick32;
                rem -= speed32;
            }
        }
        if (tick32 > limit) return limit;
        uint32_t arrival32 = tick32 - 1u + (rem + top32 - 1u) / top32;
        return arrival32 > limit ? limit : arrival32;
    }
    if (accel <= 0) speed = top;
    /* Step tick by tick only while accelerating toward top speed. */
    uint32_t tick = 1;
    for (; tick <= limit && accel > 0 && speed < top; ++tick) {
        speed += accel;
        if (speed > top) speed = top;
        if (speed <= 0) speed = top;
        if ((uint64_t)speed >= remaining) return tick;
        remaining -= (uint64_t)speed;
    }
    if (tick > limit) return limit;
    /* At top speed the first covering tick is ceil(remaining / top) away. */
    uint64_t arrival = (uint64_t)tick - 1u + (remaining + (uint64_t)top - 1u) / (uint64_t)top;
    return arrival > limit ? limit : (uint32_t)arrival;
}

static inline uint32_t
sc64_weapon_source_trace_flight_ticks(const Sc64WeaponSourceAttackProfile *p,
                                      int x0, int y0, int x1, int y1)
{
    return sc64_weapon_source_trace_flight_ticks_distance(
        p, sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1));
}

/* Internal values may only be reused with the identical profile and endpoints
 * passed to prepare; NULL requests fresh values. Profiles must not change. */
typedef struct {
    uint64_t distance;
    uint32_t flight;
} Sc64WeaponSourceMotion;

static inline Sc64WeaponSourceMotion
sc64_weapon_source_motion_prepare(const Sc64WeaponSourceAttackProfile *p,
                                  int x0, int y0, int x1, int y1)
{
    Sc64WeaponSourceMotion m = {0, 0};
    if (p && !sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type)) {
        m.distance = sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1);
        m.flight = sc64_weapon_source_trace_flight_ticks_distance(p, m.distance);
    }
    return m;
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
        /* IScript sigorder with mask 1 sets order_signal |= 1, transitioning an
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
sc64_weapon_source_trace_position_motion(const Sc64WeaponSourceAttackProfile *p,
                                         int x0, int y0, int x1, int y1,
                                         uint32_t source_age, int *x, int *y,
                                         const Sc64WeaponSourceMotion *motion)
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
    Sc64WeaponSourceMotion local;
    if (!motion) {
        local = sc64_weapon_source_motion_prepare(p, x0, y0, x1, y1);
        motion = &local;
    }
    uint64_t distance = motion->distance;
    uint32_t flight = motion->flight;
    if (!flight) {
        if (!distance) {
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
    if (distance <= INT32_MAX) {
        /* Same walk in 32 bits: speed <= top <= INT32_MAX and travelled <
         * distance <= INT32_MAX before each add, so nothing wraps. */
        uint32_t d32 = (uint32_t)distance, top32 = (uint32_t)top;
        uint32_t speed32 = 0, travelled32 = 0, tick32 = 0;
        for (; tick32 < source_age && accel > 0 && speed32 < top32; ++tick32) {
            speed32 += (uint32_t)accel;
            if (speed32 > top32) speed32 = top32;
            travelled32 += speed32;
            if (travelled32 >= d32) { travelled32 = d32; break; }
        }
        if (travelled32 < d32 && tick32 < source_age) {
            uint32_t rest = source_age - tick32, room = d32 - travelled32;
            travelled32 = rest >= (room + top32 - 1u) / top32 ?
                          d32 : travelled32 + rest * top32;
        }
        int64_t px = dx * (int64_t)travelled32, py = dy * (int64_t)travelled32;
        int64_t qx = px >= INT32_MIN && px <= INT32_MAX ?
            (int64_t)((int32_t)px / (int32_t)d32) : px / (int64_t)d32;
        int64_t qy = py >= INT32_MIN && py <= INT32_MAX ?
            (int64_t)((int32_t)py / (int32_t)d32) : py / (int64_t)d32;
        *x = (int)((int64_t)x0 + qx);
        *y = (int)((int64_t)y0 + qy);
        return 1;
    }
    /* Step tick by tick only while accelerating; top > 0 since flight > 0. */
    uint32_t tick = 0;
    for (; tick < source_age && accel > 0 && speed < top; ++tick) {
        speed += accel;
        if (speed > top) speed = top;
        if (speed <= 0) speed = top;
        travelled += (uint64_t)speed;
        if (travelled >= distance) { travelled = distance; break; }
    }
    if (travelled < distance && tick < source_age) {
        /* The remaining ticks each cover top, capped at the distance. */
        uint64_t rest = source_age - tick, room = distance - travelled;
        travelled = rest >= (room + (uint64_t)top - 1u) / (uint64_t)top ?
                    distance : travelled + rest * (uint64_t)top;
    }
    *x = (int)((int64_t)x0 + dx * (int64_t)travelled / (int64_t)distance);
    *y = (int)((int64_t)y0 + dy * (int64_t)travelled / (int64_t)distance);
    return 1;
}

static inline int
sc64_weapon_source_trace_position(const Sc64WeaponSourceAttackProfile *p,
                                  int x0, int y0, int x1, int y1,
                                  uint32_t source_age, int *x, int *y)
{
    return sc64_weapon_source_trace_position_motion(p, x0, y0, x1, y1,
                                                     source_age, x, y, 0);
}

/* Per-profile values that depend only on the immutable bank. */
typedef struct {
    uint32_t startup, death, actor;
    int image_ok;
} Sc64WeaponSourceProfileTicks;

static inline void
sc64_weapon_source_profile_ticks_compute(const Sc64WeaponSourceAttackProfile *p,
                                         Sc64WeaponSourceProfileTicks *t)
{
    const Sc64WeaponSourceAnimation *anim;
    t->startup = t->death = t->actor = 0;
    t->image_ok = p && p->projectile_image &&
                  sc64_weapon_source_image_lookup(p->projectile_image) != 0;
    if (!t->image_ok) return;
    t->startup = sc64_weapon_source_trace_startup_ticks(p);
    anim = sc64_weapon_source_animation_lookup(p->projectile_image, 1u);
    if (anim) t->death = sc64_weapon_source_animation_visual_ticks(anim);
    anim = sc64_weapon_source_animation_lookup(p->attack_image, p->attack_anim);
    if (anim && anim->loop_pc == UINT32_MAX)
        t->actor = sc64_weapon_source_animation_visual_ticks(anim);
}

#ifdef SC64_WEAPON_SOURCE_TICKS_CACHE
/* Index of an exact record of the const generated bank, or -1. Copies and
 * synthetic profiles are never cached. */
static inline int
sc64_weapon_source_profile_bank_index(const Sc64WeaponSourceAttackProfile *p)
{
    uintptr_t base = (uintptr_t)(const void *)sc64_weapon_source_profiles;
    uintptr_t addr = (uintptr_t)(const void *)p;
    if (addr < base) return -1;
    addr -= base;
    if (addr % sizeof(sc64_weapon_source_profiles[0])) return -1;
    addr /= sizeof(sc64_weapon_source_profiles[0]);
    return addr < SC64_WEAPON_SOURCE_PROFILE_COUNT ? (int)addr : -1;
}

/* Filled lazily, one profile at a time; 8 bytes per bank profile of RAM.
 * Values that do not fit 16 bits are recomputed instead of cached. */
static inline void
sc64_weapon_source_profile_ticks(const Sc64WeaponSourceAttackProfile *p,
                                 Sc64WeaponSourceProfileTicks *t)
{
    static uint16_t cache[SC64_WEAPON_SOURCE_PROFILE_COUNT][4];
    int index = sc64_weapon_source_profile_bank_index(p);
    if (index >= 0) {
        const uint16_t *c = cache[index];
        if (c[3]) {
            t->startup = c[0]; t->death = c[1]; t->actor = c[2];
            t->image_ok = c[3] == 2u;
            return;
        }
    }
    sc64_weapon_source_profile_ticks_compute(p, t);
    if (index >= 0 && t->startup <= 0xffffu && t->death <= 0xffffu &&
        t->actor <= 0xffffu) {
        uint16_t *c = cache[index];
        c[0] = (uint16_t)t->startup; c[1] = (uint16_t)t->death;
        c[2] = (uint16_t)t->actor; c[3] = t->image_ok ? 2u : 1u;
    }
}
#else
static inline void
sc64_weapon_source_profile_ticks(const Sc64WeaponSourceAttackProfile *p,
                                 Sc64WeaponSourceProfileTicks *t)
{
    sc64_weapon_source_profile_ticks_compute(p, t);
}
#endif

/* Duration from already prepared motion and profile values. */
static inline uint32_t
sc64_weapon_source_trace_duration_ticks(const Sc64WeaponSourceAttackProfile *p,
                                        const Sc64WeaponSourceMotion *motion,
                                        const Sc64WeaponSourceProfileTicks *t)
{
    uint32_t flight, source_ticks;
    if (!t->image_ok) return 0;
    flight = motion->flight;
    if (!sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type) &&
        motion->distance && !flight) return 0;
    /* The order signal enters GndAttkInit. For moving bullets the death script
     * starts at distance-derived arrival/expiry; stationary bullets transition
     * to Death immediately after the authored startup event. */
    source_ticks = sc64_weapon_source_trace_release_ticks(p) + t->startup +
                   flight + t->death;
    if (!source_ticks) source_ticks = p->projectile_frame_count ? 1u : 0u;
    if (t->actor > source_ticks) source_ticks = t->actor;
    return sc64_weapon_source_vfx_gba_duration(source_ticks);
}

static inline uint32_t
sc64_weapon_source_trace_duration_motion(const Sc64WeaponSourceAttackProfile *p,
                                         int x0, int y0, int x1, int y1,
                                         const Sc64WeaponSourceMotion *motion)
{
    Sc64WeaponSourceProfileTicks t;
    if (!p) return 0;
    sc64_weapon_source_profile_ticks(p, &t);
    if (!t.image_ok) return 0;
    Sc64WeaponSourceMotion local;
    if (!motion) {
        local = sc64_weapon_source_motion_prepare(p, x0, y0, x1, y1);
        motion = &local;
    }
    return sc64_weapon_source_trace_duration_ticks(p, motion, &t);
}

static inline uint32_t
sc64_weapon_source_trace_duration(const Sc64WeaponSourceAttackProfile *p,
                                  int x0, int y0, int x1, int y1)
{
    return sc64_weapon_source_trace_duration_motion(p, x0, y0, x1, y1, 0);
}

/* Nonzero exactly when sc64_weapon_source_trace_duration() is nonzero. A
 * projectile with frames always has a duration, so the overlay walks run only
 * for frameless projectiles. */
static inline int
sc64_weapon_source_trace_has_duration_ticks(const Sc64WeaponSourceAttackProfile *p,
                                            const Sc64WeaponSourceMotion *motion,
                                            const Sc64WeaponSourceProfileTicks *t)
{
    if (!t->image_ok) return 0;
    if (!sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type) &&
        motion->distance && !motion->flight) return 0;
    if (p->projectile_frame_count) return 1;
    return sc64_weapon_source_trace_duration_ticks(p, motion, t) != 0;
}

static inline int
sc64_weapon_source_trace_has_duration_motion(const Sc64WeaponSourceAttackProfile *p,
                                             int x0, int y0, int x1, int y1,
                                             const Sc64WeaponSourceMotion *motion)
{
    Sc64WeaponSourceProfileTicks t;
    if (!p) return 0;
    sc64_weapon_source_profile_ticks(p, &t);
    if (!t.image_ok) return 0;
    Sc64WeaponSourceMotion local;
    if (!motion) {
        local = sc64_weapon_source_motion_prepare(p, x0, y0, x1, y1);
        motion = &local;
    }
    return sc64_weapon_source_trace_has_duration_ticks(p, motion, &t);
}

static inline int
sc64_weapon_source_trace_has_duration(const Sc64WeaponSourceAttackProfile *p,
                                      int x0, int y0, int x1, int y1)
{
    return sc64_weapon_source_trace_has_duration_motion(p, x0, y0, x1, y1, 0);
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
