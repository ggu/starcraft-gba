#include <assert.h>
#include <string.h>
#include "../common/gba.h"

#define MAX_UNITS 240
#define MAX_BUILDINGS 48
#define WORLD_W 4096
#define WORLD_H 4096
#define SIDE 0
#define ENEMY 1
#define RESCUABLE 2
#define NEUTRAL 3
#define ORDER_IDLE 0
#define ORDER_ATTACK_MOVE 1
#define SC64_P4_RUNTIME 1
#define SC64_RACE_PHASE_COMPLETE 3
#define SC64_RACE_PHASE_FAILED 4
#define SC64_RACE_OBJECTIVE_PRIMARY 1

typedef struct {
    int alive, owner, raw_type, raw_owner, type, x, y, hp, maxhp, flags;
    int order, tx, ty;
} Unit;
typedef struct { int alive,owner,raw_type,raw_owner,type,x,y; } Building;
typedef struct { int phase,objective_flags,objective_stage; } Campaign;
static Unit u[MAX_UNITS];
static Building b[MAX_BUILDINGS];
static int race_mode,race_selected_mission=23;
static int race_completed,won,lost,cx,cy,minerals,gas,supply_used,supply_cap;
static int race_minerals[8],race_gas[8];
static Campaign race_campaign;

static int find_unit_slot(void) { for(int i=0;i<MAX_UNITS;++i) if(!u[i].alive)return i; return -1; }
static int spawn_unit(int owner,int type,int x,int y) {
    int i=find_unit_slot(); if(i<0)return -1;
    memset(&u[i],0,sizeof(u[i])); u[i].alive=1;u[i].owner=owner;u[i].type=type;
    u[i].x=x;u[i].y=y;u[i].hp=u[i].maxhp=100;u[i].raw_type=type;u[i].raw_owner=owner;
    return i;
}
static int sc64_race_unit_from_raw(int raw) { return raw>0 && raw<230 ? raw : -1; }
static int sc64_race_raw_unit(int type) { return type; }
static int sc64_race_raw_building(int type) { return type; }
static int sc64_race_is_worker(int type) { (void)type; return 0; }
static int sc64_race_unit_damage(int type) { return type ? 1 : 0; }

#include "../starcraft/sc64_p4_adapter.h"

static int add_raw(int owner,int raw,int x,int y)
{
    int i=spawn_unit(owner,raw,x,y); assert(i>=0);u[i].raw_type=raw;u[i].raw_owner=owner==SIDE?7:2;return i;
}

static void reset_world(void)
{
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));memset(&race_campaign,0,sizeof(race_campaign));
    memset(race_minerals,0,sizeof(race_minerals));memset(race_gas,0,sizeof(race_gas));
    race_mode=0;race_selected_mission=SC64_P4_GLOBAL_ID;race_completed=won=lost=0;
    cx=cy=100;supply_used=supply_cap=0;
}

static void test_startup_intro_all_87_and_escape(void)
{
    reset_world();
    assert(strstr(race_p4_objective_text(), "Tassadar") != 0);
    assert(strcmp(race_p4_victory_text(), "TASSADAR AND RAYNOR EXTRACTED") == 0);
    assert(strstr(race_p4_failure_text(), "LOST") != 0);
    /* One human control unit plus the rescued Tassadar. Startup creates
     * Raynor with source provenance slot 2; rescuing transfers runtime owner. */
    add_raw(SIDE,1,100,100);
    int tass=add_raw(RESCUABLE,79,120,120);
    /* Match the authenticated source placement after the beacon's dedicated
     * building classification: 137 unit slots and 32 separate buildings. */
    for (int i=0;i<135;++i) add_raw(SIDE,1,200+i,200);
    for (int i=0;i<32;++i) { b[i].alive=1;b[i].owner=ENEMY;b[i].raw_owner=0;b[i].raw_type=196; }
    assert(race_p4_active() && race_p4_init());
    assert(race_p4_reserved_units()==87 && race_p4_validate());
    for(unsigned tick=0;tick<3;++tick) assert(race_p4_campaign_tick());
    int ray=-1;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].raw_type==19) ray=i;
    assert(ray>=0 && u[ray].raw_owner==2 && u[ray].owner==RESCUABLE);
    u[ray].owner=SIDE;u[tass].owner=SIDE;
    for(unsigned tick=0;tick<2400;++tick) assert(race_p4_campaign_tick());
    assert(race_p4.created_count==87 && race_p4_reserved_units()==0);
    unsigned live_units=0;for(int i=0;i<MAX_UNITS;++i)live_units+=!!u[i].alive;
    assert(live_units==224); /* 137 authenticated start actors + all 87 creates. */
    assert(race_p4.ai_script_mask==1 && race_p4.switch13_set);
    assert(race_p4.switch_bits&1u);
    assert(race_p4_validate());
    /* Let all intro actions and parallel source transmissions finish. */
    for(unsigned tick=0;tick<2200;++tick) assert(race_p4_campaign_tick());
    int l,t,r,b0; assert(race_p4_location(1,&l,&t,&r,&b0));
    u[ray].x=(l+r)/2;u[ray].y=(t+b0)/2;
    u[tass].x=(l+r)/2;u[tass].y=(t+b0)/2;
    for(unsigned tick=0;tick<1000 && race_p4.model.outcome==SC64_P4_RUNNING;++tick)
        assert(race_p4_campaign_tick());
    assert(race_p4.model.outcome==SC64_P4_WON && won && race_p4_validate());
}

static void test_pause_cursor_and_corrupt_save(void)
{
    reset_world(); add_raw(SIDE,1,100,100); add_raw(RESCUABLE,79,120,120);
    assert(race_p4_init());
    assert(race_p4_campaign_tick());
    assert(race_p4.action_cursor[11]==1 && race_p4.action_ticks[11]==1);
    Sc64P4RuntimeState saved=race_p4;
    assert(race_p4_validate());
    Sc64P4RuntimeState malformed=saved;
    malformed.action_ticks[11]=SC64_P4_PRESENTATION_TICKS;
    assert(!race_p4_runtime_state_valid(&malformed)); /* create is a 1-tick action */
    malformed=saved; malformed.created_count=0;
    assert(!race_p4_runtime_state_valid(&malformed)); /* startup create already dispatched */
    race_p4.model.fired ^= 0x8000u;
    assert(!race_p4_validate());
    race_p4=saved;
    assert(race_p4_validate());
    /* A paused game freezes externally: no cursor/timer changes. */
    u16 ticks[SC64_P4_TRIGGER_COUNT]; memcpy(ticks,race_p4.action_ticks,sizeof(ticks));
    assert(memcmp(ticks,race_p4.action_ticks,sizeof(ticks))==0);
}

static void test_raynor_loss_is_sticky(void)
{
    reset_world();
    add_raw(SIDE,1,100,100);
    int tass=add_raw(SIDE,79,120,120);
    assert(race_p4_init());
    for(unsigned tick=0;tick<3;++tick) assert(race_p4_campaign_tick());
    int ray=-1;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].raw_type==19) ray=i;
    assert(ray>=0);u[ray].owner=SIDE;u[tass].owner=SIDE;
    for(unsigned tick=0;tick<1200;++tick) assert(race_p4_campaign_tick());
    u[ray].alive=0;
    for(unsigned tick=0;tick<1000 && race_p4.model.outcome==SC64_P4_RUNNING;++tick)
        assert(race_p4_campaign_tick());
    assert(race_p4.model.outcome==SC64_P4_LOST && lost && race_p4_validate());
    u[ray].alive=1;
    assert(race_p4_campaign_tick() && race_p4.model.outcome==SC64_P4_LOST);
}

static void test_tassadar_loss_uses_its_own_sequence(void)
{
    reset_world(); add_raw(SIDE,1,100,100);
    int tass=add_raw(SIDE,79,120,120);
    assert(race_p4_init());
    for(unsigned tick=0;tick<3;++tick) assert(race_p4_campaign_tick());
    int ray=-1;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].raw_type==19) ray=i;
    assert(ray>=0);u[ray].owner=SIDE;
    for(unsigned tick=0;tick<2;++tick) assert(race_p4_campaign_tick());
    u[tass].alive=0;
    for(unsigned tick=0;tick<1000 && race_p4.model.outcome==SC64_P4_RUNNING;++tick)
        assert(race_p4_campaign_tick());
    assert(race_p4.model.outcome==SC64_P4_LOST && lost && race_p4_validate());
}

int main(void)
{
    test_startup_intro_all_87_and_escape();
    test_pause_cursor_and_corrupt_save();
    test_raynor_loss_is_sticky();
    test_tassadar_loss_uses_its_own_sequence();
    return 0;
}
