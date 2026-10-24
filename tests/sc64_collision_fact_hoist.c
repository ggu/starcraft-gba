/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Differential host check for the optional Z/P legacy-collision fact hoist.
 * The oracle keeps the pre-hoist predicate and scans the full live-unit pool;
 * the production path uses the real bucket index. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_ZP_CAMPAIGN_ACTORS
#error "This fixture targets the expanded Z/P actor facts"
#endif

static unsigned rng_state = 0x4c91a2d7u;
static unsigned result_hash = 0x6b31d9a5u;
static unsigned next_random(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

/* Independent reference: same collision predicate as the old fallback, with
 * a full pool scan so the spatial bucket broad phase is checked as well. */
static int old_full_scan_clear(const Unit *q, int nx, int ny)
{
    if (IS_WORKER(q->type) && (q->order == ORDER_MINE || q->order == ORDER_GAS))
        return 1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *candidate = &u[i];
        if (candidate == q || !candidate->alive || SC64_TRANSPORT_LOADED(candidate))
            continue;
        int mover_radius = sc64_game_unit_collision_radius(q->type);
        int candidate_radius = sc64_game_unit_collision_radius(candidate->type);
        int radius = mover_radius > candidate_radius ? mover_radius : candidate_radius;
        if (candidate->type == U_CHRYSALIS ||
            sc64_race_unit_flying(candidate->type) != sc64_race_unit_flying(q->type))
            continue;
        unsigned candidate_flags = sc64_game_unit_actor_flags(candidate->type);
        unsigned mover_flags = sc64_game_unit_actor_flags(q->type);
        if ((candidate_flags && !(candidate_flags & SC64_GAME_ACTOR_COLLIDABLE)) ||
            (mover_flags && !(mover_flags & SC64_GAME_ACTOR_COLLIDABLE)))
            continue;
        int dx = candidate->x - nx, dy = candidate->y - ny;
        if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius)
            continue;
        int next_distance = dx * dx + dy * dy;
        if (next_distance < radius * radius) {
            int ox = candidate->x - q->x, oy = candidate->y - q->y;
            if (next_distance <= ox * ox + oy * oy) return 0;
        }
    }
    return 1;
}

/* Exact pre-hoist bucket query, retained as the direct differential oracle. */
static int old_bucket_clear(const Unit *q, int nx, int ny)
{
    if (IS_WORKER(q->type) && (q->order == ORDER_MINE || q->order == ORDER_GAS))
        return 1;
    const int reach = 9;
    int minx = (nx - reach) >> 4, maxx = (nx + reach) >> 4;
    int miny = (ny - reach) >> 4, maxy = (ny + reach) >> 4;
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= MAP_COLS) maxx = MAP_COLS - 1;
    if (maxy >= MAP_ROWS) maxy = MAP_ROWS - 1;
    for (int row = miny; row <= maxy; ++row)
        for (int col = minx; col <= maxx; ++col)
            for (int i = sc64_unit_bucket_head[col + row * MAP_COLS]; i != 0xff;
                 i = sc64_unit_bucket_next[i])
                if (&u[i] != q && u[i].alive && !SC64_TRANSPORT_LOADED(&u[i])) {
                    int mover_radius = sc64_game_unit_collision_radius(q->type);
                    int candidate_radius = sc64_game_unit_collision_radius(u[i].type);
                    int radius = mover_radius > candidate_radius ? mover_radius : candidate_radius;
                    if (u[i].type == U_CHRYSALIS ||
                        sc64_race_unit_flying(u[i].type) != sc64_race_unit_flying(q->type))
                        continue;
                    if ((sc64_game_unit_actor_flags(u[i].type) &&
                         !(sc64_game_unit_actor_flags(u[i].type) & SC64_GAME_ACTOR_COLLIDABLE)) ||
                        (sc64_game_unit_actor_flags(q->type) &&
                         !(sc64_game_unit_actor_flags(q->type) & SC64_GAME_ACTOR_COLLIDABLE)))
                        continue;
                    int dx = u[i].x - nx, dy = u[i].y - ny;
                    if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius)
                        continue;
                    int next_distance = dx * dx + dy * dy;
                    if (next_distance < radius * radius) {
                        int ox = u[i].x - q->x, oy = u[i].y - q->y;
                        if (next_distance <= ox * ox + oy * oy) return 0;
                    }
                }
    return 1;
}

static const int actor_types[] = {
    U_SCV, U_MARINE, U_FIREBAT, U_MEDIC, U_ZERGLING, U_HYDRALISK,
    U_RAYNOR, U_DRONE, U_OVERLORD, U_PROBE, U_ZEALOT, U_DRAGOON,
    U_SCOUT, U_TASSADAR, U_FENIX, U_CHRYSALIS, U_QUEEN, U_GOLIATH,
    U_TANK, U_WRAITH, U_MUTALISK, U_HUNTER_KILLER, U_REAVER,
    U_FENIX_DRAGOON, U_SIEGE_TANK, U_VULTURE, U_DROPSHIP, U_SCOURGE,
    U_ZP_INFESTED_KERRIGAN, U_ZP_MAP_REVEALER, U_ZP_RAYNOR_VULTURE,
    U_ZP_HIGH_TEMPLAR, U_ZP_ULTRALISK, U_Z5_GHOST, U_Z5_CIVILIAN,
    U_Z5_RHYNADON, U_Z5_BENGALAAS, U_Z5_RAGNASAUR, U_Z5_DATA_DISK,
    U_Z6_GUARDIAN, -1, SC_RU_COUNT, 255
#ifdef SC64_P5_ACTORS
    , U_P5_SHUTTLE, U_P5_BATTLECRUISER
#endif
#ifdef SC64_P6_ACTORS
    , U_P6_INFESTED_TERRAN, U_P6_HERO_DARK_TEMPLAR, U_P6_ZERATUL
#endif
#ifdef SC64_P7_ACTORS
    , U_P7_ARCHON, U_P7_ARBITER, U_P7_CARRIER, U_P7_OBSERVER
#endif
};

static void compare_case(unsigned seed)
{
    memset(u, 0, sizeof(u));
    int qslot = (int)(next_random() % MAX_UNITS);
    int qx = 128 + (int)(next_random() % (WORLD_W - 256));
    int qy = 128 + (int)(next_random() % (WORLD_H - 256));
    int qtype = actor_types[next_random() % (sizeof(actor_types) / sizeof(actor_types[0]))];
    u[qslot].alive = 1;
    u[qslot].type = qtype;
    u[qslot].owner = next_random() % 5;
    u[qslot].x = qx;
    u[qslot].y = qy;
    u[qslot].order = (seed & 7u) == 0 ? ORDER_MINE : ORDER_IDLE;
    Unit *q = &u[qslot];

    unsigned population = 1u + next_random() % (MAX_UNITS - 1u);
    for (unsigned k = 0; k < population; ++k) {
        int slot = (int)(next_random() % MAX_UNITS);
        if (slot == qslot) continue;
        Unit *v = &u[slot];
        v->alive = (next_random() & 7u) != 0;
        v->type = actor_types[next_random() % (sizeof(actor_types) / sizeof(actor_types[0]))];
        v->owner = next_random() % 5;
        /* Mix local collisions with distant actors and 16px bucket edges. */
        if ((next_random() & 3u) != 0) {
            v->x = qx + (int)(next_random() % 49) - 24;
            v->y = qy + (int)(next_random() % 49) - 24;
        } else {
            v->x = (int)(next_random() % WORLD_W);
            v->y = (int)(next_random() % WORLD_H);
        }
        if (v->x < 0) v->x = 0;
        if (v->y < 0) v->y = 0;
        if (v->x >= WORLD_W) v->x = WORLD_W - 1;
        if (v->y >= WORLD_H) v->y = WORLD_H - 1;
        if ((next_random() & 15u) == 0) v->order = ORDER_TRANSPORTED;
    }

    int nx = qx + (int)(next_random() % 17) - 8;
    int ny = qy + (int)(next_random() % 17) - 8;
    sc64_unit_bucket_initialized = 0;
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    int expected = old_full_scan_clear(q, nx, ny);
    int old_bucket = old_bucket_clear(q, nx, ny);
    int actual = sc64_unit_bucket_step_clear(q, nx, ny);
    assert(old_bucket == expected);
    assert(actual == expected);
    result_hash = result_hash * 16777619u ^ (unsigned)(actual + 3 * old_bucket + 7 * expected);
    sc64_unit_buckets_end();
}

static void test_actor_filter_and_edges(void)
{
    memset(u, 0, sizeof(u));
    u[0] = (Unit){0};
    u[0].alive = 1; u[0].type = U_ZP_ULTRALISK; u[0].x = 86; u[0].y = 80;
    u[1].alive = 1; u[1].type = U_MARINE; u[1].x = 96; u[1].y = 80;
    sc64_unit_bucket_initialized = 0;
    sc64_unit_buckets_begin();
    assert(!sc64_unit_bucket_step_clear(&u[0], 87, 80));
    sc64_unit_buckets_end();

    /* Strict-radius and exact bucket-boundary controls. */
    u[1].x = 97; u[1].y = 80;
    sc64_unit_bucket_initialized = 0;
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_step_clear(&u[0], 87, 80));
    sc64_unit_buckets_end();
    u[1].x = 96; u[1].y = 80; u[1].order = ORDER_TRANSPORTED;
    sc64_unit_bucket_initialized = 0;
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_step_clear(&u[0], 87, 80));
    sc64_unit_buckets_end();
    u[1].order = ORDER_IDLE; u[1].alive = 0;
    sc64_unit_bucket_initialized = 0;
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_step_clear(&u[0], 87, 80));
    sc64_unit_buckets_end();
}

#ifdef SC64_COLLISION_CLASS_CACHE
static void test_collision_class_cache_and_invalidation(void)
{
    int r, f, c;
    assert(sizeof(sc64_unit_collision_class_cache) == MAX_UNITS);

    memset(u, 0, sizeof(u));
    u[0].alive = 1; u[0].type = U_MARINE; u[0].x = 80; u[0].y = 80;
    u[1].alive = 1; u[1].type = U_ZP_ULTRALISK; u[1].x = 96; u[1].y = 80;
    sc64_unit_bucket_initialized = 0;
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_step_clear(&u[0], 87, 80) ==
           old_full_scan_clear(&u[0], 87, 80));
    assert(sc64_unit_collision_class_cache[0] != 0);
    assert(sc64_unit_collision_class_cache[1] != 0);

    /* An in-phase type mutation must invalidate the slot before another
     * collision query.  Radius/flying/collidable are all allowed to change. */
    u[1].type = U_SCOUT;
    sc64_unit_collision_class_invalidate(1);
    assert(sc64_unit_collision_class_cache[1] == 0);
    assert(sc64_unit_bucket_step_clear(&u[0], 87, 80) ==
           old_full_scan_clear(&u[0], 87, 80));
    assert(sc64_unit_collision_class_cache[1] != 0);

    /* Reusing a dead slot between phases starts with a fresh class cache. */
    sc64_unit_buckets_end();
    u[1].alive = 0;
    u[1].type = U_ZP_MAP_REVEALER;
    sc64_unit_buckets_begin();
    assert(sc64_unit_collision_class_cache[1] == 0);
    u[1].alive = 1;
    u[1].type = U_ZP_ULTRALISK;
    sc64_unit_bucket_insert(1, sc64_unit_bucket_cell_at(u[1].x, u[1].y));
    assert(sc64_unit_bucket_step_clear(&u[0], 87, 80) ==
           old_full_scan_clear(&u[0], 87, 80));
    sc64_unit_buckets_end();

    /* Unknown and sentinel-like enum values keep the engine's fallback
     * radius=6, ground, flags=0 (therefore collidable) behavior. */
    assert(sc64_unit_collision_class_values(-1, &r, &f, &c));
    assert(r == 6 && f == 0 && c == 1);
    assert(sc64_unit_collision_class_values(SC_RU_COUNT, &r, &f, &c));
    assert(r == 6 && f == 0 && c == 1);
    assert(sc64_unit_collision_class_values(255, &r, &f, &c));
    assert(r == 6 && f == 0 && c == 1);
    assert(sc64_unit_collision_class_pack(63, 1, 1) == 255);
    assert(sc64_unit_collision_class_pack(64, 1, 1) == 0);
    assert(sc64_unit_collision_class_pack(-1, 1, 1) == 0);
    assert(sc64_unit_collision_class_pack(0, 0, 0) == 0);
    assert(sc64_unit_collision_class(-1, -1, &r, &f, &c) == 25);
    assert(r == 6 && f == 0 && c == 1);
}
#endif

int main(void)
{
    test_actor_filter_and_edges();
#ifdef SC64_COLLISION_CLASS_CACHE
    test_collision_class_cache_and_invalidation();
#endif
    for (unsigned i = 0; i < 1500; ++i) compare_case(i);
    printf("Z/P collision fact-hoist differential tests passed: %08x\n", result_hash);
    return 0;
}
