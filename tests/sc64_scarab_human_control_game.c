/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic world; exercise the real spawn, menu command and game scheduler. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
int main(int argc,char **argv)
{
    setup();
    if (argc==2 && argv[1][0]=='1') {
        race_skirmish_map=1; race_skirmish_init();
        memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); screen=SCREEN_PLAY;
    }
    assert(spawn_build(SIDE,B_NEXUS,80,80,0)>=0);
    assert(spawn_build(ENEMY,B_HATCHERY,500,500,0)>=0);
    minerals=100;
    int p=spawn_unit(SIDE,U_REAVER,128,128);
    assert(p>=0);
    Sc64ScarabParent parent; Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(p,0,&parent,&counts));
    assert(parent.active_slot==-1 && parent.queued==0 && counts.inside==0);
    u[p].selected=1;
    int row=(int)sc64_skirmish_menu_count((unsigned)race_human);
    assert(field_menu_count()==row+1 && !strcmp(race_menu_label(row),"BUILD SCARAB"));
    game_update(KEY_B,KEY_B); assert(menu_open);
    menu_cursor=0; /* host fixture starts at the ordinary first row */
    game_update(KEY_UP,KEY_UP); assert(menu_cursor==row);
    game_update(KEY_A,KEY_A); assert(!menu_open);
    assert(minerals==85 && race_scarab_parent_read(p,0,&parent,&counts) && parent.queued==1);
    int completed=0;
    for(int n=0;n<500;n++) {
        game_update(0,0);
        assert(race_scarab_parent_read(p,0,&parent,&counts));
        if(counts.inside==1) {completed=1;break;}
    }
    assert(completed && parent.queued==0 && parent.active_slot==-1 && minerals==85);
    minerals=14; choose_command(row);
    assert(minerals==14 && race_scarab_parent_read(p,0,&parent,&counts) && parent.queued==0);
    minerals=100;
    for (int n=0;n<4;++n) choose_command(row);
    assert(minerals==40 && race_scarab_parent_read(p,0,&parent,&counts) &&
           parent.queued==4 && counts.inside==1);
    choose_command(row); /* one inside + four paid entries fills capacity */
    assert(minerals==40 && race_scarab_parent_read(p,0,&parent,&counts) && parent.queued==4);
#ifdef SC64_TRANSPORT_RUNTIME
    u[p].order=ORDER_TRANSPORTED;
    assert(race_scarab_human_selected()<0);
    u[p].order=ORDER_IDLE;
#endif
    int other=spawn_unit(SIDE,U_PROBE,130,130); assert(other>=0); u[other].selected=1;
    assert(race_scarab_human_selected()<0 && strcmp(race_menu_label(row),"BUILD SCARAB"));
    puts("real spawn, paid menu, source-tick training and rejection passed");
    return 0;
}
