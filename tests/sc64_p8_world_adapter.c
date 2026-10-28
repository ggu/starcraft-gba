/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>

#include "../starcraft/sc64_p8_world_adapter.h"

typedef struct {
    Sc64Z8ActorView *actors;
    unsigned created_property, created_owner, created_count;
    u32 ai_id;
    unsigned ai_owner;
    unsigned presented_opcode;
    int create_fail;
    u8 *invincibility;
    Sc64ResourceActionState wallet;
    u32 wait_active;
    unsigned wait_starts, wait_resumes, wait_blocked;
} FakeP8Engine;

static int create(void *opaque, const Sc64Z8Action *action, unsigned owner,
                  unsigned count, unsigned property, unsigned x1, unsigned y1,
                  unsigned x2, unsigned y2)
{
    FakeP8Engine *e = opaque;
    assert(action->opcode == 11 && x1 <= x2 && y1 <= y2);
    e->created_property = property;
    e->created_owner = owner;
    e->created_count = count;
    if (e->create_fail) return -1;
    e->actors[3] = (Sc64Z8ActorView){.raw_type=action->unit,
        .source_owner=(s16)owner,.x=(s16)((x1+x2)/4),.y=(s16)((y1+y2)/4),
        .alive=1,.completed=1,.men=1,.type_known=1};
    return 1;
}

static int actor_at(void *opaque, unsigned index, Sc64Z8ActorView *out)
{
    Sc64Z8ActorView *actors=opaque;
    if (!out || index >= 4) return -1;
    *out=actors[index];
    return 1;
}

static int present(void *opaque, const Sc64Z8Action *action, unsigned owner,
                   const Sc64Z8Rect *rect)
{
    FakeP8Engine *e = opaque;
    (void)owner; (void)rect;
    e->presented_opcode = action->opcode;
    return 1;
}

static int p8_ai(void *opaque, const Sc64P8SourceAction *action,
                 unsigned owner, const Sc64Z8Rect *rect)
{
    FakeP8Engine *e = opaque;
    assert(rect);
    e->ai_id = action->group2;
    e->ai_owner = owner;
    return 1;
}

static int async_action(void *opaque, const Sc64P8SourceAction *action,
                       unsigned owner, const Sc64Z8Rect *rect, int resume)
{
    FakeP8Engine *e = opaque;
    (void)action; (void)rect;
    if (resume) {
        ++e->wait_resumes;
        assert(e->wait_active == owner + 1u);
        e->wait_active = 0;
        return SC64_P8_ACTION_COMPLETE;
    }
    if (e->wait_active) {
        ++e->wait_blocked;
        return SC64_P8_ACTION_BLOCKED;
    }
    ++e->wait_starts;
    e->wait_active = owner + 1u;
    return SC64_P8_ACTION_PENDING;
}

static Sc64P8SourceCondition condition(unsigned opcode, unsigned selector,
                                       unsigned unit, unsigned comparison,
                                       unsigned amount, unsigned location,
                                       unsigned resource)
{
    Sc64P8SourceCondition c = {0};
    c.opcode=(u8)opcode; c.player=selector; c.unit=(u16)unit;
    c.comparison=(u8)comparison; c.amount=amount;
    c.location=location; c.resource=(u8)resource;
    return c;
}

static Sc64P8SourceAction action(unsigned opcode, unsigned location,
                                 unsigned group1, unsigned group2,
                                 unsigned unit, unsigned modifier)
{
    Sc64P8SourceAction a = {0};
    a.opcode=(u8)opcode; a.location=location;
    a.group1=group1; a.group2=group2; a.unit=(u16)unit;
    a.modifier=(u8)modifier;
    return a;
}

int main(void)
{
    FakeP8Engine engine={0};
    u8 inv0=0, inv1=0;
    Sc64Z8ActorView actors[4]={
        {.raw_type=154,.source_owner=1,.x=25,.y=25,.left=8,.top=8,
         .right=8,.bottom=8,.alive=1,.completed=1,.men=1,.type_known=1,
         .invincible=&inv0},
        {.raw_type=154,.source_owner=1,.x=50,.y=25,.left=8,.top=8,
         .right=8,.bottom=8,.alive=1,.completed=0,.men=1,.type_known=1,
         .invincible=&inv1},
        {.raw_type=154,.source_owner=2,.x=25,.y=25,.left=8,.top=8,
         .right=8,.bottom=8,.alive=1,.completed=1,.men=1,.loaded=1,
         .type_known=1},
        {0}
    };
    Sc64Z8WorldView z8={0};
    z8.actors=actors; z8.actor_count=4;
    z8.active_mask=(u8)((1u<<1)|(1u<<2));
    z8.force[1]=2; z8.force[2]=3;
    z8.wallet[1]=&engine.wallet;
    z8.services.context=&engine; z8.services.create=create;
    z8.services.present=present;
    engine.actors=actors;

    Sc64Z8Rect locations[64]={
        {40,40,112,80}, {200,200,220,220}, {58,40,80,80},
        {59,40,80,80}
    };
    u8 switches[32]={0};
    u32 elapsed=42;
    Sc64P8WorldContext context={
        .actors=&z8,.locations=locations,.location_count=64,
        .map_pixel_width=4096,.map_pixel_height=4096,
        .switches=switches,.elapsed_seconds=&elapsed,.context=&engine,
        .async_action=async_action
    };
    assert(sc64_p8_world_context_valid(&context));
    Sc64P8SourceWorldOps ops=sc64_p8_world_ops(&context);

    /* Command honors exact owner selection and completion comparison. */
    Sc64P8SourceCondition c=condition(2,1,154,0,1,0,0);
    assert(ops.condition(ops.context,&c,1)==1); /* complete actor only */
    c=condition(2,19,154,0,1,0,0); /* Force 2 maps to source owner 1. */
    assert(ops.condition(ops.context,&c,1)==1);
    c=condition(2,20,154,0,1,0,0); /* Force 3 sees cargo in Command. */
    assert(ops.condition(ops.context,&c,1)==1);
    c=condition(2,1,154,0,2,0,0);
    assert(ops.condition(ops.context,&c,1)==0);
    c.flags=0x20; /* unsupported condition flags are not ignored */
    assert(ops.condition(ops.context,&c,1)==-1);
    c.flags=0;
    assert(ops.condition(ops.context,&c,SC64_Z8_OWNER_LIMIT)==-1);
    c=condition(2,1,154,1,2,0,0);
    assert(ops.condition(ops.context,&c,1)==1); /* AtMost includes incomplete */

    /* Bring uses source-pixel half-open overlap and excludes cargo. */
    c=condition(3,1,154,0,1,1,0);
    assert(ops.condition(ops.context,&c,1)==1);
    c=condition(3,17,154,0,2,1,0);
    assert(ops.condition(ops.context,&c,1)==0);
    c=condition(3,1,154,0,1,3,0); /* edge starts before actor's right edge */
    assert(ops.condition(ops.context,&c,1)==1);
    c.location=4; /* half-open boundary touch does not overlap */
    assert(ops.condition(ops.context,&c,1)==0);
    c.location=0;
    assert(ops.condition(ops.context,&c,1)==0);
    c.location=65;
    assert(ops.condition(ops.context,&c,1)==-1);

    /* Switch and elapsed predicates share the caller's live state. */
    c=condition(11,1,0,2,0,0,9);
    assert(ops.condition(ops.context,&c,1)==0);
    Sc64P8SourceAction a=action(13,0,0,9,0,4);
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_COMPLETE);
    assert(ops.condition(ops.context,&c,1)==1);
    c.comparison=3;
    assert(ops.condition(ops.context,&c,1)==0);
    c=condition(12,1,0,0,42,0,0);
    assert(ops.condition(ops.context,&c,1)==1);
    c.amount=43;
    assert(ops.condition(ops.context,&c,1)==0);
    context.elapsed_seconds=0;
    assert(ops.condition(ops.context,&c,1)==-1);
    context.elapsed_seconds=&elapsed;

    /* MoveLocation reuses the Z8 native-backed ordered anchor and clamp. */
    a=action(38,1,1,2,154,0);
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_COMPLETE);
    assert(locations[1].left==40 && locations[1].top==40 &&
           locations[1].right==60 && locations[1].bottom==60);

    /* Create forwards the source property selector and sees the new slot on
     * the next census; it does not assume default properties. */
    a=action(11,1,1,3,74,0);
    z8.actors=0; z8.actor_at=actor_at; z8.actor_context=actors;
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_COMPLETE);
    assert(engine.created_owner==1 && engine.created_count==1 &&
           engine.created_property==3 && actors[3].alive);
    c=condition(2,1,74,0,1,0,0);
    assert(ops.condition(ops.context,&c,1)==1);
    engine.create_fail=1;
    a=action(11,1,1,2,74,0);
    assert(ops.action(ops.context,&a,1,0)==-1); /* exhausted pool/service */
    engine.create_fail=0;

    /* Invincibility validates the complete target set before writing. */
    actors[1].invincible=0;
    a=action(43,1,1,0,154,4);
    assert(ops.action(ops.context,&a,1,0)==-1 && inv0==0);
    actors[1].invincible=&inv1;
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_COMPLETE);
    assert(inv0==1 && inv1==1);

    /* Economy and presentation use existing explicit world services. */
    a=action(26,0,1,321,SC64_RESOURCE_MINERALS,
             SC64_RESOURCE_MOD_SET);
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_COMPLETE);
    assert(engine.wallet.current_minerals==321);
    a=action(9,0,0,0,0,0);
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_COMPLETE);
    assert(engine.presented_opcode==9);

    /* P8 AI stays opaque and fail-closed; it is never mapped to a Z8 script. */
    a=action(16,1,0,0x12345678u,0,0);
    assert(ops.action(ops.context,&a,1,0)==-1);
    context.run_ai=p8_ai;
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_COMPLETE);
    assert(engine.ai_id==0x12345678u && engine.ai_owner==1);

    /* The caller supplies the single owner wait policy and per-action armed
     * marker. BLOCKED is not-started and PENDING resumes the same action. */
    a=action(4,0,0,0,0,0);
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_PENDING);

    /* Missing actor facts fail closed, while disabled rows are pure skips. */
    actors[0].type_known=0;
    c=condition(2,17,154,0,1,0,0);
    assert(ops.condition(ops.context,&c,1)==-1);
    c.flags=2;
    assert(ops.condition(0,&c,1)==1);
    actors[0].type_known=1;
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_BLOCKED);
    assert(ops.action(ops.context,&a,1,1)==SC64_P8_ACTION_COMPLETE);
    assert(engine.wait_starts==1 && engine.wait_blocked==1 &&
           engine.wait_resumes==1);
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_PENDING);

    a=action(1,0,0,0,0,0);
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_WON);
    a=action(2,0,0,0,0,0);
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_LOST);
    a=action(11,0,1,3,74,0); /* native location-zero no-op */
    engine.created_count=0;
    assert(ops.action(ops.context,&a,1,0)==SC64_P8_ACTION_COMPLETE);
    assert(engine.created_count==0);
    a=action(13,0,0,10,0,4);
    assert(ops.action(ops.context,&a,1,1)==-1); /* bad resume cannot mutate */
    assert((switches[1] & 4u)==0);
    a=action(4,0,0,0,0,0);
    a.trailing[0]=1; /* no lossy conversion through the Z8 action shape */
    assert(ops.action(ops.context,&a,1,0)==-1);
    assert(engine.wait_starts==2);
    a=action(3,1,0,0,0,0);
    assert(ops.action(ops.context,&a,1,0)==-1); /* repeat not implemented */

    puts("sc64_p8_world_adapter: ok (typed conditions/effects, fail-closed services)");
    return 0;
}
