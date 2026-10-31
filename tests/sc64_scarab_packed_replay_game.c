/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
#include <stdlib.h>
int main(int argc,char **argv) {
    if(argc!=4)return 2;setup();minerals=500;
    int p=spawn_unit(SIDE,U_REAVER,50,50);assert(p>=0 && race_scarab_parent_init(p) && race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;assert(race_scarab_parent_read(p,0,&parent,&counts));int c=parent.active_slot;
    for(int i=0;i<105;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED && race_scarab_launch_child(p,0,c));
    int x=atoi(argv[1]),y=atoi(argv[2]),heading=atoi(argv[3]);assert(x>=0 && y>=0 && heading>=0 && heading<256);
    u[c].x=x/512;u[c].y=y/512;u[c].carry|=(x%512)<<10 | (y%512)<<19;u[c].cd=heading;u[c].target=-1;
    screen=SCREEN_PLAY;frame=0;game_update(0,0);
    int wx,wy;
    while(scanf("%d %d",&wx,&wy)==2) {
        u[c].path_x=wx;u[c].path_y=wy;screen=SCREEN_PLAY;frame=0;game_update(0,0);
        Sc64ScarabSteer state;assert(race_scarab_probe_motion_load(&u[c],&state));
        printf("%d %d %u %u\n",state.x,state.y,state.heading,state.velocity);
    }
    return 0;
}
