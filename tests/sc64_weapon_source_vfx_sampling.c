/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_weapon_source_vfx.h"

#include <assert.h>
#include <stdint.h>

static void test_key_round_trip(void)
{
    uint32_t packed;
    Sc64WeaponSourceVfxKey got;
    for (unsigned weapon = 0; weapon <= SC64_SOURCE_VFX_MAX_WEAPON_ID; ++weapon)
        for (unsigned dir = 0; dir < 32; ++dir)
            for (unsigned air = 0; air < 2; ++air) {
                for (unsigned variant = 0; variant <= SC64_SOURCE_VFX_VARIANT_MAX; ++variant) {
                    Sc64WeaponSourceVfxKey in = {
                        (uint8_t)(weapon ^ (dir * 7u)),
                        (uint8_t)((weapon + dir) % (SC64_SOURCE_VFX_MAX_SLOT_PLUS1 + 1u)),
                        (uint8_t)weapon,
                        (uint8_t)dir,
                        (uint8_t)air,
                        (uint8_t)variant
                    };
                    assert(sc64_weapon_source_vfx_pack(in, &packed));
                    assert(sc64_weapon_source_vfx_is_tagged(packed));
                    assert(((packed >> 8) & 0xffu) == SC64_SOURCE_VFX_STYLE_BASE + weapon);
                    assert((packed & 31u) == dir);
                    assert(sc64_weapon_source_vfx_unpack(packed, &got));
                    assert(got.actor_raw == in.actor_raw);
                    assert(got.actor_slot_plus1 == in.actor_slot_plus1);
                    assert(got.weapon_id == in.weapon_id);
                    assert(got.direction == in.direction);
                    assert(got.target_air == in.target_air);
                    assert(got.variant == in.variant);
                }
            }
    Sc64WeaponSourceVfxKey bad = {1, 1, (uint8_t)(SC64_SOURCE_VFX_MAX_WEAPON_ID + 1u), 0, 0, 0};
    assert(!sc64_weapon_source_vfx_pack(bad, &packed));
    bad.weapon_id = 1;
    bad.variant = (uint8_t)(SC64_SOURCE_VFX_VARIANT_MAX + 1u);
    assert(!sc64_weapon_source_vfx_pack(bad, &packed));
    bad.variant = 0;
    bad.actor_slot_plus1 = (uint8_t)(SC64_SOURCE_VFX_MAX_SLOT_PLUS1 + 1u);
    assert(!sc64_weapon_source_vfx_pack(bad, &packed));
    assert(!sc64_weapon_source_vfx_is_tagged(UINT32_C(0x00000001)));
    assert(!sc64_weapon_source_vfx_unpack(UINT32_C(0x00000001), &got));
}

static void test_24_to_30_clock(void)
{
    for (uint32_t tick = 0; tick < 128; ++tick) {
        uint32_t age = sc64_weapon_source_vfx_first_gba_age(tick);
        assert(sc64_weapon_source_vfx_source_tick(age) >= tick);
        if (age) assert(sc64_weapon_source_vfx_source_tick(age - 1) < tick);
    }
    assert(sc64_weapon_source_vfx_source_tick(0) == 0);
    assert(sc64_weapon_source_vfx_source_tick(1) == 0);
    assert(sc64_weapon_source_vfx_source_tick(2) == 1);
    assert(sc64_weapon_source_vfx_source_tick(3) == 2);
    assert(sc64_weapon_source_vfx_first_gba_age(1) == 2);
    assert(sc64_weapon_source_vfx_first_gba_age(2) == 3);
    assert(sc64_weapon_source_vfx_first_gba_age(3) == 4);
    assert(sc64_weapon_source_vfx_gba_duration(3) == 4);
    assert(sc64_weapon_source_vfx_gba_duration(0) == 0);
}

static void test_frame_sampling(void)
{
    uint8_t frame = 0xff;
    assert(!sc64_weapon_source_vfx_sample_frames(2, 3, 1, 34, 5, &frame));
    assert(sc64_weapon_source_vfx_sample_frames(3, 3, 1, 34, 5, &frame) && frame == 34);
    assert(sc64_weapon_source_vfx_sample_frames(6, 3, 1, 34, 5, &frame) && frame == 37);
    assert(!sc64_weapon_source_vfx_sample_frames(8, 3, 1, 34, 5, &frame));
    assert(!sc64_weapon_source_vfx_sample_frames(3, 3, 0, 0, 5, &frame));
    assert(!sc64_weapon_source_vfx_sample_frames(5, 3, 1, 254, 3, &frame));

    {
        uint16_t directional;
        int flip;
        assert(sc64_weapon_source_vfx_direction_frame(34, 17, 15,
                                                       &directional, &flip));
        assert(directional == 49 && !flip);
        assert(sc64_weapon_source_vfx_direction_frame(34, 17, 17,
                                                       &directional, &flip));
        assert(directional == 49 && flip);
        assert(sc64_weapon_source_vfx_direction_frame(34, 17, 31,
                                                       &directional, &flip));
        assert(directional == 35 && flip);
        assert(!sc64_weapon_source_vfx_direction_frame(0, 16, 16,
                                                        &directional, &flip));
        assert(!sc64_weapon_source_vfx_direction_frame(65535, 17, 1,
                                                        &directional, &flip));
    }
}

static void test_projectile_preview(void)
{
    int x, y;
    assert(sc64_weapon_source_vfx_linear_sample(10, 20, 110, 220, 0, 10, &x, &y));
    assert(x == 10 && y == 20);
    assert(sc64_weapon_source_vfx_linear_sample(10, 20, 110, 220, 5, 10, &x, &y));
    assert(x == 60 && y == 120);
    assert(sc64_weapon_source_vfx_linear_sample(10, 20, 110, 220, 10, 10, &x, &y));
    assert(x == 110 && y == 220);
    assert(sc64_weapon_source_vfx_linear_sample(100, 100, 0, -20, 5, 10, &x, &y));
    assert(x == 50 && y == 40);
    assert(sc64_weapon_source_vfx_linear_sample(INT32_MIN, INT32_MAX,
                                                INT32_MAX, INT32_MIN, 50, 100, &x, &y));
    assert(x == -1 && y == 0);
    assert(!sc64_weapon_source_vfx_linear_sample(0, 0, 10, 10, 11, 10, &x, &y));
    assert(!sc64_weapon_source_vfx_linear_sample(0, 0, 10, 10, 1, 0, &x, &y));
    assert(sc64_weapon_source_vfx_projectile_position(
        SC64_SOURCE_BULLET_APPEAR_AT_TARGET, 10, 20, 110, 220, 0, 10, &x, &y));
    assert(x == 110 && y == 220);
    assert(sc64_weapon_source_vfx_projectile_position(
        SC64_SOURCE_BULLET_APPEAR_AT_TARGET, 10, 20, 110, 220, 5, 10, &x, &y));
    assert(x == 110 && y == 220);
    assert(sc64_weapon_source_vfx_projectile_position(
        0, 10, 20, 110, 220, 5, 10, &x, &y));
    assert(x == 60 && y == 120);
}

int main(void)
{
    test_key_round_trip();
    test_24_to_30_clock();
    test_frame_sampling();
    test_projectile_preview();
    return 0;
}
