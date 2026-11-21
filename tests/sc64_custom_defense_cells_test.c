/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Full pass differential on independent scalar and Custom cell paths. */
#define main sc64_original_defense_fixture_main
#include "sc64_zp_defense_targets.c"
#undef main

static void compare_custom(void)
{
#ifdef SC64_PROJECTILE_DEPTH
    static Sc64TraceSlot trace_before[MAX_TRACERS], trace_expected[MAX_TRACERS];
    memcpy(trace_before, sc64_trace_slots, sizeof(trace_before));
#endif
    u32 seed_before = seed, seed_expected;
    race_mode = 1;
    world_save(&before);
    race_tick_defenses_fullscan();
    world_save(&expected);
    seed_expected = seed;
#ifdef SC64_PROJECTILE_DEPTH
    memcpy(trace_expected, sc64_trace_slots, sizeof(trace_expected));
    memcpy(sc64_trace_slots, trace_before, sizeof(trace_before));
#endif
    world_restore(&before);
    seed = seed_before;
    race_tick_defenses();
    world_save(&actual);
    assert(!memcmp(&expected, &actual, sizeof(actual)));
    assert(seed == seed_expected);
#ifdef SC64_PROJECTILE_DEPTH
    assert(!memcmp(trace_expected, sc64_trace_slots, sizeof(trace_expected)));
    memcpy(sc64_trace_slots, trace_before, sizeof(trace_before));
#endif
    world_restore(&before);
    seed = seed_before;
}

static void random_cases(void)
{
    for (int i = 0; i < 800; ++i) {
        fill_random_world();
        race_mode = 1;
        race_bots = 3;
        if (i % 13 == 0)
            u[MAX_UNITS - 1].alive = 1, u[MAX_UNITS - 1].x = -1;
        if (i % 17 == 0)
            u[MAX_UNITS - 1].alive = 1, u[MAX_UNITS - 1].y = DEFENSE_CELL_LIMIT;
        if (i % 19 == 0)
            u[MAX_UNITS - 1].alive = 1, u[MAX_UNITS - 1].owner = 7;
        compare_custom();
    }
    puts("800 randomized full-pass cases matched");
}

static void edge_cases(void)
{
    clear_world();
    race_mode = 1;
    race_bots = 1;
    b[0] = (Building){.x = 255,
                      .y = 255,
                      .hp = 300,
                      .maxhp = 300,
                      .owner = SIDE,
                      .type = B_SUNKEN_COLONY,
                      .alive = 1,
                      .raw_type = 146};
    b[1] = b[0];
    u[0] = (Unit){.x = 270,
                  .y = 270,
                  .hp = 15,
                  .maxhp = 15,
                  .owner = ENEMY,
                  .type = U_ZERGLING,
                  .alive = 1,
                  .raw_type = 37};
    u[1] = u[0];
    u[1].x = 240;
    u[1].y = 240;
    u[1].hp = u[1].maxhp = 100;
    compare_custom();
    assert(!actual.units[0].alive && actual.units[1].hp == 80);
    assert(actual.buildings[0].queue_progress == 40 && actual.buildings[1].queue_progress == 40);
    /* Strict circular range, visibility, cooldown and out-of-domain fallback. */
    b[1].alive = 0;
    u[0].x = 367;
    u[0].y = 255;
    u[0].hp = 100;
    u[1].alive = 0;
    compare_custom();
    assert(actual.units[0].hp == 100);
    u[0].x = 300;
    SC64_GRID_CLEAR(visible);
    compare_custom();
    assert(actual.units[0].hp == 100);
    SC64_GRID_SET(visible, 300 / 16 + (255 / 16) * MAP_COLS, 1);
    b[0].queue_progress = 3;
    compare_custom();
    assert(actual.units[0].hp == 100 && actual.buildings[0].queue_progress == 2);
    b[0].queue_progress = 0;
    u[MAX_UNITS - 1].alive = 1;
    u[MAX_UNITS - 1].x = -1;
    u[MAX_UNITS - 1].y = 0;
    compare_custom();
    assert(actual.units[0].hp < 100);
    puts("tie, early kill, range, visibility, cooldown and fallback matched");
}

#if defined(SC64_DEFENSE_PYLON_ROSTER) && defined(SC64_IMPACT_EVENTS)
static void saturated_impact(void)
{
    clear_world();
    race_mode = 1;
    race_bots = 1;
    b[0] = (Building){.x = 255,
                      .y = 255,
                      .hp = 300,
                      .maxhp = 300,
                      .owner = SIDE,
                      .type = B_PHOTON_CANNON,
                      .alive = 1,
                      .raw_type = 162};
    b[1] = b[0];
    b[2] = (Building){.x = 240,
                      .y = 255,
                      .hp = 300,
                      .maxhp = 300,
                      .owner = SIDE,
                      .type = B_PYLON,
                      .alive = 1,
                      .raw_type = 156};
    u[0] = (Unit){.x = 270,
                  .y = 270,
                  .hp = 100,
                  .maxhp = 100,
                  .owner = ENEMY,
                  .type = U_ZERGLING,
                  .alive = 1,
                  .raw_type = 37};
    u[1] = u[0];
    u[1].x = 240;
    u[1].y = 240;
    u[2] = u[0];
    u[2].x = u[2].y = 900;
    for (int i = 0; i < RACE_IMPACT_CAPACITY - 1; ++i)
        race_impact_launch(2, 1, 1, SIDE, 0, U_MARINE, 200u);
    compare_custom();
    assert(actual.impact_count == RACE_IMPACT_CAPACITY && actual.impact_deferred == 1);
    assert(sc64_impact_event_target(&actual.impacts[RACE_IMPACT_CAPACITY - 1]) == 0);
    assert(actual.buildings[0].queue_progress == 28 && !actual.buildings[1].queue_progress);
    puts("saturated projectile pool preserved lower-slot target and firing order");
}
#endif

int main(void)
{
    gba_init();
    game_init();
    random_cases();
    edge_cases();
#if defined(SC64_DEFENSE_PYLON_ROSTER) && defined(SC64_IMPACT_EVENTS)
    saturated_impact();
#endif
    puts("custom defense scalar/cell differential passed");
    return 0;
}
