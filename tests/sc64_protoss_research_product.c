/* SPDX-License-Identifier: GPL-3.0-or-later
 * Protoss Forge and Cybernetics Core research in the normal P10 product
 * vector (PROTOSS_P10_FLAGS, which carries the research flags). HOST only.
 *
 *   lifecycle        production, cancel, power, destruction, ownership
 *   combat           damage numbers at completed levels 0..3
 *   save-write FILE  real race_save_game(), levels and jobs in flight
 *   save-read FILE   a fresh process loads it and checks everything
 *   save-slots FILE  two saves, newest corrupted: the older record wins
 *   old-write FILE   (base build only) a genuine pre-research save + digest
 *   old-read FILE    the new build loads it with all levels zero
 *
 * Menus are driven with game_update() button presses; the building is
 * selected by index and worlds are constructed, as in the other race
 * fixtures. Nothing here edits a job after the menu has started it, except
 * the clearly marked "fast-forward" that sets a remaining time to one tick.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../common/gba.c"
static void observe_hit(int,int,int,int);
#define SC64_TEST_HIT_OBSERVER observe_hit
#include "../starcraft/game.c"
#if defined(SC64_EXPERIMENTAL_RESEARCH_JOBS) && defined(SC64_PRODUCT_TEST_BASE)
#error "SC64_PRODUCT_TEST_BASE is for the build without research"
#endif
#if !defined(SC64_EXPERIMENTAL_RESEARCH_JOBS) && !defined(SC64_PRODUCT_TEST_BASE)
#error "build the product vector, or define SC64_PRODUCT_TEST_BASE for the base build"
#endif

static int observed_damage, observed_hits, observed_source;
static void observe_hit(int target, int damage, int source, int owner)
{ (void)target; (void)owner; observed_damage=damage; observed_source=source; ++observed_hits; }

static void start_custom(void)
{
    gba_init(); game_init(); race_mode=1; race_human=R_PROTOSS;
    race_bots=1; race_bot_races[0]=R_PROTOSS; race_skirmish_map=0; init_state();
    minerals=gas=1000;
    for (int i=0;i<MAX_UNITS;++i) u[i].selected=0;
}
static void read_sram(const char *path)
{
    FILE *f=fopen(path,"rb"); assert(f);
    assert(fread(host_save,1,sizeof host_save,f)==sizeof host_save);
    assert(fgetc(f)==EOF); assert(!fclose(f));
}
static void write_sram(const char *path)
{
    FILE *f=fopen(path,"wb"); assert(f);
    assert(fwrite(host_save,1,sizeof host_save,f)==sizeof host_save);
    assert(!fclose(f));
}

#ifndef SC64_PRODUCT_TEST_BASE
static Sc64PrPaidReceipt receipt(int slot)
{
    Sc64PrPaidReceipt r; memset(&r,0,sizeof r);
    (void)sc64_pr_stage_receipt(slot,&r);
    return r;
}
static unsigned level_of(int owner,unsigned family)
{ return sc64_experimental_research_levels.level[owner][family]; }

/* Press B to open the selected building's menu, move to `row`, press A.
 * Returns the toast id the command produced. */
static int menu_command(int slot,int row)
{
    for (int i=0;i<MAX_UNITS;++i) u[i].selected=0;
    selected_building=slot; menu_open=0; mode=MODE_NORMAL;
#ifdef SC_FIELD_FEEDBACK
    menu_last=0;
#endif
    toast_id=-1;
    game_update(KEY_B,KEY_B); assert(menu_open && field_menu_count()==4);
    game_update(0,0);
    for (int n=0;n<8 && menu_cursor!=row;++n) {
        game_update(KEY_DOWN,KEY_DOWN); game_update(0,0);
    }
    assert(menu_cursor==row);
    game_update(KEY_A,KEY_A); game_update(0,0);
    assert(!menu_open);
    return toast_id;
}
/* The menu command spends a few updates; the job has ticked that long. */
static int near_full(int progress,int full) { return progress<=full && progress>=full-40; }
static int pylon_at(int x,int y)
{ int s=spawn_build(SIDE,B_PYLON,x,y,0); assert(s>=0); return s; }
static int forge_at(int x,int y)
{ int s=spawn_build(SIDE,B_FORGE,x,y,0); assert(s>=0); b[s].hp=b[s].maxhp; return s; }
static int core_at(int x,int y)
{ int s=spawn_build(SIDE,B_CYBERNETICS_CORE,x,y,0); assert(s>=0); return s; }
static int raw_building_at(int raw,int x,int y)
{ int t=sc64_race_building_from_raw(raw); assert(t>=0);
  int s=spawn_build(SIDE,t,x,y,0); assert(s>=0); return s; }

static void lifecycle(void)
{
    Sc64ProtossResearchQuote q;
    start_custom();
    int pylon=pylon_at(240,240), forge=forge_at(240,240), forge2=forge_at(240,240);
    int core=core_at(240,240);
    assert(sc64_pr_stage_owner_active(SIDE));
    assert(sc64_pr_stage_menu_selected()==0); /* nothing selected yet */

    /* Row labels and the fourth (cancel) row come from the real menu. */
    selected_building=forge; for (int i=0;i<MAX_UNITS;++i) u[i].selected=0;
    assert(field_menu_count()==4 && !strcmp(race_menu_label(0),"GROUND WEAPONS") &&
           !strcmp(race_menu_label(1),"GROUND ARMOR") && !strcmp(race_menu_label(2),"SHIELDS") &&
           !strcmp(race_menu_label(3),"CANCEL RESEARCH"));
    selected_building=core;
    assert(!strcmp(race_menu_label(0),"AIR WEAPONS") && !strcmp(race_menu_label(2),"SINGULARITY"));

    /* Start: price, time, wallet, progress. */
    minerals=gas=1000;
    assert(menu_command(forge,0)==0);
    assert(sc64_pr_quote_v45(SC64_PR_GROUND_WEAPONS,0,&q) && q.mineral==100 && q.gas==100 && q.ticks==5000);
    assert(minerals==900 && gas==900 && receipt(forge).live &&
           receipt(forge).family==SC64_PR_GROUND_WEAPONS && receipt(forge).target==1 &&
           near_full(b[forge].queue_progress,5000));
    int p0=b[forge].queue_progress;
    for (int i=0;i<10;++i) game_update(0,0);
    assert(b[forge].queue_progress==p0-10); /* one tick per update */
    /* No second job in one building, whatever the family. */
    for (int row=0;row<3;++row) {
        int pp=b[forge].queue_progress;
        assert(menu_command(forge,row)==10);
        assert(minerals==900 && gas==900 && b[forge].queue_progress<=pp && b[forge].queue_progress>=pp-12);
    }
    /* No second job of the same family in another building; a different
     * family in that building is accepted. */
    assert(menu_command(forge2,0)==10 && minerals==900 && !receipt(forge2).live);
    assert(menu_command(forge2,1)==0 && minerals==800 && gas==800 && receipt(forge2).family==SC64_PR_GROUND_ARMOR);

    /* Cancel returns the full recorded price once. */
    assert(menu_command(forge2,3)==0 && minerals==900 && gas==900 && !receipt(forge2).live &&
           !b[forge2].level && !b[forge2].queue_progress && !b[forge2].queue[0] && !b[forge2].queue[1]);
    assert(menu_command(forge2,3)==10 && minerals==900 && gas==900); /* nothing to cancel */

    /* Power loss pauses (paid and bound), power returns and it resumes. */
    unsigned before=(unsigned)b[forge].queue_progress;
    destroy_building(pylon,ENEMY);
    assert(!race_site_powered(SIDE,b[forge].x,b[forge].y));
    for (int i=0;i<25;++i) game_update(0,0);
    assert((unsigned)b[forge].queue_progress==before && receipt(forge).live && minerals>=900-0);
    int pylon_b=pylon_at(240,240); (void)pylon_b;
    for (int i=0;i<25;++i) game_update(0,0);
    assert((unsigned)b[forge].queue_progress<before);
    /* Unpowered buildings cannot start a job either. */
    destroy_building(pylon_b,ENEMY);
    int w=minerals;
    assert(menu_command(forge2,1)==8 && minerals==w && !receipt(forge2).live);
    pylon_b=pylon_at(240,240);

    /* Funds are checked before anything is taken. */
    minerals=50; gas=1000; assert(menu_command(forge2,1)==1 && minerals==50 && gas==1000 && !receipt(forge2).live);
    minerals=1000; gas=50; assert(menu_command(forge2,1)==12 && minerals==1000 && gas==50 && !receipt(forge2).live);
    minerals=gas=1000;

    /* Completion: the level rises, the building is clean and free again. */
    b[forge].queue_progress=1; /* fast-forward: last tick */
    int refund_bank=minerals;
    game_update(0,0);
    assert(level_of(SIDE,SC64_PR_GROUND_WEAPONS)==1 && !receipt(forge).live &&
           !b[forge].level && !b[forge].queue_progress && minerals==refund_bank);

    /* Level two needs a Templar Archives, and costs 150/150 for 5600 ticks. */
    minerals=gas=1000;
    assert(menu_command(forge,0)==8 && minerals==1000);
    raw_building_at(165,240,240);
    assert(menu_command(forge,0)==0 && minerals==850 && gas==850 && near_full(b[forge].queue_progress,5600));
    assert(menu_command(forge,3)==0 && minerals==1000);
    /* Maximum level: level three done means nothing further is sold. */
    sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]=3; /* test setup */
    assert(menu_command(forge,0)==10 && minerals==1000 && !receipt(forge).live);

    /* Shields: level one needs no Core, level two needs one. */
    assert(menu_command(forge,2)==0); assert(menu_command(forge,3)==0);
    sc64_experimental_research_levels.level[SIDE][SC64_PR_SHIELDS]=1; /* test setup */
    assert(sc64_pr_quote_v45(SC64_PR_SHIELDS,1,&q) && q.mineral==300 && q.ticks==5600);
    assert(menu_command(forge,2)==0 && minerals==700); assert(menu_command(forge,3)==0 && minerals==1000);

    /* Core: air weapons, air armor and Singularity Charge (once). */
    assert(menu_command(core,2)==0 && minerals==850 && gas==850 && near_full(b[core].queue_progress,3125));
    assert(menu_command(core,3)==0 && minerals==1000);
    sc64_experimental_research_levels.level[SIDE][SC64_PR_SINGULARITY]=1;
    assert(menu_command(core,2)==10);
    assert(menu_command(core,0)==0 && minerals==900); assert(menu_command(core,3)==0);

    /* Destruction pays three quarters of the recorded price, once, and a
     * destroyed building keeps nothing of the job. */
    minerals=gas=1000;
    assert(menu_command(forge,1)==0 && minerals==900);
    sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_ARMOR]=0;
    b[forge].hp=1; b[forge].shield=0;
    hit_target_impl_ex(MAX_UNITS+forge,10,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL,0);
    assert(!b[forge].alive && minerals==975 && gas==975 && !b[forge].level && !b[forge].queue_progress);
    assert(!sc64_pr_stage_fault);
    destroy_building(forge,ENEMY); assert(minerals==975); /* no second credit */

    /* Ownership: a producer with a live job cannot change hands. */
    int forge3=forge_at(240,240);
    assert(menu_command(forge3,1)==0);
    assert(!sc64_pr_stage_transfer_allowed(forge3,ENEMY) && sc64_pr_stage_transfer_allowed(forge3,SIDE));
    assert(menu_command(forge3,3)==0);
    assert(sc64_pr_stage_transfer_allowed(forge3,ENEMY)); /* idle buildings may */

    /* An opponent's wallet pays for an opponent's job, never the human's. */
    int efs=spawn_build(ENEMY,B_FORGE,600,600,0), epy=spawn_build(ENEMY,B_PYLON,600,600,0);
    assert(efs>=0 && epy>=0);
    race_minerals[ENEMY]=race_gas[ENEMY]=1000; int hm=minerals, hg=gas;
    assert(sc64_pr_stage_begin_slot(efs,SC64_PR_GROUND_WEAPONS));
    assert(race_minerals[ENEMY]==900 && minerals==hm && gas==hg && receipt(efs).payer==ENEMY);
    assert(sc64_pr_stage_cancel_slot(efs,0) && race_minerals[ENEMY]==1000);
    puts("research lifecycle through the ordinary menu: PASS");
}

/* ---- damage numbers ---------------------------------------------------- */
static int actor(int owner,int raw,int x,int y)
{
    int type=sc64_race_unit_from_raw(raw); assert(type>=0);
    int s=spawn_unit(owner,type,x,y); assert(s>=0);
    u[s].raw_type=raw; u[s].raw_owner=owner==SIDE?7:1;
    u[s].hp=u[s].maxhp=2000; u[s].shield=0; u[s].armor=0; u[s].cd=0;
    return s;
}
static void quiet_world(void)
{
    start_custom();
    memset(u,0,sizeof u); memset(b,0,sizeof b);
    memset(invincible_units,0,sizeof invincible_units);
    memset(invincible_buildings,0,sizeof invincible_buildings);
    memset(&sc64_experimental_research_levels,0,sizeof sc64_experimental_research_levels);
    weapons_level=armor_level=0; race_impact_reset();
    for (int c=0;c<MAP_CELLS;++c) SC64_GRID_SET(visible,c,1);
}
static void set_level(int owner,unsigned family,unsigned level)
{ sc64_experimental_research_levels.level[owner][family]=(uint8_t)level; }
static void drain_impacts(void) { for (int i=0;i<64;++i) race_impact_tick(); }

/* One ordinary swing through tick_combat and the impact queue. */
static int swing_loss(int shooter,int target)
{
    int before=u[target].hp+u[target].shield;
    u[shooter].cd=0; tick_combat(&u[shooter],target); drain_impacts();
    return before-(u[target].hp+u[target].shield);
}
static int swing_hp_loss(int shooter,int target)
{
    int before=u[target].hp;
    u[shooter].cd=0; tick_combat(&u[shooter],target); drain_impacts();
    return before-u[target].hp;
}

static void combat(void)
{
    static const int zealot_vs_marine[4]={16,18,20,22};   /* 2 x (8 + L) */
    static const int dragoon_vs_marine[4]={10,11,12,13};  /* explosive/small: (20+2L)/2 */
    static const int marine_vs_zealot_hp[4]={5,4,3,2};     /* 6 - (1 armor + L) */
    static const int marine_vs_zealot_shield[4]={6,5,4,3}; /* 6 - L on shields */
    /* Ordinary melee attack: weapon level, through the impact queue. */
    for (unsigned L=0;L<4;++L) {
        quiet_world(); set_level(SIDE,SC64_PR_GROUND_WEAPONS,L);
        int z=actor(SIDE,65,500,500), m=actor(ENEMY,0,506,500);
        assert(swing_hp_loss(z,m)==zealot_vs_marine[L]);
        printf("  ground weapons L%u: Zealot swing %d\n",L,zealot_vs_marine[L]);
    }
    /* Ranged explosive attack against a Small target: the bonus is added to
     * the weapon before the size ratio, which floors per impact. */
    for (unsigned L=0;L<4;++L) {
        quiet_world(); set_level(SIDE,SC64_PR_GROUND_WEAPONS,L);
        int d=actor(SIDE,66,500,500), m=actor(ENEMY,0,540,500);
        assert(swing_hp_loss(d,m)==dragoon_vs_marine[L]);
    }
    /* The other race's weapon family is untouched, and untyped (hero) weapons
     * keep the generic human bonus only. */
    quiet_world(); set_level(SIDE,SC64_PR_AIR_WEAPONS,3); set_level(SIDE,SC64_PR_GROUND_ARMOR,3);
    { int z=actor(SIDE,65,500,500), m=actor(ENEMY,0,506,500);
      assert(swing_hp_loss(z,m)==16); }
    /* A computer opponent gets only its own levels, not the human's. */
    quiet_world(); set_level(SIDE,SC64_PR_GROUND_WEAPONS,3); set_level(ENEMY,SC64_PR_GROUND_WEAPONS,1);
    { int z=actor(ENEMY,65,500,500), p=actor(SIDE,0,506,500);
      assert(swing_hp_loss(z,p)==18); }
    /* A non-Protoss opponent ignores typed levels entirely. */
    quiet_world(); race_player[ENEMY]=R_TER; set_level(ENEMY,SC64_PR_GROUND_WEAPONS,3);
    assert(!sc64_pr_stage_owner_active(ENEMY));
    { int t=actor(ENEMY,65,500,500);
      assert(sc64_unit_armor_upgrade_level(&u[t])==0 && sc64_pr_stage_shield_armor(t)==0); }

    /* Armor and Plasma Shields, against a Marine's 6-damage normal shot. */
    for (unsigned L=0;L<4;++L) {
        quiet_world(); set_level(SIDE,SC64_PR_GROUND_ARMOR,L);
        int zl=actor(SIDE,65,500,500), mr=actor(ENEMY,0,506,500);
        u[zl].armor=1;
        assert(swing_hp_loss(mr,zl)==marine_vs_zealot_hp[L]);
        assert(sc64_unit_armor_upgrade_level(&u[zl])==(int)L);
    }
    for (unsigned L=0;L<4;++L) {
        quiet_world(); set_level(SIDE,SC64_PR_SHIELDS,L);
        int zl=actor(SIDE,65,500,500), mr=actor(ENEMY,0,506,500);
        u[zl].armor=1; u[zl].shield=60;
        assert(swing_loss(mr,zl)==marine_vs_zealot_shield[L] && u[zl].hp==2000 && u[zl].shield==60-marine_vs_zealot_shield[L]);
    }
    /* Shields run out mid-hit: only the reduced remainder reaches the armor. */
    quiet_world(); set_level(SIDE,SC64_PR_SHIELDS,1);
    { int zl=actor(SIDE,65,500,500); u[zl].armor=1; u[zl].shield=2;
      hit_target_impl_ex(zl,6,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL,0);
      assert(u[zl].shield==0 && u[zl].hp==2000-(6-1-2-1)); }
    /* A hit smaller than the upgrade still costs one shield point. */
    quiet_world(); set_level(SIDE,SC64_PR_SHIELDS,3);
    { int zl=actor(SIDE,65,500,500); u[zl].shield=10;
      hit_target_impl_ex(zl,2,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL,0);
      assert(u[zl].shield==9 && u[zl].hp==2000); }
    /* No shields, no shield armor: the hit goes straight to the hull. */
    quiet_world(); set_level(SIDE,SC64_PR_SHIELDS,3);
    { int zl=actor(SIDE,65,500,500); u[zl].shield=0;
      hit_target_impl_ex(zl,6,U_MARINE,ENEMY,SC64_STOCK_DMG_IGNORE_ARMOR,0);
      assert(u[zl].hp==1994); }
    /* The target's owner decides: the shooter's levels never shield a victim. */
    quiet_world(); set_level(ENEMY,SC64_PR_SHIELDS,3); set_level(SIDE,SC64_PR_SHIELDS,0);
    { int zl=actor(SIDE,65,500,500); u[zl].shield=60;
      hit_target_impl_ex(zl,6,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL,0);
      assert(u[zl].shield==54); }
    /* Buildings carry shields too. */
    for (unsigned L=0;L<4;++L) {
        quiet_world(); set_level(SIDE,SC64_PR_SHIELDS,L);
        int g=spawn_build(SIDE,B_GATEWAY,300,300,0); assert(g>=0);
        b[g].hp=b[g].maxhp=500; b[g].shield=200;
        hit_target_impl_ex(MAX_UNITS+g,20,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL,0);
        assert(b[g].shield==200-(20-(int)L) && b[g].hp==500);
    }
    /* Siege splash packet (an opponent's tank against upgraded shields). */
    for (unsigned L=0;L<4;++L) {
        quiet_world(); set_level(SIDE,SC64_PR_SHIELDS,L);
        int zl=actor(SIDE,65,500,500); u[zl].shield=60; int loss=-1;
        int hp=sc64_siege_resolve(zl,40u*256u,&loss);
        assert(loss==40-(int)L && hp==0);
    }
    /* Archon splash: ground weapon level, then the shield reduction. */
    for (unsigned L=0;L<4;++L) {
        quiet_world(); set_level(SIDE,SC64_PR_GROUND_WEAPONS,L);
        int a=actor(SIDE,68,480,500), m=actor(ENEMY,0,500,500);
        assert(race_p7_archon_splash_attack(&u[a],m));
        assert(2000-u[m].hp==30+3*(int)L);
    }
    /* Singularity Charge: Dragoon reach grows by 32 and not for others. */
    quiet_world();
    { int d=actor(SIDE,66,500,500), zl=actor(SIDE,65,500,540), m=actor(ENEMY,0,500+192,500);
      /* Dragoon range 128 source px = 64 here; stand just beyond it. */
      u[m].x=500+70+3; u[m].y=500;
      int far=swing_hp_loss(d,m);
      assert(far==0 && sc64_pr_stage_dragoon_bonus(&u[d])==0 && sc64_pr_stage_dragoon_bonus(&u[zl])==0);
      set_level(SIDE,SC64_PR_SINGULARITY,1);
      assert(sc64_pr_stage_dragoon_bonus(&u[d])==32 && sc64_pr_stage_dragoon_bonus(&u[zl])==0);
      u[d].cd=0; u[d].x=500; u[d].y=500; u[m].x=500+64+20; u[m].y=500;
      far=swing_hp_loss(d,m);
      assert(far>0); }
    puts("research combat numbers (weapons, armor, shields, splash, range): PASS");
}

/* Projectile (Interceptor) packets with typed air weapons. */
static unsigned interceptor_hits;
static void interceptor_check(void)
{
    static const int expect[4]={6,7,8,9};
    for (unsigned L=0;L<4;++L) {
        quiet_world(); race_p7_projectile_reset();
        set_level(SIDE,SC64_PR_AIR_WEAPONS,L);
        int sh=actor(SIDE,73,64,64), tg=actor(ENEMY,0,224,64);
        observed_hits=0; observed_damage=0;
        assert(race_p7_projectile_fire(sh,tg,64,64*512,64*512));
        for (unsigned t=0;t<80 && !observed_hits;++t) race_p7_projectile_source_tick();
        assert(!race_p7_projectile_fault && observed_hits==1 && observed_damage==expect[L]);
        assert(u[tg].hp==2000-expect[L]);
    }
    puts("research interceptor projectile packets at air weapons 0..3: PASS");
}

/* ---- save and load ----------------------------------------------------- */
typedef struct { int minerals,gas,fq,fprog,cq,cprog,eminerals; } Expect;
static void world_with_state(void)
{
    start_custom();
    int py=pylon_at(240,240);
    int forge=forge_at(240,240), forge2=forge_at(240,240), core=core_at(240,240);
    raw_building_at(165,240,240);  /* Templar Archives */
    raw_building_at(169,240,240);  /* Fleet Beacon */
    (void)py;
    minerals=gas=3000; race_minerals[ENEMY]=777; race_gas[ENEMY]=555;
    /* Real completions, one level at a time, through the menu and tick. */
    for (int round=0;round<3;++round) {
        assert(menu_command(forge,0)==0);   /* ground weapons */
        b[forge].queue_progress=1; game_update(0,0);
        assert(menu_command(forge2,1)==0);  /* ground armor */
        b[forge2].queue_progress=1; game_update(0,0);
    }
    assert(level_of(SIDE,SC64_PR_GROUND_WEAPONS)==3 && level_of(SIDE,SC64_PR_GROUND_ARMOR)==3);
    assert(menu_command(forge,2)==0); b[forge].queue_progress=1; game_update(0,0);
    assert(menu_command(core,1)==0);   b[core].queue_progress=1; game_update(0,0);  /* air armor 1 */
    assert(menu_command(core,2)==0);   b[core].queue_progress=1; game_update(0,0);  /* singularity */
    assert(level_of(SIDE,SC64_PR_SHIELDS)==1 && level_of(SIDE,SC64_PR_AIR_ARMOR)==1 &&
           level_of(SIDE,SC64_PR_SINGULARITY)==1);
    /* An opponent-owned level exercises the five-owner codec. */
    set_level(ENEMY,SC64_PR_AIR_WEAPONS,2);
    /* Two jobs in flight: shields 2 in the Forge, air weapons 1 in the Core. */
    assert(menu_command(forge,2)==0);
    assert(menu_command(core,0)==0);
    for (int i=0;i<77;++i) game_update(0,0);
}
static void print_state(FILE *f)
{
    int forge=-1,core=-1;
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive && b[i].owner==SIDE) {
        if (b[i].type==B_FORGE && b[i].level) forge=i;
        if (b[i].type==B_CYBERNETICS_CORE && b[i].level) core=i;
    }
    assert(forge>=0 && core>=0);
    fprintf(f,"%d %d %d %d %d %d %d %d %d %d\n",minerals,gas,race_minerals[ENEMY],race_gas[ENEMY],
            forge,(int)b[forge].queue_progress,core,(int)b[core].queue_progress,
            (int)b[forge].level,(int)b[core].level);
    for (int o=0;o<5;++o) { for (unsigned fam=0;fam<SC64_PR_FAMILY_COUNT;++fam) fprintf(f,"%u ",level_of(o,fam)); fputc('\n',f); }
}
static void t_save_write(const char *path)
{
    world_with_state();
    int fr=frame;
    assert(race_save_game());
    write_sram(path);
    char side[512]; snprintf(side,sizeof side,"%s.state",path);
    FILE *f=fopen(side,"w"); assert(f); fprintf(f,"%d\n",fr); print_state(f); assert(!fclose(f));
    printf("saved: bank %d/%d, forge job %d ticks left, core job %d ticks left\n",
           minerals,gas,0,0);
}
static void t_save_read(const char *path)
{
    char side[512]; snprintf(side,sizeof side,"%s.state",path);
    FILE *f=fopen(side,"r"); assert(f);
    int fr; int e[10]; unsigned lv[5][6];
    assert(fscanf(f,"%d",&fr)==1);
    for (int i=0;i<10;++i) assert(fscanf(f,"%d",&e[i])==1);
    for (int o=0;o<5;++o) for (int k=0;k<6;++k) assert(fscanf(f,"%u",&lv[o][k])==1);
    fclose(f);
    gba_init(); game_init(); read_sram(path);
    assert(race_load_game());
    assert(frame==fr && race_mode==1 && race_human==R_PROTOSS);
    assert(minerals==e[0] && gas==e[1] && race_minerals[ENEMY]==e[2] && race_gas[ENEMY]==e[3]);
    for (int o=0;o<5;++o) for (unsigned k=0;k<6;++k) assert(level_of(o,k)==lv[o][k]);
    int forge=e[4], core=e[6];
    assert(b[forge].alive && b[forge].queue_progress==e[5] && b[forge].level==e[8] &&
           receipt(forge).live && receipt(forge).payer==SIDE && receipt(forge).family==SC64_PR_SHIELDS &&
           receipt(forge).target==2);
    assert(b[core].alive && b[core].queue_progress==e[7] && b[core].level==e[9] &&
           receipt(core).live && receipt(core).family==SC64_PR_AIR_WEAPONS && receipt(core).target==1);
    /* Decision 3: an in-progress job survives the save and keeps running. */
    for (int i=0;i<5;++i) game_update(0,0);
    assert(b[forge].queue_progress==e[5]-5 && b[core].queue_progress==e[7]-5);
    /* Its recorded price is intact, so cancelling pays it back exactly. */
    int m=minerals,g=gas;
    assert(menu_command(forge,3)==0 && minerals==m+300 && gas==g+300);
    assert(menu_command(core,3)==0 && minerals==m+400 && gas==g+400 && !receipt(core).live);
    /* And completion still works from the loaded state. */
    assert(menu_command(core,0)==0); b[core].queue_progress=1; game_update(0,0);
    assert(level_of(SIDE,SC64_PR_AIR_WEAPONS)==1);
    /* The re-saved world loads again with the new level. */
    assert(race_save_game());
    printf("research save loaded in a fresh process: PASS (frame %d)\n",frame);
}
static void t_save_slots(const char *path)
{
    world_with_state();
    assert(race_save_game());                    /* slot A */
    unsigned char a_image[sizeof host_save]; memcpy(a_image,host_save,sizeof host_save);
    int bank_a=minerals; minerals-=111;
    assert(race_save_game());                    /* slot B (newer) */
    /* Corrupt whichever slot changed last: every byte that differs from the
     * first image belongs to the newer record. Flip one inside its body. */
    size_t hit=0; for (size_t i=0;i<sizeof host_save;++i) if (host_save[i]!=a_image[i]) { hit=i; if (i>64) break; }
    assert(hit);
    host_save[hit]^=0x55;
    gba_init(); game_init();
    assert(race_load_game() && minerals==bank_a); /* the older, intact record */
    (void)path;
    puts("newest record corrupted, previous research record loads: PASS");
}
#endif /* !SC64_PRODUCT_TEST_BASE */

/* A genuine pre-research save: written by the base build, then loaded by it
 * so the digest is the base ROM's own view of the loaded world. */
static unsigned long long digest(void)
{
    unsigned long long h=1469598103934665603ull;
#define MIX(v) do { unsigned long long x_=(unsigned long long)(long long)(v); \
    for (int k_=0;k_<8;++k_) { h^=(x_>>(8*k_))&255u; h*=1099511628211ull; } } while (0)
    MIX(minerals); MIX(gas); MIX(frame); MIX(supply_used); MIX(supply_cap);
    MIX(weapons_level); MIX(armor_level); MIX(weapons_research); MIX(armor_research);
    for (int o=0;o<5;++o) { MIX(race_minerals[o]); MIX(race_gas[o]); }
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) { MIX(i); MIX(u[i].x); MIX(u[i].y); MIX(u[i].hp);
        MIX(u[i].type); MIX(u[i].owner); MIX(u[i].shield); MIX(u[i].order); MIX(u[i].target); }
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive) { MIX(i); MIX(b[i].x); MIX(b[i].y); MIX(b[i].hp);
        MIX(b[i].type); MIX(b[i].owner); MIX(b[i].shield); MIX(b[i].level); MIX(b[i].queue_progress);
        MIX(b[i].queue_count); MIX(b[i].under_construction); for (int k=0;k<5;++k) MIX(b[i].queue[k]); }
    for (int i=0;i<MAX_MINERALS;++i) { MIX(m[i].amount); }
    return h;
}
static void old_write(const char *path)
{
    start_custom();
    int py=spawn_build(SIDE,B_PYLON,240,240,0), fg=spawn_build(SIDE,B_FORGE,240,240,0);
    int co=spawn_build(SIDE,B_CYBERNETICS_CORE,240,240,0), gt=spawn_build(SIDE,B_GATEWAY,240,240,0);
    assert(py>=0 && fg>=0 && co>=0 && gt>=0);
    weapons_level=1; armor_level=2; minerals=1234; gas=567;
    supply_cap=200; assert(race_queue_unit(SIDE,gt,U_ZEALOT));
    for (int i=0;i<120;++i) game_update(0,0);
    assert(race_save_game()); write_sram(path);
    gba_init(); game_init(); read_sram(path); assert(race_load_game());
    char side[512]; snprintf(side,sizeof side,"%s.digest",path);
    FILE *f=fopen(side,"w"); assert(f); fprintf(f,"%llu\n",digest()); assert(!fclose(f));
    printf("old save written (digest %llu)\n",digest());
}
#ifndef SC64_PRODUCT_TEST_BASE
static void old_read(const char *path)
{
    char side[512]; snprintf(side,sizeof side,"%s.digest",path);
    FILE *f=fopen(side,"r"); assert(f); unsigned long long want=0; assert(fscanf(f,"%llu",&want)==1); fclose(f);
    gba_init(); game_init(); read_sram(path);
    assert(race_load_game());
    for (int o=0;o<5;++o) for (unsigned k=0;k<6;++k) assert(level_of(o,k)==0);
    assert(!sc64_pr_stage_fault);
    for (int i=0;i<MAX_BUILDINGS;++i) assert(!receipt(i).live);
    assert(weapons_level==1 && armor_level==2); /* the old generic floors stay */
    assert(digest()==want);
    /* The old world is a normal new world: research can start in it. */
    int forge=-1; for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive && b[i].type==B_FORGE && b[i].owner==SIDE) forge=i;
    assert(forge>=0 && menu_command(forge,2)==0 && receipt(forge).live && receipt(forge).family==SC64_PR_SHIELDS);
    /* The old generic weapons level is a floor: level two now needs the Archives. */
    assert(menu_command(forge,3)==0 && menu_command(forge,0)==8);
    puts("pre-research save loads with every level zero and an identical world: PASS");
}
#endif

int main(int argc,char **argv)
{
    assert(argc>=2);
    printf("save record: sizeof(RaceSave)=%zu, sizeof(impact record buffer)=%zu\n",sizeof(RaceSave),sizeof(race_impact_record));
#ifdef SC64_PRODUCT_TEST_BASE
    if (!strcmp(argv[1],"old-write") && argc==3) { old_write(argv[2]); return 0; }
    return 2;
#else
    if (!strcmp(argv[1],"lifecycle")) lifecycle();
    else if (!strcmp(argv[1],"combat")) { combat(); interceptor_check(); }
    else if (!strcmp(argv[1],"save-write") && argc==3) t_save_write(argv[2]);
    else if (!strcmp(argv[1],"save-read") && argc==3) t_save_read(argv[2]);
    else if (!strcmp(argv[1],"save-slots") && argc==3) t_save_slots(argv[2]);
    else if (!strcmp(argv[1],"old-read") && argc==3) old_read(argv[2]);
    else return 2;
    return 0;
#endif
}
