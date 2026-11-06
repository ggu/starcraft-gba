/* SPDX-License-Identifier: GPL-3.0-or-later
 * Shared Protoss power admission against pinned source psi geometry. */
#define main sc64_skirmish_runtime_other_main
#include "sc64_skirmish_runtime.c"
#undef main

int main(void)
{
    int pylon, gateway, core, stargate, outside_gateway, before;
    gba_init();
    start_race(R_PROTOSS);
    for (int i=0; i<MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type==B_PYLON) b[i].alive=0;
    gateway=fixture_building_raw(SIDE,160,300,300);
    core=fixture_building_raw(SIDE,164,360,300);
    pylon=fixture_building_raw(SIDE,156,500,500);
    stargate=building_raw(167);
    assert(gateway>=0 && core>=0 && pylon>=0 && stargate>=0);
    assert(race_can_site(SIDE,stargate,500,579));
    assert(!race_can_site(SIDE,stargate,500,580));
    assert(race_site_powered(SIDE,627,500));
    assert(!race_site_powered(SIDE,628,500));
    assert(!race_site_powered(SIDE,596,564)); /* masked corner, inside old circle */
    assert(race_site_powered(SIDE,500,579));
    b[pylon].under_construction=1;
    assert(!race_site_powered(SIDE,500,500));
    b[pylon].under_construction=0;
    b[pylon].owner=ENEMY;
    assert(!race_site_powered(SIDE,500,500));
    assert(race_site_powered(ENEMY,500,500));
    b[pylon].owner=SIDE;
    outside_gateway=fixture_building_raw(SIDE,160,500,580);
    assert(outside_gateway>=0);
    before=minerals;
    assert(!race_queue_unit(SIDE,outside_gateway,U_ZEALOT));
    assert(minerals==before && !b[outside_gateway].queue_count);
    b[outside_gateway].y=579;
    assert(race_queue_unit(SIDE,outside_gateway,U_ZEALOT));
    puts("PASS shared Protoss psi boundaries, ownership, placement and production");
    return 0;
}
