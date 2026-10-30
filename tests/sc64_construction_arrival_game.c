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

/* Automatic building selection leaves existing construction intact. */
static void test_busy_builder_selection(int race)
{
    reset_fixture(race); add_prerequisites(race);
    int type=build_type(race), cost=building_cost_for(type);
    minerals=cost*3;
    int busy=add_worker(race,80,80), free_worker=add_worker(race,88,80);
    assert(busy>=0 && free_worker>=0);
    u[busy].selected=1;
    assert(race_start_build(SIDE,busy,type,240,240));
#ifdef SC64_FIELD_CONTROLS
    cx=280; cy=240;
    assert(field_ui_build_worker(type)==free_worker);
#endif
    assert(race_owner_worker(SIDE,worker_type(race),280,240)==free_worker);
    assert(!race_start_build(SIDE,busy,type,280,240));
    assert(u[busy].order==ORDER_BUILD_PENDING && u[busy].target==type &&
           u[busy].tx==240 && u[busy].ty==240);
    u[free_worker].selected=1;
#ifdef SC64_FIELD_CONTROLS
    assert(field_ui_build_worker(type)==free_worker);
#endif
    u[free_worker].alive=0;
    assert(race_owner_worker(SIDE,worker_type(race),280,240)<0);
#ifdef SC64_FIELD_CONTROLS
    assert(field_ui_build_worker(type)<0);
#endif
    u[busy].x=240; u[busy].y=240; update_orders();
    if(race==R_TER) {
        int foundation=u[busy].target;
        assert(u[busy].order==ORDER_BUILD && foundation>=0 && b[foundation].alive);
        assert(!race_start_build(SIDE,busy,type,280,240));
        assert(u[busy].order==ORDER_BUILD && u[busy].target==foundation);
#ifdef SC64_FIELD_CONTROLS
        assert(field_ui_build_worker(type)<0);
#endif
    } else if(race==R_PROTOSS) {
        assert(u[busy].order==ORDER_IDLE && build_worker_available(busy,SIDE,U_PROBE));
    } else assert(!u[busy].alive && !build_worker_available(busy,SIDE,U_DRONE));
}

static void test_builder_eligibility(void)
{
    reset_fixture(R_TER);
    int worker=spawn_unit(SIDE,U_SCV,80,80);
    int enemy=spawn_unit(ENEMY,U_SCV,96,80);
    int marine=spawn_unit(SIDE,U_MARINE,112,80);
    assert(worker>=0 && enemy>=0 && marine>=0);
    assert(build_worker_available(worker,SIDE,U_SCV));
    assert(!build_worker_available(-1,SIDE,U_SCV));
    assert(!build_worker_available(MAX_UNITS,SIDE,U_SCV));
    assert(!build_worker_available(enemy,SIDE,U_SCV));
    assert(!build_worker_available(marine,SIDE,-1));
    assert(!build_worker_available(worker,SIDE,U_PROBE));
#ifdef SC64_TRANSPORT_RUNTIME
    u[worker].order=ORDER_TRANSPORTED;
    assert(!build_worker_available(worker,SIDE,U_SCV));
    u[worker].order=ORDER_IDLE;
#endif
    u[worker].alive=0;
    assert(!build_worker_available(worker,SIDE,U_SCV));
}

#ifdef SC64_FIELD_CONTROLS
/* Issue the second order while the selected SCV is travelling or building. */
static void test_two_terran_builders(int wait_for_foundation)
{
    reset_fixture(R_TER); add_prerequisites(R_TER);
    int cost=building_cost_for(B_DEPOT);
    minerals=cost*2;
    int first=add_worker(R_TER,80,80), second=add_worker(R_TER,88,80);
    assert(first>=0 && second>=0);
    u[first].selected=1;
    cx=240; cy=240;
    assert(field_ui_build_worker(B_DEPOT)==first);
    assert(race_start_build(SIDE,first,B_DEPOT,cx,cy));
    if(wait_for_foundation) {
        for(int tick=0;tick<2000 && u[first].order==ORDER_BUILD_PENDING;tick++)
            update_orders();
        assert(u[first].order==ORDER_BUILD);
        assert(minerals==cost);
    }
    int old_order=u[first].order, old_target=u[first].target;
    int old_tx=u[first].tx, old_ty=u[first].ty, old_bank=minerals;
    assert(!race_start_build(SIDE,first,B_DEPOT,320,240));
    assert(u[first].order==old_order && u[first].target==old_target &&
           u[first].tx==old_tx && u[first].ty==old_ty && minerals==old_bank);
    cx=320; cy=240;
    assert(field_ui_build_worker(B_DEPOT)==second);
    assert(race_start_build(SIDE,second,B_DEPOT,cx,cy));
    assert(minerals==old_bank && u[first].order==old_order && u[first].target==old_target);
    for(int tick=0;tick<2000 && (u[first].order==ORDER_BUILD_PENDING ||
                                 u[second].order==ORDER_BUILD_PENDING);tick++)
        update_orders();
    assert(u[first].order==ORDER_BUILD && u[second].order==ORDER_BUILD);
    int a=u[first].target, bslot=u[second].target;
    assert(a>=0 && bslot>=0 && a!=bslot && b[a].alive && b[bslot].alive);
    assert(minerals==0);
    int progress_a=b[a].build_progress, progress_b=b[bslot].build_progress;
    update_orders();
    assert(b[a].build_progress>progress_a && b[bslot].build_progress>progress_b);
    for(int tick=0;tick<5000 && (b[a].under_construction || b[bslot].under_construction);tick++)
        update_orders();
    assert(!b[a].under_construction && !b[bslot].under_construction);
    assert(u[first].order==ORDER_IDLE && u[second].order==ORDER_IDLE);
}
#endif

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
        test_busy_builder_selection(races[i]);
    }
    test_builder_eligibility();
#ifdef SC64_FIELD_CONTROLS
    test_two_terran_builders(0);
    test_two_terran_builders(1);
#endif
    test_extractor_arrival_and_drone_interruption();
    test_ai_pending_dedup_and_budget();
    puts("construction arrival fixtures passed");
    return 0;
}
