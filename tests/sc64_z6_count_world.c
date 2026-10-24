/* Compare the streaming cache against the real Z6 world_count implementation
 * over randomized engine Unit/Building arrays. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z6_count_snapshot.h"

static u32 rng = 0x73a0c4d1u;
static u32 rnd(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static void init_ctx(Sc64Z6WorldContext *ctx,
                     Sc64Z6CountRect rects[7], u8 active[12], u8 forces[8])
{
    memset(ctx,0,sizeof(*ctx));
    memset(active,0,12);
    for (unsigned i=0;i<8;++i) {
        active[i]=1;
        forces[i]=(u8)sc64_z6_source_forces[i];
        ctx->source_slot_active[i]=1;
        ctx->source_force_for_slot[i]=forces[i];
    }
    ctx->source_current_player=3;
    race_cols=race_rows=128;
    memset(rects,0,sizeof(Sc64Z6CountRect)*7u);
    for (unsigned id=1;id<=6;++id) {
        int l,t,r,bottom;
        assert(sc64_z6_world_location(ctx,id,&l,&t,&r,&bottom));
        rects[id]=(Sc64Z6CountRect){l,t,r,bottom,1};
    }
}

static int executor_for_test(unsigned trigger,const u8 active[12],
                             const u8 forces[8],unsigned *out)
{
    unsigned found=12;
    for (unsigned p=0;p<27;++p) if(sc64_z6_trigger_enabled(trigger,p)) {
        if(p<8) {
            if(!active[p]||found!=12)return 0;
            found=p;
        } else if(p>=18&&p<=21) {
            for(unsigned s=0;s<8;++s)if(active[s]&&forces[s]==p-18) {
                if(found!=12)return 0;found=s;
            }
            if(found==12)return 0;
        } else return 0;
    }
    if(found>=12)return 0;
    *out=found;return 1;
}

static int pick_raw_unit(void)
{
    static const int raws[]={37,38,39,43,44,50,51,64,65,67,74,75,79,83,84,101,218,222};
    return raws[rnd()%(sizeof(raws)/sizeof(raws[0]))];
}

static int pick_raw_building(void)
{
    static const int raws[]={106,109,110,131,132,135,142,143,149,154,156,162,194,195,196,222};
    return raws[rnd()%(sizeof(raws)/sizeof(raws[0]))];
}

static void seed_engine_banks(void)
{
    memset(u,0,sizeof(u));
    memset(b,0,sizeof(b));
    for(unsigned i=0;i<MAX_UNITS;++i) {
        Unit *q=&u[i];
        if((rnd()%4u)==0)continue;
        int raw=pick_raw_unit();
        q->alive=1;
        q->raw_type=(s16)raw;
        q->raw_owner=(s16)(rnd()%8u);
        q->owner=q->raw_owner==3?SIDE:ENEMY;
        q->x=(int)(rnd()%2049u);q->y=(int)(rnd()%2049u);
        int mapped=sc64_race_unit_from_raw(raw);
        q->type=mapped>=0?mapped:0;
        if((rnd()%5u)==0 && mapped>=0) q->raw_type=-1;
    }
    for(unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building *q=&b[i];
        if((rnd()%4u)==0)continue;
        int raw=pick_raw_building();
        q->alive=1;
        q->raw_type=(s16)raw;
        q->raw_owner=(s16)(rnd()%8u);
        q->owner=q->raw_owner==3?SIDE:ENEMY;
        q->x=(int)(rnd()%2049u);q->y=(int)(rnd()%2049u);
        int mapped=sc64_race_building_from_raw(raw);
        q->type=mapped>=0?mapped:0;
        if((rnd()%5u)==0 && mapped>=0) q->raw_type=-1;
    }
    /* Positive but unmapped raw IDs still count as AnyUnit in production. */
    u[0].alive=1;u[0].raw_type=222;u[0].raw_owner=3;u[0].owner=SIDE;
    u[0].x=120;u[0].y=120;u[0].type=0;
}

static void test_real_world_differential(void)
{
    Sc64Z6WorldContext ctx;
    Sc64Z6CountRect rects[7];
    u8 active[12],forces[8];
    Sc64Z6CountSnapshotBuilder builder;
    Sc64Z6CountSnapshot snapshot;
    Sc64Z6CountActor actor;
    u32 expected;
    init_ctx(&ctx,rects,active,forces);
    for(unsigned trial=0;trial<100;++trial) {
        seed_engine_banks();
#ifdef SC64_Z6_COUNT_SNAPSHOT
        Sc64Z6CountSnapshot integrated;
        assert(race_z6_count_snapshot_build(&ctx,&integrated));
#endif
        assert(sc64_z6_count_snapshot_begin(&snapshot,&builder,active,forces,rects,7));
        for(unsigned i=0;i<MAX_UNITS;++i) {
            int raw=u[i].raw_type>=0?u[i].raw_type:sc64_race_raw_unit(u[i].type);
            actor=(Sc64Z6CountActor){u[i].x,u[i].y,(s16)raw,u[i].raw_owner,
                u[i].alive,(u8)(raw>=0&&sc64_race_unit_from_raw(raw)>=0)};
            assert(sc64_z6_count_snapshot_add_actor(&builder,&actor,0,rects));
        }
        for(unsigned i=0;i<MAX_BUILDINGS;++i) {
            int raw=b[i].raw_type>=0?b[i].raw_type:sc64_race_raw_building(b[i].type);
            actor=(Sc64Z6CountActor){b[i].x,b[i].y,(s16)raw,b[i].raw_owner,
                b[i].alive,(u8)(raw>=0&&sc64_race_building_from_raw(raw)>=0)};
            assert(sc64_z6_count_snapshot_add_actor(&builder,&actor,1,rects));
        }
        assert(sc64_z6_count_snapshot_finish(&builder,&snapshot));
        for(unsigned qi=0;qi<SC64_Z6_COUNT_QUERY_COUNT;++qi) {
            const Sc64Z6CountQueryKey *key=&sc64_z6_count_query_keys[qi];
            const Sc64Z6SourceCondition *condition;
            unsigned executor;
            u32 actual=0;
            assert(sc64_z6_condition_row(key->trigger,key->index,&condition));
            assert(executor_for_test(key->trigger,active,forces,&executor));
            ctx.source_current_player=(u8)executor;
            assert(sc64_z6_world_count(&ctx,key->selector,key->raw,
                                       key->location,&expected));
            assert(sc64_z6_count_snapshot_lookup(&snapshot,condition,&actual));
            assert(actual==expected);
#ifdef SC64_Z6_COUNT_SNAPSHOT
            assert(sc64_z6_count_snapshot_lookup(&integrated,condition,&actual));
            assert(actual==expected);
#endif
        }
    }
}

static void test_invalid_owner_matches_oracle_failure(void)
{
    Sc64Z6WorldContext ctx;
    Sc64Z6CountRect rects[7];
    u8 active[12],forces[8];
    Sc64Z6CountSnapshotBuilder builder;
    Sc64Z6CountSnapshot snapshot;
    Sc64Z6CountActor actor={120,120,222,-1,1,0};
    u32 count;
    init_ctx(&ctx,rects,active,forces);
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    u[0].alive=1;u[0].raw_type=222;u[0].raw_owner=-1;
    assert(!sc64_z6_world_count(&ctx,13,229,0,&count));
#ifdef SC64_Z6_COUNT_SNAPSHOT
    assert(!race_z6_count_snapshot_build(&ctx,&snapshot));
    assert(!snapshot.active);
#endif
    assert(sc64_z6_count_snapshot_begin(&snapshot,&builder,active,forces,rects,7));
    assert(!sc64_z6_count_snapshot_add_actor(&builder,&actor,0,rects));
    assert(!sc64_z6_count_snapshot_finish(&builder,&snapshot));
}

#ifdef SC64_Z6_COUNT_SNAPSHOT
static void test_integrated_action_invalidation(void)
{
    Sc64Z6WorldContext world;
    Sc64Z6CountRect rects[7];
    u8 active[12], forces[8];
    Sc64Z6CountSnapshot snapshot;
    Sc64Z6CachedOpsContext cached={&world,&snapshot};
    Sc64Z6RuntimeState state={0};
    const Sc64Z6SourceCondition *condition;
    init_ctx(&world,rects,active,forces);
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    u[0].alive=1; u[0].raw_type=37; u[0].raw_owner=3; u[0].type=U_ZERGLING;
    assert(sc64_z6_condition_row(0,0,&condition));
    assert(race_z6_count_snapshot_build(&world,&snapshot));
    assert(race_z6_count_cached_condition(&cached,&state,condition)==0);
    /* Simulate a mutation outside the completed condition phase, then
       verify even failed action dispatch discards the old phase cache. */
    u[0].alive=0;
    assert(race_z6_count_cached_action(&cached,0)==0);
    assert(!snapshot.active);
    assert(race_z6_count_cached_condition(&cached,&state,condition)==1);
    assert(world.source_current_player==3);
}
#endif

int main(void)
{
    game_init();
    test_real_world_differential();
    test_invalid_owner_matches_oracle_failure();
#ifdef SC64_Z6_COUNT_SNAPSHOT
    test_integrated_action_invalidation();
#endif
    puts("Z6 real-world count snapshot differential passed");
    return 0;
}
