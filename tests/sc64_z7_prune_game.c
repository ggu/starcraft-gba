/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Differential host fixture for the private Z7 started-trigger fast path. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z7_runtime_adapter.h"

#if !defined(SC64_Z7_PREVALIDATED_TICK) || !defined(SC64_Z7_COUNT_SNAPSHOT)
#error "Compile with the gated Z7 prevalidated/count-snapshot flags"
#endif

static unsigned held_keys;
static void input(unsigned held)
{
    game_update((u16)held,(u16)(held&~held_keys));
    held_keys=held;
}
static void press(unsigned key)
{
    input(key);
    input(0);
}

static const Sc64RaceMap *load_canonical_z7(void)
{
    const Sc64RaceMap *map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    int ordinal;
    game_init();
    held_keys=0;
    ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z7_GLOBAL_ID);
    assert(ordinal==6);
    press(KEY_A);
    assert(race_front.page==SC_FRONT_MISSION && race_front.faction==SC_FRONT_ZERG);
    for(int page=0;page<ordinal/2;++page) press(KEY_RIGHT);
    if(ordinal&1) press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page==SC_FRONT_BRIEFING && race_front.mission==SC64_Z7_GLOBAL_ID);
    press(KEY_A);
    assert(screen==SCREEN_PLAY && !race_mode && !won && !lost && race_z7_active());
    assert(race_z7_map==map && race_z7_world.source_tables_prevalidated);
    memset(&race_z7.runtime,0,sizeof(race_z7.runtime));
    race_z7_world.presentation_opcode=0;
    race_z7_world.last_string_id=0;
    race_z7_world.victory_requested=0;
    race_z7_world.defeat_requested=0;
    race_z7_world.objective_refresh_requested=0;
    return map;
}

static void assert_full_snapshot(void)
{
    static const u8 trigger[5]={0,2,4,6,7};
    static const u8 selector[5]={13,19,19,18,18};
    static const u16 wanted[5]={229,231,231,229,229};
    static const u8 location[5]={0,0,4,3,5};
    assert(race_z7_live_world_valid());
    assert(sc64_z7_count_snapshot_begin(&race_z7_world));
    for(unsigned q=0;q<5;++q) {
        const Sc64Z7SourceCondition *c;
        u32 live=~(u32)0,cached=~(u32)0;
        assert(sc64_z7_condition_row(trigger[q],0,&c));
        assert(sc64_z7_world_count(&race_z7_world,selector[q],wanted[q],
                                   location[q],&live));
        assert(sc64_z7_count_snapshot_lookup(&race_z7_world,c,&cached));
        assert(cached==live);
    }
}

#ifdef SC64_Z7_PRUNE_STARTED
static unsigned expected_cache_mask(u16 started)
{
    static const u8 trigger[5]={0,2,4,6,7};
    unsigned mask=0;
    for(unsigned q=0;q<5;++q)
        if(!(started&(1u<<trigger[q]))) mask|=1u<<q;
    return mask;
}

static void assert_pruned_snapshot(u16 started)
{
    static const u8 trigger[5]={0,2,4,6,7};
    static const u8 selector[5]={13,19,19,18,18};
    static const u16 wanted[5]={229,231,231,229,229};
    static const u8 location[5]={0,0,4,3,5};
    unsigned mask=expected_cache_mask(started);
    assert(race_z7_live_world_valid());
    assert(sc64_z7_count_snapshot_begin_pruned(&race_z7_world,started));
    assert(race_z7_world.count_snapshot.active==mask);
    for(unsigned q=0;q<5;++q) {
        const Sc64Z7SourceCondition *c;
        u32 live=~(u32)0,cached=77;
        assert(sc64_z7_condition_row(trigger[q],0,&c));
        assert(sc64_z7_world_count(&race_z7_world,selector[q],wanted[q],
                                   location[q],&live));
        if(mask&(1u<<q)) {
            assert(sc64_z7_count_snapshot_lookup(&race_z7_world,c,&cached));
            assert(cached==live);
        } else {
            assert(!sc64_z7_count_snapshot_lookup(&race_z7_world,c,&cached));
            assert(cached==77u);
        }
    }
}

static void test_started_conditions_and_active_waits(void)
{
    const Sc64RaceMap *map=load_canonical_z7();
    int human=-1;
    unsigned left,top,right,bottom;
    const Sc64Z7SourceCondition *region;
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive&&u[i].raw_owner==3u) { human=i; break; }
    assert(human>=0);
    assert(sc64_z7_world_location_source(&race_z7_world,3u,&left,&top,&right,&bottom));
    u[human].x=(s16)(left/2u+8u); u[human].y=(s16)(top/2u+8u);
    assert(sc64_z7_condition_row(6,0,&region));
    assert(race_z7_prevalidated_condition_callback(&race_z7_world,&race_z7.runtime,
                                                   region)==1);

    /* Started is permanent for this authenticated non-preserved program.
       A changed live condition therefore cannot restart it. */
    race_z7.runtime.model.started=(u16)(1u<<6);
    assert_pruned_snapshot(race_z7.runtime.model.started);
    assert(race_z7_prevalidated_condition_callback(&race_z7_world,&race_z7.runtime,
                                                   region)==1);
    assert(race_z7_pruned_condition_callback(&race_z7_world,&race_z7.runtime,
                                              region)==0);

    /* During an active wait, both the checked scheduler and the pruned game
       path continue ticking action time while the started condition is false. */
    race_z7.runtime.model.started=(u16)((1u<<4)|(1u<<6));
    race_z7.runtime.model.action_cursor[4]=1;
    race_z7.runtime.action_ticks[4]=5;
    assert(sc64_z7_runtime_state_valid(&race_z7.runtime));
    assert(race_z7_runtime_tick(1,10u));
    assert(race_z7.runtime.action_ticks[4]==5u);
    assert(race_z7_runtime_tick(0,11u));
    assert(race_z7.runtime.action_ticks[4]==4u);
    assert(race_z7.runtime.model.action_cursor[4]==1u);
    assert(race_z7.runtime.model.started&((1u<<4)|(1u<<6)));
    assert(race_z7_map==map);
}

static void test_mutations_refresh_only_unstarted_counts(void)
{
    const Sc64RaceMap *map=load_canonical_z7();
    static const u8 trigger[5]={0,2,4,6,7};
    int human=-1,product;
    unsigned left,top,right,bottom;
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive&&u[i].raw_owner==3u) { human=i; break; }
    assert(human>=0);
    /* First reference census and pruned census are identical with no started
       triggers. */
    assert_full_snapshot();
    race_z7_world.count_snapshot.active=0;
    assert_pruned_snapshot(0);
    /* Exercise every possible live-counter mask against direct world counts. */
    for(unsigned needed=0;needed<32u;++needed) {
        u16 started=0xffu;
        for(unsigned q=0;q<5;++q)
            if(needed&(1u<<q)) started=(u16)(started&~(1u<<trigger[q]));
        assert_pruned_snapshot(started);
    }

    /* Move across region 3's half-open left edge, then kill the moved actor. */
    assert(sc64_z7_world_location_source(&race_z7_world,3u,&left,&top,&right,&bottom));
    u[human].x=(s16)(left/2u+8u); u[human].y=(s16)(top/2u+8u);
    assert_pruned_snapshot((u16)(1u<<6));
    assert(sc64_z7_world_condition_prevalidated(&race_z7_world,&race_z7.runtime,
                                                &sc64_z7_source_conditions[6])==1);
    u[human].alive=0;
    assert_pruned_snapshot((u16)(1u<<6));

    /* A paid/runtime product is reconciled before the new per-tick census. */
    product=spawn_unit(SIDE,U_MUTALISK,(int)(left/2u+12u),(int)(top/2u+12u));
    assert(product>=0);
    u[product].raw_owner=-1;
    race_z7_reconcile_paid_products();
    assert(u[product].raw_owner==3 && race_z7_live_world_valid());
    assert_pruned_snapshot((u16)(1u<<6));
    assert(race_z7_map==map);
}

static void test_authentication_fallback_save_load_and_terminal(void)
{
    const Sc64RaceMap *map=load_canonical_z7();
    Sc64Z7WorldOps saved_ops=race_z7_ops;
    const Sc64Z7SourceCondition *c;
    Sc64Z7SourceCondition copied;
    assert(sc64_z7_condition_row(6,0,&c));
    copied=*c;
    assert(race_z7_pruned_condition_callback(&race_z7_world,&race_z7.runtime,
                                              &copied)<0);
    assert(race_z7_pruned_condition_callback((void *)1,&race_z7.runtime,c)<0);
    assert(sc64_z7_count_snapshot_begin_pruned(&race_z7_world,0));
    assert(!sc64_z7_count_snapshot_begin_pruned(&race_z7_world,0x100u));
    assert(!race_z7_world.count_snapshot.active);

    /* A stale callback binding retains the baseline fail-closed behavior. */
    race_z7_ops.context=(void *)1;
    assert(!race_z7_pruned_binding_valid());
    assert(!race_z7_runtime_tick(0,0u));
    assert(race_z7.runtime.fault==SC64_Z7_FAULT_CONDITION);
    race_z7_ops=saved_ops;
    assert(race_z7_map==map && race_z7_world.source_tables_prevalidated);
    memset(&race_z7.runtime,0,sizeof(race_z7.runtime));
    sc64_z7_count_snapshot_invalidate(&race_z7_world);

    /* A cleared authentication bit also follows the baseline callback and
       fails closed instead of reconstructing a checked binding. */
    race_z7_world.source_tables_prevalidated=0;
    assert(!race_z7_runtime_tick(0,0u));
    assert(race_z7.runtime.fault==SC64_Z7_FAULT_CONDITION);
    race_z7_world.source_tables_prevalidated=1;
    memset(&race_z7.runtime,0,sizeof(race_z7.runtime));
    sc64_z7_count_snapshot_invalidate(&race_z7_world);

    /* Runtime ticks fail closed on malformed actor provenance before building
       a snapshot, then load rebind restores the authenticated live world. */
    int actor=-1;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive) { actor=i; break; }
    assert(actor>=0);
    s16 owner=u[actor].raw_owner;
    u[actor].raw_owner=12;
    assert(!race_z7_runtime_tick(0,1u));
    assert(race_z7.runtime.fault==SC64_Z7_RUNTIME_FAULT_ACTOR);
    u[actor].raw_owner=owner;

    /* Active wait state is serialized and restored, while paused ticks keep
       its clock stable. */
    assert(race_z7_map==map);
    memset(&race_z7.runtime,0,sizeof(race_z7.runtime));
    race_z7.runtime.model.started=(u16)(1u<<4);
    race_z7.runtime.model.action_cursor[4]=1;
    race_z7.runtime.action_ticks[4]=5;
    assert(race_save_game());
    race_z7.runtime.action_ticks[4]=1;
    assert(race_load_game());
    assert(race_z7.runtime.action_ticks[4]==5u);
    assert(race_z7_runtime_tick(1,20u));
    assert(race_z7.runtime.action_ticks[4]==5u);
    assert(race_z7_runtime_tick(0,21u));
    assert(race_z7.runtime.action_ticks[4]==4u);

    /* Loss remains live while an own actor exists; eliminating all own actors
       reaches the terminal source action. */
    load_canonical_z7();
    int survivor=-1;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive&&u[i].owner==SIDE) { survivor=i; break; }
    assert(survivor>=0);
    for(int i=0;i<MAX_UNITS;++i) if(i!=survivor&&u[i].owner==SIDE) u[i].alive=0;
    sc64_z7_count_snapshot_invalidate(&race_z7_world);
    assert(sc64_z7_condition_row(0,0,&c));
    assert(sc64_z7_world_condition_prevalidated(&race_z7_world,&race_z7.runtime,c)==0);
    u[survivor].alive=0;
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].owner==SIDE) b[i].alive=0;
    sc64_z7_count_snapshot_invalidate(&race_z7_world);
    assert(sc64_z7_world_condition_prevalidated(&race_z7_world,&race_z7.runtime,c)==1);
    assert(race_z7_runtime_tick(0,30u));
    assert(race_z7.runtime.model.started&(1u<<SC64_Z7_TRIGGER_LOSS));
    assert(race_z7_runtime_tick(0,31u));
    assert(race_z7.runtime.model.outcome==SC64_Z7_LOST);
    update_imported_mission();
    assert(lost && !won);
    assert(!race_z7_world.count_snapshot.active);

    /* Victory is also evaluated while unstarted: retain one hostile building,
       prove the condition false, then remove it and allow the terminal action. */
    load_canonical_z7();
    int keep=-1;
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive&&b[i].owner==ENEMY) { keep=i; break; }
    assert(keep>=0);
    for(int i=0;i<MAX_BUILDINGS;++i) if(i!=keep&&b[i].owner==ENEMY) b[i].alive=0;
    sc64_z7_count_snapshot_invalidate(&race_z7_world);
    assert(sc64_z7_condition_row(SC64_Z7_TRIGGER_VICTORY,0,&c));
    assert(sc64_z7_world_condition_prevalidated(&race_z7_world,&race_z7.runtime,c)==0);
    b[keep].alive=0;
    assert(race_z7_runtime_tick(0,40u));
    assert(race_z7.runtime.model.started&(1u<<SC64_Z7_TRIGGER_VICTORY));
    assert(race_z7_runtime_tick(0,41u));
    assert(race_z7.runtime.model.outcome==SC64_Z7_WON);
    update_imported_mission();
    assert(won && !lost);
    assert(!race_z7_world.count_snapshot.active);
}
#endif

static void test_reference_path_remains_checked(void)
{
    load_canonical_z7();
    assert_full_snapshot();
    Sc64Z7SourceCondition copied=sc64_z7_source_conditions[0];
    u32 count=99;
    assert(!sc64_z7_count_snapshot_lookup(&race_z7_world,&copied,&count));
    assert(count==99u);
    assert(race_z7_runtime_tick(0,1u));
    assert(!race_z7.runtime.fault);
}

int main(void)
{
    test_reference_path_remains_checked();
#ifdef SC64_Z7_PRUNE_STARTED
    test_started_conditions_and_active_waits();
    test_mutations_refresh_only_unstarted_counts();
    test_authentication_fallback_save_load_and_terminal();
    puts("Z7 started-trigger pruning host checks passed");
#else
    puts("Z7 full checked snapshot reference host checks passed (pruning off)");
#endif
    return 0;
}
