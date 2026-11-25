/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Campaign research, boarding, and ordinary save/load through shared transport. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#include "sc64_save_candidate_fixture.h"
#ifndef SC64_SHARED_TRANSPORT
#error "campaign Ventral Sacs test requires shared transport"
#endif
#ifndef TEST_CAMPAIGN_MISSION
#define TEST_CAMPAIGN_MISSION 18
#endif

int main(void)
{
    int lair = -1, overlord = -1, passenger = -1;
#if TEST_CAMPAIGN_MISSION == 15
    unsigned char before[MAX_UNITS] = {0};
#endif
    gba_init();
    for (int mission = 0; mission < 40; ++mission)
        assert(!!sc64_campaign_transport_allowed(mission) ==
               (mission == 15 || (mission >= 18 && mission <= 21)));
    game_init(); race_mode = 0; race_selected_mission = TEST_CAMPAIGN_MISSION; init_state();
    assert(screen == SCREEN_PLAY && !race_mode && race_human == R_ZERG);
    /* Seed the prerequisite and Hatchery where the campaign map permits.
     * The morph and research below use normal paid command handlers. */
    for (int row = 4; row < race_rows - 5 && lair < 0; ++row)
        for (int col = 4; col < race_cols - 8 && lair < 0; ++col) {
            int x = col * 16 + 8, y = row * 16 + 8;
            if (!can_place(B_HATCHERY, x, y) ||
                !can_place(B_SPAWNING_POOL, x + 48, y + 8)) continue;
            lair = spawn_build(SIDE, B_HATCHERY, x, y, 0);
            if (lair >= 0) assert(spawn_build(SIDE, B_SPAWNING_POOL, x + 48, y + 8, 0) >= 0);
        }
    assert(lair >= 0);
    /* Use the campaign command to pay for and begin the actual morph. */
    minerals = gas = 5000;
    selected_building = lair;
    choose_command(MENU_ZP_LAIR);
    assert(b[lair].type == B_LAIR && b[lair].under_construction &&
           b[lair].raw_type == sc64_race_raw_building(B_LAIR));
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(B_LAIR);
    assert(rule && rule->build_frames > 1);
    b[lair].build_progress = (rule->build_frames * 5 + 3) / 4 - 1;
    race_tick_buildings();
    assert(!b[lair].under_construction);
#if TEST_CAMPAIGN_MISSION == 15
    for (int i = 0; i < MAX_UNITS; ++i) {
        before[i] = u[i].alive;
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_DRONE && passenger < 0)
            passenger = i;
    }
    /* Queue through the live producer and wait for its normal completion. */
    assert(race_queue_unit(SIDE, lair, U_OVERLORD));
    for (int tick = 0; tick < 5000 && overlord < 0; ++tick) {
        update_production(); update_orders();
        for (int i = 0; i < MAX_UNITS; ++i)
            if (!before[i] && u[i].alive && u[i].owner == SIDE && u[i].type == U_OVERLORD) {
                overlord = i; break;
            }
    }
    assert(overlord >= 0 && u[overlord].raw_owner == 3);
#else
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
        if (u[i].type == U_OVERLORD && overlord < 0) overlord = i;
        if (u[i].type == U_DRONE && passenger < 0) passenger = i;
    }
#endif
    assert(overlord >= 0 && passenger >= 0);
    assert(!race_transport_carrier_type(&u[overlord]));
    assert(!race_transport_request_board(passenger, overlord));
    minerals = gas = 1000;
    selected_building = lair;
    assert(race_sk_transport_research_menu());
    choose_command(MENU_STOP + 1);
    assert(minerals == 800 && gas == 800);
    assert(race_skirmish_transport_tech.owner[0].remaining == 3000);
    for (int i = 0; i < 60; ++i) update_production();
    assert(race_save_game());
    memset(&race_skirmish_transport_tech, 0, sizeof(race_skirmish_transport_tech));
    assert(race_load_game());
    assert(race_skirmish_transport_tech.owner[0].remaining == 2940);
    for (int i = 0; i < 2940; ++i) update_production();
    assert(race_skirmish_transport_tech.owner[0].researched);
    assert(race_transport_carrier_type(&u[overlord]));
    u[passenger].x = u[overlord].x; u[passenger].y = u[overlord].y;
    clear_selection(); u[passenger].selected = 1; selected_building = -1;
    cx = u[overlord].x; cy = u[overlord].y; mode = MODE_NORMAL; interact();
    assert(u[passenger].order == ORDER_BOARD && race_transport_tick_unit(passenger));
    assert(u[passenger].order == ORDER_TRANSPORTED);
    assert(race_save_game() && race_load_game());
    assert(race_skirmish_transport_tech.owner[0].researched);
    assert(u[passenger].order == ORDER_TRANSPORTED && u[passenger].target == overlord);
    /* Validators must use detached candidate research, not the live state.
     * A loaded Overlord cannot borrow live research or accept malformed jobs. */
    assert(race_sk_transport_candidate_valid(&race_data));
    assert(race_skirmish_saved_transport(&race_data, &u[overlord]));
    Sc64SkTransportTech valid_candidate = race_skirmish_transport_candidate;
    race_skirmish_transport_candidate.owner[0].researched = 0;
    assert(!race_skirmish_saved_transport(&race_data, &u[overlord]));
    race_skirmish_transport_candidate = valid_candidate;
    race_skirmish_transport_candidate.owner[0].researched = 2;
    assert(!race_sk_transport_candidate_valid(&race_data));
    race_skirmish_transport_candidate = valid_candidate;
    race_skirmish_transport_candidate.owner[1].researched = 1;
    assert(!race_sk_transport_candidate_valid(&race_data));
    race_skirmish_transport_candidate = valid_candidate;
    int invalid_producer = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type != B_LAIR && b[i].type != B_HIVE) {
            invalid_producer = i; break;
        }
    assert(invalid_producer >= 0);
    race_skirmish_transport_candidate.owner[0].researched = 0;
    race_skirmish_transport_candidate.owner[0].remaining = 1;
    race_skirmish_transport_candidate.owner[0].producer_plus_one = invalid_producer + 1;
    assert(!race_sk_transport_candidate_valid(&race_data));
    race_skirmish_transport_candidate = valid_candidate;
    /* Older Z4-created actors can carry the unassigned source-owner marker. */
    if (TEST_CAMPAIGN_MISSION == 15) {
        u[overlord].raw_owner = -1;
        assert(race_transport_carrier_type(&u[overlord]));
        assert(race_save_game() && race_load_game());
        assert(u[overlord].raw_owner == -1 &&
               u[passenger].order == ORDER_TRANSPORTED && u[passenger].target == overlord);
    }
    /* Reject a checksum-valid candidate whose loaded Overlord has no
     * candidate research. The live game must remain unchanged. */
    static RaceSave loaded_candidate;
    fixture_capture_save(&loaded_candidate);
    assert(race_skirmish_transport_save_valid(&race_data, race_cols, race_rows));
    Unit passenger_before = u[passenger], carrier_before = u[overlord];
    Sc64SkTransportTech live_tech_before = race_skirmish_transport_tech;
    race_skirmish_transport_candidate.owner[0].researched = 0;
    write_candidate_record(&loaded_candidate);
    assert(!race_load_game());
    assert(!memcmp(&u[passenger], &passenger_before, sizeof(Unit)) &&
           !memcmp(&u[overlord], &carrier_before, sizeof(Unit)) &&
           !memcmp(&race_skirmish_transport_tech, &live_tech_before, sizeof(live_tech_before)));
    puts("campaign Ventral Sacs research/boarding/save: ok");
    return 0;
}
