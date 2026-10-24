/* Z7 ordinary colony construction and Creep -> Sunken morph fixture. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z7_colony_adapter.h"

#ifndef SC64_Z7_RUNTIME
#error "Compile with the authenticated Z7 runtime profile"
#endif
#ifndef SC64_Z7_COLONY_DEFENSE_ENABLED
#error "Compile with the explicit Z7 colony defense feature gate"
#endif

static unsigned held_keys;

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}

static void press(unsigned key)
{
    input(key);
    input(0);
}

static void open_command_row(int row)
{
    int guard = 0;
    assert(row >= 0 && row < field_menu_count());
    press(KEY_B);
    assert(menu_open && menu_cursor == 0);
    while (menu_cursor != row && guard++ < field_menu_count())
        press(KEY_DOWN);
    assert(menu_cursor == row);
}

static void launch_z7_normally(void)
{
    game_init();
    held_keys = 0;
    assert(race_front.page == SC_FRONT_HOME);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION &&
           race_front.faction == SC_FRONT_ZERG);
    int ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,
                                                 SC64_Z7_GLOBAL_ID);
    assert(ordinal >= 0);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING &&
           race_front.mission == SC64_Z7_GLOBAL_ID);
    press(KEY_A);
    assert(screen == SCREEN_PLAY && race_z7_active() &&
           race_z7_map == &sc64_race_maps[SC64_Z7_GLOBAL_ID]);
    for (int i = 0; i < 4; ++i) game_update(0, 0);
    assert(race_minerals[SIDE] >= 800 && race_gas[SIDE] >= 200);
}

static void reset_world(void)
{
    const Sc64RaceMap *map = &sc64_race_maps[SC64_Z7_GLOBAL_ID];
    game_init();
    race_mode = 0;
    race_human = R_ZERG;
    race_selected_mission = SC64_Z7_GLOBAL_ID;
    race_cols = map->width;
    race_rows = map->height;
    race_player[SIDE] = R_ZERG;
    race_player[ENEMY] = R_ZERG;
    race_player[NEUTRAL] = R_ZERG;
    race_z7_map = map;
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(geysers, 0, sizeof(geysers));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    race_minerals[SIDE] = race_gas[SIDE] = 0;
    minerals = 1000;
    gas = 0;
    supply_used = 0;
    supply_cap = 0;
    geyser_count = 0;
}

static int find_legal_site(int type, int near_x, int near_y)
{
    int best = -1, best_d = 0x7fffffff;
    for (int y = 32; y < WORLD_H - 32; y += 8)
        for (int x = 32; x < WORLD_W - 32; x += 8) {
            int d = dist2(x, y, near_x, near_y);
            if (d >= best_d || !race_can_site(SIDE, type, x, y) ||
                !can_place(type, x, y)) continue;
            best = x + y * WORLD_W;
            best_d = d;
        }
    return best;
}

static int find_any_legal_site(int type, int near_x, int near_y)
{
    int best = -1, best_d = 0x7fffffff;
    for (int y = 32; y < WORLD_H - 32; y += 8)
        for (int x = 32; x < WORLD_W - 32; x += 8) {
            int d = dist2(x, y, near_x, near_y);
            if (d >= best_d || !can_place(type, x, y)) continue;
            best = x + y * WORLD_W;
            best_d = d;
        }
    return best;
}

static int create_colony_through_normal_build(void)
{
    int hatch, pool, site, worker, colony;
    int hatch_x = -1, hatch_y = -1;
    for (int y = 48; y < WORLD_H - 48 && hatch_x < 0; y += 8)
        for (int x = 48; x < WORLD_W - 48; x += 8)
            if (can_place(B_HATCHERY, x, y)) {
                hatch_x = x; hatch_y = y; break;
            }
    assert(hatch_x >= 0);
    hatch = spawn_build(SIDE, B_HATCHERY, hatch_x, hatch_y, 0);
    assert(hatch >= 0);
    pool = find_legal_site(B_SPAWNING_POOL, hatch_x + 64, hatch_y);
    assert(pool >= 0);
    pool = spawn_build(SIDE, B_SPAWNING_POOL,
                       pool % WORLD_W, pool / WORLD_W, 0);
    assert(pool >= 0);

    site = find_legal_site(B_CREEP_COLONY, hatch_x + 80, hatch_y + 48);
    assert(site >= 0);
    int x = site % WORLD_W, y = site / WORLD_W;
    worker = spawn_unit(SIDE, U_DRONE, x + 32, y);
    assert(worker >= 0);
    assert(race_start_build(SIDE, worker, B_CREEP_COLONY, x, y));
    assert(minerals == 925 && u[worker].order == ORDER_BUILD);
    colony = u[worker].target;
    assert(colony >= 0 && colony < MAX_BUILDINGS &&
           b[colony].alive && b[colony].type == B_CREEP_COLONY &&
           b[colony].raw_type == SC64_Z7_CREEP_RAW &&
           b[colony].raw_owner == 3 && b[colony].under_construction);

    /* Model the normal worker-arrived callback after its path reaches the
       selected site; construction then uses the production frame ticker. */
    u[worker].x = x + 32;
    u[worker].y = y;
    assert(race_builder_arrived(worker));
    assert(!u[worker].alive); /* Creep Colony construction consumes Drone. */
    for (int t = 0; t < (300 * 5 + 3) / 4; ++t) race_tick_buildings();
    assert(building_complete(colony));
    assert(b[colony].type == B_CREEP_COLONY && b[colony].hp == 400 &&
           b[colony].maxhp == 400 && minerals == 925);
    return colony;
}

static void test_paid_in_place_morph_and_roundtrip(int damage_mode)
{
    RaceSave checkpoint;
    Building decoded, forged;
    int colony = create_colony_through_normal_build();
    int old_hp = damage_mode == 1 ? 200 : 400;
    assert(race_selected_mission == SC64_Z7_GLOBAL_ID);
    b[colony].hp = old_hp;
    assert(race_z7_colony_morph_available(colony));

    /* Failed eligibility and affordability checks are nonmutating. */
    int before = minerals;
    assert(!race_z7_colony_start_sunken(-1));
    assert(minerals == before && b[colony].type == B_CREEP_COLONY);
    minerals = 49;
    assert(!race_z7_colony_start_sunken(colony));
    assert(minerals == 49 && b[colony].type == B_CREEP_COLONY);
    minerals = before;

    assert(race_z7_colony_start_sunken(colony));
    assert(minerals == before - 50 && gas == 0);
    assert(b[colony].alive && b[colony].type == B_CREEP_COLONY &&
           b[colony].raw_type == SC64_Z7_CREEP_RAW && b[colony].raw_owner == 3 &&
           b[colony].hp == old_hp && b[colony].maxhp == 400 &&
           b[colony].under_construction && b[colony].build_progress == 0 &&
           b[colony].queue_progress == old_hp && b[colony].shield == 0 &&
           (b[colony].flags & SC64_Z7_SUNKEN_MORPH_FLAG));
    assert(sc64_race_building_armor(b[colony].type) == 0);
    hit_target(MAX_UNITS + colony, 10, U_MARINE, ENEMY);
    assert(b[colony].hp == old_hp - 10 && b[colony].shield == 0);
    assert(race_z7_colony_morph_state_valid(&b[colony]));

    /* Persist a mid-morph building via the production packed save codec. */
    memset(&checkpoint, 0, sizeof(checkpoint));
    for (int t = 0; t < 75; ++t) {
        assert(race_z7_colony_pre_building_tick());
        race_tick_buildings();
        assert(race_z7_colony_post_building_tick());
    }
    assert(b[colony].under_construction && b[colony].build_progress == 75);
    assert(sc64_building_save_encode(&checkpoint.buildings[colony], &b[colony],
                                     SC64_Z7_MAP_COLS * 16u,
                                     SC64_Z7_MAP_ROWS * 16u));
    assert(race_save_decode_building(&checkpoint, (unsigned)colony, &decoded));
    assert(race_z7_colony_morph_state_valid(&decoded));
    assert(decoded.type == B_CREEP_COLONY && decoded.raw_type == 143 &&
           decoded.raw_owner == 3 && decoded.build_progress == 75 &&
           decoded.maxhp == 400 && decoded.shield == 0 &&
           decoded.queue_progress == old_hp - 10 &&
           (decoded.flags & SC64_Z7_SUNKEN_MORPH_FLAG));
    forged = decoded;
    forged.raw_owner = 4;
    assert(!race_z7_colony_morph_state_valid(&forged));
    forged = decoded;
    forged.build_progress = (300 * 5 + 3) / 4;
    assert(!race_z7_colony_morph_state_valid(&forged));
    forged = decoded;
    forged.type = B_SUNKEN_COLONY;
    forged.raw_type = SC64_Z7_SUNKEN_RAW;
    assert(!race_z7_colony_morph_state_valid(&forged));
    forged = decoded;
    forged.queue_progress = 0;
    assert(!race_z7_colony_morph_state_valid(&forged));
    forged = decoded;
    forged.queue_progress = SC64_Z7_CREEP_HP + 1;
    assert(!race_z7_colony_morph_state_valid(&forged));
    forged = decoded;
    forged.owner = ENEMY;
    assert(!race_z7_colony_morph_state_valid(&forged));
    forged = decoded;
    forged.maxhp = SC64_Z7_SUNKEN_HP;
    assert(!race_z7_colony_morph_state_valid(&forged));
    forged = decoded;
    forged.shield_remainder = 1;
    assert(!race_z7_colony_morph_state_valid(&forged));
    b[colony] = decoded; /* exact candidate roundtrip as on accepted load */

    for (int t = 75; t < (300 * 5 + 3) / 4; ++t) {
        assert(race_z7_colony_pre_building_tick());
        race_tick_buildings();
        assert(race_z7_colony_post_building_tick());
        if (damage_mode == 2 && t == 150)
            hit_target(MAX_UNITS + colony, 10, U_MARINE, ENEMY);
    }
    assert(building_complete(colony));
    assert(b[colony].type == B_SUNKEN_COLONY && b[colony].raw_type == 146 &&
           b[colony].raw_owner == 3 && b[colony].maxhp == 300);
    assert(b[colony].hp == (damage_mode == 1 ? 90 :
                            damage_mode == 2 ? 280 : 290));
    assert(!(b[colony].flags & SC64_Z7_SUNKEN_MORPH_FLAG) &&
           b[colony].shield == 0 && b[colony].shield_remainder == 0);
    int sunken_hp = b[colony].hp;
    assert(sc64_race_building_armor(b[colony].type) ==
           sc64_race_building_armor(B_SUNKEN_COLONY));
    hit_target(MAX_UNITS + colony, 10, U_MARINE, ENEMY);
    assert(b[colony].hp == sunken_hp - 8 && b[colony].shield == 0);
    assert(minerals == before - 50);
}

static void test_prerequisite_and_identity_rejections(void)
{
    int colony;
    reset_world();
    colony = spawn_build(SIDE, B_CREEP_COLONY, 700, 700, 0);
    assert(colony >= 0);
    b[colony].raw_type = 143; b[colony].raw_owner = 3;
    minerals = 500;
    assert(!race_z7_colony_morph_available(colony)); /* no completed Pool */
    int pool = spawn_build(SIDE, B_SPAWNING_POOL, 620, 700, 0);
    assert(pool >= 0);
    assert(race_z7_colony_morph_available(colony));
    b[colony].raw_owner = 4;
    assert(!race_z7_colony_morph_available(colony));
    b[colony].raw_owner = 3;
    b[colony].under_construction = 1;
    assert(!race_z7_colony_morph_available(colony));
    b[colony].under_construction = 0;
    b[colony].type = B_SUNKEN_COLONY;
    assert(!race_z7_colony_morph_available(colony));
}

static void test_real_campaign_sram_resume(void)
{
    RaceSave saved, old_generation;
    Building loaded;
    int hatch, pool, colony, worker = -1;
    int site, x, y;
    launch_z7_normally();

    /* Test-only setup adds the already-completed Pool/Hatchery prerequisites
       to the authenticated world. The colony itself uses ordinary paid
       race_start_build and race_builder_arrived helpers. */
    site = find_any_legal_site(B_HATCHERY, 700, 700);
    assert(site >= 0);
    hatch = spawn_build(SIDE, B_HATCHERY, site % WORLD_W, site / WORLD_W, 0);
    assert(hatch >= 0 && b[hatch].raw_owner == 3 && b[hatch].raw_type == 131);
    site = find_legal_site(B_SPAWNING_POOL, b[hatch].x + 56, b[hatch].y);
    assert(site >= 0);
    pool = spawn_build(SIDE, B_SPAWNING_POOL,
                       site % WORLD_W, site / WORLD_W, 0);
    assert(pool >= 0 && b[pool].raw_owner == 3 && b[pool].raw_type == 142);
    site = find_legal_site(B_CREEP_COLONY, b[hatch].x + 96, b[hatch].y + 48);
    assert(site >= 0);
    x = site % WORLD_W; y = site / WORLD_W;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_owner == 3 &&
            u[i].raw_type == 41 && u[i].type == U_DRONE &&
            u[i].order != ORDER_BUILD) { worker = i; break; }
    assert(worker >= 0);
    u[worker].x = x + 32; u[worker].y = y;
    assert(race_start_build(SIDE, worker, B_CREEP_COLONY, x, y));
    colony = u[worker].target;
    u[worker].x = x + 32; u[worker].y = y;
    assert(race_builder_arrived(worker));
    assert(!u[worker].alive && b[colony].raw_type == 143);
    for (int i = 0; i < (300 * 5 + 3) / 4; ++i) race_tick_buildings();
    assert(building_complete(colony) && b[colony].hp == 400);
    assert(race_z7_colony_start_sunken(colony));

    for (int i = 0; i < 90; ++i) {
        assert(race_z7_colony_pre_building_tick());
        race_tick_buildings();
        assert(race_z7_colony_post_building_tick());
    }
    assert(b[colony].type == B_CREEP_COLONY && b[colony].raw_type == 143 &&
           b[colony].under_construction && b[colony].build_progress == 90 &&
           b[colony].shield == 0 && b[colony].queue_progress == 400);
    assert(race_z7_colony_morph_state_valid(&b[colony]));
    assert(race_save_game());
    saved = race_data;
    assert(race_z7_save_candidate_valid(&saved, &saved.z7,
                                        SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
    assert(race_save_decode_building(&saved, (unsigned)colony, &loaded));
    assert(race_z7_colony_morph_state_valid(&loaded));
    old_generation = saved;
    old_generation.version = SAVE_VERSION - 1;
    assert(!race_z7_save_candidate_valid(&old_generation,
                                        &old_generation.z7,
                                        SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
    save_write(&saved, sizeof(saved), SAVE_VERSION);
    /* Store a generation-33 payload under the current profile's tag so the
       real load path must reject it rather than selecting the prior valid
       dual-slot record. */
    save_write(&old_generation, sizeof(old_generation), SAVE_VERSION);
    assert(!race_load_game()); /* generation 33 is rejected by generation 34 */
    assert(race_z7_active() && b[colony].alive &&
           b[colony].type == B_CREEP_COLONY &&
           b[colony].build_progress == 90 &&
           (b[colony].flags & SC64_Z7_SUNKEN_MORPH_FLAG));
    save_write(&saved, sizeof(saved), SAVE_VERSION);
    /* A genuine cold load starts from HOME with no bound Z7 world. The row
       validator must therefore depend only on the decoded building bytes. */
    game_init();
    held_keys = 0;
    assert(screen == SCREEN_TITLE && !race_z7_active());
    assert(race_z7_colony_morph_state_valid(&loaded));
    assert(race_load_game());
    assert(race_z7_active() && race_z7_live_world_valid());
    assert(b[colony].type == B_CREEP_COLONY && b[colony].raw_type == 143 &&
           b[colony].under_construction && b[colony].build_progress == 90 &&
           b[colony].shield == 0 && b[colony].queue_progress == 400 &&
           (b[colony].flags & SC64_Z7_SUNKEN_MORPH_FLAG));
    assert(race_z7_colony_morph_state_valid(&b[colony]));
    assert(race_save_decode_building(&saved, (unsigned)colony, &loaded));
    assert(loaded.type == B_CREEP_COLONY && loaded.raw_type == 143 &&
           loaded.queue_progress == 400 && loaded.shield == 0);

    for (int i = 90; i < (300 * 5 + 3) / 4; ++i) {
        assert(race_z7_colony_pre_building_tick());
        race_tick_buildings();
        assert(race_z7_colony_post_building_tick());
    }
    assert(building_complete(colony) && b[colony].type == B_SUNKEN_COLONY &&
           b[colony].raw_type == 146 && b[colony].raw_owner == 3 &&
           b[colony].hp == 300 && b[colony].shield == 0);

    /* Death can occur after the normal pre/post production hooks. Use a
       second canonical-world colony, retain its morph marker through save and
       a HOME cold-load, then let the next real frame clean the dead row. */
    site = find_legal_site(B_CREEP_COLONY, b[hatch].x + 120, b[hatch].y + 64);
    if (site < 0)
        site = find_legal_site(B_CREEP_COLONY, b[hatch].x - 120, b[hatch].y + 64);
    assert(site >= 0);
    colony = spawn_build(SIDE, B_CREEP_COLONY,
                         site % WORLD_W, site / WORLD_W, 0);
    assert(colony >= 0 && b[colony].raw_type == SC64_Z7_CREEP_RAW);
    assert(race_z7_colony_start_sunken(colony));
    assert(race_z7_colony_pre_building_tick());
    race_tick_buildings();
    assert(race_z7_colony_post_building_tick());
    hit_target(MAX_UNITS + colony, b[colony].hp + 1000,
               U_MARINE, ENEMY);
    assert(!b[colony].alive &&
           (b[colony].flags & SC64_Z7_SUNKEN_MORPH_FLAG));
    assert(race_save_game());
    saved = race_data;
    save_write(&saved, sizeof(saved), SAVE_VERSION);
    game_init();
    held_keys = 0;
    assert(!race_z7_active() && screen == SCREEN_TITLE);
    assert(race_load_game());
    assert(race_z7_active() && !b[colony].alive &&
           (b[colony].flags & SC64_Z7_SUNKEN_MORPH_FLAG));
    game_update(0, 0);
    assert(race_z7_active() && !b[colony].alive &&
           !(b[colony].flags & SC64_Z7_SUNKEN_MORPH_FLAG) &&
           b[colony].queue_progress == 0);
    assert(race_save_game());
}

static void test_normal_menu_colony_build_and_sunken_dispatch(void)
{
    int hatch, pool, worker = -1, site, colony = -1, before;
    launch_z7_normally();
    site = find_any_legal_site(B_HATCHERY, 700, 700);
    assert(site >= 0);
    hatch = spawn_build(SIDE, B_HATCHERY, site % WORLD_W, site / WORLD_W, 0);
    assert(hatch >= 0);
    site = find_legal_site(B_SPAWNING_POOL, b[hatch].x + 56, b[hatch].y);
    assert(site >= 0);
    pool = spawn_build(SIDE, B_SPAWNING_POOL,
                       site % WORLD_W, site / WORLD_W, 0);
    assert(pool >= 0 && building_complete(pool));
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 41 &&
            u[i].order != ORDER_BUILD) { worker = i; break; }
    assert(worker >= 0);

    /* The ordinary Drone command list exposes Creep Colony after the
       existing Zerg structure rows. Placement and payment then use the normal
       build mode and race_start_build path. */
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    selected_building = -1;
    u[worker].selected = 1;
    assert(field_menu_count() == MENU_COUNT);
    open_command_row(MENU_Z7_CREEP);
    press(KEY_A);
    assert(!menu_open && mode == MODE_BUILD && menu == B_CREEP_COLONY);
    site = find_legal_site(B_CREEP_COLONY, b[hatch].x + 80, b[hatch].y + 48);
    assert(site >= 0);
    int x = site % WORLD_W, y = site / WORLD_W;
    before = minerals;
    cx = b[hatch].x; cy = b[hatch].y; /* occupied Hatchery footprint */
    press(KEY_A);
    assert(mode == MODE_BUILD && minerals == before && u[worker].alive &&
           u[worker].order != ORDER_BUILD);
    cx = x; cy = y;
    press(KEY_A);
    assert(mode == MODE_NORMAL && minerals == before - 75 &&
           u[worker].order == ORDER_BUILD && u[worker].target >= 0);
    colony = u[worker].target;
    u[worker].x = x + 32; u[worker].y = y;
    assert(race_builder_arrived(worker) && !u[worker].alive);
    for (int i = 0; i < SC64_Z7_SUNKEN_MORPH_TICKS; ++i)
        race_tick_buildings();
    assert(building_complete(colony) && b[colony].type == B_CREEP_COLONY);

    /* A completed selected Creep Colony gets the contextual SUNKEN row.
       Insufficient minerals closes the menu but changes neither state nor
       identity; a funded activation begins the exact in-place morph. */
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    selected_building = colony;
    minerals = 49;
    int hp = b[colony].hp;
    assert(field_menu_count() == MENU_STOP + 2);
    open_command_row(MENU_STOP + 1);
    press(KEY_A);
    assert(!menu_open && b[colony].type == B_CREEP_COLONY &&
           !(b[colony].flags & SC64_Z7_SUNKEN_MORPH_FLAG) && minerals == 49);
    minerals = 500;
    selected_building = colony;
    open_command_row(MENU_STOP + 1);
    press(KEY_A);
    assert(!menu_open && mode == MODE_NORMAL && minerals == 450 &&
           b[colony].type == B_CREEP_COLONY && b[colony].raw_type == 143 &&
           b[colony].hp == hp && b[colony].under_construction &&
           (b[colony].flags & SC64_Z7_SUNKEN_MORPH_FLAG));
}

int main(void)
{
    test_prerequisite_and_identity_rejections();
    reset_world();
    test_paid_in_place_morph_and_roundtrip(0);
    reset_world();
    test_paid_in_place_morph_and_roundtrip(1);
    reset_world();
    test_paid_in_place_morph_and_roundtrip(2);
    test_real_campaign_sram_resume();
    test_normal_menu_colony_build_and_sunken_dispatch();
    puts("Z7 normal Creep Colony build and faithful Sunken morph passed");
    return 0;
}
