/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
#include <stdlib.h>
int main(int argc,char **argv) {
    int phase=(argc==2 && !strncmp(argv[1],"phase",5)) ? atoi(argv[1]+5) : -1;
    int upgrade=argc==2 && !strcmp(argv[1],"upgrade");
    int poolfull=argc==2 && !strcmp(argv[1],"poolfull");
    int destroy=argc==2 && !strcmp(argv[1],"building-destroy");
    int building=(argc==2 && !strcmp(argv[1],"building")) || destroy;
    int turncoat=argc==2 && !strcmp(argv[1],"turncoat");
    int reuse=argc==2 && !strcmp(argv[1],"reuse");
    int shield=argc==2 && !strcmp(argv[1],"shield");
    int fraction=argc==2 && !strcmp(argv[1],"fraction");assert(argc==1 || upgrade || poolfull || building || turncoat || reuse || shield || fraction || (phase>=0 && phase<5));
    if(fraction)upgrade=1;
    setup();minerals=500;
    int p=spawn_unit(SIDE,U_REAVER,128,128),t;
    if(building) {int bslot=spawn_build(ENEMY,B_DEPOT,190,128,0);assert(bslot>=0);if(destroy)b[bslot].hp=b[bslot].maxhp=80;t=MAX_UNITS+bslot;}
    else {t=spawn_unit(ENEMY,shield?U_PROBE:U_MARINE,180,128);assert(t>=0);u[t].hp=u[t].maxhp=200;if(shield)u[t].shield=20;}
    int friendly=spawn_unit(SIDE,U_PROBE,180,135),air=spawn_unit(ENEMY,U_OVERLORD,180,132);
    assert(friendly>=0 && air>=0);int friendly_hp=u[friendly].hp,air_hp=u[air].hp;
    assert(p>=0 && t>=0 && race_scarab_parent_init(p) && race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;assert(race_scarab_parent_read(p,0,&parent,&counts));int c=parent.active_slot;
    for(int i=0;i<105;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED);
    if(phase>=0) frame=phase; /* controlled game-update cadence phase */
    u[p].selected=1;assert(issue_order(ORDER_ATTACK_MOVE,target_x(t),target_y(t)));int hp=building?b[t-MAX_UNITS].hp:u[t].hp,arrived=0;
    for(int n=0;n<100;n++) {
#ifdef SC64_SCARAB_WEAPON_PROBE
        if(poolfull)for(int i=0;i<SC64_SCARAB_PROBE_BULLETS;i++)sc64_scarab_probe_bullets[i].ticks=200;
#endif
        screen=SCREEN_PLAY;game_update(0,0);assert(u[p].alive && (building?b[t-MAX_UNITS].hp:u[t].hp)==hp);
        if(u[c].alive && u[c].order==4){arrived=1;break;}}
    if(!arrived)fprintf(stderr,"no arrival parent=%d child=%d order=%d x=%d y=%d target=%d targetpos=%d,%d\n",u[p].alive,u[c].alive,u[c].order,u[c].x,u[c].y,u[c].target,target_x(t),target_y(t));
    assert(arrived && u[c].target==t && race_scarab_probe_arrived(&u[c]));
#ifdef SC64_SCARAB_WEAPON_PROBE
    assert(race_scarab_probe_bullets_pending() && !race_save_game());
    if(poolfull) {
        for(int n=0;n<10;n++){screen=SCREEN_PLAY;game_update(0,0);assert(u[t].hp==hp);}
        assert(!u[c].alive && u[p].alive);puts("full experimental bullet pool: no fabricated damage");return 0;
    }
    if(turncoat)u[t].owner=SIDE; /* Controlled ownership change: same object. */
    if(reuse) {
        hit_target_impl_ex(t,1000,U_MARINE,SIDE,-1,SC64_HIT_PRECOMPUTED_HP);
        assert(!u[t].alive);int replacement=spawn_unit(SIDE,U_PROBE,180,128);assert(replacement==t);
        hp=u[t].hp;
    }
    int collateral=-1;
    if(fraction) {collateral=spawn_unit(ENEMY,U_MARINE,u[c].x+20,u[c].y);assert(collateral>=0);u[collateral].hp=u[collateral].maxhp=200;}
    if(upgrade)sc64_scarab_probe_damage_upgrades|=1u<<SIDE;
    int armor=building?sc64_race_building_armor(b[t-MAX_UNITS].type):u[t].armor+(u[t].owner==SIDE?armor_level:0);
    int expected=reuse?0:sc64_stock_hp_damage_after_armor(100+25*upgrade-(shield?u[t].shield:0),armor,SC64_STOCK_DMG_NORMAL,SC64_STOCK_SIZE_LARGE);
    int source_ticks=0;
#ifndef SC64_DEATH_VFX_DISABLED
    int death_seen=0;u32 death_birth=0;
#endif
    while(source_ticks<5) {
        screen=SCREEN_PLAY;game_update(0,0);
        if(sc64_battlecruiser_source_tick_due())source_ticks++;
        /* Source sigorder removes the child on the next source tick, while
         * the separate bullet remains pending until its Death-script event. */
        if(source_ticks==0)assert(u[c].alive);
        else assert(!u[c].alive);
        if(source_ticks<5)assert(race_scarab_probe_bullets_pending());
#ifndef SC64_DEATH_VFX_DISABLED
        if(!u[c].alive) {
            int found=0;
            for(int i=0;i<SC64_DEATH_VFX_POOL;i++) {
                Sc64DeathVfx effect;death_vfx_test_load_row(i,&effect);
                if(effect.style && effect.raw==85) {
                    ++found;
                    if(!death_seen) { death_birth=effect.birth;assert(effect.birth==(u32)frame); }
#ifdef SC64_DEATH_SOURCE_ART
                    assert(effect.source && effect.burst_su==10 && effect.total_su==10);
#endif
                }
            }
            assert(found<=1);
            if(found)death_seen=1;
            assert(death_seen); /* Die update creates the source overlay. */
        }
#endif
        if(source_ticks<5)assert((building?b[t-MAX_UNITS].hp:u[t].hp)==hp);
        assert(u[friendly].hp==friendly_hp && u[air].hp==air_hp);
    }
#ifndef SC64_DEATH_VFX_DISABLED
    assert(death_seen);
#endif
    assert((destroy?!b[t-MAX_UNITS].alive:(building?b[t-MAX_UNITS].alive:u[t].alive)) && (building?b[t-MAX_UNITS].hp:u[t].hp)==hp-expected && !u[c].alive && u[p].alive);
    if(fraction)assert(u[collateral].hp==138); /* Q8 62.5 quantized at integer HP boundary. */
    if(shield)assert(u[t].shield==0);
    if(upgrade)sc64_scarab_probe_damage_upgrades=0;
    assert(!race_scarab_probe_bullets_pending());
    for(int n=0;n<20;n++) {screen=SCREEN_PLAY;game_update(0,0);
        assert((building?b[t-MAX_UNITS].hp:u[t].hp)==hp-expected);
#ifndef SC64_DEATH_VFX_DISABLED
        int found=0;
        for(int i=0;i<SC64_DEATH_VFX_POOL;i++) {
            Sc64DeathVfx effect;death_vfx_test_load_row(i,&effect);
            if(effect.style && effect.raw==85 && effect.birth==death_birth) {
                ++found;
                if((u32)frame-death_birth>=13u)assert(sc64_death_vfx_expired(&effect,death_vfx_now()));
            }
        }
        assert(found<=1); /* the alternate-frame scan never duplicates it */
        if(phase>=0)assert(found==1);
#endif
    }
    printf("ordinary attack: five-source-tick impact, damage=%d, friendly/air unchanged\n",expected);
#else
    puts("ordinary attack launched paid Scarab without generic hit; impact unimplemented");
#endif
    return 0;
}
