#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_p6.h"

static void ready(Sc64P6Input *in, u8 trigger, u16 mask)
{ assert(trigger<SC64_P6_TRIGGER_COUNT); in->condition_true[trigger]=mask; }
static void complete(Sc64P6Input *in, u8 trigger)
{ assert(trigger<SC64_P6_TRIGGER_COUNT); in->action_complete|=sc64_p6_trigger_bit(trigger); }

static void inventory_validation(void)
{
    static const u8 actions[SC64_P6_TRIGGER_COUNT]={
        1,1,2,2,2,2,2,2,5,2,2,8,8,1,7,12,5,2,3,3,7,12,3,1,3,1
    };
    static const u8 conditions[SC64_P6_TRIGGER_COUNT]={
        1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2
    };
    unsigned ac=0,cc=0;
    Sc64P6State s;
    Sc64P6Input in;
    for (u8 i=0;i<SC64_P6_TRIGGER_COUNT;++i) {
        assert(sc64_p6_action_count(i)==actions[i]);
        assert(sc64_p6_condition_count(i)==conditions[i]);
        ac+=actions[i]; cc+=conditions[i];
    }
    assert(ac==SC64_P6_ACTION_TOTAL && cc==SC64_P6_CONDITION_TOTAL);
    assert(!sc64_p6_action_count(SC64_P6_TRIGGER_COUNT));
    assert(!sc64_p6_condition_count(SC64_P6_TRIGGER_COUNT));
    sc64_p6_init(&s);
    assert(sc64_p6_state_valid(&s));
    s.action_cursor[0]=1; assert(!sc64_p6_state_valid(&s));
    sc64_p6_init(&s); s.started=(u32)1u<<31; assert(!sc64_p6_state_valid(&s));
    sc64_p6_init(&s); s.started=sc64_p6_trigger_bit(SC64_P6_TRIGGER_VICTORY);
    assert(!sc64_p6_state_valid(&s));
    s.outcome=SC64_P6_WON; s.action_cursor[SC64_P6_TRIGGER_VICTORY]=1;
    assert(!sc64_p6_state_valid(&s));
    memset(&in,0,sizeof(in));
    in.condition_true[9]=1; assert(sc64_p6_input_valid(&in));
    in.condition_true[9]=4; assert(!sc64_p6_input_valid(&in));
    memset(&in,0,sizeof(in)); in.action_complete=(u32)1u<<31;
    assert(!sc64_p6_input_valid(&in));
}

static void one_shot_pause_and_resume(void)
{
    Sc64P6State s,saved;
    Sc64P6Input in;
    Sc64P6Output out;
    sc64_p6_init(&s); memset(&in,0,sizeof(in));
    ready(&in,2,1); ready(&in,3,1); in.paused=1;
    out=sc64_p6_step(&s,&in);
    assert(!out.action_count && !s.started);
    in.paused=0; out=sc64_p6_step(&s,&in);
    assert(out.action_count==2 && out.actions[0].trigger==2 && out.actions[1].trigger==3);
    saved=s;
    memset(&in,0,sizeof(in)); complete(&in,2); complete(&in,3);
    out=sc64_p6_step(&saved,&in);
    assert(out.action_count==2);
    assert(out.actions[0].trigger==2 && out.actions[0].action_index==1);
    assert(out.actions[1].trigger==3 && out.actions[1].action_index==1);
    complete(&in,2); complete(&in,3);
    out=sc64_p6_step(&saved,&in);
    assert(!out.action_count);
    assert(saved.action_cursor[2]==0 && saved.action_cursor[3]==0);
    assert(sc64_p6_trigger_started(&saved,2) && sc64_p6_state_valid(&saved));
    memset(&in,0,sizeof(in)); ready(&in,2,1);
    out=sc64_p6_step(&saved,&in); assert(!out.action_count); /* one-shot latch */
}

static void source_order_terminal_tie(void)
{
    Sc64P6State s; Sc64P6Input in; Sc64P6Output out;
    sc64_p6_init(&s);
    s.started=sc64_p6_trigger_bit(1)|sc64_p6_trigger_bit(8)|
              sc64_p6_trigger_bit(15);
    s.action_cursor[1]=sc64_p6_action_count(1);
    s.action_cursor[8]=sc64_p6_action_count(8);
    s.action_cursor[15]=sc64_p6_action_count(15);
    assert(sc64_p6_state_valid(&s));
    memset(&in,0,sizeof(in)); complete(&in,1); complete(&in,8); complete(&in,15);
    out=sc64_p6_step(&s,&in);
    assert(out.events&SC64_P6_EVENT_LOST);
    assert(!(out.events&SC64_P6_EVENT_WON));
    assert(s.outcome==SC64_P6_LOST && sc64_p6_state_valid(&s));

    sc64_p6_init(&s); s.started=sc64_p6_trigger_bit(15);
    s.action_cursor[15]=sc64_p6_action_count(15);
    memset(&in,0,sizeof(in)); complete(&in,15);
    out=sc64_p6_step(&s,&in);
    assert(out.events&SC64_P6_EVENT_WON);
    assert(s.outcome==SC64_P6_WON && sc64_p6_state_valid(&s));
}

int main(void)
{
    inventory_validation();
    one_shot_pause_and_resume();
    source_order_terminal_tie();
    return 0;
}
