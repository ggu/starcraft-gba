/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic shared-order fixtures; no native pacing or controller-win claim. */
#define main existing_worker_gather_main
#include "sc64_worker_gather_game.c"
#undef main

static void continuation_scene(int race, int worker_type, int depot_type)
{
    int w;
    reset_scene(race, R_TER);
    w = spawn_unit(SIDE, worker_type, 120, 120);
    assert(w >= 0);
    set_depot(0, SIDE, depot_type, 120, 120);
    m[2].x = 200; m[2].y = 200; m[2].amount = 0;
    m[3].x = 216; m[3].y = 200; m[3].amount = 24;
    u[w].order = ORDER_MINE; u[w].target = 2;
    /* Return the final cargo before selecting a replacement field. */
    u[w].carry = 5;
    one_tick();
    assert(minerals == 5 && u[w].carry == 0 && u[w].target == 2);
    one_tick();
    assert(u[w].order == ORDER_MINE && u[w].target == 3);
    assert(m[3].amount == 24 && minerals == 5);
    u[w].x = 216; u[w].y = 200;
    for (int tick = 0; tick < 80 && !u[w].carry; ++tick) one_tick();
    assert(u[w].carry == 8 && m[3].amount == 16);
    /* A deliberate Stop never restarts work. */
    u[w].selected = 1;
    stop_selected(); one_tick();
    assert(u[w].order == ORDER_IDLE && u[w].carry == 8);
}

static void unavailable_fields(void)
{
    int w;
    reset_scene(R_PROTOSS, R_TER);
    w = spawn_unit(SIDE, U_PROBE, 120, 120);
    set_depot(0, SIDE, B_NEXUS, 120, 120);
    m[2].x=200; m[2].y=200;
    m[3].x=216; m[3].y=200; m[3].amount=16;
    SC64_GRID_CLEAR(visible); SC64_GRID_CLEAR(explored);
    u[w].order=ORDER_MINE; u[w].target=2;
    one_tick();
    assert(u[w].order==ORDER_IDLE && u[w].target==2);
    /* Previously explored nearby minerals remain eligible in fog. */
    SC64_GRID_SET(explored, 216/16+(200/16)*MAP_COLS, 1);
    u[w].order=ORDER_MINE;
    one_tick(); assert(u[w].order==ORDER_MINE && u[w].target==3);
    /* An isolated cluster must not fall back to a distant field. */
    u[w].target=2; m[3].x=600; m[3].y=600;
    SC64_GRID_SET(visible, 600/16+(600/16)*MAP_COLS, 1);
    one_tick(); assert(u[w].order==ORDER_IDLE && u[w].target==2);
    /* A closer mineral behind a complete barrier cannot steal the job. */
    SC64_GRID_CLEAR(path_walkable);
    for (int y=0;y<MAP_ROWS;++y)
        for (int x=0;x<10;++x) SC64_GRID_SET(path_walkable,x+y*MAP_COLS,1);
    m[2].x=144; m[2].y=120;
    m[3].x=184; m[3].y=120;
    m[4].x=120; m[4].y=184; m[4].amount=16;
    SC64_GRID_SET(visible,184/16+(120/16)*MAP_COLS,1);
    SC64_GRID_SET(visible,120/16+(184/16)*MAP_COLS,1);
    u[w].order=ORDER_MINE; u[w].target=2;
#ifdef SC64_IMPORTED_MISSION
    sc64_routes_reset();
#endif
    one_tick(); assert(u[w].target==4 && u[w].order==ORDER_MINE);
}
/* A real selected Probe order is still pending when another Probe collects
 * the final cargo. Retarget through the same known/reachable-field policy. */
static void pending_trip_depletes(void)
{
    reset_scene(R_PROTOSS,R_TER);
    int traveler=spawn_unit(SIDE,U_PROBE,112,112);
    int finisher=spawn_unit(SIDE,U_PROBE,240,240);
    assert(traveler>=0 && finisher>=0);
    set_depot(0,SIDE,B_NEXUS,112,112);
    m[2].x=320; m[2].y=320; m[2].amount=8;
    m[3].x=336; m[3].y=320; m[3].amount=24;
    u[traveler].selected=1; tap_a(320,320);
    assert(u[traveler].order==ORDER_MINE && u[traveler].target==2 &&
           sc64_worker_gather_pending(&u[traveler]));
    u[finisher].x=320; u[finisher].y=320;
    u[finisher].order=ORDER_MINE; u[finisher].target=2; u[finisher].work_timer=1;
    one_tick();
    assert(m[2].amount==0 && u[finisher].carry==8 && u[traveler].target==2);
    one_tick();
    assert(u[traveler].order==ORDER_MINE && u[traveler].target==3 &&
           sc64_worker_gather_pending(&u[traveler]) && m[3].amount==24);
    /* Stop remains authoritative after the retarget. */
    stop_selected(); one_tick();
    assert(u[traveler].order==ORDER_IDLE && !sc64_worker_gather_pending(&u[traveler]));

    /* A hidden replacement must not be chosen, including while carrying gas. */
    reset_scene(R_PROTOSS,R_TER);
    traveler=spawn_unit(SIDE,U_PROBE,112,112); assert(traveler>=0);
    set_depot(0,SIDE,B_NEXUS,112,112);
    m[2].x=320; m[2].y=320; m[2].amount=8;
    m[3].x=336; m[3].y=320; m[3].amount=24;
    u[traveler].selected=1; u[traveler].carry=6;
    u[traveler].flags|=SC64_WORKER_CARGO_GAS;
    tap_a(320,320); assert(sc64_worker_gather_pending(&u[traveler]));
    m[2].amount=0; SC64_GRID_CLEAR(visible); SC64_GRID_CLEAR(explored);
    one_tick();
    assert(u[traveler].order==ORDER_IDLE && u[traveler].target==2 &&
           u[traveler].carry==6 && sc64_worker_cargo_is_gas(&u[traveler]));
}
int main(void)
{
    assert(existing_worker_gather_main()==0);
    continuation_scene(R_TER,U_SCV,B_COMMAND);
    continuation_scene(R_ZERG,U_DRONE,B_HATCHERY);
    continuation_scene(R_PROTOSS,U_PROBE,B_NEXUS);
    unavailable_fields();
    pending_trip_depletes();
    puts("mineral depletion continuation fixtures: PASS");
    return 0;
}
