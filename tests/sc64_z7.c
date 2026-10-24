#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_z7.h"

static void test_source_scheduler(void)
{
    Sc64Z7State s; Sc64Z7Input in={{0},0,0}; Sc64Z7Output out;
    assert(sc64_z7_init(&s)==0 && sc64_z7_state_valid(&s));
    in.condition_true[1]=1; in.condition_true[4]=1;
    out=sc64_z7_step(&s,&in);
    assert(out.action_count==2 && out.actions[0].trigger==1 && out.actions[0].action_index==0 &&
           out.actions[1].trigger==4 && out.actions[1].action_index==0);
    assert(s.started==((1u<<1)|(1u<<4)) && s.action_cursor[1]==1 && s.action_cursor[4]==1);

    in.condition_true[1]=in.condition_true[4]=0;
    in.action_complete=(1u<<1)|(1u<<4);
    out=sc64_z7_step(&s,&in);
    assert(out.action_count==2 && out.actions[0].trigger==1 && out.actions[0].action_index==1 &&
           out.actions[1].trigger==4 && out.actions[1].action_index==1);
    in.action_complete=(1u<<1)|(1u<<4);
    out=sc64_z7_step(&s,&in);
    assert(out.action_count==1 && out.actions[0].trigger==1 && out.actions[0].action_index==2);
    in.action_complete=(1u<<1);
    out=sc64_z7_step(&s,&in);
    assert(!s.action_cursor[1] && !s.action_cursor[4] && sc64_z7_state_valid(&s));

    /* Terminal resolution follows authenticated source record order. */
    in.action_complete=0; in.condition_true[0]=1; in.condition_true[2]=1;
    out=sc64_z7_step(&s,&in);
    assert(out.action_count==2 && out.actions[0].trigger==0 && out.actions[1].trigger==2);
    in.condition_true[0]=in.condition_true[2]=0; in.action_complete=(1u<<0)|(1u<<2);
    out=sc64_z7_step(&s,&in);
    assert(s.outcome==SC64_Z7_LOST && (out.events&SC64_Z7_EVENT_LOST));
    assert(sc64_z7_state_valid(&s));
}

static void test_pause_and_bad_inputs(void)
{
    Sc64Z7State s; Sc64Z7Input in={{0},0,0}; Sc64Z7Output out;
    sc64_z7_init(&s); in.condition_true[1]=1; in.paused=1;
    out=sc64_z7_step(&s,&in);
    assert(!out.action_count && !s.started);
    in.paused=0; out=sc64_z7_step(&s,&in);
    assert(out.action_count==1 && s.action_cursor[1]==1);
    in.action_complete=(1u<<8);
    assert(!sc64_z7_input_valid(&in));
    in.action_complete=0; s.action_cursor[1]=4;
    assert(!sc64_z7_state_valid(&s));
}

int main(void)
{
    test_source_scheduler();
    test_pause_and_bad_inputs();
    return 0;
}
