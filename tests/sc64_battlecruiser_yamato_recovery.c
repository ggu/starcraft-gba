/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void) {
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    int a=spawn_unit(ENEMY,unit_raw(12),500,500), t=spawn_unit(SIDE,unit_raw(0),510,500);
    assert(a>=0 && t>=0); u[a].work_timer=64<<3;u[a].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));
    if (!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(race_bc_yamato_tick_caster(a));
    for(int n=0;n<55;++n) {++frame;assert(race_bc_yamato_tick_caster(a));}
    assert(race_bc_yamato_launch_resolved(0,64,0,1020,1000));
    assert(race_bc_yamato_projectile_step(0) && !u[t].alive && !sc64_bc_shots[0].state);
    assert((u[a].work_timer&7)==6 && u[a].order==ORDER_ATTACK_TARGET);
    assert(race_save_game()); u[a].work_timer&=~7;
    assert(race_load_game() && (u[a].work_timer&7)==6 && !u[t].alive);
    int ticks=0;
    while(u[a].work_timer&7) {
        ++frame; ticks+=sc64_battlecruiser_source_tick_due();
        assert(race_bc_yamato_recovery_tick(&u[a]));
    }
    assert(ticks==6 && u[a].order==ORDER_IDLE && u[a].target==-1);
    /* Tagged saves reject misuse of BC energy as worker cargo and malformed
     * recovery markers before touching the live state. */
    assert(race_save_game()); int energy=u[a].carry;
    u[a].order=ORDER_MINE;u[a].target=0;assert(!race_save_game());
    u[a].order=ORDER_IDLE;u[a].target=-1;u[a].work_timer=(64<<3)|7;assert(!race_save_game());
    u[a].work_timer&=~7;assert(u[a].carry==energy);
    u[a].owner=SIDE;u[a].selected=1;u[a].order=ORDER_ATTACK_TARGET;u[a].work_timer=(64<<3)|6;
    int cooldown=u[a].build;
    stop_selected();assert(u[a].order==ORDER_IDLE && (u[a].work_timer&7)==0 && u[a].carry==energy && u[a].build==cooldown);
    /* Target removal after payment does not cancel the source animation or
     * refund energy; the cast reaches its release point and cannot fire. */
    init_state(); a=spawn_unit(ENEMY,unit_raw(12),500,500);t=spawn_unit(SIDE,unit_raw(0),510,500);
    u[a].work_timer=64<<3;u[a].carry=200*256;sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t)); if (!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(race_bc_yamato_tick_caster(a)); hit_target(t,9999,unit_raw(12),ENEMY);
    assert(!u[t].alive && sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING &&
           sc64_bc_shots[0].target==SC64_BC_SHOT_NO_TARGET && u[a].target==-1);
    assert(race_save_game()); memset(sc64_bc_shots,0,sizeof(sc64_bc_shots));
    assert(race_load_game() && sc64_bc_shots[0].target==SC64_BC_SHOT_NO_TARGET);
    for (int n=0;n<60 && sc64_bc_shots[0].state;++n) {++frame;assert(race_bc_yamato_tick_caster(a));}
    assert(!sc64_bc_shots[0].state && (u[a].work_timer&7)==6 && u[a].carry==50*256);
    /* Scripted removals can bypass the ordinary damage callback. The save
     * boundary must reconcile references before serializing the queue. */
    init_state(); a=spawn_unit(ENEMY,unit_raw(12),500,500);t=spawn_unit(SIDE,unit_raw(0),510,500);
    u[a].work_timer=64<<3;u[a].carry=200*256;sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t)); if (!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(race_bc_yamato_tick_caster(a)); u[t].alive=0;
    assert(race_save_game() && sc64_bc_shots[0].target==SC64_BC_SHOT_NO_TARGET && u[a].target==-1);
    puts("private BC early-impact recovery, target loss, order guards and STOP pass");
}
