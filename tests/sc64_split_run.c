/* Controller-driven three-race skirmish/profile regression.
 * Build once per SC64_CAMPAIGN_RACE value with SC64_IMPORTED_SKIRMISH. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static u16 previous;
static void frame_input(u16 held) {
    game_update(held, held & (u16)~previous);
    previous = held;
}
static void press(u16 key) { frame_input(key); frame_input(0); }

static int hall_type(int race) {
    return race == R_ZERG ? B_HATCHERY : race == R_PROTOSS ? B_NEXUS : B_COMMAND;
}
static int owner_at(int player) {
    static const int owners[4] = { SIDE, ENEMY, RESCUABLE, 4 };
    return owners[player];
}
static int count_units(int owner) {
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == owner) ++n;
    return n;
}
static int count_buildings(int owner) {
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == owner) ++n;
    return n;
}
static int count_building_type(int owner, int type) {
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) ++n;
    return n;
}
static int count_type(int owner, int type) {
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].type == type) ++n;
    return n;
}
static int find_hall(int owner) {
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == hall_type(race_player[owner])) return i;
    return -1;
}
static void row_to(int row) {
    while (race_front.row < row) press(KEY_DOWN);
    while (race_front.row > row) press(KEY_UP);
    assert(race_front.row == row);
}
static void cycle_field(int *field, int value) {
    int guard = 0;
    while (*field != value && guard++ < 3) press(KEY_RIGHT);
    assert(*field == value);
}
static void launch_skirmish(int map, int human, int bots) {
    game_init(); previous = 0;
    press(KEY_DOWN);                 /* HOME: custom game */
    press(KEY_A);
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_SKIRMISH);
    row_to(0); cycle_field(&race_front.map, map);
    row_to(1); cycle_field(&race_front.race, human);
    row_to(2); cycle_field(&race_front.bots, bots);
    for (int i = 0; i < bots; ++i) {
        int ai = (human + i + 1) % 3;
        row_to(3 + i);
        cycle_field(&race_front.bot_races[i], ai);
    }
    row_to(3 + bots);
    press(KEY_A);
    assert(screen == SCREEN_PLAY && race_mode == 1);
    assert(race_skirmish_map == map && race_human == human && race_bots == bots);
    for (int i = 0; i < bots; ++i)
        assert(race_bot_races[i] == (human + i + 1) % 3);
}
static void check_opening(int map, int human, int bots) {
    int expected_w = 64;
    int expected_h = expected_w;
    assert(race_cols == expected_w && race_rows == expected_h);
    for (int p = 0; p <= bots; ++p) {
        int owner = owner_at(p), race = p == 0 ? human : race_bot_races[p - 1];
        int hall = find_hall(owner);
        assert(race_player[owner] == race);
        assert(hall >= 0 && !b[hall].under_construction);
        int worker = race == R_ZERG ? U_DRONE : race == R_PROTOSS ? U_PROBE : U_SCV;
        assert(count_type(owner, worker) == 4);
        assert(count_buildings(owner) >= (race == R_ZERG ? 1 : 2));
        assert(race_base_x[p] >= 0 && race_base_x[p] < race_cols * 16);
        assert(race_base_y[p] >= 0 && race_base_y[p] < race_rows * 16);
        int cx = race_base_x[p] >> 4, cy = race_base_y[p] >> 4;
        for (int y = cy - 1; y <= cy + 1; ++y)
            for (int x = cx - 1; x <= cx + 1; ++x) assert(!terrain_blocked(x, y));
    }
    if (map > 0) {
        const Sc64SkirmishMap *mdata = &sc64_skirmish_maps[map - 1];
        int active = 0;
        assert(mdata->width == expected_w && mdata->height == expected_h);
        assert(mdata->minerals_count == 32);
        assert(geyser_count == mdata->geyser_count && geyser_count == 4);
        for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount) {
            ++active;
            assert(m[i].x >= 0 && m[i].x < expected_w * 16);
            assert(m[i].y >= 0 && m[i].y < expected_h * 16);
        }
        assert(active == mdata->minerals_count);
        for (int i = 0; i < geyser_count; ++i) {
            assert(geysers[i].x >= 0 && geysers[i].x < expected_w * 16);
            assert(geysers[i].y >= 0 && geysers[i].y < expected_h * 16);
        }
    }
}
static void check_save_reload(int map, int human, int bots) {
    int choices[3];
    for (int i = 0; i < 3; ++i) choices[i] = race_bot_races[i];
    press(KEY_START); assert(screen == SCREEN_PAUSE);
    press(KEY_B); /* save through the ordinary pause control */
    game_init(); previous = 0;
    press(KEY_L); /* load from HOME after fresh game initialization */
    assert(screen == SCREEN_PLAY && race_mode == 1);
    assert(race_skirmish_map == map && race_human == human && race_bots == bots);
    for (int i = 0; i < 3; ++i) assert(race_bot_races[i] == choices[i]);
    assert(race_cols == 64 && race_rows == race_cols);
}
static void check_invalid_save_rejection(void) {
    int old_screen = screen, old_mode = race_mode, old_map = race_skirmish_map;
    int old_human = race_human, old_cols = race_cols, old_rows = race_rows;
    int old_x = u[0].x, old_type = b[0].type;
    int old_races[3];
    for (int i = 0; i < 3; ++i) old_races[i] = race_bot_races[i];

    assert(race_save_game());
    race_data.state[20] = SC64_SKIRMISH_IMPORTED_COUNT + 1;
    save_write(&race_data, sizeof(race_data), SAVE_VERSION);
    assert(!race_load_game());
    assert(screen == old_screen && race_mode == old_mode && race_skirmish_map == old_map);
    assert(race_human == old_human && race_cols == old_cols && race_rows == old_rows);
    assert(u[0].x == old_x && b[0].type == old_type);
    for (int i = 0; i < 3; ++i) assert(race_bot_races[i] == old_races[i]);

#ifdef SC64_CAMPAIGN_RACE
    assert(race_save_game());
    int alien = -1;
    for (int i = 0; i < SC64_BASE_MISSION_COUNT; ++i)
        if (!sc64_catalog_mission_allowed(i, SC64_CAMPAIGN_RACE)) { alien = i; break; }
    assert(alien >= 0);
    race_data.state[0] = 0;
    race_data.state[2] = alien;
    race_data.campaign.mission = alien;
    save_write(&race_data, sizeof(race_data), SAVE_VERSION);
    assert(!race_load_game());
    assert(screen == old_screen && race_mode == old_mode && race_skirmish_map == old_map);
    assert(race_human == old_human && race_cols == old_cols && race_rows == old_rows);
    assert(u[0].x == old_x && b[0].type == old_type);
    for (int i = 0; i < 3; ++i) assert(race_bot_races[i] == old_races[i]);
#endif
}

int main(void) {
    gba_init();
    for (int map = 0; map <= SC64_SKIRMISH_IMPORTED_COUNT; ++map)
        for (int human = R_TER; human <= R_PROTOSS; ++human)
            for (int bots = 1; bots <= 3; ++bots) {
                launch_skirmish(map, human, bots);
                check_opening(map, human, bots);
                check_save_reload(map, human, bots);
            }

    /* Let the live simulation run for 10,000 bounded controller-neutral ticks.
       Read-only observation checks resource gathering and faction production. */
    launch_skirmish(1, R_TER, 3);
    int before_min = 0, before_units[4];
    for (int i = 0; i < MAX_MINERALS; ++i) before_min += m[i].amount;
    for (int p = 0; p < 4; ++p) before_units[p] = count_units(owner_at(p));
    int peak_workers[4] = {0}, peak_units[4] = {0}, peak_buildings[4] = {0};
    int saw_production[4] = {0}, saw_gas[4] = {0}, saw_refinery[4] = {0};
    for (int tick = 0; tick < 10000; ++tick) {
        frame_input(0);
        /* Observe ordinary progress as it happens. Later combat can wipe
           workers and structures, so the neutral fixture measures milestones
           reached during the run instead of survival at its arbitrary end. */
        for (int p = 0; p < 4; ++p) {
            int owner = owner_at(p);
            int workers = count_type(owner, race_player[owner] == R_ZERG ? U_DRONE :
                                     race_player[owner] == R_PROTOSS ? U_PROBE : U_SCV);
            int units = count_units(owner), buildings = count_buildings(owner);
            int refinery = race_player[owner] == R_ZERG ? B_EXTRACTOR :
                           race_player[owner] == R_PROTOSS ? B_ASSIMILATOR : B_REFINERY;
            if (workers > peak_workers[p]) peak_workers[p] = workers;
            if (units > peak_units[p]) peak_units[p] = units;
            if (buildings > peak_buildings[p]) peak_buildings[p] = buildings;
            if (units > before_units[p]) saw_production[p] = 1;
            if (race_owner_gas(owner) > 0) saw_gas[p] = 1;
            if (count_building_type(owner, refinery) > 0) saw_refinery[p] = 1;
        }
    }
    int after_min = 0;
    for (int i = 0; i < MAX_MINERALS; ++i) after_min += m[i].amount;
    assert(after_min < before_min);
    for (int p = 0; p < 4; ++p) {
        int owner = owner_at(p);
        if (p == 0) {
            /* The human base mines from the normal worker orders while input
               stays neutral; bots demonstrate production/build progression. */
            assert(race_owner_minerals(owner) > 200);
        } else {
            int refinery = race_player[owner] == R_ZERG ? B_EXTRACTOR :
                           race_player[owner] == R_PROTOSS ? B_ASSIMILATOR : B_REFINERY;
            assert(peak_workers[p] >= 5);
            assert(saw_production[p]);
            assert(peak_buildings[p] > (race_player[owner] == R_ZERG ? 1 : 2));
            assert(saw_refinery[p] || saw_gas[p]);
        }
    }
    printf("split profile=%d: launches=18; maps=2; human_races=3; ai_counts=1..3; "
           "economy_ticks=10000; neutral_minerals=%d->%d; human_bank=%d; "
           "bot_banks=(%d,%d,%d); bot_gas=(%d,%d,%d); buildings=(%d,%d,%d,%d)\n",
           SC64_CAMPAIGN_RACE, before_min, after_min, race_owner_minerals(SIDE),
           race_owner_minerals(ENEMY), race_owner_minerals(RESCUABLE), race_owner_minerals(4),
           race_owner_gas(ENEMY), race_owner_gas(RESCUABLE), race_owner_gas(4),
           count_buildings(SIDE), count_buildings(ENEMY), count_buildings(RESCUABLE), count_buildings(4));
    check_invalid_save_rejection();
    puts("split race skirmish controller/profile: ok");
    return 0;
}
