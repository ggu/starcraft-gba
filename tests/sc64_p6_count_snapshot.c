#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_p6_adapter.h"

enum { MAX_UNITS = 240, MAX_BUILDINGS = 160, SIDE = 0, RESCUABLE = 3,
       ENEMY = 1, NEUTRAL = 2, WORLD_W = 2048, WORLD_H = 2048 };
typedef struct {
    int x,y,type,owner,raw_type,raw_owner,alive,selected,target,order;
} Unit;
typedef struct { int x,y,type,owner,raw_type,raw_owner,alive; } Building;
Unit u[MAX_UNITS];
Building b[MAX_BUILDINGS];
u8 invincible_units[MAX_UNITS], invincible_buildings[MAX_BUILDINGS];
static int men_error_raw = -1;
int sc64_race_raw_unit(int type) { return type; }
int sc64_race_raw_building(int type) { return type; }
#include "../starcraft/sc64_p6_world_adapter.h"
#include "../starcraft/sc64_p6_count_snapshot.h"

int sc64_p6_reference_count(const Sc64P6WorldContext *, unsigned, unsigned,
                            unsigned, u32 *);
int sc64_p6_reference_tick(Sc64P6RuntimeState *, Sc64P6WorldContext *, int,
                           u8 *, unsigned *);

static unsigned rng_state = 0x65f47a31u;
static Sc64P6CountSnapshot *mutation_snapshot;
static unsigned local_random(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static int is_men(void *context, unsigned raw)
{
    (void)context;
    if ((int)raw == men_error_raw) return -1;
    return raw != 101u && raw != 218u;
}

static int mutate_from_ai(void *opaque, const Sc64P6SourceAction *action)
{
    Sc64P6CountSnapshot *snapshot=(Sc64P6CountSnapshot *)opaque;
    assert(snapshot==mutation_snapshot && action && !snapshot->active);
    u[0].alive=0;
    return 1;
}

static int mutate_from_doodad(void *opaque, const Sc64P6SourceAction *action)
{
    Sc64P6CountSnapshot *snapshot=(Sc64P6CountSnapshot *)opaque;
    assert(snapshot==mutation_snapshot && action && !snapshot->active);
    u[0].alive=0;
    return 1;
}

struct CandidateContext {
    Sc64P6CountSnapshotOpsContext snapshot_ops;
    u8 *actions;
    unsigned *action_count;
};

static void initialize_context(Sc64P6WorldContext *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->source_current_player = 7;
    ctx->map_cols = SC64_P6_MAP_COLS;
    ctx->map_rows = SC64_P6_MAP_ROWS;
    ctx->is_men_raw = is_men;
    for (unsigned slot = 0; slot < 8; ++slot) {
        ctx->source_slot_active[slot] = 1;
        ctx->live_owner_for_source[slot] = slot == 7 ? SIDE :
                                           slot == 3 ? ENEMY : RESCUABLE;
        ctx->source_force_for_slot[slot] = sc64_p6_source_forces[slot];
    }
}

static void random_world(unsigned density, int invalid_owner, int inject_men_error)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    men_error_raw = inject_men_error ? 101 : -1;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        if (local_random() % 100u >= density) continue;
        u[i].alive = 1;
        u[i].raw_type = (int)(local_random() % 228u);
        u[i].type = u[i].raw_type;
        u[i].raw_owner = invalid_owner && i == 0 ? 12 : (int)(local_random() % 8u);
        u[i].owner = u[i].raw_owner == 7 ? SIDE :
                     u[i].raw_owner == 3 ? ENEMY : RESCUABLE;
        u[i].x = (int)(local_random() % WORLD_W);
        u[i].y = (int)(local_random() % WORLD_H);
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        if (local_random() % 100u >= density / 2u) continue;
        b[i].alive = 1;
        b[i].raw_type = (int)(local_random() % 228u);
        b[i].type = b[i].raw_type;
        b[i].raw_owner = invalid_owner && i == 0 ? 12 : (int)(local_random() % 8u);
        b[i].owner = b[i].raw_owner == 7 ? SIDE :
                     b[i].raw_owner == 3 ? ENEMY : RESCUABLE;
        b[i].x = (int)(local_random() % WORLD_W);
        b[i].y = (int)(local_random() % WORLD_H);
    }
}

static int snapshot_condition(void *opaque, const Sc64P6RuntimeState *state,
                              const Sc64P6SourceCondition *condition)
{
    struct CandidateContext *context = opaque;
    return sc64_p6_count_snapshot_condition(&context->snapshot_ops,
                                             state, condition);
}

static int candidate_action(void *opaque, const Sc64P6SourceAction *action)
{
    struct CandidateContext *context = opaque;
    unsigned n = (*context->action_count)++;
    if (!action || n >= SC64_P6_ACTION_TOTAL) return 0;
    context->actions[n * 3u] = action->trigger;
    context->actions[n * 3u + 1u] = action->index;
    context->actions[n * 3u + 2u] = action->opcode;
    return 1;
}

static int candidate_ai(void *opaque, const Sc64P6SourceAction *action)
{ return candidate_action(opaque, action); }

static void test_randomized_world_counts(void)
{
    Sc64P6WorldContext ctx;
    Sc64P6CountSnapshot snapshot;
    Sc64P6State model;
    initialize_context(&ctx);
    sc64_p6_init(&model);
    for (unsigned world = 0; world < 150; ++world) {
        random_world(25u + world % 70u, 0, world % 11u == 0);
        assert(sc64_p6_count_snapshot_begin(&snapshot, &ctx, &model, 0));
        for (unsigned row = 0; row < SC64_P6_SOURCE_CONDITION_COUNT; ++row) {
            const Sc64P6SourceCondition *condition = &sc64_p6_source_conditions[row];
            u32 expected, cached;
            int found;
            if (condition->opcode != SC64_P6_CONDITION_COMMAND &&
                condition->opcode != SC64_P6_CONDITION_BRING) continue;
            expected = 0;
            found = sc64_p6_reference_count(&ctx, condition->player,
                    condition->unit,
                    condition->opcode == SC64_P6_CONDITION_BRING ?
                        condition->location : 0, &expected);
            if (sc64_p6_count_snapshot_lookup(&snapshot, condition, &cached)) {
                assert(found);
                assert(cached == expected);
                assert(sc64_p6_count_comparison(cached, condition->comparison,
                                                condition->amount) ==
                       sc64_p6_count_comparison(expected, condition->comparison,
                                                condition->amount));
            } else if (found) {
                /* Unsupported rows must remain on the exact original path. */
                assert(!snapshot.active || !(snapshot.cached_rows & ((u32)1u << row)) ||
                       (snapshot.fallback_rows & ((u32)1u << row)));
            }
        }
        sc64_p6_count_snapshot_end(&snapshot);
        assert(!snapshot.active);
    }
}

static void test_failclosed_fallbacks_and_pointer_authentication(void)
{
    Sc64P6WorldContext ctx;
    Sc64P6CountSnapshot snapshot;
    Sc64P6State model;
    const Sc64P6SourceCondition *men_row = &sc64_p6_source_conditions[2];
    Sc64P6SourceCondition copied = *men_row;
    u32 count = 0;
    initialize_context(&ctx);
    sc64_p6_init(&model);
    random_world(100, 1, 0);
    assert(sc64_p6_count_snapshot_begin(&snapshot, &ctx, &model, 0));
    assert(!sc64_p6_count_snapshot_lookup(&snapshot, men_row, &count));
    assert(sc64_p6_world_condition(&ctx, NULL, men_row) < 0);
    random_world(100, 0, 1);
    u[0].alive = 1; u[0].raw_type = 101; u[0].type = 101;
    u[0].raw_owner = 7; u[0].owner = SIDE; u[0].x = 256; u[0].y = 256;
    assert(sc64_p6_count_snapshot_begin(&snapshot, &ctx, &model, 0));
    assert(!sc64_p6_count_snapshot_lookup(&snapshot, men_row, &count));
    assert(sc64_p6_count_snapshot_lookup(&snapshot,
           &sc64_p6_source_conditions[1], &count));
    assert(!sc64_p6_count_snapshot_lookup(&snapshot, &copied, &count));
    sc64_p6_count_snapshot_end(&snapshot);
    assert(!sc64_p6_count_snapshot_lookup(&snapshot,
           &sc64_p6_source_conditions[1], &count));
}

static void test_invalid_begin_null_state_and_mutation_invalidation(void)
{
    Sc64P6WorldContext ctx;
    Sc64P6CountSnapshot snapshot;
    Sc64P6CountSnapshotOpsContext snapshot_ops;
    Sc64P6State model;
    const Sc64P6SourceAction *victory=NULL,*doodad=NULL,*run_ai=NULL;
    Sc64P6RuntimeState runtime;
    u32 count=0;
    initialize_context(&ctx);sc64_p6_init(&model);random_world(70,0,0);
    memset(&snapshot,0xa5,sizeof(snapshot));
    assert(!sc64_p6_count_snapshot_begin(&snapshot,&ctx,&model,-1));
    assert(!snapshot.active && !snapshot.cached_rows && !snapshot.fallback_rows);
    assert(!sc64_p6_count_snapshot_begin(&snapshot,&ctx,&model,2));
    assert(!snapshot.active);
    assert(sc64_p6_count_snapshot_begin(&snapshot,&ctx,&model,0));
    snapshot_ops.world=&ctx;snapshot_ops.snapshot=&snapshot;
    assert(sc64_p6_count_snapshot_condition(&snapshot_ops,NULL,
           &sc64_p6_source_conditions[1])==-1);

    for(unsigned t=0;t<SC64_P6_TRIGGER_COUNT;++t)
        for(unsigned i=0;i<sc64_p6_action_count((u8)t);++i) {
            const Sc64P6SourceAction *row;
            assert(sc64_p6_source_action(t,i,&row));
            if(!victory&&row->opcode==SC64_P6_ACTION_VICTORY)victory=row;
            if(!doodad&&row->opcode==SC64_P6_ACTION_SET_DOODAD)doodad=row;
            if(!run_ai&&row->opcode==SC64_P6_ACTION_RUN_AI)run_ai=row;
        }
    assert(victory&&doodad&&run_ai);
    assert(sc64_p6_count_snapshot_action(&snapshot_ops,victory)==1);
    assert(!snapshot.active && ctx.victory_requested);
    assert(!sc64_p6_count_snapshot_lookup(&snapshot,
           &sc64_p6_source_conditions[1],&count));

    assert(sc64_p6_count_snapshot_begin(&snapshot,&ctx,&model,0));
    snapshot_ops.world=&ctx;snapshot_ops.snapshot=&snapshot;
    mutation_snapshot=&snapshot;
    ctx.set_doodad_state=mutate_from_doodad;ctx.callback_context=&snapshot;
    u[0].alive=1;
    assert(sc64_p6_count_snapshot_action(&snapshot_ops,doodad)==1);
    assert(!snapshot.active && !u[0].alive);
    sc64_p6_runtime_init(&runtime);
    assert(sc64_p6_count_snapshot_condition(&snapshot_ops,&runtime,
           &sc64_p6_source_conditions[1]) ==
           sc64_p6_world_condition(&ctx,&runtime,&sc64_p6_source_conditions[1]));

    assert(sc64_p6_count_snapshot_begin(&snapshot,&ctx,&model,0));
    snapshot_ops.world=&ctx;snapshot_ops.snapshot=&snapshot;
    ctx.run_ai=mutate_from_ai;ctx.callback_context=&snapshot;
    u[0].alive=1;
    assert(sc64_p6_count_snapshot_opaque_ai(&snapshot_ops,run_ai)==1);
    assert(!snapshot.active && !u[0].alive);
    mutation_snapshot=NULL;
}

static void test_12000_tick_full_runtime_parity(void)
{
    Sc64P6WorldContext ctx;
    Sc64P6RuntimeState cached_state, reference_state;
    Sc64P6CountSnapshot snapshot;
    Sc64P6WorldOps cached_ops;
    struct CandidateContext candidate_context;
    u8 reference_actions[SC64_P6_ACTION_TOTAL * 3u];
    u8 cached_actions[SC64_P6_ACTION_TOTAL * 3u];
    unsigned reference_action_count = 0, cached_action_count = 0;
    initialize_context(&ctx);
    random_world(55, 0, 0);
    sc64_p6_runtime_init(&cached_state);
    reference_state = cached_state;
    memset(reference_actions, 0, sizeof(reference_actions));
    memset(cached_actions, 0, sizeof(cached_actions));
    candidate_context.snapshot_ops.world = &ctx;
    candidate_context.snapshot_ops.snapshot = &snapshot;
    candidate_context.actions = cached_actions;
    candidate_context.action_count = &cached_action_count;
    for (u32 tick = 0; tick < 12000u; ++tick) {
        if (tick % 7u == 0) random_world(35u + tick % 61u, 0, 0);
        cached_ops.context = &candidate_context;
        cached_ops.condition = snapshot_condition;
        cached_ops.action = candidate_action;
        cached_ops.opaque_ai = candidate_ai;
        assert(sc64_p6_count_snapshot_begin(&snapshot, &ctx,
                 &cached_state.model, 1));
        int cached = sc64_p6_adapter_tick_policy(&cached_state, &cached_ops,
                                                 (tick % 97u) == 31u, 1, 1);
        int reference = sc64_p6_reference_tick(&reference_state, &ctx,
                 (tick % 97u) == 31u, reference_actions,
                 &reference_action_count);
        assert(cached == reference);
        assert(memcmp(&cached_state, &reference_state, sizeof(cached_state)) == 0);
        assert(cached_action_count == reference_action_count);
        assert(memcmp(cached_actions, reference_actions,
                      cached_action_count * 3u) == 0);
        assert(sc64_p6_runtime_state_valid(&cached_state));
        sc64_p6_count_snapshot_end(&snapshot);
    }
}

int main(void)
{
    test_randomized_world_counts();
    test_failclosed_fallbacks_and_pointer_authentication();
    test_invalid_begin_null_state_and_mutation_invalidation();
    test_12000_tick_full_runtime_parity();
    puts("P6 transient Command/Bring snapshot matches full scans and runtime oracle");
    return 0;
}
