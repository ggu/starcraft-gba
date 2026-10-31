/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Compare the full scalar and packed rebuilds, including building overlays. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#if !defined(SC64_BULK_TERRAIN_GRID_TEST) || !defined(SC64_IMPORTED_SKIRMISH)
#error "requires the imported packed-grid test profile"
#endif
static u8 expected[sizeof(path_walkable)];
static unsigned cases;
static void check_grid(void)
{
    for (unsigned pass=0;pass<3;pass++) {
        memset(path_walkable,0xa5,sizeof(path_walkable));
        sc64_bulk_terrain_force_scalar=1;invalidate_path_grid();build_path_grid();
        memcpy(expected,path_walkable,sizeof(expected));
        memset(path_walkable,0xa5,sizeof(path_walkable));
        sc64_bulk_terrain_force_scalar=0;invalidate_path_grid();build_path_grid();
        assert(!memcmp(expected,path_walkable,sizeof(expected)));++cases;
        if(pass==0) {
            memset(b,0,sizeof(b));
            b[0].alive=1;b[0].type=B_HATCHERY;b[0].owner=SIDE;b[0].x=64;b[0].y=64;
            b[1].alive=1;b[1].type=B_COMMAND;b[1].owner=ENEMY;b[1].x=160;b[1].y=112;
        } else if(pass==1) {b[0].alive=0;b[1].x+=16;}
    }
    memset(b,0,sizeof(b));
}
int main(void)
{
    gba_init();game_init();
    for(unsigned k=0;k<SC64_RACE_MAP_COUNT;k++) {
        const Sc64RaceMap *map=&sc64_race_maps[k];
        if(!map->width||!map->height||!map->blocked||
           (unsigned)map->width*map->height>MAP_CELLS)continue;
        race_mode=0;race_selected_mission=(int)k;race_cols=map->width;race_rows=map->height;
        check_grid();
    }
    race_mode=1;
    for(unsigned k=0;k<SC64_SKIRMISH_IMPORTED_COUNT;k++) {
        race_skirmish_map=(int)k+1;race_cols=sc64_skirmish_maps[k].width;race_rows=sc64_skirmish_maps[k].height;
        check_grid();
    }
    race_skirmish_map=0;race_cols=race_rows=64;check_grid();
#ifdef SC64_SHOWCASE_ENABLED
    sc64_showcase_scene=0;check_grid();
#endif
    printf("terrain grid complete rebuild parity: %u map/overlay cases\n",cases);
    return 0;
}
