/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#include "sc64_save_candidate_fixture.h"
#undef main
int main(void)
{
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    int bc=unit_raw(12), marine=unit_raw(0);
    int a=spawn_unit(ENEMY,bc,500,500), t=spawn_unit(SIDE,marine,520,500);
    assert(a>=0 && t>=0); u[a].work_timer=64<<3;u[a].carry=200*256; u[a].cd=100;
    assert(!race_bc_yamato_issue(a,t)); /* research required */
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t));
    if (!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(sc64_bc_shots[0].state==SC64_BC_SHOT_APPROACH && u[a].carry==200*256);
    assert(race_bc_yamato_tick_caster(a));
    assert(sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING && u[a].carry==50*256);
    assert(u[a].cd==100 && u[a].build==13 && sc64_bc_spell_rng==953210035u);
    for (int n=0;n<12;++n) { ++frame; assert(race_bc_yamato_tick_caster(a)); }
    unsigned before=sc64_bc_shots[0].age;
    assert(before>0 && before<44 && race_save_game());
    memset(sc64_bc_shots,0,sizeof(sc64_bc_shots)); u[a].carry=0;
    assert(race_load_game() && sc64_bc_shots[0].age==before && u[a].carry==50*256);
    /* Invalid queue metadata must be rejected before any live mutation. */
    Sc64BcYamatoShot saved_shot=sc64_bc_shots[0];
    int saved_minerals=minerals;
    static RaceSave bad_record; fixture_capture_save(&bad_record);
    bad_record.battlecruiser_shots[0].phase|=0x80u;
    write_candidate_record(&bad_record);
    assert(!race_bc_shots_candidate_valid(&race_data,(unsigned)WORLD_W,(unsigned)WORLD_H));
    assert(!race_load_game());
    assert(!memcmp(&saved_shot,&sc64_bc_shots[0],sizeof(saved_shot)) &&
           minerals==saved_minerals && u[a].carry==50*256);
    assert(race_save_game());
    while (sc64_bc_shots[0].age < 43) { ++frame; assert(race_bc_yamato_tick_caster(a)); }
    assert(sc64_bc_shots[0].age==43 && sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING);
    u[a].order=ORDER_IDLE; assert(!race_bc_yamato_tick_caster(a));
    assert(!sc64_bc_shots[0].state && u[a].carry==50*256);
    /* The release event is indexed by source ticks, including all five
     * possible starting frame phases; no averaged 44-GBA-frame shortcut. */
    for (int phase=0;phase<5;++phase) {
        frame=phase; u[a].work_timer=64<<3;u[a].carry=200*256; u[a].build=-1;
        assert(race_bc_yamato_issue(a,t));
        if (!sc64_battlecruiser_source_tick_due()) { assert(race_bc_yamato_tick_caster(a)); ++frame; }
        assert(race_bc_yamato_tick_caster(a));
        int source_ticks=0;
        for (int n=0;n<55;++n) {
            ++frame;
            if (sc64_battlecruiser_source_tick_due()) ++source_ticks;
            assert(race_bc_yamato_tick_caster(a));
            if (source_ticks < 44) assert(sc64_bc_shots[0].age==source_ticks);
        }
        assert(source_ticks==44);
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
        assert(sc64_bc_shots[0].state==SC64_BC_SHOT_FLIGHT);
        sc64_bc_shot_clear(&sc64_bc_shots[0]);
        u[a].order=ORDER_IDLE;
#else
        assert(sc64_bc_shots[0].age==44 && sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING);
        u[a].order=ORDER_IDLE; assert(!race_bc_yamato_tick_caster(a));
#endif
    }
    /* Target flags allow friendly targets but reject invincible targets. */
    u[t].flags|=16; u[a].work_timer=64<<3;u[a].carry=200*256; u[a].build=-1;
    assert(!race_bc_yamato_issue(a,t)); u[t].flags&=~16;
    assert(race_bc_yamato_issue(a,a));
    int a2=spawn_unit(ENEMY,bc,540,500),a3=spawn_unit(ENEMY,bc,560,500);
    assert(a2>=0 && a3>=0); u[a2].carry=u[a3].carry=200*256;
    assert(race_bc_yamato_issue(a2,t));
    assert(!race_bc_yamato_issue(a3,t) && u[a3].carry==200*256);
    /* Recycling a target clears all reservations that still name the slot. */
    sc64_bc_shot_target_gone(sc64_bc_shots,(unsigned)t);
    assert(sc64_bc_shots[0].state && !sc64_bc_shots[1].state);
    assert(race_bc_yamato_issue(a3,t));
    /* A seeded in-flight row exercises the real save codec, without
     * claiming that the yet-unwired mover produced the row. */
    Sc64BcYamatoShot *flight=&sc64_bc_shots[1];
    flight->state=SC64_BC_SHOT_FLIGHT;
    Sc64BcMotionFields m={.x_q8=1120*256,.y_q8=1000*256,
        .destination_x=1040,.destination_y=1000,.speed=8533,.turn_rate=133,
        .heading=201,.velocity_direction=197,.lifetime=237,.turning=1}, decoded;
    assert(sc64_bc_motion_pack(flight,&m));
    Sc64BcYamatoShot expected=*flight;
    assert(race_save_game()); memset(sc64_bc_shots,0,sizeof(sc64_bc_shots));
    assert(race_load_game() && !memcmp(&sc64_bc_shots[1],&expected,sizeof(expected)));
    assert(sc64_bc_motion_unpack(&sc64_bc_shots[1],&decoded));
    assert(decoded.destination_x==1040 && decoded.speed==8533 && decoded.heading==201);
    fixture_capture_save(&bad_record);
    bad_record.battlecruiser_shots[1].motion|=0x80000000u;
    write_candidate_record(&bad_record);
    assert(!race_bc_shots_candidate_valid(&race_data,(unsigned)WORLD_W,(unsigned)WORLD_H));
    assert(!race_load_game() && !memcmp(&sc64_bc_shots[1],&expected,sizeof(expected)));
    sc64_bc_shot_target_gone(sc64_bc_shots,(unsigned)t);
    assert(sc64_bc_shots[1].state==SC64_BC_SHOT_FLIGHT &&
           sc64_bc_shots[1].target==SC64_BC_SHOT_NO_TARGET);
    expected=sc64_bc_shots[1];
    assert(race_save_game()); memset(sc64_bc_shots,0,sizeof(sc64_bc_shots));
    assert(race_load_game() && !memcmp(&sc64_bc_shots[1],&expected,sizeof(expected)));
    puts("experimental Yamato research, charge and seeded-flight save validation pass");
    return 0;
}
