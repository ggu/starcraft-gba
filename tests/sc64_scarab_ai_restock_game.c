/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored HOST world; exercises the real gated Reaver lifecycle adapter. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main

int main(void)
{
    setup();
    race_bots=2; race_player[SIDE]=R_PROTOSS;
    race_player[ENEMY]=R_PROTOSS; race_player[RESCUABLE]=R_PROTOSS;
    race_bot_races[0]=R_PROTOSS; race_bot_races[1]=R_PROTOSS;
    minerals=100; race_minerals[ENEMY]=100; race_minerals[RESCUABLE]=80;
    int ai=spawn_unit(ENEMY,U_REAVER,128,128);
    int human=spawn_unit(SIDE,U_REAVER,140,128);
    int target=spawn_unit(SIDE,U_MARINE,145,128);
    assert(ai>=0 && human>=0 && target>=0);
    int hp=u[target].hp;
    tick_combat(&u[ai],target);
    assert(u[target].hp==hp); /* no generic free instant attack */
    assert(race_scarab_ai_restock_stage(ai));
    assert(race_minerals[ENEMY]==85 && race_minerals[RESCUABLE]==80 && minerals==100);
    assert(!race_scarab_ai_restock_stage(ai) && !race_scarab_ai_restock_stage(human));
    assert(race_scarab_enqueue(human,0) && minerals==85);
    Sc64ScarabParent parent; Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(ai,0,&parent,&counts) && parent.queued==1);
    u[ai].owner=RESCUABLE; /* capture before settlement must not redirect payer */
    assert(!race_scarab_cancel(ai,0,0) && race_minerals[ENEMY]==85 && race_minerals[RESCUABLE]==80);
    u[ai].owner=ENEMY;
    assert(race_scarab_cancel(ai,0,0) && race_minerals[ENEMY]==100);
    race_minerals[ENEMY]=14;
    assert(!race_scarab_ai_restock_stage(ai) && race_minerals[ENEMY]==14);
    race_minerals[ENEMY]=100;
    race_bot_races[0]=R_ZERG; /* Stale active roster cannot authorize payment. */
    assert(!race_scarab_ai_restock_stage(ai) && race_minerals[ENEMY]==100);
    race_bot_races[0]=R_PROTOSS;
    race_mode=0; /* collapsed campaign owner cannot choose a source-town wallet */
    assert(!race_scarab_ai_restock_stage(ai) && race_minerals[ENEMY]==100);
    race_mode=1;
    assert(!save_game() && !load_game() && !race_save_game() && !race_load_game());
    race_scarab_human_parent_step(ai); /* actual hooked source-tick body */
    assert(race_minerals[ENEMY]==85 && race_scarab_parent_read(ai,0,&parent,&counts));
    assert(parent.queued==1 && parent.active_slot>=0);
    for(int i=0;i<106;++i) race_scarab_human_parent_step(ai);
    assert(race_scarab_parent_read(ai,0,&parent,&counts) && counts.inside>=1);
    hit_target(ai,10000,U_MARINE,SIDE); /* real death path retires owned children */
    assert(!u[ai].alive);
    int reused=spawn_unit(RESCUABLE,U_REAVER,128,128);
    assert(reused==ai && race_scarab_ai_restock_stage(reused));
    assert(race_minerals[RESCUABLE]==65 && race_minerals[ENEMY]==85);
    puts("HOST Scarab custom payer and bounded AI restock stage passed");
    return 0;
}
