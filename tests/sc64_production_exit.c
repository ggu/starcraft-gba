/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free fixtures for the bounded production-exit search. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/sc64_production_exit.h"

typedef struct {
    int min_x, min_y, max_x, max_y;
    int blocked_building_x, blocked_building_y, blocked_unit_x, blocked_unit_y;
    int block_building, block_all_buildings, block_unit, block_terrain, block_reach;
    int terrain_calls, building_calls, unit_calls, reach_calls, bounds_calls;
    int seen_flying;
} Fixture;

static int in_bounds(void *context, int x, int y)
{
    Fixture *f = (Fixture *)context;
    ++f->bounds_calls;
    return x >= f->min_x && x <= f->max_x && y >= f->min_y && y <= f->max_y;
}

static int terrain_fits(void *context, int x, int y)
{
    Fixture *f = (Fixture *)context;
    (void)x; (void)y;
    ++f->terrain_calls;
    return !f->block_terrain;
}

static int building_blocked(void *context, int x, int y, int flying)
{
    Fixture *f = (Fixture *)context;
    ++f->building_calls;
    f->seen_flying = flying;
    return f->block_all_buildings ||
           (f->block_building && x == f->blocked_building_x && y == f->blocked_building_y);
}

static int unit_blocked(void *context, int x, int y, int flying)
{
    Fixture *f = (Fixture *)context;
    ++f->unit_calls;
    f->seen_flying = flying;
    return f->block_unit && x == f->blocked_unit_x && y == f->blocked_unit_y;
}

static int reachable(void *context, int from_x, int from_y, int x, int y)
{
    Fixture *f = (Fixture *)context;
    (void)from_x; (void)from_y; (void)x; (void)y;
    ++f->reach_calls;
    return !f->block_reach;
}

static Sc64ProductionExitOps ops_for(Fixture *f)
{
    Sc64ProductionExitOps ops;
    ops.context = f;
    ops.in_bounds = in_bounds;
    ops.terrain_fits = terrain_fits;
    ops.building_blocked = building_blocked;
    ops.unit_blocked = unit_blocked;
    ops.reachable = reachable;
    return ops;
}

static Fixture fixture(void)
{
    Fixture f;
    memset(&f, 0, sizeof(f));
    f.min_x = f.min_y = 0;
    f.max_x = f.max_y = 128;
    f.blocked_building_x = f.blocked_building_y = -1000;
    f.blocked_unit_x = f.blocked_unit_y = -1000;
    return f;
}

static void preferred_exit_is_unchanged_when_open(void)
{
    Fixture f = fixture();
    Sc64ProductionExitOps ops = ops_for(&f);
    int x = -7, y = -9;
    assert(sc64_production_exit_find(&ops, 64, 64, 84, 68, 0, &x, &y));
    assert(x == 84 && y == 68);
    assert(f.bounds_calls == 1 && f.building_calls == 1 && f.unit_calls == 1);
    assert(f.reach_calls == 1 && f.terrain_calls == 1);
}

static void blocked_exit_searches_outward_in_reference_edge_order(void)
{
    Fixture f = fixture();
    Sc64ProductionExitOps ops = ops_for(&f);
    int x = -7, y = -9;
    /* Preferred is unit-blocked; producer center is building-blocked. */
    f.block_unit = f.block_building = 1;
    f.blocked_unit_x = 84; f.blocked_unit_y = 68;
    f.blocked_building_x = 64; f.blocked_building_y = 64;
    assert(sc64_production_exit_find(&ops, 64, 64, 84, 68, 0, &x, &y));
    assert(x == 60 && y == 68);
}

static void terrain_and_reachability_are_required_for_ground_units(void)
{
    Fixture f = fixture();
    Sc64ProductionExitOps ops = ops_for(&f);
    int x = 111, y = 113;
    f.block_terrain = 1;
    assert(!sc64_production_exit_find(&ops, 64, 64, 84, 68, 0, &x, &y));
    assert(x == 111 && y == 113); /* failure leaves outputs untouched */

    memset(&f, 0, sizeof(f)); f = fixture();
    ops = ops_for(&f);
    f.block_reach = 1;
    assert(!sc64_production_exit_find(&ops, 64, 64, 84, 68, 0, &x, &y));
}

static void flying_units_skip_ground_terrain_but_still_need_reachability(void)
{
    Fixture f = fixture();
    Sc64ProductionExitOps ops = ops_for(&f);
    int x = -1, y = -1;
    f.block_terrain = 1;
    assert(sc64_production_exit_find(&ops, 64, 64, 84, 68, 1, &x, &y));
    assert(x == 84 && y == 68);
    assert(f.terrain_calls == 0 && f.seen_flying == 1);
    f.block_reach = 1;
    assert(!sc64_production_exit_find(&ops, 64, 64, 84, 68, 1, &x, &y));
}

static void search_clips_candidates_at_map_bounds(void)
{
    Fixture f = fixture();
    Sc64ProductionExitOps ops = ops_for(&f);
    int x = -1, y = -1;
    f.block_building = 1;
    f.blocked_building_x = 0; f.blocked_building_y = 0;
    assert(sc64_production_exit_find(&ops, 0, 0, -20, 4, 0, &x, &y));
    assert(x == 0 && y == 4);
    assert(x >= f.min_x && x <= f.max_x && y >= f.min_y && y <= f.max_y);
}

static void bounded_exhaustion_never_returns_an_unchecked_exit(void)
{
    Fixture f = fixture();
    Sc64ProductionExitOps ops = ops_for(&f);
    int x = 7, y = 9;
    f.block_all_buildings = 1;
    assert(!sc64_production_exit_find(&ops, 64, 64, 84, 68, 0, &x, &y));
    assert(x == 7 && y == 9);
    assert(f.bounds_calls == SC64_PRODUCTION_EXIT_MAX_CANDIDATES);
}

static void invalid_arguments_fail_closed(void)
{
    Fixture f = fixture();
    Sc64ProductionExitOps ops = ops_for(&f);
    int x = 1, y = 2;
    assert(!sc64_production_exit_find(NULL, 64, 64, 84, 68, 0, &x, &y));
    assert(!sc64_production_exit_find(&ops, 64, 64, 84, 68, 2, &x, &y));
    assert(!sc64_production_exit_find(&ops, 64, 64, 84, 68, 0, NULL, &y));
}

int main(void)
{
    preferred_exit_is_unchanged_when_open();
    blocked_exit_searches_outward_in_reference_edge_order();
    terrain_and_reachability_are_required_for_ground_units();
    flying_units_skip_ground_terrain_but_still_need_reachability();
    search_clips_candidates_at_map_bounds();
    bounded_exhaustion_never_returns_an_unchecked_exit();
    invalid_arguments_fail_closed();
    puts("sc64_production_exit: ok");
    return 0;
}
