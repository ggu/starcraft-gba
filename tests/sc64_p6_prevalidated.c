#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SC64_P6_PREVALIDATED_TICK 1
#include "../starcraft/sc64_p6_adapter.h"

int sc64_p6_full_reference_tick(void *state, const void *ops, int user_paused);

typedef struct {
    u32 tick;
    unsigned policy;
    int bad_condition_trigger;
    int bad_action_trigger;
    unsigned conditions;
    unsigned actions;
    u8 action_trigger[SC64_P6_ACTION_TOTAL];
    u8 action_index[SC64_P6_ACTION_TOTAL];
    u8 action_opcode[SC64_P6_ACTION_TOTAL];
} Fixture;

static int fixture_condition(void *opaque, const Sc64P6RuntimeState *state,
                             const Sc64P6SourceCondition *row)
{
    Fixture *f=(Fixture *)opaque;
    assert(state && row && row->trigger<SC64_P6_TRIGGER_COUNT);
    ++f->conditions;
    if ((int)row->trigger==f->bad_condition_trigger) return -1;
    if (f->policy==1) return row->trigger==SC64_P6_TRIGGER_VICTORY;
    if (f->policy==2) return 0;
    if (f->policy==3) return 1;
    return ((f->tick+row->trigger*7u+row->index*13u)%23u)<17u;
}

static int fixture_action_common(Fixture *f, const Sc64P6SourceAction *row)
{
    assert(row && row->trigger<SC64_P6_TRIGGER_COUNT);
    assert(f->actions<SC64_P6_ACTION_TOTAL);
    f->action_trigger[f->actions]=row->trigger;
    f->action_index[f->actions]=row->index;
    f->action_opcode[f->actions]=row->opcode;
    ++f->actions;
    return (int)row->trigger==f->bad_action_trigger ? 0 : 1;
}

static int fixture_action(void *opaque, const Sc64P6SourceAction *row)
{ return fixture_action_common((Fixture *)opaque,row); }

static int fixture_ai(void *opaque, const Sc64P6SourceAction *row)
{ return fixture_action_common((Fixture *)opaque,row); }

static Sc64P6WorldOps fixture_ops(Fixture *f)
{ Sc64P6WorldOps ops={f,fixture_condition,fixture_action,fixture_ai}; return ops; }

static void assert_equal(const Sc64P6RuntimeState *fast,
                         const Sc64P6RuntimeState *full,
                         const Fixture *a, const Fixture *b)
{
    assert(memcmp(fast,full,sizeof(*fast))==0);
    assert(a->conditions==b->conditions);
    assert(a->actions==b->actions);
    assert(memcmp(a->action_trigger,b->action_trigger,a->actions)==0);
    assert(memcmp(a->action_index,b->action_index,a->actions)==0);
    assert(memcmp(a->action_opcode,b->action_opcode,a->actions)==0);
}

static void test_differential_timeline(void)
{
    Sc64P6RuntimeState fast,full;
    Fixture a={.bad_condition_trigger=-1,.bad_action_trigger=-1};
    Fixture b=a;
    Sc64P6WorldOps aops=fixture_ops(&a), bops=fixture_ops(&b);
    sc64_p6_runtime_init(&fast); full=fast;
    for (u32 tick=0;tick<12000u;++tick) {
        int paused=(tick%101u)==37u;
        a.tick=b.tick=tick;
        a.policy=b.policy=(tick%7u==0)?2u:(tick%5u);
        a.conditions=b.conditions=0;
        a.actions=b.actions=0;
        int x=sc64_p6_adapter_tick_prevalidated(&fast,&aops,paused);
        int y=sc64_p6_full_reference_tick(&full,&bops,paused);
        assert(x==y);
        assert_equal(&fast,&full,&a,&b);
    }
}

static void test_terminal_and_errors(void)
{
    Sc64P6RuntimeState fast,full;
    Fixture a={.bad_condition_trigger=-1,.bad_action_trigger=-1,.policy=1};
    Fixture b=a;
    Sc64P6WorldOps aops=fixture_ops(&a), bops=fixture_ops(&b);
    sc64_p6_runtime_init(&fast); full=fast;
    for (unsigned tick=0;tick<12000u && fast.model.outcome==SC64_P6_RUNNING;++tick) {
        a.tick=b.tick=tick;
        a.conditions=b.conditions=0;
        a.actions=b.actions=0;
        int x=sc64_p6_adapter_tick_prevalidated(&fast,&aops,0);
        int y=sc64_p6_full_reference_tick(&full,&bops,0);
        assert(x==y); assert_equal(&fast,&full,&a,&b);
    }
    assert(fast.model.outcome==SC64_P6_WON);

    memset(&a,0,sizeof(a)); memset(&b,0,sizeof(b));
    a.bad_condition_trigger=b.bad_condition_trigger=4;
    a.bad_action_trigger=b.bad_action_trigger=-1;
    aops=fixture_ops(&a); bops=fixture_ops(&b);
    sc64_p6_runtime_init(&fast); full=fast;
    assert(sc64_p6_adapter_tick_prevalidated(&fast,&aops,0)<0);
    assert(sc64_p6_full_reference_tick(&full,&bops,0)<0);
    assert_equal(&fast,&full,&a,&b);

    memset(&a,0,sizeof(a)); memset(&b,0,sizeof(b));
    a.bad_condition_trigger=b.bad_condition_trigger=-1;
    a.bad_action_trigger=b.bad_action_trigger=2;
    a.policy=b.policy=3;
    aops=fixture_ops(&a); bops=fixture_ops(&b);
    sc64_p6_runtime_init(&fast); full=fast;
    assert(sc64_p6_adapter_tick_prevalidated(&fast,&aops,0)<0);
    assert(sc64_p6_full_reference_tick(&full,&bops,0)<0);
    assert_equal(&fast,&full,&a,&b);
}

int main(void)
{
    assert(sc64_p6_runtime_metadata_valid());
    test_differential_timeline();
    test_terminal_and_errors();
    puts("P6 prevalidated tick matches full-validation reference");
    return 0;
}
