#include <assert.h>
#include <string.h>
#include "../common/gba.h"
#define MAX_UNITS 192
#define MAX_BUILDINGS 96
#define SIDE 0
#define ENEMY 1
#define NEUTRAL 2
#define RESCUABLE 3
#define ORDER_IDLE 0
#define ORDER_ATTACK_MOVE 1
#define WORLD_W 1536
#define WORLD_H 1536
static int race_mode, race_selected_mission, race_completed, won, lost;
static int minerals, gas, supply_used, supply_cap, selected_building, cx, cy;
static int race_minerals[5], race_gas[5];
typedef struct {int x,y,hp,maxhp,owner,type,alive,selected,order,tx,ty,raw_type,raw_owner,flags;} Unit;
typedef struct {int x,y,hp,maxhp,owner,type,alive,under_construction,raw_type,raw_owner;} Building;
static Unit u[MAX_UNITS]; static Building b[MAX_BUILDINGS];
static u8 invincible_units[MAX_UNITS];
#include "../starcraft/race_missions.h"
static Sc64RaceCampaignState race_campaign;
static int find_unit_slot(void){for(int i=0;i<MAX_UNITS;i++)if(!u[i].alive)return i;return -1;}
static int spawn_unit(int owner,int type,int x,int y){int i=find_unit_slot();if(i<0)return -1;u[i]=(Unit){.x=x,.y=y,.owner=owner,.type=type,.alive=1,.maxhp=100,.hp=100,.raw_type=-1,.raw_owner=-1};return i;}
static int sc64_race_unit_from_raw(int raw){return raw;}
static int sc64_race_raw_unit(int type){return type;}
static int sc64_race_raw_building(int type){return type;}
static int sc64_race_unit_damage(int type){(void)type;return 1;}
static int sc64_race_is_worker(int type){(void)type;return 0;}
static int unit_supply(int type){(void)type;return 1;}
static int race_owner_supply(int owner){(void)owner;return 0;}
static int race_owner_supply_cap(int owner){(void)owner;return 0;}
#include "../starcraft/sc64_z4_adapter.h"

static void fixture_setup(int include_chrysalis)
{
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(&race_campaign, 0, sizeof(race_campaign));
    memset(race_minerals, 0, sizeof(race_minerals));
    memset(race_gas, 0, sizeof(race_gas));
    race_mode = 0; race_selected_mission = SC64_Z4_GLOBAL_MISSION_ID;
    won = lost = race_completed = 0;
    if (include_chrysalis)
        u[0] = (Unit){.x=256,.y=576,.hp=100,.maxhp=100,.owner=SIDE,.type=150,
                      .alive=1,.raw_type=150,.raw_owner=3};
    u[1] = (Unit){.x=1320,.y=112,.hp=100,.maxhp=100,.owner=ENEMY,.type=37,
                  .alive=1,.raw_type=37,.raw_owner=1};
    u[2] = (Unit){.x=200,.y=1180,.hp=100,.maxhp=100,.owner=ENEMY,.type=37,
                  .alive=1,.raw_type=37,.raw_owner=1};
    b[0] = (Building){.x=1000,.y=500,.hp=100,.maxhp=100,.owner=4,.type=106,
                      .alive=1,.raw_type=106,.raw_owner=2};
    assert(race_z4_init());
}

static void tick(unsigned count)
{
    while (count--) {
        ++race_campaign.elapsed_frames;
        assert(race_z4_campaign_tick());
        assert(!race_z4.faults);
        assert(race_z4_runtime_state_valid(&race_z4));
    }
}

static int find_live_raw_unit(int raw)
{
    for (int i=0;i<MAX_UNITS;i++)
        if (u[i].alive && u[i].raw_type == raw) return i;
    return -1;
}

static void test_hatch_transition_save_and_survival(void)
{
    fixture_setup(1);
    tick(600); /* Startup, Force 2 setup, and authored startup waits. */
    assert(race_z4.countdown_active && race_z4.ai_script_mask == 3);
    race_z4.countdown_ticks = 1; /* Reach the source hatch through its real actions. */
    tick(1);
    assert(race_z4.model.fired & (1u << SC64_Z4_TRIGGER_HATCH_WARNING));
    assert(race_z4.model.fired & (1u << SC64_Z4_TRIGGER_HATCH));
    int found_timer = 0;
    for (unsigned t=0;t<SC64_Z4_TRIGGER_COUNT;++t) if (race_z4.action_ticks[t]) {
        Sc64Z4RuntimeState malformed = race_z4;
        malformed.action_ticks[t] = 36000;
        assert(!race_z4_runtime_state_valid(&malformed));
        found_timer = 1;
    }
    assert(found_timer);

    int observed_transition_checkpoint = 0;
    int observed_invincible_property = 0;
    for (unsigned i=0;i<5000 && race_z4.model.action_cursor[SC64_Z4_TRIGGER_HATCH];i++) {
        tick(1);
        if ((race_z4.hatch_effects & 1u) && !(race_z4.hatch_effects & 2u) &&
            !observed_transition_checkpoint) {
            Sc64Z4RuntimeState saved = race_z4;
            assert(race_z4_runtime_state_valid(&saved));
            Sc64Z4RuntimeState malformed = saved;
            malformed.hatch_effects = 2u; /* Spawn cannot precede source kill. */
            assert(!race_z4_runtime_state_valid(&malformed));
            malformed = saved;
            malformed.hatch_effects = 0;
            assert(!race_z4_runtime_state_valid(&malformed)); /* cursor passed kill */
            malformed = saved;
            malformed.model.action_cursor[SC64_Z4_TRIGGER_HATCH] = 3;
            assert(!race_z4_runtime_state_valid(&malformed)); /* effect cannot precede cursor */
            race_z4 = saved; /* Save/restore the live action cursor and phase bits. */
            observed_transition_checkpoint = 1;
        }
        if (race_z4.model.action_cursor[SC64_Z4_TRIGGER_HATCH] >= 14) {
            int vulture = find_live_raw_unit(19);
            if (vulture >= 0) {
                assert(invincible_units[vulture]);
                observed_invincible_property = 1;
            }
        }
    }
    assert(observed_transition_checkpoint);
    assert(observed_invincible_property);
    assert(!race_z4.model.action_cursor[SC64_Z4_TRIGGER_HATCH]);
    assert(race_z4.hatch_effects == 3);
    assert(race_z4.model.outcome == SC64_Z4_RUNNING && !lost);
    assert(find_live_raw_unit(150) < 0);
    int kerrigan = find_live_raw_unit(51);
    assert(kerrigan >= 0);

    /* The scripted handoff has ended; a real later Kerrigan death still loses. */
    u[kerrigan].alive = 0;
    tick(200);
    assert(race_z4.model.outcome == SC64_Z4_LOST && lost);
}

static void test_chrysalis_loss_before_scripted_removal(void)
{
    fixture_setup(0);
    tick(250);
    assert(race_z4.model.outcome == SC64_Z4_LOST && lost);
    assert(!(race_z4.hatch_effects & 1u));
}

static void test_command_center_objective_dispatches_victory(void)
{
    fixture_setup(1);
    tick(600);
    b[0].alive = 0;
    tick(1800);
    assert(race_z4.model.outcome == SC64_Z4_WON && won && !lost);
}

int main(void)
{
    test_hatch_transition_save_and_survival();
    test_chrysalis_loss_before_scripted_removal();
    test_command_center_objective_dispatches_victory();
    return 0;
}
