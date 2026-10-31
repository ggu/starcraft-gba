/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic common-service integration; actual pad/cast and slot paths. */
#include <string.h>
#define main previous_defiler_test
#include "sc64_defiler_executor_game.c"
#undef main
#ifndef SC64_P7_CARRIER_TRAINING_ENABLED
#error "requires staged Carrier training"
#endif
#ifndef SC64_SHARED_ACTOR_STATE_UNION_PROBE
#error "requires staged actor-state union"
#endif
int main(void)
{
    game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    for(int i=0;i<MAX_UNITS;i++)u[i].selected=0;
    int carrier=spawn_unit(SIDE,U_P7_CARRIER,160,160);assert(carrier>=0);
    u[carrier].selected=1;selected_building=-1;minerals=100;
    assert(race_p7_carrier_training_menu_available());
    int carrier_command=race_carrier_command_base();
    assert(carrier_command<field_menu_count() && !strcmp(race_menu_label(carrier_command),"BUILD INTERCEPTOR"));
    choose_command(carrier_command);assert(minerals==75 && u[carrier].carry==1);
    u[carrier].selected=0;
    int caster=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(caster>=0);
    u[caster].selected=1;assert(sc64_defiler_stage_selected()==caster);
    assert(!strcmp(race_menu_label(field_menu_count_base()),"DARK SWARM"));
    test_phase=0;click_cast(caster,400,320);
    for(int n=0;n<30 && !sc64_shared_projectiles.active;n++)game_update(0,0);
    assert(sc64_shared_projectiles.active);
    /* Force the real projectile to allocate a former Carrier slot. */
    while(find_unit_slot()>=0)assert(spawn_unit(SIDE,U_ZERGLING,330,320)>=0);
    u[carrier].alive=0;
    for(int n=0;n<30 && !sc64_defiler_field_get(carrier).remaining;n++)game_update(0,0);
    Sc64DarkSwarmEffect f=sc64_defiler_field_get(carrier);
    assert(f.remaining && f.x==800 && f.y==640 && !SC64_UNIT_SLOT_FREE(carrier));
    assert(!u[carrier].alive && u[carrier].raw_type==202 && !target_alive(carrier));
    assert(!race_save_game() && !race_load_game());
    test_field_remaining_set(carrier,1);
    for(int n=0;n<30 && sc64_defiler_field_get(carrier).remaining;n++)game_update(0,0);
    assert(!sc64_defiler_field_get(carrier).remaining && SC64_UNIT_SLOT_FREE(carrier));
    assert(spawn_unit(SIDE,U_ZERGLING,330,320)==carrier);
    puts("current Carrier and Defiler menus, field former-Carrier slot and reuse passed");return 0;
}
