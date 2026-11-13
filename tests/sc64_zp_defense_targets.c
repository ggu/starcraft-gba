/* Differential full-tick coverage for the opt-in Z/P defense lists.
 * Compile this only with a Z/P storage profile and SC64_ZP_DEFENSE_TARGETS. */
#define SC64_ZP_DEFENSE_TARGETS 1
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_ZP_CAMPAIGN_STORAGE
#error "This test must use the Z/P campaign storage profile"
#endif

typedef struct {
    Unit units[MAX_UNITS];
    Building buildings[MAX_BUILDINGS];
    Tracer tracer_data[MAX_TRACERS];
#ifdef SC64_IMPACT_EVENTS
    Sc64ImpactEvent impacts[RACE_IMPACT_CAPACITY];
    u8 impact_count, impact_high_water;
    u16 impact_deferred;
#endif
    u8 invincible_units[MAX_UNITS], invincible_buildings[MAX_BUILDINGS];
    int supply_used, supply_cap, path_valid;
#ifdef SC64_P5_RUNTIME
    Sc64P5PersistentState p5;
#endif
} DefenseWorld;

static DefenseWorld before, expected, actual;
static u32 rng_state = 0x8a6e39d1u;

static u32 rnd(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void world_save(DefenseWorld *s)
{
    memcpy(s->units, u, sizeof(u));
    memcpy(s->buildings, b, sizeof(b));
    memcpy(s->tracer_data, tracers, sizeof(tracers));
#ifdef SC64_IMPACT_EVENTS
    memcpy(s->impacts, race_impact_rows, sizeof(race_impact_rows));
    s->impact_count = race_impact_count;
    s->impact_high_water = race_impact_high_water;
    s->impact_deferred = race_impact_deferred;
#endif
    memcpy(s->invincible_units, invincible_units, sizeof(invincible_units));
    memcpy(s->invincible_buildings, invincible_buildings, sizeof(invincible_buildings));
    s->supply_used = supply_used;
    s->supply_cap = supply_cap;
    s->path_valid = path_grid_valid;
#ifdef SC64_P5_RUNTIME
    s->p5 = race_p5;
#endif
}

static void world_restore(const DefenseWorld *s)
{
    memcpy(u, s->units, sizeof(u));
    memcpy(b, s->buildings, sizeof(b));
    memcpy(tracers, s->tracer_data, sizeof(tracers));
#ifdef SC64_IMPACT_EVENTS
    memcpy(race_impact_rows, s->impacts, sizeof(race_impact_rows));
    race_impact_count = s->impact_count;
    race_impact_high_water = s->impact_high_water;
    race_impact_deferred = s->impact_deferred;
#endif
    memcpy(invincible_units, s->invincible_units, sizeof(invincible_units));
    memcpy(invincible_buildings, s->invincible_buildings, sizeof(invincible_buildings));
    supply_used = s->supply_used;
    supply_cap = s->supply_cap;
    path_grid_valid = s->path_valid;
#ifdef SC64_P5_RUNTIME
    race_p5 = s->p5;
#endif
}

static void compare_paths(void)
{
    world_save(&before);
#ifdef SC64_DEFENSE_PYLON_ROSTER
    race_defense_scalar_power = 1;
#endif
    race_tick_defenses_fullscan();
    world_save(&expected);
#ifdef SC64_DEFENSE_PYLON_ROSTER
    race_defense_scalar_power = 0;
    world_restore(&before);
    race_tick_defenses_fullscan();
    world_save(&actual);
    assert(!memcmp(&expected, &actual, sizeof(expected)));
#endif
    world_restore(&before);
    race_tick_defenses_zp_targets();
    world_save(&actual);
    assert(!memcmp(&expected, &actual, sizeof(expected)));
    world_restore(&before);
}

static void clear_world(void)
{
    /* Preserve historical differential coverage; the dedicated fog case
     * below clears and seeds the exact visibility cells it needs. */
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        SC64_GRID_SET(visible, cell, 1);
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    clear_tracers();
#ifdef SC64_IMPACT_EVENTS
    memset(race_impact_rows, 0, sizeof(race_impact_rows));
#endif
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    supply_used = supply_cap = 0;
    path_grid_valid = 0;
    race_mode = 0;
    race_human = R_PROTOSS;
    race_bots = 0;
#ifdef SC64_P5_RUNTIME
    memset(&race_p5, 0, sizeof(race_p5));
    race_selected_mission = SC64_P5_GLOBAL_ID;
#elif defined(SC64_P4_RUNTIME)
    race_selected_mission = SC64_P4_GLOBAL_ID;
#elif defined(SC64_Z5_RUNTIME)
    race_selected_mission = 16;
#elif defined(SC64_Z4_RUNTIME)
    race_selected_mission = 15;
#else
    race_selected_mission = 0;
#endif
}

static int random_unit_type(void)
{
    static const int types[] = {
        U_ZERGLING, U_MARINE, U_HYDRALISK, U_MUTALISK, U_WRAITH,
        U_DRAGOON, U_SCOUT, U_ZP_MAP_REVEALER
    };
    return types[rnd() % (sizeof(types) / sizeof(types[0]))];
}

static int random_owner(void)
{
    static const int owners[] = { SIDE, ENEMY, RESCUABLE, NEUTRAL, 4 };
    return owners[rnd() % (sizeof(owners) / sizeof(owners[0]))];
}

static int random_defense_type(void)
{
    static const int types[] = {
        B_PHOTON_CANNON, B_SUNKEN_COLONY, B_SPORE_COLONY,
        B_MISSILE_TURRET, B_PYLON
    };
    return types[rnd() % (sizeof(types) / sizeof(types[0]))];
}

static void fill_random_world(void)
{
    clear_world();
    for (int j = 0; j < MAX_UNITS; ++j) {
        if (rnd() % 5 == 0) continue;
        Unit *q = &u[j];
        q->alive = 1;
        q->owner = (u8)random_owner();
        q->type = (u8)random_unit_type();
        q->x = (s16)(160 + rnd() % 420);
        q->y = (s16)(160 + rnd() % 420);
        q->hp = q->maxhp = 1 + (int)(rnd() % 150);
        q->armor = (u8)(rnd() % 5);
        q->shield = (int)(rnd() % 55);
        q->flags = (u16)(rnd() % 17 == 0 ? 16 : 0);
        invincible_units[j] = (u8)(rnd() % 31 == 0);
#ifdef SC64_T5_RUNTIME
        if ((rnd() % 37) == 0) q->order = ORDER_TRANSPORTED;
#endif
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (rnd() % 4 == 0) continue;
        Building *q = &b[i];
        q->alive = 1;
        q->owner = (u8)random_owner();
        q->type = (u8)random_defense_type();
        q->x = (s16)(170 + rnd() % 400);
        q->y = (s16)(170 + rnd() % 400);
        q->hp = q->maxhp = 300;
        q->under_construction = (u8)(rnd() % 13 == 0);
        q->queue_progress = (u16)(rnd() % 44);
        invincible_buildings[i] = (u8)(rnd() % 29 == 0);
        /* Some cannons are powered; others must remain idle. */
        if (q->type == B_PHOTON_CANNON && (rnd() & 1u) && i + 1 < MAX_BUILDINGS) {
            b[i + 1].alive = 1;
            b[i + 1].owner = q->owner;
            b[i + 1].type = B_PYLON;
            b[i + 1].x = q->x + 16;
            b[i + 1].y = q->y;
            b[i + 1].hp = b[i + 1].maxhp = 300;
            ++i;
        }
    }
    path_grid_valid = (int)(rnd() & 1u);
}

static void randomized_differential(void)
{
    for (int trial = 0; trial < 800; ++trial) {
        fill_random_world();
        compare_paths();
    }
}

static void tie_range_and_early_kill(void)
{
    clear_world();
    b[0] = (Building){ .x=240, .y=240, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1 };
    b[1] = (Building){ .x=240, .y=240, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1 };
    /* The first two equal-distance targets straddle owners and class lists;
     * slot zero wins the tie, and the second defense sees its death. */
    u[0] = (Unit){ .x=256, .y=240, .hp=25, .maxhp=25,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
    u[1] = (Unit){ .x=224, .y=240, .hp=90, .maxhp=90,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
    compare_paths();
    race_tick_defenses_zp_targets();
    assert(!u[0].alive && u[1].hp == 50);

    clear_world();
    b[0] = (Building){ .x=240, .y=240, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SPORE_COLONY, .alive=1 };
    u[0] = (Unit){ .x=352, .y=240, .hp=90, .maxhp=90,
        .owner=ENEMY, .type=U_MUTALISK, .alive=1 };
    compare_paths();
    assert(u[0].hp == 90); /* exactly range is outside the strict radius */
}

static void sunken_explosive_size_shields_and_armor(void)
{
    /* Expected HP packets are asserted independently of the differential
     * snapshot: Sunken's 40 explosive damage is reduced after shields and
     * armor, with the stock size ratio applied last. */
    static const struct {
        int unit_type, raw_type, armor, shield, expected_hp_damage;
    } cases[] = {
        { U_ZERGLING, 37, 0, 0, 20 }, /* small: 40 / 2 */
        { U_HYDRALISK, 38, 0, 0, 30 }, /* medium: 40 * 3 / 4 */
        { U_GOLIATH, 3, 0, 0, 40 }, /* large: full damage */
        { U_DRAGOON, 66, 2, 17, 21 } /* large: 40 - 17 shield - 2 armor */
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        clear_world();
        b[0] = (Building){ .x=240, .y=240, .hp=300, .maxhp=300,
            .owner=SIDE, .type=B_SUNKEN_COLONY, .raw_type=146, .alive=1 };
        u[0] = (Unit){ .x=256, .y=240, .hp=100, .maxhp=100,
            .shield=cases[i].shield, .armor=cases[i].armor,
            .owner=ENEMY, .type=(u8)cases[i].unit_type,
            .raw_type=(s8)cases[i].raw_type, .alive=1 };
        compare_paths();
        race_tick_defenses_zp_targets();
        assert(u[0].hp == 100 - cases[i].expected_hp_damage);
        assert(u[0].shield == 0);
    }
}

static void air_defense_filters_ground_goliath(void)
{
    clear_world();
    b[0] = (Building){ .x=240, .y=240, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SPORE_COLONY, .raw_type=144, .alive=1 };
    /* The closer Goliath is a large ground unit. The farther Mutalisk is the
     * only valid target for the Spore Colony's air weapon. */
    u[0] = (Unit){ .x=248, .y=240, .hp=125, .maxhp=125,
        .owner=ENEMY, .type=U_GOLIATH, .raw_type=3, .alive=1 };
    u[1] = (Unit){ .x=264, .y=240, .hp=100, .maxhp=100,
        .owner=ENEMY, .type=U_MUTALISK, .raw_type=43, .alive=1 };
    compare_paths();
    race_tick_defenses_zp_targets();
    assert(u[0].hp == 125);
    assert(u[1].hp == 85);
    assert(b[0].queue_progress == 19);
}

static void same_update_mutations_precede_defense_query(void)
{
    clear_world();
    b[0] = (Building){ .x=240, .y=240, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_CREEP_COLONY, .raw_type=143,
        .alive=1, .under_construction=1 };
    u[0] = (Unit){ .x=248, .y=240, .hp=10, .maxhp=10,
        .owner=ENEMY, .type=U_ZERGLING, .raw_type=37, .alive=1 };
    u[1] = (Unit){ .x=256, .y=240, .hp=100, .maxhp=100,
        .owner=SIDE, .type=U_MARINE, .raw_type=0, .alive=1 };

    /* Model changes made by the mission/state stage immediately before the
     * defense phase: the nearest enemy died, the Marine changed sides, and
     * the colony finished morphing into a Sunken Colony. */
    u[0].alive = 0;
    u[1].owner = ENEMY;
    b[0].type = B_SUNKEN_COLONY;
    b[0].raw_type = 146;
    b[0].under_construction = 0;

    compare_paths();
    race_tick_defenses_zp_targets();
    assert(!u[0].alive);
    assert(u[1].hp == 80); /* the newly hostile small target receives 20 */
    assert(b[0].queue_progress == 40); /* completed morph fired this tick */
}

static void invalid_owner_falls_back_before_mutation(void)
{
    for (int invalid_kind = 0; invalid_kind < 2; ++invalid_kind) {
        clear_world();
        b[0] = (Building){ .x=240, .y=240, .hp=300, .maxhp=300,
            .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1,
            .queue_progress=3 };
        u[0] = (Unit){ .x=256, .y=240, .hp=90, .maxhp=90,
            .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
        if (invalid_kind == 0) u[MAX_UNITS - 1].alive = 1,
                              u[MAX_UNITS - 1].owner = 7;
        else b[MAX_BUILDINGS - 1].alive = 1,
             b[MAX_BUILDINGS - 1].owner = 7,
             b[MAX_BUILDINGS - 1].type = B_PYLON;
        compare_paths();
        assert(b[0].queue_progress == 3); /* compare restores the input state */
        world_restore(&before);
        race_tick_defenses_zp_targets();
        assert(b[0].queue_progress == 2);
    }
}

static void dispatcher_preserves_p3_and_skirmish_paths(void)
{
    clear_world();
    b[0] = (Building){ .x=240, .y=240, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1 };
    u[0] = (Unit){ .x=256, .y=240, .hp=90, .maxhp=90,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
#ifdef SC64_P3_DEFENSE_TARGETS
    race_selected_mission = SC64_P3_GLOBAL_ID;
    world_save(&before);
    race_tick_defenses_fullscan(); world_save(&expected);
    world_restore(&before); race_tick_defenses(); world_save(&actual);
    assert(!memcmp(&expected, &actual, sizeof(expected)));
#endif
    clear_world();
    b[0] = (Building){ .x=240, .y=240, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1 };
    u[0] = (Unit){ .x=256, .y=240, .hp=90, .maxhp=90,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
    world_save(&before);
    race_tick_defenses_fullscan(); world_save(&expected);
    world_restore(&before); race_tick_defenses(); world_save(&actual);
    assert(!memcmp(&expected, &actual, sizeof(expected)));

    clear_world();
    b[0] = (Building){ .x=240, .y=240, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1 };
    u[0] = (Unit){ .x=256, .y=240, .hp=90, .maxhp=90,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
    race_mode = 1;
    world_save(&before);
    race_tick_defenses_fullscan(); world_save(&expected);
    world_restore(&before); race_tick_defenses(); world_save(&actual);
    assert(!memcmp(&expected, &actual, sizeof(expected)));
}

static void assert_stock_defense_interval(int defense_type, int defense_raw,
                                          int cooldown, int target_type,
                                          int target_raw, int powered,
                                          int skirmish)
{
    clear_world();
    race_mode = skirmish;
    b[0] = (Building){ .x=240, .y=240, .hp=500, .maxhp=500,
        .owner=SIDE, .type=(u8)defense_type, .raw_type=defense_raw,
        .alive=1 };
    if (powered) {
        b[1] = (Building){ .x=240, .y=240, .hp=500, .maxhp=500,
            .owner=SIDE, .type=B_PYLON, .raw_type=156, .alive=1 };
    }
    u[0] = (Unit){ .x=256, .y=240, .hp=1000, .maxhp=1000,
        .owner=ENEMY, .type=(u8)target_type, .raw_type=target_raw,
        .alive=1 };

    race_tick_defenses();
    int first_hp = u[0].hp;
    assert(first_hp < 1000);
    assert(b[0].queue_progress == cooldown);
    /* Both campaign target lists and skirmish fullscan dispatch through
     * this fixture's caller-selected mode. Expiry must attack on tick N. */
    for (int tick = 1; tick < cooldown; ++tick) {
        race_tick_defenses();
        assert(u[0].hp == first_hp);
    }
    race_tick_defenses();
    assert(u[0].hp < first_hp);
    assert(b[0].queue_progress == cooldown);
}

static void stock_defense_attack_intervals(void)
{
    static const struct {
        int building, raw, cooldown, target, target_raw, powered;
    } defenses[] = {
        { B_PHOTON_CANNON, 162, 28, U_MARINE, 0, 1 },
        { B_MISSILE_TURRET, 124, 19, U_MUTALISK, 43, 0 },
        { B_SPORE_COLONY, 144, 19, U_MUTALISK, 43, 0 },
        { B_SUNKEN_COLONY, 146, 40, U_ZERGLING, 37, 0 },
    };
    for (int skirmish = 0; skirmish <= 1; ++skirmish) {
        for (unsigned i = 0; i < sizeof(defenses) / sizeof(defenses[0]); ++i) {
            assert_stock_defense_interval(defenses[i].building, defenses[i].raw,
                defenses[i].cooldown, defenses[i].target, defenses[i].target_raw,
                defenses[i].powered, skirmish);
        }
    }
}

static void human_defense_fog_acquisition(void)
{
    clear_world();
    b[0] = (Building){ .x=400, .y=400, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1 };
    u[0] = (Unit){ .x=416, .y=400, .hp=100, .maxhp=100,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
    u[1] = (Unit){ .x=448, .y=400, .hp=100, .maxhp=100,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
    SC64_GRID_CLEAR(visible);
    SC64_GRID_SET(visible, 448 / 16 + (400 / 16) * MAP_COLS, 1);
    compare_paths();
    race_tick_defenses_zp_targets();
    assert(u[0].hp == 100);
    assert(u[1].hp < 100);
}

#ifdef SC64_DEFENSE_SPATIAL_PROBE
static void spatial_skips_target_killed_earlier_in_same_pass(void)
{
    clear_world();
    /* Both targets are the same distance away, in different cells. The
     * smaller slot wins even though its cell is visited second. Leave it
     * in the cached cell list after death; the next Sunken must skip it. */
    b[0] = (Building){ .x=255, .y=255, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1, .raw_type=146 };
    b[1] = b[0];
    u[0] = (Unit){ .x=270, .y=270, .hp=15, .maxhp=15,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1, .raw_type=37 };
    u[MAX_UNITS-1] = u[0];
    u[MAX_UNITS-1].x = u[MAX_UNITS-1].y = 240;
    u[MAX_UNITS-1].hp = u[MAX_UNITS-1].maxhp = 100;
    compare_paths();
    assert(!actual.units[0].alive);
    assert(actual.units[MAX_UNITS-1].hp == 80);
    assert(actual.buildings[0].queue_progress == 40);
    assert(actual.buildings[1].queue_progress == 40);
}

static void spatial_boundaries_and_ties(void)
{
    clear_world();
    b[0] = (Building){ .x=255, .y=255, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1, .raw_type=-1 };
    u[0] = (Unit){ .x=270, .y=270, .hp=100, .maxhp=100,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1, .raw_type=-1 };
    u[1] = u[0];
    u[1].x = 240;
    u[1].y = 240;
    compare_paths();
    assert(actual.units[0].hp < 100 && actual.units[1].hp == 100);

    /* This target remains in range across the last spatial cell's boundary.
     * Moving it off the covered domain must retain the scalar answer. Use
     * an AI defense so human viewport/fog bounds do not mask the fallback. */
    b[0].owner = ENEMY;
    u[0].owner = SIDE;
    b[0].x = 1937;
    u[0].x = DEFENSE_CELL_LIMIT - 1;
    u[0].y = 255;
    u[1].alive = 0;
    compare_paths();
    assert(actual.units[0].hp < 100);
    u[0].x = DEFENSE_CELL_LIMIT;
    compare_paths();
    assert(actual.units[0].hp < 100);
    ++u[0].x;
    compare_paths();
    assert(actual.units[0].hp == 100);
}
#endif

#ifdef SC64_DEFENSE_PYLON_ROSTER
static void pylon_slot_state_at_defense_boundary(void)
{
    clear_world();
    b[0] = (Building){ .x=400, .y=400, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_PHOTON_CANNON, .alive=1, .raw_type=-1 };
    b[MAX_BUILDINGS-1] = (Building){ .x=432, .y=400, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_PYLON, .alive=1, .raw_type=-1,
        .under_construction=1 };
    u[0] = (Unit){ .x=416, .y=400, .hp=100, .maxhp=100,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1, .raw_type=-1 };

    compare_paths();
    assert(actual.buildings[0].queue_progress == 0);
    b[MAX_BUILDINGS-1].under_construction = 0;
    compare_paths();
    assert(actual.buildings[0].queue_progress == 28);
    b[MAX_BUILDINGS-1].owner = ENEMY;
    compare_paths();
    assert(actual.buildings[0].queue_progress == 0);
    b[MAX_BUILDINGS-1].owner = SIDE;
    b[MAX_BUILDINGS-1].alive = 0;
    compare_paths();
    assert(actual.buildings[0].queue_progress == 0);
    b[0].queue_progress = 3;
    compare_paths();
    assert(actual.buildings[0].queue_progress == 3);
    b[MAX_BUILDINGS-1].alive = 1;
    compare_paths();
    assert(actual.buildings[0].queue_progress == 2);
}
#endif

#if defined(SC64_DEFENSE_SPATIAL_PROBE) && defined(SC64_DEFENSE_PYLON_ROSTER) && \
    defined(SC64_IMPACT_EVENTS)
static void combined_cannons_preserve_projectile_admission(void)
{
    clear_world();
    b[0] = (Building){ .x=255, .y=255, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_PHOTON_CANNON, .alive=1, .raw_type=162 };
    b[1] = b[0];
    b[2] = (Building){ .x=240, .y=255, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_PYLON, .alive=1, .raw_type=156 };
    u[0] = (Unit){ .x=290, .y=290, .hp=100, .maxhp=100,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1, .raw_type=37 };
    u[1] = u[0];
    u[1].x = u[1].y = 220;
    u[2] = u[0];
    u[2].x = u[2].y = 900;
    assert(race_impact_building_steps(&b[0], 0, 0) > 1);
    assert(race_impact_building_steps(&b[0], 1, 0) > 1);

    /* Only one projectile fits. The first cannon must choose the lower
     * slot across the cell boundary; the second remains ready, with no
     * projectile or damage recorded for its deferred shot. */
    for (int i = 0; i < RACE_IMPACT_CAPACITY - 1; ++i)
        race_impact_launch(2, 1, 1, SIDE, 0, U_MARINE, 200u);
    compare_paths();
    assert(actual.impact_count == RACE_IMPACT_CAPACITY);
    assert(actual.impact_deferred == 1);
    assert(sc64_impact_event_target(&actual.impacts[RACE_IMPACT_CAPACITY - 1]) == 0);
    assert(actual.units[0].hp == 100 && actual.units[1].hp == 100);
    assert(actual.buildings[0].queue_progress == 28);
    assert(actual.buildings[1].queue_progress == 0);
}
#endif

int main(void)
{
    gba_init();
    game_init();
    randomized_differential();
#ifdef SC64_DEFENSE_SPATIAL_PROBE
    spatial_boundaries_and_ties();
    spatial_skips_target_killed_earlier_in_same_pass();
#endif
#if !defined(SC64_DEFENSE_PYLON_ROSTER_TEST_ONLY) && !defined(SC64_DEFENSE_SPATIAL_PROBE)
    tie_range_and_early_kill();
    sunken_explosive_size_shields_and_armor();
    air_defense_filters_ground_goliath();
    stock_defense_attack_intervals();
#endif
#ifndef SC64_DEFENSE_PYLON_ROSTER_TEST_ONLY
    same_update_mutations_precede_defense_query();
    invalid_owner_falls_back_before_mutation();
    dispatcher_preserves_p3_and_skirmish_paths();
    human_defense_fog_acquisition();
#endif
#ifdef SC64_DEFENSE_PYLON_ROSTER
    pylon_slot_state_at_defense_boundary();
#endif
#if defined(SC64_DEFENSE_SPATIAL_PROBE) && defined(SC64_DEFENSE_PYLON_ROSTER) && \
    defined(SC64_IMPACT_EVENTS)
    combined_cannons_preserve_projectile_admission();
#endif
    puts("Z/P defense target differential tests passed");
    return 0;
}
