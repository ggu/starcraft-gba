/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exercise the frame-scoped roster and direct legacy helper entry points. */
#define main sc64_old_interceptor_fixture_main
#include "sc64_p7_interceptor_flight_game.c"
#undef main

static void test_reuse_during_source_tick(void)
{
    static Unit before[MAX_UNITS], direct[MAX_UNITS];
    int target, parent = arena(&target, 100);
    int slot = find_state(parent, SC64_P7_INTERCEPTOR_INSIDE);
    assert(slot >= 0);
    sc64_p7_carrier_child_remove(slot, race_p7_projectile_clear_references);
    /* The first pass retires a live invalid trainer; a queued parent may
     * immediately reuse that very row for a new trainer. */
    u[slot].alive=1; u[slot].build=-1;
    sc64_p7_carrier_child_set_state(&u[slot], SC64_P7_INTERCEPTOR_TRAINING);
    u[slot].work_timer=5; u[parent].carry=1;
    u[parent].order=ORDER_IDLE; u[parent].target=-1;
    memcpy(before,u,sizeof(u));
    assert(race_p7_carrier_training_source_tick());
    race_p7_interceptor_flight_source_tick();
    race_p7_interceptor_flight_frame_move();
    memcpy(direct,u,sizeof(u));
    int created=find_state(parent,SC64_P7_INTERCEPTOR_TRAINING);
    assert(created==slot && u[created].work_timer==SC64_P7_INTERCEPTOR_BUILD_TICKS);
    memcpy(u,before,sizeof(u));
    RACE_PROTOSS_SERVICE_CLOCK[0]=0;
    race_p7_carrier_training_frame_tick();
    assert(!sc64_carrier_roster_active);
    assert(!memcmp(direct,u,sizeof(u)));
    puts("retired trainer / immediate slot reuse matched direct passes");
}

static void test_dead_parent_end_frame(void)
{
    int target, parent=arena(&target,100);
    u[parent].order=ORDER_ATTACK_TARGET; u[parent].target=target;
    source_step(8);
    assert(count_state(parent,SC64_P7_INTERCEPTOR_OUTSIDE)==2);
    u[parent].alive=0;
    race_p7_carrier_training_end_frame();
    assert(count_state(-1,SC64_P7_INTERCEPTOR_INSIDE)==0);
    assert(count_state(-1,SC64_P7_INTERCEPTOR_DETACHED)==2);
    assert(race_p7_carrier_training_live_world_valid());
    puts("dead parent cleaned inside children and detached outside children");
}

static void test_reused_parent_slot(void)
{
    int target, parent=arena(&target,100);
    u[parent].order=ORDER_ATTACK_TARGET; u[parent].target=target;
    source_step(8);
    assert(count_state(parent,SC64_P7_INTERCEPTOR_OUTSIDE)==2);
    /* A slot reused by a non-parent must not retain its old hangar. */
    u[parent].type=U_ZEALOT;
    race_p7_carrier_training_end_frame();
    assert(count_state(-1,SC64_P7_INTERCEPTOR_INSIDE)==0);
    assert(count_state(-1,SC64_P7_INTERCEPTOR_DETACHED)==2);
    puts("reused parent slot did not keep stale children");
}

static void test_full_dead_child_roster(void)
{
    int target; (void)arena(&target,100);
    for (int i=0;i<MAX_UNITS;++i) {
        memset(&u[i],0,sizeof(u[i]));
        u[i].type=U_P7_INTERCEPTOR;
        u[i].raw_type=SC64_P7_INTERCEPTOR_RAW;
        u[i].build=-1;
        sc64_p7_carrier_child_set_state(&u[i],SC64_P7_INTERCEPTOR_TRAINING);
    }
    race_p7_carrier_training_end_frame();
    for (int i=0;i<MAX_UNITS;++i)
        assert(!u[i].alive && u[i].build==-1 &&
               !(u[i].flags & SC64_P7_CARRIER_RUNTIME_MASK));
    printf("full %d-slot dead child roster cleaned\n",MAX_UNITS);
}

static void test_empty_and_skipped_source_phase(void)
{
    int target; (void)arena(&target,100);
    memset(u,0,sizeof(u));
    RACE_PROTOSS_SERVICE_CLOCK[0]=0;
    race_p7_carrier_training_frame_tick();
    assert(!sc64_carrier_roster_active && !sc64_carrier_parent_count &&
           !sc64_carrier_child_count);
    RACE_PROTOSS_SERVICE_CLOCK[0]=4;
    race_p7_carrier_training_frame_tick();
    assert(!sc64_carrier_roster_active && !sc64_carrier_parent_count &&
           !sc64_carrier_child_count);
    puts("empty due and skipped source phases leave no live roster");
}

int main(void)
{
    game_init();
    /* These use direct helper calls rather than frame_tick. */
    test_launch_attack_and_return();
    test_shield_retreat_and_hangar_rows();
    test_parent_death_detaches();
    test_reuse_during_source_tick();
    test_dead_parent_end_frame();
    test_reused_parent_slot();
    test_full_dead_child_roster();
    test_empty_and_skipped_source_phase();
    /* This uses actual frame_tick, including training, launch and reload. */
    test_game_updates();
    puts("carrier roster direct and frame tests passed");
    return 0;
}
