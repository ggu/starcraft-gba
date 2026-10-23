/* P3 save-adapter tests use normal menu launch and bounded fixture changes.
 * Clearing foe buildings is an explicit trigger fixture, not a gameplay win. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u16 previous_keys;
static Unit saved_units[MAX_UNITS];
static Building saved_buildings[MAX_BUILDINGS];
static Mineral saved_minerals[MAX_MINERALS];
static Sc64P3State saved_p3;
static u16 saved_action_ticks;
static Sc64RaceCampaignState saved_campaign;
static int saved_frame, saved_x, saved_y, saved_ore, saved_gas;
static int saved_race_ore[5], saved_race_gas[5], saved_players[5];
static int saved_completed, saved_won, saved_lost, saved_screen;

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

static void start_p3_through_menu(void)
{
    game_init();
    previous_keys = 0;
    press(KEY_A); /* HOME -> Protoss mission list. */
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_PROTOSS);
    press(KEY_RIGHT); /* Higher Ground is Protoss playable ordinal 2, page 2. */
    assert(race_front.mission_page == 1);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 22);
    frame_input(KEY_A); /* Deploy without releasing A during live simulation. */
    assert(screen == SCREEN_PLAY && !race_mode && race_selected_mission == 22);
    assert(race_campaign.mission == 22 && sc64_p3_model_state_valid(&race_p3));
    assert(minerals == SC64_P3_HUMAN_MINERALS && gas == SC64_P3_HUMAN_GAS);
    assert(race_minerals[SIDE] == SC64_P3_HUMAN_MINERALS);
    assert(race_gas[SIDE] == SC64_P3_HUMAN_GAS);
    assert(race_player[SIDE] == R_PROTOSS);
    assert(race_player[ENEMY] == R_ZERG && race_player[4] == R_ZERG);
}

static void snapshot_runtime(void)
{
    memcpy(saved_units, u, sizeof(u));
    memcpy(saved_buildings, b, sizeof(b));
    memcpy(saved_minerals, m, sizeof(m));
    saved_p3 = race_p3;
    saved_action_ticks = race_p3_action_ticks;
    saved_campaign = race_campaign;
    saved_frame = frame; saved_x = cx; saved_y = cy;
    saved_ore = minerals; saved_gas = gas;
    memcpy(saved_race_ore, race_minerals, sizeof(saved_race_ore));
    memcpy(saved_race_gas, race_gas, sizeof(saved_race_gas));
    memcpy(saved_players, race_player, sizeof(saved_players));
    saved_completed = race_completed; saved_won = won; saved_lost = lost;
    saved_screen = screen;
}

static void assert_runtime_unchanged(void)
{
    assert(memcmp(saved_units, u, sizeof(u)) == 0);
    assert(memcmp(saved_buildings, b, sizeof(b)) == 0);
    assert(memcmp(saved_minerals, m, sizeof(m)) == 0);
    assert(memcmp(&saved_p3, &race_p3, sizeof(saved_p3)) == 0);
    assert(saved_action_ticks == race_p3_action_ticks);
    assert(memcmp(&saved_campaign, &race_campaign, sizeof(saved_campaign)) == 0);
    assert(saved_frame == frame && saved_x == cx && saved_y == cy);
    assert(saved_ore == minerals && saved_gas == gas);
    assert(memcmp(saved_race_ore, race_minerals, sizeof(saved_race_ore)) == 0);
    assert(memcmp(saved_race_gas, race_gas, sizeof(saved_race_gas)) == 0);
    assert(memcmp(saved_players, race_player, sizeof(saved_players)) == 0);
    assert(saved_completed == race_completed && saved_won == won && saved_lost == lost);
    assert(saved_screen == screen);
}

static void reject_checksumming_corruption(void)
{
    save_write(&race_data, sizeof(race_data), SAVE_VERSION);
    assert(!race_load_game());
    assert_runtime_unchanged();
}

static void fixture_pending_victory_action(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && (b[i].owner == ENEMY || b[i].owner == 4))
            b[i].alive = 0;
    race_p3_campaign_tick();
    assert(race_p3.outcome == SC64_P3_RUNNING);
    assert(race_p3.sequence == SC64_P3_SEQ_WIN_WAIT_2000);
    assert(race_p3_action_ticks == race_p3_action_duration(&race_p3));
    assert(race_campaign.objective_stage == race_p3.sequence);
}

static void test_roundtrip_and_timer_validation(void)
{
    start_p3_through_menu();
    frame_input(0); /* Advance one real mission update and fire source force grants. */
    assert(race_minerals[ENEMY] == SC64_P3_FORCE2_COMBINED_RESOURCES);
    assert(race_gas[ENEMY] == SC64_P3_FORCE2_COMBINED_RESOURCES);
    assert(race_minerals[4] == SC64_P3_FORCE3_COMBINED_RESOURCES);
    assert(race_gas[4] == SC64_P3_FORCE3_COMBINED_RESOURCES);
    int ore = race_minerals[ENEMY], gas_store = race_gas[4];
    frame_input(0);
    assert(race_minerals[ENEMY] == ore && race_gas[4] == gas_store);

    fixture_pending_victory_action();
    race_p3_action_ticks = 1; /* Valid last tick of the current action. */
    assert(race_save_game());
    Sc64P3State expected = race_p3;
    int expected_frame = frame;
    u16 expected_ticks = race_p3_action_ticks;
    int expected_force2_ore = race_minerals[ENEMY];
    int expected_force3_gas = race_gas[4];
    race_p3_action_ticks = 0;
    frame++;
    race_minerals[ENEMY]++;
    race_gas[4]++;
    assert(race_load_game());
    assert(memcmp(&race_p3, &expected, sizeof(expected)) == 0);
    assert(race_p3_action_ticks == expected_ticks && frame == expected_frame);
    assert(race_minerals[ENEMY] == expected_force2_ore && race_gas[4] == expected_force3_gas);

    snapshot_runtime();
    assert(race_save_game());
    race_data.p3_action_ticks = (u16)(race_p3_action_duration(&race_p3) + 1);
    reject_checksumming_corruption();

    assert(race_save_game());
    race_data.p3_action_ticks = 0;
    reject_checksumming_corruption();

    assert(race_save_game());
    race_data.campaign.objective_stage = SC64_P3_SEQ_NONE;
    reject_checksumming_corruption();

    assert(race_save_game());
    race_data.p3.sequence = SC64_P3_SEQ_WIN_TX_2;
    race_data.p3.transmission = 2;
    reject_checksumming_corruption(); /* Runtime stage and campaign stage disagree. */

    assert(race_save_game());
    race_data.state[1] = R_ZERG;
    reject_checksumming_corruption(); /* Higher Ground save cannot change human race. */

    assert(race_save_game());
    race_data.players[SIDE] = R_ZERG;
    reject_checksumming_corruption(); /* Source Protoss human slot must remain Protoss. */

    assert(race_save_game());
    race_data.players[ENEMY] = R_PROTOSS;
    reject_checksumming_corruption(); /* Force 2 is a Zerg source player. */

    assert(race_save_game());
    race_data.players[4] = R_PROTOSS;
    reject_checksumming_corruption(); /* Force 3 is a distinct Zerg source player. */
}

static void test_won_and_lost_payload_consistency(void)
{
    start_p3_through_menu();
    fixture_pending_victory_action();
    for (int i = 0; i < 400 && race_p3.outcome == SC64_P3_RUNNING; ++i)
        race_p3_campaign_tick();
    assert(race_p3.outcome == SC64_P3_WON && won && !lost);
    assert(race_completed & (1 << 22));
    assert(race_campaign.phase == SC64_RACE_PHASE_COMPLETE);
    assert(race_save_game() && race_load_game());
    snapshot_runtime();

    /* A saved win must agree across the result flags, phase, and completion bit. */
    assert(race_save_game());
    race_data.state[3] &= ~(1 << 22);
    reject_checksumming_corruption();

    start_p3_through_menu();
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) u[i].alive = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE) b[i].alive = 0;
    race_p3_campaign_tick(); /* Explicit no-human-AnyUnit defeat fixture. */
    assert(race_p3.outcome == SC64_P3_LOST && lost && !won);
    assert(race_campaign.phase == SC64_RACE_PHASE_FAILED);
    assert(race_save_game() && race_load_game());
    snapshot_runtime();
    assert(race_save_game());
    race_data.campaign.phase = SC64_RACE_PHASE_COMPLETE;
    reject_checksumming_corruption();
}

static void test_other_mission_requires_zero_p3_payload(void)
{
    game_init();
    previous_keys = 0;
    press(KEY_A); /* P1 list, page 1. */
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 2);
    frame_input(KEY_A);
    assert(screen == SCREEN_PLAY && race_selected_mission == 2);
    assert(race_p3.fired == 0 && race_p3_action_ticks == 0);
    assert(race_save_game());
    for (unsigned i = 0; i < sizeof(race_data.p3); ++i)
        assert(((const u8 *)&race_data.p3)[i] == 0);
    assert(race_data.p3_action_ticks == 0);
    snapshot_runtime();

    assert(race_save_game());
    race_data.p3.fired = SC64_P3_F_MISSION_START;
    reject_checksumming_corruption();

    assert(race_save_game());
    save_write(&race_data, sizeof(race_data), 0x53435036u);
    assert(!race_load_game()); /* Previous large Protoss save generation. */
    assert_runtime_unchanged();
}

int main(void)
{
    gba_init();
    test_roundtrip_and_timer_validation();
    test_won_and_lost_payload_consistency();
    test_other_mission_requires_zero_p3_payload();
    printf("P3 save adapter: menu launch, action timer bounds, results, owner wallets, and atomic corruption checks passed (%zu bytes).\n",
           sizeof(RaceSave));
    return 0;
}
