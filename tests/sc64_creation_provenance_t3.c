/* SPDX-License-Identifier: GPL-3.0-or-later
 * T3 two-source ownership and custom helper coverage. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
int main(void)
{
    gba_init();game_init();race_mode=0;race_selected_mission=SC64_RACE_MISSION_T3;
    const Sc64RaceMap *map=&sc64_race_maps[race_selected_mission];
    assert(map->owner_modes && race_campaign_owner(map,0)==ENEMY && race_campaign_owner(map,4)==ENEMY);
    assert(race_creation_source_owner(ENEMY,-1)==-1);
    /* Seed two legal completed queues and exercise ordinary paired completion;
     * the source owners remain distinct even though live ownership collapses. */
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));race_player[ENEMY]=R_ZERG;
    for(int i=0;i<2;++i) {
        b[i].alive=1;b[i].owner=ENEMY;b[i].raw_owner=i?4:0;
        b[i].type=B_HATCHERY;b[i].x=400+i*96;b[i].y=400;
        b[i].queue_count=1;b[i].queue_head=0;b[i].queue[0]=U_ZERGLING;
        b[i].queue_progress=(sc64_race_unit_rule(U_ZERGLING)->build_frames*5+3)/4-1;
    }
    race_tick_buildings();
    assert(!b[0].queue_count&&!b[1].queue_count);
    int source0=0,source4=0;
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].type==U_ZERGLING) {
        source0+=u[i].raw_owner==0;source4+=u[i].raw_owner==4;
    }
    assert(source0==2&&source4==2);
    race_mode=1;
    for(int owner=0;owner<5;++owner) {
        assert(race_creation_source_owner(owner,owner)==-1);
        int slot=race_spawn_product(owner,U_DRAGOON,540+owner*8,500,-1);
        assert(slot>=0&&u[slot].raw_owner==-1);
    }
    puts("T3 paired completion and custom helper: PASS");return 0;
}
