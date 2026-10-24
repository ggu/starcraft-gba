#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z6_world_adapter.h"

static unsigned hallucination_calls;
static unsigned hallucinated_slot;
static unsigned ai_calls;
static unsigned ai_seen_script, ai_seen_location, ai_seen_owner;
static unsigned energy_calls, energy_percent_seen;

static void mark_hallucinated(void *context, unsigned slot)
{
    (void)context;
    ++hallucination_calls;
    hallucinated_slot=slot;
}

static void set_energy(void *context,unsigned slot,u8 percent)
{
    (void)context;(void)slot;++energy_calls;energy_percent_seen=percent;
}

static int run_ai(void *context, unsigned script, unsigned location,
                  unsigned trigger_player)
{
    (void)context;
    ++ai_calls; ai_seen_script=script; ai_seen_location=location;
    ai_seen_owner=trigger_player;
    return 1;
}

static void init_ctx(Sc64Z6WorldContext *ctx)
{
    memset(ctx,0,sizeof(*ctx));
    ctx->source_current_player=3;
    for(unsigned i=0;i<8;i++) {
        ctx->source_slot_active[i]=1;
        ctx->live_owner_for_source[i]=(s8)(i==3?SIDE:ENEMY);
        ctx->source_force_for_slot[i]=sc64_z6_source_forces[i];
    }
    ctx->mark_hallucinated=mark_hallucinated;
    ctx->set_energy_percent=set_energy;
    ctx->run_ai_script=run_ai;
    race_cols=race_rows=128;
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
}

static int spawn_source_unit(unsigned raw,unsigned owner,int x,int y)
{
    int type=sc64_race_unit_from_raw((int)raw);
    assert(type>=0);
    int slot=spawn_unit(owner==3?SIDE:ENEMY,type,x,y);
    assert(slot>=0);
    u[slot].raw_type=(s16)raw;u[slot].raw_owner=(s16)owner;
    return slot;
}

static const Sc64Z6SourceAction *find_action(unsigned opcode,unsigned unit,
                                              unsigned group2)
{
    for(unsigned i=0;i<SC64_Z6_SOURCE_ACTION_COUNT;i++) {
        const Sc64Z6SourceAction *a=&sc64_z6_source_actions[i];
        if(a->opcode==opcode && (unit==0xffffu || a->unit==unit) &&
           (group2==0xffffffffu || a->group2==group2)) return a;
    }
    return 0;
}

static const Sc64Z6SourceCondition *find_condition(unsigned opcode,
                                                    unsigned unit,
                                                    unsigned resource)
{
    for(unsigned i=0;i<SC64_Z6_SOURCE_CONDITION_COUNT;i++) {
        const Sc64Z6SourceCondition *c=&sc64_z6_source_conditions[i];
        if(c->opcode==opcode && (unit==0xffffu || c->unit==unit) &&
           (resource==0xffffffffu || c->resource==resource)) return c;
    }
    return 0;
}

static void test_locations_owners_and_conditions(void)
{
    Sc64Z6WorldContext ctx;Sc64Z6RuntimeState state;
    const Sc64Z6SourceCondition *c;u32 count=0;
    init_ctx(&ctx);(void)sc64_z6_runtime_init(&state);
    ctx.source_force_for_slot[2]=1; /* Force 2. */
    int l,t,r,btm;
    assert(sc64_z6_world_location(&ctx,1,&l,&t,&r,&btm));
    int slot=spawn_source_unit(79,2,(l+r)/2,(t+btm)/2);
    assert(sc64_z6_world_count(&ctx,2,79,0,&count)&&count==1);
    assert(sc64_z6_world_count(&ctx,19,79,0,&count)&&count==1);
    assert(sc64_z6_world_count(&ctx,13,79,0,&count)&&count==0);
    c=find_condition(2,229,0xffffffffu);assert(c);
    assert(sc64_z6_world_condition(&ctx,&state,c)==1);
    int current=spawn_source_unit(79,3,(l+r)/2,(t+btm)/2);
    assert(sc64_z6_world_count(&ctx,13,79,0,&count)&&count==1);
    assert(sc64_z6_world_condition(&ctx,&state,c)==0);
    assert(sc64_z6_world_count(&ctx,2,79,1,&count)&&count==1);
    c=find_condition(3,229,0xffffffffu);assert(c);
    assert(sc64_z6_world_condition(&ctx,&state,c)>=0);
    c=find_condition(2,229,0xffffffffu);assert(c);
    assert(sc64_z6_world_condition(&ctx,&state,c)>=0);
    c=find_condition(11,0,1);assert(c);
    assert(sc64_z6_world_condition(&ctx,&state,c)==0);
    state.switch_bits[0]|=2u;
    assert(sc64_z6_world_condition(&ctx,&state,c)==1);
    c=find_condition(12,0,0);assert(c);ctx.elapsed_frames=5300u*30u;
    assert(sc64_z6_world_condition(&ctx,&state,c)==1);
    Sc64Z6SourceCondition fake=*c;fake.opcode=99;
    assert(sc64_z6_world_condition(&ctx,&state,&fake)==-1);
    u[slot].x=0;u[slot].y=0;
    assert(sc64_z6_world_count(&ctx,19,79,1,&count)&&count==0);
    assert(u[current].alive);
    u[slot].raw_owner=-1;
    assert(!sc64_z6_world_count(&ctx,19,79,0,&count));
}

static void test_move_location_and_kill_unit_at_location(void)
{
    Sc64Z6WorldContext ctx;const Sc64Z6SourceAction *a;
    int l,t,r,btm;
    init_ctx(&ctx);ctx.source_force_for_slot[1]=0; /* Force 1. */
    assert(sc64_z6_world_location(&ctx,1,&l,&t,&r,&btm));
    int slot=spawn_source_unit(79,1,(l+r)/2,(t+btm)/2);
    a=find_action(38,79,1);assert(a);
    assert(sc64_z6_world_action(&ctx,a));
    assert(ctx.moved_locations_mask&1u);
    int nl,nt,nr,nb;
    assert(sc64_z6_world_location(&ctx,1,&nl,&nt,&nr,&nb));
    assert(u[slot].x>=nl&&u[slot].x<nr&&u[slot].y>=nt&&u[slot].y<nb);

    int other=spawn_source_unit(37,2,(nl+nr)/2,(nt+nb)/2);
    int target=spawn_source_unit(37,1,(nl+nr)/2,(nt+nb)/2);
    a=find_action(23,230,0xffffffffu);assert(a);
    assert(sc64_z6_world_action(&ctx,a));
    assert(u[slot].alive&&!u[other].alive&&u[target].alive);
    assert(!target_lists_valid);
}

static void test_resource_switch_presentation_and_ai(void)
{
    Sc64Z6WorldContext ctx;Sc64Z6RuntimeState state;
    const Sc64Z6SourceAction *a;
    init_ctx(&ctx);(void)sc64_z6_runtime_init(&state);
    ctx.source_force_for_slot[3]=0; /* Force 1 selects source slot 3. */
    a=find_action(26,2,250);assert(a);
    assert(sc64_z6_world_action(&ctx,a));
    assert(minerals==250&&gas==250);
    a=find_action(13,0,1);assert(a);assert(sc64_z6_world_action(&ctx,a));
    a=find_action(7,151,0xffffffffu);assert(a);
    assert(sc64_z6_world_action(&ctx,a)&&ctx.presentation_opcode==7);
    a=find_action(16,0,0xffffffffu);assert(a);
    assert(sc64_z6_world_action(&ctx,a));
    assert(ai_calls==1&&ai_seen_script==a->group2&&ai_seen_location==a->location&&
           ai_seen_owner==ctx.source_current_player);
    ctx.run_ai_script=0;assert(!sc64_z6_world_action(&ctx,a));
    a=find_action(1,0xffffu,0xffffffffu);assert(a);
    assert(sc64_z6_world_action(&ctx,a)&&ctx.victory_requested);
}

static void test_create_properties_fail_closed(void)
{
    Sc64Z6WorldContext ctx;const Sc64Z6SourceAction *a;
    init_ctx(&ctx);ctx.source_force_for_slot[2]=1;
    int l,t,r,btm;assert(sc64_z6_world_location(&ctx,1,&l,&t,&r,&btm));
    /* UPRP 1 is the audited full-stat hallucinated Tassadar record. */
    a=find_action(11,79,1);assert(a);
    ctx.mark_hallucinated=0;
    assert(!sc64_z6_world_action(&ctx,a));
    assert(find_unit_slot()==0);
    ctx.mark_hallucinated=mark_hallucinated;
    assert(sc64_z6_world_action(&ctx,a));
    assert(hallucination_calls==1&&u[hallucinated_slot].raw_type==79);
    assert(energy_calls==1&&energy_percent_seen==100);
    /* UPRP 2 is the bounded full-HP Map Revealer variant. */
    a=find_action(11,101,2);assert(a);
    assert(sc64_z6_world_action(&ctx,a));
    int found=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==101&&u[i].raw_owner==3)found=1;
    assert(found);
    Sc64Z6SourceAction fake=*a;fake.group2=3;assert(!sc64_z6_world_action(&ctx,&fake));
}

static void test_real_source_rows_and_fail_closed(void)
{
    Sc64Z6WorldContext ctx;Sc64Z6RuntimeState state;
    init_ctx(&ctx);(void)sc64_z6_runtime_init(&state);
    for(unsigned i=0;i<SC64_Z6_SOURCE_CONDITION_COUNT;i++)
        assert(sc64_z6_world_condition(&ctx,&state,&sc64_z6_source_conditions[i])>=0);
    for(unsigned i=0;i<SC64_Z6_SOURCE_ACTION_COUNT;i++) {
        const Sc64Z6SourceAction *a=&sc64_z6_source_actions[i];
        /* AI remains a required explicit callback, not an implicit no-op. */
        if(a->opcode==16) { ctx.run_ai_script=0;assert(!sc64_z6_world_action(&ctx,a));ctx.run_ai_script=run_ai; }
        else if(a->opcode==11) {
            if(a->group2==1) ctx.mark_hallucinated=mark_hallucinated;
            assert(sc64_z6_world_action(&ctx,a));
        } else assert(sc64_z6_world_action(&ctx,a));
    }
    assert((ctx.moved_locations_mask&3u)==3u);
    Sc64Z6SourceAction fake=sc64_z6_source_actions[0];fake.opcode=11;
    assert(!sc64_z6_world_action(&ctx,&fake));
}

int main(void)
{
    test_locations_owners_and_conditions();
    test_move_location_and_kill_unit_at_location();
    test_resource_switch_presentation_and_ai();
    test_create_properties_fail_closed();
    test_real_source_rows_and_fail_closed();
    return 0;
}
