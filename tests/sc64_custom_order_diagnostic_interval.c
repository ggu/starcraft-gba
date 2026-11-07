/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
int main(void) {
    race_mode=1; race_human=2; race_bots=3; race_skirmish_map=1;
    race_bot_races[0]=0; race_bot_races[1]=1; race_bot_races[2]=2;
    init_state(); sc64_leaf_active=0;
    sc64_leaf_grid_count=sc64_leaf_point_count=sc64_leaf_depot_count=0;
    build_path_grid(); sc64_routes_reset();
    int goal=-1; for (int i=0;i<MAP_COLS*MAP_ROWS;i++) if(SC64_GRID_GET(path_walkable,i)){goal=i;break;}
    assert(goal>=0); (void)sc64_route_slot(goal);
    (void)sc64_route_depot_slot_for_origin(0,(goal%MAP_COLS)*16+8,(goal/MAP_COLS)*16+8,-1);
    assert(!sc64_leaf_grid_count && !sc64_leaf_point_count && !sc64_leaf_depot_count);
    sc64_leaf_active=1; build_path_grid(); sc64_routes_reset();
    (void)sc64_route_slot(goal);
    (void)sc64_route_depot_slot_for_origin(0,(goal%MAP_COLS)*16+8,(goal/MAP_COLS)*16+8,-1);
    sc64_leaf_active=0;
    assert(sc64_leaf_grid_count==1 && sc64_leaf_point_count==1 && sc64_leaf_depot_count==1);
    assert(sc64_leaf_point_nodes>0 && sc64_leaf_depot_nodes>0);
    puts("diagnostic interval: outside zero, inside grid/point/depot one each"); return 0;
}
