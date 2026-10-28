/* Host save-payload regression for the optional Terran T3 profile.
 * Build with SC64_UNIFIED, SC64_T3_ENABLED, and SC64_CAMPAIGN_RACE=0. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u16 previous;

static void frame_input(u16 held) {
    game_update(held, held & (u16)~previous);
    previous = held;
}

static void press(u16 key) {
    frame_input(key);
    frame_input(0);
}

static void start_t3_through_menu(void) {
    game_init();
    previous = 0;
    press(KEY_A); /* HOME -> Terran mission list in this profile. */
    assert(race_front.page == SC_FRONT_MISSION);
    press(KEY_RIGHT); /* Terran ordinal 2 is mission page 1, row 0. */
    assert(race_front.mission_page == 1);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 6);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && race_selected_mission == 6);
    assert(sc64_t3_state_valid(&race_t3));
    frame_input(0); /* Establish one real T3 simulation tick for timer checks. */
    assert(race_t3.elapsed_ticks == race_campaign.elapsed_frames &&
           race_t3.elapsed_ticks == (u32)frame);
    assert(mission_unsupported_count == 0);
}

static int alive_units(void) {
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive != 0;
    return n;
}

static int alive_buildings(void) {
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) n += b[i].alive != 0;
    return n;
}

static void fill_unused_unit_slots(void) {
    int start = alive_units();
    assert(start == 138);
    for (int i = start; i < MAX_UNITS; ++i) {
        int n = spawn_unit(SIDE, U_MARINE, 24 + (i % 12) * 8,
                           24 + ((i - start) / 12) * 8);
        assert(n >= 0);
    }
    assert(alive_units() == MAX_UNITS);
    assert(alive_buildings() == 31);
}

static void save_round_trip(void) {
    Sc64T3State expected = race_t3;
    u16 expected_kills = race_t3_buildings_killed;
    u32 expected_frame = (u32)frame;
    int expected_first_x = u[0].x;
    assert(race_save_game());
    race_t3.remaining_ticks--;
    race_t3.elapsed_ticks++;
    race_t3_buildings_killed++;
    frame++;
    u[0].x++;
    assert(race_load_game());
    assert(memcmp(&race_t3, &expected, sizeof(expected)) == 0);
    assert(race_t3_buildings_killed == expected_kills);
    assert((u32)frame == expected_frame && u[0].x == expected_first_x);
    assert(alive_units() == MAX_UNITS && alive_buildings() == 31);
}

static void corrupt_t3_payload_is_atomic(void) {
    int saved_frame = frame, saved_x = u[0].x;
    Sc64T3State saved_t3 = race_t3;
    u16 saved_kills = race_t3_buildings_killed;

    assert(race_save_game());
    race_data.t3.remaining_ticks++;
    save_write(&race_data, sizeof(race_data), SAVE_VERSION);
    assert(!race_load_game());
    assert(frame == saved_frame && u[0].x == saved_x);
    assert(memcmp(&race_t3, &saved_t3, sizeof(saved_t3)) == 0);
    assert(race_t3_buildings_killed == saved_kills);

    /* A valid alternate-objective state still requires evidence of a kill. */
    assert(race_save_game());
    race_data.t3.objective = 1;
    race_data.t3.fired |= SC64_T3_F_ALTERNATE | SC64_T3_F_SWITCH_1;
    race_data.t3_buildings_killed = 0;
    save_write(&race_data, sizeof(race_data), SAVE_VERSION);
    assert(sc64_t3_state_valid(&race_data.t3));
    assert(!race_load_game());
    assert(frame == saved_frame && u[0].x == saved_x);
    assert(memcmp(&race_t3, &saved_t3, sizeof(saved_t3)) == 0);
    assert(race_t3_buildings_killed == saved_kills);

    /* A checksummed payload cannot move the campaign clock independently. */
    assert(race_save_game());
    ++race_data.campaign.elapsed_frames;
    save_write(&race_data, sizeof(race_data), SAVE_VERSION);
    assert(!race_load_game());
    assert(frame == saved_frame && u[0].x == saved_x);
    assert(memcmp(&race_t3, &saved_t3, sizeof(saved_t3)) == 0);
    assert(race_t3_buildings_killed == saved_kills);

    /* Nor can it persist an outcome inconsistent with the game's result flags. */
    assert(race_save_game());
    race_data.t3.outcome = SC64_T3_WON;
    race_data.t3.objective = 1;
    race_data.t3.fired |= SC64_T3_F_ALTERNATE | SC64_T3_F_SWITCH_1 | SC64_T3_F_RESULT;
    race_data.t3_buildings_killed = 1;
    race_data.campaign.objective_stage = 1;
    race_data.campaign.phase = SC64_RACE_PHASE_COMPLETE;
    race_data.campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
    race_data.state[18] = 0; /* T3 says won, but the engine result says running. */
    race_data.state[19] = 0;
    save_write(&race_data, sizeof(race_data), SAVE_VERSION);
    assert(sc64_t3_state_valid(&race_data.t3));
    assert(!race_load_game());
    assert(frame == saved_frame && u[0].x == saved_x);
    assert(memcmp(&race_t3, &saved_t3, sizeof(saved_t3)) == 0);
    assert(race_t3_buildings_killed == saved_kills);
}

static void previous_stage_save_is_rejected(void) {
    int saved_frame = frame, saved_x = u[0].x;
    assert(race_save_game());
    save_write(&race_data, sizeof(race_data), 0x53435432u);
    save_write(&race_data, sizeof(race_data), 0x53435432u);
    assert(!race_load_game());
    assert(frame == saved_frame && u[0].x == saved_x);
}

int main(void) {
    gba_init();
    start_t3_through_menu();
    fill_unused_unit_slots();
    save_round_trip();
    corrupt_t3_payload_is_atomic();
    previous_stage_save_is_rejected();
    printf("T3 save adapter: %d-unit pool, 31 buildings, source roster, corrupted payload atomicity and previous-stage version rejection pass (%zu bytes).\n",
           MAX_UNITS, sizeof(RaceSave));
    return 0;
}
