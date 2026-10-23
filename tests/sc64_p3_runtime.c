#define SC64_P3_RUNTIME 1
#define SC64_P3_ENABLED 1
#define SC64_UNIFIED 1

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.h"
#include "../starcraft/race_missions.h"
#include "../starcraft/sc64_p3_model.h"

enum { MAX_UNITS = 64, MAX_BUILDINGS = 24, SIDE = 0, ENEMY = 1,
       ORDER_IDLE = 0, ORDER_ATTACK_MOVE = 2, ORDER_MINE = 4, U_ZERGLING = 4,
       U_DRONE = 7, U_HYDRALISK = 5, B_HATCHERY = 6, B_LAIR = 25, B_HIVE = 26 };
enum { U_SCOURGE = 27 };
typedef struct { int alive, owner, type, order, x, y, tx, ty, target, work_timer; } Unit;
typedef struct {
    int alive, owner, type, x, y, under_construction, queue_count, queue_head;
    int queue[5], larva;
} Building;

static Unit u[MAX_UNITS];
static Building b[MAX_BUILDINGS];
typedef struct { int x, y, amount; } Mineral;
static Mineral m[8];
static int minerals, gas, race_minerals[5], race_gas[5];
static int race_completed, won, lost, cx = 100, cy = 100;
static Sc64RaceCampaignState race_campaign;
static Sc64P3State race_p3;
static u16 race_p3_action_ticks;
static int queue_calls[5];

static int building_complete(int i)
{ return i >= 0 && i < MAX_BUILDINGS && b[i].alive && !b[i].under_construction; }
static int sc64_race_is_zerg_townhall(int t)
{ return t == B_HATCHERY || t == B_LAIR || t == B_HIVE; }
static int sc64_race_is_worker(int t) { return t == U_DRONE; }
static int sc64_race_unit_damage(int t) { return t == U_ZERGLING || t == U_HYDRALISK ? 5 : 0; }
static int sc64_race_unit_damage_vs(int t, int flying)
{ return t == U_SCOURGE && flying ? 110 : sc64_race_unit_damage(t); }
static int nearest_mineral(int x, int y)
{
    int i, best = -1, best_d = 0x7fffffff;
    for (i = 0; i < 8; ++i) if (m[i].amount > 0) {
        int dx = x - m[i].x, dy = y - m[i].y, d = dx * dx + dy * dy;
        if (d < best_d) { best = i; best_d = d; }
    }
    return best;
}
static int race_queue_unit(int owner, int producer, int type)
{
    Building *p;
    if ((owner != ENEMY && owner != 4) || !building_complete(producer) ||
        b[producer].owner != owner || type != U_ZERGLING || b[producer].queue_count >= 5 ||
        race_minerals[owner] < 50) return 0;
    p = &b[producer];
    --p->larva;
    race_minerals[owner] -= 50;
    p->queue[(p->queue_head + p->queue_count) % 5] = type;
    ++p->queue_count;
    ++queue_calls[owner];
    return 1;
}

#include "../starcraft/sc64_p3_adapter.h"

static void reset_fixture(void)
{
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(race_minerals, 0, sizeof(race_minerals));
    memset(race_gas, 0, sizeof(race_gas));
    memset(&race_campaign, 0, sizeof(race_campaign));
    memset(queue_calls, 0, sizeof(queue_calls));
    minerals = gas = race_completed = won = lost = 0;
    race_p3_init();
}

static void test_init_sampling_and_isolated_force_starts(void)
{
    reset_fixture();
    assert(minerals == 300 && gas == 100);
    assert(race_p3.outcome == SC64_P3_RUNNING && race_campaign.objective_stage == 0);
    /* Building-only force presence satisfies source AnyUnit selectors. */
    b[0] = (Building){ .alive=1, .owner=ENEMY, .type=B_HATCHERY, .larva=3 };
    b[1] = (Building){ .alive=1, .owner=4, .type=B_HATCHERY, .larva=3 };
    b[2] = (Building){ .alive=1, .owner=2, .type=0 };
    race_campaign.elapsed_frames = 1;
    race_p3_campaign_tick();
    assert(race_minerals[ENEMY] == 5000 && race_gas[ENEMY] == 5000);
    assert(race_minerals[4] == 8000 && race_gas[4] == 8000);
    assert(race_minerals[2] == 0 && race_gas[2] == 0);
    /* One-shot grants never overwrite a wallet after its first fire. */
    race_minerals[ENEMY] = 321; race_gas[4] = 654;
    race_p3_campaign_tick();
    assert(race_minerals[ENEMY] == 321 && race_gas[4] == 654);
}

static void test_two_force_paid_production_and_waves(void)
{
    reset_fixture();
    b[0] = (Building){ .alive=1, .owner=ENEMY, .type=B_HATCHERY, .larva=3 };
    b[1] = (Building){ .alive=1, .owner=4, .type=B_HATCHERY, .larva=3 };
    u[0] = (Unit){ .alive=1, .owner=SIDE, .type=U_DRONE, .x=44, .y=55 };
    u[1] = (Unit){ .alive=1, .owner=ENEMY, .type=U_ZERGLING };
    u[2] = (Unit){ .alive=1, .owner=4, .type=U_HYDRALISK };
    race_campaign.elapsed_frames = SC64_P3_FORCE_PRODUCTION_PERIOD;
    race_p3_campaign_tick();
    assert(queue_calls[ENEMY] == 1 && queue_calls[4] == 1);
    assert(race_minerals[ENEMY] == 4950 && race_minerals[4] == 7950);
    assert(race_minerals[2] == 0);
    race_campaign.elapsed_frames = SC64_P3_FORCE_ATTACK_PERIOD;
    race_p3_campaign_tick();
    assert(u[1].order == ORDER_ATTACK_MOVE && u[2].order == ORDER_ATTACK_MOVE);
    assert(u[1].tx == 44 && u[2].tx == 44 && race_campaign.enemy_units_sent == 2);
}

static void test_reserve_mining_and_air_only_wave(void)
{
    int i;
    reset_fixture();
    m[0] = (Mineral){ .x=10, .y=10, .amount=500 };
    m[1] = (Mineral){ .x=100, .y=100, .amount=500 };
    b[0] = (Building){ .alive=1, .owner=ENEMY, .type=B_HATCHERY, .larva=3 };
    b[1] = (Building){ .alive=1, .owner=4, .type=B_HATCHERY, .larva=3 };
    b[2] = (Building){ .alive=1, .owner=SIDE, .type=B_HATCHERY };
    u[0] = (Unit){ .alive=1, .owner=ENEMY, .type=U_DRONE, .x=8, .y=9 };
    u[1] = (Unit){ .alive=1, .owner=4, .type=U_DRONE, .x=95, .y=98 };
    race_p3_init();
    assert(u[0].order == ORDER_MINE && u[0].target == 0);
    assert(u[1].order == ORDER_MINE && u[1].target == 1);

    /* At the global reserve boundary, only one Zergling pair fits. The second
     * owner must not queue after the first queue consumes the remaining slots. */
    for (i = 2; i < 46; ++i) u[i] = (Unit){ .alive=1, .owner=2, .type=0 };
    race_campaign.elapsed_frames = SC64_P3_FORCE_PRODUCTION_PERIOD;
    race_p3_campaign_tick();
    assert(race_p3_live_and_queued() == 48);
    assert(queue_calls[ENEMY] == 1 && queue_calls[4] == 0);

    reset_fixture();
    b[0] = (Building){ .alive=1, .owner=SIDE, .type=B_HATCHERY };
    u[0] = (Unit){ .alive=1, .owner=ENEMY, .type=U_SCOURGE };
    race_campaign.elapsed_frames = SC64_P3_FORCE_ATTACK_PERIOD;
    race_p3_campaign_tick();
    assert(u[0].order == ORDER_ATTACK_MOVE);
}

static void test_victory_sequence_and_defeat_preemption(void)
{
    unsigned i;
    reset_fixture();
    b[0] = (Building){ .alive=1, .owner=SIDE, .type=B_HATCHERY };
    race_p3_campaign_tick();
    assert(race_p3.sequence == SC64_P3_SEQ_WIN_WAIT_2000);
    assert(race_p3_action_ticks == 60 && race_campaign.objective_stage == race_p3.sequence);
    /* All foes' buildings are gone, but the two forces can still have units;
     * model win timing runs while it samples each force start. */
    u[0] = (Unit){ .alive=1, .owner=ENEMY, .type=U_ZERGLING };
    u[1] = (Unit){ .alive=1, .owner=4, .type=U_HYDRALISK };
    for (i = 0; i < 60; ++i) race_p3_campaign_tick();
    assert(race_p3.sequence == SC64_P3_SEQ_WIN_TX_1 && race_p3_action_ticks == 90);
    for (i = 0; i < 90; ++i) race_p3_campaign_tick();
    assert(race_p3.sequence == SC64_P3_SEQ_WIN_TX_2 && race_p3_action_ticks == 90);
    for (i = 0; i < 90; ++i) race_p3_campaign_tick();
    assert(race_p3.sequence == SC64_P3_SEQ_WIN_WAIT_3000 && race_p3_action_ticks == 90);
    for (i = 0; i < 89; ++i) race_p3_campaign_tick();
    assert(race_p3.outcome == SC64_P3_RUNNING && race_p3_action_ticks == 1);
    race_p3_campaign_tick();
    assert(race_p3.outcome == SC64_P3_WON && won && !lost);
    assert(race_campaign.phase == SC64_RACE_PHASE_COMPLETE);
    assert(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY);
    assert(race_completed & (1 << SC64_P3_GLOBAL_ID));
    assert(race_campaign.objective_stage == SC64_P3_SEQ_DONE_WIN && !race_p3_action_ticks);
    race_p3_campaign_tick();
    assert(won && !lost);

    reset_fixture();
    b[0] = (Building){ .alive=1, .owner=SIDE, .type=B_HATCHERY };
    race_p3_campaign_tick();
    assert(race_p3.sequence == SC64_P3_SEQ_WIN_WAIT_2000);
    memset(b, 0, sizeof(b)); memset(u, 0, sizeof(u));
    race_p3_campaign_tick(); /* defeat preempts the pending victory sequence */
    assert(race_p3.outcome == SC64_P3_LOST && lost && !won);
    assert(race_campaign.phase == SC64_RACE_PHASE_FAILED);
    assert(race_campaign.objective_stage == SC64_P3_SEQ_DONE_LOSS);
    assert(!(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY));
}

static void test_pause_freezes_by_outer_dispatch_contract(void)
{
    reset_fixture();
    b[0] = (Building){ .alive=1, .owner=SIDE, .type=B_HATCHERY };
    race_p3_campaign_tick();
    assert(race_p3_action_ticks == 60);
    /* The runtime adapter is intentionally not called while the game is
     * paused; its timer and trigger state consequently remain unchanged. */
    race_campaign.elapsed_frames += 900;
    assert(race_p3_action_ticks == 60 && race_p3.sequence == SC64_P3_SEQ_WIN_WAIT_2000);
}

int main(void)
{
    test_init_sampling_and_isolated_force_starts();
    test_two_force_paid_production_and_waves();
    test_reserve_mining_and_air_only_wave();
    test_victory_sequence_and_defeat_preemption();
    test_pause_freezes_by_outer_dispatch_contract();
    puts("P3 runtime adapter tests passed");
    return 0;
}
