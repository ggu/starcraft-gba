/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

extern int sc64_test_sram_transfer(const char *, int);
extern int sc64_test_replace_payload(u32, const void *, u32);
extern int sc64_test_remove_tag(u32);
extern void sc64_test_sram_snapshot(void *);
extern int sc64_test_sram_matches(const void *);

_Static_assert(U_P7_CARRIER == 47, "persistent carrier ID");
#ifdef SC64_SHOWCASE_CARRIER_PROBE
_Static_assert(U_P7_INTERCEPTOR == 49 && SC_RU_COUNT == 50, "append raw73");
#else
_Static_assert(SC_RU_COUNT == 49, "predecessor IDs");
#endif

static void start_world(void)
{
    game_init();
    race_mode=1; race_human=R_PROTOSS; race_bots=1; race_skirmish_map=0;
    init_state();
    assert(screen==SCREEN_PLAY);
}

static void fill_other_queues(void)
{
    race_impact_count=0;
    int caster[2], target[2];
    for (int n=0;n<2;++n) {
        caster[n]=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),500,500+100*n);
        target[n]=spawn_unit(SIDE,U_ZEALOT,640,500+100*n);
        assert(caster[n]>=0 && target[n]>=0);
        u[caster[n]].work_timer=64<<3;
        u[caster[n]].carry=200*256;
    }
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    for (int n=0;n<2;++n) assert(race_bc_yamato_issue(caster[n],target[n]));
    if (!sc64_battlecruiser_source_tick_due()) ++frame;
    for (int n=0;n<2;++n) assert(race_bc_yamato_tick_caster(caster[n]));
    for (unsigned i=0;i<RACE_IMPACT_ROWS;++i)
        race_impact_launch(target[i&1u],1+(int)i,1,ENEMY,0,U_MARINE,20u+i);
    assert(race_impact_count==32);
}

static void assert_other_queues(void)
{
    assert(race_impact_count==32);
    for (unsigned n=0;n<2;++n) {
        assert(sc64_bc_shots[n].state==SC64_BC_SHOT_CHARGING);
        assert(u[sc64_bc_shots[n].caster].alive);
        assert(u[sc64_bc_shots[n].target].alive);
    }
    for (unsigned i=0;i<RACE_IMPACT_ROWS;++i)
        assert(sc64_impact_event_target(&race_impact_rows[i])==sc64_bc_shots[i&1u].target);
}

#ifdef SC64_SHOWCASE_CARRIER_PROBE
static int train_and_fire(void)
{
    /* Keep this independent of the predecessor world used for migration. */
    start_world();
    int pylon=spawn_build(SIDE,B_PYLON,200,200,0);
    int gate=spawn_build(SIDE,B_STARGATE,248,200,0);
    int beacon=spawn_build(SIDE,B_P7_FLEET_BEACON,200,248,0);
    assert(pylon>=0 && gate>=0 && beacon>=0);
    minerals=1000; gas=1000; supply_cap=100;
    assert(race_queue_unit(SIDE,gate,U_P7_CARRIER) && minerals==650 && gas==750);
    for (unsigned n=0;n<2625;++n) race_tick_buildings();
    int carrier=-1;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].type==U_P7_CARRIER) carrier=i;
    assert(carrier>=0 && RACE_PROTOSS_RAW(&u[carrier])==72);
    u[carrier].selected=1;
    assert(race_carrier_command_base()<field_menu_count());
    choose_command(race_carrier_command_base());
    assert(u[carrier].carry==1 && minerals==625);
    race_p7_carrier_training_source_tick();
    for (unsigned n=0;n<300;++n) race_p7_carrier_training_source_tick();
    int child=-1;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].type==U_P7_INTERCEPTOR) child=i;
    assert(child>=0 && sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_INSIDE);
    int target=spawn_unit(ENEMY,U_ZEALOT,u[carrier].x+100,u[carrier].y);
    assert(target>=0);
    sc64_visibility_reset(); update_exploration();
    u[carrier].order=ORDER_ATTACK_TARGET; u[carrier].target=target;
    assert(race_p7_interceptor_flight_carrier_holds(carrier));
    for (unsigned n=0;n<180 && sc64_pool_active_head(&race_p7_projectiles)==SC64_END;++n) {
        race_p7_interceptor_flight_source_tick();
        race_p7_interceptor_flight_frame_move();
        race_p7_carrier_training_end_frame();
        ++frame;
    }
    assert(sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_OUTSIDE);
    assert(sc64_pool_active_head(&race_p7_projectiles)!=SC64_END);
    unsigned added=0;
    while (race_p7_projectile_fire(child,target,64,u[child].x*512,u[child].y*512)) ++added;
    assert(added==SC64_SHOWCASE_PROJECTILE_CAP-1);
    return child;
}

static unsigned active_shots(void)
{
    unsigned count=0;
    u8 id=sc64_pool_active_head(&race_p7_projectiles);
    for (;id!=SC64_END;++count) {
        Sc64ProjectileRow row;
        assert(count<SC64_POOL_CAPACITY && sc64_pool_get_row(&race_p7_projectiles,id,&row));
        id=row.next_link;
    }
    return count;
}

static void validate_predecessor(const char *file)
{
    assert(sc64_test_sram_transfer(file,0) && race_load_game());
    assert_other_queues();
    assert(active_shots()==0);
    int carrier=-1;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].type==U_P7_CARRIER) carrier=i;
    assert(carrier>=0 && u[carrier].x==360 && u[carrier].y==360 && u[carrier].carry==0);
}

static RaceImpactRecord raw;
static unsigned char before[32768];
static Unit actors_before[MAX_UNITS];
static Building buildings_before[MAX_BUILDINGS];
static u16 projectiles_before[SC64_ARENA_WORDS];
#endif

int main(int argc,char **argv)
{
    start_world();
#ifndef SC64_SHOWCASE_CARRIER_PROBE
    assert(argc==2);
    assert(spawn_unit(SIDE,U_P7_CARRIER,360,360)>=0);
    fill_other_queues();
    assert_other_queues();
    assert(race_save_game() && sc64_test_sram_transfer(argv[1],1));
#else
    if (argc==2) {
        assert(sc64_test_sram_transfer(argv[1],0) && race_load_game());
        assert(active_shots()==SC64_SHOWCASE_PROJECTILE_CAP);
        assert_other_queues();
        u32 len;
        u8 projectile_packet[SC64_SHOWCASE_PROJECTILE_BYTES];
        assert(save_read_latest(&raw,sizeof(raw),SC64_SHOWCASE_CARRIER_TAG,&len) && len==16368);
        assert(race_p7_projectile_encode(projectile_packet)==raw.save.interceptor_size);
        assert(!memcmp(raw.save.interceptor_projectiles,projectile_packet,sizeof(projectile_packet)));
        puts("fresh process restored all three projectile queues");
        return 0;
    }
    assert(argc==3);
    validate_predecessor(argv[1]);
    int child=train_and_fire();
    fill_other_queues();
    assert_other_queues();
    assert(race_save_game());
    u32 len;
    assert(save_read_latest(&raw,sizeof(raw),SC64_SHOWCASE_CARRIER_TAG,&len) && len==16368);
    assert(raw.save.interceptor_size==1919);
    assert(sc64_test_sram_transfer(argv[2],1));

    assert(race_load_game() && active_shots()==SC64_SHOWCASE_PROJECTILE_CAP);
    assert(u[child].alive && u[child].type==U_P7_INTERCEPTOR);
    assert_other_queues();

    /* Checksum-correct but invalid compact length: fall back using the
     * real predecessor, then remove it and verify rejection is detached. */
    raw.save.interceptor_size=1;
    assert(sc64_test_replace_payload(SC64_SHOWCASE_CARRIER_TAG,&raw,len));
    assert(race_load_game() && active_shots()==0);
    assert_other_queues();
    assert(sc64_test_remove_tag(SC64_TRANSPORT_SAVE_VERSION));
    sc64_test_sram_snapshot(before);
    memcpy(actors_before,u,sizeof(actors_before));
    memcpy(buildings_before,b,sizeof(buildings_before));
    memcpy(projectiles_before,race_p7_projectile_host_words,sizeof(projectiles_before));
    int old_frame=frame;
    u32 old_seed=seed;
    assert(!race_load_game() && !race_save_game());
    assert(frame==old_frame && seed==old_seed && active_shots()==0);
    assert(!memcmp(actors_before,u,sizeof(actors_before)));
    assert(!memcmp(buildings_before,b,sizeof(buildings_before)));
    assert(!memcmp(projectiles_before,race_p7_projectile_host_words,sizeof(projectiles_before)));
    assert_other_queues();
    assert(sc64_test_sram_matches(before));
    puts("Showcase paid Carrier and Interceptor; 95 shots, 32 impacts, two Yamato rows; predecessor fallback and detached rejection passed");
#endif
}
