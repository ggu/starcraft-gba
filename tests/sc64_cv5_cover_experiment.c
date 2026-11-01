/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
int main(void) {
    unsigned total=0,covered=0;
#ifdef SC64_RACES
    race_mode=0;
    for (unsigned k=0;k<SC64_RACE_MAP_COUNT;++k) {
        race_selected_mission=(int)k; race_cols=sc64_race_maps[k].width; race_rows=sc64_race_maps[k].height;
        if (!race_cols || !race_rows || !sc64_race_maps[k].blocked) continue;
        for(int y=0;y<race_rows;++y) for(int x=0;x<race_cols;++x) {
            unsigned v=sc64_race_maps[k].blocked[x+y*race_cols];
            assert(terrain_blocked(x,y)==(int)(v&1));
            assert(terrain_provides_cover(x*16,y*16)==(int)((v>>1)&1));
            assert(terrain_provides_cover(x*16+15,y*16+15)==(int)((v>>1)&1));
            total++;covered+=(v>>1)&1;
        }
        assert(!terrain_provides_cover(-1,0)); assert(!terrain_provides_cover(WORLD_W,0));
        assert(!terrain_provides_cover(0,WORLD_H));
    }
#ifdef SC64_IMPORTED_SKIRMISH
    race_mode=1; race_skirmish_map=1; race_cols=sc64_skirmish_maps[0].width;race_rows=sc64_skirmish_maps[0].height;
    for(int y=0;y<race_rows;++y)for(int x=0;x<race_cols;++x){unsigned v=sc64_skirmish_maps[0].blocked[x+y*race_cols];assert(terrain_blocked(x,y)==(int)(v&1));assert(terrain_provides_cover(x*16,y*16)==(int)((v>>1)&1));total++;covered+=(v>>1)&1;}
    race_skirmish_map=0;assert(!terrain_provides_cover(0,0));
#endif
#else
#ifdef SC64_TERRAN_CAMPAIGN
    for (campaign_mission=0;campaign_mission<2;++campaign_mission) {
#endif
    for(int y=0;y<MAP_ROWS;++y) for(int x=0;x<MAP_COLS;++x) {
#ifdef SC64_TERRAN_CAMPAIGN
        unsigned v=sc64_campaign_maps[campaign_mission].blocked[x+y*MAP_COLS];
#else
        unsigned v=sc64_terrain_blocked[x+y*MAP_COLS];
#endif
        assert(terrain_blocked(x,y)==(int)(v&1));
        assert(terrain_provides_cover(x*16,y*16)==(int)((v>>1)&1));
        total++; covered+=(v>>1)&1;
    }
#ifdef SC64_TERRAN_CAMPAIGN
    }
#endif
    assert(!terrain_provides_cover(-1,0));assert(!terrain_provides_cover(WORLD_W,0));
#endif
    printf("cover accessor cells=%u covered=%u\n",total,covered);return 0;
}
