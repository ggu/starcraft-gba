/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Generation-43 Interceptor flight: launch cadence, attack passes through
 * moving projectiles, shield retreat, return and docking, Carrier death,
 * save/load continuation and malformed-save rejection. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "sc64_save_candidate_fixture.h"

static unsigned source_ticks;

/* 30 Hz updates on the shared 24/30 source clock: four of every five
 * updates are source updates; every update moves launched children. */
static void step(unsigned updates)
{
    for (unsigned n = 0; n < updates; ++n) {
        if (frame % 5 != 4) {
            race_p7_carrier_training_source_tick();
            race_p7_interceptor_flight_source_tick();
            ++source_ticks;
        }
        race_p7_interceptor_flight_frame_move();
        if (race_p7_active()) race_p7_projectile_source_tick();
        race_p7_carrier_training_end_frame();
        race_p7_projectile_clear_dead_references();
        ++frame;
        ++race_campaign.elapsed_frames; /* saved alongside frame */
    }
}

static void source_step(unsigned ticks)
{
    while (ticks) {
        unsigned before = source_ticks;
        step(1);
        if (source_ticks != before) --ticks;
    }
}

static int count_state(int parent, unsigned state)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR &&
            sc64_p7_carrier_child_state(&u[i]) == state &&
            (parent < 0 || u[i].build == parent)) ++n;
    return n;
}

static int find_state(int parent, unsigned state)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR &&
            sc64_p7_carrier_child_state(&u[i]) == state &&
            (parent < 0 || u[i].build == parent)) return i;
    return -1;
}

static int pool_active(void)
{
    return sc64_pool_active_head(&race_p7_projectiles) != SC64_END;
}

/* A human Carrier with a full hangar facing one enemy, with every other
 * actor removed so acquisition is unambiguous. */
static int arena(int *target_out, int distance)
{
    race_mode = 0; race_selected_mission = SC64_P7_GLOBAL_ID;
    init_state(); assert(screen == SCREEN_PLAY && !mission_unsupported_count);
    int target = -1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        u[i].selected = 0;
        /* Attackability needs sight; it is checked once placed below. */
        if (target < 0 && u[i].alive && u[i].owner == ENEMY &&
            !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) && u[i].type != U_P7_CARRIER &&
            u[i].type != U_P7_OBSERVER) target = i;
    }
    assert(target >= 0);
    for (int i = 0; i < MAX_UNITS; ++i) if (i != target) u[i].alive = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) b[i].alive = 0;
    int carrier = spawn_unit(SIDE, U_P7_CARRIER, 200, 200);
    assert(carrier >= 0 && u[carrier].raw_owner == 7);
    u[target].x = 200 + distance; u[target].y = 200;
    u[target].tx = u[target].x; u[target].ty = u[target].y;
    u[target].order = ORDER_IDLE; u[target].target = -1;
    sc64_unit_bucket_move(&u[target]); sc64_target_index_move(&u[target]);
    sc64_visibility_reset(); update_exploration();
    assert(race_p7_flight_target_ok(&u[carrier], target));
    minerals = 100;
    u[carrier].selected = 1;
    for (unsigned n = 0; n < 4; ++n) {
        choose_command(MENU_STOP + 1);
        race_p7_carrier_training_source_tick();
        for (unsigned t = 0; t < 300; ++t) race_p7_carrier_training_source_tick();
    }
    u[carrier].selected = 0;
    assert(count_state(carrier, SC64_P7_INTERCEPTOR_INSIDE) == 4);
    *target_out = target;
    return carrier;
}

static void test_launch_attack_and_return(void)
{
    int target, carrier = arena(&target, 100);
    int hp_before = u[target].hp + u[target].shield, fired = 0;
    /* Out of acquisition range: no launch and no hold. */
    u[target].x = 200 + 200; sc64_target_index_move(&u[target]);
    u[carrier].order = ORDER_ATTACK_TARGET; u[carrier].target = target;
    assert(!race_p7_interceptor_flight_carrier_holds(carrier));
    source_step(3);
    assert(count_state(carrier, SC64_P7_INTERCEPTOR_OUTSIDE) == 0);
    u[target].x = 300; sc64_target_index_move(&u[target]);
    assert(race_p7_interceptor_flight_carrier_holds(carrier));

    /* Native release cadence: one child, then a 7-source-tick timer. */
    source_step(1);
    assert(count_state(carrier, SC64_P7_INTERCEPTOR_OUTSIDE) == 1);
    assert(u[carrier].work_timer == 7);
    int first = find_state(carrier, SC64_P7_INTERCEPTOR_OUTSIDE);
    assert(u[first].order == ORDER_ATTACK_TARGET && u[first].target == target);
    assert(target_alive(first));
    /* Remaining inside ranks stay dense. */
    assert(race_p7_carrier_training_live_world_valid());
    source_step(6);
    assert(count_state(carrier, SC64_P7_INTERCEPTOR_OUTSIDE) == 1);
    source_step(1);
    assert(count_state(carrier, SC64_P7_INTERCEPTOR_OUTSIDE) == 2);
    source_step(14);
    assert(count_state(carrier, SC64_P7_INTERCEPTOR_OUTSIDE) == 4);
    assert(count_state(carrier, SC64_P7_INTERCEPTOR_INSIDE) == 0);

    /* Attack passes: the children leave the Carrier, stay within the leash,
     * fire moving projectiles and only those projectiles deal damage. */
    int moved = 0;
    for (unsigned t = 0; t < 400 && u[target].alive; ++t) {
        source_step(1);
        fired |= pool_active();
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].type == U_P7_INTERCEPTOR &&
                sc64_p7_carrier_child_state(&u[i]) == SC64_P7_INTERCEPTOR_OUTSIDE) {
                moved |= dist2(u[i].x, u[i].y, u[carrier].x, u[carrier].y) > 20 * 20;
                assert(dist2(u[i].x, u[i].y, u[carrier].x, u[carrier].y) <
                       260 * 260);
            }
        assert(race_p7_carrier_training_live_world_valid());
        if (t == 40) {
            /* Mid-flight save/load restores the exact world. */
            static Unit before[MAX_UNITS];
            static u16 pool[SC64_ARENA_WORDS];
            memcpy(before, u, sizeof(u));
            memcpy(pool, race_p7_projectile_host_words, sizeof(pool));
            int f = frame;
            assert(race_save_game() && race_load_game());
            assert(frame == f);
            for (int i = 0; i < MAX_UNITS; ++i)
                if (before[i].alive) {
                    assert(u[i].alive && u[i].x == before[i].x && u[i].y == before[i].y &&
                           u[i].flags == before[i].flags && u[i].order == before[i].order &&
                           u[i].path_retry == before[i].path_retry &&
                           u[i].work_timer == before[i].work_timer &&
                           u[i].tx == before[i].tx && u[i].ty == before[i].ty &&
                           u[i].cd == before[i].cd && u[i].target == before[i].target &&
                           u[i].path_timer == before[i].path_timer &&
                           u[i].hp == before[i].hp && u[i].shield == before[i].shield);
                }
            assert(memcmp(pool, race_p7_projectile_host_words, sizeof(pool)) == 0);
        }
    }
    assert(moved && fired);
    assert(!u[target].alive || u[target].hp + u[target].shield < hp_before);
    if (u[target].alive) hit_target(target, 10000, U_P7_INTERCEPTOR, SIDE);
    assert(!u[target].alive);

    /* No target anywhere: every child returns, docks and is repaired. */
    for (unsigned t = 0; t < 600 && count_state(carrier, SC64_P7_INTERCEPTOR_INSIDE) < 4; ++t)
        source_step(1);
    assert(count_state(carrier, SC64_P7_INTERCEPTOR_INSIDE) == 4);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR)
            assert(u[i].hp == 40 && u[i].shield == 40 && !target_alive(i));
    assert(!race_p7_interceptor_flight_carrier_holds(carrier));
    assert(race_p7_carrier_training_live_world_valid());
    assert(race_save_game() && race_load_game());
}

static void test_shield_retreat_and_hangar_rows(void)
{
    int target, carrier = arena(&target, 100);
    u[carrier].order = ORDER_ATTACK_TARGET; u[carrier].target = target;
    source_step(1);
    int child = find_state(carrier, SC64_P7_INTERCEPTOR_OUTSIDE);
    int inside = find_state(carrier, SC64_P7_INTERCEPTOR_INSIDE);
    assert(child >= 0 && inside >= 0);
    /* Hangar rows stay hidden, untargetable and unable to fire. */
    assert(!target_alive(inside) && target_alive(child));
    assert(!race_p7_projectile_fire(inside, target, 0, u[inside].x * 512,
                                    u[inside].y * 512));
    source_step(20);
    assert(u[child].order == ORDER_ATTACK_TARGET);
    /* Strictly below a quarter of maximum shields: return. */
    u[child].shield = 9;
    source_step(1);
    assert(u[child].order == ORDER_MOVE && u[child].target == -1);
    /* A child below half HP is not released again. */
    for (unsigned t = 0; t < 400 &&
         sc64_p7_carrier_child_state(&u[child]) != SC64_P7_INTERCEPTOR_INSIDE; ++t)
        source_step(1);
    assert(sc64_p7_carrier_child_state(&u[child]) == SC64_P7_INTERCEPTOR_INSIDE);
    assert(race_p7_carrier_training_live_world_valid());
}

static void test_parent_death_detaches(void)
{
    int target, carrier = arena(&target, 100);
    u[carrier].order = ORDER_ATTACK_TARGET; u[carrier].target = target;
    source_step(8);
    assert(count_state(carrier, SC64_P7_INTERCEPTOR_OUTSIDE) == 2);
    hit_target(carrier, 100000, U_MARINE, ENEMY);
    assert(!u[carrier].alive);
    /* Hangar children die with the Carrier; launched ones detach. */
    assert(count_state(-1, SC64_P7_INTERCEPTOR_INSIDE) == 0);
    assert(count_state(-1, SC64_P7_INTERCEPTOR_DETACHED) == 2);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR)
            assert(u[i].build == -1 && u[i].carry >= 15 && u[i].carry <= 45);
    assert(race_p7_carrier_training_live_world_valid());
    assert(race_save_game() && race_load_game());
    assert(count_state(-1, SC64_P7_INTERCEPTOR_DETACHED) == 2);
    source_step(46);
    for (int i = 0; i < MAX_UNITS; ++i)
        assert(!u[i].alive || u[i].type != U_P7_INTERCEPTOR);
    assert(race_save_game() && race_load_game());
}

static void reject_without_live_mutation(const RaceSave *bad, u32 container_version)
{
    static Unit before_u[MAX_UNITS];
    static Building before_b[MAX_BUILDINGS];
    static u16 before_pool[SC64_ARENA_WORDS];
    Sc64P7PersistentState before_p7 = race_p7;
    int before_frame = frame, before_minerals = minerals;
    memcpy(before_u, u, sizeof(u)); memcpy(before_b, b, sizeof(b));
    memcpy(before_pool, race_p7_projectile_host_words, sizeof(before_pool));
    if (container_version == SAVE_VERSION) write_candidate_record(bad);
    else save_write(bad, sizeof(*bad), container_version);
    assert(!race_load_game());
    assert(memcmp(before_u, u, sizeof(u)) == 0 && memcmp(before_b, b, sizeof(b)) == 0);
    assert(memcmp(before_pool, race_p7_projectile_host_words, sizeof(before_pool)) == 0);
    assert(memcmp(&before_p7, &race_p7, sizeof(before_p7)) == 0);
    assert(frame == before_frame && minerals == before_minerals);
}

static void test_save_rejection(void)
{
    static RaceSave good, bad;
    int target, carrier = arena(&target, 100);
    u[carrier].order = ORDER_ATTACK_TARGET; u[carrier].target = target;
    source_step(3);
    int child = find_state(carrier, SC64_P7_INTERCEPTOR_OUTSIDE);
    assert(child >= 0 && race_save_game());
    fixture_capture_save(&good);
    for (unsigned test = 0; test < 11; ++test) {
        int slot = test == 10 ? carrier : child;
        Unit actor = u[slot];
        if (test == 0) actor.path_retry = 2;
        if (test == 1) actor.cd = 4;
        if (test == 2) actor.path_timer = 3;
        if (test == 3) actor.carry = 5;
        if (test == 4) actor.build = target;
        if (test == 5) sc64_p7_carrier_child_set_rank(&actor, 1);
        if (test == 6) actor.selected = 1;
        if (test == 7) actor.order = ORDER_IDLE;
        if (test == 8) actor.work_timer = 16;
        if (test == 9) { sc64_p7_carrier_child_set_state(&actor,
                             SC64_P7_INTERCEPTOR_DETACHED); actor.build = -1; actor.carry = 0; }
        if (test == 10) actor.work_timer = 8;
        bad = good;
        assert(sc64_p7_typed_unit_encode(&bad.units[slot], &actor,
            (unsigned)(race_cols * 16), (unsigned)(race_rows * 16)));
        reject_without_live_mutation(&bad, SAVE_VERSION);
        write_candidate_record(&good); assert(race_load_game());
    }
    /* P6 used the same Protoss magic with generation 30. A save carrying that
     * container identity must be rejected before touching the P7 live world. */
    bad = good;
    bad.version = 0x53435030u + 30u;
    reject_without_live_mutation(&bad, 0x53435030u + 30u);
    write_candidate_record(&good); assert(race_load_game());
}

/* Full game updates: an attack-moving Carrier fights through the mission
 * opening with the ordinary order loop and source clock. */
static void test_game_updates(void)
{
    race_mode = 0; race_selected_mission = SC64_P7_GLOBAL_ID;
    init_state(); assert(screen == SCREEN_PLAY);
    int carrier = spawn_unit(SIDE, U_P7_CARRIER, 64, 64);
    assert(carrier >= 0);
    minerals = 200;
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = i == carrier;
    for (unsigned n = 0; n < 4; ++n) choose_command(MENU_STOP + 1);
    for (unsigned n = 0; n < 1600 && screen == SCREEN_PLAY; ++n) game_update(0, 0);
    assert(race_p7_carrier_training_child_count(carrier) == 4);
    /* Bring one uncloaked enemy near the Carrier, away from the enemy
     * army; Arbiter-cloaked units would need an Observer to be attacked. */
    int enemy = -1;
    for (int i = 0; i < MAX_UNITS && enemy < 0; ++i) {
        if (!u[i].alive || u[i].owner != ENEMY || u[i].type == U_P7_CARRIER ||
            SC64_CARRIER_HANGAR_HIDDEN(&u[i])) continue;
        Unit saved = u[i];
        u[i].x = u[carrier].x + 120; u[i].y = u[carrier].y;
        u[i].tx = u[i].x; u[i].ty = u[i].y; u[i].order = ORDER_IDLE; u[i].target = -1;
        sc64_unit_bucket_move(&u[i]); sc64_target_index_move(&u[i]);
        update_exploration();
        if (race_p7_flight_target_ok(&u[carrier], i)) { enemy = i; break; }
        u[i] = saved;
        sc64_unit_bucket_move(&u[i]); sc64_target_index_move(&u[i]);
    }
    assert(enemy >= 0);
    u[carrier].order = ORDER_ATTACK_TARGET; u[carrier].target = enemy;
    int launched = 0;
    for (unsigned n = 0; n < 2400 && screen == SCREEN_PLAY; ++n) {
        game_update(0, 0);
        launched |= count_state(carrier, SC64_P7_INTERCEPTOR_OUTSIDE) > 0;
        assert(race_p7_carrier_training_live_world_valid());
        if (n % 600 == 0) assert(race_save_game() && race_load_game());
    }
    assert(launched);
    /* Pause freezes flight. */
    if (screen == SCREEN_PLAY) {
        game_update(KEY_START, KEY_START); assert(screen == SCREEN_PAUSE);
        static Unit before[MAX_UNITS];
        memcpy(before, u, sizeof(u));
        for (unsigned i = 0; i < 20; ++i) game_update(0, 0);
        assert(memcmp(before, u, sizeof(u)) == 0);
    }
}

static void test_profile_matrix(void)
{
    for (int mission = 0; mission <= SC64_P7_GLOBAL_ID; ++mission) {
        race_mode = 0; race_selected_mission = mission; init_state();
        if (screen == SCREEN_PLAY && !mission_unsupported_count)
            assert(race_save_game() && race_load_game());
    }
    for (int map = 0; map < 2; ++map) for (int human = 0; human < 3; ++human)
        for (int bots = 1; bots <= 3; ++bots) {
            race_mode = 1; race_human = human; race_bots = bots; race_skirmish_map = map;
            race_skirmish_init();
            assert(race_save_game() && race_load_game());
        }
}

int main(void)
{
    game_init();
    assert(SC64_RACE_SAVE_GENERATION == 43u && sizeof(RaceSave) == 16292u + SC64_BC_SAVE_EXTRA_BYTES);
    test_launch_attack_and_return();
    test_shield_retreat_and_hangar_rows();
    test_parent_death_detaches();
    test_save_rejection();
    test_game_updates();
    test_profile_matrix();
    puts("PASS Interceptor launch cadence, flight, projectile attacks, shield retreat, return/dock, Carrier death, save continuity, 11 rejections, game updates and 18 skirmish setups");
    return 0;
}
