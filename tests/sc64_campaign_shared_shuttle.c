/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Paid campaign Shuttle production and ordinary boarding/save/unload path. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#ifndef SC64_SHARED_TRANSPORT
#error "campaign test requires shared transport"
#endif

static void test_mission(int mission)
{
    int robotics = -1, pylon = -1, ship = -1, passenger = -1;
    int facility = sc64_race_building_from_raw(155);
    int pylon_type = sc64_race_building_from_raw(156);
    int shuttle = sc64_race_unit_from_raw(69);
    assert(facility >= 0 && pylon_type >= 0 && shuttle >= 0);
    game_init(); race_mode = 0; race_selected_mission = mission; init_state();
    assert(screen == SCREEN_PLAY && !race_mode && race_human == R_PROTOSS);
    for (int row = 4; row < race_rows - 5 && robotics < 0; ++row)
        for (int col = 4; col < race_cols - 8 && robotics < 0; ++col) {
            int x = col * 16 + 8, y = row * 16 + 8;
            if (!can_place(facility, x, y) || !can_place(pylon_type, x + 48, y + 8)) continue;
            robotics = spawn_build(SIDE, facility, x, y, 0);
            if (robotics >= 0) pylon = spawn_build(SIDE, pylon_type, x + 48, y + 8, 0);
        }
    assert(robotics >= 0 && pylon >= 0);
    minerals = gas = 1000;
    assert(race_queue_unit(SIDE, robotics, shuttle));
    assert(minerals == 800 && gas == 1000);
    for (int tick = 0; tick < 1800 && b[robotics].queue_count; ++tick)
        update_production();
    assert(!b[robotics].queue_count);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
        if (u[i].type == shuttle && ship < 0) ship = i;
        if (u[i].type == U_ZEALOT && passenger < 0) passenger = i;
    }
    assert(ship >= 0 && passenger >= 0 && race_transport_carrier_valid(ship));
    u[passenger].x = u[ship].x; u[passenger].y = u[ship].y;
    clear_selection(); u[passenger].selected = 1; selected_building = -1;
    cx = u[ship].x; cy = u[ship].y; mode = MODE_NORMAL; interact();
    assert(u[passenger].order == ORDER_BOARD && race_transport_tick_unit(passenger));
    assert(u[passenger].order == ORDER_TRANSPORTED);
    assert(race_save_game() && race_load_game());
    assert(u[passenger].order == ORDER_TRANSPORTED && u[passenger].target == ship);
    clear_selection(); u[ship].selected = 1; selected_building = -1;
    assert(race_shared_campaign_transport_menu_available());
    choose_command(MENU_STOP + 1); assert(mode == MODE_UNLOAD);
    assert(race_transport_request_unload(ship, u[ship].x, u[ship].y));
    for (int y = 1; y < race_rows - 1 && u[passenger].order == ORDER_TRANSPORTED; ++y)
        for (int x = 1; x < race_cols - 1 && u[passenger].order == ORDER_TRANSPORTED; ++x) {
            if (!is_traversable(x, y, -1)) continue;
            u[ship].x = u[ship].tx = x * 16 + 8; u[ship].y = u[ship].ty = y * 16 + 8;
            race_transport_try_unload(ship);
        }
    assert(u[passenger].order == ORDER_IDLE);
    assert(race_save_game() && race_load_game());
    assert(u[passenger].alive && u[passenger].order == ORDER_IDLE);
}
int main(void)
{
    gba_init(); test_mission(SC64_P7_GLOBAL_ID);
#ifdef SC64_P8_RUNTIME
    test_mission(SC64_P8_GLOBAL_ID);
#endif
    puts("paid campaign Shuttle boarding/save/unload: ok");
    return 0;
}
