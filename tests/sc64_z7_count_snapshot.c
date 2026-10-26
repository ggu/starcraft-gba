/* Focused host checks for the private Z7 same-tick command-count census. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z7_runtime_adapter.h"

#if !defined(SC64_Z7_PREVALIDATED_TICK) || !defined(SC64_Z7_COUNT_SNAPSHOT)
#error "Compile with the gated Z7 count-snapshot flags"
#endif

static const Sc64RaceMap *load_canonical_z7(void)
{
    const Sc64RaceMap *map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    unsigned minerals=0;
    race_cols=SC64_Z7_MAP_COLS; race_rows=SC64_Z7_MAP_ROWS;
    race_mode=0; race_selected_mission=SC64_Z7_GLOBAL_ID;
    race_player[ENEMY]=map->owner_races[4];
    race_minerals[SIDE]=race_gas[SIDE]=0;
    race_minerals[ENEMY]=race_gas[ENEMY]=0;
    supply_used=supply_cap=0; selected_building=-1; geyser_count=0;
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); memset(m,0,sizeof(m));
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row]; unsigned raw=src->source_type;
        if(raw==SC64_RAW_START_LOCATION) continue;
        if(raw>=176u&&raw<=178u) {
            assert(minerals<MAX_MINERALS);
            m[minerals].x=src->x_px; m[minerals].y=src->y_px;
            m[minerals].amount=src->resources?src->resources:1u;
            m[minerals].style=(int)raw; ++minerals; continue;
        }
        if(raw==188u) {
            assert(geyser_count<MAX_RACE_GEYSERS);
            geysers[geyser_count++]=(Geyser){src->x_px,src->y_px}; continue;
        }
        assert(src->owner<8u);
        int owner=src->owner==map->human_slot?SIDE:map->owner_modes[src->owner];
        int type=sc64_race_unit_from_raw((int)raw);
        if(type>=0) {
            int slot=spawn_unit(owner,type,src->x_px,src->y_px); assert(slot>=0);
            u[slot].raw_type=(s16)raw; u[slot].raw_owner=src->owner;
        } else {
            type=sc64_race_building_from_raw((int)raw); assert(type>=0);
            int slot=spawn_build(owner,type,src->x_px,src->y_px,0); assert(slot>=0);
            b[slot].raw_type=(s16)raw; b[slot].raw_owner=src->owner;
        }
    }
    return map;
}

static void check_source_bound_queries_match_checked_counts(void)
{
    static const u8 trigger[5]={0,2,4,6,7};
    static const u8 selector[5]={13,19,19,18,18};
    static const u16 wanted[5]={229,231,231,229,229};
    static const u8 location[5]={0,0,4,3,5};
    const Sc64RaceMap *map=load_canonical_z7();
    assert(race_z7_runtime_init(map));
    assert(race_z7_world.source_tables_prevalidated);
    assert(sc64_z7_count_snapshot_begin(&race_z7_world));
    for(unsigned q=0;q<5;++q) {
        const Sc64Z7SourceCondition *c;
        u32 expected=0,cached=~(u32)0;
        assert(sc64_z7_condition_row(trigger[q],0,&c));
        assert(sc64_z7_world_count(&race_z7_world,selector[q],wanted[q],
                                   location[q],&expected));
        assert(sc64_z7_count_snapshot_lookup(&race_z7_world,c,&cached));
        assert(cached==expected);
        assert(race_z7_prevalidated_condition_callback(&race_z7_world,
            &race_z7.runtime,c)==sc64_z7_world_condition(&race_z7_world,
            &race_z7.runtime,c));
    }
    /* Private rows only: a copied descriptor is never a cache key. */
    {
        Sc64Z7SourceCondition copied=sc64_z7_source_conditions[0];
        u32 out=77;
        assert(!sc64_z7_count_snapshot_lookup(&race_z7_world,&copied,&out));
        assert(out==77);
    }
    race_z7_world.count_snapshot.active=0;
    assert(!race_z7_world.count_snapshot.active);
    assert(!sc64_z7_count_snapshot_lookup(&race_z7_world,
        &sc64_z7_source_conditions[0],&(u32){0}));
}

static void check_checked_api_never_uses_cache_and_errors_fail_closed(void)
{
    const Sc64RaceMap *map=load_canonical_z7();
    int actor=-1,old_owner;
    assert(race_z7_runtime_init(map));
    assert(sc64_z7_count_snapshot_begin(&race_z7_world));
    /* If the checked API accidentally consulted the cache this counterfeit
       zero would make the live hostile-building predicate appear true. */
    race_z7_world.count_snapshot.count[1]=0;
    assert(sc64_z7_world_condition(&race_z7_world,&race_z7.runtime,
        &sc64_z7_source_conditions[2])==0);
    race_z7_world.count_snapshot.active=0;

    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive) { actor=i; break; }
    assert(actor>=0);
    old_owner=u[actor].raw_owner; u[actor].raw_owner=-1;
    assert(!sc64_z7_count_snapshot_begin(&race_z7_world));
    assert(race_z7_prevalidated_condition_callback(&race_z7_world,
        &race_z7.runtime,&sc64_z7_source_conditions[0])<0);
    assert(sc64_z7_world_condition(&race_z7_world,&race_z7.runtime,
        &sc64_z7_source_conditions[0])<0);
    u[actor].raw_owner=(s16)old_owner;
    assert(sc64_z7_count_snapshot_begin(&race_z7_world));

    /* Rebinding invalidates the transient cache regardless of old contents. */
    assert(race_z7_rebind_loaded_world(map));
    assert(!race_z7_world.count_snapshot.active);
    assert(!sc64_z7_count_snapshot_lookup(&race_z7_world,
        &sc64_z7_source_conditions[0],&(u32){0}));
}

static void check_refresh_handles_region_edges_and_building_survival(void)
{
    const Sc64RaceMap *map=load_canonical_z7();
    int human=-1;
    assert(race_z7_runtime_init(map));
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==43) { human=i; break; }
    assert(human>=0);
    /* Sweep a real friendly air unit across a source region corner. The
       fused census must agree with the checked rectangle/half-open query. */
    unsigned left,top,right,bottom;
    assert(sc64_z7_world_location_source(&race_z7_world,3u,
                                        &left,&top,&right,&bottom));
    for(int offset=-24;offset<=24;offset+=2) {
        const Sc64Z7SourceCondition *c;
        u32 checked,cached;
        u[human].x=(s16)((int)(left/2u)+offset);
        u[human].y=(s16)(top/2u);
        assert(sc64_z7_count_snapshot_begin(&race_z7_world));
        assert(sc64_z7_condition_row(6u,0,&c));
        assert(sc64_z7_world_count(&race_z7_world,18u,229u,3u,&checked));
        assert(sc64_z7_count_snapshot_lookup(&race_z7_world,c,&cached));
        assert(cached==checked);
        sc64_z7_count_snapshot_invalidate(&race_z7_world);
    }
    /* The source Any Unit loss test includes a surviving friendly building,
       even after every friendly mobile unit has died. */
    for(int i=0;i<MAX_UNITS;++i) if(u[i].owner==SIDE) u[i].alive=0;
    int hatch=spawn_build(SIDE,B_HATCHERY,1000,1750,0);
    assert(hatch>=0);
    b[hatch].raw_owner=3; b[hatch].raw_type=131;
    const Sc64Z7SourceCondition *loss;
    u32 cached;
    assert(sc64_z7_condition_row(0u,0,&loss));
    assert(sc64_z7_count_snapshot_begin(&race_z7_world));
    assert(sc64_z7_count_snapshot_lookup(&race_z7_world,loss,&cached)&&cached==1u);
    assert(race_z7_prevalidated_condition_callback(&race_z7_world,
                                                  &race_z7.runtime,loss)==0);
    b[hatch].alive=0;
    assert(sc64_z7_count_snapshot_begin(&race_z7_world));
    assert(sc64_z7_count_snapshot_lookup(&race_z7_world,loss,&cached)&&cached==0u);
    assert(race_z7_prevalidated_condition_callback(&race_z7_world,
                                                  &race_z7.runtime,loss)==1);
}

int main(void)
{
    check_source_bound_queries_match_checked_counts();
    check_checked_api_never_uses_cache_and_errors_fail_closed();
    check_refresh_handles_region_edges_and_building_survival();
    printf("Z7 count snapshot tests passed (world context %u bytes, cache %u bytes)\n",
        (unsigned)sizeof(Sc64Z7WorldContext),
        (unsigned)sizeof(race_z7_world.count_snapshot));
    return 0;
}
