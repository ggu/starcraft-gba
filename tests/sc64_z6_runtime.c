#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z6_runtime_adapter.h"

static unsigned hallu_calls, energy_calls, ai_calls;
static unsigned hallu_slot, energy_slot, energy_percent;
static unsigned seen_script, seen_location, seen_player;

static void mark_hallu(void *ctx, unsigned slot)
{ (void)ctx; ++hallu_calls; hallu_slot=slot; u[slot].flags|=8u; }
static void set_energy(void *ctx, unsigned slot, u8 percent)
{ (void)ctx; ++energy_calls; energy_slot=slot; energy_percent=percent;
  u[slot].carry=(int)((250u*256u*percent)/100u); }
static int run_ai(void *ctx, unsigned script, unsigned loc, unsigned player)
{
    (void)ctx; ++ai_calls; seen_script=script; seen_location=loc;
    seen_player=player; return 1;
}

static const Sc64RaceMap *init_map(void)
{
    const Sc64RaceMap *map=&sc64_race_maps[SC64_Z6_GLOBAL_ID];
    race_cols=race_rows=128;race_mode=0;race_selected_mission=SC64_Z6_GLOBAL_ID;
    race_player[ENEMY]=map->owner_races[SC64_Z6_AI_OWNER_SLOT];
    race_minerals[ENEMY]=race_gas[ENEMY]=0;
    supply_used=0;supply_cap=0;selected_building=-1;
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
    memset(m,0,sizeof(m));geyser_count=0;
    hallu_calls=energy_calls=ai_calls=0;
    race_z6_map=0;
    unsigned mineral_slot=0;
    for(unsigned row=0;row<map->unit_count;row++) {
        const Sc64ImportedUnit *src=&map->units[row];
        unsigned raw=src->source_type;
        if(raw==SC64_RAW_START_LOCATION) continue;
        if(raw>=176u&&raw<=178u) {
            assert(mineral_slot<MAX_MINERALS);
            m[mineral_slot].x=src->x_px;m[mineral_slot].y=src->y_px;
            m[mineral_slot].amount=src->resources?src->resources:1;
            m[mineral_slot].style=(int)raw;++mineral_slot;continue;
        }
        if(raw==188u) {
            assert(geyser_count<MAX_RACE_GEYSERS);
            geysers[geyser_count++]=(Geyser){src->x_px,src->y_px};continue;
        }
        int owner=src->owner==map->human_slot?SIDE:map->owner_modes[src->owner];
        int type=sc64_race_unit_from_raw((int)raw);
        if(type>=0) {
            int slot=spawn_unit(owner,type,src->x_px,src->y_px);assert(slot>=0);
            u[slot].raw_type=(s16)raw;u[slot].raw_owner=src->owner;continue;
        }
        type=sc64_race_building_from_raw((int)raw);assert(type>=0);
        int slot=spawn_build(owner,type,src->x_px,src->y_px,0);assert(slot>=0);
        b[slot].raw_type=(s16)raw;b[slot].raw_owner=src->owner;
    }
    race_z6_set_hooks(&(Sc64Z6RuntimeHooks){0,mark_hallu,set_energy,run_ai});
    return map;
}

static void test_bind_owner_queries_and_events(void)
{
    const Sc64RaceMap *map=init_map();
    assert(race_z6_runtime_init(map));
    assert(sizeof(race_z6)<=172);
    assert(race_z6_source_owner(3)==SIDE && race_z6_source_owner(2)==ENEMY &&
           race_z6_source_owner(1)==ENEMY);
    assert(race_z6_world.source_force_for_slot[3]==0);
    unsigned executor=99;
    assert(race_z6_trigger_executor(&race_z6_world,0,&executor)&&executor==3);
    assert(race_z6_trigger_executor(&race_z6_world,8,&executor)&&executor==1);
    int x=0,y=0,r=0,btm=0;
    assert(sc64_z6_world_location(&race_z6_world,1,&x,&y,&r,&btm));
    int slot=spawn_unit(ENEMY,sc64_race_unit_from_raw(79),(x+r)/2,(y+btm)/2);
    assert(slot>=0);u[slot].raw_type=79;u[slot].raw_owner=2;
    u32 count=0;
    assert(sc64_z6_world_count(&race_z6_world,2,79,1,&count)&&count==1);
    assert(sc64_z6_world_count(&race_z6_world,13,79,1,&count)&&count==0);
    race_z6_note_unit_death(&u[slot]);
    assert(race_z6.deaths_by_source[2]==1);
    race_z6.deaths_by_source[2]=SC64_Z6_MAX_SOURCE_DEATHS;
    race_z6_note_unit_death(&u[slot]);
    assert(race_z6.deaths_by_source[2]==SC64_Z6_MAX_SOURCE_DEATHS);
    assert(sc64_z6_persistent_state_valid(&race_z6));
    race_z6.deaths_by_source[2]=SC64_Z6_MAX_SOURCE_DEATHS+1u;
    assert(!sc64_z6_persistent_state_valid(&race_z6));
    race_z6.deaths_by_source[2]=0;
    assert(race_z6_runtime_tick(0,0));
    assert(sc64_z6_persistent_state_valid(&race_z6));
}

static void test_create_property_hooks_and_ai(void)
{
    const Sc64RaceMap *map=init_map();
    assert(race_z6_runtime_init(map));
    const Sc64Z6SourceAction *create=0,*ai=0;
    for(unsigned i=0;i<SC64_Z6_SOURCE_ACTION_COUNT;i++) {
        const Sc64Z6SourceAction *a=&sc64_z6_source_actions[i];
        if(a->opcode==11 && a->unit==79 && a->group2==1) create=a;
        if(a->opcode==16) ai=a;
    }
    assert(create&&ai);
    assert(sc64_z6_world_action(&race_z6_world,create));
    assert(hallu_calls==1&&energy_calls==1&&hallu_slot<MAX_UNITS&&
           energy_slot==hallu_slot&&energy_percent==100);
    assert(u[hallu_slot].alive&&u[hallu_slot].raw_type==79&&
           u[hallu_slot].raw_owner==(int)create->group1);
    assert((u[hallu_slot].flags&8u)!=0&&u[hallu_slot].carry==250*256);
    assert(race_z6_runtime_action(&race_z6_world,ai));
    assert(ai_calls==1&&seen_script==ai->group2&&seen_location==ai->location&&
           seen_player==SC64_Z6_AI_OWNER_SLOT);

    /* Built-in storage applies hallucination/energy; opaque AI remains gated. */
    Sc64Z6RuntimeHooks no_hooks={0};
    race_z6_set_hooks(&no_hooks);
    assert(sc64_z6_world_action(&race_z6_world,create)); /* Built-in flags/carry support. */
    assert(race_z6_runtime_action(&race_z6_world,ai));
}

static int add_source_building(int type, int x, int y)
{
    int slot=spawn_build(ENEMY,type,x,y,0);
    assert(slot>=0);
    b[slot].raw_type=(s16)sc64_race_raw_building(type);
    b[slot].raw_owner=SC64_Z6_AI_OWNER_SLOT;
    return slot;
}

static void test_authored_ai_paid_plan_and_reload(void)
{
    const Sc64RaceMap *map=init_map();
    race_z6_map=0;
    race_z6_set_hooks(0);
    assert(race_z6_runtime_init(map));
    /* Canonical import/binding was checked above. Isolate the paid-producer
     * policy here so an existing source Gateway cannot take the test queue
     * intended for the explicit producer below. */
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
    supply_used=0;
    const Sc64Z6SourceAction *create=0,*ai=0;
    for(unsigned i=0;i<SC64_Z6_SOURCE_ACTION_COUNT;i++) {
        const Sc64Z6SourceAction *a=&sc64_z6_source_actions[i];
        if(a->opcode==11 && a->unit==79 && a->group2==1) create=a;
        if(a->opcode==16) ai=a;
    }
    assert(create&&ai&&sc64_z6_world_action(&race_z6_world,create));
    int found=0;
    for(int i=0;i<MAX_UNITS;i++) if(u[i].alive&&u[i].raw_type==79) {
        found=1;assert((u[i].flags&8u)!=0&&u[i].carry==250*256);
    }
    assert(found);
    /* The replacement checks exact source identity, then uses ordinary queue
       payment and attack-move orders; it never spawns or grants resources. */
    const Sc64Z6SourceAction *resources=0;
    for(unsigned i=0;i<SC64_Z6_SOURCE_ACTION_COUNT;i++)
        if(sc64_z6_source_actions[i].trigger==8&&sc64_z6_source_actions[i].opcode==26)
            resources=&sc64_z6_source_actions[i];
    assert(resources&&race_z6_runtime_action(&race_z6_world,resources));
    assert(race_minerals[ENEMY]==5000&&race_gas[ENEMY]==5000);
    int gateway=add_source_building(B_GATEWAY,1700,1500);
    add_source_building(B_GATEWAY,1710,1510);
    add_source_building(B_NEXUS,1670,1500);
    int stargate=add_source_building(B_STARGATE,1700,1530);
    int robotics=add_source_building(B_ROBOTICS_FACILITY,1720,1530);
    add_source_building(B_PYLON,1680,1480);
    add_source_building(B_CYBERNETICS_CORE,1720,1480);
    int army=spawn_unit(ENEMY,U_DRAGOON,1770,1550);
    assert(army>=0);u[army].raw_type=66;u[army].raw_owner=SC64_Z6_AI_OWNER_SLOT;
    int unrelated=spawn_unit(ENEMY,U_DRAGOON,1800,1600);
    assert(unrelated>=0);
    u[unrelated].raw_owner=-1; /* Explicit untagged fixture actor. */
    unsigned before_units=0;
    for(int i=0;i<MAX_UNITS;i++) before_units+=u[i].alive;
    assert(!race_z6_authored_run_ai(&race_z6_world,SC64_Z6_AI_SCRIPT_ZER6,
                                    SC64_Z6_AI_LOCATION,2));
    assert(!race_z6_authored_run_ai(&race_z6_world,SC64_Z6_AI_SCRIPT_ZER6,
                                    SC64_Z6_AI_LOCATION+1,1));
    int before_m=race_minerals[ENEMY],before_g=race_gas[ENEMY];
    assert(race_z6_runtime_action(&race_z6_world,ai));
    assert(u[army].order==ORDER_ATTACK_MOVE);
    int l,t,r,btm;assert(sc64_z6_world_location(&race_z6_world,3,&l,&t,&r,&btm));
    assert(u[army].tx==(l+r)/2&&u[army].ty==(t+btm)/2);
    assert(u[unrelated].raw_owner<0);
    assert(b[gateway].queue_count==1&&
           b[gateway].queue[b[gateway].queue_head]==U_DRAGOON);
    unsigned after_plan_units=0;
    for(int i=0;i<MAX_UNITS;i++) after_plan_units+=u[i].alive;
    assert(after_plan_units==before_units); /* Queueing creates no free actor. */
    assert(race_minerals[ENEMY]==before_m-125&&race_gas[ENEMY]==before_g-50);
    assert(race_z6.ai_plan_phase==1&&race_z6.ai_plan_ticks==SC64_Z6_AI_INTERVAL_TICKS);

    /* Finish the actual engine queue and reconcile its source identity before
       any subsequent Count/Deaths condition can observe it. */
    const Sc64RaceUnitRule *dragoon_rule=sc64_race_unit_rule(U_DRAGOON);
    assert(dragoon_rule&&dragoon_rule->build_frames>1);
    int product_slot=find_unit_slot();
    assert(product_slot>=0);
    b[gateway].queue_progress=(dragoon_rule->build_frames*5+3)/4-1;
    race_tick_buildings();
    unsigned after_units=0;int new_dragoon=-1;
    for(int i=0;i<MAX_UNITS;i++) if(u[i].alive) {
        ++after_units;
        if(i==product_slot&&u[i].type==U_DRAGOON)new_dragoon=i;
    }
    assert(after_units==before_units+1&&new_dragoon>=0);
    race_z6_reconcile_ai_products();
    assert(u[new_dragoon].raw_owner==SC64_Z6_AI_OWNER_SLOT&&
           u[new_dragoon].raw_type==66);

    /* Model a completed trigger-8 script and continue its private AI timer
       across a persisted-state rebind. */
    /* This isolated producer world has only the few actors above. Mark its
       unrelated production-trigger rows done so their real-source creation
       reservations do not consume this intentionally sparse fixture. */
    race_z6.runtime.model.started |= (u16)((1u<<1)|(1u<<2)|(1u<<3)|
                                           (1u<<4)|(1u<<7)|(1u<<8));
    race_z6.runtime.model.action_cursor[8]=0;
    Sc64Z6PersistentState saved=race_z6;
    memset(&race_z6,0,sizeof(race_z6));race_z6=saved;
    assert(race_z6_rebind_loaded_world(map));
    assert(race_z6_authored_ai_advance(1)); /* User pause holds the plan clock. */
    assert(race_z6.ai_plan_ticks==SC64_Z6_AI_INTERVAL_TICKS);
    for(unsigned tick=0;tick<SC64_Z6_AI_INTERVAL_TICKS-1u;tick++)
        assert(race_z6_authored_ai_advance(0));
    assert(b[gateway].queue_count==0&&race_z6.ai_plan_ticks==1);
    assert(race_z6_authored_ai_advance(0));
    assert(b[gateway].queue_count==1);
    assert(race_z6.ai_plan_ticks==SC64_Z6_AI_INTERVAL_TICKS);
    assert(race_z6.ai_plan_phase==2);
    assert(race_minerals[ENEMY]==before_m-125-100&&
           race_gas[ENEMY]==before_g-50);
    race_z6.ai_plan_ticks=1;
    assert(race_z6_authored_ai_advance(0));
    assert(b[stargate].queue_count==1&&
           b[stargate].queue[b[stargate].queue_head]==U_SCOUT);
    assert(race_z6.ai_plan_phase==3&&
           race_minerals[ENEMY]==before_m-125-100-275&&
           race_gas[ENEMY]==before_g-50-125);
    race_z6.ai_plan_ticks=1;
    assert(race_z6_authored_ai_advance(0));
    assert(b[robotics].queue_count==1&&
           b[robotics].queue[b[robotics].queue_head]==U_REAVER);
    assert(race_z6.ai_plan_phase==0&&
           race_minerals[ENEMY]==before_m-125-100-275-200&&
           race_gas[ENEMY]==before_g-50-125-100);
    assert(sc64_z6_persistent_state_valid(&race_z6));
    race_z6.ai_plan_phase=4;
    assert(!sc64_z6_persistent_state_valid(&race_z6));
    race_z6.ai_plan_phase=2;
    race_z6.ai_plan_ticks=241;
    assert(!sc64_z6_persistent_state_valid(&race_z6));
}

static void test_move_location_roundtrip_and_invalid_map(void)
{
    const Sc64RaceMap *map=init_map();
    assert(race_z6_runtime_init(map));
    const Sc64Z6SourceAction *move=0;
    for(unsigned i=0;i<SC64_Z6_SOURCE_ACTION_COUNT;i++)
        if(sc64_z6_source_actions[i].opcode==38) { move=&sc64_z6_source_actions[i];break; }
    assert(move&&sc64_z6_world_action(&race_z6_world,move));
    race_z6.moved_locations_mask=race_z6_world.moved_locations_mask;
    memcpy(race_z6.moved_location,race_z6_world.moved_location,
           sizeof(race_z6.moved_location));
    assert(sc64_z6_persistent_state_valid(&race_z6));
    Sc64Z6WorldContext rebound;
    Sc64Z6RuntimeBinding binding;
    assert(race_z6_bind_world(map,&race_z6,&binding));
    rebound=binding.world;
    int l,t,r,bottom;
    assert(sc64_z6_world_location(&rebound,move->group2,&l,&t,&r,&bottom));
    assert(r>l&&bottom>t);
    race_z6.moved_location[0][2]=0;
    assert(!sc64_z6_persistent_state_valid(&race_z6));
    Sc64RaceMap invalid=*map;invalid.width=64;
    assert(!race_z6_map_shape_valid(&invalid));
}

int main(void)
{
    const Sc64RaceMap *canonical=&sc64_race_maps[SC64_Z6_GLOBAL_ID];
    race_mode=0;
    race_selected_mission=SC64_Z6_GLOBAL_ID;
    race_z6_map=0;
    /* The Z6 map contains a raw-172 Shield Battery. Until this profile has
       its own explicit building identity, the canonical map must stay
       fail-closed. Owner resolution must nevertheless work before binding,
       because the map importer consults it during initial placement. */
    assert(race_z6_source_owner(1)==ENEMY);
    assert(race_z6_source_owner(3)==SIDE);
    if (sc64_race_building_from_raw(172)<0) {
        assert(!race_z6_map_shape_valid(canonical));
        return 0;
    }
    test_bind_owner_queries_and_events();
    test_create_property_hooks_and_ai();
    test_authored_ai_paid_plan_and_reload();
    test_move_location_roundtrip_and_invalid_map();
    return 0;
}
