/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored HOST pressure cases over the actual Z10 pools and trigger callback. */
#define main sc64_previous_zl_scarab_main
#include "sc64_zl_scarab_controller_game.c"
#undef main
static int free_units(void)
{ int n=0;for(int i=0;i<MAX_UNITS;++i)n+=!u[i].alive;return n; }
int main(void)
{
    int parent=-1,building=-1,last=-1;
    launch_z10_from_normal_menu();
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].type==U_REAVER && sc64_controller_unit(i)==0){parent=i;break;}
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive && !b[i].queue_count){building=i;break;}
    assert(parent>=0 && building>=0 && sc64_controller_transfer_unit(parent,7));
    race_zl_ai_minerals[1]=100;
    while(free_units()>SC64_ZL_AI_RESERVE_UNITS) {
        last=spawn_unit(ENEMY,U_ZERGLING,100,100);assert(last>=0);
    }
    assert(free_units()==SC64_ZL_AI_RESERVE_UNITS);
    Sc64ScarabParent state;Sc64ScarabCounts counts;
    assert(!race_scarab_ai_restock_stage(parent) && race_zl_ai_minerals[1]==100);
    assert(race_scarab_parent_read(parent,0,&state,&counts) && !state.queued);
    assert(last>=0);u[last].alive=0;assert(free_units()==SC64_ZL_AI_RESERVE_UNITS+1);
    assert(race_product_slots_available(SC64_ZL_AI_RESERVE_UNITS+1u));
    int old_head=b[building].queue_head, old_item=b[building].queue[0];
    b[building].queue_head=0;b[building].queue_count=1;b[building].queue[0]=U_ZERGLING;
    assert(!race_scarab_ai_restock_stage(parent) && race_zl_ai_minerals[1]==100);
    b[building].queue_count=0;b[building].queue_head=old_head;b[building].queue[0]=old_item;
    assert(race_scarab_ai_restock_stage(parent) && race_zl_ai_minerals[1]==85);
    last=spawn_unit(ENEMY,U_ZERGLING,100,100);assert(last>=0 && free_units()==SC64_ZL_AI_RESERVE_UNITS);
    assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_WAIT);
    assert(race_scarab_parent_read(parent,0,&state,&counts) && state.queued==1 && state.active_slot<0);
    assert(race_zl_ai_minerals[1]==85 && free_units()==SC64_ZL_AI_RESERVE_UNITS);
    assert(race_scarab_cancel(parent,0,0) && race_zl_ai_minerals[1]==100);
    assert(!race_scarab_ai_restock_stage(parent));
    u[last].alive=0;assert(race_scarab_ai_restock_stage(parent) && race_zl_ai_minerals[1]==85);
    last=spawn_unit(ENEMY,U_ZERGLING,100,100);assert(last>=0);
    assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_WAIT && race_zl_ai_minerals[1]==85);
    u[last].alive=0;
    assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_ALLOCATED);
    assert(free_units()==SC64_ZL_AI_RESERVE_UNITS && race_zl_ai_minerals[1]==85);
    Sc64Z8Action action={.location=1,.opcode=11,.flags=0x10,.unit=SC64_ZL_RAW_DRONE};
    assert(race_zl_create(0,&action,SC64_ZL_HUMAN_SLOT,1,0,100,100,200,200)==1);
    action.unit=SC64_ZL_RAW_CRYSTAL;
    assert(race_zl_create(0,&action,SC64_ZL_HUMAN_SLOT,1,0,100,100,200,200)==1);
    assert(free_units()==SC64_ZL_AI_RESERVE_UNITS-2);
    puts("Z10 paid Scarab reserve and actual scripted creation: PASS");return 0;
}
