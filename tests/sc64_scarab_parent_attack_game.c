/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
int main(int argc,char **argv) {
    int upgrade=argc==2 && !strcmp(argv[1],"upgrade");
    int poolfull=argc==2 && !strcmp(argv[1],"poolfull");assert(argc==1 || upgrade || poolfull);
    setup();minerals=500;
    int p=spawn_unit(SIDE,U_REAVER,128,128),t=spawn_unit(ENEMY,U_MARINE,180,128);
    assert(t>=0);u[t].hp=u[t].maxhp=200; /* Synthetic durable damage witness. */
    int friendly=spawn_unit(SIDE,U_PROBE,180,135),air=spawn_unit(ENEMY,U_OVERLORD,180,132);
    assert(friendly>=0 && air>=0);int friendly_hp=u[friendly].hp,air_hp=u[air].hp;
    assert(p>=0 && t>=0 && race_scarab_parent_init(p) && race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;assert(race_scarab_parent_read(p,0,&parent,&counts));int c=parent.active_slot;
    for(int i=0;i<105;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED);
    u[p].selected=1;assert(issue_order(ORDER_ATTACK_MOVE,u[t].x,u[t].y));int hp=u[t].hp,arrived=0;
    for(int n=0;n<100;n++) {
#ifdef SC64_SCARAB_WEAPON_PROBE
        if(poolfull)for(int i=0;i<SC64_SCARAB_PROBE_BULLETS;i++)sc64_scarab_probe_bullets[i].ticks=200;
#endif
        screen=SCREEN_PLAY;game_update(0,0);assert(u[p].alive && u[t].hp==hp);
        if(u[c].alive && u[c].order==4){arrived=1;break;}}
    if(!arrived)fprintf(stderr,"no arrival parent=%d child=%d order=%d x=%d y=%d target=%d targetpos=%d,%d hp=%d\n",u[p].alive,u[c].alive,u[c].order,u[c].x,u[c].y,u[c].target,u[t].x,u[t].y,u[t].hp);
    assert(arrived && u[c].target==t && race_scarab_probe_arrived(&u[c]));
#ifdef SC64_SCARAB_WEAPON_PROBE
    assert(race_scarab_probe_bullets_pending() && !race_save_game());
    if(poolfull) {
        for(int n=0;n<10;n++){screen=SCREEN_PLAY;game_update(0,0);assert(u[t].hp==hp);}
        assert(!u[c].alive && u[p].alive);puts("full experimental bullet pool: no fabricated damage");return 0;
    }
    if(upgrade)sc64_scarab_probe_damage_upgrades|=1u<<SIDE;
    int expected=sc64_stock_hp_damage_after_armor(100+25*upgrade,u[t].armor,SC64_STOCK_DMG_NORMAL,SC64_STOCK_SIZE_LARGE);
    int source_ticks=0;
    while(source_ticks<5) {
        screen=SCREEN_PLAY;game_update(0,0);
        if(sc64_battlecruiser_source_tick_due())source_ticks++;
        if(source_ticks<5)assert(u[t].hp==hp);
        assert(u[friendly].hp==friendly_hp && u[air].hp==air_hp);
    }
    assert(u[t].alive && u[t].hp==hp-expected && !u[c].alive && u[p].alive);
    if(upgrade)sc64_scarab_probe_damage_upgrades=0;
    assert(!race_scarab_probe_bullets_pending());
    for(int n=0;n<20;n++) {screen=SCREEN_PLAY;game_update(0,0);
        assert(u[t].hp==hp-expected && u[friendly].hp==friendly_hp && u[air].hp==air_hp);}
    printf("ordinary attack: five-source-tick impact, damage=%d, friendly/air unchanged\n",expected);
#else
    puts("ordinary attack launched paid Scarab without generic hit; impact unimplemented");
#endif
    return 0;
}
