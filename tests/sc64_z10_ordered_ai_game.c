/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Targeted actual-engine fixture; actor setup is synthetic, not a playthrough. */
#define main z10_existing_fixture_main
#include "sc64_z10_game.c"
#undef main

static void actor(int i, int raw, int type, int source)
{
    memset(&u[i], 0, sizeof(u[i]));
    u[i].alive=1; u[i].owner=ENEMY; u[i].raw_owner=(s16)source;
    u[i].raw_type=(s16)raw; u[i].type=(s16)type; u[i].order=ORDER_IDLE;
}

static int ordered(int raw, int source)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)
        n+=u[i].alive&&u[i].raw_owner==source&&u[i].raw_type==raw&&
           u[i].order==ORDER_ATTACK_MOVE;
    return n;
}

int main(void)
{
    assert(SC64_RACE_SAVE_GENERATION==50u);
    launch_z10_from_normal_menu();
    /* Four globally reserved guards, followed by the source-requested actors.
     * Distinct source slots and an unrelated type must not be dispatched. */
    memset(u,0,sizeof(u));
    for(int i=0;i<4;i++) actor(i,65,U_ZEALOT,2);
    for(int i=4;i<10;i++) actor(i,65,U_ZEALOT,2);
    for(int i=10;i<14;i++) actor(i,66,U_DRAGOON,2);
    actor(14,65,U_ZEALOT,7);
    memset(&race_zl_ai,0,sizeof(race_zl_ai));
    race_zl_ai.started=1;
    race_zl_ai.wave_timer[0]=1;
    race_zl_ai.produce_timer[0]=1;
    race_zl_ai_tick();
    assert(ordered(65,2)==1 && ordered(66,2)==0 && ordered(65,7)==0);
    assert(race_zl_ai.wave[0]==1 && race_zl_ai.wave_timer[0]==6250);
    /* Request one Zealot and one Dragoon next; no per-type guard. */
    race_zl_ai.wave_timer[0]=1; race_zl_ai.produce_timer[0]=1;
    race_zl_ai_tick();
    assert(ordered(65,2)==2 && ordered(66,2)==1);
    assert(race_zl_ai.wave[0]==2 && race_zl_ai.wave_timer[0]==8125);
    /* D's first request is two Scouts, one Zealot; missing Scouts stay missing. */
    memset(u,0,sizeof(u));
    for(int i=0;i<4;i++) actor(i,66,U_DRAGOON,7);
    actor(4,70,U_SCOUT,7); actor(5,65,U_ZEALOT,7); actor(6,65,U_ZEALOT,2);
    memset(&race_zl_ai,0,sizeof(race_zl_ai)); race_zl_ai.started=2;
    race_zl_ai.wave_timer[1]=1; race_zl_ai.produce_timer[1]=1;
    race_zl_ai_tick();
    assert(ordered(70,7)==1 && ordered(65,7)==1 && ordered(65,2)==0);
    assert(race_zl_ai.wave[1]==1 && race_zl_ai.wave_timer[1]==6250);
    /* Wrapping returns to the decoded loop entry and its inherited wait. */
    race_zl_ai.wave[1]=SC64_Z10D_COUNT-1; race_zl_ai.wave_timer[1]=1;
    race_zl_ai_tick();
    assert(race_zl_ai.wave[1]==SC64_Z10D_LOOP && race_zl_ai.wave_timer[1]==11875);
    assert(RACE_ZL_AI_VALID(&race_zl_ai,SC64_Z10_GLOBAL_ID));
    race_zl_ai.wave[1]=SC64_Z10D_COUNT;
    assert(!RACE_ZL_AI_VALID(&race_zl_ai,SC64_Z10_GLOBAL_ID));
    race_zl_ai.wave[1]=SC64_Z10D_LOOP; race_zl_ai.wave_timer[1]=11876;
    assert(!RACE_ZL_AI_VALID(&race_zl_ai,SC64_Z10_GLOBAL_ID));
    puts("Z10 ordered AI first requests, partial groups, guard and loop pass");
    return 0;
}
