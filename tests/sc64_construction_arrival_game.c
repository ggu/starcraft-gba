/* SPDX-License-Identifier: GPL-3.0-or-later
 * Construction intent and arrival lifecycle checks against the real engine. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#ifndef SC64_RACES
#error "Compile with the race engine enabled"
#endif

static int owner_building_count(int owner)
{
    int count=0;
    for(int i=0;i<MAX_BUILDINGS;i++) count+=b[i].alive&&b[i].owner==owner;
    return count;
}

static void reset_fixture(int race)
{
    game_init();
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    race_mode=1; race_human=race; race_bots=0;
    race_player[SIDE]=race; race_player[ENEMY]=R_TER;
    race_player[RESCUABLE]=R_ZERG; race_player[4]=R_PROTOSS;
    race_selected_mission=-1; screen=SCREEN_PLAY;
    race_skirmish_init();
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    race_mode=1; race_human=race; race_bots=0;
    race_player[SIDE]=race; race_player[ENEMY]=R_TER;
    race_player[RESCUABLE]=R_ZERG; race_player[4]=R_PROTOSS;
    race_selected_mission=-1; screen=SCREEN_PLAY;
    minerals=gas=0; supply_used=0; supply_cap=200;
    selected_building=-1;
}

static int worker_type(int race)
{ return race==R_ZERG?U_DRONE:race==R_PROTOSS?U_PROBE:U_SCV; }

static int build_type(int race)
{ return race==R_ZERG?B_SPAWNING_POOL:race==R_PROTOSS?B_PYLON:B_DEPOT; }

static int building_cost_for(int type)
{ return sc64_race_building_rule(type)->minerals; }

static int add_worker(int race, int x, int y)
{ return spawn_unit(SIDE,worker_type(race),x,y); }

static void add_prerequisites(int race)
{
    int type=race==R_ZERG?B_HATCHERY:race==R_PROTOSS?B_NEXUS:B_COMMAND;
    assert(spawn_build(SIDE,type,160,160,0)>=0);
}

static void test_intent_and_commit(int race)
{
    reset_fixture(race); add_prerequisites(race);
    int type=build_type(race), cost=building_cost_for(type);
    minerals=cost+17; gas=9;
    int worker=add_worker(race,80,80);
    assert(worker>=0);
    int before=owner_building_count(SIDE), bank=minerals;
    assert(race_start_build(SIDE,worker,type,240,240));
    assert(owner_building_count(SIDE)==before);
    assert(minerals==bank && gas==9);
    assert(u[worker].order==ORDER_BUILD_PENDING && u[worker].target==type);
    assert(!race_owner_has_type(SIDE,type));

    /* Exercise arrival through update_orders rather than calling the commit helper. */
    u[worker].x=240; u[worker].y=240;
    update_orders();
    assert(owner_building_count(SIDE)==before+1);
    int foundation=-1;
    for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive&&b[i].owner==SIDE&&b[i].type==type&&
        b[i].x==240&&b[i].y==240) foundation=i;
    assert(foundation>=0 && b[foundation].under_construction);
    assert(minerals==bank-cost && gas==9);
    if(race==R_ZERG) assert(!u[worker].alive && u[worker].order==ORDER_BUILD);
    else if(race==R_PROTOSS) assert(u[worker].alive && u[worker].order==ORDER_IDLE);
    else assert(u[worker].alive && u[worker].order==ORDER_BUILD);
}

static void cancel_selected_order(int worker, int order)
{
    for(int i=0;i<MAX_UNITS;i++) u[i].selected=0;
    u[worker].selected=1;
    issue_order(order,300,300);
}

static void test_cancel_before_arrival(int race)
{
    for(int kind=0;kind<3;kind++) {
        reset_fixture(race); add_prerequisites(race);
        int type=build_type(race),cost=building_cost_for(type);
        minerals=cost+5;
        int worker=add_worker(race,160,160), before=owner_building_count(SIDE);
        assert(worker>=0 && race_start_build(SIDE,worker,type,240,240));
        if(kind==0) cancel_selected_order(worker,ORDER_IDLE); /* STOP */
        else if(kind==1) cancel_selected_order(worker,ORDER_MOVE); /* MOVE */
        else u[worker].alive=0; /* death */
        update_orders();
        assert(owner_building_count(SIDE)==before && minerals==cost+5);
    }
}

static void test_arrival_revalidation(int race)
{
    int type=build_type(race), cost=building_cost_for(type), worker, before;
    /* Funds may be spent while the worker walks; failed commit is atomic. */
    reset_fixture(race); add_prerequisites(race); minerals=cost;
    worker=add_worker(race,80,80); assert(race_start_build(SIDE,worker,type,240,240));
    u[worker].x=240; u[worker].y=240;
    minerals=cost-1; before=owner_building_count(SIDE); update_orders();
    assert(owner_building_count(SIDE)==before && minerals==cost-1);
    assert(u[worker].order==ORDER_IDLE);

    /* Another actor entering the site blocks the commit. */
    reset_fixture(race); add_prerequisites(race); minerals=cost;
    worker=add_worker(race,80,80); assert(race_start_build(SIDE,worker,type,240,240));
    u[worker].x=240; u[worker].y=240;
    assert(spawn_unit(ENEMY,U_MARINE,240,240)>=0);
    before=owner_building_count(SIDE); update_orders();
    assert(owner_building_count(SIDE)==before && minerals==cost);

    /* Prerequisites are rechecked at arrival. */
    if(race==R_ZERG) {
        reset_fixture(race); add_prerequisites(race); minerals=cost;
        worker=add_worker(race,80,80); assert(race_start_build(SIDE,worker,type,240,240));
        u[worker].x=240; u[worker].y=240;
        memset(b,0,sizeof(b)); before=owner_building_count(SIDE); update_orders();
        assert(owner_building_count(SIDE)==before && minerals==cost);
    }
}

static void test_overlapping_intents_first_wins(int race)
{
    reset_fixture(race); add_prerequisites(race);
    int type=build_type(race),cost=building_cost_for(type);
    minerals=cost*2;
    int a=add_worker(race,80,80),c=add_worker(race,88,80);
    assert(race_start_build(SIDE,a,type,240,240));
    assert(race_start_build(SIDE,c,type,240,240));
    assert(owner_building_count(SIDE)==1); /* prerequisite only */
    u[a].x=240; u[a].y=240;
    update_orders();
    u[c].x=240; u[c].y=240;
    update_orders();
    assert(owner_building_count(SIDE)==2);
    assert(minerals==cost);
    assert(u[c].order==ORDER_IDLE || !u[c].alive);
}

static void test_actual_walk_to_arrival(int race)
{
    reset_fixture(race); add_prerequisites(race);
    int type=build_type(race),cost=building_cost_for(type);
    minerals=cost;
    int worker=add_worker(race,80,80),before=owner_building_count(SIDE);
    assert(race_start_build(SIDE,worker,type,240,240));
    assert(can_place(type,240,240)); /* intent is not a footprint or actor */
    for(int tick=0;tick<2000&&owner_building_count(SIDE)==before;tick++) update_orders();
    assert(owner_building_count(SIDE)==before+1);
    assert(minerals==0);
    assert(dist2(u[worker].x,u[worker].y,240,240)<=36*36 || race==R_ZERG);
}

static void test_building_slots_fail_atomically(int race)
{
    reset_fixture(race); add_prerequisites(race);
    int type=build_type(race), cost=building_cost_for(type);
    minerals=cost+3;
    int worker=add_worker(race,80,80);
    assert(worker>=0);
    for(int i=0;i<MAX_BUILDINGS;i++) if(!b[i].alive)
        assert(spawn_build(ENEMY,type,300+(i%4)*8,300+(i/4)*8,0)>=0);
    int before=owner_building_count(SIDE), bank=minerals;
    assert(!race_start_build(SIDE,worker,type,240,240));
    assert(owner_building_count(SIDE)==before && minerals==bank);

    /* Slots can also fill after the intent was admitted but before arrival. */
    reset_fixture(race); add_prerequisites(race); minerals=cost+3;
    worker=add_worker(race,80,80);
    assert(race_start_build(SIDE,worker,type,240,240));
    for(int i=0;i<MAX_BUILDINGS;i++) if(!b[i].alive)
        assert(spawn_build(ENEMY,type,300+(i%4)*8,300+(i/4)*8,0)>=0);
    before=owner_building_count(SIDE); bank=minerals;
    u[worker].x=240; u[worker].y=240;
    update_orders();
    assert(owner_building_count(SIDE)==before && minerals==bank);
    assert(u[worker].alive && u[worker].order==ORDER_IDLE);

}

static void test_extractor_arrival_and_drone_interruption(void)
{
    reset_fixture(R_ZERG); add_prerequisites(R_ZERG);
    assert(geyser_count>0);
    int x=geysers[0].x,y=geysers[0].y,cost=building_cost_for(B_EXTRACTOR);
    minerals=cost;
    int worker=add_worker(R_ZERG,80,80),before=owner_building_count(SIDE);
    assert(race_start_build(SIDE,worker,B_EXTRACTOR,x,y));
    assert(owner_building_count(SIDE)==before && race_owner_refinery(SIDE)<0);
    u[worker].alive=0; update_orders();
    assert(owner_building_count(SIDE)==before && minerals==cost);
    worker=add_worker(R_ZERG,80,80);
    assert(race_start_build(SIDE,worker,B_EXTRACTOR,x,y));
    u[worker].x=x; u[worker].y=y; update_orders();
    int extractor=-1;
    for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_EXTRACTOR) extractor=i;
    assert(extractor>=0 && b[extractor].build_progress==0 && !u[worker].alive);
    assert(minerals==0 && race_owner_refinery(SIDE)<0);
    for(int i=0;i<MAX_BUILDINGS*1000&&race_owner_refinery(SIDE)<0;i++) race_tick_buildings();
    assert(race_owner_refinery(SIDE)==extractor);
}

static void test_ai_pending_dedup_and_budget(void)
{
    reset_fixture(R_TER); race_bots=1;
    race_player[ENEMY]=R_TER;
    assert(spawn_build(ENEMY,B_COMMAND,64,64,0)>=0);
    assert(spawn_build(ENEMY,B_DEPOT,112,64,0)>=0);
    int w0=spawn_unit(ENEMY,U_SCV,200,200), w1=spawn_unit(ENEMY,U_SCV,208,200);
    assert(w0>=0 && w1>=0);
    race_minerals[ENEMY]=250; race_gas[ENEMY]=0;
    frame=60; race_tick_ai();
    int pending=0;
    for(int i=0;i<MAX_UNITS;i++) pending+=u[i].alive&&u[i].owner==ENEMY&&u[i].order==ORDER_BUILD_PENDING;
    assert(pending==1);
    int reserved_minerals=0,reserved_gas=0;
    race_pending_build_cost(ENEMY,&reserved_minerals,&reserved_gas);
    assert(reserved_minerals>0 && reserved_minerals<=race_minerals[ENEMY]);
    assert(race_minerals[ENEMY]==250);
    for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive&&b[i].owner==ENEMY&&b[i].type==B_COMMAND)
        assert(b[i].queue_count==0); /* pending structure cost is reserved */
    frame+=60; race_tick_ai();
    pending=0;
    for(int i=0;i<MAX_UNITS;i++) pending+=u[i].alive&&u[i].owner==ENEMY&&u[i].order==ORDER_BUILD_PENDING;
    assert(pending==1);
    for(int i=0;i<MAX_UNITS;i++) if(u[i].alive&&u[i].owner==ENEMY&&u[i].order==ORDER_BUILD_PENDING)
        u[i].alive=0;
    frame+=60; race_tick_ai();
    pending=0;
    for(int i=0;i<MAX_UNITS;i++) pending+=u[i].alive&&u[i].owner==ENEMY&&u[i].order==ORDER_BUILD_PENDING;
    assert(pending==1); /* cancelled plans can be retried */
    assert(race_minerals[ENEMY]==250);
}

int main(void)
{
    const int races[]={R_TER,R_ZERG,R_PROTOSS};
    for(unsigned i=0;i<sizeof(races)/sizeof(races[0]);i++) {
        test_intent_and_commit(races[i]);
        test_cancel_before_arrival(races[i]);
        test_arrival_revalidation(races[i]);
        test_overlapping_intents_first_wins(races[i]);
        test_actual_walk_to_arrival(races[i]);
        test_building_slots_fail_atomically(races[i]);
    }
    test_extractor_arrival_and_drone_interruption();
    test_ai_pending_dedup_and_budget();
    puts("construction arrival fixtures passed");
    return 0;
}
