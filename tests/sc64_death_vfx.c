/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free death-effect pool, timing, family and renderer bounds checks. */
#include "gba.h"
#include "../starcraft/sc64_death_vfx_render.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_age_and_timing(void)
{
    assert(sc64_death_vfx_age_su(0, 0) == 0);
    assert(sc64_death_vfx_age_su(0, 5) == 4);
    assert(sc64_death_vfx_age_su(0, 30) == 24);
    assert(sc64_death_vfx_age_su(10, 40) == 24);
    /* Wrapped frame counters remain monotonic and overflow-free. */
    assert(sc64_death_vfx_age_su(0xfffffffbu, 0) == 4);
    for (u32 f = 0; f < 16384u; ++f) assert(sc64_death_vfx_age_su(7, 7 + f) == f * 4u / 5u);
    assert(sc64_death_vfx_age_su(0, 16384u) == SC64_DEATH_VFX_AGE_SATURATED);
    assert(sc64_death_vfx_age_su(0, 0xffffffffu) == SC64_DEATH_VFX_AGE_SATURATED);
    for (int style = 1; style < SC64_DEATH_VFX_STYLE_COUNT; ++style) {
        Sc64DeathVfxTiming t = sc64_death_vfx_timing(style);
        assert(t.burst_su >= 24 && t.burst_su <= 42);
        assert(t.remnant_start <= t.total_su && t.burst_su <= t.total_su);
        assert(t.total_su <= 4 + SC64_DEATH_VFX_RUBBLE_SU);
    }
    assert(!sc64_death_vfx_timing(0).total_su);
    assert(!sc64_death_vfx_timing(SC64_DEATH_VFX_STYLE_COUNT).total_su);
}

static void test_families(void)
{
    assert(sc64_death_vfx_style_for_raw(0, 0) == SC64_DEATH_VFX_INFANTRY);      /* Marine */
    assert(sc64_death_vfx_style_for_raw(7, 0) == SC64_DEATH_VFX_EXPLOSION_SMALL); /* SCV */
    assert(sc64_death_vfx_style_for_raw(32, 0) == SC64_DEATH_VFX_EXPLOSION_SMALL); /* Firebat */
    assert(sc64_death_vfx_style_for_raw(5, 0) == SC64_DEATH_VFX_EXPLOSION_LARGE); /* Tank */
    assert(sc64_death_vfx_style_for_raw(37, 0) == SC64_DEATH_VFX_ZERG_SMALL);   /* Zergling */
    assert(sc64_death_vfx_style_for_raw(39, 0) == SC64_DEATH_VFX_ZERG_LARGE);   /* Ultralisk */
    assert(sc64_death_vfx_style_for_raw(43, 0) == SC64_DEATH_VFX_ZERG_AIR);     /* Mutalisk */
    assert(sc64_death_vfx_style_for_raw(65, 0) == SC64_DEATH_VFX_PROTOSS_SMALL); /* Zealot */
    assert(sc64_death_vfx_style_for_raw(83, 0) == SC64_DEATH_VFX_PROTOSS_LARGE); /* Reaver */
    assert(sc64_death_vfx_style_for_raw(106, 1) == SC64_DEATH_VFX_BUILDING_TERRAN);
    assert(sc64_death_vfx_style_for_raw(130, 1) == SC64_DEATH_VFX_BUILDING_TERRAN);
    assert(sc64_death_vfx_style_for_raw(131, 1) == SC64_DEATH_VFX_BUILDING_ZERG);
    assert(sc64_death_vfx_style_for_raw(154, 1) == SC64_DEATH_VFX_BUILDING_PROTOSS);
    assert(sc64_death_vfx_style_for_raw(-1, 0) == SC64_DEATH_VFX_EXPLOSION_SMALL);
    assert(sc64_death_vfx_style_for_raw(-1, 1) == SC64_DEATH_VFX_BUILDING_TERRAN);
}

static void test_pool(void)
{
    Sc64DeathVfx pool[4];
    Sc64DeathVfxTiming unit = sc64_death_vfx_timing(SC64_DEATH_VFX_EXPLOSION_SMALL);
    Sc64DeathVfxTiming rubble = sc64_death_vfx_timing(SC64_DEATH_VFX_BUILDING_TERRAN);
    memset(pool, 0, sizeof(pool));
    assert(sc64_death_vfx_spawn(pool, 4, 0, 0, 7, 0, 0, 5, &unit, 0, 0) < 0);
    assert(sc64_death_vfx_spawn(pool, 4, 0, SC64_DEATH_VFX_STYLE_COUNT, 7, 0, 0, 5, &unit, 0, 0) < 0);
    assert(sc64_death_vfx_spawn(0, 4, 0, 1, 7, 0, 0, 5, &unit, 0, 0) < 0);
    int slot = sc64_death_vfx_spawn(pool, 4, 100, SC64_DEATH_VFX_EXPLOSION_SMALL, 7,
                                    -70000, 70000, 200, &unit, 0, 3);
    assert(slot == 0 && pool[0].x == -32768 && pool[0].y == 32767);
    assert(pool[0].extent == SC64_DEATH_VFX_MAX_EXTENT && pool[0].color == 3);
    assert(sc64_death_vfx_active(pool, 4, 100) == 1);
    /* 27 source updates is 34 frames: alive at 33, expired at 34. */
    assert(!sc64_death_vfx_expired(&pool[0], 133));
    assert(sc64_death_vfx_expired(&pool[0], 134));
    /* Expired rows are reused first. */
    assert(sc64_death_vfx_spawn(pool, 4, 134, SC64_DEATH_VFX_BUILDING_TERRAN, 106,
                                10, 10, 20, &rubble, 0, 0) == 0);
    for (int i = 1; i < 4; ++i)
        assert(sc64_death_vfx_spawn(pool, 4, 140 + (u32)i, SC64_DEATH_VFX_BUILDING_TERRAN,
                                    106, i, i, 20, &rubble, 0, 0) == i);
    /* All rows full of bursting buildings: oldest burst is replaced. */
    assert(sc64_death_vfx_spawn(pool, 4, 150, SC64_DEATH_VFX_EXPLOSION_SMALL, 7,
                                99, 99, 5, &unit, 0, 0) == 0);
    /* The short unit expires first; refill it, then rubble whose burst has
     * finished yields (oldest first) before the still-bursting unit. */
    u32 later = 200;
    assert(sc64_death_vfx_slot(pool, 4, later) == 0);
    assert(sc64_death_vfx_spawn(pool, 4, later, SC64_DEATH_VFX_EXPLOSION_SMALL, 7,
                                5, 5, 5, &unit, 0, 0) == 0);
    assert(sc64_death_vfx_slot(pool, 4, later) == 1);
    assert(sc64_death_vfx_active(pool, 4, later) == 4);
}

static int screen_changed(int background, int *outside)
{
    int changed = 0;
    *outside = 0;
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x) {
        int p = gfx_test_pixel(x, y);
        if (y < SC64_DEATH_VFX_VIEW_TOP || y > SC64_DEATH_VFX_VIEW_BOTTOM) *outside += p != background;
        else changed += p != background;
    }
    return changed;
}

static void test_render(void)
{
    static const int positions[][2] = { {120, 66}, {0, 12}, {239, 119}, {-30, 140}, {300, -20} };
    for (int style = 1; style < SC64_DEATH_VFX_STYLE_COUNT; ++style) {
        Sc64DeathVfxTiming t = sc64_death_vfx_timing(style);
        Sc64DeathVfx e;
        memset(&e, 0, sizeof(e));
        sc64_death_vfx_spawn(&e, 1, 0, style, 1, 120, 66, style >= SC64_DEATH_VFX_BUILDING_TERRAN ?
                             SC64_DEATH_VFX_MAX_EXTENT : 8, &t, 0, C_GREEN);
        for (unsigned p = 0; p < sizeof(positions) / sizeof(*positions); ++p) {
            for (u32 age = 0; age < (u32)t.total_su + 2; age += age < 60 ? 1 : 97) {
                int outside;
                gfx_begin(C_PANEL);
                sc64_death_vfx_draw_remnant(&e, positions[p][0], positions[p][1], age);
                sc64_death_vfx_draw_burst(&e, positions[p][0], positions[p][1], age);
                int changed = screen_changed(C_PANEL, &outside);
                assert(!outside);
                if (p == 0) {
                    int burst = age < t.burst_su;
                    int remnant = age >= t.remnant_start && age < t.total_su &&
                                  t.remnant_start < t.total_su;
                    if (burst || remnant) assert(changed > 0);
                    if (age >= t.total_su) assert(!changed);
                }
            }
        }
    }
}

int main(void)
{
    gba_init();
    test_age_and_timing();
    test_families();
    test_pool();
    test_render();
    puts("PASS death VFX timing, source families, pool eviction and clipped authored rendering");
    return 0;
}
