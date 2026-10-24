#include <assert.h>
#include <string.h>

#include "../starcraft/sc64_z6_adapter.h"

typedef struct {
    u8 true_trigger, action_calls, condition_calls;
    u8 fail_action, fail_condition;
    u8 last_opcode, last_trigger, last_index;
    u16 creates;
} Mock;

static int mock_condition(void *opaque, const Sc64Z6RuntimeState *state,
                          const Sc64Z6SourceCondition *condition)
{
    Mock *m=(Mock *)opaque;
    (void)state;
    ++m->condition_calls;
    if (m->fail_condition) return -1;
    return condition->trigger==m->true_trigger;
}

static int mock_action(void *opaque, const Sc64Z6SourceAction *action)
{
    Mock *m=(Mock *)opaque;
    ++m->action_calls;
    m->last_opcode=action->opcode;
    m->last_trigger=action->trigger;
    m->last_index=action->index;
    if (action->opcode==11) ++m->creates;
    return m->fail_action ? 0 : 1;
}

static void source_metadata_and_actor_census(void)
{
    unsigned actions=0,conditions=0,create=0, invincibility=0,properties=0;
    assert(sc64_z6_runtime_metadata_valid());
    assert(SC64_Z6_SOURCE_GLOBAL_ID==17);
    assert(SC64_Z6_SOURCE_MAP_COLS==128 && SC64_Z6_SOURCE_MAP_ROWS==128);
    assert(sc64_z6_trigger_enabled(0,18));
    assert(sc64_z6_trigger_enabled(7,18));
    assert(sc64_z6_trigger_enabled(8,1));
    assert(!sc64_z6_trigger_enabled(8,18));
    assert(!sc64_z6_trigger_enabled(SC64_Z6_TRIGGER_COUNT,18));
    assert(!sc64_z6_trigger_enabled(0,27));
    for (unsigned t=0;t<SC64_Z6_TRIGGER_COUNT;++t) {
        const Sc64Z6SourceTrigger *tr=&sc64_z6_source_triggers[t];
        actions+=tr->action_count;
        conditions+=tr->condition_count;
        for (unsigned a=0;a<tr->action_count;++a) {
            const Sc64Z6SourceAction *row;
            assert(sc64_z6_source_action(t,a,&row));
            assert(sc64_z6_action_duration(t,a)>0);
            if (row->opcode==11 || row->opcode==44) ++create;
            if (row->opcode==43) ++invincibility;
        }
    }
    assert(actions==48 && conditions==13);
    assert(create==3 && invincibility==0);
    assert(sc64_z6_ceil_ticks(2000)==60);
    assert(sc64_z6_ceil_ticks(1)==1);
    assert(sc64_z6_ceil_ticks(0xffffffffu)==36000);
    for (unsigned i=0;i<64;++i) properties+=sc64_z6_source_property_used[i]!=0;
    assert(properties==2);
    assert(sc64_z6_source_property_used[0]==1);
    assert(sc64_z6_source_property_used[1]==1);
    for (unsigned i=2;i<64;++i) assert(sc64_z6_source_property_used[i]==0);

    /* Create-unit requests retain exact numeric rows, owners/counts and
       property flags for the adapter. Raw 79 is Tassadar; 101 is a revealer. */
    {
        static const u8 triggers[3]={4,5,7};
        static const u8 indexes[3]={0,2,3};
        static const u16 raw[3]={79,101,101};
        static const u32 owner_operand[3]={2,3,18};
        static const u32 count_operand[3]={1,2,0};
        static const u8 flags[3]={24,24,16};
        Mock mock={0};
        Sc64Z6WorldOps ops={&mock,mock_condition,mock_action};
        for (u8 n=0;n<3;++n) {
            Sc64Z6RuntimeState state;
            Sc64Z6ActionEvent event={triggers[n],indexes[n]};
            (void)sc64_z6_runtime_init(&state);
            state.model.started=(u16)1u<<event.trigger;
            state.model.action_cursor[event.trigger]=(u8)(event.action_index+1u);
            assert(sc64_z6_dispatch_action(&state,&ops,&event)==SC64_Z6_FAULT_NONE);
            assert(mock.last_opcode==11 && mock.last_trigger==event.trigger);
            assert(mock.last_index==event.action_index);
            assert(mock.creates==n+1);
            const Sc64Z6SourceAction *row;
            assert(sc64_z6_source_action(event.trigger,event.action_index,&row));
            assert(row->unit==raw[n] && row->group1==owner_operand[n]);
            assert(row->group2==count_operand[n] && row->flags==flags[n]);
            assert(row->location==1);
        }
    }
}

static void adapter_switch_and_fail_closed(void)
{
    Sc64Z6RuntimeState state;
    Mock mock={.true_trigger=2};
    Sc64Z6WorldOps ops={&mock,mock_condition,mock_action};
    (void)sc64_z6_runtime_init(&state);
    assert(sc64_z6_runtime_state_valid(&state));
    assert(sc64_z6_adapter_tick(&state,&ops,0)==1);
    assert(mock.condition_calls==SC64_Z6_CONDITION_TOTAL);
    assert(mock.action_calls==1 && mock.last_opcode==13 && mock.last_trigger==2);
    assert(state.model.action_cursor[2]==1);
    assert(sc64_z6_switch_is_set(&state,0));
    assert(!sc64_z6_switch_is_set(&state,1));
    assert(sc64_z6_runtime_state_valid(&state));
    assert(sc64_z6_adapter_tick(&state,&ops,1)==0); /* user pause freezes timer */
    assert(state.action_ticks[2]==1);
    assert(sc64_z6_adapter_tick(&state,&ops,0)==1);
    assert(state.model.action_cursor[2]==0);
    assert(state.model.started&((u16)1u<<2)); /* one-shot latch */
    assert(sc64_z6_runtime_state_valid(&state));

    (void)sc64_z6_runtime_init(&state);
    mock=(Mock){.true_trigger=2,.fail_action=1};
    assert(sc64_z6_adapter_tick(&state,&ops,0)==-SC64_Z6_FAULT_ACTION);
    assert(state.fault==SC64_Z6_FAULT_ACTION);
    assert(!sc64_z6_runtime_state_valid(&state));

    (void)sc64_z6_runtime_init(&state);
    mock=(Mock){.fail_condition=1};
    assert(sc64_z6_adapter_tick(&state,&ops,0)==-SC64_Z6_FAULT_CONDITION);
    assert(state.fault==SC64_Z6_FAULT_CONDITION);
}

int main(void)
{
    source_metadata_and_actor_census();
    adapter_switch_and_fail_closed();
    return 0;
}
