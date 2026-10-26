/* SPDX-License-Identifier: GPL-3.0-or-later */
#define SC64_WEAPON_SOURCE_BANK_HEADER "../tests/fixtures/weapon_source_mock.h"
#include "../starcraft/sc64_weapon_source_runtime.h"

#include <assert.h>

int main(void)
{
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
