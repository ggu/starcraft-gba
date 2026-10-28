/* Differential test for the opt-in P3 defense candidate-list optimization. */
#define SC64_P3_DEFENSE_TARGETS 1
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

typedef struct {
    Unit units[MAX_UNITS];
    Building buildings[MAX_BUILDINGS];
    Tracer tracer_data[MAX_TRACERS];
    int supply_used, supply_cap, path_valid;
} DefenseWorld;

static void world_save(DefenseWorld *s)
{
    memcpy(s->units, u, sizeof(u));
    memcpy(s->buildings, b, sizeof(b));
    memcpy(s->tracer_data, tracers, sizeof(tracers));
    s->supply_used = supply_used;
    s->supply_cap = supply_cap;
    s->path_valid = path_grid_valid;
}

static void world_restore(const DefenseWorld *s)
{
    memcpy(u, s->units, sizeof(u));
    memcpy(b, s->buildings, sizeof(b));
    memcpy(tracers, s->tracer_data, sizeof(tracers));
    supply_used = s->supply_used;
    supply_cap = s->supply_cap;
    path_grid_valid = s->path_valid;
}

static void world_equal(const DefenseWorld *a, const DefenseWorld *b_)
{
    assert(!memcmp(a->units, b_->units, sizeof(a->units)));
    assert(!memcmp(a->buildings, b_->buildings, sizeof(a->buildings)));
    assert(!memcmp(a->tracer_data, b_->tracer_data, sizeof(a->tracer_data)));
    assert(a->supply_used == b_->supply_used);
    assert(a->supply_cap == b_->supply_cap);
    assert(a->path_valid == b_->path_valid);
}

static void compare_paths(void)
{
    DefenseWorld initial, reference, optimized;
    world_save(&initial);
    race_tick_defenses_fullscan();
    world_save(&reference);
    world_restore(&initial);
    race_tick_defenses_p3_targets();
    world_save(&optimized);
    world_equal(&reference, &optimized);
    world_restore(&initial);
}

static unsigned rng_state = 0x9e3779b9u;
static unsigned rnd(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static int random_type(void)
{
    static const int types[] = { U_ZERGLING, U_MARINE, U_MUTALISK, U_WRAITH,
                                 U_DRAGOON, U_HYDRALISK, U_SCOUT };
    return types[rnd() % (sizeof(types) / sizeof(types[0]))];
}

static void clear_world(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(tracers, 0, sizeof(tracers));
    race_mode = 0;
    race_selected_mission = SC64_P3_GLOBAL_ID;
    race_human = R_PROTOSS;
    race_bots = 0;
    supply_used = supply_cap = 0;
    path_grid_valid = 0;
}

static void test_randomized_worlds(void)
{
    for (int trial = 0; trial < 1200; ++trial) {
        clear_world();
        int units = (int)(rnd() % (MAX_UNITS + 1));
        int buildings = (int)(rnd() % (MAX_BUILDINGS + 1));
        for (int i = 0; i < units; ++i) {
            static const int owners[] = { SIDE, ENEMY, 4, 2, RESCUABLE };
            Unit *q = &u[i];
            q->alive = (rnd() % 5) != 0;
            q->owner = owners[rnd() % 5];
            q->type = random_type();
            q->x = (int)(rnd() % 768);
            q->y = (int)(rnd() % 768);
            q->hp = q->maxhp = (int)(1 + rnd() % 110);
            q->armor = (int)(rnd() % 4);
            q->shield = (int)(rnd() % 25);
            q->flags = (rnd() % 11 == 0) ? 16 : 0;
        }
        for (int i = 0; i < buildings; ++i) {
            static const int owners[] = { SIDE, ENEMY, 4, 2, RESCUABLE };
            static const int types[] = { B_SUNKEN_COLONY, B_SPORE_COLONY,
                                         B_MISSILE_TURRET, B_PHOTON_CANNON,
                                         B_HATCHERY };
            Building *q = &b[i];
            q->alive = (rnd() % 7) != 0;
            q->owner = owners[rnd() % 5];
            q->type = types[rnd() % 5];
            q->x = (int)(rnd() % 768);
            q->y = (int)(rnd() % 768);
            q->hp = q->maxhp = 1 + (int)(rnd() % 600);
            q->queue_progress = (int)(rnd() % 44);
            q->under_construction = (rnd() % 9 == 0);
            q->shield = (int)(rnd() % 40);
        }
        compare_paths();
    }
}

static void test_ties_and_death_between_defenses(void)
{
    clear_world();
    b[0] = (Building){ .x=200, .y=200, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1 };
    b[1] = (Building){ .x=204, .y=200, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1 };
    /* Equal distance must keep slot zero as the target; it then dies before
       building one selects, exercising the per-defense alive recheck. */
    u[0] = (Unit){ .x=220, .y=200, .hp=35, .maxhp=35,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
    u[1] = (Unit){ .x=180, .y=200, .hp=100, .maxhp=100,
        .owner=4, .type=U_ZERGLING, .alive=1 };
    compare_paths();
    race_tick_defenses_p3_targets();
    assert(!u[0].alive && u[1].hp == 60);
}

static void test_dense_maximum_and_other_mode_dispatch(void)
{
    clear_world();
    for (int i = 0; i < MAX_UNITS; ++i) {
        u[i] = (Unit){ .x=180 + (i % 16) * 12, .y=180 + (i / 16) * 12,
            .hp=500, .maxhp=500, .owner=(i % 2) ? ENEMY : SIDE,
            .type=(i % 4) ? U_ZERGLING : U_MUTALISK, .alive=1 };
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        b[i] = (Building){ .x=200 + (i % 8) * 8, .y=200 + (i / 8) * 8,
            .hp=500, .maxhp=500, .owner=SIDE,
            .type=(i % 2) ? B_SUNKEN_COLONY : B_SPORE_COLONY, .alive=1 };
    }
    compare_paths();

    /* The dispatcher must preserve the full scan outside the P3 runtime. */
    clear_world();
    race_mode = 1;
    u[0] = (Unit){ .x=220, .y=200, .hp=100, .maxhp=100,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
    b[0] = (Building){ .x=200, .y=200, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1 };
    DefenseWorld initial, expected;
    world_save(&initial);
    race_tick_defenses_fullscan();
    world_save(&expected);
    world_restore(&initial);
    race_tick_defenses();
    DefenseWorld actual;
    world_save(&actual);
    world_equal(&expected, &actual);

    clear_world();
    race_selected_mission = 21;
    u[0] = (Unit){ .x=220, .y=200, .hp=100, .maxhp=100,
        .owner=ENEMY, .type=U_ZERGLING, .alive=1 };
    b[0] = (Building){ .x=200, .y=200, .hp=300, .maxhp=300,
        .owner=SIDE, .type=B_SUNKEN_COLONY, .alive=1 };
    world_save(&initial);
    race_tick_defenses_fullscan();
    world_save(&expected);
    world_restore(&initial);
    race_tick_defenses();
    world_save(&actual);
    world_equal(&expected, &actual);
}

static void test_human_defense_fog_acquisition(void)
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
    race_tick_defenses_p3_targets();
    assert(u[0].hp == 100);
    assert(u[1].hp < 100);
}

int main(void)
{
    gba_init();
    game_init();
    test_randomized_worlds();
    test_ties_and_death_between_defenses();
    test_dense_maximum_and_other_mode_dispatch();
    test_human_defense_fog_acquisition();
    puts("P3 defense target differential tests passed");
    return 0;
}
