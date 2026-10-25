#include <assert.h>
#include <stdio.h>

#include "../starcraft/sc64_p7.h"

static void test_source_bounded_counts(void)
{
    static const u8 actions[SC64_P7_TRIGGER_COUNT] = {1,5,5,5,4,17,3,2,1,1};
    Sc64P7State s;
    sc64_p7_init(&s);
    assert(sc64_p7_state_valid(&s));
    for (u8 i=0;i<SC64_P7_TRIGGER_COUNT;++i) {
        assert(sc64_p7_action_count(i)==actions[i]);
        assert(sc64_p7_condition_count(i)==1);
    }
    assert(sc64_p7_action_count(SC64_P7_TRIGGER_COUNT)==0);
}

static void test_one_shot_and_tie_order(void)
{
    Sc64P7State s;
    Sc64P7Input in={{0},0,0};
    Sc64P7Output out;
    sc64_p7_init(&s);
    for (unsigned i=0;i<SC64_P7_TRIGGER_COUNT;++i) in.condition_true[i]=1;
    out=sc64_p7_step(&s,&in);
    assert(out.action_count==SC64_P7_TRIGGER_COUNT);
    assert(out.actions[0].trigger==0 && out.actions[9].trigger==9);
    assert(s.outcome==SC64_P7_RUNNING && s.action_cursor[0]==1);

    in.action_complete=(u16)0x03ff;
    out=sc64_p7_step(&s,&in);
    assert(s.outcome==SC64_P7_LOST);
    assert(out.events&SC64_P7_EVENT_LOST);
    assert(s.action_cursor[0]==0 && s.action_cursor[1]==1);
    assert(sc64_p7_state_valid(&s));
}

static void test_nonterminal_stays_one_shot(void)
{
    Sc64P7State s;
    Sc64P7Input in={{0},0,0};
    Sc64P7Output out;
    sc64_p7_init(&s);
    in.condition_true[SC64_P7_TRIGGER_OBJECTIVE_SETUP]=1;
    out=sc64_p7_step(&s,&in);
    assert(out.action_count==1 && out.actions[0].action_index==0);
    for (unsigned next=1;next<sc64_p7_action_count(SC64_P7_TRIGGER_OBJECTIVE_SETUP);++next) {
        in.action_complete=sc64_p7_trigger_bit(SC64_P7_TRIGGER_OBJECTIVE_SETUP);
        out=sc64_p7_step(&s,&in);
        assert(out.action_count==1 && out.actions[0].trigger==SC64_P7_TRIGGER_OBJECTIVE_SETUP);
        assert(out.actions[0].action_index==next);
    }
    in.action_complete=sc64_p7_trigger_bit(SC64_P7_TRIGGER_OBJECTIVE_SETUP);
    out=sc64_p7_step(&s,&in);
    assert(out.action_count==0 && s.action_cursor[SC64_P7_TRIGGER_OBJECTIVE_SETUP]==0);
    assert(sc64_p7_trigger_started(&s,SC64_P7_TRIGGER_OBJECTIVE_SETUP));
    out=sc64_p7_step(&s,&in);
    assert(out.action_count==0); /* No PreserveTrigger action exists in this source. */
}

static void test_pause_and_state_rejection(void)
{
    Sc64P7State s, before;
    Sc64P7Input in={{0},0,0};
    Sc64P7Output out;
    sc64_p7_init(&s);
    in.condition_true[4]=1;
    in.paused=1;
    before=s;
    out=sc64_p7_step(&s,&in);
    assert(!out.action_count && !out.events);
    assert(!__builtin_memcmp(&s,&before,sizeof(s)));
    in.paused=0;
    in.action_complete=1;
    assert(!sc64_p7_input_state_valid(&s,&in));
    s.started=(u16)1u<<5;
    s.outcome=SC64_P7_WON;
    s.action_cursor[5]=1;
    assert(!sc64_p7_state_valid(&s)); /* A won record must be fully complete. */
    s.action_cursor[5]=0;
    assert(sc64_p7_state_valid(&s));
    s.started|=(u16)1u<<1;
    assert(!sc64_p7_state_valid(&s)); /* Two completed terminal records cannot occur. */
}

static void test_victory_loss_same_tick_precedence(void)
{
    Sc64P7State s;
    Sc64P7Input in={{0},0,0};
    Sc64P7Output out;
    sc64_p7_init(&s);
    s.started=sc64_p7_trigger_bit(3)|sc64_p7_trigger_bit(5);
    s.action_cursor[3]=sc64_p7_action_count(3);
    s.action_cursor[5]=sc64_p7_action_count(5);
    in.action_complete=sc64_p7_trigger_bit(3)|sc64_p7_trigger_bit(5);
    assert(sc64_p7_state_valid(&s));
    out=sc64_p7_step(&s,&in);
    assert(s.outcome==SC64_P7_LOST && (out.events&SC64_P7_EVENT_LOST));

    sc64_p7_init(&s);
    s.started=sc64_p7_trigger_bit(5)|sc64_p7_trigger_bit(6);
    s.action_cursor[5]=sc64_p7_action_count(5);
    s.action_cursor[6]=sc64_p7_action_count(6);
    in.action_complete=sc64_p7_trigger_bit(5)|sc64_p7_trigger_bit(6);
    assert(sc64_p7_state_valid(&s));
    out=sc64_p7_step(&s,&in);
    assert(s.outcome==SC64_P7_WON && (out.events&SC64_P7_EVENT_WON));
}

int main(void)
{
    test_source_bounded_counts();
    test_one_shot_and_tie_order();
    test_nonterminal_stays_one_shot();
    test_pause_and_state_rejection();
    test_victory_loss_same_tick_precedence();
    puts("P7 scheduler tests passed");
    return 0;
}
