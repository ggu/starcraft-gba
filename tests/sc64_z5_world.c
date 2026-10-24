#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z5_world_adapter.h"
#include "sc64_z5_doodads_generated.h"

static void init_world(Sc64Z5WorldContext *ctx)
{
    memset(ctx,0,sizeof(*ctx));
    ctx->source_current_player=3;
    for(unsigned i=0;i<8;i++) {
        ctx->source_slot_active[i]=1;
        ctx->live_owner_for_source[i]=(s8)(i==3?SIDE:ENEMY);
    }
}

static void test_conditions(void)
{
    Sc64Z5WorldContext ctx; Sc64Z5RuntimeState state;
    Sc64Z5SourceCondition c={0};
    init_world(&ctx); (void)sc64_z5_runtime_init(&state);
    race_cols=race_rows=128;
    int left,top,right,bottom;
    assert(sc64_z5_world_location(3,&left,&top,&right,&bottom));
    int slot=spawn_unit(SIDE,sc64_race_unit_from_raw(51),(left+right)/2,(top+bottom)/2);
    assert(slot>=0);u[slot].raw_type=51;u[slot].raw_owner=3;
    c.player=13;c.amount=1;c.unit=51;c.comparison=0;c.opcode=2;c.flags=0x10;
    assert(sc64_z5_world_condition(&ctx,&state,&c)==1);
    c.amount=2;assert(sc64_z5_world_condition(&ctx,&state,&c)==0);
    c.comparison=2;c.amount=1;assert(sc64_z5_world_condition(&ctx,&state,&c)==1);
    c.opcode=3;c.comparison=0;c.amount=1;c.location=3;
    assert(sc64_z5_world_condition(&ctx,&state,&c)==1);
    u[slot].x=30;u[slot].y=30;
    assert(sc64_z5_world_condition(&ctx,&state,&c)==0);
    c.opcode=12;c.amount=0;c.comparison=0;c.flags=0;
    assert(sc64_z5_world_condition(&ctx,&state,&c)==1);
    c.opcode=11;c.resource=14;c.comparison=2;
    assert(sc64_z5_world_condition(&ctx,&state,&c)==0);
    state.switch_bits[0]|=1u<<14;
    assert(sc64_z5_world_condition(&ctx,&state,&c)==1);
    c.opcode=22;c.flags=0;
    assert(sc64_z5_world_condition(&ctx,&state,&c)==1);
    c.opcode=99;assert(sc64_z5_world_condition(&ctx,&state,&c)==-1);
    c.opcode=43;assert(sc64_z5_world_condition(&ctx,&state,&c)==-1);
}

static void test_effects_and_pause_request(void)
{
    Sc64Z5WorldContext ctx; Sc64Z5RuntimeState state;
    Sc64Z5SourceAction a={0};
    init_world(&ctx); (void)sc64_z5_runtime_init(&state);
    race_cols=race_rows=128;
    int sl,st,sr,sb;
    assert(sc64_z5_world_location(3,&sl,&st,&sr,&sb));
    int slot=spawn_unit(SIDE,sc64_race_unit_from_raw(51),(sl+sr)/2,(st+sb)/2);
    assert(slot>=0);u[slot].raw_type=51;u[slot].raw_owner=3;
    a.opcode=39;a.location=3;a.group1=13;a.group2=6;a.unit=230;
    assert(u[slot].x>=sl && u[slot].x<sr && u[slot].y>=st && u[slot].y<sb);
    assert(sc64_z5_world_owner_matches(&ctx,13,3)==1);
    assert(sc64_z5_world_type_match(230,51,0));
    assert(sc64_z5_world_action(&ctx,&a)==1);
    int dl,dt,dr,db;
    assert(sc64_z5_world_location(6,&dl,&dt,&dr,&db));
    assert(u[slot].x==(dl+dr)/2 && u[slot].y==(dt+db)/2);

    a=(Sc64Z5SourceAction){0};a.opcode=5;
    assert(sc64_z5_world_action(&ctx,&a)==1 && ctx.presentation_pause_requested==1);
    assert(sc64_z5_world_pause(&ctx,0)==1 && !ctx.presentation_pause_requested);

    a=(Sc64Z5SourceAction){0};a.opcode=11;a.location=3;a.group1=13;a.unit=101;
    int before=find_unit_slot();
    assert(sc64_z5_world_action(&ctx,&a)==1);
    assert(u[before].alive && u[before].raw_type==101 && u[before].raw_owner==3);

    a=(Sc64Z5SourceAction){0};a.opcode=25;a.location=25;a.group1=13;a.unit=218;
    assert(sc64_z5_world_action(&ctx,&a)==1);
    a.opcode=43; /* CHK SetInvincibility is unsupported, not CreateUnit. */
    assert(sc64_z5_world_action(&ctx,&a)==0);
    a.opcode=99;assert(sc64_z5_world_action(&ctx,&a)==0);
}

static void test_doodad_state(void)
{
    Sc64Z5WorldContext ctx;
    Sc64Z5DoodadState doodads[SC64_Z5_DOODAD_CAPACITY]={{0}};
    Sc64Z5SourceAction action={0};
    init_world(&ctx);ctx.doodads=doodads;
    ctx.doodad_count=SC64_Z5_DOODAD_CAPACITY;
    const Sc64Z5SourceLocation *loc=&sc64_z5_source_locations[20];
    doodads[0].raw_type=205;doodads[0].source_owner=0;
    doodads[0].x=(u16)loc->left;doodads[0].y=(u16)loc->top;
    doodads[0].enabled=1;
    action.opcode=42;action.location=21;action.group1=17;action.unit=205;action.modifier=0;
    assert(sc64_z5_world_action(&ctx,&action)==1 && doodads[0].enabled==0);
    doodads[0].raw_flags=0x1000;
    assert(sc64_z5_world_action(&ctx,&action)==0);
}

static void test_all_active_source_rows(void)
{
    Sc64Z5WorldContext ctx; Sc64Z5RuntimeState state;
    Sc64Z5DoodadState doodads[SC64_Z5_DOODAD_CAPACITY];
    unsigned doodad_count=0;
    init_world(&ctx);(void)sc64_z5_runtime_init(&state);
    race_cols=race_rows=128;
    memset(doodads,0,sizeof(doodads));
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    ctx.doodads=doodads;
    for(unsigned i=0;i<SC64_Z5_DOODAD_COUNT;i++) {
        const Sc64Z5SourceDoodad *source=&sc64_z5_source_doodads[i];
        if(!source->unit_backed) continue;
        assert(doodad_count<SC64_Z5_DOODAD_CAPACITY);
        doodads[doodad_count].raw_type=source->type;
        doodads[doodad_count].source_owner=source->owner;
        doodads[doodad_count].x=source->x;
        doodads[doodad_count].y=source->y;
        doodads[doodad_count].raw_flags=source->flags;
        doodads[doodad_count].enabled=source->initial_enabled;
        ++doodad_count;
    }
    assert(doodad_count==SC64_Z5_DOODAD_CAPACITY);
    ctx.doodad_count=(u8)doodad_count;
    /* Run every active source condition against a valid empty initial world. */
    for(unsigned i=0;i<SC64_Z5_SOURCE_CONDITION_COUNT;i++) {
        const Sc64Z5SourceCondition *c=&sc64_z5_source_conditions[i];
        if(!c->opcode) continue;
        assert(sc64_z5_world_condition(&ctx,&state,c)>=0);
    }
    /* Dispatch each nonempty numeric action record once; this checks every
     * authored opcode/operand path without embedding source text or assets. */
    for(unsigned i=0;i<SC64_Z5_SOURCE_ACTION_COUNT;i++) {
        const Sc64Z5SourceAction *a=&sc64_z5_source_actions[i];
        if(!a->opcode) continue;
        assert(sc64_z5_world_action(&ctx,a)==1);
    }
}

static void test_source_actor_query_coverage(void)
{
    static const unsigned unit_raws[]={0,1,2,3,7,15,32,37,51,53,89,90,95,101,218};
    int l,t,r,bounds;
    for(unsigned i=0;i<sizeof(unit_raws)/sizeof(unit_raws[0]);i++)
        assert(sc64_z5_world_actor_rect(unit_raws[i],100,100,&l,&t,&r,&bounds));
    assert(sc64_z5_world_building_bounds(195,&l,&t,&r,&bounds));
}

int main(void)
{
    test_conditions();
    test_effects_and_pause_request();
    test_doodad_state();
    test_all_active_source_rows();
    test_source_actor_query_coverage();
    return 0;
}
