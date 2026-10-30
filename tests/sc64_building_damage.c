/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_building_damage.h"

int main(void)
{
    unsigned hp, i, count;
    /* Displayed HP rounds up and percentage rounds down; equality at 33%.
       34/100 does not burn, even one Q8 unit below 34 still displays 34. */
    assert(sc64_building_burn_threshold(33u * 256u, 100));
    assert(!sc64_building_burn_threshold(33u * 256u + 1u, 100));
    assert(sc64_building_burn_threshold(50u * 256u, 150));
    assert(!sc64_building_burn_threshold(51u * 256u, 150));
    assert(!sc64_building_burn_threshold(0, 100));
    assert(!sc64_building_burn_threshold(1, 0));

    hp = 300u * 256u;
    for (i = 0; i < 3000; ++i)
        hp = sc64_building_burn_tick(hp, 1000, 1, 1, 1, 0, i);
    assert(hp == 300u * 256u - 2400u * 20u);
    /* Phase is derived from saved global frame, no building-local timer. */
    count = 0;
    for (i = 1234; i < 6234; ++i) count += sc64_building_burn_source_frame(i);
    assert(count == 4000);
    hp = 10u * 256u;
    assert(sc64_building_burn_tick(hp, 100, 0, 1, 1, 0, 1) == hp);
    assert(sc64_building_burn_tick(hp, 100, 1, 0, 1, 0, 1) == hp);
    assert(sc64_building_burn_tick(hp, 100, 1, 1, 0, 0, 1) == hp);
    assert(sc64_building_burn_tick(hp, 100, 1, 1, 1, 1, 1) == hp - 20);
    assert(sc64_building_burn_tick(hp, 100, 1, 1, 1, 0, 0) == hp);
    /* Repair above the displayed threshold immediately stops burning. */
    hp = 33u * 256u;
    assert(sc64_building_burn_tick(hp, 100, 1, 1, 1, 0, 1) == hp - 20);
    hp += 1;
    assert(sc64_building_burn_tick(hp, 100, 1, 1, 1, 0, 2) == hp);
    assert(sc64_building_burn_tick(20, 100, 1, 1, 1, 0, 1) == 0);
    assert(sc64_building_burn_tick(1, 100, 1, 1, 1, 0, 1) == 0);
    assert(sc64_building_burn_tick(0, 100, 1, 1, 1, 0, 1) == 0);
    /* Packed state survives save/load; owner 0 is valid and not a sentinel. */
    {
        unsigned owner, saved, whole, packed, continued, resumed, f;
        for (owner = 0; owner <= SC64_BUILDING_BURN_OWNER_MAX; ++owner) {
            packed = sc64_building_burn_with_owner(255u, owner);
            assert(packed <= SC64_BUILDING_BURN_PACKED_MAX);
            assert(sc64_building_burn_owner(packed) == owner);
            assert(sc64_building_burn_hp_q8(1, packed) == 1);
            sc64_building_burn_store_hp(0, &whole, &packed);
            assert(whole == 0 && (packed & 255u) == 0);
            assert(sc64_building_burn_owner(packed) == owner);
        }
        packed = sc64_building_burn_with_owner(0, 4);
        sc64_building_burn_store_hp(10u * 256u - 20u, &whole, &packed);
        assert(whole == 10 && (packed & 255u) == 20);
        assert(sc64_building_burn_hp_q8(whole, packed) == 10u * 256u - 20u);
        sc64_building_burn_full_repair(99, 100, &packed);
        assert((packed & 255u) == 20);
        sc64_building_burn_full_repair(100, 100, &packed);
        assert(packed == 4u * 256u);
        packed = sc64_building_burn_with_owner(0, 3);
        whole = 30;
        for (f = 1; f < 129; ++f) {
            hp = sc64_building_burn_tick(sc64_building_burn_hp_q8(whole, packed),
                                         100, 1, 1, 1, 0, f);
            sc64_building_burn_store_hp(hp, &whole, &packed);
        }
        saved = packed;
        resumed = whole;
        continued = sc64_building_burn_hp_q8(whole, packed);
        for (f = 129; f < 400; ++f) {
            continued = sc64_building_burn_tick(continued, 100, 1, 1, 1, 0, f);
            hp = sc64_building_burn_tick(sc64_building_burn_hp_q8(resumed, saved),
                                         100, 1, 1, 1, 0, f);
            sc64_building_burn_store_hp(hp, &resumed, &saved);
        }
        assert(sc64_building_burn_hp_q8(resumed, saved) == continued);
        assert(sc64_building_burn_owner(saved) == 3);
    }
    puts("building burn rules: ok");
    return 0;
}
