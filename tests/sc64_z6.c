#include <assert.h>
#include <string.h>

#include "../starcraft/sc64_z6.h"

static void set_ready(Sc64Z6Input *in, u8 trigger, u16 mask)
{
    assert(trigger < SC64_Z6_TRIGGER_COUNT);
    in->condition_true[trigger] = mask;
}

static void set_complete(Sc64Z6Input *in, u8 trigger)
{
    assert(trigger < SC64_Z6_TRIGGER_COUNT);
    in->action_complete |= (u32)1u << trigger;
}

static void inventory_and_validation(void)
{
    static const u8 actions[SC64_Z6_TRIGGER_COUNT] = {1,2,1,4,2,13,5,18,2};
    static const u8 conditions[SC64_Z6_TRIGGER_COUNT] = {1,1,2,1,3,2,1,1,1};
    Sc64Z6State s;
    Sc64Z6Input in;
    u16 action_total=0,condition_total=0;
    for (u8 t=0;t<SC64_Z6_TRIGGER_COUNT;++t) {
        assert(sc64_z6_action_count(t)==actions[t]);
        assert(sc64_z6_condition_count(t)==conditions[t]);
        action_total=(u16)(action_total+actions[t]);
        condition_total=(u16)(condition_total+conditions[t]);
    }
    assert(action_total==SC64_Z6_ACTION_TOTAL);
    assert(condition_total==SC64_Z6_CONDITION_TOTAL);
    assert(sc64_z6_action_count(SC64_Z6_TRIGGER_COUNT)==0);
    assert(sc64_z6_condition_count(SC64_Z6_TRIGGER_COUNT)==0);

    (void)sc64_z6_init(&s);
    assert(sc64_z6_state_valid(&s));
    s.action_cursor[0]=1;
    assert(!sc64_z6_state_valid(&s));
    (void)sc64_z6_init(&s);
    s.started=(u16)1u<<9;
    assert(!sc64_z6_state_valid(&s));
    (void)sc64_z6_init(&s);
    s.started=(u16)1u<<SC64_Z6_TRIGGER_VICTORY;
    assert(!sc64_z6_state_valid(&s)); /* completed but RUNNING */
    s.outcome=SC64_Z6_WON;
    s.action_cursor[SC64_Z6_TRIGGER_VICTORY]=1;
    assert(!sc64_z6_state_valid(&s)); /* cannot win before all 13 actions */

    (void)sc64_z6_init(&s);
    s.started=(u16)1u<<SC64_Z6_TRIGGER_DEFEAT_0;
    s.started|=(u16)1u<<SC64_Z6_TRIGGER_DEFEAT_1;
    s.outcome=SC64_Z6_LOST;
    assert(!sc64_z6_state_valid(&s)); /* both losses cannot have completed */

    memset(&in,0,sizeof(in));
    in.condition_true[2]=1;
    assert(sc64_z6_input_valid(&in));
    assert(!sc64_z6_conditions_met(&in,2));
    in.condition_true[2]=3;
    assert(sc64_z6_conditions_met(&in,2));
    in.condition_true[2]=4;
    assert(!sc64_z6_input_valid(&in));
    memset(&in,0,sizeof(in));
    in.action_complete=(u32)1u<<9;
    assert(!sc64_z6_input_valid(&in));
}

static void pause_multiactive_and_resume(void)
{
    Sc64Z6State s, saved;
    Sc64Z6Input in;
    Sc64Z6Output out;
    (void)sc64_z6_init(&s);
    memset(&in,0,sizeof(in));
    set_ready(&in,2,3); /* both Bring conditions are required */
    set_ready(&in,3,1);
    in.paused=1;
    out=sc64_z6_step(&s,&in);
    assert(out.action_count==0 && s.started==0);
    in.paused=0;
    out=sc64_z6_step(&s,&in);
    assert(out.action_count==2);
    assert(out.actions[0].trigger==2 && out.actions[0].action_index==0);
    assert(out.actions[1].trigger==3 && out.actions[1].action_index==0);
    assert(sc64_z6_state_valid(&s));
    saved=s;

    memset(&in,0,sizeof(in));
    set_complete(&in,2);
    set_complete(&in,3);
    out=sc64_z6_step(&saved,&in);
    assert(out.action_count==1);
    assert(out.actions[0].trigger==3 && out.actions[0].action_index==1);
    assert(saved.action_cursor[2]==0); /* one-action switch record completed */
    assert(saved.action_cursor[3]==2);
    assert(sc64_z6_state_valid(&saved));
}

static void terminal_source_order(void)
{
    Sc64Z6State s;
    Sc64Z6Input in;
    Sc64Z6Output out;
    (void)sc64_z6_init(&s);
    s.started=(u16)((1u<<SC64_Z6_TRIGGER_DEFEAT_0)|
                    (1u<<SC64_Z6_TRIGGER_VICTORY)|
                    (1u<<SC64_Z6_TRIGGER_DEFEAT_1));
    s.action_cursor[SC64_Z6_TRIGGER_DEFEAT_0]=1;
    s.action_cursor[SC64_Z6_TRIGGER_VICTORY]=SC64_Z6_ACTION_TOTAL;
    s.action_cursor[SC64_Z6_TRIGGER_DEFEAT_1]=SC64_Z6_ACTION_TOTAL;
    /* Set exact valid terminal cursors for each record. */
    s.action_cursor[SC64_Z6_TRIGGER_VICTORY]=sc64_z6_action_count(SC64_Z6_TRIGGER_VICTORY);
    s.action_cursor[SC64_Z6_TRIGGER_DEFEAT_1]=sc64_z6_action_count(SC64_Z6_TRIGGER_DEFEAT_1);
    assert(sc64_z6_state_valid(&s));
    memset(&in,0,sizeof(in));
    set_complete(&in,SC64_Z6_TRIGGER_DEFEAT_0);
    set_complete(&in,SC64_Z6_TRIGGER_VICTORY);
    set_complete(&in,SC64_Z6_TRIGGER_DEFEAT_1);
    out=sc64_z6_step(&s,&in);
    assert(out.events&SC64_Z6_EVENT_LOST);
    assert(!(out.events&SC64_Z6_EVENT_WON));
    assert(s.outcome==SC64_Z6_LOST && sc64_z6_state_valid(&s));

    (void)sc64_z6_init(&s);
    s.started=(u16)1u<<SC64_Z6_TRIGGER_VICTORY;
    s.action_cursor[SC64_Z6_TRIGGER_VICTORY]=
        sc64_z6_action_count(SC64_Z6_TRIGGER_VICTORY);
    memset(&in,0,sizeof(in));
    set_complete(&in,SC64_Z6_TRIGGER_VICTORY);
    out=sc64_z6_step(&s,&in);
    assert(out.events&SC64_Z6_EVENT_WON);
    assert(s.outcome==SC64_Z6_WON && sc64_z6_state_valid(&s));
}

int main(void)
{
    inventory_and_validation();
    pause_multiactive_and_resume();
    terminal_source_order();
    return 0;
}
