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
    press(KEY_START);
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
    assert(sc64_z7_count_snapshot_begin_pruned(&race_z7_world,started,0));
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

#ifdef SC64_Z7_VALIDATED_CENSUS
static unsigned long fused_geometry_checks, fused_failure_checks, fused_two_counter_checks;

/* One tick's worth of counting both ways: the validation-pass census against
 * the separate pruned census, with the same result, counts and active mask. */
static void census_both_ways(u16 started)
{
    u32 late[2]={0,0};
    Sc64Z7WorldContext *w=&race_z7_world;
    unsigned needed=sc64_z7_count_snapshot_needed(started);
    int armed=sc64_z7_census_arm(w,started);
    int fused_path=armed && needed!=3u;
    u32 fused_count[5]; u8 fused_active; int fused_ok, ref_ok;
    assert(race_z7_pruned_binding_valid());
    assert(race_z7_validate_live_world(armed?late:0));
    fused_ok=sc64_z7_census_begin_pruned(w,started,armed?late:0);
    memcpy(fused_count,w->count_snapshot.count,sizeof(fused_count));
    fused_active=w->count_snapshot.active;
    ref_ok=sc64_z7_count_snapshot_begin_pruned(w,started,0);
    assert(fused_ok==ref_ok);
    assert(fused_active==w->count_snapshot.active);
    assert(!memcmp(fused_count,w->count_snapshot.count,sizeof(fused_count)));
    if (fused_path) { ++fused_geometry_checks; fused_failure_checks+=!ref_ok; }
    else if (armed) ++fused_two_counter_checks;
    /* The fused state is consumed by one snapshot. */
    assert(!sc64_z7_census.armed && !sc64_z7_census.done);
}

static void census_every_mask(void)
{
    static const u8 trigger[5]={0,2,4,6,7};
    for (unsigned needed=0;needed<32u;++needed) {
        u16 started=0xffu;
        for (unsigned q=0;q<5;++q)
            if (needed&(1u<<q)) started=(u16)(started&~(1u<<trigger[q]));
        census_both_ways(started);
    }
}

static void test_fused_census_matches_separate(void)
{
    unsigned l[3][4];
    int own=-1,hostile_building=-1,force0_unit=-1,force0_building=-1,other_unit=-1;
    load_canonical_z7();
    census_every_mask();
    for (unsigned k=0;k<3;++k) {
        static const unsigned id[3]={3u,4u,5u};
        assert(sc64_z7_world_location_source(&race_z7_world,id[k],&l[k][0],&l[k][1],
                                              &l[k][2],&l[k][3]));
    }
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
        unsigned slot=(unsigned)u[i].raw_owner;
        if (slot==race_z7_world.source_current_player && own<0) own=i;
        else if (race_z7_world.source_force_for_slot[slot]==0u && force0_unit<0) force0_unit=i;
        else if (race_z7_world.source_force_for_slot[slot]==1u && other_unit<0) other_unit=i;
    }
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive) {
        unsigned slot=(unsigned)b[i].raw_owner;
        if (race_z7_world.source_force_for_slot[slot]==1u && hostile_building<0) hostile_building=i;
        if (race_z7_world.source_force_for_slot[slot]==0u && force0_building<0) force0_building=i;
    }
    assert(own>=0 && force0_unit>=0 && hostile_building>=0);
    /* Move force-0 actors across each region edge, one pixel at a time, and
       place a hostile building in its named region. */
    for (unsigned k=0;k<3;++k) for (int d=-3;d<=3;++d) {
        int x=(int)(l[k][0]/2u)+d, y=(int)(l[k][1]/2u)+d;
        u[force0_unit].x=(s16)x; u[force0_unit].y=(s16)y;
        b[hostile_building].x=(s16)x; b[hostile_building].y=(s16)y;
        if (force0_building>=0) { b[force0_building].x=(s16)(x+2); b[force0_building].y=(s16)(y+2); }
        x=(int)(l[k][2]/2u)+d; y=(int)(l[k][3]/2u)+d;
        if (other_unit>=0) { u[other_unit].x=(s16)x; u[other_unit].y=(s16)y; }
        census_every_mask();
    }
    /* Liveness and ownership transitions. */
    for (unsigned mix=0;mix<16u;++mix) {
        u[own].alive=(mix&1u)!=0;
        u[force0_unit].alive=(mix&2u)!=0;
        b[hostile_building].alive=(mix&4u)!=0;
        if (force0_building>=0) b[force0_building].alive=(mix&8u)!=0;
        census_every_mask();
    }
    u[own].alive=u[force0_unit].alive=b[hostile_building].alive=1;
    if (force0_building>=0) b[force0_building].alive=1;
    /* A raw id outside the source table, a unit without a live type, and an
       actor without region bounds each make both censuses refuse. */
    {
        s16 raw=u[force0_unit].raw_type; int type=u[force0_unit].type;
        int found=0;
        for (int r=0;r<=227 && !found;++r) {
            int al,at,ar,ab;
            if (sc64_race_unit_from_raw(r)>=0 &&
                !sc64_z7_world_actor_rect((unsigned)r,100,100,&al,&at,&ar,&ab)) {
                u[force0_unit].raw_type=(s16)r; u[force0_unit].type=(__typeof__(u[0].type))sc64_race_unit_from_raw(r);
                found=1;
            }
        }
        if (found && race_z7_live_world_valid()) census_every_mask();
        found=0;
        for (int r=0;r<=227 && !found;++r) if (sc64_race_unit_from_raw(r)<0) {
            u[force0_unit].raw_type=(s16)r; u[force0_unit].type=(__typeof__(u[0].type))-1;
            found=1;
        }
        /* Compact unit records store an unsigned type and cannot hold -1. */
        if (found && race_z7_live_world_valid()) census_every_mask();
        u[force0_unit].raw_type=raw; u[force0_unit].type=(__typeof__(u[0].type))type;
    }
    census_every_mask();
    assert(fused_geometry_checks>1000 && fused_two_counter_checks>0);
    printf("Z7 fused census: %lu geometry comparisons (%lu refusals), %lu two-counter\n",
           fused_geometry_checks,fused_failure_checks,fused_two_counter_checks);
}

static void test_validated_late_census(void)
{
    load_canonical_z7();
    const u16 started=(1u<<4)|(1u<<6)|(1u<<7);
    int own=-1, enemy=-1;
    u32 counts[2];
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].raw_owner==3) { own=i; break; }
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].raw_owner==4) { enemy=i; break; }
    assert(own>=0 && enemy>=0 && sc64_z7_count_snapshot_needed(started)==3u);
    for (unsigned i=0;i<4;++i) {
        u[own].alive=(i&1u)!=0;
        b[enemy].alive=(i&2u)!=0;
        assert(race_z7_validate_live_world(counts));
        assert(sc64_z7_count_snapshot_begin_pruned(&race_z7_world,started,0));
        Sc64Z7WorldContext ref=race_z7_world;
        assert(sc64_z7_count_snapshot_begin_pruned(&race_z7_world,started,counts));
        assert(race_z7_world.count_snapshot.active==3u);
        assert(!memcmp(ref.count_snapshot.count,race_z7_world.count_snapshot.count,
                       sizeof(ref.count_snapshot.count)));
    }
    counts[0]=counts[1]=0xcafeu;
    b[enemy].raw_type=-1;
    assert(!race_z7_validate_live_world(counts));
    assert(counts[0]==0xcafeu && counts[1]==0xcafeu);
    race_z7.runtime.model.started=started;
    assert(!race_z7_runtime_tick(0,1u));
    assert(race_z7.runtime.fault==SC64_Z7_RUNTIME_FAULT_ACTOR);
}
#endif

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
    assert(sc64_z7_count_snapshot_begin_pruned(&race_z7_world,0,0));
    assert(!sc64_z7_count_snapshot_begin_pruned(&race_z7_world,0x100u,0));
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
#ifdef SC64_Z7_VALIDATED_CENSUS
    test_validated_late_census();
    test_fused_census_matches_separate();
#endif
    test_started_conditions_and_active_waits();
    test_mutations_refresh_only_unstarted_counts();
    test_authentication_fallback_save_load_and_terminal();
    puts("Z7 started-trigger pruning host checks passed");
#else
    puts("Z7 full checked snapshot reference host checks passed (pruning off)");
#endif
    return 0;
}
