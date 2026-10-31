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

static void test_burn_shared_in_both_modes(int mode, int race)
{
    for (int owner = 0; owner <= 4; ++owner) {
        reset_world(mode, race);
        int slot = spawn_build(owner, B_BARRACKS, 320, 320, 0);
        assert(slot >= 0 && race_building_can_burn(&b[slot]));
        assert(sc64_building_burn_owner((unsigned)b[slot].shield_remainder) == NEUTRAL);
        b[slot].maxhp = 100;
        b[slot].hp = 33;
        frame = 1;
        race_tick_state();
        assert(sc64_building_burn_hp_q8((unsigned)b[slot].hp,
               (unsigned)b[slot].shield_remainder) == 33u * 256u - 20u);
        /* The lifted state is irrelevant to burn. */
        b[slot].flags |= 4;
        frame = 2;
        race_tick_state();
        assert(sc64_building_burn_hp_q8((unsigned)b[slot].hp,
               (unsigned)b[slot].shield_remainder) == 33u * 256u - 40u);
        b[slot].under_construction = 1;
        frame = 3;
        race_tick_state();
        assert(sc64_building_burn_hp_q8((unsigned)b[slot].hp,
               (unsigned)b[slot].shield_remainder) == 33u * 256u - 40u);
        b[slot].under_construction = 0;
        b[slot].hp = 34;
        b[slot].shield_remainder = NEUTRAL * 256;
        frame = 4;
        race_tick_state();
        assert(b[slot].hp == 34 && b[slot].shield_remainder == NEUTRAL * 256);
        b[slot].hp = 33;
        for (frame = 1; frame <= 100; ++frame) race_tick_state();
        assert(sc64_building_burn_hp_q8((unsigned)b[slot].hp,
               (unsigned)b[slot].shield_remainder) == 33u * 256u - 80u * 20u);
    }
    reset_world(mode, race);
    int p = spawn_build(SIDE, B_GATEWAY, 320, 320, 0);
    int z = spawn_build(SIDE, B_CREEP_COLONY, 400, 320, 0);
    assert(p >= 0 && z >= 0);
    assert(!race_building_can_burn(&b[p]) && !race_building_can_burn(&b[z]));
    b[p].hp = b[z].hp = 1;
    frame = 1;
    race_tick_state();
    assert(b[p].hp == 1 && b[z].hp == 1);

    reset_world(mode, race);
    int depot = spawn_build(SIDE, B_DEPOT, 320, 320, 0);
    assert(depot >= 0);
    b[depot].hp = 1;
    b[depot].shield_remainder = (int)sc64_building_burn_with_owner(236, ENEMY);
    path_grid_valid = 1;
    supply_cap = 16;
    frame = 1;
    race_tick_state();
    assert(!b[depot].alive && !path_grid_valid && supply_cap == 0);
}

static void test_invincibility_protects_shields(int mode)
{
    reset_world(mode, R_PROTOSS);
    int unit = spawn_unit(SIDE, U_PROBE, 100, 100);
    int building = spawn_build(SIDE, B_GATEWAY, 320, 320, 0);
    assert(unit >= 0 && building >= 0);
    u[unit].shield = b[building].shield = 50;
    int uhp = u[unit].hp, bhp = b[building].hp;
    u[unit].flags |= 16;
    b[building].flags |= 16;
    hit_target(unit, 20, U_MARINE, ENEMY);
    hit_target(MAX_UNITS + building, 20, U_MARINE, ENEMY);
    assert(u[unit].shield == 50 && b[building].shield == 50);
    assert(u[unit].hp == uhp && b[building].hp == bhp);
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    u[unit].flags &= ~16;
    b[building].flags &= ~16;
    invincible_units[unit] = invincible_buildings[building] = 1;
    hit_target(unit, 20, U_MARINE, ENEMY);
    hit_target(MAX_UNITS + building, 20, U_MARINE, ENEMY);
    assert(u[unit].shield == 50 && b[building].shield == 50);
    assert(u[unit].hp == uhp && b[building].hp == bhp);
#endif
}

#ifdef SC64_T6_RUNTIME
static void test_burn_fraction_repair(int late)
{
#if !defined(SC64_TERRAN_LATE_RUNTIME)
    if (late) return;
#endif
    reset_world(0, R_TER);
    race_selected_mission = late ? 10 : SC64_T6_GLOBAL_ID;
    int slot = spawn_build(SIDE, B_BARRACKS, 320, 320, 0);
    int worker = spawn_unit(SIDE, U_SCV, 320, 320);
    assert(slot >= 0 && worker >= 0);
    b[slot].raw_type = sc64_race_raw_building(B_BARRACKS);
    assert(race_building_can_burn(&b[slot]));
    b[slot].hp = b[slot].maxhp;
    b[slot].shield_remainder = ENEMY * 256 + 20;
    u[worker].order = ORDER_REPAIR;
    u[worker].target = MAX_UNITS + slot;
    frame = 1;
    minerals = gas = 0;
    /* An unpaid step cannot erase the deficit. */
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (late) race_late_repair_tick_unit(worker); else
#endif
    race_t6_repair_tick_unit(worker);
    assert(b[slot].hp == b[slot].maxhp &&
           b[slot].shield_remainder == ENEMY * 256 + 20);
    minerals = gas = 10000;
    u[worker].order = ORDER_REPAIR;
    u[worker].target = MAX_UNITS + slot;
    for (int i = 0; i < 2000 && (b[slot].shield_remainder & 255); ++i) {
        frame = i + 1;
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (late) race_late_repair_tick_unit(worker); else
#endif
        race_t6_repair_tick_unit(worker);
    }
    assert(b[slot].hp == b[slot].maxhp && b[slot].shield_remainder == ENEMY * 256);
    /* A repair to max from a lower displayed HP must also discard the deficit. */
    b[slot].hp = b[slot].maxhp - 1;
    b[slot].shield_remainder = 4 * 256 + 100;
    u[worker].order = ORDER_REPAIR;
    u[worker].target = MAX_UNITS + slot;
    u[worker].carry = u[worker].work_timer = 0;
    for (int i = 0; i < 2000 && b[slot].hp < b[slot].maxhp; ++i) {
        frame = i + 1;
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (late) race_late_repair_tick_unit(worker); else
#endif
        race_t6_repair_tick_unit(worker);
    }
    assert(b[slot].hp == b[slot].maxhp && b[slot].shield_remainder == 4 * 256);
}
#endif

static RaceSave saved_candidate;
static Unit saved_world_units[MAX_UNITS];
static Building saved_world_buildings[MAX_BUILDINGS];
static u8 saved_world_explored[SC64_GRID_STORAGE_BYTES];

/* Match the active production journal format when injecting a candidate.
 * Yamato + delayed impacts use a compact envelope, not a raw RaceSave tail. */
static void write_candidate_record(void)
{
#if defined(SC64_IMPACT_EVENTS) && defined(SC64_BATTLECRUISER_YAMATO_EXPERIMENT) && !defined(SC64_SCARAB_NAMESPACE_STAGE)
    race_data = saved_candidate;
    assert(race_impact_save_write());
#else
    save_write(&saved_candidate, sizeof(saved_candidate), SAVE_VERSION);
#endif
}

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
    write_candidate_record();
    assert(!race_load_game());
    assert(memcmp(saved_world_units, u, sizeof(u)) == 0);
    assert(memcmp(saved_world_buildings, b, sizeof(b)) == 0);
    assert(memcmp(saved_world_explored, explored, sizeof(explored)) == 0);
    assert(minerals == old_minerals && gas == old_gas);
    assert(frame == old_frame && cx == old_cx && cy == old_cy);
    assert(race_mode == old_mode && race_human == old_human &&
           race_selected_mission == old_mission && race_completed == old_completed);
}

static void test_burn_save_compatibility(void)
{
    game_init();
    race_mode = 1; race_human = R_TER; race_bots = 1;
    race_skirmish_map = 0;
    race_skirmish_init();
    screen = SCREEN_PLAY;
    int building = spawn_build(SIDE, B_BARRACKS, 320, 320, 0);
    assert(building >= 0 && race_building_can_burn(&b[building]));
    b[building].hp = 200;
    b[building].shield_remainder = (int)sc64_building_burn_with_owner(97, 4);
    assert(race_save_game());
    assert(race_data.state[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX] ==
           SC64_BUILDING_DAMAGE_SAVE_MARKER);
    saved_candidate = race_data;
    b[building].hp = 1; b[building].shield_remainder = 0;
    assert(race_load_game());
    assert(b[building].hp == 200 && b[building].shield_remainder == 4 * 256 + 97);
    saved_candidate.state[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX] = 0;
    write_candidate_record();
    assert(race_load_game());
    assert(b[building].hp == 200 && b[building].shield_remainder == NEUTRAL * 256);
    assert(race_save_game());
    assert(race_data.state[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX] ==
           SC64_BUILDING_DAMAGE_SAVE_MARKER);
    saved_candidate = race_data;
    saved_candidate.state[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX] = 1;
    expect_candidate_rejected();
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
    write_candidate_record();
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

#ifdef SC64_SAVE_UI_CONTEXT
static void test_ui_save_context(int human)
{
    game_init();
    race_mode=1; race_human=human; race_bots=1; race_skirmish_map=0;
    race_skirmish_init(); screen=SCREEN_PLAY;
    int building=spawn_build(SIDE, foundation_for_race(human),320,320,0);
    assert(building>=0); selected_building=building; menu_cursor=3;
#ifdef SC_FIELD_FEEDBACK
    menu_last=2;
#endif
    assert(race_save_game()); saved_candidate=race_data;
    selected_building=-1; menu_cursor=0;
#ifdef SC_FIELD_FEEDBACK
    menu_last=0;
#endif
    assert(race_load_game()); assert(selected_building==building && menu_cursor==3);
#ifdef SC_FIELD_FEEDBACK
    assert(menu_last==2);
#endif
    /* The candidate validator accepts a legacy zero and a now-dead but
       in-range building; malformed extensions fail before commit. */
    int absent=-1; for(int i=0;i<MAX_BUILDINGS;i++) if(!b[i].alive) { absent=i; break; }
    assert(absent>=0);
    assert(sc64_save_ui_valid(0,MAX_BUILDINGS));
    unsigned stale=sc64_save_ui_encode(absent,MAX_BUILDINGS,3,2);
    assert(sc64_save_ui_valid(stale,MAX_BUILDINGS));
    const unsigned invalid[] = {0x12345678u,SC64_SAVE_UI_TAG | (MAX_BUILDINGS+1u),
                                SC64_SAVE_UI_TAG | (64u<<8),SC64_SAVE_UI_TAG | (64u<<16)};
    for (unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i) {
        assert(!sc64_save_ui_valid(invalid[i],MAX_BUILDINGS));
        race_data=saved_candidate;
        race_data.state[SC64_SAVE_UI_STATE_INDEX]=(int)invalid[i];
        RaceLoadPlan plan={0};
        assert(!race_load_preflight_candidate(&plan));
    }
    /* Saving a stale selection succeeds and emits none. */
    selected_building=absent; menu_cursor=3;
    assert(race_save_game()); assert(race_load_game());
    assert(selected_building==-1);
    selected_building=MAX_BUILDINGS+5; menu_cursor=300;
#ifdef SC_FIELD_FEEDBACK
    menu_last=255;
#endif
    assert(race_save_game());
    assert((unsigned)race_data.state[SC64_SAVE_UI_STATE_INDEX] == SC64_SAVE_UI_TAG);
}
#endif

int main(void)
{
#ifdef SC64_SAVE_UI_FIXTURE_ONLY
    for (int race=R_TER;race<=R_PROTOSS;++race) test_ui_save_context(race);
    return 0;
#endif
    for (int mode = 0; mode <= 1; ++mode) {
        test_construction_shared_in_both_modes(mode, R_TER);
        test_construction_shared_in_both_modes(mode, R_PROTOSS);
        test_construction_shared_in_both_modes(mode, R_ZERG);
        test_siege_splash_shared_in_both_modes(mode);
        for (int race = R_TER; race <= R_PROTOSS; ++race)
            test_burn_shared_in_both_modes(mode, race);
        test_invincibility_protects_shields(mode);
    }
    test_construction_save_compatibility();
    test_burn_save_compatibility();
#ifdef SC64_SAVE_UI_CONTEXT
    for (int race=R_TER;race<=R_PROTOSS;++race) test_ui_save_context(race);
#endif
#ifdef SC64_T6_RUNTIME
    test_burn_fraction_repair(0);
    test_burn_fraction_repair(1);
#endif
    return 0;
}
