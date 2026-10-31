/* SPDX-License-Identifier: GPL-3.0-or-later */
/* P5 mission transport: normal source carrier death with a pending boarding
 * order, ordinary immediate save/load, and carrier-slot reuse. */
#define main sc64_p5_runtime_baseline_main
#include "sc64_p5_runtime.c"
#undef main

int main(void)
{
    int carrier = -1, passenger, replacement;
    gba_init(); game_init(); prior_keys = 0;
    select_protoss_mission(SC64_P5_GLOBAL_ID);
    assert(race_p5_active() && !race_mode);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_P5_SHUTTLE &&
            u[i].raw_type == 69) { carrier = i; break; }
    assert(carrier >= 0 && race_transport_carrier_valid(carrier));
    passenger = spawn_unit(SIDE, U_ZEALOT, u[carrier].x + 48, u[carrier].y);
    assert(passenger >= 0 && u[passenger].raw_type == 65 && u[passenger].raw_owner == 7);
    assert(race_transport_request_board(passenger, carrier));
    assert(u[passenger].order == ORDER_BOARD);
    u[carrier].hp = 1; u[carrier].shield = 0; invincible_units[carrier] = 0;
    hit_target(carrier, 10000, U_ZEALOT, ENEMY);
    assert(!u[carrier].alive && u[passenger].alive &&
           u[passenger].order == ORDER_IDLE && u[passenger].target == -1);
    assert(race_save_game() && race_load_game());
    assert(u[passenger].alive && u[passenger].order == ORDER_IDLE &&
           u[passenger].target == -1);
    assert(find_unit_slot() == carrier);
    replacement = spawn_unit(SIDE, U_P5_SHUTTLE, u[passenger].x, u[passenger].y);
    assert(replacement == carrier && !race_transport_tick_unit(passenger));
    assert(u[passenger].order == ORDER_IDLE && u[passenger].target == -1 &&
           race_transport_cargo_space(replacement) == 0);
    puts("P5 pending transport order canceled on carrier death: ok");
    return 0;
}
