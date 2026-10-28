/* Controller-only T3 pause and SRAM reload regression. Build with the
 * experimental T3/unified Terran campaign flags; no mission state is injected. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u16 previous;
static Unit saved_units[MAX_UNITS];
static Building saved_buildings[MAX_BUILDINGS];
static Mineral saved_minerals[MAX_MINERALS];
static Sc64T3State saved_t3;
static Sc64RaceCampaignState saved_campaign;
static u16 saved_kills;
static int saved_frame, saved_minerals_bank, saved_gas_bank;

static void frame_input(u16 held) {
    game_update(held, held & (u16)~previous);
    previous = held;
}

static void press(u16 key) {
    frame_input(key);
    frame_input(0);
}

static void launch_t3_through_menu(void) {
    game_init();
    previous = 0;
    press(KEY_A); /* HOME -> the profile's Terran campaign mission list. */
    assert(race_front.page == SC_FRONT_MISSION);
    press(KEY_RIGHT); /* T3 is Terran campaign ordinal 2, on page 2. */
    assert(race_front.mission_page == 1);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 6);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && race_selected_mission == 6);
    assert(race_campaign.mission == 6 && sc64_t3_state_valid(&race_t3));
    assert(mission_unsupported_count == 0);
}

static void capture_saved_state(void) {
    memcpy(saved_units, u, sizeof(saved_units));
    memcpy(saved_buildings, b, sizeof(saved_buildings));
    memcpy(saved_minerals, m, sizeof(saved_minerals));
    saved_t3 = race_t3;
    saved_campaign = race_campaign;
    saved_kills = race_t3_buildings_killed;
    saved_frame = frame;
    saved_minerals_bank = minerals;
    saved_gas_bank = gas;
}

static void assert_restored_state(void) {
    assert(screen == SCREEN_PLAY);
    assert(race_selected_mission == 6 && race_campaign.mission == 6);
    assert(memcmp(&race_t3, &saved_t3, sizeof(saved_t3)) == 0);
    assert(memcmp(&race_campaign, &saved_campaign, sizeof(saved_campaign)) == 0);
    assert(race_t3_buildings_killed == saved_kills);
    assert(frame == saved_frame && race_t3.elapsed_ticks == (u32)saved_frame);
    assert(minerals == saved_minerals_bank && gas == saved_gas_bank);
    assert(memcmp(u, saved_units, sizeof(saved_units)) == 0);
    assert(memcmp(b, saved_buildings, sizeof(saved_buildings)) == 0);
    assert(memcmp(m, saved_minerals, sizeof(saved_minerals)) == 0);
}

int main(void) {
    gba_init();
    launch_t3_through_menu();

    /* Advance only through ordinary game updates, then prove pause freezes
     * both the mission clock and its model/campaign state. */
    for (int i = 0; i < 1000; ++i) frame_input(0);
    assert(race_t3.elapsed_ticks == race_campaign.elapsed_frames);
    assert(race_t3.elapsed_ticks == (u32)frame && race_t3.elapsed_ticks >= 1000);
    press(KEY_START);
    assert(screen == SCREEN_PAUSE);
    capture_saved_state();
    for (int i = 0; i < 120; ++i) frame_input(0);
    assert(screen == SCREEN_PAUSE && frame == saved_frame);
    assert(memcmp(&race_t3, &saved_t3, sizeof(saved_t3)) == 0);
    assert(memcmp(&race_campaign, &saved_campaign, sizeof(saved_campaign)) == 0);
    assert(race_t3_buildings_killed == saved_kills);
    assert(memcmp(u, saved_units, sizeof(saved_units)) == 0);
    assert(memcmp(b, saved_buildings, sizeof(saved_buildings)) == 0);

    /* B saves through the pause controller. game_init supplies a fresh HOME
     * state while the host SRAM backend retains the real serialized record. */
    press(KEY_B);
    game_init();
    previous = 0;
    frame_input(KEY_L); /* Observe immediately after load, before a live tick. */
    assert_restored_state();

    /* The normal title-menu load resumes into live play; one ordinary tick
     * must advance both clocks together. */
    frame_input(0);
    assert(screen == SCREEN_PLAY && frame == saved_frame + 1);
    assert(race_t3.elapsed_ticks == (u32)frame);
    assert(race_campaign.elapsed_frames == (u32)frame);
    puts("T3 controller pause/save/reload/resume continuity passes.");
    return 0;
}
