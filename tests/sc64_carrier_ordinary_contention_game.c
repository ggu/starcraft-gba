/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic actor positions; real projectile adapters and ordinary attacks. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#if !defined(SC64_SHARED_CARRIER_EXPERIMENT) || !defined(SC64_SHARED_ORDINARY_STAGE)
#error "requires shared Carrier and ordinary projectile experiments"
#endif
static void setup(int *fighter,int *laser_target,int *actor,int *victim,int raw,int ground)
{
    game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    int carrier=spawn_unit(SIDE,SC64_CARRIER_TYPE,48,48);
    *fighter=spawn_unit(SIDE,SC64_INTERCEPTOR_TYPE,64,64);
    *laser_target=spawn_unit(ENEMY,U_MARINE,400,400);
    int type=sc64_race_unit_from_raw(raw);assert(type>=0);
    *actor=spawn_unit(SIDE,type,160,160);
    *victim=spawn_unit(ENEMY,ground?U_MARINE:sc64_race_unit_from_raw(12),220,160);
    assert(carrier>=0 && *fighter>=0 && *laser_target>=0 && *actor>=0 && *victim>=0);
    /* A controlled launched fighter state; no paid-training claim. */
    u[*fighter].build=(s16)carrier;assert(!SC64_CARRIER_HANGAR_HIDDEN(&u[*fighter]));
    u[*laser_target].hp=u[*laser_target].maxhp=10000;
    u[*victim].hp=u[*victim].maxhp=1000;
    u[*actor].raw_type=raw;u[*actor].order=ORDER_ATTACK_TARGET;u[*actor].target=*victim;u[*actor].cd=0;
}
static int scenario(int raw,int occupied,int ground)
{
    int fighter,laser_target,actor,victim;setup(&fighter,&laser_target,&actor,&victim,raw,ground);
    for(int i=0;i<occupied;i++)assert(race_p7_projectile_fire(fighter,laser_target,64,64*512,64*512));
    assert(sc64_shared_projectiles.active==(unsigned)occupied);
    tick_combat(&u[actor],victim);
    assert(u[actor].cd>0 && SC64_ORDINARY_PENDING(actor).active);
    /* Windup does not reserve a source bullet slot. */
    assert(sc64_shared_projectiles.active==(unsigned)occupied && u[victim].hp==1000);
    int max=raw==70?2:1,free=100-occupied,expected=free<max?free:max;
    for(int n=0;n<40 && SC64_ORDINARY_PENDING(actor).active;n++)race_shared_ordinary_tick();
    assert(!SC64_ORDINARY_PENDING(actor).active && sc64_shared_projectiles.active==(unsigned)(occupied+expected));
    int saw_hit=0;
    for(int n=0;n<200;n++) {
        race_shared_ordinary_tick();
        if(!saw_hit && u[victim].hp<1000) {
            saw_hit=1;assert(sc64_shared_projectiles.active==(unsigned)(occupied+expected));
            if(occupied+expected==100)assert(!race_p7_projectile_fire(fighter,laser_target,64,64*512,64*512));
        }
    }
    int damage=1000-u[victim].hp;
    assert(sc64_shared_projectiles.active==(unsigned)occupied);
    assert((damage>0)==(expected>0));
    race_p7_projectile_reset();assert(!sc64_shared_projectiles.active);
    return damage;
}
int main(void)
{
    int m2=scenario(0,98,0),m1=scenario(0,99,0),m0=scenario(0,100,0);
    int s2=scenario(70,98,0),s1=scenario(70,99,0),s0=scenario(70,100,0);
    int w2=scenario(8,98,0),w1=scenario(8,99,0),w0=scenario(8,100,0);
    int ba2=scenario(12,98,0),ba1=scenario(12,99,0),ba0=scenario(12,100,0);
    int bg2=scenario(12,98,1),bg1=scenario(12,99,1),bg0=scenario(12,100,1);
    assert(m2==m1 && m1>0 && !m0);
    assert(s2==2*s1 && s1>0 && !s0);
    assert(w2==w1 && w1>0 && !w0);
    assert(ba2==ba1 && ba1>0 && !ba0);
    assert(bg2==bg1 && bg1>0 && !bg0);
    /* Exercise the regular game-update dispatcher at a full arena. */
    int fighter,laser_target,actor,victim;setup(&fighter,&laser_target,&actor,&victim,8,0);
    for(int i=0;i<100;i++)assert(race_p7_projectile_fire(fighter,laser_target,64,64*512,64*512));
    tick_combat(&u[actor],victim);
    assert(SC64_ORDINARY_PENDING(actor).active && u[actor].cd>0);
    for(int n=0;n<30 && SC64_ORDINARY_PENDING(actor).active;n++) {screen=SCREEN_PLAY;game_update(0,0);}
    assert(!SC64_ORDINARY_PENDING(actor).active && u[victim].hp==1000);
    assert(sc64_shared_projectiles.active==100);
    race_p7_projectile_reset();assert(!sc64_shared_projectiles.active);
    puts("real Carrier Interceptor and ordinary source-slot contention passed");return 0;
}
