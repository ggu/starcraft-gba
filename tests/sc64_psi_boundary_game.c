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
    int progress=b[outside_gateway].queue_progress;
    b[pylon].alive=0;
    race_tick_buildings();
    assert(b[outside_gateway].queue_count==1 && b[outside_gateway].queue_progress==progress);
    b[pylon].alive=1;
    race_tick_buildings();
    assert(b[outside_gateway].queue_progress==progress+1);

    /* Find a real legal site, then use the same request/arrival path as a
     * Probe. Moving the only Pylon outside psi must neither debit nor build. */
    int worker=find_worker_raw(SIDE,64), sx=-1, sy=-1;
    assert(worker>=0);
    for (int y=80; y<WORLD_H-80 && sx<0; y+=8)
        for (int x=80; x<WORLD_W-80; x+=8) {
            int tx=x,ty=y;
            if (race_build_request_valid(SIDE,worker,stargate,&tx,&ty,0)) {
                sx=tx; sy=ty; break;
            }
        }
    assert(sx>=0);
    int px=b[pylon].x, py=b[pylon].y, old_minerals=minerals, old_gas=gas;
    b[pylon].x=sx; b[pylon].y=sy-80;
    assert(!race_start_build(SIDE,worker,stargate,sx,sy));
    assert(minerals==old_minerals && gas==old_gas);
    b[pylon].x=px; b[pylon].y=py;
    assert(race_start_build(SIDE,worker,stargate,sx,sy));
    assert(minerals==old_minerals && gas==old_gas);
    u[worker].x=sx; u[worker].y=sy;
    b[pylon].alive=0;
    race_pending_build_tick(worker);
    assert(u[worker].order==ORDER_IDLE && minerals==old_minerals && gas==old_gas);
    b[pylon].alive=1;

    /* A Cannon must not fire while its last Pylon is absent. */
    int cannon=fixture_building_raw(SIDE,162,500,500);
    int enemy=spawn_unit(ENEMY,U_MARINE,510,500);
    assert(cannon>=0 && enemy>=0 && owners_hostile(SIDE,ENEMY));
    SC64_GRID_SET(visible, (510/16)+(500/16)*MAP_COLS, 1);
    b[cannon].queue_progress=0;
    b[pylon].alive=0;
    int enemy_hp=u[enemy].hp, enemy_shield=u[enemy].shield;
    race_tick_defenses();
    assert(!b[cannon].queue_progress && u[enemy].hp==enemy_hp && u[enemy].shield==enemy_shield);
    b[pylon].alive=1;
    race_tick_defenses();
    assert(b[cannon].queue_progress>0);
    puts("PASS shared Protoss psi boundaries, ownership, placement and production");
    return 0;
}
