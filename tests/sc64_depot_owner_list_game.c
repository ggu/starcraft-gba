/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Compare the actual nearest-depot selector with the derived list and fallback. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#if !defined(SC64_DEPOT_OWNER_LIST_PROBE) || !defined(SC64_NEAREST_RESOURCE_DEPOT) || defined(SC64_ROUTE_ACTIVE_STRIDE)
#error "fixture requires the owner-list probe, nearest depots and fixed route storage"
#endif
static void list_snapshot(void)
{
    for (int j=0;j<5;j++) target_building_head[j]=-1;
    for (int i=MAX_BUILDINGS-1;i>=0;i--) if (b[i].alive) {
        assert(b[i].owner>=0 && b[i].owner<5);
        target_building_next[i]=target_building_head[b[i].owner];
        target_building_head[b[i].owner]=i;
    }
    target_lists_valid=sc64_depot_owner_lists_valid=1;
}
static void compare(Unit *q)
{
    static u8 before_field[sizeof(sc64_route_field)], after_field[sizeof(sc64_route_field)];
    u8 before_valid[sizeof(sc64_route_valid)],after_valid[sizeof(sc64_route_valid)];
    u16 before_goal[SC64_ROUTE_SLOTS],after_goal[SC64_ROUTE_SLOTS];
    u32 before_age[SC64_ROUTE_SLOTS],after_age[SC64_ROUTE_SLOTS];
    u32 clock0=sc64_route_clock,hits0=sc64_route_hits,builds0=sc64_route_builds,clock1,hits1,builds1;
    int prior=q->build,owner_valid=sc64_depot_owner_lists_valid,scalar,fast;
    memcpy(before_field,sc64_route_field,sizeof(before_field));memcpy(before_valid,sc64_route_valid,sizeof(before_valid));
    memcpy(before_goal,sc64_route_goal,sizeof(before_goal));memcpy(before_age,sc64_route_age,sizeof(before_age));
    sc64_depot_owner_lists_valid=0;scalar=race_nearest_resource_depot(q);
    memcpy(after_field,sc64_route_field,sizeof(after_field));memcpy(after_valid,sc64_route_valid,sizeof(after_valid));
    memcpy(after_goal,sc64_route_goal,sizeof(after_goal));memcpy(after_age,sc64_route_age,sizeof(after_age));
    clock1=sc64_route_clock;hits1=sc64_route_hits;builds1=sc64_route_builds;
    memcpy(sc64_route_field,before_field,sizeof(before_field));memcpy(sc64_route_valid,before_valid,sizeof(before_valid));
    memcpy(sc64_route_goal,before_goal,sizeof(before_goal));memcpy(sc64_route_age,before_age,sizeof(before_age));
    sc64_route_clock=clock0;sc64_route_hits=hits0;sc64_route_builds=builds0;q->build=prior;sc64_depot_owner_lists_valid=owner_valid;
    fast=race_nearest_resource_depot(q);assert(fast==scalar && q->build==(scalar<0?-2:scalar));
    assert(!memcmp(after_field,sc64_route_field,sizeof(after_field))&&!memcmp(after_valid,sc64_route_valid,sizeof(after_valid)));
    assert(!memcmp(after_goal,sc64_route_goal,sizeof(after_goal))&&!memcmp(after_age,sc64_route_age,sizeof(after_age)));
    assert(clock1==sc64_route_clock && hits1==sc64_route_hits && builds1==sc64_route_builds);
}
int main(void)
{
    gba_init();game_init();memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    race_mode=1;race_human=R_PROTOSS;race_bots=0;memset(race_player,0xff,sizeof(race_player));race_player[SIDE]=R_PROTOSS;
    supply_used=0;supply_cap=200;int w=spawn_unit(SIDE,U_PROBE,400,400);assert(w>=0);
    u[w].order=ORDER_MINE;u[w].build=-1;
    int a=spawn_build(SIDE,B_NEXUS,340,400,0),d=spawn_build(SIDE,B_NEXUS,460,400,0);assert(a>=0 && d>a);
    list_snapshot();assert(!path_grid_valid);build_path_grid();u[w].build=-1;compare(&u[w]);assert(u[w].build==a); /* equal distance */
    /* A newly spawned depot invalidates the derived snapshot and scalar sees it. */
    int n=spawn_build(SIDE,B_NEXUS,408,400,0);assert(n>=0 && !sc64_depot_owner_lists_valid);build_path_grid();u[w].build=-1;compare(&u[w]);assert(u[w].build==n);
    list_snapshot();destroy_building(n,ENEMY);assert(!sc64_depot_owner_lists_valid);build_path_grid();u[w].build=-1;compare(&u[w]);assert(u[w].build==a);
    /* Completion and morph change live eligibility but do not omit existing rows. */
    b[a].under_construction=1;list_snapshot();u[w].build=-1;compare(&u[w]);assert(u[w].build==d);
    b[a].under_construction=0;u[w].build=-1;compare(&u[w]);assert(u[w].build==a);
    b[a].type=B_LAIR;b[a].under_construction=1;u[w].build=-1;compare(&u[w]);assert(u[w].build==a);
    /* Ownership transfer is modeled after its production invalidation hook. */
    b[a].owner=ENEMY;invalidate_path_grid();assert(!sc64_depot_owner_lists_valid);build_path_grid();u[w].build=-1;compare(&u[w]);assert(u[w].build==d);
    target_lists_valid=0;sc64_depot_owner_lists_valid=1;u[w].build=-1;compare(&u[w]);assert(u[w].build==d);
    puts("owner-list nearest depot parity: tie, spawn, death, completion, morph, transfer, fallback");return 0;
}
