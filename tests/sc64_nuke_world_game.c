/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/game.c"
#ifdef SC64_NUKE_ARM_WORLD_TEST
volatile u16 demo_keys;
static void nuke_fixture_emit(const char *prefix, const char *line)
{
    volatile char *out = (volatile char *)0x04fff600;
    while (*prefix) *out++ = *prefix++;
    while (*line) *out++ = *line++;
    *out = 0;
    *(volatile u16 *)0x04fff700 = 0x102;
}
#define nuke_fixture_log(line) nuke_fixture_emit("IMPACT NUKE ", line)
#define NUKE_FIXTURE_STR1(x) #x
#define NUKE_FIXTURE_STR(x) NUKE_FIXTURE_STR1(x)
#define assert(expr) do { if (!(expr)) { \
    nuke_fixture_log("FAIL line " NUKE_FIXTURE_STR(__LINE__)); \
    for (;;) __asm__ volatile ("" ::: "memory"); \
} } while (0)
#define puts(msg) nuke_fixture_log(msg)
#else
#include <assert.h>
#include <stdio.h>
#endif

static int nuke_fixture_equal(const char *a, const char *b)
{
    while (*a && *a == *b) ++a, ++b;
    return *a == *b;
}

static int fixture_building(int owner, int raw, int x, int y)
{
    int type = sc64_race_building_from_raw(raw);
    assert(type >= 0);
    int slot = spawn_build(owner, type, x, y, 0);
    assert(slot >= 0 && sc64_race_raw_building(b[slot].type) == raw);
    b[slot].raw_type = raw;
    return slot;
}

static int fixture_unit(int owner, int raw, int x, int y)
{
    int type = sc64_race_unit_from_raw(raw);
    assert(type >= 0);
    int slot = spawn_unit(owner, type, x, y);
    assert(slot >= 0 && sc64_race_raw_unit(u[slot].type) == raw);
    u[slot].raw_type = raw;
    return slot;
}

static void nuke_action(int action)
{
    int row = race_nuke_command_index();
    assert(row >= 0 && row < field_menu_count());
    assert(nuke_fixture_equal(race_menu_label(row), race_nuke_menu_label(action)));
    assert(!menu_open);
    game_update(KEY_B, KEY_B);
    assert(menu_open);
    menu_cursor = row;
    game_update(KEY_A, KEY_A);
    assert(!menu_open);
}

static void advance_world(int updates)
{
    for (int i = 0; i < updates; ++i) {
        ++frame;
        race_nuke_tick();
    }
}

static int find_ghost(int owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && sc64_race_ordinary_ghost(u[i].type))
            return i;
    return -1;
}

#ifdef SC64_TERRAN_LATE_RUNTIME
static void t8_attachment(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T8;
    init_state();
    assert(race_late_active() && screen == SCREEN_PLAY);
    int silo = -1, cc = -1, witness = -1, spotter = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (race_nuke_raw_building(i, 108) && b[i].owner == NEUTRAL) silo = i;
        if (race_nuke_raw_building(i, 106) && b[i].owner == SIDE && race_late_airborne(i)) cc = i;
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (race_nuke_ghost_eligible(i, SIDE)) {
        if (u[i].x == 3484 / 2 && u[i].y == 3885 / 2) witness = i;
        else spotter = i;
    }
    assert(silo >= 0 && cc >= 0 && find_ghost(SIDE) >= 0);
    assert(!race_nuke_silo_online(silo, SIDE));
    int x = b[silo].x - 48, y = b[silo].y - 8;
    /* Relocate the flying CC as fixture setup; the campaign landing, Silo
     * ownership transfer and source availability use the real world path. */
    b[cc].x = x + 2; b[cc].y = y;
    assert(race_late_flight_order(cc, x + 2, y - 2, 1));
    for (int i = 0; i < 20 && race_late_airborne(cc); ++i) {
#ifdef SC64_NUKE_SILO_JOURNAL
        game_update(0, 0);
#else
        ++frame;
        race_late_flight_tick();
#endif
    }
    assert(!race_late_airborne(cc) && b[cc].x == x && b[cc].y == y);
    assert(b[silo].owner == SIDE && b[silo].raw_owner == b[cc].raw_owner &&
           race_nuke_silo_online(silo, SIDE));
    minerals = gas = 500;
    assert(sc64_terran_late_source_product_available(
           sc64_terran_late_product_source_slot(SIDE, b[silo].raw_owner), SC64_NUKE_MISSILE_RAW));
    assert(race_nuke_arm(silo, SIDE) == 2); /* T8 opens at 20/20. */
    fixture_building(SIDE, 109, x - 96, y); /* supply from an additional completed Depot */
    assert(race_nuke_arm(silo, SIDE) == 4);
#ifdef SC64_NUKE_SILO_JOURNAL
    assert(race_save_game());
    race_nuke_reset();
    assert(!race_nuke_reserved_supply(SIDE) && race_load_game());
    assert(race_nuke_silos[silo].ammo.remaining == SC64_NUKE_MISSILE_SOURCE_TICKS &&
           race_nuke_reserved_supply(SIDE) == 16);
    puts("T8: queued Silo round-trips through the cartridge SRAM journal");
#endif
    advance_world(1876);
    assert(race_nuke_armed_slot(SIDE) == silo);
    assert(race_late_lift(cc));
    assert(b[silo].owner == NEUTRAL && race_nuke_armed_slot(SIDE) < 0 &&
           race_nuke_reserved_supply(SIDE) == 16);
    assert(race_late_flight_order(cc, x, y, 1));
    ++frame; race_late_flight_tick();
    assert(b[silo].owner == SIDE && race_nuke_armed_slot(SIDE) == silo &&
           race_nuke_reserved_supply(SIDE) == 16);
    puts("T8: native roster, Command Center lands, claims neutral Silo and trains an allowed missile");
    /* Match the native target and the specific friendly Ghost that died. */
    assert(witness >= 0 && u[witness].alive && spotter >= 0 &&
           race_nuke_issue(spotter, SIDE, 3666 / 2, 3882 / 2));
    advance_world(600);
    assert(!u[witness].alive && !race_nuke_reserved_supply(SIDE));
    puts("T8: same friendly Ghost dies at the observed native strike point");
}
#endif

static void nuke_supply_boundary(void)
{
    game_init();
    race_mode = 1; race_human = R_TER; race_bots = 1;
    init_state();
    memset(u, 0, sizeof u); memset(b, 0, sizeof b);
    int silo = fixture_building(SIDE, 108, 240, 200);
    fixture_building(SIDE, 106, 192, 192);
    for (int i = 0; i < 25; ++i) fixture_building(SIDE, 109, 160, 160);
    minerals = gas = 10000;
    for (int i = 0; i < 107; ++i) fixture_unit(SIDE, 0, 80, 80);
    for (int i = 0; i < 17; ++i) {
        int producer = fixture_building(SIDE, 111, 100, 100);
        for (int q = 0; q < 5; ++q) assert(race_queue_unit(SIDE, producer, U_MARINE));
    }
    minerals = gas = 1000;
    assert(race_owner_supply_capacity(SIDE) > 400);
    assert(race_owner_supply_cap(SIDE) == 400 && race_owner_reserved_supply(SIDE) == 384);
    int marine = fixture_unit(SIDE, 0, 80, 80);
    assert(race_nuke_arm(silo, SIDE) == 2 && minerals == 1000 && gas == 1000);
    hit_target_impl_ex(marine, 10000, -1, ENEMY, -1, 0);
    assert(race_nuke_arm(silo, SIDE) == 4 && race_owner_reserved_supply(SIDE) == 400);
    assert(race_nuke_reserved_supply(ENEMY) == 0);
    puts("missile reserves exactly to 200 supply, refuses above it without spending");
}

int main(void)
{
    gba_init();
#ifdef SC64_NUKE_ARM_WORLD_TEST
    *(volatile u16 *)0x04fff780 = 0xc0de;
#endif
    game_init();
    race_mode = 1; race_human = R_TER; race_bots = 1;
    init_state();
    assert(screen == SCREEN_PLAY && race_mode && race_player[SIDE] == R_TER);
    memset(u, 0, sizeof u); memset(b, 0, sizeof b);
    invalidate_path_grid();
    minerals = gas = 5000;
    int command = fixture_building(SIDE, 106, 192, 192);
    int facility = fixture_building(SIDE, 116, 320, 192);
    int barracks = fixture_building(SIDE, 111, 128, 360);
    fixture_building(SIDE, 112, 192, 360);
    fixture_building(ENEMY, 131, 720, 720); /* keep the custom match active */
    const Sc64SkirmishRecord *silo_row = race_skirmish_build_record(SIDE, 108);
    const Sc64SkirmishRecord *covert_row = race_skirmish_build_record(SIDE, 117);
    assert(silo_row && covert_row && !race_skirmish_record_ready(SIDE, command, silo_row));
    assert(!race_queue_unit(SIDE, barracks, sc64_race_unit_from_raw(1)));
    assert(race_skirmish_start_record(SIDE, facility, covert_row));
    int covert = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (race_nuke_raw_building(i, 117)) covert = i;
    assert(covert >= 0 && !building_complete(covert));
    for (int i = 0; i < 4000 && !building_complete(covert); ++i) race_tick_buildings();
    assert(building_complete(covert));
    assert(race_queue_unit(SIDE, barracks, sc64_race_unit_from_raw(1)));
    for (int i = 0; i < 4000 && find_ghost(SIDE) < 0; ++i) race_tick_buildings();
    int ghost = find_ghost(SIDE);
    assert(ghost >= 0 && sc64_race_raw_unit(u[ghost].type) == 1);

    assert(race_skirmish_start_record(SIDE, command, silo_row));
    int silo = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (race_nuke_raw_building(i, 108)) silo = i;
    assert(silo >= 0 && !race_nuke_silo_online(silo, SIDE));
    for (int i = 0; i < 4000 && !building_complete(silo); ++i) race_tick_buildings();
    assert(race_nuke_silo_online(silo, SIDE));
    int ore = minerals, gas_before = gas;
    int already = race_owner_reserved_supply(SIDE);
    selected_building = silo;
    nuke_action(NUKE_MENU_ARM);
    assert(minerals == ore - 200 && gas == gas_before - 200);
    assert(race_owner_reserved_supply(SIDE) == already + 16);
#ifdef SC64_FIELD_CONTROLS
    HudJobs jobs;
    hud_building_jobs(silo, &jobs, 0);
    assert(jobs.count == 1 && jobs.icon[0] == card_portrait(14) && jobs.level[0] == 0);
#endif
    assert(race_nuke_arm(silo, SIDE) == NUKE_TOAST_OCCUPIED);
    nuke_action(NUKE_MENU_CANCEL_ARM);
    assert(minerals == ore && gas == gas_before && race_owner_reserved_supply(SIDE) == already);
    nuke_action(NUKE_MENU_ARM);
    advance_world(1876);
    assert(race_nuke_armed_slot(SIDE) == silo && race_owner_reserved_supply(SIDE) == already + 16);
#ifdef SC64_FIELD_CONTROLS
    hud_building_jobs(silo, &jobs, 0);
    assert(jobs.count == 1 && jobs.level[0] == HUD_BAR_LEVELS);
#endif
#ifndef SC64_NUKE_SILO_JOURNAL
    assert(!race_save_game()); /* the no-journal world test must fail closed */
#endif

    selected_building = -1; u[ghost].selected = 1;
    int x = u[ghost].x + 48, y = u[ghost].y;
    int friendly = fixture_unit(SIDE, 0, x, y);
    int enemy = fixture_unit(ENEMY, 0, x, y + 8);
    already = race_owner_reserved_supply(SIDE) - 16;
    int far_x = x + 170;
    assert(race_nuke_issue(ghost, SIDE, far_x, y));
    assert(race_nuke_track(ghost) && !race_nuke_ghost_locked(ghost) &&
           u[ghost].order == ORDER_MOVE && u[ghost].tx == far_x);
    assert(race_nuke_abort(ghost) && race_nuke_armed_slot(SIDE) == silo);
    nuke_action(NUKE_MENU_CAST);
    assert(mode == MODE_NUCLEAR_TARGET);
    cx = x; cy = y;
    game_update(KEY_A, KEY_A);
    assert(mode == MODE_NORMAL && race_nuke_ghost_locked(ghost));
    assert(!race_save_game()); /* never save a partial in-flight state */
    gfx_begin(0); gfx_set_clip_y(12, 120); draw_unit(&u[ghost]); gfx_end();
    assert(race_nuke_armed_slot(SIDE) < 0 && race_owner_reserved_supply(SIDE) == already + 16);
    assert(!race_nuke_issue(ghost, SIDE, x, y));
    nuke_action(NUKE_MENU_ABORT);
    assert(!race_nuke_ghost_locked(ghost) && u[enemy].alive &&
           race_owner_reserved_supply(SIDE) == already);

    assert(race_nuke_arm(silo, SIDE) == 4);
    advance_world(1876);
    assert(race_nuke_issue(ghost, SIDE, x, y) && race_nuke_ghost_locked(ghost));
    hit_target_impl_ex(ghost, 10000, -1, ENEMY, -1, 0);
    assert(!u[ghost].alive && !race_nuke_ghost_locked(ghost) &&
           race_nuke_reserved_supply(SIDE) == 0 && u[enemy].alive);
    ghost = fixture_unit(SIDE, 1, x - 100, y);
    assert(race_nuke_arm(silo, SIDE) == 4);
    advance_world(1876);
    /* Keep the kill targets stationary until the impact by stepping only
     * the nuclear clock. Other combat paths must not be the source of death. */
    u[ghost].x = x - 100; u[ghost].y = y;
    assert(race_nuke_issue(ghost, SIDE, x, y));
    for (int i = 0; i < 500 && race_nuke_ghost_locked(ghost); ++i) {
        ++frame; race_nuke_tick();
    }
    assert(!race_nuke_ghost_locked(ghost));
    hit_target_impl_ex(ghost, 10000, -1, ENEMY, -1, 0);
    hit_target_impl_ex(MAX_UNITS + silo, 10000, -1, ENEMY, -1, 0);
    assert(!u[ghost].alive && !b[silo].alive && race_nuke_reserved_supply(SIDE) == 16);
    for (int i = 0; i < 200 && race_nuke_reserved_supply(SIDE); ++i) {
        ++frame; race_nuke_tick();
    }
    assert(!race_nuke_reserved_supply(SIDE));
    assert(!u[friendly].alive && !u[enemy].alive);
    assert(race_nuke_active_flights);
    gfx_begin(0); gfx_set_clip_y(12, 120); race_nuke_draw(); gfx_end();
    for (int i = 0; i < 80; ++i) { ++frame; race_nuke_tick(); }
    assert(!race_nuke_active_flights);
    puts("nuclear strike: stock add-on and Ghost production, GBA menu, paid ammo, abort and independent impact");
    nuke_supply_boundary();
#ifdef SC64_TERRAN_LATE_RUNTIME
    t8_attachment();
#endif
#ifdef SC64_NUKE_ARM_WORLD_TEST
    nuke_fixture_log("ordinary input and production checks passed");
    nuke_fixture_emit("IMPACT ", "DONE"); /* use the shared no-GDB runner */
    for (;;) __asm__ volatile ("" ::: "memory");
#endif
    return 0;
}
