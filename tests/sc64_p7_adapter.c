#include <assert.h>
#include <stdio.h>

#include "../starcraft/sc64_p7_adapter.h"

typedef struct {
    u8 true_condition[SC64_P7_TRIGGER_COUNT];
    unsigned actions, ai_calls, condition_calls;
    u8 action_trigger[32], action_index[32], action_opcode[32];
    int fail_action, fail_ai;
} Fake;

static int fake_condition(void *opaque, const Sc64P7RuntimeState *state,
                          const Sc64P7SourceCondition *condition)
{
    Fake *f=opaque;
    (void)state;
    assert(condition && condition->opcode==SC64_P7_CONDITION_COMMAND);
    ++f->condition_calls;
    return f->true_condition[condition->trigger];
}

static int fake_action(void *opaque, const Sc64P7SourceAction *action)
{
    Fake *f=opaque;
    if (f->fail_action) return 0;
    assert(f->actions<32);
    f->action_trigger[f->actions]=action->trigger;
    f->action_index[f->actions]=action->index;
    f->action_opcode[f->actions]=action->opcode;
    ++f->actions;
    return 1;
}

static int fake_ai(void *opaque, const Sc64P7SourceAction *action)
{
    Fake *f=opaque;
    assert(action->opcode==SC64_P7_ACTION_RUN_AI_AT_LOCATION);
    ++f->ai_calls;
    return f->fail_ai ? 0 : 1;
}

static Sc64P7WorldOps ops_for(Fake *f)
{
    Sc64P7WorldOps ops={f,0,fake_condition,fake_action,fake_ai};
    return ops;
}

static int saw_action(const Fake *f, unsigned trigger, unsigned index)
{
    for (unsigned i=0;i<f->actions;++i)
        if (f->action_trigger[i]==trigger && f->action_index[i]==index) return 1;
    return 0;
}

static void test_authenticated_metadata(void)
{
    unsigned totals=0;
    unsigned l,t,r,b;
    assert(sc64_p7_runtime_metadata_valid());
    for (unsigned t=0;t<SC64_P7_TRIGGER_COUNT;++t) {
        const Sc64P7SourceTrigger *tr=&sc64_p7_source_triggers[t];
        assert(tr->action_count==sc64_p7_action_count((u8)t));
        assert(tr->condition_count==1);
        totals+=tr->action_count;
        assert(sc64_p7_trigger_enabled(t,t==6?19:t==7?18:20));
        assert(!sc64_p7_trigger_enabled(t,0));
        for (unsigned j=0;j<tr->action_count;++j) {
            const Sc64P7SourceAction *a;
            assert(sc64_p7_source_action(t,j,&a));
            assert(a->trigger==t && a->index==j);
            assert(a->opcode==sc64_p7_expected_opcode(t,j));
            assert(a->opcode!=3); /* No PreserveTrigger: records are one-shot. */
            assert(a->opcode!=5 && a->opcode!=6); /* No source PauseGame/Unpause. */
        }
    }
    assert(totals==SC64_P7_ACTION_TOTAL);
    assert(sc64_p7_action_duration(1,0)==30);
    assert(sc64_p7_action_duration(1,1)==13);
    assert(sc64_p7_action_duration(1,2)==SC64_P7_PRESENTATION_HOLD_TICKS);
    assert(sc64_p7_action_duration(2,3)==15);
    assert(sc64_p7_action_duration(5,11)==90);
    assert(sc64_p7_action_duration(5,16)==1);
    assert(sc64_p7_ceil_ticks(0)==1);
    assert(sc64_p7_ceil_ticks(0xffffffffu)==36000);
    assert(sc64_p7_location_source(2,128,128,&l,&t,&r,&b));
    assert(l==0x0e60u && t==0x07a0u && r==0x0f40u && b==0x0860u);
    assert(sc64_p7_location_source(64,128,128,&l,&t,&r,&b));
    assert(l==0 && t==0 && r==4096 && b==4096);
    assert(!sc64_p7_location_source(0,128,128,&l,&t,&r,&b));
    assert(!sc64_p7_location_source(65,128,128,&l,&t,&r,&b));
}

static void test_elapsed_and_switch_conditions(void)
{
    Sc64P7RuntimeState s;
    Fake f={0};
    Sc64P7WorldOps ops=ops_for(&f);
    sc64_p7_runtime_init(&s);
    ops.elapsed_frames=0;
    assert(sc64_p7_adapter_tick(&s,&ops,0)==1);
    assert(sc64_p7_trigger_started(&s.model,SC64_P7_TRIGGER_OBJECTIVE_SETUP));
    assert(f.actions==1 && f.action_opcode[0]==SC64_P7_ACTION_OBJECTIVES);

    /* Both gate actions set source switch zero; its source condition is only
     * eligible on the following condition pass. */
    sc64_p7_runtime_init(&s);
    f=(Fake){0}; ops=ops_for(&f);
    f.true_condition[SC64_P7_TRIGGER_SET_GATE_FORCE2]=1;
    assert(sc64_p7_adapter_tick(&s,&ops,0)==1);
    assert(s.switch_bits==SC64_P7_SWITCH_MASK);
    assert(!sc64_p7_trigger_started(&s.model,SC64_P7_TRIGGER_VICTORY));
    assert(f.actions==2 && f.action_opcode[1]==SC64_P7_ACTION_SET_SWITCH);
    assert(sc64_p7_adapter_tick(&s,&ops,0)==1);
    assert(sc64_p7_trigger_started(&s.model,SC64_P7_TRIGGER_VICTORY));
    assert(s.model.action_cursor[SC64_P7_TRIGGER_VICTORY]==1);
    assert(f.actions==4 && f.action_opcode[3]==SC64_P7_ACTION_WAIT);
    assert(sc64_p7_runtime_state_valid(&s));
}

static void test_wait_pause_and_dispatch_order(void)
{
    Sc64P7RuntimeState s;
    Fake f={0};
    Sc64P7WorldOps ops=ops_for(&f);
    unsigned dispatched;
    sc64_p7_runtime_init(&s);
    f.true_condition[SC64_P7_TRIGGER_DEFEAT_HERO_A]=1;
    assert(sc64_p7_adapter_tick(&s,&ops,0)==1);
    assert(saw_action(&f,SC64_P7_TRIGGER_DEFEAT_HERO_A,0));
    assert(s.action_ticks[1]==30);
    dispatched=f.actions;
    assert(sc64_p7_adapter_tick(&s,&ops,1)==0);
    assert(s.action_ticks[1]==30 && f.actions==dispatched);
    for (unsigned i=0;i<29;++i) assert(sc64_p7_adapter_tick(&s,&ops,0)==1);
    assert(s.action_ticks[1]==1);
    assert(sc64_p7_adapter_tick(&s,&ops,0)==1);
    assert(saw_action(&f,SC64_P7_TRIGGER_DEFEAT_HERO_A,1));
    assert(s.action_ticks[1]==13);
}

static void test_opaque_ai_fails_closed(void)
{
    Sc64P7RuntimeState s;
    Fake f={0};
    Sc64P7WorldOps ops=ops_for(&f);
    sc64_p7_runtime_init(&s);
    /* Trigger 6 has just dispatched its resource action; its next action is
     * the opaque location-script call. */
    s.model.started=sc64_p7_trigger_bit(6);
    s.model.action_cursor[6]=2;
    s.action_ticks[6]=1;
    ops.opaque_ai=NULL;
    assert(sc64_p7_adapter_tick(&s,&ops,0)==-SC64_P7_FAULT_AI);
    assert(s.fault==SC64_P7_FAULT_AI && !sc64_p7_runtime_state_valid(&s));
}

static void test_effect_failure_and_terminal_outcome(void)
{
    Sc64P7RuntimeState s;
    Fake f={0};
    Sc64P7WorldOps ops=ops_for(&f);
    sc64_p7_runtime_init(&s);
    f.true_condition[SC64_P7_TRIGGER_DEFEAT_INITIAL]=1;
    f.fail_action=1;
    assert(sc64_p7_adapter_tick(&s,&ops,0)==-SC64_P7_FAULT_ACTION);
    assert(s.fault==SC64_P7_FAULT_ACTION);

    /* A synthetic sequence verifies that the terminal action is only a win
     * once its action duration completes. */
    sc64_p7_runtime_init(&s);
    s.model.started=sc64_p7_trigger_bit(SC64_P7_TRIGGER_VICTORY);
    s.model.action_cursor[5]=17;
    s.action_ticks[5]=1;
    s.switch_bits=SC64_P7_SWITCH_MASK;
    assert(sc64_p7_runtime_state_valid(&s));
    assert(sc64_p7_adapter_tick(&s,&ops,0)==1);
    assert(s.model.outcome==SC64_P7_WON);
    assert(s.fault==SC64_P7_FAULT_NONE && sc64_p7_runtime_state_valid(&s));
}

int main(void)
{
    test_authenticated_metadata();
    test_elapsed_and_switch_conditions();
    test_wait_pause_and_dispatch_order();
    test_opaque_ai_fails_closed();
    test_effect_failure_and_terminal_outcome();
    puts("P7 adapter tests passed");
    return 0;
}
