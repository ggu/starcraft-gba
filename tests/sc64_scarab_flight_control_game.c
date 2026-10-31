/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
#include "../starcraft/sc64_scarab_unit_adapter.h"
int main(void) {
    setup();minerals=500;int p=spawn_unit(SIDE,U_REAVER,128,128);
    assert(p>=0 && race_scarab_parent_init(p) && race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;assert(race_scarab_parent_read(p,0,&parent,&counts));int c=parent.active_slot;
    for(int i=0;i<105;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED && race_scarab_launch_child(p,0,c));
    assert(u[c].order==1 && u[c].work_timer==0);
    assert(race_scarab_flight_control_tick(c,0,0)==SC64_SCARAB_CONTROL_CONTINUE && u[c].order==2);
    Sc64ScarabChildFields f;assert(sc64_scarab_child_fields_unpack(u[c].carry,u[c].work_timer,&f) && f.life==90 && f.no_collision==7);
    for(int i=1;i<=7;i++) {assert(race_scarab_flight_control_tick(c,0,0)==SC64_SCARAB_CONTROL_CONTINUE);assert(sc64_scarab_child_fields_unpack(u[c].carry,u[c].work_timer,&f) && f.life==90-i && f.no_collision==7-i);}
    assert(u[c].order==3 && race_scarab_stop_signal(p,0));
    assert(race_scarab_flight_control_tick(c,0,0)==SC64_SCARAB_CONTROL_REMOVE && u[c].alive);
    assert(race_scarab_child_set(c,(unsigned)p,SC64_SCARAB_CHILD_OUTSIDE,f.life,f.no_collision));
    assert(race_scarab_flight_control_tick(c,1,0)==SC64_SCARAB_CONTROL_ARRIVED && u[c].order==4);
    assert(race_scarab_parent_lost(p,0));u[p].alive=0;
    assert(sc64_scarab_child_fields_unpack(u[c].carry,u[c].work_timer,&f) && f.parent_slot==SC64_SCARAB_FIELD_NO_SLOT);
    assert(race_scarab_flight_control_tick(c,0,1)==SC64_SCARAB_CONTROL_REMOVE && u[c].alive);
    assert(race_scarab_child_set(c,SC64_SCARAB_FIELD_NO_SLOT,SC64_SCARAB_CHILD_OUTSIDE,90,7));u[c].order=2;
    assert(race_scarab_flight_control_tick(c,1,0)==SC64_SCARAB_CONTROL_ARRIVED && u[c].order==4);
    assert(race_scarab_child_set(c,SC64_SCARAB_FIELD_NO_SLOT,SC64_SCARAB_CHILD_OUTSIDE,1,0));u[c].order=3;
    assert(race_scarab_flight_control_tick(c,0,0)==SC64_SCARAB_CONTROL_REMOVE && u[c].alive);
    assert(race_scarab_child_set(c,SC64_SCARAB_FIELD_NO_SLOT,SC64_SCARAB_CHILD_SELF_DESTRUCT,10,0));
    assert(race_scarab_flight_control_tick(c,0,0)==SC64_SCARAB_CONTROL_REMOVE && u[c].alive);
    puts("actual Unit flight control slice passed; movement and cleanup delegated");return 0;
}
