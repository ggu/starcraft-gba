/* Full Terran catalog/runtime integration smoke. These direct setups exercise
 * startup and persistence only; they are not controller-win evidence. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_TERRAN_FULL_CAMPAIGN_ENTRY) || !defined(SC64_T6_CAMPAIGN_ENTRY)
#error "Compile with the full Terran T1-T10 campaign profile"
#endif

static int frontend_press(Sc64Frontend *f, unsigned key)
{
    int action = sc64_frontend_update(f, key);
    (void)sc64_frontend_update(f, 0);
    return action;
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

static void test_normal_campaign_frontend_pages(void)
{
    Sc64Frontend f;
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) == 10);
    sc64_frontend_reset(&f, 4, R_TER, 2, 0);
    assert(frontend_press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION);
    for (int ordinal = 0; ordinal < 10; ++ordinal) {
        int page = ordinal / 2;
        int row = ordinal % 2;
        sc64_frontend_reset(&f, 4, R_TER, 2, 0);
        assert(frontend_press(&f, KEY_A) == SC_FRONT_NONE);
        for (int p = 0; p < page; ++p) frontend_press(&f, KEY_RIGHT);
        if (row) frontend_press(&f, KEY_DOWN);
        assert(f.page == SC_FRONT_MISSION);
        assert(f.mission_page == page && f.row == row);
        assert(sc64_catalog_playable_at(SC64_FACTION_TERRAN,
                                        f.mission_page * 2 + f.row) == ordinal + 4);
        assert(frontend_press(&f, KEY_A) == SC_FRONT_NONE);
        assert(f.page == SC_FRONT_BRIEFING && f.mission == ordinal + 4);
        assert(frontend_press(&f, KEY_START) == SC_FRONT_START_CAMPAIGN);
        assert(f.mission == ordinal + 4);
    }
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 12) == 13);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 13) == -1);
}

static void campaign_mission_save_roundtrip(int mission)
{
    game_init();
    race_mode = 0;
    race_human = R_TER;
    race_selected_mission = mission;
    race_completed = 0;
    init_state();
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == mission);
    assert(race_late_active() == (mission >= SC64_TERRAN_LATE_GLOBAL_T7));
    assert(!mission_unsupported_count);
    assert(race_cols == sc64_race_maps[mission].width);
    assert(race_rows == sc64_race_maps[mission].height);

    for (int tick = 0; tick < 60; ++tick) game_update(0, 0);
    assert(screen == SCREEN_PLAY && !won && !lost && !mission_unsupported_count);
    assert(race_campaign.mission == mission);
    assert(race_campaign.elapsed_frames == (u32)frame);
    assert(race_save_game());
    RaceSave expected = race_data;

    /* Loading the just-saved record must restore world, mission model, and
     * whichever T3/T4/T5/T6/late state is active for this mission. */
    if (!race_load_game()) {
        fprintf(stderr, "full profile campaign load failed at mission %d (generation=%u frame=%d)\n",
                mission, race_data.version, frame);
        assert(0);
    }
    assert(screen == SCREEN_PLAY && race_selected_mission == mission);
    assert(race_campaign.mission == mission && race_campaign.elapsed_frames == (u32)frame);
    assert(!mission_unsupported_count);
    assert(race_save_game());
    assert(!memcmp(&expected, &race_data, sizeof(expected)));
}

static void test_all_ten_campaign_missions_start_and_roundtrip(void)
{
    for (int mission = 4; mission <= 13; ++mission)
        campaign_mission_save_roundtrip(mission);
}

#ifdef SC64_IMPORTED_SKIRMISH
static void skirmish_save_roundtrip(int map, int human, int bots,
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
    if (map == 0) {
        /* Outpost uses race_skirmish_geometry()'s authored 64x64 field. */
        assert(race_cols == 64 && race_rows == 64);
    } else {
        assert(race_cols == sc64_skirmish_maps[map - 1].width);
        assert(race_rows == sc64_skirmish_maps[map - 1].height);
    }
    assert(race_player[SIDE] == human);
    assert(race_player[ENEMY] == race_bot_races[0]);
    if (bots > 1) assert(race_player[RESCUABLE] == race_bot_races[1]);
    if (bots > 2) assert(race_player[4] == race_bot_races[2]);
    assert(live_units_for_owner(SIDE) > 0 && live_buildings_for_owner(SIDE) > 0);
    const int owners[3] = { ENEMY, RESCUABLE, 4 };
    for (int i = 0; i < bots; ++i) {
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

static void test_imported_skirmish_matrix(void)
{
    assert(SC64_SKIRMISH_IMPORTED_COUNT >= 1);
    assert(SC64_FRONT_MAP_COUNT == SC64_SKIRMISH_IMPORTED_COUNT + 1);
    assert(SC64_FRONT_MAP_COUNT == 2);
    int cases = 0;
    for (int map = 0; map < SC64_FRONT_MAP_COUNT; ++map)
        for (int human = R_TER; human <= R_PROTOSS; ++human)
            for (int bots = 1; bots <= 3; ++bots) {
                int combinations = bots == 1 ? 3 : bots == 2 ? 9 : 27;
                for (int config = 0; config < combinations; ++config) {
                    int bot_races[3] = { R_TER, R_TER, R_TER };
                    int remaining = config;
                    for (int slot = 0; slot < bots; ++slot) {
                        bot_races[slot] = remaining % 3;
                        remaining /= 3;
                    }
                    skirmish_save_roundtrip(map, human, bots, bot_races);
                    ++cases;
                }
            }
    assert(cases == SC64_FRONT_MAP_COUNT * 3 * (3 + 9 + 27));
    assert(cases == 234);
    printf("Full Terran skirmish startup/save matrix: %d configurations passed\n", cases);
}
#endif

int main(void)
{
    test_normal_campaign_frontend_pages();
    test_all_ten_campaign_missions_start_and_roundtrip();
#ifdef SC64_IMPORTED_SKIRMISH
    test_imported_skirmish_matrix();
#endif
    puts("Full Terran profile startup/save integration checks passed");
    return 0;
}
