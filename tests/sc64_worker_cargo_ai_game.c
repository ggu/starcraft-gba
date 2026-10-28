/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Focused AI resumption checks for explicit worker cargo. Authored scenes are
 * engine behavior fixtures, not normal-controller or campaign evidence. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#ifndef SC64_WORKER_CARGO_KIND
#error "Enable explicit worker cargo for this fixture"
#endif

static int add_refinery(int owner, int type, int x, int y)
{
    int slot=spawn_build(owner,type,x,y,0);
    assert(slot>=0 && building_complete(slot));
    return slot;
}

static void idle_cargo(Unit *q, int kind)
{
    q->order=ORDER_IDLE; q->target=-1; q->carry=6; q->flags&=~SC64_WORKER_CARGO_MASK;
    q->flags|=kind; q->work_timer=0; q->path_timer=4; q->path_retry=3;
}

static void use_flat_authored_routes(void)
{
    /* Authored actor scenes have no imported terrain. Reset the route cache,
     * then provide a bounded open grid so nearest-home checks use real routes. */
    invalidate_path_grid();
    memset(path_walkable, 0xff, sizeof(path_walkable));
    path_grid_valid = 1;
}

int main(void)
{
    const int races[3]={R_TER,R_PROTOSS,R_ZERG};
    const int workers[3]={U_SCV,U_PROBE,U_DRONE};
    const int refineries[3]={B_REFINERY,B_ASSIMILATOR,B_EXTRACTOR};
    const int depots[3]={B_COMMAND,B_NEXUS,B_HATCHERY};
    for(int r=0;r<3;r++) {
        game_init(); race_mode=1; race_human=R_ZERG; race_bots=1; init_state();
        memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); memset(m,0,sizeof(m));
        use_flat_authored_routes();
        race_player[ENEMY]=races[r];
        int home=add_refinery(ENEMY,depots[r],200,200); (void)home;
        if(r==0) { add_refinery(ENEMY,B_BARRACKS,230,200); add_refinery(ENEMY,B_DEPOT,250,200); }
        if(r==1) { add_refinery(ENEMY,B_PYLON,230,200); add_refinery(ENEMY,B_GATEWAY,250,200); add_refinery(ENEMY,B_CYBERNETICS_CORE,270,200); }
        if(r==2) { add_refinery(ENEMY,B_SPAWNING_POOL,230,200); add_refinery(ENEMY,B_HYDRALISK_DEN,250,200); }
        int ref=add_refinery(ENEMY,refineries[r],290,200);
        for(int j=0;j<2;j++) {
            int g=spawn_unit(ENEMY,workers[r],270+j*8,230); assert(g>=0);
            u[g].order=ORDER_GAS; u[g].target=ref;
        }
        int patch=0; m[patch].x=400; m[patch].y=400; m[patch].amount=30;
        int worker=spawn_unit(ENEMY,workers[r],240,240); assert(worker>=0);
        idle_cargo(&u[worker],0);
        frame=60; race_tick_ai();
        assert(u[worker].order==ORDER_MINE && u[worker].target==patch);
        assert(u[worker].carry==6 && !(u[worker].flags&SC64_WORKER_CARGO_KIND_MASK));
        assert(sc64_worker_gather_pending(&u[worker]));
        assert(u[worker].path_timer==0 && u[worker].path_retry==0);
        int ore_before=race_owner_minerals(ENEMY), gas_before=race_owner_gas(ENEMY);
        u[worker].x=m[patch].x; u[worker].y=m[patch].y; use_flat_authored_routes(); update_orders();
        assert(!sc64_worker_gather_pending(&u[worker]));
        u[worker].x=b[home].x+32; u[worker].y=b[home].y; use_flat_authored_routes(); update_orders();
        assert(race_owner_minerals(ENEMY)==ore_before+6 && race_owner_gas(ENEMY)==gas_before);

        int gas_worker=spawn_unit(ENEMY,workers[r],260,240); assert(gas_worker>=0);
        idle_cargo(&u[gas_worker],SC64_WORKER_CARGO_GAS);
        race_tick_ai();
        assert(u[gas_worker].order==ORDER_GAS && u[gas_worker].target==ref);
        assert(u[gas_worker].carry==6 && sc64_worker_cargo_is_gas(&u[gas_worker]));
        ore_before=race_owner_minerals(ENEMY); gas_before=race_owner_gas(ENEMY);
        /* The resumed order first reaches the refinery, then returns home. */
        int depot=-1;
        for(int j=0;j<MAX_BUILDINGS;j++) if(building_complete(j)&&b[j].owner==ENEMY&&
            (b[j].type==B_HATCHERY||b[j].type==B_NEXUS||b[j].type==B_COMMAND)){depot=j;break;}
        assert(depot>=0);
        u[gas_worker].x=b[ref].x; u[gas_worker].y=b[ref].y;
        update_orders();
        assert(!sc64_worker_gather_pending(&u[gas_worker]));
        u[gas_worker].x=b[depot].x; u[gas_worker].y=b[depot].y;
        update_orders();
        assert(race_owner_gas(ENEMY)==gas_before+6 && race_owner_minerals(ENEMY)==ore_before);
    }
    /* Gas cargo remains held without a refinery and resumes once one exists. */
    game_init(); race_mode=1; race_human=R_ZERG; race_bots=1; init_state();
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); memset(m,0,sizeof(m));
    use_flat_authored_routes();
    race_player[ENEMY]=R_TER;
    add_refinery(ENEMY,B_COMMAND,200,200);
    int gas_worker=spawn_unit(ENEMY,U_SCV,240,240); assert(gas_worker>=0);
    idle_cargo(&u[gas_worker],SC64_WORKER_CARGO_GAS);
    frame=60; race_tick_ai();
    assert(u[gas_worker].order==ORDER_IDLE && u[gas_worker].carry==6);
    int ref=add_refinery(ENEMY,B_REFINERY,440,440);
    race_tick_ai();
    assert(u[gas_worker].order==ORDER_GAS && u[gas_worker].target==ref);
    int ore_before=race_owner_minerals(ENEMY), gas_before=race_owner_gas(ENEMY);
    u[gas_worker].x=b[ref].x+24; u[gas_worker].y=b[ref].y; use_flat_authored_routes(); update_orders();
    int home=-1;
    for(int j=0;j<MAX_BUILDINGS;j++) if(building_complete(j)&&b[j].owner==ENEMY&&b[j].type==B_COMMAND){home=j;break;}
    assert(home>=0);
    u[gas_worker].x=b[home].x+32; u[gas_worker].y=b[home].y; use_flat_authored_routes(); update_orders();
    assert(race_owner_gas(ENEMY)==gas_before+6 && race_owner_minerals(ENEMY)==ore_before);
    /* Human Stop remains authoritative even with cargo. */
    game_init(); race_mode=1; race_human=R_ZERG; race_bots=1; init_state();
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); memset(m,0,sizeof(m));
    use_flat_authored_routes();
    race_player[ENEMY]=R_TER;
    add_refinery(ENEMY,B_COMMAND,200,200);
    int patch=0; m[patch].x=400; m[patch].y=400; m[patch].amount=20;
    int human=spawn_unit(SIDE,U_DRONE,240,240); assert(human>=0); idle_cargo(&u[human],0);
    int bot=spawn_unit(ENEMY,U_SCV,240,240); assert(bot>=0); idle_cargo(&u[bot],0);
    frame=60; race_tick_ai();
    assert(u[human].order==ORDER_IDLE && u[human].carry==6);
    assert(u[bot].order==ORDER_MINE);
    puts("Explicit cargo skirmish AI resumption fixtures passed.");
    return 0;
}
