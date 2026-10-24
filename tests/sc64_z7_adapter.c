#include <assert.h>
#include "../starcraft/sc64_z7_adapter.h"

int main(void)
{
    static const u8 actions[8]={1,3,1,5,2,1,1,1};
    assert(sc64_z7_runtime_metadata_valid());
    assert(SC64_Z7_SOURCE_GLOBAL_ID==18u && SC64_Z7_SOURCE_MAP_COLS==128u &&
           SC64_Z7_SOURCE_MAP_ROWS==128u);
    assert(SC64_Z7_SOURCE_ACTION_COUNT==15u && SC64_Z7_SOURCE_CONDITION_COUNT==8u);
    for (unsigned t=0;t<8;++t) {
        unsigned executor=99;
        assert(sc64_z7_action_count((u8)t)==actions[t]);
        assert(sc64_z7_condition_count((u8)t)==1u);
        assert(sc64_z7_trigger_executor(t,&executor));
        assert(executor==(t==3?19u:18u));
        for (unsigned a=0;a<actions[t];++a) {
            const Sc64Z7SourceAction *row=0;
            assert(sc64_z7_source_action(t,a,&row));
            assert(row->trigger==t && row->index==a);
            assert(sc64_z7_action_duration(t,a)>0);
        }
    }
    assert(sc64_z7_action_duration(4,0)==300u); /* Authenticated 10-second wait. */
    assert(sc64_z7_action_duration(3,4)==1u);
    assert(!sc64_z7_action_duration(3,5));
    {
        Sc64Z7RuntimeState s={0};
        assert(sc64_z7_runtime_state_valid(&s));
        s.ai_script_mask=2;
        assert(!sc64_z7_runtime_state_valid(&s)); /* Only source-prefix AI cursors are valid. */
        s.ai_script_mask=3; s.model.started=1u<<SC64_Z7_TRIGGER_AI;
        s.model.action_cursor[SC64_Z7_TRIGGER_AI]=3u;
        s.action_ticks[SC64_Z7_TRIGGER_AI]=1u;
        s.ai_plan_ticks=SC64_Z7_AI_PERIOD_TICKS;
        assert(sc64_z7_runtime_state_valid(&s));
        s.ai_plan_ticks=301;
        assert(!sc64_z7_runtime_state_valid(&s));
    }
    return 0;
}
