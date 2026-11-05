/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST fixture: custom game setup, real input/menu, real journal; not guest SRAM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
extern void guardian_test_reset(void);
static void press(unsigned key)
{
    game_update((u16)key,(u16)key);game_update(0,0);
}
static int egg_for(int owner,int type,int exclude)
{
    for(int i=0;i<MAX_UNITS;i++) if(i!=exclude && u[i].alive && u[i].owner==owner &&
        u[i].type==SC64_EGG_ACTOR && u[i].target==type) return i;
    return -1;
}
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
/* Check both services share the same paid future-slot budget. */
static void carrier_after_paid_egg(void)
{
    guardian_test_reset();game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_ZERG;init_state();assert(screen==SCREEN_PLAY);
    int hall=-1;for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive && b[i].owner==ENEMY &&
        sc64_race_is_zerg_townhall(b[i].type)) {hall=i;break;}
    assert(hall>=0);race_minerals[ENEMY]=1000;race_gas[ENEMY]=1000;b[hall].larva=1;
    assert(spawn_build(ENEMY,B_SPAWNING_POOL,b[hall].x+100,b[hall].y,0)>=0);
    assert(race_queue_unit(ENEMY,hall,U_ZERGLING));int pair=egg_for(ENEMY,U_ZERGLING,-1);assert(pair>=0);
    minerals=1000;int carrier=spawn_unit(SIDE,U_P7_CARRIER,100,100);assert(carrier>=0);
    int free_slots=0,last=-1;for(int i=0;i<MAX_UNITS;i++) free_slots+=!u[i].alive;
    while(free_slots>1) {last=spawn_unit(ENEMY,U_DRONE,160,160);assert(last>=0);--free_slots;}
    assert(last>=0);int before=minerals;assert(!race_p7_carrier_training_queue(carrier));
    assert(minerals==before && u[carrier].carry==0);
    hit_target(last,9999,U_MARINE,SIDE);
    assert(race_p7_carrier_training_queue(carrier));
    /* An unrelated allocation after admission must not steal the paid
     * Carrier's last slot when the paired Egg reaches completion. */
    int transient=spawn_unit(ENEMY,U_DRONE,160,160);assert(transient>=0);
    u[pair].work_timer=1;assert(race_z7_egg_tick_unit(pair));
    assert(u[pair].alive && u[pair].type==SC64_EGG_ACTOR && u[pair].work_timer==1);
    hit_target(transient,9999,U_MARINE,SIDE);
    assert(race_z7_egg_tick_unit(pair));assert(u[pair].alive && u[pair].type==U_ZERGLING);
    assert(u[carrier].carry==1);
}
#endif

int main(void)
{
    guardian_test_reset();game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_ZERG;init_state();assert(screen==SCREEN_PLAY);
    int hall=-1,enemy=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive && sc64_race_is_zerg_townhall(b[i].type)) {
        if(b[i].owner==SIDE) hall=i;else if(b[i].owner==ENEMY)enemy=i;
    }
    assert(hall>=0 && enemy>=0);minerals=1000;gas=1000;
    race_minerals[ENEMY]=1000;race_gas[ENEMY]=1000;b[hall].larva=3;b[enemy].larva=3;
    /* HOST selects the actual Hatchery; R and A use the real input path. */
    selected_building=hall;cx=b[hall].x;cy=b[hall].y;train_type=U_OVERLORD;
    int found=0;for(int i=0;i<16;i++) { press(KEY_R);if(train_type==U_DRONE){found=1;break;} }
    assert(found && selected_building==hall);
    int before=minerals;press(KEY_A);
    int a=egg_for(SIDE,U_DRONE,-1);assert(a>=0 && minerals==before-sc64_race_unit_rule(U_DRONE)->minerals);
    assert(u[a].raw_type==36 && u[a].raw_owner==-1);
    press(KEY_A);int z=egg_for(SIDE,U_DRONE,a);assert(z>=0);
    assert(race_queue_unit(ENEMY,enemy,U_DRONE));int bot=egg_for(ENEMY,U_DRONE,-1);assert(bot>=0 && u[bot].raw_owner==-1);
    assert(race_save_game());int remaining=u[a].work_timer, remaining_z=u[z].work_timer, remaining_bot=u[bot].work_timer;
    assert(remaining>0 && remaining_z>0 && remaining_bot>0);
    for(int i=0;i<3;i++)game_update(0,0);
    assert(u[a].work_timer==remaining-3 && u[z].work_timer==remaining_z-3);
    assert(race_load_game());assert(u[a].type==SC64_EGG_ACTOR && u[a].work_timer==remaining);
    /* Set selection as HOST setup, then invoke the ordinary menu with input. */
    selected_building=-1;for(int i=0;i<MAX_UNITS;i++)u[i].selected=0;u[a].selected=1;
    int cancel_row=race_context_command_base(RACE_CONTEXT_EGG);
    assert(cancel_row<race_skirmish_command_count());
    assert(!strcmp(race_menu_label(cancel_row),"CANCEL EGG"));
    before=minerals;press(KEY_B);assert(menu_open);
    for(int i=0;i<cancel_row;i++)press(KEY_DOWN);
    assert(menu_cursor==cancel_row);press(KEY_A);assert(!u[a].alive);
    assert(minerals==before+sc64_race_unit_rule(U_DRONE)->minerals);
    int wait=remaining_z>remaining_bot?remaining_z:remaining_bot;
    for(int i=0;i<wait;i++)game_update(0,0);
    assert(u[z].alive && u[z].type==U_DRONE && u[bot].alive && u[bot].type==U_DRONE);
    assert(race_save_game());assert(race_load_game());
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    carrier_after_paid_egg();
#endif
    puts("shared Egg paid input/menu, parallel birth and journal passed");return 0;
}
