/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic world; actual paid build, parent attack, STOP and damage paths. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
static int parent, target;
static void world(int map)
{
    setup();
    if(map) { race_skirmish_map=1; race_skirmish_init();
        memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); screen=SCREEN_PLAY; }
    assert(spawn_build(SIDE,B_NEXUS,80,80,0)>=0);
    assert(spawn_build(ENEMY,B_HATCHERY,500,500,0)>=0);
    minerals=500;
    parent=spawn_unit(SIDE,U_REAVER,128,128); assert(parent>=0);
    target=-1;
    u[parent].selected=1;
}
static void queue(void)
{
    int before=minerals;
    assert(race_scarab_human_enqueue() && minerals==before-15);
}
static int training(void)
{
    Sc64ScarabParent p; Sc64ScarabCounts c;
    for(int n=0;n<20;n++) {
        game_update(0,0);
        assert(race_scarab_parent_read(parent,0,&p,&c));
        if(p.active_slot>=0) return p.active_slot;
    }
    assert(!"training allocation timed out"); return -1;
}
static void kill_parent(void)
{
    hit_target(parent,10000,U_MARINE,ENEMY);
    assert(!u[parent].alive);
}
int main(int argc,char **argv)
{
    assert(argc==3); world(argv[2][0]=='1'); queue();
    int c=training(), bank=minerals;
    if(argv[1][0]=='t') {
        race_impact_launch(c,1,1,ENEMY,-1,U_MARINE,200);
        assert(race_impact_count==1);
        kill_parent(); assert(!u[c].alive && minerals==bank && race_impact_count==0);
    } else if(argv[1][0]=='c') {
#ifdef SC64_TRANSPORT_RUNTIME
        int carrier=spawn_unit(SIDE,U_DROPSHIP,130,128); assert(carrier>=0);
        /* The existing cargo table rejects Reaver. Exercise cleanup of an
         * injected transported row as a robustness case, not gameplay. */
        unsigned space;
        assert(!sc64_transport_space_required_raw(83,&space));
        assert(!race_transport_request_board(parent,carrier));
        u[parent].order=ORDER_TRANSPORTED; u[parent].target=carrier;
        u[carrier].alive=0; race_transport_destroy_cargo(carrier);
        assert(!u[carrier].alive && !u[parent].alive && !u[c].alive && minerals==bank);
#else
        puts("transport is absent in this vector");
#endif
    } else {
        Sc64ScarabParent p; Sc64ScarabCounts counts;
        for(int n=0;n<500;n++) {game_update(0,0);assert(race_scarab_parent_read(parent,0,&p,&counts));if(counts.inside==1)break;}
        assert(counts.inside==1);
        if(argv[1][0]=='i') {kill_parent();assert(!u[c].alive && minerals==bank);}
        else {
            target=spawn_unit(ENEMY,U_MARINE,215,128); assert(target>=0);
            u[target].hp=u[target].maxhp=1000;
            assert(issue_order(ORDER_ATTACK_MOVE,u[target].x,u[target].y));
            int slot=-1; unsigned phase=0;
            for(int n=0;n<40;n++) {game_update(0,0);if(race_scarab_child_lookup(c,&slot,&phase)&&phase==SC64_SCARAB_CHILD_OUTSIDE)break;}
            assert(slot==parent && phase==SC64_SCARAB_CHILD_OUTSIDE);
            if(argv[1][0]=='s') {
                queue(); int active=training(), paid=minerals;
                assert(race_scarab_parent_read(parent,0,&p,&counts)); int carry=u[parent].carry,timer=u[parent].work_timer;
                stop_selected();
                assert(u[parent].carry==carry && u[parent].work_timer==timer && minerals==paid && u[active].alive);
                assert(race_scarab_child_lookup(c,&slot,&phase) && slot==parent && phase==SC64_SCARAB_CHILD_SELF_DESTRUCT);
                for(int n=0;n<20 && u[c].alive;n++) game_update(0,0);
                assert(!u[c].alive && u[active].alive && minerals==paid &&
                       race_scarab_parent_read(parent,0,&p,&counts) && p.queued==1);
            } else {
                queue(); int active=training(),paid=minerals;
                int x=u[c].x,y=u[c].y,t=u[c].target;
                kill_parent();
                assert(!u[active].alive && minerals==paid && u[c].alive && u[c].x==x && u[c].y==y && u[c].target==t);
                assert(race_scarab_child_lookup(c,&slot,&phase) && slot==-1 && phase==SC64_SCARAB_CHILD_OUTSIDE);
            }
        }
    }
    if(argv[1][0]!='s' && (argv[1][0]!='c' || !u[parent].alive)) {
        int replacement=spawn_unit(SIDE,U_REAVER,128,128);
        Sc64ScarabParent p; Sc64ScarabCounts counts;
        assert(replacement==parent && race_scarab_parent_read(replacement,0,&p,&counts) &&
               p.active_slot==-1 && p.queued==0 && counts.inside==0);
        if(argv[1][0]=='o') {int slot; unsigned phase; assert(u[c].alive &&
            race_scarab_child_lookup(c,&slot,&phase) && slot==-1 && phase==SC64_SCARAB_CHILD_OUTSIDE);}
    }
    puts("paid training/inside/outside parent cleanup and STOP scenario passed");
    return 0;
}
