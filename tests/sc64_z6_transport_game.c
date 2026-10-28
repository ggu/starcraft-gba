/* Actual Z6 game-array transport and Ventral Sacs integration. Source actors
 * are used for all cargo/save tests; this is not a campaign-win fixture. */
#define main sc64_z6_runtime_test_main
#include "sc64_z6_runtime.c"
#undef main

#include <assert.h>
#include <stdio.h>
#include <string.h>

#if !defined(SC64_Z6_TRANSPORT_ENABLED) || !defined(SC64_TRANSPORT_RUNTIME)
#error "Compile this fixture with Z6 Ventral Sacs and shared transport enabled"
#endif

static unsigned z6_transport_held;

static void z6_transport_input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~z6_transport_held));
    z6_transport_held = held;
}

static void z6_transport_press(unsigned key)
{
    z6_transport_input(key);
    z6_transport_input(0);
}

static void z6_transport_launch(void)
{
    int ordinal;
    game_init();
    z6_transport_held = 0;
    assert(race_front.page == SC_FRONT_HOME);
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,
                                            SC64_Z6_GLOBAL_ID);
    assert(ordinal == 5);
    z6_transport_press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION &&
           race_front.faction == SC_FRONT_ZERG);
    for (int page = 0; page < ordinal / 2; ++page)
        z6_transport_press(KEY_RIGHT);
    if (ordinal & 1) z6_transport_press(KEY_DOWN);
    z6_transport_press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING &&
           race_front.mission == SC64_Z6_GLOBAL_ID);
    z6_transport_press(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_z6_active() && race_z6_transport_state_valid());
    assert(!race_z6_transport_overlord_ready());
}

static int source_building(unsigned raw)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == (int)raw && b[i].raw_owner == 3)
            return i;
    return -1;
}

static int source_unit(unsigned raw, unsigned ordinal)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == (int)raw && u[i].raw_owner == 3) {
            if (!ordinal) return i;
            --ordinal;
        }
    return -1;
}

static void select_building_normal(int slot)
{
    assert(slot >= 0 && slot < MAX_BUILDINGS && b[slot].alive);
    clear_selection();
    selected_building = -1;
    cx = b[slot].x;
    cy = b[slot].y;
    z6_transport_press(KEY_A);
    assert(selected_building == slot);
}

static void select_unit_normal(int slot)
{
    assert(slot >= 0 && slot < MAX_UNITS && u[slot].alive);
    clear_selection();
    selected_building = -1;
    cx = u[slot].x;
    cy = u[slot].y;
    z6_transport_press(KEY_A);
    assert(selected_building < 0 && u[slot].selected);
}

static void open_command_row(unsigned row)
{
    z6_transport_press(KEY_B);
    assert(menu_open && menu_cursor == 0);
    for (unsigned i = 0; i < row; ++i) z6_transport_press(KEY_DOWN);
    assert(menu_cursor == (int)row);
}

static void morph_starting_hatchery_to_lair(void)
{
    int hatchery = source_building(131);
    int pool = source_building(142);
    assert(hatchery >= 0 && pool >= 0);
    assert(b[hatchery].type == B_HATCHERY && !b[hatchery].under_construction);
    assert(race_human == R_ZERG);
    minerals = gas = 5000;

    select_building_normal(hatchery);
    open_command_row(MENU_ZP_LAIR);
    assert(field_menu_count() == MENU_COUNT);
    int before_minerals = minerals, before_gas = gas;
    z6_transport_press(KEY_A);
    assert(!menu_open && b[hatchery].type == B_LAIR &&
           b[hatchery].raw_type == 132 && b[hatchery].raw_owner == 3 &&
           b[hatchery].under_construction);
    assert(minerals == before_minerals - building_cost(B_LAIR));
    assert(gas == before_gas - building_gas_cost(B_LAIR));

    /* Use the ordinary race construction clock, without spending thousands
       of unrelated campaign ticks in this isolated technology fixture. */
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(B_LAIR);
    assert(rule && rule->build_frames > 1);
    b[hatchery].build_progress = (rule->build_frames * 5 + 3) / 4 - 1;
    race_tick_buildings();
    assert(!b[hatchery].under_construction && b[hatchery].raw_type == 132);
    assert(race_z6_transport_research_selected());
}

static int nearby_legal_pool_site(int hatchery)
{
    /* Search a small ring on source creep. Selection, placement, Drone
       consumption and the resulting save still pass through live handlers. */
    for (int radius = 48; radius <= 144; radius += 16)
        for (int dy = -radius; dy <= radius; dy += 16)
            for (int dx = -radius; dx <= radius; dx += 16) {
                if (FAST_ABS(dx) != radius && FAST_ABS(dy) != radius) continue;
                int x = b[hatchery].x + dx, y = b[hatchery].y + dy;
                if (x >= 16 && y >= 16 && x < WORLD_W - 16 &&
                    y < WORLD_H - 16 && race_can_site(SIDE, B_SPAWNING_POOL, x, y) &&
                    can_place(B_SPAWNING_POOL, x, y)) return x | (y << 12);
            }
    return -1;
}

static void build_pool_with_source_drone_and_save(void)
{
    int hatchery = source_building(132);
    int drone = source_unit(41, 0);
    assert(hatchery >= 0 && drone >= 0);
    int packed = nearby_legal_pool_site(hatchery);
    assert(packed >= 0);
    int x = packed & 0xfff, y = packed >> 12;
    u32 old_deaths = race_z6.deaths_by_source[3];
    minerals = gas = 5000;

    /* Direct positioning is isolated fixture setup; the normal field menu
       and build placement invoke the actual worker/building engine paths.
       Keep the Drone outside the placement exclusion radius. */
    u[drone].x = x - 40; u[drone].y = y;
    select_unit_normal(drone);
    open_command_row(MENU_BARRACKS);
    z6_transport_press(KEY_A);
    assert(mode == MODE_BUILD && menu == B_SPAWNING_POOL);
    cx = x; cy = y;
    z6_transport_press(KEY_A);
    assert(u[drone].order == ORDER_BUILD &&
           u[drone].target >= 0 && u[drone].target < MAX_BUILDINGS);
    int pool = u[drone].target;
    assert(b[pool].alive && b[pool].type == B_SPAWNING_POOL &&
           b[pool].raw_type == 142 && b[pool].raw_owner == 3 &&
           b[pool].under_construction);
    for (unsigned tick = 0; tick < 80 && u[drone].alive; ++tick)
        z6_transport_input(0);
    assert(!u[drone].alive && race_z6.human_drone_morphs == 1u);
    assert(race_z6.deaths_by_source[3] == old_deaths);
    assert(race_save_game());
    RaceSave good = race_data;
    assert(good.z6.human_drone_morphs == 1u);
    assert(race_load_game());
    assert(race_z6.human_drone_morphs == 1u && !u[drone].alive &&
           b[pool].alive && b[pool].raw_type == 142);
}

static void reject_transport_save_without_live_mutation(const RaceSave *bad)
{
    static Unit before_units[MAX_UNITS];
    static Building before_buildings[MAX_BUILDINGS];
    Sc64Z6PersistentState before_state = race_z6;
    int before_frame = frame;
    int before_minerals = minerals, before_gas = gas;
    memcpy(before_units, u, sizeof(u));
    memcpy(before_buildings, b, sizeof(b));
    save_write(bad, sizeof(*bad), SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(before_units, u, sizeof(u)));
    assert(!memcmp(before_buildings, b, sizeof(b)));
    assert(!memcmp(&before_state, &race_z6, sizeof(race_z6)));
    assert(frame == before_frame && minerals == before_minerals && gas == before_gas);
}

static void research_is_paid_timed_pauseable_and_saved(void)
{
    RaceSave good, bad;
    z6_transport_launch();
    assert(source_unit(42, 0) >= 0 && source_building(131) >= 0);
    assert(!race_z6_transport_overlord_ready());
    assert(!race_z6_overlord_menu_available());

    morph_starting_hatchery_to_lair();
    build_pool_with_source_drone_and_save();
    int lair = source_building(132);
    assert(lair >= 0);
    select_building_normal(lair);
    assert(race_z6_transport_research_menu_available());
    assert(field_menu_count() == MENU_STOP + 2);
    open_command_row(MENU_STOP + 1);
    assert(race_z6_transport_research_selected());
    int before_minerals = minerals, before_gas = gas;
    z6_transport_press(KEY_A);
    assert(!menu_open && minerals == before_minerals - 200 &&
           gas == before_gas - 200);
    u16 after_start = race_z6.transport.ticks_remaining;
    /* The press and key-release are both live game updates. */
    assert(after_start > 0 && after_start <= SC64_Z6_TRANSPORT_RESEARCH_TICKS);
    assert(!race_z6_transport_overlord_ready());

    for (unsigned i = 0; i < 80; ++i) z6_transport_input(0);
    assert(race_z6.transport.ticks_remaining == after_start - 80u);
    assert(race_save_game());
    good = race_data;
    assert(good.z6.transport.ticks_remaining == race_z6.transport.ticks_remaining);
    assert(race_load_game());
    assert(race_z6.transport.ticks_remaining == good.z6.transport.ticks_remaining);

    bad = good;
    bad.z6.transport.researched = 2;
    reject_transport_save_without_live_mutation(&bad);

    u16 before_pause = race_z6.transport.ticks_remaining;
    z6_transport_press(KEY_START);
    assert(screen == SCREEN_PAUSE);
    for (unsigned i = 0; i < 60; ++i) z6_transport_input(0);
    assert(race_z6.transport.ticks_remaining == before_pause);
    z6_transport_press(KEY_A);
    assert(screen == SCREEN_PLAY &&
           race_z6.transport.ticks_remaining <= before_pause);
    u16 before_resume = race_z6.transport.ticks_remaining;

    unsigned live_ticks = 0;
    while (race_z6.transport.ticks_remaining && live_ticks < 3100u) {
        z6_transport_input(0);
        ++live_ticks;
        assert(screen == SCREEN_PLAY && !won && !lost);
    }
    assert(live_ticks == before_resume && race_z6.transport.ticks_remaining == 0);
    assert(race_z6_transport_overlord_ready() &&
           race_z6.transport.researched == 1);
    assert(race_z6_transport_state_valid());
    assert(race_save_game());
    good = race_data;
    assert(race_load_game() && race_z6_transport_overlord_ready());
}

static int loaded_to(int carrier)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].order == ORDER_TRANSPORTED &&
                 u[i].target == carrier;
    return count;
}

static void board_source_hydras_and_overflow_drone(int carrier)
{
    int hydras[4];
    for (unsigned i = 0; i < 4; ++i) {
        hydras[i] = source_unit(38, i);
        assert(hydras[i] >= 0 && race_transport_passenger_space(&u[hydras[i]]) == 2);
        u[hydras[i]].x = u[carrier].x + (int)i;
        u[hydras[i]].y = u[carrier].y;
        u[hydras[i]].order = ORDER_IDLE;
        u[hydras[i]].selected = 1;
    }
    int overflow = source_unit(41, 0);
    assert(overflow >= 0 && race_transport_passenger_space(&u[overflow]) == 1);
    u[overflow].x = u[carrier].x + 5;
    u[overflow].y = u[carrier].y;
    u[overflow].order = ORDER_IDLE;
    u[overflow].selected = 1;

    clear_selection();
    for (unsigned i = 0; i < 4; ++i) u[hydras[i]].selected = 1;
    cx = u[carrier].x; cy = u[carrier].y;
    z6_transport_press(KEY_A); /* Normal click-to-board dispatch. */
    for (unsigned tick = 0; tick < 8 && loaded_to(carrier) != 4; ++tick)
        z6_transport_input(0);
    assert(loaded_to(carrier) == 4);
    assert(race_transport_cargo_space(carrier) == 8);

    /* A fifth passenger is rejected by the real click-to-board handler when
       the carrier already has exactly its eight source capacity points full. */
    clear_selection();
    u[overflow].selected = 1;
    cx = u[carrier].x; cy = u[carrier].y;
    z6_transport_press(KEY_A);
    assert(u[overflow].order != ORDER_BOARD &&
           u[overflow].order != ORDER_TRANSPORTED);

    clear_selection();
    select_unit_normal(carrier);
    assert(race_z6_overlord_menu_available());
    assert(field_menu_count() == MENU_STOP + 2);
    assert(race_save_game());
    RaceSave loaded = race_data;
    assert(race_load_game());
    assert(loaded_to(carrier) == 4 && race_transport_cargo_space(carrier) == 8);
    assert(race_z6_transport_save_candidate_valid(
        &loaded, &loaded.z6.transport, SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
    RaceSave bad = loaded;
    bad.z6.transport.researched = 0;
    reject_transport_save_without_live_mutation(&bad);
}

static void unload_by_normal_menu_and_board_kerrigan(void)
{
    int carrier = source_unit(42, 0);
    int hero = race_z6_cloak_hero_slot();
    assert(carrier >= 0 && hero >= 0 && loaded_to(carrier) == 4);

    clear_selection();
    select_unit_normal(carrier);
    open_command_row(MENU_STOP + 1);
    assert(race_z6_overlord_menu_available());
    z6_transport_press(KEY_A);
    assert(mode == MODE_UNLOAD && !menu_open);
    cx = u[carrier].x; cy = u[carrier].y;
    z6_transport_press(KEY_A); /* Normal unload destination click. */
    assert(u[carrier].order == ORDER_UNLOAD && u[carrier].target == -1);
    int immediate_cooldown_seen = 0;
    for (unsigned tick = 0; tick < 800 && loaded_to(carrier); ++tick) {
        z6_transport_input(0);
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].raw_type == 38 &&
                u[i].order != ORDER_TRANSPORTED && u[i].cd >= 10)
                immediate_cooldown_seen = 1;
    }
    assert(loaded_to(carrier) == 0 && immediate_cooldown_seen);

    /* Kerrigan consumes one capacity point and keeps her source identity. */
    u[hero].x = u[carrier].x;
    u[hero].y = u[carrier].y;
    u[hero].order = ORDER_IDLE;
    clear_selection();
    u[hero].selected = 1;
    cx = u[carrier].x; cy = u[carrier].y;
    z6_transport_press(KEY_A);
    assert(u[hero].order == ORDER_TRANSPORTED && u[hero].target == carrier);
    assert(loaded_to(carrier) == 1 && race_transport_cargo_space(carrier) == 1);
    assert(race_save_game());
    RaceSave loaded = race_data;
    assert(race_load_game());
    assert(loaded_to(carrier) == 1 && u[hero].order == ORDER_TRANSPORTED);
    assert(race_z6_transport_save_candidate_valid(
        &loaded, &loaded.z6.transport, SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));

    unsigned source3_deaths = race_z6.deaths_by_source[3];
    hit_target(carrier, 10000, U_ZERGLING, ENEMY);
    assert(!u[carrier].alive && !u[hero].alive);
    assert(race_z6.deaths_by_source[3] == source3_deaths + 2u);
    assert(loaded_to(carrier) == 0);
}

int main(void)
{
    research_is_paid_timed_pauseable_and_saved();
    int carrier = source_unit(42, 0);
    assert(carrier >= 0 && race_z6_transport_overlord_ready());
    board_source_hydras_and_overflow_drone(carrier);
    unload_by_normal_menu_and_board_kerrigan();
    puts("Z6 normal-menu transport research, cargo orders, persistence and death accounting passed");
    return 0;
}
