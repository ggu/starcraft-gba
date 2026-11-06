/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual engine HOST lifecycle scenes; no persisted or hardware claim. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/game.c"
#ifndef SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT
#error "requires experimental Q8 stage"
#endif
static void reset_custom(void)
{
    game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;
    race_bot_races[0]=R_PROTOSS;race_skirmish_map=0;init_state();
    memset(u,0,sizeof u);memset(b,0,sizeof b);
}
static void burn_and_construction(void)
{
    unsigned q8,shield;
    reset_custom();
    int t=spawn_build(SIDE,B_BARRACKS,300,300,0);assert(t>=0);
    int bt=MAX_UNITS+t;b[t].hp=100;
    assert(sc64_pr_q8_commit_hp(bt,100u*256u-128u));
    assert((b[t].shield_remainder&255)==128 && !sc64_pr_hp_debt[bt]);
    frame=1;race_tick_building_damage(t);
    assert(sc64_pr_q8_actor_hp(bt,&q8) && q8==100u*256u-148u);
    hit_target_impl_ex(bt,128,U_MARINE,ENEMY,SC64_STOCK_DMG_IGNORE_ARMOR,SC64_HIT_PACKET_Q8);
    assert(sc64_pr_q8_actor_hp(bt,&q8) && q8==100u*256u-276u);
    assert(sc64_building_burn_owner((unsigned)b[t].shield_remainder)==ENEMY);
    frame=2;race_tick_building_damage(t);
    assert(sc64_pr_q8_actor_hp(bt,&q8) && q8==100u*256u-296u);
    /* Burn-aware full repair helper works on the same authoritative debt. */
    b[t].hp=b[t].maxhp;
    unsigned burn_phase=(unsigned)b[t].shield_remainder;
    sc64_building_burn_full_repair((unsigned)b[t].hp,(unsigned)b[t].maxhp,
                                   &burn_phase);
    b[t].shield_remainder=(int)burn_phase;
    assert(sc64_pr_q8_actor_hp(bt,&q8) && q8==(unsigned)b[t].maxhp*256u);

    int n=spawn_build(SIDE,B_NEXUS,400,400,1);assert(n>=0);
    int bn=MAX_UNITS+n;b[n].hp=b[n].maxhp;
    assert(sc64_pr_q8_commit_hp(bn,(unsigned)b[n].maxhp*256u-128u));
    int maxshield=sc64_race_building_shields(b[n].type);assert(maxshield>0);
    b[n].shield=maxshield-1;b[n].shield_remainder=640;
    race_construction_hp_progress(n,1,1);
    assert(sc64_pr_q8_actor_hp(bn,&q8) && q8==(unsigned)b[n].maxhp*256u);
    assert(sc64_pr_q8_actor_shield(bn,&shield) && shield==(unsigned)maxshield*256u && !b[n].shield_remainder);
    /* Non-shield trap/energy alias is never read or changed as a shield. */
    b[t].shield_remainder=ENEMY*256+37;
    assert(sc64_pr_q8_actor_shield(bt,&shield) && shield==0);
    assert(sc64_pr_q8_commit_shield(bt,0) && b[t].shield_remainder==ENEMY*256+37);
    assert(!sc64_pr_q8_commit_shield(bt,1) && b[t].shield_remainder==ENEMY*256+37);
    /* Existing source morph policy adds the max-HP delta, retaining Q8. */
    int lair=spawn_build(SIDE,B_LAIR,500,500,1);assert(lair>=0);
    b[lair].hp=1000;
    assert(sc64_pr_q8_commit_hp(MAX_UNITS+lair,1000u*256u-128u));
    int source_max=sc64_race_building_rule(B_HATCHERY)->hp;
    int expected=1000+b[lair].maxhp-source_max;assert(expected>1 && expected<b[lair].maxhp);
    race_complete_building(lair);
    assert(sc64_pr_q8_actor_hp(MAX_UNITS+lair,&q8) && q8==(unsigned)expected*256u-128u);
}
#ifdef SC64_Z5_RUNTIME
static void z5_fractional_regen(void)
{
    unsigned hp;game_init();race_mode=0;race_selected_mission=SC64_Z5_GLOBAL_ID;init_state();
    assert(race_z5_active());memset(u,0,sizeof u);
    int i=spawn_unit(SIDE,U_ZERGLING,300,300);assert(i>=0);
    assert(sc64_pr_q8_commit_hp(i,(unsigned)u[i].maxhp*256u-128u));
    u[i].shield_remainder=0;
    for(int n=0;n<39;++n)race_tick_state();
    assert(sc64_pr_q8_actor_hp(i,&hp) && hp==(unsigned)u[i].maxhp*256u-4u);
    race_tick_state();
    assert(sc64_pr_q8_actor_hp(i,&hp) && hp==(unsigned)u[i].maxhp*256u && !u[i].shield_remainder);
}
#endif
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
static void colony_morph_fraction(int target,unsigned initial,unsigned expected)
{
    unsigned hp;
    game_init();race_mode=0;race_selected_mission=SC64_Z7_GLOBAL_ID;init_state();
    assert(race_colony_profile_allowed() && race_human==R_ZERG);
    memset(u,0,sizeof u);memset(b,0,sizeof b);
    int pool=spawn_build(SIDE,B_SPAWNING_POOL,300,300,0);
    int chamber=spawn_build(SIDE,B_EVOLUTION_CHAMBER,340,300,0);
    int colony=spawn_build(SIDE,B_CREEP_COLONY,380,300,0);
    assert(pool>=0 && chamber>=0 && colony>=0);
    /* Explicit fixture ownership matches Z7's source slot; no save or
     * ordinary-input construction claim is made by this HOST scene. */
    b[pool].raw_owner=b[chamber].raw_owner=b[colony].raw_owner=3;
    assert(b[colony].raw_type==143 && b[colony].maxhp==400);
    assert(sc64_pr_q8_commit_hp(MAX_UNITS+colony,initial));
    minerals=1000;gas=0;
    assert(race_colony_start_morph(colony,target));
    assert(minerals==950 && b[colony].under_construction);
    for(int tick=0;tick<SC64_Z7_SUNKEN_MORPH_TICKS;++tick) {
        assert(race_colony_pre_building_tick());
        race_tick_buildings();
        assert(race_colony_post_building_tick());
        assert(b[colony].alive);
        if(tick+1<SC64_Z7_SUNKEN_MORPH_TICKS) {
            assert(b[colony].under_construction && b[colony].type==B_CREEP_COLONY);
            assert(sc64_pr_q8_actor_hp(MAX_UNITS+colony,&hp) && hp==initial);
        }
    }
    assert(!b[colony].under_construction && b[colony].type==target);
    assert(!(b[colony].flags&(SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG)) &&
           !b[colony].queue_progress && !b[colony].shield_remainder);
    assert(sc64_pr_q8_actor_hp(MAX_UNITS+colony,&hp) && hp==expected);
}
static void colony_morphs(void)
{
    colony_morph_fraction(B_SUNKEN_COLONY,100u*256u+128u,256u);
    colony_morph_fraction(B_SUNKEN_COLONY,101u*256u+128u,256u+128u);
    colony_morph_fraction(B_SUNKEN_COLONY,399u*256u+128u,299u*256u+128u);
    colony_morph_fraction(B_SPORE_COLONY,100u*256u+128u,100u*256u+128u);
}
#endif
#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_T6_RUNTIME)
static void late_unit_repair(void)
{
    unsigned hp;game_init();race_mode=0;race_selected_mission=SC64_TERRAN_LATE_GLOBAL_T7;init_state();
    assert(race_late_active());memset(u,0,sizeof u);memset(b,0,sizeof b);
    int worker=spawn_unit(SIDE,U_SCV,300,300),v=spawn_unit(SIDE,U_VULTURE,310,300);
    assert(worker>=0 && v>=0);u[worker].raw_type=7;u[v].raw_type=2;
    assert(sc64_pr_q8_commit_hp(v,(unsigned)u[v].maxhp*256u-128u));
    assert(race_late_repair_target_unit_valid(v));
    u[worker].order=ORDER_REPAIR;u[worker].target=v;u[worker].carry=0;u[worker].work_timer=0;
    minerals=gas=10000;
    for(int n=0;n<1000 && sc64_pr_hp_debt[v];++n) {frame=n+1;race_late_repair_tick_unit(worker);}
    assert(sc64_pr_q8_actor_hp(v,&hp) && hp==(unsigned)u[v].maxhp*256u && !sc64_pr_hp_debt[v]);
    assert(!race_late_repair_target_unit_valid(v));
    int building=spawn_build(SIDE,B_BARRACKS,315,300,0);assert(building>=0);
    b[building].raw_type=sc64_race_raw_building(B_BARRACKS);
    assert(sc64_pr_q8_commit_hp(MAX_UNITS+building,(unsigned)b[building].maxhp*256u-128u));
    assert(race_late_repair_target_building_valid(building));
    u[worker].order=ORDER_REPAIR;u[worker].target=MAX_UNITS+building;
    u[worker].carry=0;u[worker].work_timer=0;
    for(int n=0;n<1000 && (b[building].shield_remainder&255);++n) {frame=n+1;race_late_repair_tick_unit(worker);}
    assert(sc64_pr_q8_actor_hp(MAX_UNITS+building,&hp) && hp==(unsigned)b[building].maxhp*256u &&
           !(b[building].shield_remainder&255));
    assert(!race_late_repair_target_building_valid(building));
}
#endif
int main(void)
{
    gba_init();burn_and_construction();
#ifdef SC64_Z5_RUNTIME
    z5_fractional_regen();
#endif
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    colony_morphs();
#endif
#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_T6_RUNTIME)
    late_unit_repair();
#endif
    puts("Q8 actual burn, construction and enabled scenario lifecycle: PASS");return 0;
}
