/* Promoted race-profile integration: normal-menu campaign starts, profile
 * save generations, and supported skirmish startup/save combinations. These
 * bounded setups are not mission-win evidence. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_UNIFIED) || !defined(SC64_IMPORTED_SKIRMISH) || \
    !defined(SC64_CAMPAIGN_RACE)
#error "Compile with a promoted unified Zerg or Protoss profile"
#endif

#if SC64_CAMPAIGN_RACE == 1 && defined(SC64_Z7_RUNTIME)
#define PROFILE_FACTION SC64_FACTION_ZERG
#define PROFILE_FRONT_FACTION SC_FRONT_ZERG
#define PROFILE_RACE R_ZERG
#if defined(SC64_Z7_DERIVED_SAVE)
#define PROFILE_SAVE_GENERATION 38u
#elif defined(SC64_Z7_EGG_PRODUCTION)
#define PROFILE_SAVE_GENERATION 36u
#elif defined(SC64_Z7_COLONY_DEFENSE_ENABLED)
#define PROFILE_SAVE_GENERATION 35u
#else
#define PROFILE_SAVE_GENERATION 33u
#endif
#define PROFILE_OLD_SAVE_TAG 0x53435a32u
static const int profile_missions[] = {0, 1, 14, 15, 16, 17, 18};
static const char *const profile_titles[] = {
    "AMONG THE RUINS", "EGRESSION", "THE NEW DOMINION", "AGENT OF THE SWARM",
    "THE AMERIGO", "THE DARK TEMPLAR", "THE CULLING"
};
#elif SC64_CAMPAIGN_RACE == 1 && defined(SC64_Z6_RUNTIME)
#define PROFILE_FACTION SC64_FACTION_ZERG
#define PROFILE_FRONT_FACTION SC_FRONT_ZERG
#define PROFILE_RACE R_ZERG
#ifdef SC64_Z6_TRANSPORT_ENABLED
#define PROFILE_SAVE_GENERATION 32u
#else
#define PROFILE_SAVE_GENERATION 31u
#endif
#define PROFILE_OLD_SAVE_TAG 0x53435a32u
static const int profile_missions[] = {0, 1, 14, 15, 16, 17};
static const char *const profile_titles[] = {
    "AMONG THE RUINS", "EGRESSION", "THE NEW DOMINION", "AGENT OF THE SWARM",
    "THE AMERIGO", "THE DARK TEMPLAR"
};
#elif SC64_CAMPAIGN_RACE == 1 && defined(SC64_Z5_RUNTIME)
#define PROFILE_FACTION SC64_FACTION_ZERG
#define PROFILE_FRONT_FACTION SC_FRONT_ZERG
#define PROFILE_RACE R_ZERG
#if defined(SC64_Z5_CONSUME_ENABLED)
#define PROFILE_SAVE_GENERATION 27u
#elif defined(SC64_Z5_CLOAK_ENABLED)
#define PROFILE_SAVE_GENERATION 25u
#else
#define PROFILE_SAVE_GENERATION 23u
#endif
#define PROFILE_OLD_SAVE_TAG 0x53435a32u
static const int profile_missions[] = {0, 1, 14, 15, 16};
static const char *const profile_titles[] = {
    "AMONG THE RUINS", "EGRESSION", "THE NEW DOMINION", "AGENT OF THE SWARM",
    "THE AMERIGO"
};
#elif SC64_CAMPAIGN_RACE == 1 && defined(SC64_Z4_RUNTIME)
#define PROFILE_FACTION SC64_FACTION_ZERG
#define PROFILE_FRONT_FACTION SC_FRONT_ZERG
#define PROFILE_RACE R_ZERG
#define PROFILE_SAVE_GENERATION 20u
#define PROFILE_OLD_SAVE_TAG 0x53435a32u
static const int profile_missions[] = {0, 1, 14, 15};
static const char *const profile_titles[] = {
    "AMONG THE RUINS", "EGRESSION", "THE NEW DOMINION", "AGENT OF THE SWARM"
};
#elif SC64_CAMPAIGN_RACE == 2 && defined(SC64_P7_RUNTIME)
#define PROFILE_FACTION SC64_FACTION_PROTOSS
#define PROFILE_FRONT_FACTION SC_FRONT_PROTOSS
#define PROFILE_RACE R_PROTOSS
#define PROFILE_SAVE_GENERATION 43u
#define PROFILE_OLD_SAVE_TAG 0x53435032u
#if !defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || !defined(SC64_P7_CARRIER_TRAINING_ENABLED) || !defined(SC64_P7_CAMPAIGN_ENTRY)
#error "Canonical Protoss P7 requires its qualified Carrier and campaign-entry features"
#endif
_Static_assert(SC64_RACE_SAVE_GENERATION == 43u,
               "canonical P7 retains its qualified generation-43 save schema");
_Static_assert(SAVE_VERSION == 0x5343505bu,
               "canonical P7 keeps its generation-43 Protoss save identity");
_Static_assert(SC64_P7_GLOBAL_ID == 26,
               "canonical Protoss P7 uses its stable global catalog identity");
static const int profile_missions[] = {2, 3, 22, 23, 24, 25, 26};
static const char *const profile_titles[] = {
    "FIRST STRIKE", "INTO THE FLAMES", "HIGHER GROUND", "THE HUNT FOR TASSADAR",
    "CHOOSING SIDES", "INTO THE DARKNESS", "HOMELAND"
};
#elif SC64_CAMPAIGN_RACE == 2 && defined(SC64_P6_RUNTIME)
#define PROFILE_FACTION SC64_FACTION_PROTOSS
#define PROFILE_FRONT_FACTION SC_FRONT_PROTOSS
#define PROFILE_RACE R_PROTOSS
#define PROFILE_SAVE_GENERATION 30u
#define PROFILE_OLD_SAVE_TAG 0x53435032u
#if !defined(SC64_P6_STORM_ENABLED) || !defined(SC64_P6_CAMPAIGN_ENTRY)
#error "Canonical Protoss P6 requires its qualified Storm and campaign-entry features"
#endif
_Static_assert(SC64_RACE_SAVE_GENERATION == 30u,
               "canonical P6 retains its qualified generation-30 save schema");
_Static_assert(SAVE_VERSION == 0x5343504eu,
               "canonical P6 keeps its generation-30 Protoss save identity");
_Static_assert(SC64_P6_GLOBAL_ID == 25,
               "canonical Protoss P6 uses its stable global catalog identity");
static const int profile_missions[] = {2, 3, 22, 23, 24, 25};
static const char *const profile_titles[] = {
    "FIRST STRIKE", "INTO THE FLAMES", "HIGHER GROUND", "THE HUNT FOR TASSADAR",
    "CHOOSING SIDES", "INTO THE DARKNESS"
};
#elif SC64_CAMPAIGN_RACE == 2 && defined(SC64_P5_RUNTIME)
#define PROFILE_FACTION SC64_FACTION_PROTOSS
#define PROFILE_FRONT_FACTION SC_FRONT_PROTOSS
#define PROFILE_RACE R_PROTOSS
#ifdef SC64_P5_SHUTTLE_ENABLED
#define PROFILE_SAVE_GENERATION 26u
#else
#define PROFILE_SAVE_GENERATION 24u
#endif
#define PROFILE_OLD_SAVE_TAG 0x53435032u
static const int profile_missions[] = {2, 3, 22, 23, 24};
static const char *const profile_titles[] = {
    "FIRST STRIKE", "INTO THE FLAMES", "HIGHER GROUND", "THE HUNT FOR TASSADAR",
    "CHOOSING SIDES"
};
#elif SC64_CAMPAIGN_RACE == 2 && defined(SC64_P4_RUNTIME)
#define PROFILE_FACTION SC64_FACTION_PROTOSS
#define PROFILE_FRONT_FACTION SC_FRONT_PROTOSS
#define PROFILE_RACE R_PROTOSS
#ifdef SC64_ZP_FULL_STORAGE
#define PROFILE_SAVE_GENERATION 22u
#elif defined(SC64_P4_STORM_ENABLED)
#define PROFILE_SAVE_GENERATION 21u
#else
#define PROFILE_SAVE_GENERATION 20u
#endif
#define PROFILE_OLD_SAVE_TAG 0x53435032u
static const int profile_missions[] = {2, 3, 22, 23};
static const char *const profile_titles[] = {
    "FIRST STRIKE", "INTO THE FLAMES", "HIGHER GROUND", "THE HUNT FOR TASSADAR"
};
#elif SC64_CAMPAIGN_RACE == 1 && defined(SC64_Z3_ENABLED)
#define PROFILE_FACTION SC64_FACTION_ZERG
#define PROFILE_FRONT_FACTION SC_FRONT_ZERG
#define PROFILE_RACE R_ZERG
#define PROFILE_SAVE_GENERATION 5u
#define PROFILE_OLD_SAVE_TAG 0x53435a32u
static const int profile_missions[] = {0, 1, 14};
static const char *const profile_titles[] = {
    "AMONG THE RUINS", "EGRESSION", "THE NEW DOMINION"
};
#elif SC64_CAMPAIGN_RACE == 2 && defined(SC64_P3_ENABLED) && defined(SC64_P3_RUNTIME)
#define PROFILE_FACTION SC64_FACTION_PROTOSS
#define PROFILE_FRONT_FACTION SC_FRONT_PROTOSS
#define PROFILE_RACE R_PROTOSS
#define PROFILE_SAVE_GENERATION 7u
#define PROFILE_OLD_SAVE_TAG 0x53435032u
static const int profile_missions[] = {2, 3, 22};
static const char *const profile_titles[] = {
    "FIRST STRIKE", "INTO THE FLAMES", "HIGHER GROUND"
};
#else
#error "Build must be the canonical promoted Z3 or P3 profile"
#endif

static u16 previous_keys;

static void frame_input(u16 held)
{
    game_update(held, held & (u16)~previous_keys);
    previous_keys = held;
}

static void menu_press(u16 key)
{
    frame_input(key);
    frame_input(0);
}

static void campaign_start_from_menu(int mission)
{
    game_init();
    previous_keys = 0;
    assert(race_front.page == SC_FRONT_HOME);
    menu_press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION);
    assert(race_front.faction == PROFILE_FRONT_FACTION);
    int ordinal = sc64_catalog_playable_ordinal(PROFILE_FACTION, mission);
    assert(ordinal >= 0);
    int page = ordinal / 2;
    for (int i = 0; i < page; ++i)
        menu_press(KEY_RIGHT);
    if (ordinal & 1) menu_press(KEY_DOWN);
    assert(race_front.mission_page == page);
    menu_press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == mission);
    /* Deploy through the actual normal-menu entry. */
    frame_input(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == mission && race_campaign.mission == mission);
    assert(race_human == PROFILE_RACE);
}

static void test_campaign_mission_roundtrip(int mission)
{
    campaign_start_from_menu(mission);
    for (int tick = 0; tick < 60; ++tick) game_update(0, 0);
    assert(screen == SCREEN_PLAY && !won && !lost);
    assert(race_campaign.mission == mission);
    assert(race_campaign.elapsed_frames == (u32)frame);
    assert(race_save_game());
    RaceSave expected = race_data;
#ifdef SC64_P5_RUNTIME
    if (mission == SC64_P5_GLOBAL_ID) {
        assert(expected.p5.runtime.model.started != 0);
        assert(expected.campaign.objective_stage != 0);
        assert(memcmp(expected.p5.runtime.model.action_cursor,
                      race_p5.runtime.model.action_cursor,
                      sizeof(expected.p5.runtime.model.action_cursor)) == 0);
        assert(expected.p5.runtime.model.started == race_p5.runtime.model.started);
        assert(expected.campaign.objective_stage == race_campaign.objective_stage);
    }
#endif
    assert(expected.version == SAVE_VERSION);
    assert(SC64_RACE_SAVE_GENERATION == PROFILE_SAVE_GENERATION);
    assert(race_load_game());
    assert(screen == SCREEN_PLAY && race_selected_mission == mission);
    assert(race_campaign.mission == mission);
    assert(race_save_game());
    assert(memcmp(&expected, &race_data, sizeof(expected)) == 0);
}

#ifdef SC64_P5_RUNTIME
static void test_p5_inflight_save_payload(void)
{
    campaign_start_from_menu(SC64_P5_GLOBAL_ID);
    game_update(0, 0);
    assert(screen == SCREEN_PLAY && race_p5_active());

    u8 any_cursor = 0;
    for (unsigned i = 0; i < SC64_P5_TRIGGER_COUNT; ++i)
        any_cursor |= race_p5.runtime.model.action_cursor[i];
    assert(race_p5.runtime.model.started != 0);
    assert(any_cursor != 0);
    assert(race_campaign.objective_stage != 0);

    assert(race_save_game());
    RaceSave expected = race_data;
    assert(expected.p5.runtime.model.started == race_p5.runtime.model.started);
    assert(memcmp(expected.p5.runtime.model.action_cursor,
                  race_p5.runtime.model.action_cursor,
                  sizeof(expected.p5.runtime.model.action_cursor)) == 0);
    assert(expected.campaign.objective_stage == race_campaign.objective_stage);

    assert(race_load_game());
    assert(race_p5.runtime.model.started == expected.p5.runtime.model.started);
    assert(memcmp(race_p5.runtime.model.action_cursor,
                  expected.p5.runtime.model.action_cursor,
                  sizeof(expected.p5.runtime.model.action_cursor)) == 0);
    assert(race_campaign.objective_stage == expected.campaign.objective_stage);
}
#endif

static void reject_generation2_save(int mission, int legacy_container)
{
    campaign_start_from_menu(mission);
    for (int tick = 0; tick < 3; ++tick) game_update(0, 0);
    RaceSave old = race_data;
    old.version = PROFILE_OLD_SAVE_TAG;
    if (legacy_container) {
        /* Invalidate both current-profile records so the loader cannot fall
         * back to a prior valid current save after rejecting the old tag. */
        save_test_corrupt_latest(sizeof(RaceSave), SAVE_VERSION);
        save_test_corrupt_latest(sizeof(RaceSave), SAVE_VERSION);
        save_write(&old, sizeof(old), PROFILE_OLD_SAVE_TAG);
        RaceSave legacy;
        assert(save_read(&legacy, sizeof(legacy), PROFILE_OLD_SAVE_TAG));
        assert(legacy.version == PROFILE_OLD_SAVE_TAG);
    } else {
        /* A current container carrying the old generation payload checks
         * the explicit in-payload version guard independently. */
        save_write(&old, sizeof(old), SAVE_VERSION);
    }

    Unit old_units[MAX_UNITS];
    Building old_buildings[MAX_BUILDINGS];
    Mineral old_minerals[MAX_MINERALS];
    memcpy(old_units, u, sizeof(u));
    memcpy(old_buildings, b, sizeof(b));
    memcpy(old_minerals, m, sizeof(m));
    Sc64RaceCampaignState old_campaign = race_campaign;
    int old_players[5];
    memcpy(old_players, race_player, sizeof(old_players));
    int old_selected = race_selected_mission, old_mode = race_mode;
    int old_human = race_human, old_bots = race_bots, old_completed = race_completed;
    int old_map = race_skirmish_map;
    int old_bot_races[3];
    memcpy(old_bot_races, race_bot_races, sizeof(old_bot_races));
#if defined(SC64_Z7_RUNTIME)
    Sc64Z7PersistentState old_profile_state = race_z7;
#elif defined(SC64_Z4_RUNTIME)
    Sc64Z4RuntimeState old_profile_state = race_z4;
#elif defined(SC64_P4_RUNTIME)
    Sc64P4RuntimeState old_profile_state = race_p4;
#elif SC64_CAMPAIGN_RACE == 1
    Sc64Z3State old_profile_state = race_z3;
    u16 old_profile_ticks = race_z3_action_ticks;
#else
    Sc64P3State old_profile_state = race_p3;
    u16 old_profile_ticks = race_p3_action_ticks;
#endif
    int old_frame = frame, old_screen = screen, old_x = cx, old_y = cy;
    int old_ore = minerals, old_gas = gas;
    if (race_load_game()) {
        fprintf(stderr, "accepted generation-2 save (%s container)\n",
                legacy_container ? "legacy-tag" : "current-tag");
        assert(0);
    }
    assert(memcmp(old_units, u, sizeof(u)) == 0);
    assert(memcmp(old_buildings, b, sizeof(b)) == 0);
    assert(memcmp(old_minerals, m, sizeof(m)) == 0);
    assert(memcmp(&old_campaign, &race_campaign, sizeof(old_campaign)) == 0);
    assert(memcmp(old_players, race_player, sizeof(old_players)) == 0);
    assert(race_selected_mission == old_selected && race_mode == old_mode &&
           race_human == old_human && race_bots == old_bots &&
           race_completed == old_completed && race_skirmish_map == old_map);
    assert(memcmp(old_bot_races, race_bot_races, sizeof(old_bot_races)) == 0);
#if defined(SC64_Z7_RUNTIME)
    assert(memcmp(&old_profile_state, &race_z7, sizeof(old_profile_state)) == 0);
#elif defined(SC64_Z4_RUNTIME)
    assert(memcmp(&old_profile_state, &race_z4, sizeof(old_profile_state)) == 0);
#elif defined(SC64_P4_RUNTIME)
    assert(memcmp(&old_profile_state, &race_p4, sizeof(old_profile_state)) == 0);
#elif SC64_CAMPAIGN_RACE == 1
    assert(memcmp(&old_profile_state, &race_z3, sizeof(old_profile_state)) == 0);
    assert(old_profile_ticks == race_z3_action_ticks);
#else
    assert(memcmp(&old_profile_state, &race_p3, sizeof(old_profile_state)) == 0);
    assert(old_profile_ticks == race_p3_action_ticks);
#endif
    assert(frame == old_frame && screen == old_screen && cx == old_x && cy == old_y);
    assert(minerals == old_ore && gas == old_gas);
}

static void test_older_generation_rejection(int mission)
{
    reject_generation2_save(mission, 1);
    reject_generation2_save(mission, 0);
}

static int live_units_for_owner(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++count;
    return count;
}

static int live_buildings_for_owner(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) ++count;
    return count;
}

static void test_skirmish_roundtrip(int map, int human, int bots,
                                    const int bot_races[3])
{
    game_init();
    race_mode = 1;
    race_human = human;
    race_bots = bots;
    race_skirmish_map = map;
    for (int i = 0; i < 3; ++i) race_bot_races[i] = bot_races[i];
    init_state();
    assert(screen == SCREEN_PLAY && race_mode && !won && !lost);
    assert(race_human == human && race_bots == bots && race_skirmish_map == map);
    if (map == 0) assert(race_cols == 64 && race_rows == 64);
    else {
        assert(race_cols == sc64_skirmish_maps[map - 1].width);
        assert(race_rows == sc64_skirmish_maps[map - 1].height);
    }
    assert(race_player[SIDE] == human);
    const int owners[3] = {ENEMY, RESCUABLE, 4};
    for (int i = 0; i < bots; ++i) {
        assert(race_player[owners[i]] == bot_races[i]);
        assert(live_units_for_owner(owners[i]) > 0);
        assert(live_buildings_for_owner(owners[i]) > 0);
    }
    for (int i = bots; i < 3; ++i) {
        assert(live_units_for_owner(owners[i]) == 0);
        assert(live_buildings_for_owner(owners[i]) == 0);
    }
    for (int tick = 0; tick < 60; ++tick) game_update(0, 0);
    assert(screen == SCREEN_PLAY && !won && !lost);
    assert(race_save_game());
    RaceSave expected = race_data;
    assert(race_load_game());
    assert(screen == SCREEN_PLAY && race_mode && race_human == human &&
           race_bots == bots && race_skirmish_map == map);
    assert(!memcmp(race_bot_races, expected.state + 21, sizeof(race_bot_races)));
    assert(race_save_game());
    assert(!memcmp(&expected, &race_data, sizeof(expected)));
}

static void test_all_skirmish_lineups(void)
{
    assert(SC64_FRONT_MAP_COUNT == SC64_SKIRMISH_IMPORTED_COUNT + 1);
    assert(SC64_FRONT_MAP_COUNT == 2);
    int cases = 0;
    for (int map = 0; map < SC64_FRONT_MAP_COUNT; ++map)
        for (int human = R_TER; human <= R_PROTOSS; ++human)
            for (int bots = 1; bots <= 3; ++bots) {
                int combinations = bots == 1 ? 3 : bots == 2 ? 9 : 27;
                for (int config = 0; config < combinations; ++config) {
                    int bot_races[3] = {R_TER, R_TER, R_TER};
                    int remaining = config;
                    for (int slot = 0; slot < bots; ++slot) {
                        bot_races[slot] = remaining % 3;
                        remaining /= 3;
                    }
                    test_skirmish_roundtrip(map, human, bots, bot_races);
                    ++cases;
                }
            }
    assert(cases == 234);
    printf("Promoted race skirmish startup/save matrix: %d configurations passed\n", cases);
}

int main(void)
{
    gba_init();
    assert(SC64_RACE_SAVE_GENERATION == PROFILE_SAVE_GENERATION);
    const unsigned mission_count = sizeof(profile_missions) / sizeof(profile_missions[0]);
    assert(sc64_catalog_playable_count(PROFILE_FACTION) == mission_count);
    assert(SAVE_VERSION == PROFILE_OLD_SAVE_TAG +
           (PROFILE_SAVE_GENERATION - 2u));
    for (unsigned i = 0; i < mission_count; ++i) {
        assert(sc64_campaign_catalog[profile_missions[i]].id == profile_missions[i]);
        assert(sc64_campaign_catalog[profile_missions[i]].faction == PROFILE_FACTION);
        assert(sc64_campaign_catalog[profile_missions[i]].playable);
        assert(strcmp(sc64_campaign_catalog[profile_missions[i]].title,
                      profile_titles[i]) == 0);
        test_campaign_mission_roundtrip(profile_missions[i]);
    }
#ifdef SC64_P5_RUNTIME
    test_p5_inflight_save_payload();
#endif
    test_older_generation_rejection(profile_missions[0]);
    test_all_skirmish_lineups();
    puts("Promoted race profile startup/save qualification passed");
    return 0;
}
