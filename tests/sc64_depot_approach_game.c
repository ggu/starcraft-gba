/* SPDX-License-Identifier: GPL-3.0-or-later
 * Asset-free default-profile worker return, with a long detour and a real
 * save/load boundary. The setup is synthetic; all movement uses update_orders. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_IMPORTED_MISSION
static void check_return(int order)
{
    game_init();
    init_state();
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b)); memset(m, 0, sizeof(m));
    won=lost=0; minerals=gas=0; supply_used=0; supply_cap=20; train_type=U_MARINE;
    /* The default map's row-5/6 wall covers columns 8..11 / 8..10.
     * A north-to-south trip must go around an end of that wall. */
    int home=spawn_build(SIDE,B_COMMAND,160,144,0);
    int refinery=spawn_build(SIDE,B_REFINERY,40,120,0);
    int worker=spawn_unit(SIDE,U_SCV,160,32);
    assert(home>=0 && refinery>=0 && worker>=0);
    m[0].x=160; m[0].y=32; m[0].amount=1000;
    u[worker].order=order; u[worker].target=order==ORDER_MINE?0:refinery;
    u[worker].carry=8; u[worker].path_timer=0; u[worker].path_retry=0;
    invalidate_path_grid();
    assert(!direct_path_clear(160,32,160,112)); /* 32px from the depot */
    unsigned ticks=0,refreshes=0; int went_around=0;
    while (ticks<70 && u[worker].carry) {
        int prev=u[worker].path_timer;
        update_orders(); ++ticks;
        if (!prev && u[worker].path_timer>0) ++refreshes;
        if (u[worker].x<128 || u[worker].x>=192) went_around=1;
    }
    assert(ticks==70 && u[worker].carry==8 && refreshes>=2);
    while(ticks<140 && u[worker].carry &&
          (u[worker].path_timer==0 ||
           (u[worker].path_x==u[worker].x && u[worker].path_y==u[worker].y))) {
        update_orders(); ++ticks;
        if (u[worker].x<128 || u[worker].x>=192) went_around=1;
    }
    assert(u[worker].path_timer>0 &&
           (u[worker].path_x!=u[worker].x || u[worker].path_y!=u[worker].y));
    /* Actual default codec: a restored committed waypoint must take the same
     * next simulation step as the uninterrupted worker. */
    assert(save_game());
    update_orders(); Unit expected=u[worker]; int expected_minerals=minerals, expected_gas=gas;
    assert(load_game());
    update_orders();
    assert(!memcmp(&u[worker],&expected,sizeof(expected)));
    assert(minerals==expected_minerals && gas==expected_gas);
    while(ticks++<1000 && u[worker].carry) {
        update_orders();
        if (u[worker].x<128 || u[worker].x>=192) went_around=1;
    }
    assert(u[worker].alive && u[worker].carry==0);
    assert(went_around);
    assert(u[worker].order==order);
    assert(order==ORDER_MINE ? minerals==8 && gas==0 : minerals==0 && gas==8);
}

static void check_blocked_start_is_not_goal(void)
{
    game_init(); init_state(); memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    int home=spawn_build(SIDE,B_COMMAND,240,168,0);
    assert(home>=0);
    assert(spawn_build(ENEMY,B_BARRACKS,216,136,0)>=0);
    assert(spawn_build(ENEMY,B_BARRACKS,200,136,0)>=0);
    assert(spawn_build(ENEMY,B_BARRACKS,232,136,0)>=0);
    assert(spawn_build(ENEMY,B_BARRACKS,216,120,0)>=0);
    assert(spawn_build(ENEMY,B_BARRACKS,216,152,0)>=0);
    invalidate_path_grid();
    int x=208,y=128,nx,ny,px,py;
    assert(dist2(x,y,b[home].x,b[home].y)>32*32);
    assert(sc64_depot_cell_point(b[home].x,b[home].y,x/16,y/16,&px,&py));
    assert(!is_traversable(x/16,y/16,-1));
    assert(!worker_depot_next(x,y,home,&nx,&ny));
}
#endif
int main(void)
{
#ifndef SC64_IMPORTED_MISSION
    check_return(ORDER_MINE);
    check_return(ORDER_GAS);
    check_blocked_start_is_not_goal();
#endif
    puts("default worker depot-circle return passed");
    return 0;
}
