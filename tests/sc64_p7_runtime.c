#include "../starcraft/game.c"
#include "../starcraft/sc64_p7_runtime_adapter.h"

#include <assert.h>

static void setup_context(Sc64P7PersistentState *state,
                          Sc64P7WorldContext *ctx)
{
    memset(state,0,sizeof(*state));
    sc64_p7_runtime_init(&state->runtime);
    memset(ctx,0,sizeof(*ctx));
    ctx->map=&sc64_race_maps[SC64_P7_GLOBAL_ID];
    ctx->persistent=state;
    ctx->source_current_player=7;
    for (unsigned i=0;i<8;++i) {
        ctx->source_slot_active[i]=sc64_p7_active_slots[i];
        ctx->source_force_for_slot[i]=sc64_p7_force_for_slot[i];
        ctx->live_owner_for_source[i]=(s8)(i==7?SIDE:sc64_p7_owner_modes_expected[i]);
    }
}

static void test_source_metadata_and_candidate_bound(void)
{
    Sc64P7PersistentState state;
    memset(&state,0,sizeof(state));
    sc64_p7_runtime_init(&state.runtime);
    assert(sizeof(state)<=148u);
    assert(sc64_p7_persistent_state_valid(&state));
    assert(sc64_p7_runtime_metadata_valid());
    assert(SC64_P7_SOURCE_GLOBAL_ID==26u);
    assert(SC64_P7_SOURCE_TRIGGER_COUNT==10u);
    assert(SC64_P7_SOURCE_ACTION_COUNT==44u);
    assert(SC64_P7_SOURCE_CONDITION_COUNT==10u);
    assert(!race_p7_source_row_counts(&sc64_race_maps[SC64_P7_GLOBAL_ID]));
    /* The real map remains fail-closed until raw 68, 71 and 72 receive their
       dedicated actor rules; this must not turn into a type alias. */
    assert(sc64_race_unit_from_raw(68)<0);
    assert(sc64_race_unit_from_raw(71)<0);
    assert(sc64_race_unit_from_raw(72)<0);
    assert(sc64_race_unit_from_raw(87)<0); /* voice operand, not source actor */
}

static void test_command_owner_force_and_atmost(void)
{
    Sc64P7PersistentState state;
    Sc64P7WorldContext ctx;
    setup_context(&state,&ctx);
    assert(sc64_p7_world_condition(&ctx,&state.runtime,
                    &sc64_p7_source_conditions[0])==1); /* no human Any Unit */

    int human=spawn_unit(SIDE,U_MARINE,80,90);
    assert(human>=0);
    u[human].raw_type=0; u[human].raw_owner=7;
    assert(sc64_p7_world_condition(&ctx,&state.runtime,
                    &sc64_p7_source_conditions[0])==0);
    assert(sc64_p7_world_condition(&ctx,&state.runtime,
                    &sc64_p7_source_conditions[1])==1); /* Force3 has no raw78 */

    int force2=spawn_unit(ENEMY,U_PROBE,200,200);
    assert(force2>=0);
    u[force2].raw_type=64; u[force2].raw_owner=0;
    assert(sc64_p7_world_condition(&ctx,&state.runtime,
                    &sc64_p7_source_conditions[6])==1); /* Force2 >= 1 Any Unit */
    u[force2].raw_owner=4; /* Same live owner, different source force. */
    assert(sc64_p7_world_condition(&ctx,&state.runtime,
                    &sc64_p7_source_conditions[6])==0);
    assert(sc64_p7_world_condition(&ctx,&state.runtime,
                    &sc64_p7_source_conditions[7])==1); /* Force1 */
}

static void test_source_pixel_bounds_and_dynamic_location(void)
{
    Sc64P7PersistentState state;
    Sc64P7WorldContext ctx;
    unsigned l,t,r,bm;
    int al,at,ar,ab;
    setup_context(&state,&ctx);
    assert(sc64_p7_world_location_source(&ctx,2,&l,&t,&r,&bm));
    assert(l==0x0e60u && t==0x07a0u && r==0x0f40u && bm==0x0860u);
    assert(sc64_p7_world_actor_rect_source(65,100,100,0,&al,&at,&ar,&ab));
    assert(al==194 && at==197 && ar==207 && ab==208);
    state.moved_location3_valid=1;
    state.moved_location3[0]=400; state.moved_location3[1]=500;
    state.moved_location3[2]=600; state.moved_location3[3]=700;
    assert(sc64_p7_world_location_source(&ctx,3,&l,&t,&r,&bm));
    assert(l==400 && t==500 && r==600 && bm==700);
    state.moved_location3[2]=400;
    assert(!sc64_p7_persistent_state_valid(&state));
}

static void test_source_wallets_and_unmapped_create(void)
{
    Sc64P7PersistentState state;
    Sc64P7WorldContext ctx;
    setup_context(&state,&ctx);
    /* Source Force2 and Force1 grants stay separate even though their
       imported actors share ENEMY. No engine resource bucket is overwritten. */
    assert(sc64_p7_world_action(&ctx,&sc64_p7_source_actions[37]));
    assert(state.source_minerals[0]==10000u && state.source_minerals[4]==0u);
    assert(sc64_p7_world_action(&ctx,&sc64_p7_source_actions[40]));
    assert(state.source_minerals[0]==10000u && state.source_minerals[4]==10000u);
    assert(race_minerals[ENEMY]==0);

    /* The two authenticated raw72 Carrier creation rows fail closed until a
       distinct source-backed actor mapping exists, with no partial spawn. */
    int live_before=0;
    for (int i=0;i<MAX_UNITS;++i) live_before+=u[i].alive!=0;
    assert(!sc64_p7_world_action(&ctx,&sc64_p7_source_actions[29]));
    assert(!sc64_p7_world_action(&ctx,&sc64_p7_source_actions[30]));
    /* Authenticated row28 creates the Shuttle for source slot0. */
    assert(sc64_p7_world_action(&ctx,&sc64_p7_source_actions[28]));
    int shuttle=-1;
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive && u[i].raw_type==69 && u[i].raw_owner==0) shuttle=i;
    assert(shuttle>=0 && u[shuttle].owner==ENEMY && state.created_mask==1);
    int live_after=0;
    for (int i=0;i<MAX_UNITS;++i) live_after+=u[i].alive!=0;
    assert(live_before+1==live_after && state.created_mask==1);
}

static void test_opaque_ai_is_bounded_and_uses_source_force(void)
{
    Sc64P7PersistentState state;
    Sc64P7WorldContext ctx;
    setup_context(&state,&ctx);
    int force2=spawn_unit(ENEMY,U_PROBE,200,200);
    assert(force2>=0);
    u[force2].raw_type=64; u[force2].raw_owner=0;
    assert(sc64_p7_world_run_ai(&ctx,&sc64_p7_source_actions[38]));
    assert(u[force2].order==ORDER_ATTACK_MOVE);
    int l,t,r,bm;
    assert(sc64_p7_world_location(&ctx,4,&l,&t,&r,&bm));
    assert(u[force2].tx==(l+r)/2 && u[force2].ty==(t+bm)/2);
    assert(sc64_p7_world_run_ai(&ctx,&sc64_p7_source_actions[39]));
    Sc64P7SourceAction forged=sc64_p7_source_actions[39];
    forged.group2=0;
    assert(!sc64_p7_world_run_ai(&ctx,&forged));
}

int main(void)
{
    /* Exercise the real three resource rows, including CurrentPlayer's
       human bank and both independent enemy Force wallets. */
    {
        Sc64P7PersistentState state; Sc64P7WorldContext ctx;
        setup_context(&state,&ctx);
        state.runtime.model.started=(1u<<4)|(1u<<6)|(1u<<7);
        for(unsigned i=0;i<SC64_P7_SOURCE_ACTION_COUNT;++i)
            if(sc64_p7_source_actions[i].opcode==26)
                assert(sc64_p7_world_action(&ctx,&sc64_p7_source_actions[i]));
        assert(minerals==2500 && gas==2500 && race_minerals[SIDE]==2500);
        assert(state.source_minerals[0]==10000 && state.source_gas[0]==10000);
        assert(state.source_minerals[4]==10000 && state.source_gas[4]==10000);
        assert(state.source_minerals[7]==0 && state.source_gas[7]==0);
        assert(sc64_p7_persistent_state_valid(&state));
    }
    test_source_metadata_and_candidate_bound();
    test_command_owner_force_and_atmost();
    test_source_pixel_bounds_and_dynamic_location();
    test_source_wallets_and_unmapped_create();
    test_opaque_ai_is_bounded_and_uses_source_force();
    return 0;
}
