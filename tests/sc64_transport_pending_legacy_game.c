/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Default-off custom-game carrier loss: ordinary pending order, save/load and
 * same-slot reuse. Run only in profiles that already support Dropships. */
#define main sc64_resume_baseline_main
#include "sc64_skirmish_resume.c"
#undef main
#ifdef SC64_SHARED_TRANSPORT
#error "This fixture checks the existing transport runtime without the staged feature"
#endif

static void pending_loss(int map)
{
    int ship, passenger, replacement;
    launch(map, R_TER, 1, -1);
    assert(sc64_race_unit_from_raw(11) == U_DROPSHIP);
    ship = spawn_unit(SIDE, U_DROPSHIP, race_base_x[0], race_base_y[0]);
    passenger = spawn_unit(SIDE, U_MARINE, race_base_x[0] + 48, race_base_y[0]);
    assert(ship >= 0 && passenger >= 0);
    assert(dist2(u[ship].x,u[ship].y,u[passenger].x,u[passenger].y) >
           SC64_TRANSPORT_BOARD_RADIUS * SC64_TRANSPORT_BOARD_RADIUS);
    assert(race_transport_request_board(passenger, ship));
    hit_target(ship, 10000, U_MARINE, ENEMY);
    assert(!u[ship].alive && u[passenger].alive &&
           u[passenger].order == ORDER_IDLE && u[passenger].target == -1 &&
           !u[passenger].path_timer && !u[passenger].path_retry);
    assert(race_save_game() && race_load_game());
    assert(u[passenger].alive && u[passenger].order == ORDER_IDLE &&
           u[passenger].target == -1);
    replacement = spawn_unit(SIDE, U_DROPSHIP, u[passenger].x, u[passenger].y);
    assert(replacement == ship && !race_transport_tick_unit(passenger));
    assert(u[passenger].order == ORDER_IDLE && u[passenger].target == -1 &&
           race_transport_cargo_space(replacement) == 0);
    assert(race_save_game() && race_load_game());
    assert(u[passenger].alive && u[passenger].order == ORDER_IDLE &&
           u[passenger].target == -1);
}
int main(void)
{
    gba_init();
    for (int map = 0; map <= SC64_SKIRMISH_IMPORTED_COUNT; ++map) pending_loss(map);
    puts("legacy custom transport pending-order carrier loss: ok");
    return 0;
}
