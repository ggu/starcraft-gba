/* SPDX-License-Identifier: GPL-3.0-or-later */
#define SC64_WEAPON_SOURCE_BANK_HEADER "../tests/fixtures/weapon_source_mock.h"
#include "../starcraft/sc64_weapon_source_runtime.h"

#include <assert.h>

/* The original per-tick and 64-bit implementations, kept as the oracle for
 * the closed-form cruise phase and the 32-bit root. */
static uint32_t ref_sqroot(uint64_t n)
{
    uint64_t bit = UINT64_C(1) << 62, root = 0;
    while (bit > n) bit >>= 2;
    while (bit) {
        if (n >= root + bit) { n -= root + bit; root = (root >> 1) + bit; }
        else root >>= 1;
        bit >>= 2;
    }
    return root > UINT32_MAX ? UINT32_MAX : (uint32_t)root;
}

static uint32_t ref_flight(uint64_t remaining, int64_t top, int64_t accel, uint32_t limit)
{
    int64_t speed = 0;
    if (!remaining || top <= 0) return 0;
    if (accel <= 0) speed = top;
    for (uint32_t tick = 1; tick <= limit; ++tick) {
        if (accel > 0) { speed += accel; if (speed > top) speed = top; }
        if (speed <= 0) speed = top;
        if ((uint64_t)speed >= remaining) return tick;
        remaining -= (uint64_t)speed;
    }
    return limit;
}

static uint64_t ref_travelled(uint64_t distance, int64_t top, int64_t accel, uint32_t age)
{
    int64_t speed = 0;
    uint64_t travelled = 0;
    for (uint32_t tick = 0; tick < age; ++tick) {
        if (accel > 0) { speed += accel; if (speed > top) speed = top; }
        else speed = top;
        if (speed <= 0) speed = top;
        travelled += (uint64_t)speed;
        if (travelled >= distance) { travelled = distance; break; }
    }
    return travelled;
}

static void closed_forms_match_the_tick_loops(void)
{
    uint64_t rng = 0x9e3779b97f4a7c15u;
    for (int i = 0; i < 200000; ++i) {
        rng = rng * 6364136223846793005u + 1442695040888963407u;
        uint64_t n = i < 100000 ? rng >> (rng & 63) : (uint64_t)i * i;
        assert(sc64_weapon_source_trace_sqroot(n) == ref_sqroot(n));
    }
    static const uint64_t edges[] = { 0, 1, 2, 3, 4, UINT32_MAX - 1u, UINT32_MAX,
        (uint64_t)UINT32_MAX + 1u, UINT64_MAX };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i)
        assert(sc64_weapon_source_trace_sqroot(edges[i]) == ref_sqroot(edges[i]));

    static const int tops[] = { 1, 7, 160, 256, 853, 1280, 2560, 10240 };
    static const int accels[] = { -30, 0, 1, 17, 64, 256, 1000, 20000 };
    static const int coords[][2] = { {0, 1}, {3, 4}, {10, 0}, {37, -59}, {-120, 85}, {400, 300}, {2048, -1500} };
    static const uint16_t limits[] = { 0, 1, 4, 37, 255 };
    unsigned cases = 0;
    test_source_profile.weapon_bullet_type = 0;
    for (unsigned t = 0; t < sizeof(tops) / sizeof(tops[0]); ++t)
        for (unsigned a = 0; a < sizeof(accels) / sizeof(accels[0]); ++a)
            for (unsigned c = 0; c < sizeof(coords) / sizeof(coords[0]); ++c)
                for (unsigned l = 0; l < sizeof(limits) / sizeof(limits[0]); ++l) {
                    const Sc64WeaponSourceAttackProfile *p = &test_source_profile;
                    test_source_profile.speed_raw = tops[t];
                    test_source_profile.accel_raw = (int16_t)accels[a];
                    test_source_profile.projectile_lifetime = limits[l];
                    int x1 = coords[c][0], y1 = coords[c][1];
                    uint64_t distance = sc64_weapon_source_trace_distance_raw(0, 0, x1, y1);
                    uint32_t limit = limits[l] ? limits[l] : 65535u;
                    uint32_t flight = sc64_weapon_source_trace_flight_ticks(p, 0, 0, x1, y1);
                    for (int frames = 0; frames < 2; ++frames) {
                        test_source_profile.projectile_frame_count = (uint16_t)frames;
                        assert(sc64_weapon_source_trace_has_duration(p, 0, 0, x1, y1) ==
                               (sc64_weapon_source_trace_duration(p, 0, 0, x1, y1) != 0));
                    }
                    assert(flight == ref_flight(distance, tops[t], accels[a], limit));
                    if (!flight) continue;
                    for (uint32_t age = 0; age <= flight + 2u && age < 70000u; age += 1u + age / 64u) {
                        int x, y;
                        assert(sc64_weapon_source_trace_position(p, 0, 0, x1, y1, age, &x, &y));
                        int ex = x1, ey = y1;
                        if (age < flight) {
                            uint64_t tr = ref_travelled(distance, tops[t], accels[a], age);
                            ex = (int)((int64_t)x1 * (int64_t)tr / (int64_t)distance);
                            ey = (int)((int64_t)y1 * (int64_t)tr / (int64_t)distance);
                        }
                        assert(x == ex && y == ey);
                        ++cases;
                    }
                }
    assert(cases > 10000);
}

int main(void)
{
    closed_forms_match_the_tick_loops();
    test_source_profile = (Sc64WeaponSourceAttackProfile){
        .actor_raw = 6, .weapon_id = 11, .mode = 0, .variant = 0,
        .flags = SC64_SOURCE_PROFILE_HAS_PROJECTILE,
        .weapon_bullet_type = SC64_SOURCE_BULLET_APPEAR_AT_TARGET,
        .projectile_image = 531, .projectile_lifetime = 100,
        .projectile_frame_count = 10, .speed_raw = 256
    };
    test_source_image.image_id = 531;
    test_source_init = (Sc64WeaponSourceAnimation){
        .event_count = 1, .duration_ticks = 9, .animation_id = 0
    };
    test_source_death = (Sc64WeaponSourceAnimation){
        .duration_ticks = 3, .animation_id = 1
    };
    sc64_weapon_source_events[0] = (Sc64WeaponSourceEvent){
        .image_id = 531, .source_tick = 2, .pc = 100, .source_frame = 0xffff,
        .opcode = 36
    };

    uint32_t packed;
    Sc64WeaponSourceVfxKey key = {6, 1, 11, 8, 0, 0};
    assert(sc64_weapon_source_vfx_pack(key, &packed));
    const Sc64WeaponSourceAttackProfile *p = sc64_weapon_source_trace_profile(packed);
    assert(p == &test_source_profile);
    assert(sc64_weapon_source_trace_flight_ticks(p, 0, 0, 100, 0) == 0);
    assert(sc64_weapon_source_trace_duration(p, 0, 0, 100, 0) == 7);
    int x, y;
    assert(sc64_weapon_source_trace_position(p, 0, 0, 100, 0, 0, &x, &y));
    assert(x == 100 && y == 0);

    test_source_profile.weapon_bullet_type = 0;
    test_source_profile.projectile_lifetime = 100;
    assert(sc64_weapon_source_trace_flight_ticks(p, 0, 0, 10, 0) == 20);
    assert(sc64_weapon_source_trace_duration(p, 0, 0, 10, 0) == 32);
    assert(sc64_weapon_source_trace_position(p, 0, 0, 10, 0, 3, &x, &y));
    assert(x == 1 && y == 0);

    test_source_profile.speed_raw = 1024;
    test_source_profile.accel_raw = 256;
    assert(sc64_weapon_source_trace_flight_ticks(p, 0, 0, 10, 0) == 7);
    assert(sc64_weapon_source_trace_position(p, 0, 0, 10, 0, 3, &x, &y));
    assert(x == 3 && y == 0);

    test_source_profile.weapon_bullet_type = 5;
    assert(sc64_weapon_source_trace_position(p, 0, 0, 10, 0, 5, &x, &y));
    assert(x == 0 && y == 0);

    test_source_profile.weapon_bullet_type = 3;
    assert(sc64_weapon_source_trace_position(p, 0, 0, 10, 0, 1, &x, &y));
    assert(x == 10 && y == 0);
    test_source_profile.weapon_bullet_type = 7;
    assert(sc64_weapon_source_trace_flight_ticks(p, 0, 0, 10, 0) > 0);

    test_source_profile.projectile_lifetime = 4;
    test_source_profile.speed_raw = 256;
    test_source_profile.accel_raw = 0;
    assert(sc64_weapon_source_trace_flight_ticks(p, 0, 0, 10, 0) == 4);

    test_source_profile.weapon_bullet_type = 0;
    test_source_profile.speed_raw = 0;
    assert(sc64_weapon_source_trace_duration(p, 0, 0, 10, 0) == 0);
    assert(!sc64_weapon_source_trace_position(p, 0, 0, 10, 0, 1, &x, &y));

    key.weapon_id = 10;
    assert(sc64_weapon_source_vfx_pack(key, &packed));
    assert(!sc64_weapon_source_trace_profile(packed));
    assert(sc64_weapon_source_trace_duration(0, 0, 0, 0, 0) == 0);
    return 0;
}
