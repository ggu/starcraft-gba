/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/sc64_z8_world_adapter.h"

typedef struct {
    unsigned owner, count, property, x1, y1, x2, y2;
    u32 ai;
    unsigned ai_owner, ai_location;
    unsigned present_opcode;
    unsigned kills_owner, kills_raw;
    u32 kills;
} FakeEngine;

static int create(void *context, const Sc64Z8Action *a, unsigned owner,
                  unsigned count, unsigned property, unsigned x1, unsigned y1,
                  unsigned x2, unsigned y2)
{
    FakeEngine *e = context;
    assert(a->opcode == 11);
    e->owner=owner; e->count=count; e->property=property;
    e->x1=x1; e->y1=y1; e->x2=x2; e->y2=y2;
    return 1;
}

static int run_ai(void *context, u32 fourcc, unsigned owner, unsigned location,
                  const Sc64Z8Rect *rect)
{
    FakeEngine *e = context;
    assert(rect && rect->left == 32 && rect->top == 32);
    e->ai=fourcc; e->ai_owner=owner; e->ai_location=location;
    return 1;
}

static int present(void *context, const Sc64Z8Action *a, unsigned owner,
                   const Sc64Z8Rect *rect)
{
    FakeEngine *e = context;
    (void)owner; (void)rect;
    e->present_opcode=a->opcode;
    return 1;
}

static int kill_count(void *context, unsigned owner, unsigned raw, u32 *out)
{
    FakeEngine *e = context;
    e->kills_owner=owner; e->kills_raw=raw; *out=e->kills;
    return 1;
}

int main(void)
{
    FakeEngine engine={0};
    u8 invincible0=0, invincible1=0;
    Sc64ResourceActionState wallets[8]={{0}};
    Sc64Z8ActorView actors[3]={
        {.raw_type=154,.source_owner=1,.x=20,.y=20,
         .left=8,.top=8,.right=8,.bottom=8,.alive=1,.completed=1,
         .men=1,.type_known=1,.invincible=&invincible0},
        {.raw_type=154,.source_owner=1,.x=22,.y=20,
         .left=8,.top=8,.right=8,.bottom=8,.alive=1,.completed=0,
         .men=1,.type_known=1,.invincible=&invincible1},
        {.raw_type=171,.source_owner=2,.x=20,.y=20,
         .left=16,.top=16,.right=16,.bottom=16,.alive=1,.completed=1,
         .building=1,.type_known=1}
    };
    Sc64Z8WorldView w={0};
    Sc64Z8Rect rect={48,48,64,64};
    u32 count=0;
    s16 x=0,y=0;
    Sc64Z8Condition c={0};
    Sc64Z8Action a={0};

    w.actors=actors; w.actor_count=3;
    w.active_mask=(u8)((1u<<1)|(1u<<2)|(1u<<3));
    w.force[1]=2; w.force[2]=3; w.force[3]=1;
    w.source_current_player=1;
    for (unsigned i=0;i<8;++i) w.wallet[i]=&wallets[i];
    w.services.context=&engine;
    w.services.create=create; w.services.run_ai=run_ai;
    w.services.present=present; w.services.kill_count=kill_count;

    c.opcode=2; c.player=19; c.unit=154; c.comparison=0; c.amount=1;
    assert(sc64_z8_world_count(&w,&c,3,0,&count)==1 && count==1);
    c.comparison=1; c.amount=2;
    assert(sc64_z8_world_count(&w,&c,3,0,&count)==1 && count==2);
    c.opcode=3; c.player=1; c.comparison=0; c.amount=1;
    assert(sc64_z8_world_count(&w,&c,1,&rect,&count)==1 && count==1);
    rect.left=49; rect.top=48; rect.right=64; rect.bottom=64;
    assert(sc64_z8_world_count(&w,&c,1,&rect,&count)==1 && count==0);

    c.opcode=5; c.player=13; c.unit=154; c.comparison=0; c.amount=1;
    engine.kills=4;
    assert(sc64_z8_world_count(&w,&c,1,0,&count)==1 && count==4);
    assert(engine.kills_owner==1 && engine.kills_raw==154);

    rect=(Sc64Z8Rect){32,32,80,80};
    a.opcode=38; a.unit=154; a.group1=19;
    assert(sc64_z8_world_find_unit(&w,&a,3,&rect,&x,&y)==1);
    assert(x==40 && y==40); /* First matching actor in caller's actor order. */

    a=(Sc64Z8Action){.opcode=11,.location=1,.group1=19,.group2=4,
                     .unit=74,.modifier=0};
    assert(sc64_z8_world_action(&w,&a,3,&rect)==1);
    assert(engine.owner==1 && engine.count==1 && engine.property==4);
    assert(engine.x1==32 && engine.y1==32 && engine.x2==80 && engine.y2==80);

    a=(Sc64Z8Action){.opcode=43,.location=1,.group1=19,.unit=154,.modifier=4};
    actors[1].invincible=0;
    assert(sc64_z8_world_action(&w,&a,3,&rect)==-1);
    assert(invincible0==0); /* Invalid late entry does not partially mutate. */
    actors[1].invincible=&invincible1;
    assert(sc64_z8_world_action(&w,&a,3,&rect)==1);
    assert(invincible0==1 && invincible1==1);
    a.modifier=5;
    assert(sc64_z8_world_action(&w,&a,3,&rect)==1);
    assert(invincible0==0 && invincible1==0);

    a=(Sc64Z8Action){.opcode=26,.group1=19,.group2=350,
                     .unit=SC64_RESOURCE_BOTH,.modifier=SC64_RESOURCE_MOD_SET};
    assert(sc64_z8_world_action(&w,&a,3,0)==1);
    assert(wallets[1].current_minerals==350 && wallets[1].current_gas==350);
    assert(wallets[1].total_minerals_gathered==350);

    a=(Sc64Z8Action){.opcode=16,.location=4,.group2=SC64_Z8_AI_ZER8};
    w.services.run_ai=0;
    assert(sc64_z8_world_action(&w,&a,3,&rect)==-1); /* No invented AI. */
    w.services.run_ai=run_ai;
    assert(sc64_z8_world_action(&w,&a,3,&rect)==1);
    assert(engine.ai==SC64_Z8_AI_ZER8 && engine.ai_owner==3 && engine.ai_location==4);

    a=(Sc64Z8Action){.opcode=9,.location=1};
    assert(sc64_z8_world_action(&w,&a,3,&rect)==1 && engine.present_opcode==9);
    assert(sc64_z8_world_ops.count && sc64_z8_world_ops.action &&
           sc64_z8_world_ops.timed && sc64_z8_world_ops.find_unit);

    actors[2].type_known=0;
    c=(Sc64Z8Condition){.opcode=2,.player=17,.unit=229,.comparison=0};
    assert(sc64_z8_world_count(&w,&c,1,0,&count)==-1); /* Unknown relevant actor. */
    puts("sc64_z8_world: ok (live typed view, exact selectors, fail-closed services)");
    return 0;
}
