/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST skirmish, using the real menu, scheduler and journal. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main

int main(void)
{
    setup();
    assert(spawn_build(SIDE,B_NEXUS,80,80,0)>=0);
    assert(spawn_build(ENEMY,B_HATCHERY,500,500,0)>=0);
    minerals=100;
    int parent=spawn_unit(SIDE,U_REAVER,128,128);
    int target=spawn_unit(ENEMY,U_MARINE,180,128);
    assert(parent>=0 && target>=0);u[parent].selected=1;
    int row=(int)sc64_skirmish_menu_count((unsigned)race_human);
    game_update(KEY_B,KEY_B);assert(menu_open);
    menu_cursor=0;game_update(KEY_UP,KEY_UP);assert(menu_cursor==row);
    game_update(KEY_A,KEY_A);assert(!menu_open && minerals==85);
    Sc64ScarabParent p;Sc64ScarabCounts counts;int child=-1;
    for(int i=0;i<700;++i) {
        game_update(0,0);assert(race_scarab_parent_read(parent,0,&p,&counts));
        if(counts.inside==1)break;
    }
    assert(counts.inside==1 && !p.queued);
    for(int i=0;i<MAX_UNITS;++i) {
        int owner;unsigned phase;
        if(race_scarab_child_lookup(i,&owner,&phase) && owner==parent && phase==SC64_SCARAB_CHILD_INSIDE)child=i;
    }
    assert(child>=0 && issue_order(ORDER_ATTACK_MOVE,u[target].x,u[target].y));
    for(int i=0;i<200 && !race_scarab_probe_bullets_pending();++i)game_update(0,0);
    if (!race_scarab_probe_bullets_pending() || race_impact_count!=1 || !u[target].alive)
        fprintf(stderr,"attack state: parent alive=%d order=%d target=%d x=%d y=%d child alive=%d order=%d target=%d x=%d y=%d foe alive=%d hp=%d impacts=%u frame=%d\n",
                u[parent].alive,u[parent].order,u[parent].target,u[parent].x,u[parent].y,
                u[child].alive,u[child].order,u[child].target,u[child].x,u[child].y,
                u[target].alive,u[target].hp,race_impact_count,frame);
    assert(race_scarab_probe_bullets_pending() && race_impact_count==1 && u[target].alive);
    Sc64ScarabImpact packet;assert(sc64_scarab_impact_unpack(&race_impact_rows[0],&packet) && packet.ticks>0);
    assert(race_save_game());race_impact_reset();assert(race_load_game());
    assert(race_impact_count==1 && sc64_scarab_impact_unpack(&race_impact_rows[0],&packet) && packet.ticks>0);
    for(int i=0;i<100 && race_impact_count;++i)game_update(0,0);
    assert(!race_impact_count && !u[target].alive);
    puts("paid menu training, scheduled attack, pending journal and resumed impact passed");
    return 0;
}
