#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_p6_adapter.h"

typedef struct {
    unsigned actions,conditions,ai,creates,doodads,kills;
    int true_trigger,fail_condition,fail_action,fail_ai;
    u8 last_opcode,last_trigger,last_index;
} Mock;

static int mock_condition(void *opaque,const Sc64P6RuntimeState *state,
                          const Sc64P6SourceCondition *row)
{
    Mock *m=(Mock *)opaque; (void)state; ++m->conditions;
    if (m->fail_condition) return -1;
    return row->trigger==(u8)m->true_trigger;
}

static int mock_action(void *opaque,const Sc64P6SourceAction *row)
{
    Mock *m=(Mock *)opaque; ++m->actions;
    m->last_opcode=row->opcode; m->last_trigger=row->trigger; m->last_index=row->index;
    if (row->opcode==SC64_P6_ACTION_CREATE_UNIT) ++m->creates;
    if (row->opcode==SC64_P6_ACTION_SET_DOODAD) ++m->doodads;
    if (row->opcode==SC64_P6_ACTION_KILL_UNIT) ++m->kills;
    return m->fail_action ? 0 : 1;
}

static int mock_ai(void *opaque,const Sc64P6SourceAction *row)
{
    Mock *m=(Mock *)opaque; ++m->ai;
    m->last_opcode=row->opcode; m->last_trigger=row->trigger; m->last_index=row->index;
    return m->fail_ai ? 0 : 1;
}

static void metadata_and_source_geometry(void)
{
    static const u8 executor[SC64_P6_TRIGGER_COUNT]={
        17,17,18,18,18,18,18,18,7,18,18,18,18,19,18,7,18,18,18,18,18,18,18,17,17,17
    };
    unsigned actions=0,conditions=0,creates=0,properties=0,ais=0;
    unsigned left,top,right,bottom;
    int rleft,rtop,rright,rbottom;
    assert(sc64_p6_runtime_metadata_valid());
    assert(SC64_P6_SOURCE_GLOBAL_ID==25);
    assert(SC64_P6_SOURCE_MAP_COLS==128 && SC64_P6_SOURCE_MAP_ROWS==128);
    for (u8 t=0;t<SC64_P6_TRIGGER_COUNT;++t) {
        assert(sc64_p6_trigger_enabled(t,executor[t]));
        assert(!sc64_p6_trigger_enabled(t,27));
        const Sc64P6SourceTrigger *tr=&sc64_p6_source_triggers[t];
        actions+=tr->action_count; conditions+=tr->condition_count;
        for (u8 i=0;i<tr->action_count;++i) {
            const Sc64P6SourceAction *a;
            assert(sc64_p6_source_action(t,i,&a));
            assert(sc64_p6_action_duration(t,i)>0);
            creates+=a->opcode==SC64_P6_ACTION_CREATE_UNIT;
            properties+=a->opcode==SC64_P6_ACTION_CREATE_UNIT && a->group2!=0;
            ais+=a->opcode==SC64_P6_ACTION_RUN_AI;
        }
    }
    assert(actions==99 && conditions==29 && creates==18 && properties==13 && ais==1);
    assert(sc64_p6_ceil_ticks(1000)==30 && sc64_p6_ceil_ticks(1)==1);
    assert(sc64_p6_ceil_ticks(0xffffffffu)==36000);
    assert(sc64_p6_location_source(1,128,128,&left,&top,&right,&bottom));
    assert(left<=right && top<=bottom && right<=4096 && bottom<=4096);
    assert(sc64_p6_location_source(64,128,128,&left,&top,&right,&bottom));
    assert(left==0 && top==0 && right==4096 && bottom==4096);
    assert(!sc64_p6_location_source(0,128,128,&left,&top,&right,&bottom));
    assert(!sc64_p6_location_source(65,128,128,&left,&top,&right,&bottom));
    assert(!sc64_p6_location_source(1,129,128,&left,&top,&right,&bottom));
    assert(sc64_p6_location_runtime(64,128,128,&rleft,&rtop,&rright,&rbottom));
    assert(rleft==0 && rtop==0 && rright==2048 && rbottom==2048);
    for (unsigned id=1;id<=13;++id) {
        const unsigned char *property=NULL;
        assert(sc64_p6_source_property(id,&property) && property);
    }
    { const unsigned char *property=NULL; assert(!sc64_p6_source_property(14,&property)); }
}

static void selector_comparison_and_switch_semantics(void)
{
    u8 active[12]={0},forces[8]={0};
    Sc64P6RuntimeState s;
    Sc64P6RuntimeState condition_state;
    const Sc64P6SourceCondition *row;
    active[7]=active[2]=active[6]=1; forces[7]=0; forces[2]=0; forces[6]=1;
    assert(sc64_p6_owner_matches(13,7,7,active,forces)==1);
    assert(sc64_p6_owner_matches(17,2,7,active,forces)==1);
    assert(sc64_p6_owner_matches(18,2,7,active,forces)==1);
    assert(sc64_p6_owner_matches(19,6,7,active,forces)==1);
    assert(sc64_p6_owner_matches(18,6,7,active,forces)==0);
    assert(sc64_p6_owner_matches(22,7,7,active,forces)==-1);
    assert(sc64_p6_count_comparison(3,0,3)==1);
    assert(sc64_p6_count_comparison(2,1,3)==1);
    assert(sc64_p6_count_comparison(3,10,3)==1);
    assert(sc64_p6_count_comparison(3,7,3)==-1);
    sc64_p6_runtime_init(&s);
    assert(sc64_p6_condition_row(9,1,&row));
    assert(row->opcode==SC64_P6_CONDITION_SWITCH && row->comparison==3 && row->resource==1);
    condition_state=s;
    assert(sc64_p6_switch_condition(&condition_state,row)==1);
    condition_state.switch_bits|=(u32)1u<<1;
    assert(sc64_p6_switch_condition(&condition_state,row)==0);
    assert(sc64_p6_condition_row(25,0,&row));
    assert(row->opcode==SC64_P6_CONDITION_SWITCH && row->comparison==2 && row->resource==0);
    assert(sc64_p6_switch_condition(&condition_state,row)==0);
    condition_state.switch_bits|=1u;
    assert(sc64_p6_switch_condition(&condition_state,row)==1);
}

static void callbacks_and_exact_source_rows(void)
{
    Mock mock={.true_trigger=0};
    Sc64P6WorldOps ops={&mock,mock_condition,mock_action,mock_ai};
    Sc64P6RuntimeState s;
    sc64_p6_runtime_init(&s);
    assert(sc64_p6_runtime_state_valid(&s));
    assert(sc64_p6_adapter_tick(&s,&ops,1)==0);
    assert(!mock.conditions && !mock.actions);
    assert(sc64_p6_adapter_tick(&s,&ops,0)==1);
    assert(mock.conditions==SC64_P6_CONDITION_TOTAL);
    assert(mock.actions==1 && mock.last_opcode==SC64_P6_ACTION_SET_SWITCH);
    assert(sc64_p6_switch_is_set(&s,4)==0); /* source action clears switch 4 */
    assert(s.model.action_cursor[0]==1 && s.action_ticks[0]==1);
    assert(sc64_p6_adapter_tick(&s,&ops,0)==1);
    assert(s.model.action_cursor[0]==0 && sc64_p6_trigger_started(&s.model,0));
    assert(sc64_p6_runtime_state_valid(&s));

    /* Dispatch each authenticated row independently: callbacks receive the
     * original trigger/index/operands; Create, Kill, doodad and opaque-AI
     * paths are each exercised without claiming an AI implementation. */
    unsigned creates=0,doodads=0,kills=0,ais=0;
    for (u8 t=0;t<SC64_P6_TRIGGER_COUNT;++t) {
        const Sc64P6SourceTrigger *tr=&sc64_p6_source_triggers[t];
        for (u8 i=0;i<tr->action_count;++i) {
            Sc64P6RuntimeState fresh; Sc64P6ActionEvent ev={t,i};
            sc64_p6_runtime_init(&fresh);
            fresh.model.started=sc64_p6_trigger_bit(t);
            fresh.model.action_cursor[t]=(u8)(i+1u);
            assert(sc64_p6_dispatch_action(&fresh,&ops,&ev)==SC64_P6_FAULT_NONE);
            assert(fresh.action_ticks[t]==sc64_p6_action_duration(t,i));
            assert(sc64_p6_runtime_state_valid(&fresh));
            creates+=mock.last_opcode==SC64_P6_ACTION_CREATE_UNIT;
            doodads+=mock.last_opcode==SC64_P6_ACTION_SET_DOODAD;
            kills+=mock.last_opcode==SC64_P6_ACTION_KILL_UNIT;
            ais+=mock.last_opcode==SC64_P6_ACTION_RUN_AI;
        }
    }
    assert(creates==18 && doodads==18 && kills==2 && ais==1 && mock.ai==1);

    { /* Opaque source AI cannot silently succeed when no policy is supplied. */
        Sc64P6RuntimeState fresh; Sc64P6ActionEvent ev={13,0};
        Sc64P6WorldOps no_ai={&mock,mock_condition,mock_action,NULL};
        sc64_p6_runtime_init(&fresh);
        fresh.model.started=sc64_p6_trigger_bit(13);
        fresh.model.action_cursor[13]=1;
        assert(sc64_p6_dispatch_action(&fresh,&no_ai,&ev)==SC64_P6_FAULT_AI);
    }
    { /* A condition failure makes the state unsaveable. */
        Sc64P6RuntimeState bad; Mock failing={.fail_condition=1};
        Sc64P6WorldOps bad_ops={&failing,mock_condition,mock_action,mock_ai};
        sc64_p6_runtime_init(&bad);
        assert(sc64_p6_adapter_tick(&bad,&bad_ops,0)==-SC64_P6_FAULT_CONDITION);
        assert(!sc64_p6_runtime_state_valid(&bad));
    }
    { /* A failed exact action callback fails closed. */
        Sc64P6RuntimeState bad; Mock failing={.fail_action=1,.true_trigger=0};
        Sc64P6WorldOps bad_ops={&failing,mock_condition,mock_action,mock_ai};
        sc64_p6_runtime_init(&bad);
        assert(sc64_p6_adapter_tick(&bad,&bad_ops,0)==-SC64_P6_FAULT_ACTION);
        assert(!sc64_p6_runtime_state_valid(&bad));
    }
}

int main(void)
{
    metadata_and_source_geometry();
    selector_comparison_and_switch_semantics();
    callbacks_and_exact_source_rows();
    return 0;
}
