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

#if defined(GBA_HOST) && defined(SC64_WORLD_DEPTH)
static void assert_nuke_draw_key(u32 key, unsigned part, unsigned layer)
{
    assert((key & DEPTH_SLOT_MASK) == part);
    assert(((key >> DEPTH_SLOT_BITS) & DEPTH_KIND_MASK) == DEPTH_NUKE_EFFECT);
    assert(key >> DEPTH_LAYER_BITS == layer);
}

static void assert_nuke_hud_untouched(void)
{
    static _Alignas(4) u8 hud[240 * 40];
    assert(gfx_read_band(0, 12, hud));
    for (unsigned i = 0; i < 240 * 12; ++i) assert(hud[i] == C_PANEL);
    assert(gfx_read_band(120, 40, hud));
    for (unsigned i = 0; i < sizeof hud; ++i) assert(hud[i] == C_PANEL);
}

static void test_nuke_dot_depth(void)
{
    enum { FIELD_BYTES = 240 * 108 };
    static _Alignas(4) u8 actor[FIELD_BYTES], dot[FIELD_BYTES], pair[FIELD_BYTES];
    game_init();
    race_mode = 1; race_human = R_TER; race_bots = 1;
    init_state();
    memset(u, 0, sizeof u);
    memset(b, 0, sizeof b);
    memset(m, 0, sizeof m);
    geyser_count = 0;
    camx = camy = 0;
    race_nuke_reset();
    const int x = 108, y = 52;
    int spotter = fixture_unit(SIDE, SC64_NUKE_GHOST_RAW, 300, y);
    Sc64NukeWorldFlight *flight = &race_nuke_flights[0];
    flight->strike = (Sc64NukeFlight){ .phase = SC64_NUKE_PAINTING, .spotter = spotter,
                                     .owner = SIDE, .x = x, .y = y };
    race_nuke_active_flights = 1;
    gfx_begin(C_PANEL);
    gfx_set_clip_y(12, 120);
    assert(sc64_nuke_draw_frame(&sc64_nuke_dot_frames[0], x, y + 12));
    assert(gfx_read_band(12, 108, dot));
    gfx_end();

    const int raw_types[] = { 5, 8 };
    for (unsigned j = 0; j < sizeof raw_types / sizeof *raw_types; ++j) {
        int slot = fixture_unit(SIDE, raw_types[j], x, y);
        assert(race_nuke_active_flights == 1);
        assert(sc64_unit_world_layer(&u[slot]) == (j ? 18u : 4u));
        gfx_begin(C_PANEL);
        gfx_set_clip_y(12, 120);
        draw_unit(&u[slot]);
        assert(gfx_read_band(12, 108, actor));
        gfx_end();

        gfx_begin(C_PANEL);
        gfx_set_clip_y(12, 120);
        sc64_draw_depth_ordered_actors();
        assert(gfx_read_band(12, 108, pair));
        assert_nuke_hud_untouched();
        gfx_end();

        unsigned overlap = 0;
        for (unsigned i = 0; i < FIELD_BYTES; ++i) {
            if (dot[i] == C_PANEL || actor[i] == C_PANEL || actor[i] == dot[i])
                continue;
            ++overlap;
            assert(pair[i] == (j ? actor[i] : dot[i]));
        }
        assert(overlap);
        printf("nuclear target dot against raw %d: %u overlap pixels\n",
               raw_types[j], overlap);
        u[slot].alive = 0;
    }
    puts("nuclear target dot: Tank below, Wraith above, HUD clear");
}

static void nuke_fixture_paint(int actor, int actor_above, const Sc64NukeFrame *part, int x, int y,
                               const Sc64NukeFrame *child)
{
    if (!actor_above) draw_unit(&u[actor]);
    assert(sc64_nuke_draw_frame(part, x, y));
    if (child) assert(sc64_nuke_draw_frame(child, x, y + SC64_NUKE_IMPACT_SOURCE_Y / 2));
    if (actor_above) draw_unit(&u[actor]);
}

static void compare_nuke_paint(int actor, int actor_above, const Sc64NukeFrame *part, int x, int y,
                               const Sc64NukeFrame *child)
{
    enum { FIELD_BYTES = 240 * 108 };
    static _Alignas(4) u8 expected[FIELD_BYTES], actual[FIELD_BYTES];
    gfx_begin(C_PANEL);
    gfx_set_clip_y(12, 120);
    nuke_fixture_paint(actor, actor_above, part, x, y, child);
    assert(gfx_read_band(12, 108, expected));
    gfx_end();

    /* Opaque missiles and mapped explosion pixels have different coverage.
     * A reversed control must change visible pixels for either raster mode. */
    gfx_begin(C_PANEL);
    gfx_set_clip_y(12, 120);
    nuke_fixture_paint(actor, !actor_above, part, x, y, child);
    assert(gfx_read_band(12, 108, actual));
    assert(memcmp(expected, actual, sizeof expected) != 0);
    gfx_end();

    for (unsigned page = 0; page < 2; ++page) {
        gfx_begin(C_PANEL);
        gfx_set_clip_y(12, 120);
        sc64_draw_depth_ordered_actors();
        assert(gfx_read_band(12, 108, actual));
        assert(memcmp(expected, actual, sizeof expected) == 0);
        assert_nuke_hud_untouched();
        gfx_end();
    }
}

static void test_nuke_missile_depth(void)
{
    game_init();
    race_mode = 1; race_human = R_TER; race_bots = 1;
    init_state();
    memset(u, 0, sizeof u);
    memset(b, 0, sizeof b);
    memset(m, 0, sizeof m);
    memset(visible, 0, sizeof visible);
    geyser_count = 0;
    camx = camy = 0;
    race_nuke_reset();
    const int x = 108, y = 52;
    int actor = fixture_unit(SIDE, 12, x, y);
    assert(sc64_unit_world_layer(&u[actor]) == 16);
    Sc64NukeWorldFlight *flight = &race_nuke_flights[0];
    flight->origin_x = x;
    flight->origin_y = y;
    Sc64NukeCadence cadence = race_nuke_cadence_at(y, y, &flight->ascent_ticks);
    /* An enemy launch can show its ascent while its remote target is unseen. */
    flight->strike = (Sc64NukeFlight){.x = 200, .y = y, .owner = ENEMY,
                                    .phase = SC64_NUKE_PAINTING, .cadence = cadence};
    race_nuke_active_flights = 1;
    SC64_GRID_SET(visible, y / 16 * MAP_COLS + x / 16, 1);
    u32 heap[SC64_NUKE_FLIGHT_LIMIT * SC64_NUKE_MAX_DRAW_PARTS];
    unsigned size = 0;
    sc64_depth_push_nuke_effects(heap, &size);
    assert(size == 1);
    assert_nuke_draw_key(sc64_depth_pop(heap, &size), SC64_NUKE_DRAW_MISSILE, 18);
    compare_nuke_paint(actor, 0, &sc64_nuke_missile_frames[0], x, y + 12, NULL);
    /* A synthetic raw-202 painter row isolates the next elevation without
     * assuming that this optional runtime exposes a playable Dark Swarm. */
    u[actor].raw_type = 202;
    assert(sc64_unit_world_layer(&u[actor]) == 19);
    compare_nuke_paint(actor, 1, &sc64_nuke_missile_frames[0], x, y + 12, NULL);
    u[actor].raw_type = 12;

    flight->strike.phase = SC64_NUKE_DESCENDING;
    flight->strike.x = x;
    flight->strike.age = cadence.paint + cadence.descent - SC64_NUKE_DAMAGE_LEAD - 1;
    sc64_depth_push_nuke_effects(heap, &size);
    assert(size == 1);
    assert_nuke_draw_key(sc64_depth_pop(heap, &size), SC64_NUKE_DRAW_MISSILE, 18);
    compare_nuke_paint(actor, 0, &sc64_nuke_missile_frames[1], x, y + 12, NULL);

    /* The attachment and independent child start together before damage.
     * After damage, its retained row preserves the same independent layers. */
    const unsigned tick = 4 * SC64_NUKE_BURST_SOURCE_TICKS;
    const Sc64NukeFrame *burst = &sc64_nuke_burst_frames[tick / SC64_NUKE_BURST_SOURCE_TICKS];
    const Sc64NukeFrame *child = &sc64_nuke_impact_frames[tick / SC64_NUKE_IMPACT_SOURCE_TICKS];
    for (unsigned phase = 0; phase < 2; ++phase) {
        flight->strike.age = cadence.paint + cadence.descent - SC64_NUKE_DAMAGE_LEAD + tick;
        if (phase) {
            flight->strike.phase = SC64_NUKE_IDLE;
            flight->impact_tick = tick + 1;
        }
        sc64_depth_push_nuke_effects(heap, &size);
        assert(size == 2);
        assert_nuke_draw_key(sc64_depth_pop(heap, &size), SC64_NUKE_DRAW_BURST, 18);
        assert_nuke_draw_key(sc64_depth_pop(heap, &size), SC64_NUKE_DRAW_IMPACT, 19);
        compare_nuke_paint(actor, 0, burst, x, y + 12, child);
    }
    /* Once the burst ends, the remaining layer-19 child still modifies an
     * actual layer-18 Wraith. This assertion avoids an equal-layer tie. */
    const unsigned child_tick = SC64_NUKE_BURST_FRAME_COUNT * SC64_NUKE_BURST_SOURCE_TICKS;
    flight->impact_tick = child_tick + 1;
    u[actor].alive = 0;
    int flyer = fixture_unit(SIDE, 8, x, y);
    assert(sc64_unit_world_layer(&u[flyer]) == 18);
    sc64_depth_push_nuke_effects(heap, &size);
    assert(size == 1);
    assert_nuke_draw_key(sc64_depth_pop(heap, &size), SC64_NUKE_DRAW_IMPACT, 19);
    compare_nuke_paint(flyer, 0, &sc64_nuke_impact_frames[child_tick / SC64_NUKE_IMPACT_SOURCE_TICKS],
                       x, y + 12 + SC64_NUKE_IMPACT_SOURCE_Y / 2, NULL);
    SC64_GRID_SET(visible, y / 16 * MAP_COLS + x / 16, 0);
    sc64_depth_push_nuke_effects(heap, &size);
    assert(size == 0);
    puts("nuclear missile depth: ascent and descent 18, burst 18, impact child 19, fog and HUD clear");
}
#endif

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
    /* Other fixtures test the complete training clock. Keep the real T8
     * trigger and campaign clocks synchronized while proving the ARM journal. */
    race_nuke_silos[silo].ammo.remaining = 1;
    for (int i = 0; i < 3 && !race_nuke_silos[silo].ammo.armed; ++i) game_update(0, 0);
#else
    advance_world(1876);
#endif
    assert(race_nuke_armed_slot(SIDE) == silo);
    assert(race_late_lift(cc));
    assert(b[silo].owner == NEUTRAL && race_nuke_armed_slot(SIDE) < 0 &&
           race_nuke_reserved_supply(SIDE) == 16);
    assert(race_late_flight_order(cc, x, y, 1));
#ifdef SC64_NUKE_SILO_JOURNAL
    game_update(0, 0);
#else
    ++frame; race_late_flight_tick();
#endif
    assert(b[silo].owner == SIDE && race_nuke_armed_slot(SIDE) == silo &&
           race_nuke_reserved_supply(SIDE) == 16);
    puts("T8: native roster, Command Center lands, claims neutral Silo and trains an allowed missile");
    /* Match the native target and the specific friendly Ghost that died. */
    assert(witness >= 0 && u[witness].alive && spotter >= 0 &&
           race_nuke_issue(spotter, SIDE, 3666 / 2, 3882 / 2));
#ifdef SC64_NUKE_ACTIVE_JOURNAL
    for (int i = 0; i < 25; ++i) game_update(0, 0);
    Sc64NukeWorldFlight *painting = race_nuke_track(spotter);
    assert(painting && painting->actor_slot_plus1);
    assert((unsigned)frame == race_campaign.elapsed_frames);
    int age = painting->strike.age, actor_slot = painting->actor_slot_plus1;
    assert(race_save_game());
    race_nuke_reset();
    assert(race_load_game());
    painting = race_nuke_track(spotter);
    assert(painting && painting->strike.age == age && painting->actor_slot_plus1 == actor_slot);
    puts("T8: painting strike round-trips through the cartridge SRAM journal");
#endif
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
    fixture_building(SIDE, 156, 160, 164);
    minerals = gas = 10000;
    for (int i = 0; i < 3; ++i) fixture_unit(SIDE, 0, 80, 80);
    assert(race_nuke_arm(silo, SIDE) == 2); /* Protoss supply cannot subsidize a Terran warhead. */
    for (int i = 0; i < 25; ++i) fixture_building(SIDE, 109, 160, 160);
    for (int i = 3; i < 107; ++i) fixture_unit(SIDE, 0, 80, 80);
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
    assert(race_supply_ledger_reserved(race_owner_supply_ledger(SIDE, R_TER)) == 400 &&
           race_supply_ledger_reserved(race_owner_supply_ledger(SIDE, R_PROTOSS)) == 0);
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
    assert(!race_save_game()); /* an unfinished approach cannot be dropped */
    assert(race_nuke_abort(ghost) && race_nuke_armed_slot(SIDE) == silo);
    nuke_action(NUKE_MENU_CAST);
    assert(mode == MODE_NUCLEAR_TARGET);
    cx = x; cy = y;
    game_update(KEY_A, KEY_A);
    assert(mode == MODE_NORMAL && race_nuke_ghost_locked(ghost));
    assert(!issue_order(ORDER_MOVE, x - 30, y));
#ifdef SC64_DIRECT_ATTACK
    assert(!sc64_direct_attack_issue_target(enemy));
#endif
    stop_selected();
    assert(u[ghost].order == ORDER_IDLE && race_nuke_ghost_locked(ghost));
#ifndef SC64_NUKE_ACTIVE_JOURNAL
    assert(!race_save_game()); /* never save a partial in-flight state */
#endif
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
    gfx_begin(0); gfx_set_clip_y(12, 120);
#ifdef SC64_WORLD_DEPTH
    sc64_draw_depth_ordered_actors();
#else
    race_nuke_draw();
#endif
    gfx_end();
    for (int i = 0; i < 80; ++i) { ++frame; race_nuke_tick(); }
    assert(!race_nuke_active_flights);
    puts("nuclear strike: stock add-on and Ghost production, GBA menu, paid ammo, abort and independent impact");
#if defined(GBA_HOST) && defined(SC64_WORLD_DEPTH)
    test_nuke_dot_depth();
    test_nuke_missile_depth();
#endif
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
