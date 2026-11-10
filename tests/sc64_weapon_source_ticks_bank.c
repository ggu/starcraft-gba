/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Differential of the per-profile tick cache and the 32-bit flight path
 * against retained copies of the earlier implementations, over every profile
 * of an already-prepared source bank. Never regenerates private inputs. */
#define SC64_WEAPON_SOURCE_TICKS_CACHE 1
#include "sc64_weapon_source_runtime.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>

/* Earlier implementations, kept verbatim as the reference. */
static uint32_t ref_flight_distance(const Sc64WeaponSourceAttackProfile *p, uint64_t remaining)
{
    int64_t top = p->speed_raw;
    int64_t accel = p->accel_raw;
    uint32_t limit = p->projectile_lifetime ? p->projectile_lifetime : 65535u;
    int64_t speed = 0;
    if (!remaining || sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type))
        return 0;
    if (top <= 0) return 0;
    if (accel <= 0) speed = top;
    uint32_t tick = 1;
    for (; tick <= limit && accel > 0 && speed < top; ++tick) {
        speed += accel;
        if (speed > top) speed = top;
        if (speed <= 0) speed = top;
        if ((uint64_t)speed >= remaining) return tick;
        remaining -= (uint64_t)speed;
    }
    if (tick > limit) return limit;
    uint64_t arrival = (uint64_t)tick - 1u + (remaining + (uint64_t)top - 1u) / (uint64_t)top;
    return arrival > limit ? limit : (uint32_t)arrival;
}

static uint32_t ref_duration(const Sc64WeaponSourceAttackProfile *p, int x0, int y0, int x1, int y1)
{
    uint32_t flight, startup_ticks, death_ticks = 0;
    uint32_t source_ticks, actor_ticks = 0, release_ticks;
    const Sc64WeaponSourceAnimation *anim;
    if (!p || !p->projectile_image ||
        !sc64_weapon_source_image_lookup(p->projectile_image)) return 0;
    uint64_t distance = sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1);
    int stationary = sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type);
    flight = stationary ? 0 : ref_flight_distance(p, distance);
    if (!stationary && distance && !flight) return 0;
    startup_ticks = sc64_weapon_source_trace_startup_ticks(p);
    anim = sc64_weapon_source_animation_lookup(p->projectile_image, 1u);
    if (anim) death_ticks = sc64_weapon_source_animation_visual_ticks(anim);
    release_ticks = sc64_weapon_source_trace_release_ticks(p);
    source_ticks = release_ticks + startup_ticks + flight + death_ticks;
    if (!source_ticks) source_ticks = p->projectile_frame_count ? 1u : 0u;
    anim = sc64_weapon_source_animation_lookup(p->attack_image, p->attack_anim);
    if (anim && anim->loop_pc == UINT32_MAX)
        actor_ticks = sc64_weapon_source_animation_visual_ticks(anim);
    if (actor_ticks > source_ticks) source_ticks = actor_ticks;
    return sc64_weapon_source_vfx_gba_duration(source_ticks);
}

static int ref_has_duration(const Sc64WeaponSourceAttackProfile *p, int x0, int y0, int x1, int y1)
{
    if (!p || !p->projectile_image ||
        !sc64_weapon_source_image_lookup(p->projectile_image)) return 0;
    uint64_t distance = sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1);
    if (!sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type) && distance &&
        !ref_flight_distance(p, distance)) return 0;
    if (p->projectile_frame_count) return 1;
    return ref_duration(p, x0, y0, x1, y1) != 0;
}

static int ref_position(const Sc64WeaponSourceAttackProfile *p, int x0, int y0, int x1, int y1,
                        uint32_t source_age, int *x, int *y)
{
    if (!p || !x || !y) return 0;
    if (p->weapon_bullet_type == SC64_SOURCE_BULLET_APPEAR_SOURCE ||
        p->weapon_bullet_type == SC64_SOURCE_BULLET_SELF_DESTRUCT) { *x = x0; *y = y0; return 1; }
    if (p->weapon_bullet_type == SC64_SOURCE_BULLET_APPEAR_TARGET_UNIT ||
        p->weapon_bullet_type == SC64_SOURCE_BULLET_PERSIST_TARGET_POS ||
        p->weapon_bullet_type == SC64_SOURCE_BULLET_APPEAR_TARGET_POS) { *x = x1; *y = y1; return 1; }
    int64_t dx = (int64_t)x1 - x0, dy = (int64_t)y1 - y0;
    uint64_t distance = sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1);
    uint32_t flight = ref_flight_distance(p, distance);
    if (!flight) {
        if (!distance) { *x = x1; *y = y1; return 1; }
        return 0;
    }
    if (source_age >= flight || !distance) { *x = x1; *y = y1; return 1; }
    int64_t speed = 0, top = p->speed_raw;
    int64_t accel = p->accel_raw;
    uint64_t travelled = 0;
    uint32_t tick = 0;
    for (; tick < source_age && accel > 0 && speed < top; ++tick) {
        speed += accel;
        if (speed > top) speed = top;
        if (speed <= 0) speed = top;
        travelled += (uint64_t)speed;
        if (travelled >= distance) { travelled = distance; break; }
    }
    if (travelled < distance && tick < source_age) {
        uint64_t rest = source_age - tick, room = distance - travelled;
        travelled = rest >= (room + (uint64_t)top - 1u) / (uint64_t)top ?
                    distance : travelled + rest * (uint64_t)top;
    }
    *x = (int)((int64_t)x0 + dx * (int64_t)travelled / (int64_t)distance);
    *y = (int)((int64_t)y0 + dy * (int64_t)travelled / (int64_t)distance);
    return 1;
}

static uint32_t rng = 0x2468ace1u;
static uint32_t next(void) { rng = rng * 1664525u + 1013904223u; return rng; }

int main(void)
{
    static const int edges[] = {INT_MIN, -65536, -32768, -2048, -1, 0, 1, 2047, 2048, 32767, 65535, INT_MAX};
    unsigned long cases = 0, bank_hits = 0;
    for (unsigned pi = 0; pi < SC64_WEAPON_SOURCE_PROFILE_COUNT; ++pi) {
        const Sc64WeaponSourceAttackProfile *p = &sc64_weapon_source_profiles[pi];
        Sc64WeaponSourceAttackProfile copy = *p;
        assert(sc64_weapon_source_profile_bank_index(p) == (int)pi);
        assert(sc64_weapon_source_profile_bank_index(&copy) == -1);
        Sc64WeaponSourceProfileTicks direct, cached, again;
        sc64_weapon_source_profile_ticks_compute(p, &direct);
        sc64_weapon_source_profile_ticks(p, &cached);
        sc64_weapon_source_profile_ticks(p, &again);
        assert(direct.startup == cached.startup && direct.death == cached.death &&
               direct.actor == cached.actor && !direct.image_ok == !cached.image_ok);
        assert(cached.startup == again.startup && cached.death == again.death &&
               cached.actor == again.actor && !cached.image_ok == !again.image_ok);
        ++bank_hits;
        for (unsigned n = 0; n < 4000; ++n) {
            int a[4];
            for (int i = 0; i < 4; ++i)
                a[i] = n < 400 ? edges[next() % (sizeof edges / sizeof edges[0])]
                     : n < 2000 ? (int)(next() % 2200u) - 100
                     : (int)(next() % 65536u) - 32768;
            for (int use_copy = 0; use_copy < 2; ++use_copy) {
                const Sc64WeaponSourceAttackProfile *q = use_copy ? &copy : p;
                uint64_t distance = sc64_weapon_source_trace_distance_raw(a[0], a[1], a[2], a[3]);
                assert(sc64_weapon_source_trace_flight_ticks_distance(q, distance) ==
                       ref_flight_distance(q, distance));
                Sc64WeaponSourceMotion m = sc64_weapon_source_motion_prepare(q, a[0], a[1], a[2], a[3]);
                uint32_t want = ref_duration(q, a[0], a[1], a[2], a[3]);
                assert(sc64_weapon_source_trace_duration_motion(q, a[0], a[1], a[2], a[3], 0) == want);
                assert(sc64_weapon_source_trace_duration_motion(q, a[0], a[1], a[2], a[3], &m) == want);
                int has = ref_has_duration(q, a[0], a[1], a[2], a[3]);
                assert(!!sc64_weapon_source_trace_has_duration_motion(q, a[0], a[1], a[2], a[3], 0) == !!has);
                assert(!!sc64_weapon_source_trace_has_duration_motion(q, a[0], a[1], a[2], a[3], &m) == !!has);
                for (unsigned k = 0; k < 6; ++k) {
                    uint32_t age = k < 3 ? next() % 40u : k == 3 ? 0 : k == 4 ? 65535u : UINT32_MAX;
                    int rx = 7, ry = 9, ex = 7, ey = 9;
                    int ra = sc64_weapon_source_trace_position_motion(q, a[0], a[1], a[2], a[3], age, &rx, &ry, &m);
                    int ea = ref_position(q, a[0], a[1], a[2], a[3], age, &ex, &ey);
                    assert(ra == ea && rx == ex && ry == ey);
                }
                ++cases;
            }
        }
    }
    /* Synthetic speed/acceleration/limit combinations around the 32-bit bound. */
    Sc64WeaponSourceAttackProfile s = sc64_weapon_source_profiles[0];
    static const int32_t speeds[] = {1, 2, 255, 1024, 8533, 17067, 100000, 0x3fffffff, INT32_MAX};
    static const int16_t accels[] = {INT16_MIN, -1, 0, 1, 2, 850, 5000, INT16_MAX};
    static const uint16_t limits[] = {0, 1, 2, 7, 60, 255, 65535};
    static const uint64_t remains[] = {1, 2, 3, 1000, 65535, 76800, 0x7ffffffeull, 0x7fffffffull,
                                       0x80000000ull, 0x80000001ull, 0xffffffffull, 0x100000000ull,
                                       0x1ffffffffull, 0xffffffffffull};
    for (unsigned a = 0; a < sizeof speeds / sizeof speeds[0]; ++a)
    for (unsigned b = 0; b < sizeof accels / sizeof accels[0]; ++b)
    for (unsigned c = 0; c < sizeof limits / sizeof limits[0]; ++c) {
        s.speed_raw = speeds[a]; s.accel_raw = accels[b]; s.projectile_lifetime = limits[c];
        s.weapon_bullet_type = SC64_SOURCE_BULLET_FLY;
        for (unsigned r = 0; r < sizeof remains / sizeof remains[0]; ++r) {
            assert(sc64_weapon_source_trace_flight_ticks_distance(&s, remains[r]) ==
                   ref_flight_distance(&s, remains[r]));
            ++cases;
        }
        for (unsigned n = 0; n < 300; ++n) {
            uint64_t r = ((uint64_t)next() << (next() % 32u)) | (next() & 0xffu);
            assert(sc64_weapon_source_trace_flight_ticks_distance(&s, r) ==
                   ref_flight_distance(&s, r));
            ++cases;
        }
    }
    printf("source profile ticks: profiles=%lu cases=%lu\n", bank_hits, cases);
    return 0;
}
