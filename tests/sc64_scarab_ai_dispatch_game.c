/* SPDX-License-Identifier: GPL-3.0-or-later
 * Real game_update integration for the explicit unsaved AI probe. */
#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"
static int add_parent(int owner,int source,int x)
{
    int slot=spawn_unit(owner,U_REAVER,x,128);assert(slot>=0);
    u[slot].raw_owner=source;assert(race_scarab_parent_init(slot));return slot;
}
static void campaign_pair(int mission,int source_a,int source_b,u32 *a,u32 *b)
{
    race_mode=0;race_selected_mission=mission;init_state();
    int first=add_parent(ENEMY,source_a,128),second=add_parent(ENEMY,source_b,160);
    frame=1;game_update(0,0);assert(race_scarab_ai_roster_ready);
    *a=200;*b=300;race_minerals[ENEMY]=900;
    race_scarab_ai_clock.owner[0].remaining=1;
    race_scarab_ai_clock.owner[1].remaining=1;
    game_update(0,0);
    assert(*a==185 && *b==285 && race_minerals[ENEMY]==900);
    u[first].raw_owner=6;u[second].owner=SIDE;
    race_scarab_ai_clock.owner[0].remaining=1;
    race_scarab_ai_clock.owner[1].remaining=1;
    game_update(0,0);
    assert(*a==185 && *b==285 && race_minerals[ENEMY]==900);
}
int main(void)
{
    gba_init();race_mode=1;race_skirmish_map=0;race_human=R_TER;race_bots=3;
    race_bot_races[0]=race_bot_races[1]=race_bot_races[2]=R_PROTOSS;
    init_state();
    int a=add_parent(ENEMY,-1,128),b=add_parent(RESCUABLE,-1,160),c=add_parent(4,-1,192);
    race_minerals[ENEMY]=200;race_minerals[RESCUABLE]=300;race_minerals[4]=400;
    frame=1;game_update(0,0); /* build authorized roster */
    assert(race_scarab_ai_clock.owner[0].active && race_scarab_ai_clock.owner[1].active &&
           race_scarab_ai_clock.owner[2].active);
    for(int i=0;i<3;i++)race_scarab_ai_clock.owner[i].remaining=1;
    game_update(0,0);
    assert(race_minerals[ENEMY]==185 && race_minerals[RESCUABLE]==285 && race_minerals[4]==385);
    Sc64ScarabAiClock before=race_scarab_ai_clock;
    game_update(KEY_START,KEY_START);assert(screen==SCREEN_PAUSE);
    assert(!memcmp(&before,&race_scarab_ai_clock,sizeof(before)));
    assert(!save_game() && !load_game());
    screen=SCREEN_PLAY;
    /* A recycled or changed source identity cannot consume another bank. */
    u[a].raw_owner=1;u[b].alive=0;
    for(int i=0;i<4;i++)assert(race_scarab_ai_enqueue_one(c,4,-1));
    for(int i=0;i<3;i++)race_scarab_ai_clock.owner[i].remaining=1;
    game_update(0,0);
    assert(race_minerals[ENEMY]==185 && race_minerals[RESCUABLE]==285 && race_minerals[4]==325);
#ifdef SC64_P7_RUNTIME
    campaign_pair(SC64_P7_GLOBAL_ID,0,4,&race_p7.source_minerals[0],&race_p7.source_minerals[4]);
#endif
#ifdef SC64_P8_RUNTIME
    campaign_pair(SC64_P8_GLOBAL_ID,0,3,&race_p8.ai_minerals[0],&race_p8.ai_minerals[1]);
#endif
#ifdef SC64_Z8_RUNTIME
    campaign_pair(SC64_Z8_GLOBAL_ID,1,2,&race_z8_ai_minerals[0],&race_z8_ai_minerals[1]);
#endif
    init_state();assert(!race_scarab_ai_roster_ready);
    for(int i=0;i<SC64_SCARAB_AI_OWNER_LIMIT;i++)assert(!race_scarab_ai_clock.owner[i].active);
    return 0;
}
