/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Reuse only positive T5 fixture setup and paths; the renamed original main
 * and its malformed-save case are never called. */
#define main sc64_t5_original_transport_main
#include "sc64_transport_engine.c"
#undef main

static void test_ordinary_t5_save_load(void)
{
    int x, y;
    start_t5();
    assert(find_open_horizontal_run(3, &x, &y));
    int ship = fixture_unit(11, SIDE, 0, x, y, 750);
    int passenger = fixture_unit(16, SIDE, 0, x + 8, y, 250);
    request_board_and_advance(passenger, ship);
    assert(race_save_game());
    assert(race_load_game());
    assert(SC64_TRANSPORT_LOADED(&u[passenger]) && u[passenger].target == ship);
    clear_selection(); u[ship].selected = 1; selected_building = -1;
    assert(field_menu_count() > MENU_UNLOAD);
    assert(!strcmp(race_menu_label(MENU_UNLOAD), "UNLOAD CARGO"));
    choose_command(MENU_UNLOAD); assert(mode == MODE_UNLOAD);
}
int main(void)
{
    test_boarding_capacity_and_unobservable_cargo();
    test_blocked_then_successful_unload();
    test_unload_with_carrier_after_passenger_slot();
    test_ordinary_t5_save_load();
    test_transport_death_kills_cargo_before_t5_query();
    puts("T5 positive shared transport fixtures passed");
    return 0;
}
