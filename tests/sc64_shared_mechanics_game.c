/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual-engine parity checks for mechanics shared across game modes.
 * Feature flags come from the production compile vector; this fixture must
 * not locally enable construction HP or Siege splash.
 */
#include <assert.h>
#include "../starcraft/game.c"

#if !defined(SC64_RACES) || !defined(SC64_CONSTRUCTION_HP) || \
    !defined(SC64_SIEGE_SPLASH) || !defined(SC64_ENGINE_CORE_REQUIRED)
#error "Compile with the required shared-engine production flags"
#endif

static void reset_world(int race_mode_value, int race)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(tracers, 0, sizeof(tracers));
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
#endif
    race_mode = race_mode_value;
    race_human = race;
    race_selected_mission = 0; /* Campaign first mission; shared-mechanic fixture only. */
    race_player[SIDE] = race;
    race_player[ENEMY] = R_TER;
    race_player[RESCUABLE] = R_ZERG;
    race_player[4] = R_PROTOSS;
    screen = SCREEN_PLAY;
    minerals = gas = supply_used = supply_cap = 0;
    weapons_level = armor_level = 0;
}

static int worker_for_race(int race)
{
    return race == R_TER ? U_SCV : race == R_PROTOSS ? U_PROBE : U_DRONE;
}

static int foundation_for_race(int race)
{
    return race == R_TER ? B_BARRACKS :
           race == R_PROTOSS ? B_GATEWAY : B_CREEP_COLONY;
}

static void test_construction_shared_in_both_modes(int mode, int race)
{
    const int x = 320, y = 320;
    reset_world(mode, race);
    int type = foundation_for_race(race);
    int worker = spawn_unit(SIDE, worker_for_race(race), x, y);
    int building = spawn_build(SIDE, type, x, y, 1);
    assert(worker >= 0 && building >= 0);
    assert(b[building].under_construction);
    assert(b[building].hp == b[building].maxhp / 10);
    int max_shields = race == R_PROTOSS ? sc64_race_building_shields(type) : 0;
    if (race == R_PROTOSS) assert(b[building].shield == max_shields / 10);

    /* Use the production builder-arrival/order path, then the normal building
       construction clock for Zerg and Protoss. Terran advances in ORDER_BUILD. */
    u[worker].order = ORDER_BUILD;
    u[worker].target = building;
    update_orders();
    if (race == R_ZERG) assert(!u[worker].alive);
    else if (race == R_PROTOSS) assert(u[worker].alive && u[worker].order == ORDER_IDLE);
    else assert(u[worker].alive && u[worker].order == ORDER_BUILD);

    int total = building_build_time(type);
    assert(total > 4);
    for (int step = 0; step < 3; ++step) {
        if (race == R_TER) update_orders();
        else update_production();
    }
    assert(b[building].build_progress >= 3);
    int damage = 19;
    b[building].hp -= damage;
    assert(b[building].hp > 0);
    if (race == R_PROTOSS) b[building].shield -= 7;

    for (int tick = 0; b[building].under_construction && tick < total + 4; ++tick) {
        if (race == R_TER) update_orders();
        else update_production();
    }
    assert(!b[building].under_construction);
    assert(b[building].build_progress == 0);
    assert(b[building].hp == b[building].maxhp - damage);

    if (race == R_PROTOSS) {
        assert(max_shields > 0);
        assert(b[building].shield == max_shields - 7);
    }
}

static int add_raw_unit(int owner, int raw, int x, int y, int hp)
{
    int type = sc64_race_unit_from_raw(raw);
    assert(type >= 0);
    int slot = spawn_unit(owner, type, x, y);
    assert(slot >= 0);
    u[slot].raw_type = (s16)raw;
    u[slot].hp = u[slot].maxhp = hp;
    u[slot].armor = 0;
    u[slot].shield = 0;
    return slot;
}

static void test_siege_splash_shared_in_both_modes(int mode)
{
    reset_world(mode, R_TER);
    int tank = add_raw_unit(SIDE, 30, 100, 100, 150);
    int primary = add_raw_unit(ENEMY, 0, 200, 100, 100);
    int friendly = add_raw_unit(SIDE, 0, 214, 100, 100);
    int middle = add_raw_unit(ENEMY, 0, 216, 100, 100);
    int flying = add_raw_unit(ENEMY, 8, 214, 100, 120);
    int immune = add_raw_unit(ENEMY, 0, 214, 100, 100);
    u[immune].flags |= 16u;
    tick_combat(&u[tank], primary);
    /* Weapon 27's 70-point full packet is reduced by Small-size armor rules:
       35 to the primary and 17 to each middle-ring Small ground unit. */
    assert(u[primary].hp == 65);
    assert(u[friendly].hp == 83);
    assert(u[middle].hp == 83);
    assert(u[flying].hp == 120);
    assert(u[immune].hp == 100);
}

static RaceSave saved_candidate;
static Unit saved_world_units[MAX_UNITS];
static Building saved_world_buildings[MAX_BUILDINGS];
static u8 saved_world_explored[SC64_GRID_STORAGE_BYTES];

static void expect_candidate_rejected(void)
{
    /* Make scalar and fog state differ from the candidate so a partial load
       cannot pass merely because both worlds started with the same values. */
    ++frame;
    cx = (cx + 1) % (race_cols * 16);
    SC64_GRID_SET(explored, 0, !SC64_GRID_GET(explored, 0));
    memcpy(saved_world_units, u, sizeof(u));
    memcpy(saved_world_buildings, b, sizeof(b));
    memcpy(saved_world_explored, explored, sizeof(explored));
    int old_minerals = minerals, old_gas = gas;
    int old_frame = frame, old_cx = cx, old_cy = cy;
    int old_mode = race_mode, old_human = race_human;
    int old_mission = race_selected_mission, old_completed = race_completed;
    save_write(&saved_candidate, sizeof(saved_candidate), SAVE_VERSION);
    assert(!race_load_game());
    assert(memcmp(saved_world_units, u, sizeof(u)) == 0);
    assert(memcmp(saved_world_buildings, b, sizeof(b)) == 0);
    assert(memcmp(saved_world_explored, explored, sizeof(explored)) == 0);
    assert(minerals == old_minerals && gas == old_gas);
    assert(frame == old_frame && cx == old_cx && cy == old_cy);
    assert(race_mode == old_mode && race_human == old_human &&
           race_selected_mission == old_mission && race_completed == old_completed);
}

static void test_construction_save_compatibility(void)
{
    game_init();
    race_mode = 1; race_human = R_PROTOSS; race_bots = 1;
    race_skirmish_map = 0;
    race_skirmish_init();
    screen = SCREEN_PLAY;

    /* Historical completed worlds remain usable and are marked on resave. */
    assert(race_save_game());
    assert(race_data.state[SC64_CONSTRUCTION_SAVE_MARKER_INDEX] ==
           SC64_CONSTRUCTION_SAVE_MARKER);
    saved_candidate = race_data;
    saved_candidate.state[SC64_CONSTRUCTION_SAVE_MARKER_INDEX] = 0;
    save_write(&saved_candidate, sizeof(saved_candidate), SAVE_VERSION);
    assert(race_load_game());
    assert(race_save_game());
    assert(race_data.state[SC64_CONSTRUCTION_SAVE_MARKER_INDEX] ==
           SC64_CONSTRUCTION_SAVE_MARKER);

    int building = spawn_build(SIDE, B_GATEWAY, 320, 320, 1);
    assert(building >= 0);
    for (int i = 0; i < 4; ++i) update_production();
    b[building].hp -= 10;
    b[building].shield -= 5;
    int hp = b[building].hp, shield = b[building].shield;
    int progress = b[building].build_progress;
    assert(hp > 0 && shield > 0 && progress > 0);
    assert(race_save_game());
    saved_candidate = race_data;
    b[building].hp = 1; b[building].shield = 0;
    assert(race_load_game());
    assert(b[building].hp == hp && b[building].shield == shield &&
           b[building].build_progress == progress);
    for (int i = 0; b[building].under_construction && i < 2000; ++i)
        update_production();
    assert(!b[building].under_construction);
    assert(b[building].hp == b[building].maxhp - 10);
    assert(b[building].shield == sc64_race_building_shields(B_GATEWAY) - 5);

    saved_candidate.state[SC64_CONSTRUCTION_SAVE_MARKER_INDEX] = 0;
    expect_candidate_rejected();
    saved_candidate.state[SC64_CONSTRUCTION_SAVE_MARKER_INDEX] = 12345;
    expect_candidate_rejected();

    /* Legacy in-place morphs also had different completion semantics. */
    b[building].alive = 0;
    building = spawn_build(SIDE, B_LAIR, 320, 320, 1);
    assert(building >= 0 && race_save_game());
    saved_candidate = race_data;
    saved_candidate.state[SC64_CONSTRUCTION_SAVE_MARKER_INDEX] = 0;
    expect_candidate_rejected();
}

int main(void)
{
    for (int mode = 0; mode <= 1; ++mode) {
        test_construction_shared_in_both_modes(mode, R_TER);
        test_construction_shared_in_both_modes(mode, R_PROTOSS);
        test_construction_shared_in_both_modes(mode, R_ZERG);
        test_siege_splash_shared_in_both_modes(mode);
    }
    test_construction_save_compatibility();
    return 0;
}
