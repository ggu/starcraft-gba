/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exercises the real custom-game runtime and ordinary save/load path. */
#define main sc64_resume_baseline_main
#include "sc64_skirmish_resume.c"
#undef main

#ifndef SC64_SHARED_TRANSPORT
#error "compile with the staged shared transport capability"
#endif

/* An ordinary pause-menu save must use the new envelope tag. This runs first
 * in a fresh host process, before any journal record has been written. */
static void check_pause_save_namespace(void)
{
    static unsigned char saved[16384];
    u32 length = 0;
    launch(0, R_TER, 1, -1);
    tap(KEY_START); assert(screen == SCREEN_PAUSE);
    input(KEY_B);
    assert((pause_notice & 0xc0) == UI_NOTICE_SAVED);
    assert(save_read_latest(saved, sizeof(saved), SC64_TRANSPORT_SAVE_VERSION, &length));
    assert(length > 0);
    assert(!save_read_latest(saved, sizeof(saved), SAVE_VERSION, &length));
    assert(race_load_game());
}

static void check_jobs(int map)
{
    static const int owners[4] = { SIDE, ENEMY, RESCUABLE, 4 };
    int lair[4];
    launch(map, R_ZERG, 3, R_ZERG);
    for (int i = 0; i < 4; ++i) {
        lair[i] = spawn_build(owners[i], B_LAIR,
                              race_base_x[i] + 60, race_base_y[i] + 60, 0);
        assert(lair[i] >= 0);
        if (i == 0) { minerals = 1000; gas = 1000; }
        else { race_minerals[owners[i]] = 1000; race_gas[owners[i]] = 1000; }
        if (i == 0) {
            for (int j = 0; j < MAX_UNITS; ++j)
                if (u[j].alive && u[j].owner == SIDE && u[j].type == U_OVERLORD)
                    assert(!race_transport_carrier_type(&u[j]));
            selected_building = lair[i];
            assert(race_sk_transport_research_menu());
            choose_command(race_context_command_base(RACE_CONTEXT_TRANSPORT_TECH));
            assert(race_skirmish_transport_tech.owner[0].remaining == 3000);
            assert(minerals == 800 && gas == 800);
            selected_building = -1;
        } else assert(race_sk_transport_start_owner(owners[i], lair[i]));
    }
    for (int i = 0; i < 60; ++i) update_production();
    assert(race_save_game());
    memset(&race_skirmish_transport_tech, 0, sizeof(race_skirmish_transport_tech));
    assert(race_load_game());
    for (int i = 0; i < 4; ++i) {
        assert(race_skirmish_transport_tech.owner[i].remaining == 2940);
        assert(race_skirmish_transport_tech.owner[i].producer_plus_one == lair[i] + 1);
    }
    for (int i = 0; i < 2940; ++i) update_production();
    for (int i = 0; i < 4; ++i) assert(race_skirmish_transport_tech.owner[i].researched);
    assert(race_save_game());
    assert(race_load_game());
    for (int i = 0; i < 4; ++i) assert(race_skirmish_transport_tech.owner[i].researched);
    {
        int overlord = -1, drone = -1;
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
            if (u[i].type == U_OVERLORD && overlord < 0) overlord = i;
            if (u[i].type == U_DRONE && drone < 0) drone = i;
        }
        assert(overlord >= 0 && drone >= 0);
        /* The fixture seeds the starting proximity; the A-action and boarding
         * order use the ordinary player-control and unit-order paths. */
        u[drone].x = u[overlord].x; u[drone].y = u[overlord].y;
        clear_selection(); u[drone].selected = 1; selected_building = -1;
        cx = u[overlord].x; cy = u[overlord].y; mode = MODE_NORMAL;
        interact();
        assert(u[drone].order == ORDER_BOARD && u[drone].target == overlord);
        assert(race_transport_tick_unit(drone));
        assert(u[drone].order == ORDER_TRANSPORTED);
        assert(race_save_game() && race_load_game());
        assert(u[drone].order == ORDER_TRANSPORTED && u[drone].target == overlord);
    }
}

static void check_refunds(int map)
{
    int lair;
    launch(map, R_ZERG, 1, R_ZERG);
    race_minerals[ENEMY] = 1000; race_gas[ENEMY] = 1000;
    lair = spawn_build(ENEMY, B_LAIR, race_base_x[1], race_base_y[1], 0);
    assert(lair >= 0 && race_sk_transport_start_owner(ENEMY, lair));
    destroy_building(lair, SIDE);
    assert(!race_skirmish_transport_tech.owner[1].remaining);
    assert(race_minerals[ENEMY] == 950 && race_gas[ENEMY] == 950);
    race_minerals[ENEMY] = 1000; race_gas[ENEMY] = 1000;
    lair = spawn_build(ENEMY, B_LAIR, race_base_x[1], race_base_y[1], 0);
    assert(lair >= 0 && race_sk_transport_start_owner(ENEMY, lair));
    b[lair].owner = SIDE; /* Simulate a live ownership transfer. */
    race_sk_transport_research_tick();
    assert(!race_skirmish_transport_tech.owner[1].remaining);
    assert(race_minerals[ENEMY] == 1000 && race_gas[ENEMY] == 1000);
}

static void check_cargo(int map, int human, int bots)
{
    static const int raw_carriers[3] = {11, 42, 69};
    static const int passengers[3] = {U_MARINE, U_ZERGLING, U_ZEALOT};
    int type, ship, passenger;
    launch(map, human, bots, -1);
    if (human == R_ZERG) race_skirmish_transport_tech.owner[0].researched = 1;
    type = sc64_race_unit_from_raw(raw_carriers[human]);
    assert(type >= 0);
    ship = spawn_unit(SIDE, type, race_base_x[0], race_base_y[0]);
    passenger = spawn_unit(SIDE, passengers[human], race_base_x[0], race_base_y[0]);
    assert(ship >= 0 && passenger >= 0 && race_transport_carrier_valid(ship));
    assert(race_transport_request_board(passenger, ship));
    assert(race_save_game());
    assert(race_load_game());
    assert(u[passenger].order == ORDER_BOARD && u[passenger].target == ship);
    assert(race_transport_tick_unit(passenger));
    assert(u[passenger].order == ORDER_TRANSPORTED);
    assert(race_save_game());
    assert(race_load_game());
    assert(u[passenger].order == ORDER_TRANSPORTED && u[passenger].target == ship);
    assert(race_transport_cargo_space(ship) > 0);
    assert(race_transport_request_unload(ship, u[ship].x, u[ship].y));
    assert(race_save_game());
    assert(race_load_game());
    assert(u[ship].order == ORDER_UNLOAD);
    for (int y = 1; y < race_rows - 1 && u[passenger].order == ORDER_TRANSPORTED; ++y)
        for (int x = 1; x < race_cols - 1 && u[passenger].order == ORDER_TRANSPORTED; ++x) {
            if (!is_traversable(x, y, -1)) continue;
            u[ship].x = u[ship].tx = x * 16 + 8;
            u[ship].y = u[ship].ty = y * 16 + 8;
            race_transport_try_unload(ship);
        }
    assert(u[passenger].order == ORDER_IDLE);
    assert(race_save_game());
    assert(race_load_game());
    assert(u[passenger].order == ORDER_IDLE);
    assert(race_transport_request_board(passenger, ship));
    u[ship].x = u[passenger].x; u[ship].y = u[passenger].y;
    assert(race_transport_tick_unit(passenger));
    hit_target(ship, 10000, U_MARINE, ENEMY);
    assert(!u[ship].alive && !u[passenger].alive);
    assert(race_save_game());
    assert(race_load_game());
    assert(!u[ship].alive && !u[passenger].alive);
}

static void check_pending_carrier_loss(int map, int human)
{
    static const int raws[3] = {11, 42, 69};
    static const int passengers[3] = {U_MARINE, U_ZERGLING, U_ZEALOT};
    int carrier, passenger, replacement, type;
    launch(map, human, 1, -1);
    if (human == R_ZERG) race_skirmish_transport_tech.owner[0].researched = 1;
    type = sc64_race_unit_from_raw(raws[human]);
    assert(type >= 0);
    carrier = spawn_unit(SIDE, type, race_base_x[0], race_base_y[0]);
    passenger = spawn_unit(SIDE, passengers[human], race_base_x[0] + 48, race_base_y[0]);
    assert(carrier >= 0 && passenger >= 0);
    assert(dist2(u[carrier].x, u[carrier].y, u[passenger].x, u[passenger].y) >
           SC64_TRANSPORT_BOARD_RADIUS * SC64_TRANSPORT_BOARD_RADIUS);
    assert(race_transport_request_board(passenger, carrier));
    assert(u[passenger].order == ORDER_BOARD);
    hit_target(carrier, 10000, U_MARINE, ENEMY);
    assert(!u[carrier].alive && u[passenger].alive);
    assert(u[passenger].order == ORDER_IDLE && u[passenger].target == -1 &&
           !u[passenger].path_timer && !u[passenger].path_retry);
    assert(race_save_game() && race_load_game());
    assert(u[passenger].alive && u[passenger].order == ORDER_IDLE &&
           u[passenger].target == -1);
    replacement = spawn_unit(SIDE, type, u[passenger].x, u[passenger].y);
    assert(replacement == carrier);
    assert(!race_transport_tick_unit(passenger));
    assert(u[passenger].order == ORDER_IDLE && u[passenger].target == -1 &&
           race_transport_cargo_space(replacement) == 0);
    assert(race_save_game() && race_load_game());
    assert(u[passenger].alive && u[passenger].order == ORDER_IDLE &&
           u[passenger].target == -1);
}

static void check_paid_production(int map, int raw)
{
    int race = raw == 11 ? R_TER : R_PROTOSS;
    int producer_raw = raw == 11 ? 114 : 155;
    int addon_raw = raw == 11 ? 115 : 156;
    int producer_type = sc64_race_building_from_raw(producer_raw);
    int addon_type = sc64_race_building_from_raw(addon_raw);
    int unit_type = sc64_race_unit_from_raw(raw);
    int producer = -1, addon = -1, x = 0, y = 0;
    launch(map, race, 1, -1);
    assert(producer_type >= 0 && addon_type >= 0 && unit_type >= 0);
    for (int row = 4; row < race_rows - 5 && producer < 0; ++row)
        for (int col = 4; col < race_cols - 8 && producer < 0; ++col) {
            x = col * 16 + 8; y = row * 16 + 8;
            if (!is_traversable(col, row, -1) ||
                !is_traversable(col - 2, row, -1) ||
                !can_place(producer_type, x, y) ||
                !can_place(addon_type, x + 48, y + 8)) continue;
            producer = spawn_build(SIDE, producer_type, x, y, 0);
        }
    assert(producer >= 0);
    addon = spawn_build(SIDE, addon_type, x + 48, y + 8, 0);
    assert(addon >= 0);
    minerals = gas = 1000;
    assert(race_queue_unit(SIDE, producer, unit_type));
    assert(minerals == (raw == 11 ? 900 : 800));
    assert(gas == (raw == 11 ? 900 : 1000));
    for (int tick = 0; tick < 1800 && b[producer].queue_count; ++tick)
        update_production();
    assert(!b[producer].queue_count);
    int found = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == unit_type) found = 1;
    assert(found);
}

static void check_capacity_and_owner(void)
{
    int ship, a, b, excess, hostile;
    launch(0, R_PROTOSS, 1, R_ZERG);
    ship = spawn_unit(SIDE, sc64_race_unit_from_raw(69), 120, 64);
    a = spawn_unit(SIDE, U_DRAGOON, 120, 64);
    b = spawn_unit(SIDE, U_DRAGOON, 120, 64);
    excess = spawn_unit(SIDE, U_ZEALOT, 120, 64);
    hostile = spawn_unit(ENEMY, U_ZERGLING, 120, 64);
    assert(ship >= 0 && a >= 0 && b >= 0 && excess >= 0 && hostile >= 0);
    assert(!race_transport_request_board(hostile, ship));
    assert(race_transport_request_board(a, ship) && race_transport_tick_unit(a));
    assert(race_transport_request_board(b, ship) && race_transport_tick_unit(b));
    assert(race_transport_cargo_space(ship) == 8);
    assert(!race_transport_request_board(excess, ship));
}

static void check_source_art(void)
{
    static const int raws[] = {11, 42, 69};
    launch(0, R_TER, 1, -1);
    camx = camy = 0;
    memset(visible, 0xff, sizeof(visible));
    for (unsigned i = 0; i < sizeof(raws)/sizeof(raws[0]); ++i) {
        int type = sc64_race_unit_from_raw(raws[i]);
        assert(type >= 0 && sc64_race_unit_flying(type));
        const Sc64SpriteFrame *source = sc64_p8_unit_sprite_for_raw((unsigned short)raws[i]);
        assert(source && mission_unit_sprite(type) == source);
#if defined(SC64_UNIT_WORD_SPRITES) && defined(SC64_RACE_UNIT_WORD_SPRITE_METADATA)
        assert(sc64_race_unit_words_sprite((unsigned short)raws[i], source));
#endif
        int slot = spawn_unit(SIDE, type, 120, 64);
        assert(slot >= 0);
        for (int explicit_raw = 0; explicit_raw < 2; ++explicit_raw) {
            unsigned painted = 0;
            u[slot].raw_type = explicit_raw ? raws[i] : -1;
            u[slot].selected = 0;
            gfx_begin(0);
            draw_unit(&u[slot]);
            for (int y = 20; y < 120; ++y)
                for (int x = 70; x < 170; ++x) painted |= gfx_test_pixel(x,y);
            assert(painted);
        }
        u[slot].alive = 0;
    }
}

int main(void)
{
    gba_init();
    check_pause_save_namespace();
    check_source_art();
    check_capacity_and_owner();
    for (int map = 0; map <= SC64_SKIRMISH_IMPORTED_COUNT; ++map) {
        check_jobs(map);
        check_refunds(map);
        check_paid_production(map, 11);
        check_paid_production(map, 69);
        for (int race = 0; race < 3; ++race) {
            check_pending_carrier_loss(map, race);
            for (int bots = 1; bots <= 3; ++bots) check_cargo(map, race, bots);
        }
    }
    puts("shared custom transport research/save: ok");
    return 0;
}
