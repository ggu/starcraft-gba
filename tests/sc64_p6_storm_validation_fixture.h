#ifndef SC64_P6_STORM_VALIDATION_FIXTURE_H
#define SC64_P6_STORM_VALIDATION_FIXTURE_H

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.h"
#include "../starcraft/sc64_psionic_storm.h"

#define SC64_P6_RUNTIME 1
#define SC64_P6_STORM_ENABLED 1
#define SC64_P6_GLOBAL_ID 25
#define SC64_P6_RUNNING 0
#define SC64_GAME_ACTOR_HIDDEN 128u
#define SC64_GAME_ACTOR_BUILDING 8u
#define SC64_GAME_ACTOR_INVINCIBLE 64u
#define SC64_TRANSPORT_LOADED(q) 0
#define MAX_UNITS 240
#define MAX_BUILDINGS 160
#define ORDER_IDLE 0
#define WORLD_W 1024
#define WORLD_H 1024
#define SIDE 0
#define RESCUABLE 3
#define ENEMY 1
#define NEUTRAL 2
#define R_PROTOSS 2
#define C_CYAN 1
#define C_WHITE 2

typedef struct {
    int alive, owner, raw_type, carry, work_timer, selected;
    int hp, shield, flags, type, x, y, order, target, tx, ty, raw_owner;
} Unit;
typedef Unit Building;
typedef struct { int outcome; } TestModel;
typedef struct { TestModel model; } TestRuntime;
typedef struct {
    TestRuntime runtime;
    Sc64PsionicStormState storm;
    u32 deaths_by_source[12];
} TestP6;

static Unit u[MAX_UNITS];
static Building b[MAX_BUILDINGS];
static int invincible_units[MAX_UNITS], invincible_buildings[MAX_BUILDINGS];
static TestP6 race_p6;
static int race_human = R_PROTOSS, selected_building = -1;
static int supply_used, camx, camy, test_active, test_runtime_valid;
static int test_validator_calls, test_death_count;

static void invalidate_path_grid(void) { }
static int race_p6_active(void) { return test_active; }
static int race_p6_runtime_state_valid(void)
{
    ++test_validator_calls;
    if (!test_runtime_valid || race_p6.runtime.model.outcome > 1) return 0;
    for (unsigned i = 0; i < 12; ++i)
        if (race_p6.deaths_by_source[i] > 65535u) return 0;
    return 1;
}
static void race_p6_note_unit_death(const Unit *q)
{
    assert(q && q->alive);
    ++test_death_count;
    if (q->raw_owner >= 0 && q->raw_owner < 12 &&
        race_p6.deaths_by_source[q->raw_owner] < 65535u)
        ++race_p6.deaths_by_source[q->raw_owner];
}
static void race_p6_note_building_death(const Building *q)
{ assert(q && q->alive); }
static unsigned sc64_game_unit_actor_flags(int type)
{ (void)type; return 0; }
static int sc64_race_unit_flying(int type) { (void)type; return 0; }
static int unit_supply(int type) { (void)type; return 2; }

#ifdef SC64_P6_STORM_VALIDATION_CANDIDATE
#define SC64_P6_STORM_POSTCHECK_LOCAL 1
#endif
#include "../starcraft/sc64_p6_storm_adapter.h"

#define SC64_CAT2_(a,b) a##b
#define SC64_CAT2(a,b) SC64_CAT2_(a,b)
#define SC64_EXPORT(suffix) SC64_CAT2(SC64_TEST_PREFIX,suffix)

static u32 test_rng;
static u32 test_rand(void)
{
    test_rng ^= test_rng << 13;
    test_rng ^= test_rng >> 17;
    test_rng ^= test_rng << 5;
    return test_rng;
}

static void test_reset(unsigned seed, unsigned mode)
{
    static const int raw_types[] = {0, 1, 2, 37, 38, 42, 65, 66, 74, 75, 79,
                                    84, 101, 195, 196, 203, 209, 210, 211, 213};
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(&race_p6, 0, sizeof(race_p6));
    race_human = R_PROTOSS;
    selected_building = -1;
    supply_used = camx = camy = test_death_count = 0;
    test_active = 1;
    test_runtime_valid = 1;
    test_validator_calls = 0;
    test_rng = seed ? seed : 0x63a9f17bu;
    race_p6.runtime.model.outcome = mode == 3 ? 1 : SC64_P6_RUNNING;

    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        q->owner = (int)(test_rand() % 5u);
        q->raw_owner = (int)(test_rand() % 12u);
        q->raw_type = raw_types[test_rand() % (sizeof(raw_types)/sizeof(raw_types[0]))];
        q->type = q->raw_type;
        q->alive = (test_rand() % 11u) != 0;
        q->x = 500 + (int)(test_rand() % 49u) - 24;
        q->y = 500 + (int)(test_rand() % 49u) - 24;
        q->hp = (test_rand() % 7u) ? 90 : 1;
        q->shield = (int)(test_rand() % 10u);
        q->carry = q->raw_type == 79 ? 200 * SC64_STORM_ENERGY_ONE : 0;
        q->work_timer = q->raw_type == 79 ? (int)(test_rand() % 5u) : 0;
        q->selected = 0;
        q->flags = 0;
        q->order = ORDER_IDLE;
        q->target = -1;
        q->tx = q->x; q->ty = q->y;
        if ((test_rand() % 31u) == 0) invincible_units[i] = 1;
    }
    /* Guarantee live Tassadars with every owner representation in the full
       actor pool, so the all-raw-79 postcondition is exercised independently
       of the SIDE-only caster regeneration path. */
    for (int owner = 0; owner < 5; ++owner) {
        Unit *q = &u[owner];
        q->alive = 1; q->owner = owner; q->raw_type = 79;
        q->carry = 200 * SC64_STORM_ENERGY_ONE; q->work_timer = owner % 5;
        q->hp = 80; q->x = 500; q->y = 500;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *q = &b[i];
        q->alive = (test_rand() % 4u) == 0;
        q->owner = (int)(test_rand() % 5u);
        q->raw_owner = (int)(test_rand() % 12u);
        q->raw_type = raw_types[test_rand() % (sizeof(raw_types)/sizeof(raw_types[0]))];
        q->type = q->raw_type;
        q->x = 500 + (int)(test_rand() % 49u) - 24;
        q->y = 500 + (int)(test_rand() % 49u) - 24;
        q->hp = (test_rand() % 5u) ? 30 : 1;
        q->shield = (int)(test_rand() % 6u);
        if ((test_rand() % 19u) == 0) invincible_buildings[i] = 1;
    }
    if (mode == 1) test_runtime_valid = 0;
    if (mode == 2 || mode == 4 || mode == 8) {
        u[MAX_UNITS - 1].alive = 1;
        u[MAX_UNITS - 1].owner = ENEMY;
        u[MAX_UNITS - 1].raw_type = 79;
        u[MAX_UNITS - 1].carry = 200 * SC64_STORM_ENERGY_ONE;
        u[MAX_UNITS - 1].work_timer = 0;
    }
    if (mode == 2) u[MAX_UNITS - 1].carry = -1; /* invalid non-SIDE raw 79 */
    if (mode == 4) u[MAX_UNITS - 1].work_timer = 5;
    if (mode == 8) u[MAX_UNITS - 1].carry =
        (SC64_STORM_HERO_MAX_ENERGY + 1) * SC64_STORM_ENERGY_ONE;
    if (mode == 5) test_active = 0;
    if (mode == 6) test_runtime_valid = 0; /* corrupted persistent-state preflight */

    if (mode != 1 && mode != 2 && mode != 4 && mode != 5 && mode != 6) {
        /* A valid, nearly due effect guarantees pulse/damage/death paths. */
        race_p6.storm.storm[0].active = 1;
        race_p6.storm.storm[0].x = 500;
        race_p6.storm.storm[0].y = 500;
        race_p6.storm.storm[0].age_ticks = SC64_STORM_FIRST_PULSE_TICKS - 1;
        race_p6.storm.storm[0].pulses_left = SC64_STORM_PULSE_COUNT;
    }
    if (mode == 3) race_p6.runtime.model.outcome = 1;
    if (mode == 7) race_p6.storm.storm[0].x = WORLD_W;
    if (mode == 9) race_p6.runtime.model.outcome = 2;
    if (mode == 10) race_p6.deaths_by_source[1] = 65536u;
}

static unsigned test_snapshot(u32 *out, unsigned cap)
{
    unsigned n = 0;
#define PUT(v) do { if (n < cap) out[n] = (u32)(v); ++n; } while (0)
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        PUT(q->alive); PUT(q->owner); PUT(q->raw_type); PUT(q->carry);
        PUT(q->work_timer); PUT(q->selected); PUT(q->hp); PUT(q->shield);
        PUT(q->flags); PUT(q->type); PUT(q->x); PUT(q->y); PUT(q->order);
        PUT(q->target); PUT(q->tx); PUT(q->ty); PUT(q->raw_owner);
        PUT(invincible_units[i]);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const Building *q = &b[i];
        PUT(q->alive); PUT(q->owner); PUT(q->raw_type); PUT(q->carry);
        PUT(q->work_timer); PUT(q->selected); PUT(q->hp); PUT(q->shield);
        PUT(q->flags); PUT(q->type); PUT(q->x); PUT(q->y); PUT(q->order);
        PUT(q->target); PUT(q->tx); PUT(q->ty); PUT(q->raw_owner);
        PUT(invincible_buildings[i]);
    }
    PUT(race_p6.runtime.model.outcome);
    for (unsigned i = 0; i < 12; ++i) PUT(race_p6.deaths_by_source[i]);
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        const Sc64PsionicStorm *q = &race_p6.storm.storm[i];
        PUT(q->x); PUT(q->y); PUT(q->age_ticks); PUT(q->pulses_left);
        PUT(q->active);
    }
    PUT(supply_used); PUT(test_death_count); PUT(test_active);
    PUT(test_runtime_valid);
#undef PUT
    return n;
}

void SC64_EXPORT(_reset)(unsigned seed, unsigned mode) { test_reset(seed, mode); }
int SC64_EXPORT(_tick)(int paused) { return race_p6_storm_tick(paused); }
unsigned SC64_EXPORT(_snapshot)(u32 *out, unsigned cap) { return test_snapshot(out, cap); }
int SC64_EXPORT(_validator_calls)(void) { return test_validator_calls; }
int SC64_EXPORT(_death_count)(void) { return test_death_count; }
u32 SC64_EXPORT(_death_source)(unsigned source)
{ return source < 12u ? race_p6.deaths_by_source[source] : 0xffffffffu; }
void SC64_EXPORT(_force_deaths)(void)
{
    race_p6.deaths_by_source[2] = 65535u;
    u[7].alive = 1; u[7].owner = ENEMY; u[7].raw_type = 37;
    u[7].raw_owner = 2; u[7].hp = 1; u[7].x = 500; u[7].y = 500;
    u[8].alive = 1; u[8].owner = ENEMY; u[8].raw_type = 37;
    u[8].raw_owner = 3; u[8].hp = 1; u[8].x = 500; u[8].y = 500;
}
void SC64_EXPORT(_malformed_dead_hero)(void)
{
    u[MAX_UNITS - 1].alive = 0;
    u[MAX_UNITS - 1].raw_type = 79;
    u[MAX_UNITS - 1].carry = -1;
    u[MAX_UNITS - 1].work_timer = 5;
}
int SC64_EXPORT(_post_validator)(void)
{
#ifdef SC64_P6_STORM_POSTCHECK_LOCAL
    return race_p6_storm_post_state_valid();
#else
    return race_p6_storm_world_state_valid();
#endif
}

#ifndef SC64_P6_STORM_VALIDATION_REFERENCE
#include <stdio.h>
void sc64_p6_storm_reference_reset(unsigned, unsigned);
void sc64_p6_storm_reference_force_deaths(void);
void sc64_p6_storm_reference_malformed_dead_hero(void);
int sc64_p6_storm_reference_tick(int);
unsigned sc64_p6_storm_reference_snapshot(u32 *, unsigned);
int sc64_p6_storm_reference_validator_calls(void);
int sc64_p6_storm_reference_post_validator(void);
int sc64_p6_storm_reference_death_count(void);
u32 sc64_p6_storm_reference_death_source(unsigned);

int main(void)
{
    u32 candidate[12000], reference[12000];
    for (unsigned mode = 0; mode <= 10; ++mode) {
        for (unsigned i = 0; i < 120; ++i) {
            unsigned seed = 0x91f2a7u + i * 0x9e3779b9u;
            int paused = (i % 13u) == 0;
            sc64_p6_storm_reference_reset(seed, mode);
            SC64_EXPORT(_reset)(seed, mode);
            int expected = sc64_p6_storm_reference_tick(paused);
            int actual = SC64_EXPORT(_tick)(paused);
            assert(actual == expected);
            unsigned nr = sc64_p6_storm_reference_snapshot(reference, 12000);
            unsigned nc = SC64_EXPORT(_snapshot)(candidate, 12000);
            assert(nr == nc && nr <= 12000);
            if (memcmp(candidate, reference, nr * sizeof(u32)) != 0) {
                for (unsigned k = 0; k < nr; ++k) if (candidate[k] != reference[k]) {
                    fprintf(stderr, "snapshot mismatch mode=%u case=%u word=%u fast=%u full=%u\n",
                            mode, i, k, candidate[k], reference[k]);
                    break;
                }
                assert(0);
            }
            if (mode == 0 && !paused) {
                assert(sc64_p6_storm_reference_validator_calls() == 2);
                assert(SC64_EXPORT(_validator_calls)() == 1);
            } else {
                int expected_calls = mode == 5 ? 0 : 1;
                if (sc64_p6_storm_reference_validator_calls() != expected_calls ||
                    SC64_EXPORT(_validator_calls)() != expected_calls)
                    fprintf(stderr, "validator calls mode=%u paused=%d full=%d local=%d expected=%d\n",
                        mode, paused, sc64_p6_storm_reference_validator_calls(),
                        SC64_EXPORT(_validator_calls)(), expected_calls);
                assert(sc64_p6_storm_reference_validator_calls() == expected_calls);
                assert(SC64_EXPORT(_validator_calls)() == expected_calls);
            }
        }
    }
    sc64_p6_storm_reference_reset(77u, 0u);
    SC64_EXPORT(_reset)(77u, 0u);
    sc64_p6_storm_reference_force_deaths();
    SC64_EXPORT(_force_deaths)();
    assert(sc64_p6_storm_reference_tick(0) == SC64_EXPORT(_tick)(0));
    unsigned nr = sc64_p6_storm_reference_snapshot(reference, 12000);
    unsigned nc = SC64_EXPORT(_snapshot)(candidate, 12000);
    assert(nr == nc && memcmp(candidate, reference, nr * sizeof(u32)) == 0);
    assert(sc64_p6_storm_reference_death_count() >= 2);
    assert(SC64_EXPORT(_death_count)() == sc64_p6_storm_reference_death_count());
    assert(SC64_EXPORT(_death_source)(2) == 65535u);
    assert(SC64_EXPORT(_death_source)(2) == sc64_p6_storm_reference_death_source(2));
    assert(SC64_EXPORT(_death_source)(3) == sc64_p6_storm_reference_death_source(3));
    /* Verify the local postcondition still sees all-owner live raw-79 rows. */
    SC64_EXPORT(_reset)(123u, 0u);
    u[MAX_UNITS - 1].alive = 1; u[MAX_UNITS - 1].owner = ENEMY;
    u[MAX_UNITS - 1].raw_type = 79;
    u[MAX_UNITS - 1].carry = -1;
    assert(!SC64_EXPORT(_post_validator)());
    SC64_EXPORT(_reset)(124u, 0u);
    u[MAX_UNITS - 1].alive = 1; u[MAX_UNITS - 1].owner = ENEMY;
    u[MAX_UNITS - 1].raw_type = 79;
    u[MAX_UNITS - 1].work_timer = 5;
    assert(!SC64_EXPORT(_post_validator)());
    sc64_p6_storm_reference_reset(125u, 0u);
    SC64_EXPORT(_reset)(125u, 0u);
    sc64_p6_storm_reference_malformed_dead_hero();
    SC64_EXPORT(_malformed_dead_hero)();
    assert(SC64_EXPORT(_post_validator)());
    assert(sc64_p6_storm_reference_post_validator());
    puts("P6 Storm local post-check differential tests passed");
    return 0;
}
#endif

#endif
