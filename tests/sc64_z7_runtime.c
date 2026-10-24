#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z7_runtime_adapter.h"

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
    for(unsigned row=0;row<map->unit_count;++row) {
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

static void test_canonical_binding_and_source_queries(void)
{
    const Sc64RaceMap *map=load_canonical_z7();
    assert(race_z7_runtime_init(map));
    assert(race_z7_map==map && sizeof(race_z7)<=36u);
    assert(race_z7_source_owner(3)==SIDE && race_z7_source_owner(4)==ENEMY &&
           race_z7_source_owner(0)==NEUTRAL);
    assert(race_z7_world.source_force_for_slot[3]==0u &&
           race_z7_world.source_force_for_slot[4]==1u);
    unsigned executor=99;
    assert(sc64_z7_trigger_executor(0,&executor)&&executor==18u);
    assert(sc64_z7_trigger_executor(3,&executor)&&executor==19u);
    u32 count=0;
    assert(sc64_z7_world_count(&race_z7_world,19u,229u,0,&count)&&count>0u);
    assert(sc64_z7_world_count(&race_z7_world,18u,229u,0,&count)&&count>0u);
    assert(!sc64_z7_world_count(&race_z7_world,12u,229u,0,&count));
    assert(sc64_z7_persistent_state_valid(&race_z7));
}

static void test_authenticated_setup_and_switch(void)
{
    const Sc64RaceMap *map=load_canonical_z7();
    assert(race_z7_runtime_init(map));
    int initial_wait=sc64_z7_world_condition(&race_z7_world,&race_z7.runtime,
                                              &sc64_z7_source_conditions[4]);
    assert(initial_wait>=0);
    assert(race_z7_runtime_tick(0,0));
    if(initial_wait) {
        assert(race_z7.runtime.action_ticks[4]==300u);
        assert(!race_z7.runtime.switch10);
    }
    assert(race_z7_runtime_tick(0,1));
    assert(race_z7_runtime_tick(0,2));
    assert(race_z7_runtime_tick(0,3));
    assert(race_minerals[SIDE]==800 && race_gas[SIDE]==200);
    assert(race_z7_world.objective_refresh_requested &&
           race_z7_world.last_string_id==12u);
    assert(race_z7.runtime.model.action_cursor[1]==0u);
    /* Only authored source elapsed/wait ticks advance. */
    for(unsigned f=4;f<9400u;++f) {
        assert(race_z7_runtime_tick(0,f));
        if(initial_wait && f==299u) assert(!race_z7.runtime.switch10);
    }
    assert(race_z7.runtime.switch10);
    assert(race_z7.runtime.ai_script_mask==15u);
    assert(race_z7.runtime.model.action_cursor[3]==0u);
    assert(race_minerals[ENEMY]==7950 && race_gas[ENEMY]==8000 &&
           race_z7.runtime.ai_plan_phase==1u);
    int queued=0;
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive && b[i].owner==ENEMY &&
        b[i].raw_owner==4 && b[i].queue_count) queued=1;
    assert(queued); /* Authored replacement queues only through normal production. */
    assert(sc64_z7_persistent_state_valid(&race_z7));
    u16 held=race_z7.runtime.ai_plan_ticks;
    assert(race_z7_runtime_tick(1,9400u));
    assert(race_z7.runtime.ai_plan_ticks==held);
}

static void test_bring_uses_source_bbox_edges(void)
{
    const Sc64RaceMap *map=load_canonical_z7();
    assert(race_z7_runtime_init(map));
    int slot=-1,left,top,right,bottom;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].raw_owner==3 &&
        u[i].raw_type==38) { slot=i; break; }
    assert(slot>=0 && sc64_z7_world_location(&race_z7_world,3,&left,&top,&right,&bottom));
    int al,at,ar,ab;
    assert(sc64_z7_world_actor_rect(38,0,0,&al,&at,&ar,&ab));
    /* Exactly touching the half-open source region does not satisfy Bring;
       crossing one GBA pixel into the edge does. */
    u[slot].x=(s16)((left*2-ar)/2); u[slot].y=(s16)((top+bottom)/2);
    assert(sc64_z7_world_count(&race_z7_world,18u,229u,3u,&(u32){0}));
    u32 count=0;
    assert(sc64_z7_world_count(&race_z7_world,18u,229u,3u,&count)&&count==0u);
    ++u[slot].x;
    assert(sc64_z7_world_count(&race_z7_world,18u,229u,3u,&count)&&count==1u);
    assert(sc64_z7_world_condition(&race_z7_world,&race_z7.runtime,
                                   &sc64_z7_source_conditions[6])==1);
}

static void test_source_terminal_outcomes(void)
{
    const Sc64RaceMap *map=load_canonical_z7();
    assert(race_z7_runtime_init(map));
    for(int i=0;i<MAX_BUILDINGS;++i)
        if(b[i].alive && b[i].raw_owner==4) b[i].alive=0;
    assert(race_z7_runtime_tick(0,0));
    assert(race_z7.runtime.model.action_cursor[2]==1u);
    assert(race_z7_runtime_tick(0,1));
    assert(race_z7.runtime.model.outcome==SC64_Z7_WON &&
           sc64_z7_persistent_state_valid(&race_z7));

    map=load_canonical_z7();
    assert(race_z7_runtime_init(map));
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive && u[i].raw_owner==3) u[i].alive=0;
    assert(race_z7_runtime_tick(0,0));
    assert(race_z7.runtime.model.action_cursor[0]==1u);
    assert(race_z7_runtime_tick(0,1));
    assert(race_z7.runtime.model.outcome==SC64_Z7_LOST &&
           sc64_z7_persistent_state_valid(&race_z7));
}

static void test_terminal_queries_and_fail_closed_metadata(void)
{
    const Sc64RaceMap *map=load_canonical_z7();
    assert(race_z7_runtime_init(map));
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive && u[i].raw_owner==3) u[i].alive=0;
    u32 count=0;
    assert(sc64_z7_world_count(&race_z7_world,13u,229u,0,&count)&&count==0u);
    int result=sc64_z7_world_condition(&race_z7_world,&race_z7.runtime,
                                        &sc64_z7_source_conditions[0]);
    assert(result==1);
    const Sc64Z7SourceCondition *cond=&sc64_z7_source_conditions[0];
    Sc64Z7SourceCondition bad=*cond; bad.opcode=99;
    assert(sc64_z7_world_condition(&race_z7_world,&race_z7.runtime,&bad)<0);
    Sc64Z7SourceAction bad_action=sc64_z7_source_actions[0];
    bad_action.opcode=43;
    assert(!sc64_z7_world_action(&race_z7_world,&race_z7.runtime,&bad_action));
}

int main(void)
{
    assert(sc64_z7_world_compare(10u,3u,3u)==1);
    assert(sc64_z7_world_compare(10u,3u,2u)==0);
    assert(sc64_z7_world_compare(2u,3u,3u)<0);
    test_canonical_binding_and_source_queries();
    test_authenticated_setup_and_switch();
    test_bring_uses_source_bbox_edges();
    test_source_terminal_outcomes();
    test_terminal_queries_and_fail_closed_metadata();
    return 0;
}
