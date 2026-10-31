/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Z6 source actors with a seeded valid researched upgrade: carrier death,
 * ordinary immediate save/load, and carrier-slot reuse. */
#include <stdio.h>
#define main sc64_z6_runtime_baseline_main
#include "sc64_z6_runtime.c"
#undef main

static u16 held_before;
static void tap_key(u16 key)
{
    game_update(key, key & (u16)~held_before); held_before = key;
    game_update(0, 0); held_before = 0;
}
int main(void)
{
    int ordinal, carrier = -1, passenger = -1, replacement;
    gba_init(); game_init(); held_before = 0;
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG, SC64_Z6_GLOBAL_ID);
    assert(ordinal == 5);
    tap_key(KEY_A);
    for (int page = 0; page < ordinal / 2; ++page) tap_key(KEY_RIGHT);
    if (ordinal & 1) tap_key(KEY_DOWN);
    tap_key(KEY_A); tap_key(KEY_START);
    assert(screen == SCREEN_PLAY && race_z6_active() && !race_mode);
    /* A completed level is valid even if its Lair was subsequently lost. */
    race_z6.transport.researched = 1;
    race_z6.transport.ticks_remaining = 0;
    race_z6.transport.producer_slot_plus1 = 0;
    assert(race_z6_transport_state_valid() && race_z6_transport_overlord_ready());
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE && u[i].raw_owner == 3) {
        if (u[i].type == U_OVERLORD && u[i].raw_type == 42 && carrier < 0) carrier = i;
        if (u[i].type == U_DRONE && u[i].raw_type == 41 && passenger < 0) passenger = i;
    }
    assert(carrier >= 0 && passenger >= 0 && race_transport_carrier_valid(carrier));
    u[passenger].x = u[carrier].x + 48; u[passenger].y = u[carrier].y;
    assert(race_transport_request_board(passenger, carrier));
    u[carrier].hp = 1; invincible_units[carrier] = 0;
    hit_target(carrier, 10000, U_ZERGLING, ENEMY);
    assert(!u[carrier].alive && u[passenger].alive &&
           u[passenger].order == ORDER_IDLE && u[passenger].target == -1);
    assert(race_save_game() && race_load_game());
    assert(u[passenger].alive && u[passenger].order == ORDER_IDLE &&
           u[passenger].target == -1);
    assert(find_unit_slot() == carrier);
    replacement = spawn_unit(SIDE, U_OVERLORD, u[passenger].x, u[passenger].y);
    assert(replacement == carrier && !race_transport_tick_unit(passenger));
    assert(u[passenger].order == ORDER_IDLE && u[passenger].target == -1 &&
           race_transport_cargo_space(replacement) == 0);
    /* Execute the actual source trigger 7 / action 11 Kill Unit At Location.
     * Place the replacement inside its location, with a pending passenger. */
    {
        const Sc64Z6SourceAction *kill = NULL;
        int l, t, r, btm;
        for (unsigned i = 0; i < SC64_Z6_SOURCE_ACTION_COUNT; ++i)
            if (sc64_z6_source_actions[i].trigger == 7 &&
                sc64_z6_source_actions[i].index == 11) kill = &sc64_z6_source_actions[i];
        assert(kill && kill->opcode == 23 && kill->unit == 42 && kill->group1 == 18);
        assert(sc64_z6_world_location(&race_z6_world, kill->location, &l, &t, &r, &btm));
        u[replacement].raw_type = 42; u[replacement].raw_owner = 3;
        u[replacement].x = (l + r) / 2; u[replacement].y = (t + btm) / 2;
        u[passenger].x = u[replacement].x + 48; u[passenger].y = u[replacement].y;
        assert(race_transport_request_board(passenger, replacement));
        assert(race_z6_runtime_action(&race_z6_world, kill));
        assert(!u[replacement].alive && u[passenger].alive &&
               u[passenger].order == ORDER_IDLE && u[passenger].target == -1);
        assert(race_save_game() && race_load_game());
        assert(u[passenger].alive && u[passenger].order == ORDER_IDLE &&
               u[passenger].target == -1);
    }
    puts("Z6 pending transport order canceled on carrier death: ok");
    return 0;
}
