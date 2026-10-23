/* Host SRAM validation tests for the optional Zerg Z3 campaign profile. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u16 previous_keys;
static Unit saved_units[MAX_UNITS];
static Building saved_buildings[MAX_BUILDINGS];
static Sc64Z3State saved_z3;
static u16 saved_action_ticks;
static int saved_frame, saved_x, saved_y, saved_ore, saved_gas;
static int saved_won, saved_lost;
static Sc64RaceCampaignState saved_campaign;

static void frame_input(u16 held)
{
    game_update(held, held & (u16)~previous_keys);
    previous_keys = held;
}

static void press(u16 key)
{
    frame_input(key);
    frame_input(0);
}

static void start_z3(void)
{
    game_init();
    previous_keys = 0;
    press(KEY_A); /* HOME -> Zerg campaign missions. */
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_ZERG);
    press(KEY_RIGHT); /* Z3 is ordinal 2, first row of page 2. */
    assert(race_front.mission_page == 1);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 14);
    press(KEY_A);
    assert(screen == SCREEN_PLAY && race_selected_mission == 14);
    assert(race_campaign.mission == 14 && sc64_z3_state_valid(&race_z3));
}

static void start_z1(void)
{
    game_init();
    previous_keys = 0;
    press(KEY_A); /* HOME -> the profile's mission list. */
    assert(race_front.page == SC_FRONT_MISSION);
    press(KEY_A); /* Mission list -> briefing. */
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 0);
    frame_input(KEY_A); /* Deploy without a released-button live tick. */
    assert(screen == SCREEN_PLAY && race_selected_mission == 0);
    assert(race_campaign.mission == 0);
}

static void frame_fixture_win_pending(void)
{
    int units[MAX_UNITS], n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == ENEMY) units[n++] = i;
    assert(n == 45);
    for (int i = 5; i < n; ++i) u[units[i]].alive = 0;
    int kept_building = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == ENEMY) {
        if (!kept_building) kept_building = 1;
        else b[i].alive = 0;
    }
    assert(kept_building);
    race_z3_campaign_tick(); /* Deliberate fixture: Any Unit <= 6 -> switch7. */
    assert(race_z3.sequence == SC64_Z3_SEQ_WIN_WAIT_1000);
    assert(race_z3_action_ticks == race_z3_action_duration(&race_z3));
}

static void take_runtime_snapshot(void)
{
    memcpy(saved_units, u, sizeof(saved_units));
    memcpy(saved_buildings, b, sizeof(saved_buildings));
    saved_z3 = race_z3;
    saved_action_ticks = race_z3_action_ticks;
    saved_frame = frame; saved_x = cx; saved_y = cy;
    saved_ore = minerals; saved_gas = gas;
    saved_won = won; saved_lost = lost;
    saved_campaign = race_campaign;
}

static void assert_runtime_unchanged(void)
{
    assert(memcmp(saved_units, u, sizeof(saved_units)) == 0);
    assert(memcmp(saved_buildings, b, sizeof(saved_buildings)) == 0);
    assert(memcmp(&saved_z3, &race_z3, sizeof(saved_z3)) == 0);
    assert(saved_action_ticks == race_z3_action_ticks);
    assert(saved_frame == frame && saved_x == cx && saved_y == cy);
    assert(saved_ore == minerals && saved_gas == gas);
    assert(saved_won == won && saved_lost == lost);
    assert(memcmp(&saved_campaign, &race_campaign, sizeof(saved_campaign)) == 0);
}

static void reject_checksummed_mutation(void)
{
    save_write(&race_data, sizeof(race_data), SAVE_VERSION);
    if (race_load_game()) {
        assert(!"checksummed corrupt save should be rejected");
    }
    assert_runtime_unchanged();
}

static void test_pending_sequence_round_trip_and_corruption(void)
{
    start_z3();
    frame_input(0); /* Make model clock and saved game frame coherent. */
    frame_fixture_win_pending();
    assert(race_campaign.phase == SC64_RACE_PHASE_ACTIVE && !won && !lost);
    assert(race_z3_action_ticks > 0);
    assert(race_save_game());
    Sc64Z3State expected = race_z3;
    u16 expected_ticks = race_z3_action_ticks;
    int expected_frame = frame;
    assert(race_load_game());
    assert(memcmp(&race_z3, &expected, sizeof(expected)) == 0);
    assert(race_z3_action_ticks == expected_ticks && frame == expected_frame);

    take_runtime_snapshot();
    assert(race_save_game());
    race_data.z3_action_ticks = (u16)(race_z3_action_duration(&race_z3) + 1);
    reject_checksummed_mutation(); /* timer above the selected phase duration */

    assert(race_save_game());
    race_data.z3_action_ticks = 0;
    reject_checksummed_mutation(); /* active action cannot have an empty timer */

    assert(race_save_game());
    race_data.campaign.phase = SC64_RACE_PHASE_FAILED;
    reject_checksummed_mutation(); /* phase/outcome mismatch */

    assert(race_save_game());
    race_data.campaign.objective_stage = 0; /* switch7 is still true */
    reject_checksummed_mutation();

    assert(race_save_game());
    race_data.z3.switch7 = 0;
    reject_checksummed_mutation(); /* model's canonical switch/fired bits mismatch */
}

static void test_invalid_profile_and_omitted_mission_ids_are_atomic(void)
{
    start_z3();
    frame_input(0);
    take_runtime_snapshot();
    assert(race_save_game());
    race_data.state[2] = 2; /* Valid global map, wrong faction for Zerg profile. */
    reject_checksummed_mutation();

    assert(race_save_game());
    race_data.state[2] = 10; /* Gap between campaign blocks. */
    reject_checksummed_mutation();

    assert(race_save_game());
    race_data.state[2] = 15; /* Later Zerg mission exists but is omitted/locked. */
    reject_checksummed_mutation();

    assert(race_save_game());
    race_data.state[3] |= 1 << 4; /* Completed bit for another faction is invalid. */
    reject_checksummed_mutation();
}

static void test_other_mission_requires_zero_z3_payload(void)
{
    Sc64Z3State zero_state;
    memset(&zero_state, 0, sizeof(zero_state));
    start_z1();
    frame_input(0);
    assert(memcmp(&race_z3, &zero_state, sizeof(zero_state)) == 0);
    assert(race_z3_action_ticks == 0);
    assert(race_save_game());
    assert(memcmp(&race_data.z3, &zero_state, sizeof(zero_state)) == 0);
    assert(race_data.z3_action_ticks == 0);
    take_runtime_snapshot();

    assert(race_save_game());
    race_data.z3.fired = SC64_Z3_F_MISSION_START; /* stale cross-mission state */
    reject_checksummed_mutation();

    assert(race_save_game());
    race_data.z3_action_ticks = 1;
    reject_checksummed_mutation();
}

int main(void)
{
    gba_init();
    test_pending_sequence_round_trip_and_corruption();
    test_invalid_profile_and_omitted_mission_ids_are_atomic();
    test_other_mission_requires_zero_z3_payload();
    printf("Z3 save adapter: pending-action continuity, corruption atomicity, profile IDs, and cross-mission zero payload: ok (%zu bytes)\n",
           sizeof(RaceSave));
    return 0;
}
