/* Synthetic engine fixtures for the authenticated T4 CUP action subset. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

/* Include game.c without its pending world-adapter glue, then build the same
 * effect/creation chain against the actual engine types and generated data. */
#define SC64_T4_SCENARIO_RUNTIME 1
#include "../starcraft/sc64_t4_runtime.h"
#include "sc64_t4_scenario_generated.h"

static Sc64T4RuntimeState race_t4;
static int race_t4_create_action(const Sc64T4Action *, unsigned);
static int race_t4_doodad_action(const Sc64T4Action *action, unsigned owner)
{ (void)action; (void)owner; return -1; }

#include "../starcraft/sc64_t4_effects_adapter.h"
#include "../starcraft/sc64_t4_creation_adapter.h"

static const Sc64T4Action *
find_create(unsigned raw_type, unsigned location)
{
    for (unsigned t = 0; t < sc64_t4_scenario.trigger_count; ++t) {
        const Sc64T4Action *row = sc64_t4_scenario.triggers[t].actions;
        for (unsigned i = 0; i < 64; ++i)
            if (row[i].opcode == 11 && row[i].unit == raw_type &&
                row[i].location == location) return &row[i];
    }
    return 0;
}

static void
reset_world(void)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(&race_t4, 0, sizeof(race_t4));
    race_mode = 0;
    race_selected_mission = SC64_T4_GLOBAL_ID;
    race_campaign.mission = SC64_T4_GLOBAL_ID;
    race_cols = race_rows = 128;
    race_player[SIDE] = R_TER;
    minerals = 0; gas = 0; supply_used = supply_cap = 0;
    unit_tap = -1; selected_building = -1;
}

static unsigned
live_units(void)
{
    unsigned count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) count += !!u[i].alive;
    return count;
}

static int
center_coord(u32 start, u32 end)
{
    return (int)(((start + end) / 2) / 2);
}

static void
test_source_default_creations(void)
{
    static const struct { unsigned raw, location; } cases[] = {
        {218, 32}, {101, 22}, {101, 23}, {101, 20}
    };
    for (unsigned c = 0; c < sizeof(cases) / sizeof(cases[0]); ++c) {
        const Sc64T4Action *action = find_create(cases[c].raw,
                                                 cases[c].location);
        const Sc64T4Location *location;
        int expected_type, slot;
        assert(action);
        reset_world();
        location = &sc64_t4_scenario.locations[action->location - 1];
        expected_type = sc64_race_unit_from_raw(action->unit);
        assert(expected_type >= 0);
        assert(race_t4_trigger_action(&race_t4, action, 0) == 1);
        assert(live_units() == 1);
        slot = find_unit_slot();
        (void)slot;
        for (slot = 0; slot < MAX_UNITS && !u[slot].alive; ++slot) {}
        assert(slot < MAX_UNITS);
        assert(u[slot].owner == SIDE && u[slot].raw_owner == 0);
        assert(u[slot].raw_type == (int)cases[c].raw);
        assert(u[slot].type == expected_type);
        assert(u[slot].x == center_coord(location->left, location->right));
        assert(u[slot].y == center_coord(location->top, location->bottom));
        assert(u[slot].hp == u[slot].maxhp);
        assert(invincible_units[slot] ==
               sc64_race_t4_unit_intrinsically_invincible(expected_type));
        assert(race_t4_trigger_action(&race_t4, action, 0) == 1);
        assert(live_units() == 2); /* Each successful trigger action is one event. */
    }
}

static void
test_explicit_property_creation(void)
{
    const Sc64T4Action *action = find_create(101, 9);
    const Sc64T4Location *location;
    int slot;
    assert(action);
    reset_world();
    location = &sc64_t4_scenario.locations[action->location - 1];
    assert(sc64_t4_source_setup.property_used[0]);
    assert(sc64_t4_properties[0].hp_percent == 100);
    assert(race_t4_trigger_action(&race_t4, action, 0) == 1);
    assert(live_units() == 1);
    for (slot = 0; slot < MAX_UNITS && !u[slot].alive; ++slot) {}
    assert(slot < MAX_UNITS && u[slot].raw_type == 101 &&
           u[slot].raw_owner == 0);
    assert(u[slot].x == center_coord(location->left, location->right));
    assert(u[slot].y == center_coord(location->top, location->bottom));
    assert(u[slot].hp == u[slot].maxhp);
    assert(invincible_units[slot] == 0); /* UPRP special bits request neither. */
}

static void
test_rejections_are_atomic(void)
{
    const Sc64T4Action *source = find_create(101, 9);
    Sc64T4Action bad;
    static Unit units_before[MAX_UNITS];
    static Building buildings_before[MAX_BUILDINGS];
    static u8 inv_units_before[MAX_UNITS];
    assert(source);
    reset_world();
    bad = *source; bad.location = 99;
    assert(race_t4_create_action(&bad, 0) == -1 && live_units() == 0);
    bad = *source; bad.unit = 218;
    assert(race_t4_create_action(&bad, 0) == -1 && live_units() == 0);
    bad = *source; bad.player = 1;
    assert(race_t4_create_action(&bad, 0) == -1 && live_units() == 0);
    bad = *source; bad.flags ^= 0x04;
    assert(race_t4_create_action(&bad, 0) == -1 && live_units() == 0);
    bad = *source; bad.argument = 2;
    assert(race_t4_create_action(&bad, 0) == -1 && live_units() == 0);
    bad = *source; bad.modifier = 0;
    assert(race_t4_create_action(&bad, 0) == -1 && live_units() == 0);
    assert(race_t4_create_action(source, 8) == -1 && live_units() == 0);

    /* Exhaustion is a clean retry result: no actor, wallet, or pool mutation. */
    for (int i = 0; i < MAX_UNITS; ++i) {
        u[i].alive = 1; u[i].type = U_SCV; u[i].owner = ENEMY;
        u[i].raw_type = 7; u[i].raw_owner = 3; u[i].hp = u[i].maxhp = 60;
    }
    memcpy(units_before, u, sizeof(u));
    memcpy(buildings_before, b, sizeof(b));
    memcpy(inv_units_before, invincible_units, sizeof(invincible_units));
    minerals = 123; gas = 45; supply_used = 6; supply_cap = 10;
    assert(race_t4_create_action(source, 0) == 0);
    assert(memcmp(units_before, u, sizeof(u)) == 0);
    assert(memcmp(buildings_before, b, sizeof(b)) == 0);
    assert(memcmp(inv_units_before, invincible_units,
                  sizeof(invincible_units)) == 0);
    assert(minerals == 123 && gas == 45 && supply_used == 6 && supply_cap == 10);
}

int
main(void)
{
    test_source_default_creations();
    test_explicit_property_creation();
    test_rejections_are_atomic();
    puts("T4 CUP creation adapter fixtures passed (synthetic policy tests)");
    return 0;
}
