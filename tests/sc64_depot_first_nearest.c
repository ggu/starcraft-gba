/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_DEPOT_OWNER_LIST_PROBE) || !defined(SC64_NEAREST_RESOURCE_DEPOT)
#error "Run this fixture with a profile that supports both depot scan paths"
#endif

static void reset_world(void)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    race_mode = 1;
    race_human = R_PROTOSS;
    race_bots = 0;
    race_player[SIDE] = R_PROTOSS;
    invalidate_path_grid();
}

static void depot(int slot, int x, int y)
{
    b[slot] = (Building){.x=x, .y=y, .owner=SIDE, .type=B_NEXUS, .alive=1,
                        .hp=1000, .maxhp=1000};
}

static void grid(int wall_column, int indexed)
{
    build_path_grid();
    assert(MAP_COLS > 36 && MAP_ROWS > 26);
    for (int y = 0; y < MAP_ROWS; ++y)
        for (int x = 0; x < MAP_COLS; ++x)
            SC64_GRID_SET(path_walkable, x + y * MAP_COLS, x != wall_column);
    if (!indexed) return;
    for (int owner = 0; owner < 5; ++owner) target_building_head[owner] = -1;
    for (int i = MAX_BUILDINGS - 1; i >= 0; --i) if (b[i].alive) {
        int owner = b[i].owner;
        target_building_next[i] = target_building_head[owner];
        target_building_head[owner] = i;
    }
    target_lists_valid = sc64_depot_owner_lists_valid = 1;
}

static void check_route_work(int indexed)
{
    reset_world();
    depot(0, 160, 400);
    depot(1, 280, 400);
    depot(2, 400, 400);
    int worker = spawn_unit(SIDE, U_PROBE, 520, 400);
    assert(worker >= 0);
    grid(-1, indexed);
    u32 builds = sc64_route_builds, hits = sc64_route_hits;
    assert(race_nearest_resource_depot(&u[worker]) == 2 && u[worker].build == 2);
#ifdef SC64_DEPOT_FIRST_NEAREST_PROBE
    assert(sc64_route_builds == builds + 1);
#else
    assert(sc64_route_builds == builds + 3);
#endif
    assert(sc64_route_hits == hits);

    reset_world();
    depot(0, 40, 152);
    depot(1, 200, 152);
    depot(2, 136, 248);
    worker = spawn_unit(SIDE, U_PROBE, 136, 152);
    assert(worker >= 0);
    grid(9, indexed);
    builds = sc64_route_builds;
    hits = sc64_route_hits;
    /* Slot 1 is nearest but separated from the worker by column 9. Slot 0
     * wins over the equally distant slot 2. No extra lookup may retry
     * the known miss or make a route for the losing tie. */
    assert(race_nearest_resource_depot(&u[worker]) == 0 && u[worker].build == 0);
    assert(sc64_route_builds == builds + 2 && sc64_route_hits == hits);
}

static void check_bad_owner_or_invalid_snapshot(void)
{
    reset_world();
    depot(0, 160, 400);
    depot(1, 400, 400);
    int worker = spawn_unit(SIDE, U_PROBE, 520, 400);
    assert(worker >= 0);
    grid(-1, 0);
    u[worker].owner = 7;
    b[0].owner = b[1].owner = 7;
    target_lists_valid = sc64_depot_owner_lists_valid = 1;
    /* An out-of-domain owner must not index the five owner heads. */
    assert(race_nearest_resource_depot(&u[worker]) == 1);

    reset_world();
    depot(0, 160, 400);
    depot(1, 400, 400);
    worker = spawn_unit(SIDE, U_PROBE, 520, 400);
    assert(worker >= 0);
    grid(-1, 0);
    target_building_head[SIDE] = 0;
    target_building_next[0] = -1;
    sc64_depot_owner_lists_valid = 1;
    target_lists_valid = 0;
    /* A stale list that misses the nearer slot cannot decide a return. */
    assert(race_nearest_resource_depot(&u[worker]) == 1);
}

int main(void)
{
    gba_init();
    check_route_work(0);
    check_route_work(1);
    check_bad_owner_or_invalid_snapshot();
    puts("nearest depot slot, tie and route work checks passed");
    return 0;
}
