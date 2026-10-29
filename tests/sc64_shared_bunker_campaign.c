/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real first-campaign context; fixture-seeded actors, not ordinary acquisition. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#ifndef SC64_SHARED_BUNKER
#error "shared Bunker capability required"
#endif
#ifndef SC64_CAMPAIGN_RACE
#define SC64_CAMPAIGN_RACE 0
#endif
static void find_site(int *x,int *y)
{
    for (int row=4;row<race_rows-4;++row) for (int col=4;col<race_cols-4;++col) {
        int px=col*16+8,py=row*16+8,clear=1;
        for (int dy=-2;dy<=2;++dy) for (int dx=-2;dx<=2;++dx)
            if (terrain_blocked(col+dx,row+dy)) clear=0;
        for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && dist2(px,py,u[i].x,u[i].y)<96*96) clear=0;
        for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive && dist2(px,py,b[i].x,b[i].y)<96*96) clear=0;
        if (clear) { *x=px;*y=py;return; }
    }
    assert(!"campaign fixture needs an open site");
}
int main(void)
{
    setbuf(stdout,NULL);gba_init();game_init();
    int mission=sc64_frontend_first_mission(SC64_CAMPAIGN_RACE);
    race_mode=0;race_selected_mission=mission;race_completed=0;init_state();
    assert(!race_mode && screen==SCREEN_PLAY && race_selected_mission==mission);
    assert(race_campaign.mission==mission && sc64_catalog_mission_faction(mission)==SC64_CAMPAIGN_RACE);
    int x=0,y=0;find_site(&x,&y);
    int bunker=spawn_build(SIDE,B_BUNKER,x,y,0), marine=spawn_unit(SIDE,U_MARINE,x,y);
    int foe=spawn_unit(ENEMY,U_MARINE,x+40,y);
    assert(bunker>=0 && marine>=0 && foe>=0);
    /* Human-side rescued source identity may differ from the Bunker's. */
    b[bunker].raw_owner=0;u[marine].raw_owner=4;
    assert(race_bunker_request_board(marine,bunker));race_bunker_tick_board(marine);
    assert(SC64_TRANSPORT_LOADED(&u[marine]));
    unsigned old_objective=race_campaign.objective_flags;
    unsigned old_phase=race_campaign.phase;
    int old_mission=race_selected_mission,old_completed=race_completed;
    update_exploration(); assert(currently_visible(u[foe].x,u[foe].y));
    u[marine].cd=0;u[marine].build=-1;int hp=u[foe].hp;
    assert(race_bunker_tick_loaded(marine));
    assert(u[foe].hp<hp && !u[marine].selected);
    assert(race_save_game() && race_load_game());
    assert(SC64_TRANSPORT_LOADED(&u[marine]) && u[marine].target==MAX_UNITS+bunker);
    assert(race_campaign.objective_flags==old_objective && race_campaign.phase==old_phase &&
           race_selected_mission==old_mission && race_completed==old_completed);
    assert(race_bunker_request_unload(bunker)==1 && !SC64_TRANSPORT_LOADED(&u[marine]));
    int hostile_bunker=spawn_build(ENEMY,B_BUNKER,x+80,y,0);
    int hostile=spawn_unit(ENEMY,U_MARINE,x+80,y);
    assert(hostile_bunker>=0 && hostile>=0);
    b[hostile_bunker].raw_owner=0;u[hostile].raw_owner=4;
    assert(!race_bunker_request_board(hostile,hostile_bunker));
    u[hostile].raw_owner=0;
    assert(race_bunker_request_board(hostile,hostile_bunker));race_bunker_tick_board(hostile);
    assert(SC64_TRANSPORT_LOADED(&u[hostile]));
    assert(race_save_game() && race_load_game());
    assert(SC64_TRANSPORT_LOADED(&u[hostile]) && u[hostile].target==MAX_UNITS+hostile_bunker);
    puts("first campaign: seeded Bunker combat, ownership and SRAM PASS");return 0;
}
