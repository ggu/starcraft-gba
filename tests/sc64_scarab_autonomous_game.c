/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
int main(int argc,char **argv) {
    int lost=argc==2 && !strcmp(argv[1],"lost");assert(argc==1 || lost);
    setup();minerals=500;
    int p=spawn_unit(SIDE,U_REAVER,128,128),t=spawn_unit(ENEMY,U_MARINE,220,128);
    assert(spawn_unit(ENEMY,U_MARINE,500,500)>=0);
    assert(p>=0 && t>=0 && race_scarab_parent_init(p) && race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;assert(race_scarab_parent_read(p,0,&parent,&counts));int c=parent.active_slot;
    for(int i=0;i<105;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED);
    u[p].target=t;u[p].tx=u[t].x;u[p].ty=u[t].y;int hp=u[t].hp;
    assert(race_scarab_launch_child(p,0,c));int start=u[c].x;
    u[c].x=209;assert(!race_scarab_probe_arrived(&u[c]));
    u[c].x=210;assert(race_scarab_probe_arrived(&u[c]));u[c].x=start;
    /* Remove the parent through its adapter to isolate the child from the
     * generic Reaver attack path, which is not admitted gameplay. */
    assert(race_scarab_parent_lost(p,0));u[p].alive=0;
    int arrived=0;
    for(int n=0;n<100 && u[c].alive;n++) {
        if(lost && n==8)u[t].alive=0;
        screen=SCREEN_PLAY;game_update(0,0);
        assert(u[t].hp==hp); /* No arrival-as-damage shortcut. */
        if(u[c].order==4){arrived=1;break;}
    }
    if(!arrived)fprintf(stderr,"not arrived x=%d y=%d target=%d,%d childalive=%d targetalive=%d order=%d life=%d screen=%d\n",u[c].x,u[c].y,u[t].x,u[t].y,u[c].alive,u[t].alive,u[c].order,u[c].work_timer&127,screen);
    assert(arrived && u[c].x>start && race_scarab_probe_arrived(&u[c]));
    if(lost)assert(u[c].target==-1 && u[c].x==220 && u[c].y==128);
    int near_x=u[c].x;
    for(int n=0;n<200 && u[c].alive;n++){screen=SCREEN_PLAY;game_update(0,0);assert(u[t].hp==hp);}
    assert(!u[c].alive && u[t].hp==hp);
    printf("autonomous %s arrival at %d; no invented damage; expiry retired\n",lost?"lost-target":"source-range",near_x);return 0;
}
