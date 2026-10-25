#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_p6_adapter.h"

enum { MAX_UNITS = 240, MAX_BUILDINGS = 160, SIDE = 0 };
typedef struct {
    int x,y,type,owner,raw_type,raw_owner,alive,selected,target,order;
} Unit;
typedef struct { int x,y,type,owner,raw_type,raw_owner,alive; } Building;
static Unit u[MAX_UNITS];
static Building b[MAX_BUILDINGS];
static u8 invincible_units[MAX_UNITS], invincible_buildings[MAX_BUILDINGS];
static int sc64_race_raw_unit(int type) { return type; }
static int sc64_race_raw_building(int type) { return type; }
#include "../starcraft/sc64_p6_world_adapter.h"

typedef struct {
    unsigned create_calls, ai_calls, door_calls, removed_calls;
    unsigned last_raw, last_owner, last_property_order, last_valid_special;
    unsigned last_door_raw, last_door_location, last_ai_script;
    int last_x,last_y,last_live_owner;
    unsigned capabilities;
    int resolve_enabled, create_enabled, ai_enabled, door_enabled, remove_enabled;
} Fixture;

static int resolve_actor(void *context,unsigned raw,Sc64P6WorldActorBinding *out)
{
    Fixture *f=context;
    if(!f->resolve_enabled||!out)return 0;
    /* This fixture deliberately has no mapping for special P6 actors unless
     * a test explicitly grants one. */
    if(raw!=37&&raw!=38&&raw!=50&&raw!=65&&raw!=66&&raw!=101)return 0;
    out->internal_type=(int)raw;
    out->implemented_capabilities=f->capabilities;
    return 1;
}
static int create_actor(void *context,const Sc64P6WorldCreateRequest *request)
{
    Fixture *f=context;
    if(!f->create_enabled||!request)return 0;
    ++f->create_calls;f->last_raw=request->raw_type;
    f->last_owner=request->source_owner;f->last_x=request->x;f->last_y=request->y;
    f->last_live_owner=request->live_owner;
    f->last_property_order=request->has_properties?
        request->properties.requested_order:0;
    f->last_valid_special=request->has_properties?
        request->properties.valid_special:0;
    return 1;
}
static int is_men(void *context,unsigned raw)
{
    (void)context;
    if(raw>=228)return -1;
    return raw!=101&&raw!=218;
}
static int run_ai(void *context,const Sc64P6SourceAction *action)
{
    Fixture *f=context;
    if(!f->ai_enabled||!action)return 0;
    ++f->ai_calls;f->last_ai_script=action->group2;return 1;
}
static int set_door(void *context,const Sc64P6SourceAction *action)
{
    Fixture *f=context;
    if(!f->door_enabled||!action)return 0;
    ++f->door_calls;f->last_door_raw=action->unit;
    f->last_door_location=action->location;return 1;
}
static int invalidate_actors(void *context)
{
    Fixture *f=context;
    if(!f->remove_enabled)return 0;
    ++f->removed_calls;return 1;
}

static void init_world(Sc64P6WorldContext *ctx,Fixture *f,u32 deaths[12])
{
    memset(ctx,0,sizeof(*ctx));memset(f,0,sizeof(*f));
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
    memset(deaths,0,sizeof(u32)*12u);
    ctx->source_current_player=3;ctx->map_cols=ctx->map_rows=128;
    ctx->deaths_by_source=deaths;ctx->callback_context=f;
    ctx->resolve_actor=resolve_actor;ctx->create_actor=create_actor;
    ctx->is_men_raw=is_men;ctx->run_ai=run_ai;
    ctx->set_doodad_state=set_door;ctx->actors_removed=invalidate_actors;
    for(unsigned i=0;i<8;++i){
        ctx->source_slot_active[i]=1;
        ctx->live_owner_for_source[i]=(s8)(i==3?SIDE:1);
        ctx->source_force_for_slot[i]=sc64_p6_source_forces[i];
    }
}

static const Sc64P6SourceAction *find_action(unsigned opcode,unsigned raw,
                                               unsigned property)
{
    for(unsigned i=0;i<SC64_P6_SOURCE_ACTION_COUNT;++i){
        const Sc64P6SourceAction *a=&sc64_p6_source_actions[i];
        if(a->opcode==opcode&&(raw==0xffffu||a->unit==raw)&&
           (property==0xffffffffu||a->group2==property))return a;
    }
    return 0;
}
static const Sc64P6SourceCondition *find_condition(unsigned opcode,
                                                    unsigned raw,
                                                    unsigned player)
{
    for(unsigned i=0;i<SC64_P6_SOURCE_CONDITION_COUNT;++i){
        const Sc64P6SourceCondition *c=&sc64_p6_source_conditions[i];
        if(c->opcode==opcode&&(raw==0xffffu||c->unit==raw)&&
           (player==0xffffffffu||c->player==player))return c;
    }
    return 0;
}
static int add_actor(int slot,unsigned raw,int source_owner,int live_owner,
                     int x,int y)
{
    if(slot<0||slot>=MAX_UNITS)return 0;
    memset(&u[slot],0,sizeof(u[slot]));u[slot].alive=1;
    u[slot].raw_type=(int)raw;u[slot].type=(int)raw;
    u[slot].raw_owner=source_owner;u[slot].owner=live_owner;
    u[slot].x=x;u[slot].y=y;u[slot].target=-1;return 1;
}

static void test_conditions_owners_locations_and_comparisons(void)
{
    Sc64P6WorldContext ctx;Sc64P6RuntimeState state;Fixture f;u32 deaths[12];
    u32 count=0;int l,t,r,btm;
    init_world(&ctx,&f,deaths);sc64_p6_runtime_init(&state);
    assert(sc64_p6_runtime_metadata_valid());
    assert(sc64_p6_world_location(&ctx,1,&l,&t,&r,&btm));
    assert(add_actor(0,79,3,SIDE,(l+r)/2,(t+btm)/2));
    assert(sc64_p6_world_count(&ctx,13,79,0,&count)&&count==1);
    assert(sc64_p6_world_count(&ctx,3,79,0,&count)&&count==1);
    assert(sc64_p6_world_count(&ctx,2,79,0,&count)&&count==0);
    assert(sc64_p6_world_count(&ctx,13,79,1,&count)&&count==1);
    assert(sc64_p6_count_comparison(1,10,1)==1);
    assert(sc64_p6_count_comparison(0,10,1)==0);
    assert(sc64_p6_count_comparison(0,2,0)==-1);
    assert(!sc64_p6_world_count(&ctx,12,79,0,&count));
    assert(!sc64_p6_world_count(&ctx,13,79,65,&count));

    /* Any Unit counts buildings, Men excludes them, and Bring clips both. */
    b[0]=(Building){(l+r)/2,(t+btm)/2,154,SIDE,154,3,1};
    assert(sc64_p6_world_count(&ctx,13,229,0,&count)&&count==2);
    assert(sc64_p6_world_count(&ctx,13,230,1,&count)&&count==1);
    assert(sc64_p6_world_count(&ctx,13,231,1,&count)&&count==1);

    const Sc64P6SourceCondition *c=find_condition(2,229,13);assert(c);
    assert(sc64_p6_world_condition(&ctx,&state,c)==0); /* count <= zero */
    c=find_condition(3,230,0xffffffffu);assert(c);
    assert(sc64_p6_world_condition(&ctx,&state,c)>=0);
    c=find_condition(11,0,0xffffffffu);assert(c);
    assert(sc64_p6_world_condition(&ctx,&state,c)==1); /* switch is cleared */
    state.switch_bits|=(u32)1u<<c->resource;
    assert(sc64_p6_world_condition(&ctx,&state,c)==0); /* switch is set */
    c=find_condition(12,0,0xffffffffu);assert(c);
    ctx.elapsed_frames=0;
    assert(sc64_p6_world_condition(&ctx,&state,c)==1);
    for(unsigned i=0;i<SC64_P6_SOURCE_CONDITION_COUNT;++i)
        assert(sc64_p6_world_condition(&ctx,&state,
               &sc64_p6_source_conditions[i])>=0);
    Sc64P6SourceCondition malformed=*c;malformed.comparison=10;
    assert(sc64_p6_world_condition(&ctx,&state,&malformed)==-1);
    c=find_condition(22,0,0xffffffffu);assert(c);
    assert(sc64_p6_world_condition(&ctx,&state,c)==1);
    malformed=*c;malformed.amount=1;
    assert(sc64_p6_world_condition(&ctx,&state,&malformed)==-1);
    u[0].raw_owner=-1;
    count=0xabcdef01u;
    assert(!sc64_p6_world_count(&ctx,13,79,0,&count));
    assert(count==0xabcdef01u);
}

static void test_properties_and_create_policy(void)
{
    Sc64P6WorldContext ctx;Fixture f;u32 deaths[12];
    Sc64P6WorldUnitProperties properties;
    init_world(&ctx,&f,deaths);
    for(unsigned i=1;i<=13;++i){
        assert(sc64_p6_world_properties(i,&properties));
        assert(properties.valid_special==0x1a&&properties.valid_fields==2&&
               properties.hp_percent==100&&properties.special_value==2&&
               properties.requested_order==117);
    }
    assert(!sc64_p6_world_properties(0,&properties));
    assert(!sc64_p6_world_properties(14,&properties));

    const Sc64P6SourceAction *a=find_action(11,37,1);assert(a);
    assert(!sc64_p6_world_action(&ctx,a)); /* No mapping callback acceptance. */
    assert(f.create_calls==0);
    f.resolve_enabled=f.create_enabled=1;
    f.capabilities=SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY;
    assert(sc64_p6_world_action(&ctx,a));
    assert(f.create_calls==1&&f.last_raw==37&&f.last_property_order==117&&
           f.last_valid_special==0x1a&&f.last_live_owner==1);

    /* Infested Terran requires both the order property and suicide behavior. */
    a=find_action(11,50,10);assert(a);
    assert(!sc64_p6_world_action(&ctx,a));
    assert(f.create_calls==1);
    f.capabilities=SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY|
                   SC64_P6_POLICY_NEEDS_SUICIDE_EXPLOSION;
    assert(sc64_p6_world_action(&ctx,a));
    assert(f.create_calls==2&&f.last_raw==50);

    /* Unknown special actors stay unresolved; a supplied type map alone is
     * insufficient without the source-required behavior capability. */
    a=find_action(11,101,0);assert(a);
    f.resolve_enabled=0;
    assert(!sc64_p6_world_action(&ctx,a));
    assert(f.create_calls==2);
    f.resolve_enabled=1;
    {
        const unsigned unsupported[]={74,75,210};
        Sc64P6SourceAction probe=*find_action(11,50,10);
        for(unsigned i=0;i<sizeof(unsupported)/sizeof(unsupported[0]);++i){
            probe.unit=(unsigned short)unsupported[i];
            assert(!sc64_p6_world_create(&ctx,&probe));
            assert(f.create_calls==2);
        }
    }
    Sc64P6SourceAction fake=*find_action(11,50,10);
    fake.group2=14;
    assert(!sc64_p6_world_create(&ctx,&fake));
    assert(f.create_calls==2);
}

static void test_doodad_ai_kill_and_outcomes(void)
{
    Sc64P6WorldContext ctx;Fixture f;u32 deaths[12];
    init_world(&ctx,&f,deaths);
    const Sc64P6SourceAction *a=find_action(42,207,0);assert(a);
    assert(!sc64_p6_world_action(&ctx,a));
    assert(f.door_calls==0);
    f.door_enabled=1;assert(sc64_p6_world_action(&ctx,a));
    assert(f.door_calls==1&&f.last_door_raw==207&&f.last_door_location==1);

    a=find_action(15,0xffffu,0xffffffffu);assert(a);
    assert(!sc64_p6_world_action(&ctx,a));
    f.ai_enabled=1;assert(sc64_p6_world_action(&ctx,a));
    assert(f.ai_calls==1&&f.last_ai_script==a->group2);

    a=find_action(22,101,0);assert(a);
    assert(add_actor(0,101,3,SIDE,100,100));u[0].selected=1;u[0].target=9;
    assert(!sc64_p6_world_action(&ctx,a)); /* No cache invalidation callback. */
    assert(u[0].alive&&deaths[3]==0);
    f.remove_enabled=1;assert(sc64_p6_world_action(&ctx,a));
    assert(!u[0].alive&&!u[0].selected&&u[0].target==-1&&deaths[3]==1);
    assert(f.removed_calls==1);

    a=find_action(30,0xffffu,0xffffffffu);assert(a);
    assert(sc64_p6_world_action(&ctx,a)&&ctx.speech_muted);
    a=find_action(31,0xffffu,0xffffffffu);assert(a);
    assert(sc64_p6_world_action(&ctx,a)&&!ctx.speech_muted);
    a=find_action(1,0xffffu,0xffffffffu);assert(a);
    assert(sc64_p6_world_action(&ctx,a)&&ctx.victory_requested);
    a=find_action(2,0xffffu,0xffffffffu);assert(a);
    assert(sc64_p6_world_action(&ctx,a)&&ctx.defeat_requested);
}

static void test_every_authenticated_action_row_dispatches_with_callbacks(void)
{
    Sc64P6WorldContext ctx;Fixture f;u32 deaths[12];
    init_world(&ctx,&f,deaths);
    f.resolve_enabled=f.create_enabled=f.ai_enabled=f.door_enabled=
        f.remove_enabled=1;
    f.capabilities=SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY|
                    SC64_P6_POLICY_NEEDS_SUICIDE_EXPLOSION|
                    SC64_P6_POLICY_NEEDS_PERMANENT_CLOAK_AND_DETECTORS|
                    SC64_P6_POLICY_NEEDS_WALL_TRAP_ART_AND_RUNTIME;
    for(unsigned i=0;i<SC64_P6_SOURCE_ACTION_COUNT;++i)
        assert(sc64_p6_world_action(&ctx,&sc64_p6_source_actions[i]));
    assert(f.create_calls==SC64_P6_CREATE_ACTIONS);
    assert(f.door_calls>0&&f.ai_calls==1);
}

int main(void)
{
    test_conditions_owners_locations_and_comparisons();
    test_properties_and_create_policy();
    test_doodad_ai_kill_and_outcomes();
    test_every_authenticated_action_row_dispatches_with_callbacks();
    return 0;
}
