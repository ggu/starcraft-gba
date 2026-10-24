/* Isolated P6 Storm adapter contract tests. This fixture supplies the small
 * engine/runtime surface the adapter uses; root integration tests exercise
 * normal game input and the real P6 death/save hooks. */
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
#define MAX_UNITS 4
#define WORLD_W 1024
#define WORLD_H 1024
#define SIDE 0
#define RESCUABLE 1
#define ENEMY 2
#define R_PROTOSS 2
#define C_CYAN 1
#define C_WHITE 2

typedef struct {
    int alive, owner, raw_type, carry, work_timer, selected;
    int hp, shield, flags, type, x, y;
} Unit;
static Unit u[MAX_UNITS];
static int invincible_units[MAX_UNITS];
static int race_human = R_PROTOSS, selected_building = -1;
static int supply_used, camx, camy;
static int death_count;

typedef struct { struct { int outcome; } model; } TestP6Runtime;
typedef struct { TestP6Runtime runtime; Sc64PsionicStormState storm; } TestP6;
static TestP6 race_p6;

static int race_p6_active(void) { return 1; }
static int race_p6_runtime_state_valid(void) { return 1; }
static void race_p6_note_unit_death(const Unit *q) { assert(q && q->alive); ++death_count; }
static unsigned sc64_game_unit_actor_flags(int type) { (void)type; return 0; }
static int sc64_race_unit_flying(int type) { (void)type; return 0; }
static int unit_supply(int type) { (void)type; return 2; }
void gfx_line(int x1, int y1, int x2, int y2, u8 color)
{ (void)x1; (void)y1; (void)x2; (void)y2; (void)color; }

#include "../starcraft/sc64_p6_storm_adapter.h"

static void reset_fixture(void)
{
    memset(u, 0, sizeof(u));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(&race_p6, 0, sizeof(race_p6));
    death_count = supply_used = 0;
    u[0].alive = 1; u[0].owner = SIDE; u[0].raw_type = 79;
    u[0].hp = 100; u[0].x = 300; u[0].y = 300; u[0].selected = 1;
    u[1].alive = 1; u[1].owner = ENEMY; u[1].raw_type = 37;
    u[1].hp = 50; u[1].x = 300; u[1].y = 300;
    u[2].alive = 1; u[2].owner = SIDE; u[2].raw_type = 65;
    u[2].hp = 20; u[2].shield = 5; u[2].x = 300; u[2].y = 300;
    race_p6_storm_initialize_source_units();
}

static void test_authenticated_hero_energy_and_menu(void)
{
    reset_fixture();
    assert(u[0].carry == 250 * SC64_STORM_ENERGY_ONE && u[0].work_timer == 0);
    assert(race_p6_storm_max_energy(79) == SC64_STORM_HERO_MAX_ENERGY);
    assert(race_p6_storm_energy_valid(&u[0]));
    assert(race_p6_storm_menu_available());
    u[2].selected = 1;
    assert(!race_p6_storm_menu_available());
    u[2].selected = 0;
    u[0].owner = RESCUABLE;
    assert(!race_p6_storm_menu_available());
    u[0].owner = SIDE;
    u[0].carry = 0;
    assert(!race_p6_storm_menu_available());
}

static void test_cast_range_pause_and_pulses(void)
{
    reset_fixture();
    int energy = u[0].carry;
    assert(!race_p6_storm_cast_selected(445, 300)); /* 145px > 144px range */
    assert(u[0].carry == energy);
    assert(race_p6_storm_cast_selected(300, 300));
    assert(u[0].carry == energy - 75 * SC64_STORM_ENERGY_ONE);
    for (int i = 0; i < 19; ++i) assert(race_p6_storm_tick(0));
    assert(u[1].hp == 50 && race_p6.storm.storm[0].age_ticks == 19);
    int paused_energy = u[0].carry;
    assert(race_p6_storm_tick(1));
    assert(u[0].carry == paused_energy && race_p6.storm.storm[0].age_ticks == 19);
    assert(race_p6_storm_tick(0));
    assert(u[1].hp == 36 && u[2].shield == 0 && u[2].hp == 11);
    for (int i = 0; i < 10; ++i) assert(race_p6_storm_tick(0));
    assert(u[1].hp == 22 && u[2].hp == 0 && !u[2].alive);
    assert(death_count == 1 && supply_used == -2);
}

static void test_friendly_fire_and_invincibility(void)
{
    reset_fixture();
    u[1].owner = SIDE;
    invincible_units[1] = 1;
    assert(race_p6_storm_cast_selected(300, 300));
    for (int i = 0; i < 20; ++i) assert(race_p6_storm_tick(0));
    assert(u[1].hp == 50);
    assert(u[2].hp == 11 && u[2].shield == 0);
    assert(death_count == 0);
}

static void test_world_validation_and_effect_expiry(void)
{
    reset_fixture();
    race_p6.storm.storm[0].active = 1;
    race_p6.storm.storm[0].x = WORLD_W;
    race_p6.storm.storm[0].pulses_left = SC64_STORM_PULSE_COUNT;
    assert(!race_p6_storm_world_state_valid());
    sc64_psionic_storm_init(&race_p6.storm);
    assert(race_p6_storm_cast_selected(300, 300));
    for (int i = 0; i < 90; ++i) assert(race_p6_storm_tick(0));
    assert(!race_p6.storm.storm[0].active);
    assert(race_p6_storm_world_state_valid());
    race_p6_storm_draw();
}

int main(void)
{
    test_authenticated_hero_energy_and_menu();
    test_cast_range_pause_and_pulses();
    test_friendly_fire_and_invincibility();
    test_world_validation_and_effect_expiry();
    puts("P6 Storm adapter tests passed");
    return 0;
}
