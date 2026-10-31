/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
int main(void) {
    setup();minerals=500;int p=spawn_unit(SIDE,U_REAVER,128,128);
    assert(p>=0 && race_scarab_parent_init(p) && race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;assert(race_scarab_parent_read(p,0,&parent,&counts));int c=parent.active_slot;
    for(int i=0;i<105;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED && race_scarab_launch_child(p,0,c));
    /* STOP must work before the first source-order initialization. */
    assert(race_scarab_stop_signal(p,0));
    race_impact_launch(c,1,1,ENEMY,0,U_MARINE,100);assert(race_impact_count==1);
    frame=0;game_update(0,0);
    assert(!u[c].alive && !invincible_units[c] && race_impact_count==0);
    /* A second fresh shot survives loss of its parent, follows a live
     * target's latest position, and keeps that position when target dies. */
    setup();minerals=500;p=spawn_unit(SIDE,U_REAVER,128,128);
    assert(p>=0 && race_scarab_parent_init(p) && race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    assert(race_scarab_parent_read(p,0,&parent,&counts));c=parent.active_slot;
    for(int i=0;i<105;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED && race_scarab_launch_child(p,0,c));
    int target=spawn_unit(ENEMY,U_ZERGLING,400,400);assert(target>=0);
    assert(spawn_unit(ENEMY,U_ZERGLING,600,600)>=0);u[c].target=target;
    assert(race_scarab_parent_lost(p,0));u[p].alive=0;frame=0;game_update(0,0);
    Sc64ScarabChildFields f;assert(sc64_scarab_child_fields_unpack(u[c].carry,u[c].work_timer,&f));
    assert(f.parent_slot==SC64_SCARAB_FIELD_NO_SLOT && f.life==90 && f.no_collision==7);
    assert(u[c].target==target && u[c].tx==u[target].x && u[c].ty==u[target].y);
    int x=u[c].tx,y=u[c].ty;u[target].alive=0;screen=SCREEN_PLAY;game_update(0,0);
    assert(u[c].target==-1 && u[c].tx==x && u[c].ty==y);
    assert(race_scarab_child_set(c,SC64_SCARAB_FIELD_NO_SLOT,SC64_SCARAB_CHILD_OUTSIDE,1,0));
    frame=0;screen=SCREEN_PLAY;game_update(0,0);assert(!u[c].alive && !invincible_units[c]);
    puts("game update handles fresh STOP, parent loss and retained target");return 0;
}
