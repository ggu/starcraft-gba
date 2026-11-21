/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Compare Custom Game's shared cell pass to its scalar pass on Terran. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

typedef struct {
    Unit units[MAX_UNITS];
    Building buildings[MAX_BUILDINGS];
    Tracer shots[MAX_TRACERS];
#ifdef SC64_PROJECTILE_DEPTH
    Sc64TraceSlot trace[MAX_TRACERS];
#endif
#ifdef SC64_IMPACT_EVENTS
    Sc64ImpactEvent impacts[RACE_IMPACT_CAPACITY];
    u8 count, high;
    u16 deferred;
#endif
    u8 inv_units[MAX_UNITS], inv_buildings[MAX_BUILDINGS];
    int supply, cap, path;
    u32 rng;
} Sample;
static Sample before, expected, actual;
static u32 random_state = 0x8520d2a1u;
static u32 random32(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}
static void save(Sample *s)
{
    memcpy(s->units, u, sizeof(u));
    memcpy(s->buildings, b, sizeof(b));
    memcpy(s->shots, tracers, sizeof(tracers));
#ifdef SC64_PROJECTILE_DEPTH
    memcpy(s->trace, sc64_trace_slots, sizeof(sc64_trace_slots));
#endif
#ifdef SC64_IMPACT_EVENTS
    memcpy(s->impacts, race_impact_rows, sizeof(race_impact_rows));
    s->count = race_impact_count;
    s->high = race_impact_high_water;
    s->deferred = race_impact_deferred;
#endif
    memcpy(s->inv_units, invincible_units, sizeof(invincible_units));
    memcpy(s->inv_buildings, invincible_buildings, sizeof(invincible_buildings));
    s->supply = supply_used;
    s->cap = supply_cap;
    s->path = path_grid_valid;
    s->rng = seed;
}
static void restore(const Sample *s)
{
    memcpy(u, s->units, sizeof(u));
    memcpy(b, s->buildings, sizeof(b));
    memcpy(tracers, s->shots, sizeof(tracers));
#ifdef SC64_PROJECTILE_DEPTH
    memcpy(sc64_trace_slots, s->trace, sizeof(sc64_trace_slots));
#endif
#ifdef SC64_IMPACT_EVENTS
    memcpy(race_impact_rows, s->impacts, sizeof(race_impact_rows));
    race_impact_count = s->count;
    race_impact_high_water = s->high;
    race_impact_deferred = s->deferred;
#endif
    memcpy(invincible_units, s->inv_units, sizeof(invincible_units));
    memcpy(invincible_buildings, s->inv_buildings, sizeof(invincible_buildings));
    supply_used = s->supply;
    supply_cap = s->cap;
    path_grid_valid = s->path;
    seed = s->rng;
}
static void compare(void)
{
    save(&before);
    race_tick_defenses_fullscan();
    save(&expected);
    restore(&before);
    race_tick_defenses();
    save(&actual);
    assert(!memcmp(&expected, &actual, sizeof(actual)));
    restore(&before);
}
int main(void)
{
    static const int types[] = {U_MARINE, U_SCV, U_ZERGLING, U_HYDRALISK, U_WRAITH, U_MUTALISK};
    static const int defenses[] = {B_MISSILE_TURRET, B_SUNKEN_COLONY, B_SPORE_COLONY,
                                   B_PHOTON_CANNON, B_PYLON};
    gba_init();
    game_init();
    race_mode = 1;
    race_human = R_TER;
    race_bots = 3;
    for (int trial = 0; trial < 800; ++trial) {
        memset(u, 0, sizeof(u));
        memset(b, 0, sizeof(b));
        memset(invincible_units, 0, sizeof(invincible_units));
        memset(invincible_buildings, 0, sizeof(invincible_buildings));
        clear_tracers();
#ifdef SC64_IMPACT_EVENTS
        memset(race_impact_rows, 0, sizeof(race_impact_rows));
        race_impact_count = race_impact_high_water = 0;
        race_impact_deferred = 0;
#endif
        memset(visible, 0xff, sizeof(visible));
        for (int i = 0; i < MAX_UNITS; ++i)
            if (random32() % 5) {
                Unit *q = &u[i];
                q->alive = 1;
                q->owner = random32() % 5;
                q->type = types[random32() % (sizeof(types) / sizeof(types[0]))];
                q->raw_type = -1;
                q->raw_owner = -1;
                q->x = 160 + random32() % 420;
                q->y = 160 + random32() % 420;
                q->hp = q->maxhp = 1 + random32() % 150;
                q->flags = random32() % 17 == 0 ? 16 : 0;
            }
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (random32() % 4) {
                Building *q = &b[i];
                q->alive = 1;
                q->owner = random32() % 5;
                q->type = defenses[random32() % (sizeof(defenses) / sizeof(defenses[0]))];
                q->raw_type = -1;
                q->raw_owner = -1;
                q->x = 170 + random32() % 400;
                q->y = 170 + random32() % 400;
                q->hp = q->maxhp = 300;
                q->queue_progress = random32() % 44;
                q->under_construction = random32() % 13 == 0;
            }
        if (trial % 13 == 0)
            u[MAX_UNITS - 1].alive = 1, u[MAX_UNITS - 1].x = -1;
        if (trial % 17 == 0)
            u[MAX_UNITS - 1].alive = 1, u[MAX_UNITS - 1].y = DEFENSE_CELL_LIMIT;
        if (trial % 19 == 0)
            u[MAX_UNITS - 1].alive = 1, u[MAX_UNITS - 1].owner = 7;
        compare();
    }
    puts("Terran: 800 randomized Custom full-pass scalar/cell cases matched");
    return 0;
}
