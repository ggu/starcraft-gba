/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only scheduler obligations and explicit create-capacity deferral. */
#define main sc64_z9_existing_fixture_main
#include "sc64_z9_game.c"
#undef main
static int capacity_result;
static int capacity_action(void *ctx,const Sc64Z8Action *a,unsigned owner,const Sc64Z8Rect *r)
{ (void)ctx;(void)a;(void)owner;(void)r;return capacity_result; }
int main(void)
{
    Sc64Z8Action actions[3]={{0}};Sc64Z8Trigger trigger={0};Sc64Z8Program program={0};
    Sc64Z8SchedulerState sched={0};Sc64Z8Instance instance={0};Sc64Z8WorldOps ops={0};u8 outcome[8]={0};
    actions[0].opcode=11;actions[0].location=1;actions[0].group2=0;actions[0].index=1;
    actions[1]=actions[0];actions[1].index=2;actions[2]=actions[0];actions[2].index=3;
    trigger.action_count=3;program.triggers=&trigger;program.trigger_count=1;
    program.actions=actions;program.action_count=3;program.location_count=1;
    sched.instance_count=1;sched.instances[0].flags=SC64_Z8_INST_EXECUTING;sched.instances[0].cursor=1;
    assert(sc64_stage_trigger_create_obligations(&sched,&program)==3);
    sched.instances[0].cursor=2;assert(sc64_stage_trigger_create_obligations(&sched,&program)==2);
    sched.instances[0].flags=0;assert(sc64_stage_trigger_create_obligations(&sched,&program)==0);
    ops.action=capacity_action;instance.flags=SC64_Z8_INST_EXECUTING;
    capacity_result=0;assert(sc64_z8_action(&sched,&program,&instance,&actions[0],&ops,0,outcome)==0);
    capacity_result=1;assert(sc64_z8_action(&sched,&program,&instance,&actions[0],&ops,0,outcome)==1);
    fixture_clear_candidate_records();launch_z9_from_normal_menu();
    for(int i=0;i<MAX_BUILDINGS;i++) {b[i].queue_count=0;b[i].queue_head=0;}
    for(int i=0;i<MAX_UNITS;i++)u[i].alive=1;
    u[0].alive=0;u[1].alive=0;
    assert(race_product_slots_available(2));assert(!race_product_slots_available(3));
    /* Actual queue admission at the boundary: two pending Z9 creates and
     * three free bodies permit one Drone Egg, then deny the next purchase. */
    int hall=-1;
    for(int i=0;i<MAX_BUILDINGS;i++) if(building_complete(i) && b[i].owner==SIDE &&
        sc64_race_is_zerg_townhall(b[i].type)){hall=i;break;}
    assert(hall>=0);minerals=1000;gas=1000;b[hall].larva=3;
    for(int i=0;i<MAX_UNITS;i++) {u[i].alive=1;u[i].owner=ENEMY;u[i].type=U_DRONE;}
    u[0].alive=0;u[1].alive=0;u[2].alive=0;
    u[3].owner=SIDE;u[3].type=U_OVERLORD;
    race_zl_sched.core.instance_count=1;
    race_zl_sched.core.instances[0].trigger=4;
    race_zl_sched.core.instances[0].owner=SC64_ZL_HUMAN_SLOT;
    race_zl_sched.core.instances[0].flags=SC64_Z8_INST_EXECUTING;
    race_zl_sched.core.instances[0].cursor=0;
    assert(race_shared_trigger_obligations()==2);
    assert(race_queue_unit(SIDE,hall,U_DRONE));int remaining_ore=minerals,remaining_larva=b[hall].larva;
    assert(!race_queue_unit(SIDE,hall,U_DRONE));
    assert(minerals==remaining_ore && b[hall].larva==remaining_larva);
    puts("executing create obligations, defer and capacity boundary passed");return 0;
}
