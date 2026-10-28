#include <assert.h>
#include <stdio.h>
#include <string.h>
#define SC64_P6_TRAP_CULL 1
#include "../starcraft/game.c"

typedef struct {
    int target;
    Sc64Z5DetectBox box;
    unsigned eligible;
} Sc64P6TrapCandidate;
int sc64_p6_trap_reference_target(const Sc64Z5DetectBox *,
                                  const Sc64P6TrapCandidate *, unsigned,
                                  unsigned);
unsigned sc64_p6_trap_reference_box_distance(const Sc64Z5DetectBox *,
                                             const Sc64Z5DetectBox *);

static u32 rng_state=0x39b7d23du;
static unsigned next_random(void)
{ rng_state^=rng_state<<13;rng_state^=rng_state>>17;rng_state^=rng_state<<5;return rng_state; }

static void reset_world(void)
{
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
    memset(&race_p6,0,sizeof(race_p6));
    memset(race_p6_doodad_bindings,0xff,sizeof(race_p6_doodad_bindings));
    race_mode=0;race_selected_mission=SC64_P6_GLOBAL_ID;
    sc64_p6_runtime_init(&race_p6.runtime);
    race_p6_world.source_current_player=7;
    race_p6_world.map_cols=128;race_p6_world.map_rows=128;
    for(int cell=0;cell<MAP_CELLS;++cell)SC64_GRID_SET(visible,cell,1);
}

static void enable_trap(unsigned slot)
{
    for(unsigned row=0;row<SC64_P6_DOODAD_SOURCE_COUNT;++row) {
        if(sc64_p6_doodads[row].raw_type!=SC64_P6_RAW_WALL_MISSILE_TRAP ||
           sc64_p6_doodads[row].sprite_only) continue;
        race_p6_doodad_bindings[row].building_slot=(u8)slot;
        race_p6.doodad_enabled[row>>3]|=(u8)(1u<<(row&7u));
        return;
    }
    assert(!"wall trap source row exists");
}

static void trap_geometry(void)
{
    Sc64Z5DetectBox trap={2000,2000,16,16,15,15};
    unsigned s=rng_state;
    for(unsigned i=0;i<100000u;++i) {
        Sc64Z5DetectBox target;
        unsigned exact;
        target.x=(int)(next_random()%4096u);target.y=(int)(next_random()%4096u);
        target.left=next_random()%129u;target.up=next_random()%129u;
        target.right=next_random()%129u;target.down=next_random()%129u;
        exact=sc64_p6_trap_reference_box_distance(&trap,&target);
        if(!race_p6_combat_trap_center_may_reach(trap.x,trap.y,target.x,target.y))
            assert(exact>SC64_P6_WALL_TRAP_RANGE_SOURCE_PX);
    }
    assert(race_p6_combat_trap_center_may_reach(-1,2000,4095,0));
    assert(race_p6_combat_trap_center_may_reach(2000,2000,2305,2000));
    assert(!race_p6_combat_trap_center_may_reach(2000,2000,2306,2000));
    rng_state=s;
}

static unsigned actor_extents(int target, Sc64Z5DetectBox *box)
{ return race_p6_combat_box_for_target(target,box); }

static unsigned collect_eligible(Sc64Z5DetectBox *trap,
                                 Sc64P6TrapCandidate *rows,unsigned cap)
{
    unsigned n=0;
    for(int j=0;j<MAX_UNITS+MAX_BUILDINGS;++j) {
        int owner,flying=0;
        Sc64Z5DetectBox box;
        if(j==MAX_UNITS)continue;
        if(j<MAX_UNITS) {
            Unit *q=&u[j];
            if(!q->alive||SC64_TRANSPORT_LOADED(q)||race_p6_combat_unit_burrowed(j))continue;
            owner=q->owner;flying=sc64_race_unit_flying(q->type);
        } else {
            Building *q=&b[j-MAX_UNITS];
            if(!q->alive)continue;
            owner=q->owner;
        }
        if(!race_p6_owners_hostile(b[0].owner,owner)||flying||
           race_p6_combat_target_is_invincible(j)||
           !race_p6_combat_target_detected(j,b[0].owner)||
           !actor_extents(j,&box))continue;
        assert(n<cap);rows[n].target=j;rows[n].box=box;rows[n].eligible=1;++n;
    }
    (void)trap;
    return n;
}

static void assert_actual_matches_exhaustive_reference(unsigned iteration)
{
    Unit original_u[MAX_UNITS],expected_u[MAX_UNITS];
    Building original_b[MAX_BUILDINGS],expected_b[MAX_BUILDINGS];
    u8 original_iu[sizeof(invincible_units)],expected_iu[sizeof(invincible_units)];
    u8 original_ib[sizeof(invincible_buildings)],expected_ib[sizeof(invincible_buildings)];
    Sc64P6PersistentState original_state,expected_state;
    Sc64P6TrapCandidate rows[MAX_UNITS+MAX_BUILDINGS];
    Sc64Z5DetectBox trap_box;
    int expected_target;
    unsigned count;
    reset_world();
    b[0].alive=1;b[0].owner=SIDE;b[0].raw_owner=7;
    b[0].raw_type=SC64_P6_RAW_WALL_MISSILE_TRAP;
    b[0].type=sc64_race_building_from_raw(SC64_P6_RAW_WALL_MISSILE_TRAP);
    b[0].x=500;b[0].y=500;enable_trap(0);

    unsigned units=3u+next_random()%24u;
    for(unsigned i=0;i<units;++i) {
        Unit *q=&u[i];
        static const int raw_types[]={0,37,65,79,84};
        int raw=raw_types[next_random()%5u];
        int px=(int)(next_random()%3600u),py=(int)(next_random()%3600u);
        q->alive=(next_random()%11u)!=0;q->owner=(next_random()%8u)==0?SIDE:ENEMY;
        q->raw_owner=q->owner==SIDE?7:3;q->raw_type=(s16)raw;
        q->type=sc64_race_unit_from_raw(raw);q->x=(s16)(px/2);q->y=(s16)(py/2);
        q->hp=q->maxhp=10000;q->target=-1;q->order=(next_random()%17u)==0?ORDER_TRANSPORTED:ORDER_IDLE;
        if((next_random()%19u)==0)q->flags|=2u;
        if((next_random()%37u)==0)invincible_units[i]=1;
    }
    unsigned buildings=1u+next_random()%12u;
    for(unsigned i=1;i<=buildings;++i) {
        Building *q=&b[i];int raw=(next_random()%3u)==0?110:112;
        q->alive=(next_random()%9u)!=0;q->owner=(next_random()%8u)==0?SIDE:ENEMY;
        q->raw_owner=q->owner==SIDE?7:3;q->raw_type=(s16)raw;
        q->type=sc64_race_building_from_raw(raw);
        q->x=(s16)(next_random()%1800u);q->y=(s16)(next_random()%1800u);
        q->hp=q->maxhp=10000;
        if((next_random()%31u)==0)invincible_buildings[i]=1;
    }
    assert(race_p6_combat_building_box(&b[0],&trap_box));
    count=collect_eligible(&trap_box,rows,MAX_UNITS+MAX_BUILDINGS);
    expected_target=sc64_p6_trap_reference_target(&trap_box,rows,count,
                                      SC64_P6_WALL_TRAP_RANGE_SOURCE_PX);
    memcpy(original_u,u,sizeof(u));memcpy(original_b,b,sizeof(b));
    memcpy(original_iu,invincible_units,sizeof(invincible_units));
    memcpy(original_ib,invincible_buildings,sizeof(invincible_buildings));
    original_state=race_p6;

    if(expected_target>=0 && race_p6_combat_apply_explosive(expected_target,
       SC64_P6_WALL_TRAP_DAMAGE,1u,SC64_P6_TRAP_DAMAGE_SOURCE,b[0].owner))
        b[0].shield_remainder=SC64_P6_WALL_TRAP_COOLDOWN_GBA_TICKS;
    memcpy(expected_u,u,sizeof(u));memcpy(expected_b,b,sizeof(b));
    memcpy(expected_iu,invincible_units,sizeof(invincible_units));
    memcpy(expected_ib,invincible_buildings,sizeof(invincible_buildings));
    expected_state=race_p6;

    memcpy(u,original_u,sizeof(u));memcpy(b,original_b,sizeof(b));
    memcpy(invincible_units,original_iu,sizeof(invincible_units));
    memcpy(invincible_buildings,original_ib,sizeof(invincible_buildings));
    race_p6=original_state;
    assert(race_p6_combat_tick_traps()==(expected_target>=0?1:0));
    assert(memcmp(u,expected_u,sizeof(u))==0);
    assert(memcmp(b,expected_b,sizeof(b))==0);
    assert(memcmp(invincible_units,expected_iu,sizeof(invincible_units))==0);
    assert(memcmp(invincible_buildings,expected_ib,sizeof(invincible_buildings))==0);
    assert(memcmp(&race_p6,&expected_state,sizeof(race_p6))==0);
    (void)iteration;
}

static void player_trap_respects_current_visibility(void)
{
    reset_world();
    b[0].alive=1;b[0].owner=SIDE;b[0].raw_owner=7;
    b[0].raw_type=SC64_P6_RAW_WALL_MISSILE_TRAP;
    b[0].type=sc64_race_building_from_raw(SC64_P6_RAW_WALL_MISSILE_TRAP);
    b[0].x=500;b[0].y=500;enable_trap(0);
    u[0].alive=1;u[0].owner=ENEMY;u[0].raw_owner=3;
    u[0].raw_type=0;u[0].type=U_MARINE;u[0].x=510;u[0].y=500;
    u[0].hp=u[0].maxhp=100;
    SC64_GRID_CLEAR(visible);
    assert(!currently_visible(u[0].x,u[0].y));
    assert(race_p6_combat_tick_traps()==0);
    assert(u[0].hp==100);
    SC64_GRID_SET(visible,u[0].x/16+(u[0].y/16)*MAP_COLS,1);
    assert(race_p6_combat_tick_traps()==1);
    assert(u[0].hp<100);
}

int main(void)
{
    trap_geometry();
    player_trap_respects_current_visibility();
    for(unsigned i=0;i<150u;++i)assert_actual_matches_exhaustive_reference(i);
    puts("P6 wall-trap cull: 100000 box proofs and 150 full-array worlds match exhaustive target selection");
    return 0;
}
