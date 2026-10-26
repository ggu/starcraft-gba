/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual game_update() regression for legacy worker healing versus imported
 * stock profiles. No input, resources, or world state is altered mid-update. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static int find_side_worker(int type)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == type)
            return i;
    return -1;
}

#ifdef SC64_IMPORTED_MISSION
static void test_imported_workers_do_not_gain_nonstock_hp(void)
{
    static const int types[] = { U_SCV, U_DRONE, U_PROBE };
    int slots[sizeof(types) / sizeof(types[0])];

    game_init();
    init_state();
    assert(screen == SCREEN_PLAY);
    for (unsigned n = 0; n < sizeof(types) / sizeof(types[0]); ++n) {
        slots[n] = spawn_unit(SIDE, types[n], 96 + (int)n * 16, 96);
        assert(slots[n] >= 0);
        u[slots[n]].raw_type = sc64_race_raw_unit(types[n]);
        assert(u[slots[n]].maxhp > 1);
        u[slots[n]].hp = u[slots[n]].maxhp - 1;
    }

    frame = 89;
    game_update(0, 0);
    assert(frame == 90);
    for (unsigned n = 0; n < sizeof(types) / sizeof(types[0]); ++n)
        assert(u[slots[n]].hp == u[slots[n]].maxhp - 1);
}
#else
static void test_legacy_scv_keeps_periodic_heal(void)
{
    int scv;

    game_init();
    init_state();
    scv = find_side_worker(U_SCV);
    assert(scv >= 0 && u[scv].maxhp > 1);
    u[scv].hp = u[scv].maxhp - 1;

    frame = 89;
    game_update(0, 0);
    assert(frame == 90);
    assert(u[scv].hp == u[scv].maxhp);
}
#endif

int main(void)
{
    gba_init();
#ifdef SC64_IMPORTED_MISSION
    test_imported_workers_do_not_gain_nonstock_hp();
    puts("imported SCV/Drone/Probe health remains unchanged at frame 90");
#else
    test_legacy_scv_keeps_periodic_heal();
    puts("legacy SCV periodic healing remains active at frame 90");
#endif
    return 0;
}
