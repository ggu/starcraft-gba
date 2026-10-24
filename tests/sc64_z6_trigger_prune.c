#include <assert.h>
#include <stdio.h>
#include <string.h>
#define SC64_Z6_PRUNE_STARTED 1
#include "../starcraft/sc64_z6_adapter.h"

int sc64_z6_reference_tick(void *state,const void *ops,int user_paused);

typedef struct {
    u32 tick;
    int policy;
    int invalid_trigger;
    int invalid_action_trigger;
    u32 conditions[SC64_Z6_TRIGGER_COUNT];
    struct { u8 trigger,index,opcode; } actions[SC64_Z6_ACTION_TOTAL];
    unsigned action_count;
} Fixture;

static int condition(void *opaque,const Sc64Z6RuntimeState *state,
                     const Sc64Z6SourceCondition *row)
{
    Fixture *f=(Fixture *)opaque;
    assert(state && row && row->trigger<SC64_Z6_TRIGGER_COUNT);
    ++f->conditions[row->trigger];
    if ((int)row->trigger==f->invalid_trigger) return -1;
    if (row->trigger==SC64_Z6_TRIGGER_DEFEAT_0 ||
        row->trigger==SC64_Z6_TRIGGER_DEFEAT_1) return 0;
    if (row->trigger==SC64_Z6_TRIGGER_VICTORY) return f->policy==1;
    if (f->policy==2) return 0;
    if (f->policy==3) return 1;
    return ((f->tick+row->trigger*11u+row->index*5u)%23u)<18u;
}

static int action(void *opaque,const Sc64Z6SourceAction *row)
{
    Fixture *f=(Fixture *)opaque;
    assert(row && row->trigger<SC64_Z6_TRIGGER_COUNT);
    if ((int)row->trigger==f->invalid_action_trigger) return 0;
    assert(f->action_count<SC64_Z6_ACTION_TOTAL);
    f->actions[f->action_count].trigger=row->trigger;
    f->actions[f->action_count].index=row->index;
    f->actions[f->action_count].opcode=row->opcode;
    ++f->action_count;
    return 1;
}

static Sc64Z6WorldOps make_ops(Fixture *f)
{ Sc64Z6WorldOps ops={f,condition,action}; return ops; }

static void compare_states(const Sc64Z6RuntimeState *a,
                           const Sc64Z6RuntimeState *b)
{ assert(memcmp(a,b,sizeof(*a))==0); }

static void test_source_records_are_one_shot_and_prune_matches_reference(void)
{
    Sc64Z6RuntimeState fast_state,full_state;
    Fixture fast={.invalid_trigger=-1,.invalid_action_trigger=-1};
    Fixture full={.invalid_trigger=-1,.invalid_action_trigger=-1};
    Sc64Z6WorldOps fast_ops=make_ops(&fast),full_ops=make_ops(&full);
    unsigned skipped=0,preserved_rechecks=0,total_full=0;
    (void)sc64_z6_runtime_init(&fast_state); full_state=fast_state;
    for (unsigned t=0;t<SC64_Z6_TRIGGER_COUNT;++t) {
        int has_preserve=0;
        for (unsigned i=0;i<sc64_z6_action_count((u8)t);++i) {
            const Sc64Z6SourceAction *row;
            assert(sc64_z6_source_action(t,i,&row));
            has_preserve|=row->opcode==3u;
        }
        assert(sc64_z6_trigger_one_shot(t)==(!has_preserve &&
               sc64_z6_source_triggers[t].execution_flags==0));
    }
    for (u32 tick=0;tick<12000u;++tick) {
        u32 before_fast[SC64_Z6_TRIGGER_COUNT],before_full[SC64_Z6_TRIGGER_COUNT];
        u16 started=fast_state.model.started;
        int paused=(tick%113u)==41u;
        memcpy(before_fast,fast.conditions,sizeof(before_fast));
        memcpy(before_full,full.conditions,sizeof(before_full));
        fast.tick=full.tick=tick;
        int a=sc64_z6_adapter_tick(&fast_state,&fast_ops,paused);
        int b=sc64_z6_reference_tick(&full_state,&full_ops,paused);
        assert(a==b); compare_states(&fast_state,&full_state);
        assert(fast.action_count==full.action_count);
        assert(memcmp(fast.actions,full.actions,
                      fast.action_count*sizeof(fast.actions[0]))==0);
        assert(sc64_z6_runtime_state_valid(&fast_state));
        for (unsigned t=0;t<SC64_Z6_TRIGGER_COUNT;++t) {
            u32 fd=fast.conditions[t]-before_fast[t];
            u32 rd=full.conditions[t]-before_full[t];
            if ((started&((u16)1u<<t)) && sc64_z6_trigger_one_shot(t)) {
                assert(fd==0); skipped+=rd;
            } else if ((started&((u16)1u<<t)) && !sc64_z6_trigger_one_shot(t)) {
                assert(fd==rd); preserved_rechecks+=fd;
            }
        }
    }
    assert(skipped>1000u);
    assert(preserved_rechecks==0u); /* Authenticated Z6 has no PreserveTrigger rows. */
    for (unsigned t=0;t<SC64_Z6_TRIGGER_COUNT;++t) total_full+=full.conditions[t];
    printf("Z6 skipped %u of %u condition queries; no PreserveTrigger rows\n",
           skipped,total_full);
}

static void test_terminal_and_invalid_callback_parity(void)
{
    Sc64Z6RuntimeState fast_state,full_state;
    Fixture fast={.invalid_trigger=-1,.invalid_action_trigger=-1};
    Fixture full={.invalid_trigger=-1,.invalid_action_trigger=-1};
    Sc64Z6WorldOps fast_ops=make_ops(&fast),full_ops=make_ops(&full);
    (void)sc64_z6_runtime_init(&fast_state); full_state=fast_state;
    fast.policy=full.policy=1;
    for (u32 tick=0;tick<12000u;++tick) {
        fast.tick=full.tick=tick;
        int a=sc64_z6_adapter_tick(&fast_state,&fast_ops,0);
        int b=sc64_z6_reference_tick(&full_state,&full_ops,0);
        assert(a==b); compare_states(&fast_state,&full_state);
        if (fast_state.model.outcome!=SC64_Z6_RUNNING) break;
    }
    assert(fast_state.model.outcome==SC64_Z6_WON);
    assert(fast.action_count==full.action_count);
    assert(memcmp(fast.actions,full.actions,
                  fast.action_count*sizeof(fast.actions[0]))==0);

    memset(&fast,0,sizeof(fast)); memset(&full,0,sizeof(full));
    fast.invalid_trigger=full.invalid_trigger=4;
    fast.invalid_action_trigger=full.invalid_action_trigger=-1;
    fast_ops=make_ops(&fast); full_ops=make_ops(&full);
    (void)sc64_z6_runtime_init(&fast_state); full_state=fast_state;
    assert(sc64_z6_adapter_tick(&fast_state,&fast_ops,0)<0);
    assert(sc64_z6_reference_tick(&full_state,&full_ops,0)<0);
    compare_states(&fast_state,&full_state);

    memset(&fast,0,sizeof(fast)); memset(&full,0,sizeof(full));
    fast.invalid_action_trigger=full.invalid_action_trigger=2;
    fast.invalid_trigger=full.invalid_trigger=-1;
    fast_ops=make_ops(&fast); full_ops=make_ops(&full);
    (void)sc64_z6_runtime_init(&fast_state); full_state=fast_state;
    fast.policy=full.policy=3;
    int a=sc64_z6_adapter_tick(&fast_state,&fast_ops,0);
    int b=sc64_z6_reference_tick(&full_state,&full_ops,0);
    assert(a==b && a<0); compare_states(&fast_state,&full_state);
}

int main(void)
{
    test_source_records_are_one_shot_and_prune_matches_reference();
    test_terminal_and_invalid_callback_parity();
    return 0;
}
