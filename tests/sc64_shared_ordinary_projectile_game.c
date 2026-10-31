/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual combat entry and shared live projectile allocator; synthetic setup. */
#define main previous_yamato_interceptor_test
#include "sc64_shared_yamato_interceptor_game.c"
#undef main
#ifndef SC64_SHARED_ORDINARY_STAGE
#error "fixture needs ordinary projectile stage"
#endif
static int one_case(int raw,int occupied)
{
    int shooter,enemy,bc,victim;
    setup(&shooter,&enemy,&bc,&victim);
    u[enemy].hp=u[enemy].maxhp=1000;
    for(int i=0;i<occupied;i++)assert(race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    int type=sc64_race_unit_from_raw(raw);assert(type>=0);
    int actor=spawn_unit(SIDE,type,160,160);
    int target=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),220,160);
    assert(actor>=0 && target>=0);u[target].hp=u[target].maxhp=1000;
    u[actor].raw_type=raw;u[actor].order=ORDER_ATTACK_TARGET;u[actor].target=target;u[actor].cd=0;
    int before=u[target].hp;
    tick_combat(&u[actor],target);
    assert(u[actor].cd>0 && sc64_ordinary_pending[actor].active);
    fprintf(stderr,"raw=%d occupied=%d wait=%u,%u impact=%u,%u life=%u,%u cd=%d\n",raw,occupied,sc64_ordinary_pending[actor].wait[0],sc64_ordinary_pending[actor].wait[1],sc64_ordinary_pending[actor].impact[0],sc64_ordinary_pending[actor].impact[1],sc64_ordinary_pending[actor].life[0],sc64_ordinary_pending[actor].life[1],u[actor].cd);
    assert(sc64_shared_projectiles.active==occupied && u[target].hp==before);
    int limit=(raw==70 || raw==3)?2:1,available=100-occupied,expected=available<limit?available:limit;
    for(int i=0;i<40 && sc64_ordinary_pending[actor].active;i++)race_shared_ordinary_tick();
    assert(!sc64_ordinary_pending[actor].active);
    assert(sc64_shared_projectiles.active==(unsigned)(occupied+expected));
    /* Damage and retirement are separate: a hit must not free capacity
     * while the source-derived projectile tail remains live. */
    int saw_hit=0;
    for(int i=0;i<200;i++) {
        race_shared_ordinary_tick();
        if(!saw_hit && u[target].hp<before) {
            saw_hit=1;
            assert(sc64_shared_projectiles.active==(unsigned)(occupied+expected));
            if(occupied+expected==100)
                assert(!race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
        }
    }
    assert(sc64_shared_projectiles.active==(unsigned)occupied);
    if(expected==0)assert(u[target].hp==before && !saw_hit);else assert(saw_hit);
    int dealt=before-u[target].hp;
    race_p7_projectile_reset();assert(!sc64_shared_projectiles.active);
    return dealt;
}
static int one_ground_case(int raw,int occupied)
{
    int shooter,enemy,bc,victim;setup(&shooter,&enemy,&bc,&victim);
    u[enemy].hp=u[enemy].maxhp=1000;
    for(int i=0;i<occupied;i++)assert(race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    int actor=spawn_unit(SIDE,sc64_race_unit_from_raw(raw),160,160);
    int target=spawn_unit(ENEMY,U_MARINE,200,160);assert(actor>=0 && target>=0);
    u[target].hp=u[target].maxhp=1000;u[actor].raw_type=raw;
    u[actor].order=ORDER_ATTACK_TARGET;u[actor].target=target;
    tick_combat(&u[actor],target);assert(u[actor].cd>0 && sc64_ordinary_pending[actor].active);
    assert(sc64_shared_projectiles.active==(unsigned)occupied);
    for(int n=0;n<200;n++)race_shared_ordinary_tick();
    assert(sc64_shared_projectiles.active==(unsigned)occupied);
    int damage=1000-u[target].hp;race_p7_projectile_reset();return damage;
}
int main(void)
{
    const int ground_raw[4]={2,8,38,66};
    for(int i=0;i<4;i++) {
        int with_space=one_ground_case(ground_raw[i],99);
        int full=one_ground_case(ground_raw[i],100);
        assert(with_space>0 && !full);
    }
    int marine2=one_case(0,98),marine1=one_case(0,99),marine0=one_case(0,100);
    int scout2=one_case(70,98),scout1=one_case(70,99),scout0=one_case(70,100);
    int goliath2=one_case(3,98),goliath1=one_case(3,99),goliath0=one_case(3,100);
    assert(goliath2==2*goliath1 && goliath1>0 && goliath0==0);
    assert(marine2==marine1 && marine1>0 && marine0==0);
    assert(scout2==2*scout1 && scout1>0 && scout0==0);
    int shooter,enemy,bc,victim;setup(&shooter,&enemy,&bc,&victim);
    int actor=spawn_unit(SIDE,sc64_race_unit_from_raw(0),160,160);
    int target=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),220,160);
    assert(actor>=0 && target>=0);u[actor].raw_type=0;u[actor].order=ORDER_ATTACK_TARGET;u[actor].target=target;
    tick_combat(&u[actor],target);assert(sc64_ordinary_pending[actor].active);
    assert(!race_save_game() && !race_load_game());
    game_init();
    assert(!race_shared_ordinary_pending_any() && !sc64_shared_projectiles.active);
    setup(&shooter,&enemy,&bc,&victim);
    actor=spawn_unit(SIDE,sc64_race_unit_from_raw(0),160,160);
    target=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),220,160);
    assert(actor>=0 && target>=0);u[actor].raw_type=0;u[actor].order=ORDER_ATTACK_TARGET;u[actor].target=target;
    tick_combat(&u[actor],target);assert(sc64_ordinary_pending[actor].active);
    u[actor].selected=1;stop_selected();assert(!sc64_ordinary_pending[actor].active);
    u[actor].order=ORDER_ATTACK_TARGET;u[actor].target=target;u[actor].cd=0;
    tick_combat(&u[actor],target);assert(sc64_ordinary_pending[actor].active);
    u[target].alive=0;
    int replacement=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),220,160);
    assert(replacement==target && !sc64_ordinary_pending[actor].active);
    u[actor].target=replacement;u[actor].cd=0;tick_combat(&u[actor],replacement);
    assert(sc64_ordinary_pending[actor].active);u[actor].alive=0;
    race_shared_ordinary_tick();assert(!sc64_ordinary_pending[actor].active);
    /* An already released projectile survives its shooter but may never hit
     * a replacement occupying the dead target's slot. */
    u[actor].alive=1;u[actor].cd=0;u[actor].target=replacement;
    tick_combat(&u[actor],replacement);
    for(int i=0;i<40 && sc64_ordinary_pending[actor].active;i++)race_shared_ordinary_tick();
    assert(sc64_shared_projectiles.active==1);
    u[actor].alive=0;u[replacement].alive=0;
    int shooter_reuse=spawn_unit(SIDE,sc64_race_unit_from_raw(0),160,160);
    assert(shooter_reuse==actor);
    int fresh=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),220,160);
    assert(fresh==replacement);int fresh_hp=u[fresh].hp;
    for(int i=0;i<200;i++)race_shared_ordinary_tick();
    assert(u[fresh].hp==fresh_hp && !sc64_shared_projectiles.active);
    race_shared_ordinary_reset();assert(!sc64_shared_projectiles.active);
    /* A transiently untargetable actor cannot be reacquired by a live shot. */
    setup(&shooter,&enemy,&bc,&victim);
    actor=spawn_unit(SIDE,sc64_race_unit_from_raw(70),160,160);
    target=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),220,160);
    assert(actor>=0 && target>=0);u[target].hp=u[target].maxhp=1000;
    u[actor].raw_type=70;u[actor].order=ORDER_ATTACK_TARGET;u[actor].target=target;
    tick_combat(&u[actor],target);
    for(int i=0;i<40 && sc64_ordinary_pending[actor].active;i++)race_shared_ordinary_tick();
    assert(sc64_shared_projectiles.active==2);
    int untargetable_hp=u[target].hp;u[target].alive=0;
    race_shared_ordinary_tick();assert(sc64_shared_projectiles.active==2);
    u[target].alive=1;
    for(int i=0;i<200;i++)race_shared_ordinary_tick();
    assert(u[target].hp==untargetable_hp && !sc64_shared_projectiles.active);
    /* Shooter death and immediate reuse after release do not cancel an
     * independent projectile aimed at its original still-live target. */
    setup(&shooter,&enemy,&bc,&victim);
    actor=spawn_unit(SIDE,sc64_race_unit_from_raw(70),160,160);
    target=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),220,160);
    assert(actor>=0 && target>=0);u[target].hp=u[target].maxhp=1000;
    u[actor].raw_type=70;u[actor].order=ORDER_ATTACK_TARGET;u[actor].target=target;
    tick_combat(&u[actor],target);assert(sc64_ordinary_pending[actor].active);
    for(int i=0;i<40 && sc64_ordinary_pending[actor].active;i++)race_shared_ordinary_tick();
    assert(sc64_shared_projectiles.active==2);
    int live_target_hp=u[target].hp;
    u[actor].alive=0;int shooter_slot=spawn_unit(SIDE,sc64_race_unit_from_raw(0),160,160);
    assert(shooter_slot==actor);
    for(int i=0;i<200;i++)race_shared_ordinary_tick();
    assert(u[target].hp<live_target_hp && !sc64_shared_projectiles.active);
    /* Actual game_update dispatch, with real Interceptor occupancy at 100. */
    setup(&shooter,&enemy,&bc,&victim);
    for(int i=0;i<100;i++)assert(race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    actor=spawn_unit(SIDE,sc64_race_unit_from_raw(0),160,160);
    target=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),220,160);
    assert(actor>=0 && target>=0);u[target].hp=u[target].maxhp=1000;
    race_mode=1;
    u[actor].raw_type=0;u[actor].order=ORDER_ATTACK_MOVE;u[actor].target=-1;
    u[actor].tx=u[target].x;u[actor].ty=u[target].y;
    int health=u[target].hp;
    for(int i=0;i<30 && !sc64_ordinary_pending[actor].active;i++) {screen=SCREEN_PLAY;game_update(0,0);}
    assert(sc64_ordinary_pending[actor].active && u[actor].cd>0);
    for(int i=0;i<30 && sc64_ordinary_pending[actor].active;i++) {screen=SCREEN_PLAY;game_update(0,0);}
    assert(!sc64_ordinary_pending[actor].active && u[target].hp==health);
    assert(sc64_shared_projectiles.active==100);
    race_p7_projectile_reset();assert(!sc64_shared_projectiles.active);
    puts("selected common per-shot shared allocation and interruption passed");return 0;
}
