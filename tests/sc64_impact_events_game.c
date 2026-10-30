/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual-engine launch-to-impact checks, compiled with each production
 * vector. Vectors without source weapon effects must keep immediate damage.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { int target, damage, source_type, owner; } ObservedHit;
static ObservedHit observed[64];
static int observed_count;
static void observe_hit(int target, int damage, int source_type, int owner)
{
    if (observed_count < 64) {
        observed[observed_count].target = target;
        observed[observed_count].damage = damage;
        observed[observed_count].source_type = source_type;
        observed[observed_count].owner = owner;
    }
    ++observed_count;
}
#define SC64_TEST_HIT_OBSERVER(t, d, st, so) observe_hit((t), (d), (st), (so))
#include "../starcraft/game.c"

#if !defined(SC64_RACES)
#error "Use a race-engine production vector"
#endif

static void setup(void)
{
    gba_init(); game_init();
    race_mode = 1; race_human = R_PROTOSS; race_bots = 1; race_skirmish_map = 0;
    race_skirmish_init(); screen = SCREEN_PLAY;
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
#endif
    race_player[SIDE] = R_PROTOSS; race_player[ENEMY] = R_ZERG;
    selected_building = -1; menu_open = 0; mode = MODE_NORMAL;
    weapons_level = armor_level = 0;
    clear_tracers();
    observed_count = 0;
}

static int add_unit(int owner, int type, int x, int y)
{
    int i = spawn_unit(owner, type, x, y);
    assert(i >= 0);
    u[i].raw_owner = owner == SIDE ? 0 : 1;
    return i;
}

static int live_tracers(void)
{
    int n = 0;
    for (int i = 0; i < MAX_TRACERS; ++i) n += tracers[i].life != 0;
    return n;
}

#ifdef SC64_IMPACT_EVENTS
static unsigned pending(void) { return race_impact_count; }

/* Launch a Dragoon shot and return its end-of-update step count. */
static unsigned launch_dragoon(int a, int t)
{
    unsigned steps = race_impact_unit_steps(&u[a], t, 0);
    unsigned before = pending();
    assert(steps > 1);
    tick_combat(&u[a], t);
    assert(u[a].cd > 0 && pending() == before + 1);
    return steps;
}

/* Damage lands on the update whose presentation first shows the projectile's
 * Death script, and not one update earlier. */
static void test_impact_matches_presentation(void)
{
    setup();
    int a = add_unit(SIDE, U_DRAGOON, 100, 100);
    int t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    int hp = u[t].hp;
    int tracer_count = live_tracers();
    unsigned steps = launch_dragoon(a, t);
    assert(u[t].hp == hp && observed_count == 0 && live_tracers() > tracer_count);

    int raw = sc64_race_raw_unit(U_DRAGOON);
    Sc64StockAttack attack;
    assert(sc64_stock_attack_select(raw, 0, &attack));
    const Sc64WeaponSourceAttackProfile *p =
        sc64_weapon_source_profile_lookup((uint8_t)raw, attack.weapon_id, 0, 0);
    assert(p && !sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type));
    int lx, ly;
    sc64_weapon_source_trace_launch_origin(p,
        (uint8_t)sc64_source_effect_direction(u[t].x - u[a].x, u[t].y - u[a].y),
        u[a].x, u[a].y, &lx, &ly);
    uint32_t arrival = sc64_weapon_source_trace_startup_ticks(p) +
        sc64_weapon_source_trace_flight_ticks(p, lx, ly, u[t].x, u[t].y);
    uint32_t release = sc64_weapon_source_trace_release_ticks(p);
    unsigned age = steps - 1u; /* presentation age on the resolving update */
    assert(sc64_weapon_source_vfx_source_tick(age) >= release + arrival);
    assert(sc64_weapon_source_vfx_source_tick(age - 1u) < release + arrival);

    for (unsigned s = 1; s < steps; ++s) {
        update_tracers();
        assert(u[t].hp == hp && pending() == 1 && observed_count == 0);
    }
    update_tracers();
    assert(pending() == 0 && observed_count == 1);
    assert(observed[0].target == t && observed[0].source_type == U_DRAGOON &&
           observed[0].owner == SIDE);
    /* Same packet the immediate path used: stock damage, class and armor. */
    const Sc64StockUnitProfile *stock = sc64_stock_unit_profile(sc64_race_raw_unit(U_ZERGLING));
    int expected = sc64_stock_hp_damage_after_armor(
        sc64_stock_attack_damage_per_impact(&attack, 0), u[t].armor,
        attack.weapon->damage_class, stock ? stock->size : SC64_STOCK_SIZE_LARGE);
    assert(u[t].hp == hp - expected);
    for (int s = 0; s < 40; ++s) update_tracers();
    assert(u[t].hp == hp - expected && observed_count == 1);
}

static void test_moving_target_still_hit(void)
{
    setup();
    int a = add_unit(SIDE, U_DRAGOON, 100, 100);
    int t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    int hp = u[t].hp;
    unsigned steps = launch_dragoon(a, t);
    u[t].x += 300; u[t].y += 40; /* out of weapon range before impact */
    for (unsigned s = 0; s < steps; ++s) update_tracers();
    assert(u[t].hp < hp && observed_count == 1);
}

static void test_dead_target_and_slot_reuse(void)
{
    setup();
    int a = add_unit(SIDE, U_DRAGOON, 100, 100);
    int t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    unsigned steps = launch_dragoon(a, t);
    u[t].hp = 1;
    hit_target(t, 50, U_MARINE, SIDE);
    assert(!u[t].alive);
    observed_count = 0;
    for (unsigned s = 0; s < steps; ++s) update_tracers();
    assert(pending() == 0 && observed_count == 0);

    setup();
    a = add_unit(SIDE, U_DRAGOON, 100, 100);
    t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    steps = launch_dragoon(a, t);
    u[t].hp = 1;
    hit_target(t, 50, U_MARINE, SIDE);
    int reused = add_unit(SIDE, U_ZEALOT, 156, 100);
    assert(reused == t && pending() == 0); /* the new occupant is never aimed at */
    int hp = u[reused].hp, shield = u[reused].shield;
    observed_count = 0;
    for (unsigned s = 0; s < steps; ++s) update_tracers();
    assert(u[reused].hp == hp && u[reused].shield == shield && observed_count == 0);

    /* A target that is untargetable for one step loses the impact for good. */
    setup();
    a = add_unit(SIDE, U_DRAGOON, 100, 100);
    t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    hp = u[t].hp;
    steps = launch_dragoon(a, t);
    u[t].alive = 0;
    update_tracers();
    u[t].alive = 1;
    assert(pending() == 0);
    for (unsigned s = 1; s < steps; ++s) update_tracers();
    assert(u[t].hp == hp && observed_count == 0);
#ifdef SC64_T5_RUNTIME
    setup();
    a = add_unit(SIDE, U_DRAGOON, 100, 100);
    t = add_unit(ENEMY, U_MARINE, 156, 100);
    hp = u[t].hp;
    steps = launch_dragoon(a, t);
    int order = u[t].order;
    u[t].order = ORDER_TRANSPORTED; /* loaded for one step, then unloaded */
    update_tracers();
    u[t].order = order;
    for (unsigned s = 1; s < steps; ++s) update_tracers();
    assert(pending() == 0 && u[t].hp == hp && observed_count == 0);
#endif
}

static void test_attacker_death_and_ownership(void)
{
    setup();
    int a = add_unit(SIDE, U_DRAGOON, 100, 100);
    int t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    int hp = u[t].hp;
    unsigned steps = launch_dragoon(a, t);
    u[a].hp = 1; u[a].shield = 0;
    hit_target(a, 50, U_ZERGLING, ENEMY);
    assert(!u[a].alive);
    observed_count = 0;
    for (unsigned s = 0; s < steps; ++s) update_tracers();
    assert(u[t].hp < hp && observed_count == 1);
    assert(observed[0].owner == SIDE && observed[0].source_type == U_DRAGOON);

    /* The packet keeps its launch owner; an owner change on either side does
     * not cancel it (the PC reference has no hostility check at impact). */
    setup();
    a = add_unit(SIDE, U_DRAGOON, 100, 100);
    t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    hp = u[t].hp;
    steps = launch_dragoon(a, t);
    u[a].owner = ENEMY;
    u[t].owner = SIDE;
    for (unsigned s = 0; s < steps; ++s) update_tracers();
    assert(u[t].hp < hp && observed_count == 1 && observed[0].owner == SIDE);
}

/* Same-update impacts resolve in launch order; a dead target absorbs no
 * later packet and dies once. */
static void test_simultaneous_impacts(void)
{
    setup();
    int first = add_unit(SIDE, U_DRAGOON, 100, 100);
    int second = add_unit(SIDE, U_FENIX_DRAGOON, 212, 100);
    int t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    unsigned s1 = launch_dragoon(first, t);
    unsigned s2 = race_impact_unit_steps(&u[second], t, 0);
    assert(s2 == s1);
    tick_combat(&u[second], t);
    assert(pending() == 2);
    int hp = u[t].hp = 500;
    for (unsigned s = 0; s < s1; ++s) update_tracers();
    assert(observed_count == 2 && pending() == 0);
    assert(observed[0].source_type == U_DRAGOON && observed[1].source_type == U_FENIX_DRAGOON);
    assert(u[t].hp < hp);

    setup();
    first = add_unit(SIDE, U_DRAGOON, 100, 100);
    second = add_unit(SIDE, U_DRAGOON, 212, 100);
    t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    s1 = launch_dragoon(first, t);
    tick_combat(&u[second], t);
    assert(pending() == 2);
    u[t].hp = 1;
    for (unsigned s = 0; s < s1; ++s) update_tracers();
    assert(!u[t].alive && observed_count == 1 && pending() == 0);
}

static void test_shields_invincibility_and_cleanup(void)
{
    setup();
    int a = add_unit(ENEMY, U_DRAGOON, 100, 100);
    int t = add_unit(SIDE, U_ZEALOT, 156, 100);
    unsigned steps = launch_dragoon(a, t);
    int hp = u[t].hp;
    u[t].shield = 5; /* shields are read at impact, not at launch */
    for (unsigned s = 0; s < steps; ++s) update_tracers();
    assert(u[t].shield == 0 && u[t].hp < hp);

    setup();
    a = add_unit(ENEMY, U_DRAGOON, 100, 100);
    t = add_unit(SIDE, U_ZEALOT, 156, 100);
    steps = launch_dragoon(a, t);
    hp = u[t].hp;
    int shield = u[t].shield;
    u[t].flags |= 16; /* invincible from launch to impact */
    for (unsigned s = 0; s < steps; ++s) update_tracers();
    assert(pending() == 0 && u[t].hp == hp && u[t].shield == shield);
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    setup();
    a = add_unit(ENEMY, U_DRAGOON, 100, 100);
    t = add_unit(SIDE, U_ZEALOT, 156, 100);
    steps = launch_dragoon(a, t);
    hp = u[t].hp; shield = u[t].shield;
    invincible_units[t] = 1;
    for (unsigned s = 0; s < steps; ++s) update_tracers();
    assert(pending() == 0 && u[t].hp == hp && u[t].shield == shield);
#endif

    /* A lethal impact uses the ordinary death path exactly once. */
    setup();
    a = add_unit(ENEMY, U_DRAGOON, 100, 100);
    t = add_unit(SIDE, U_ZEALOT, 156, 100);
    steps = launch_dragoon(a, t);
    u[t].shield = 0; u[t].hp = 1;
    int supply = supply_used;
    for (unsigned s = 0; s < steps; ++s) update_tracers();
    assert(!u[t].alive && supply_used == supply - unit_slot_supply(&u[t]));
}

/* A full queue keeps the weapon ready: no effect, cooldown or damage until
 * a row resolves. */
static void test_overflow_defers_launch(void)
{
    setup();
    int a = add_unit(SIDE, U_DRAGOON, 100, 100);
    int t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    int far = add_unit(ENEMY, U_ZERGLING, 900, 900);
    for (int i = 0; i < RACE_IMPACT_CAPACITY; ++i)
        race_impact_launch(far, 1, 1, ENEMY, 0, U_MARINE, i == 0 ? 2u : 200u);
    assert(pending() == RACE_IMPACT_CAPACITY);
    int hp = u[t].hp, tracer_count = live_tracers();
    tick_combat(&u[a], t);
    assert(u[a].cd == 0 && live_tracers() == tracer_count && race_impact_deferred == 1);
    assert(pending() == RACE_IMPACT_CAPACITY && u[t].hp == hp);
    update_tracers();
    tick_combat(&u[a], t);
    assert(u[a].cd == 0 && race_impact_deferred == 2);
    update_tracers(); /* the two-step row resolves */
    assert(pending() == RACE_IMPACT_CAPACITY - 1);
    tick_combat(&u[a], t);
    assert(u[a].cd > 0 && pending() == RACE_IMPACT_CAPACITY && u[t].hp == hp);

    /* Towers follow the same refusal. */
    int cannon = spawn_build(ENEMY, B_PHOTON_CANNON, 140, 140, 0);
    assert(cannon >= 0);
    int victim = add_unit(SIDE, U_ZEALOT, 180, 140);
    assert(race_impact_building_steps(&b[cannon], victim, 0) > 1);
    assert(!hit_stock_building_target(&b[cannon], victim, 20));
}

static void test_tower_projectiles_wait(void)
{
    setup();
    int cannon = spawn_build(ENEMY, B_PHOTON_CANNON, 140, 140, 0);
    int t = add_unit(SIDE, U_ZEALOT, 190, 140);
    assert(cannon >= 0);
    int hp = u[t].hp, shield = u[t].shield;
    unsigned steps = race_impact_building_steps(&b[cannon], t, 0);
    assert(steps > 1 && hit_stock_building_target(&b[cannon], t, 20));
    assert(pending() == 1 && u[t].hp == hp && u[t].shield == shield);
    for (unsigned s = 1; s < steps; ++s) update_tracers();
    assert(u[t].shield == shield);
    update_tracers();
    assert(u[t].shield < shield && pending() == 0);
}
#endif

/* Melee and appear-at-target weapons keep their immediate path. */
static void test_immediate_weapons(void)
{
    const int attackers[] = { U_MARINE, U_ZEALOT, U_HYDRALISK, U_ZERGLING };
    for (unsigned i = 0; i < sizeof(attackers) / sizeof(attackers[0]); ++i) {
        setup();
        int a = add_unit(SIDE, attackers[i], 100, 100);
        int t = add_unit(ENEMY, U_ZERGLING, 104, 100);
        int hp = u[t].hp;
        tick_combat(&u[a], t);
        assert(u[a].cd > 0 && u[t].hp < hp && observed_count >= 1);
#ifdef SC64_IMPACT_EVENTS
        assert(pending() == 0);
#endif
    }
#ifndef SC64_IMPACT_EVENTS
    /* Without source projectiles, a Dragoon still deals damage on launch. */
    setup();
    int a = add_unit(SIDE, U_DRAGOON, 100, 100);
    int t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    int hp = u[t].hp;
    tick_combat(&u[a], t);
    assert(u[a].cd > 0 && u[t].hp < hp);
#endif
}

#ifdef SC64_IMPACT_EVENTS
static RaceImpactRecord record_copy;

static u32 latest_record(void)
{
    u32 len = 0;
    assert(save_read_latest(&record_copy, sizeof(record_copy), SAVE_VERSION, &len));
    return len;
}

/* Saves carry pending impacts; a resumed game resolves them on the same
 * update as the uninterrupted run. */
static void test_save_load(void)
{
    setup();
    assert(save_game());
    assert(latest_record() == sizeof(RaceSave)); /* empty queue: historical length */

    int a = add_unit(SIDE, U_DRAGOON, 100, 100);
    int t = add_unit(ENEMY, U_ZERGLING, 156, 100);
    int a2 = add_unit(SIDE, U_DRAGOON, 100, 140);
    int t2 = add_unit(ENEMY, U_ZERGLING, 150, 150);
    unsigned steps = launch_dragoon(a, t);
    update_tracers();
    (void)launch_dragoon(a2, t2);
    assert(save_game());
    assert(latest_record() == sizeof(RaceSave) + sc64_impact_save_bytes(2));
    Sc64ImpactEvent saved_rows[2];
    memcpy(saved_rows, race_impact_rows, sizeof(saved_rows));

    /* Reference continuation. */
    int ref_hp[64], ref_count[64];
    for (unsigned s = 0; s < steps + 2; ++s) {
        update_tracers();
        ref_hp[s] = u[t].hp * 1000 + u[t2].hp;
        ref_count[s] = pending();
    }
    assert(ref_count[steps + 1] == 0);

    /* Diverge, then resume from SRAM. */
    u[t].hp = 3; u[t2].x += 40;
    assert(load_game());
    assert(pending() == 2 && !memcmp(race_impact_rows, saved_rows, sizeof(saved_rows)));
    for (unsigned s = 0; s < steps + 2; ++s) {
        update_tracers();
        assert(u[t].hp * 1000 + u[t2].hp == ref_hp[s] && (int)pending() == ref_count[s]);
    }

    /* A malformed impact block rejects the record without touching live state. */
    assert(load_game() && pending() == 2);
    u32 len = latest_record();
    record_copy.impacts[0] ^= 1;
    save_write(&record_copy, len, SAVE_VERSION);
    update_tracers();
    Sc64ImpactEvent live[RACE_IMPACT_CAPACITY];
    unsigned live_count = pending();
    memcpy(live, race_impact_rows, sizeof(live));
    assert(!load_game());
    assert(pending() == live_count && !memcmp(live, race_impact_rows, sizeof(live)));

    /* The newest record wins regardless of length, in both directions. */
    record_copy.impacts[0] ^= 1;
    save_write(&record_copy, sizeof(RaceSave), SAVE_VERSION);
    assert(load_game() && pending() == 0);
    save_write(&record_copy, len, SAVE_VERSION);
    assert(load_game() && pending() == 2);
}

/* A profile stores at most RACE_IMPACT_SAVE_CAPACITY rows. Beyond that the
 * save is refused before SRAM changes; where the whole queue fits, a full
 * queue round-trips. */
static void test_save_capacity(void)
{
    static RaceImpactRecord before;
    setup();
    int far = add_unit(ENEMY, U_ZERGLING, 600, 600);
    assert(save_game());
    u32 before_len = latest_record();
    memcpy(&before, &record_copy, sizeof(before));
    int rows = RACE_IMPACT_SAVE_CAPACITY < RACE_IMPACT_CAPACITY ?
               RACE_IMPACT_SAVE_CAPACITY + 1 : RACE_IMPACT_CAPACITY;
    for (int i = 0; i < rows; ++i)
        race_impact_launch(far, 1, 1, SIDE, 0, U_DRAGOON, i == 0 ? 1u : 5u + (unsigned)i);
    if (RACE_IMPACT_SAVE_CAPACITY < RACE_IMPACT_CAPACITY) {
        assert(!save_game());
        assert(latest_record() == before_len && !memcmp(&before, &record_copy, before_len));
        update_tracers(); /* the first impact lands */
        assert((int)pending() == RACE_IMPACT_SAVE_CAPACITY);
    }
    unsigned count = pending();
    Sc64ImpactEvent rows_before[RACE_IMPACT_CAPACITY];
    memcpy(rows_before, race_impact_rows, sizeof(rows_before));
    assert(save_game());
    assert(latest_record() == sizeof(RaceSave) + sc64_impact_save_bytes(count));
    race_impact_reset();
    assert(load_game() && pending() == count);
    assert(!memcmp(rows_before, race_impact_rows, count * sizeof(rows_before[0])));
}
#endif

int main(void)
{
#ifdef SC64_IMPACT_EVENTS
    test_impact_matches_presentation();
    test_moving_target_still_hit();
    test_dead_target_and_slot_reuse();
    test_attacker_death_and_ownership();
    test_simultaneous_impacts();
    test_shields_invincibility_and_cleanup();
    test_overflow_defers_launch();
    test_tower_projectiles_wait();
    test_save_load();
    test_save_capacity();
    printf("Launch-to-impact damage (queue %d, save %d): PASS\n",
           RACE_IMPACT_CAPACITY, RACE_IMPACT_SAVE_CAPACITY);
#else
    puts("Immediate damage without source projectiles: PASS");
#endif
    test_immediate_weapons();
    return 0;
}
