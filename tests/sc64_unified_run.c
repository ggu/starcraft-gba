/* Unified controller/menu, SRAM, and Terran objective coverage.
 * Compile with SC64_UNIFIED; the six campaign IDs are a save contract. */
#include <stdio.h>
#include "../starcraft/game.c"

static u16 previous;
static RaceSave beacon_valid_candidate;
static RaceSave beacon_invalid_candidate;
static RaceSave beacon_legacy_candidate;

static void frame_input(u16 held)
{
    game_update(held, held & (u16)~previous);
    previous = held;
}

static void press(u16 key)
{
    frame_input(key);
    frame_input(0);
}

static int fail(const char *message)
{
    fprintf(stderr, "Unified campaign: %s\n", message);
    return 1;
}

static int count_unit(int owner, int type)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].type == type) ++n;
    return n;
}

static int count_owner_units(int owner)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++n;
    return n;
}

static int count_building(int owner, int type)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) ++n;
    return n;
}

static int find_unit(int owner, int type)
{
    int i;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && (owner < 0 || u[i].owner == owner) && u[i].type == type) return i;
    return -1;
}

static int find_building(int owner, int type)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) return i;
    return -1;
}

static int faction_row(int mission)
{
    return mission >= 4 ? SC_FRONT_TERRAN :
           mission >= 2 ? SC_FRONT_PROTOSS : SC_FRONT_ZERG;
}

static int open_mission(int mission)
{
    int target = faction_row(mission), guard;
    game_init();
    previous = 0;
    press(KEY_A); /* HOME → CAMPAIGN */
    if (race_front.page != SC_FRONT_CAMPAIGN) return 0;
    for (guard = 0; guard < 3 && race_front.row != target; ++guard)
        press(KEY_DOWN);
    if (race_front.row != target) return 0;
    press(KEY_A); /* CAMPAIGN → the faction's two missions */
    if (race_front.page != SC_FRONT_MISSION || race_front.faction != target) return 0;
    if (mission & 1) press(KEY_DOWN);
    if (race_front.row != (mission & 1)) return 0;
    press(KEY_A); /* MISSION → BRIEFING */
    if (race_front.page != SC_FRONT_BRIEFING || race_front.mission != mission) return 0;
    frame_input(KEY_A); /* BRIEFING → game */
    if (screen != SCREEN_PLAY) return 0;
    /* Inspect immediately after the launch edge; the following release is a
     * real simulation update and can expose scenario initialization defects. */
    if (won || lost) return 0;
    frame_input(0);
    if (won || lost) {
        fprintf(stderr, "Unified mission %d ended on first simulation update: won=%d lost=%d side_units=%d raynor=%d\n",
                mission, won, lost, count_owner_units(SIDE), count_unit(SIDE, U_RAYNOR));
        return 0;
    }
    return screen == SCREEN_PLAY && !race_mode &&
           race_selected_mission == mission && race_campaign.mission == mission;
}

static int roundtrip_mission(int mission)
{
    int expected_race = mission >= 4 ? R_TER : mission >= 2 ? R_PROTOSS : R_ZERG;
    int zerg_beacon = -1;
    if (!open_mission(mission)) {
        fprintf(stderr, "  menu launch: page=%d row=%d faction=%d selected=%d campaign=%d screen=%d\n",
                race_front.page, race_front.row, race_front.faction,
                race_selected_mission, race_campaign.mission, screen);
        return 0;
    }
    if (race_human != expected_race) { fprintf(stderr, "  launch race=%d expected=%d\n", race_human, expected_race); return 0; }
    if (mission == SC64_RACE_MISSION_Z1) {
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].raw_type == 194) { zerg_beacon = i; break; }
        if (zerg_beacon < 0 || b[zerg_beacon].type != B_ZERG_BEACON ||
            b[zerg_beacon].hp != 100000 || b[zerg_beacon].maxhp != 100000)
            return 0;
    }
    frame_input(KEY_START);
    frame_input(0);
    press(KEY_B); /* pause B writes the campaign save */
    if (screen != SCREEN_PAUSE) { fprintf(stderr, "  pause before save: screen=%d\n", screen); return 0; }
    if (zerg_beacon >= 0 &&
        (race_data.buildings[zerg_beacon][offsetof(Building,hp)/sizeof(int)] !=
             SC64_LEGACY_BEACON_HP_SENTINEL ||
         race_data.buildings[zerg_beacon][offsetof(Building,maxhp)/sizeof(int)] !=
             SC64_LEGACY_BEACON_HP_SENTINEL)) return 0;
    if (zerg_beacon >= 0) {
        beacon_valid_candidate = race_data;
        beacon_invalid_candidate = beacon_valid_candidate;
        beacon_invalid_candidate.buildings[zerg_beacon]
            [offsetof(Building,maxhp)/sizeof(int)] =
                SC64_LEGACY_BEACON_HP_SENTINEL + 1;
        save_write(&beacon_invalid_candidate,sizeof(beacon_invalid_candidate),
                   SAVE_VERSION);
        if (race_load_game() || b[zerg_beacon].hp != 100000 ||
            b[zerg_beacon].maxhp != 100000) return 0;
        beacon_legacy_candidate = beacon_valid_candidate;
        beacon_legacy_candidate.buildings[zerg_beacon]
            [offsetof(Building,hp)/sizeof(int)] = 10000;
        beacon_legacy_candidate.buildings[zerg_beacon]
            [offsetof(Building,maxhp)/sizeof(int)] = 10000;
        save_write(&beacon_legacy_candidate,sizeof(beacon_legacy_candidate),
                   SAVE_VERSION);
        if (!race_load_game() || b[zerg_beacon].hp != 10000 ||
            b[zerg_beacon].maxhp != 10000) return 0;
        if (!race_save_game() ||
            race_data.buildings[zerg_beacon]
                [offsetof(Building,hp)/sizeof(int)] != 10000 ||
            race_data.buildings[zerg_beacon]
                [offsetof(Building,maxhp)/sizeof(int)] != 10000) return 0;
        beacon_legacy_candidate.buildings[zerg_beacon]
            [offsetof(Building,hp)/sizeof(int)] = 4321;
        save_write(&beacon_legacy_candidate,sizeof(beacon_legacy_candidate),
                   SAVE_VERSION);
        if (!race_load_game() || b[zerg_beacon].hp != 4321 ||
            b[zerg_beacon].maxhp != 10000 || !race_save_game() ||
            race_data.buildings[zerg_beacon]
                [offsetof(Building,hp)/sizeof(int)] != 4321 ||
            race_data.buildings[zerg_beacon]
                [offsetof(Building,maxhp)/sizeof(int)] != 10000) return 0;
        save_write(&beacon_valid_candidate,sizeof(beacon_valid_candidate),
                   SAVE_VERSION);
        race_data = beacon_valid_candidate;
    }
    game_init();
    previous = 0;
    press(KEY_B); /* HOME B restores the saved run */
    if (!(screen == SCREEN_PLAY && !race_mode &&
           race_selected_mission == mission && race_campaign.mission == mission &&
           race_human == expected_race)) {
        fprintf(stderr, "  HOME load: screen=%d mode=%d selected=%d campaign=%d race=%d\n",
                screen, race_mode, race_selected_mission, race_campaign.mission, race_human);
        return 0;
    }
    if (zerg_beacon >= 0 &&
        (b[zerg_beacon].raw_type != 194 || b[zerg_beacon].type != B_ZERG_BEACON ||
         b[zerg_beacon].hp != 100000 || b[zerg_beacon].maxhp != 100000)) return 0;
    return 1;
}

static int terran_objective_fixtures(void)
{
    int i, raynor = -1;
    if (!open_mission(SC64_RACE_MISSION_T1)) return 0;
    for (i = 0; i < 31; ++i) frame_input(0);
    if (minerals != 40) return 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_RAYNOR) { raynor = i; break; }
    if (raynor < 0 || u[raynor].owner != NEUTRAL) return 0;
    u[raynor].owner = SIDE;
    for (i = 0; i < 10; ++i)
        if (spawn_unit(SIDE, U_MARINE, 120 + (i % 5) * 8, 120 + (i / 5) * 8) < 0)
            return 0;
    if (spawn_build(SIDE, B_BARRACKS, 176, 176, 0) < 0) return 0;
    race_campaign_tick();
    if (!won || lost || !(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY))
        return 0;
    screen=SCREEN_RESULT;
    press(KEY_A);
    if(screen!=SCREEN_PLAY||race_selected_mission!=SC64_RACE_MISSION_T2||
       !(race_completed&(1<<SC64_RACE_MISSION_T1)))return 0;

    if (!open_mission(SC64_RACE_MISSION_T2)) return 0;
    if (backwater.outcome != SC64_BACKWATER_RUNNING ||
        !count_building(SIDE, B_COMMAND) || find_unit(-1, U_RAYNOR) < 0) return 0;
    /* These authored-world fixtures exercise original triggers; they do not
     * claim a player-driven full T2 playthrough. */
    {
        int marine = find_unit(SIDE, U_MARINE), bunker, command;
        int left, top, right, bottom, timer_before, invincible_before;
        if (marine < 0) return 0;
        raynor = find_unit(-1, U_RAYNOR);
        u[marine].x = u[raynor].x; u[marine].y = u[raynor].y;
        race_campaign_tick();
        if (u[raynor].owner != SIDE) return 0;

        bunker = find_building(RESCUABLE, B_BUNKER);
        if (bunker < 0 || !bw_location_bounds(SC64_BW_LOC_FIREBAT_BUNKER_2,
                                              &left, &top, &right, &bottom)) return 0;
        b[bunker].x = (left + right) / 2; b[bunker].y = (top + bottom) / 2;
        u[marine].x = b[bunker].x; u[marine].y = b[bunker].y;
        race_campaign_tick();
        if (bw_count(0, SC64_BW_OUTPOST, SC64_BW_FIREBAT,
                     SC64_BW_LOC_FIREBAT_BUNKER_2) != 2) return 0;
        race_campaign_tick();
        if (bw_count(0, SC64_BW_SIDE, SC64_BW_FIREBAT,
                     SC64_BW_LOC_FIREBAT_BUNKER_2) != 2) return 0;

        command = find_building(RESCUABLE, B_COMMAND);
        marine = find_unit(SIDE, U_MARINE);
        if (command < 0 || marine < 0) return 0;
        i = supply_cap;
        u[marine].x = b[command].x; u[marine].y = b[command].y;
        race_campaign_tick();
        if (b[command].owner != SIDE || supply_cap != i + 20) return 0;

        /* The Infested Command Center is the source victory gate. Removing it
         * starts the ending wait and makes Raynor invincible. */
        command = find_building(ENEMY, B_INFESTED_COMMAND);
        if (command < 0) return 0;
        hit_target(MAX_UNITS + command, 5000, U_MARINE, SIDE);
        race_campaign_tick();
        if (backwater.phase[14] != 1 || !backwater.timer[14] || won || lost ||
            !u[raynor].alive || !invincible_units[raynor]) return 0;
        timer_before = backwater.timer[14];
        invincible_before = invincible_units[raynor];
        if (!save_game()) return 0;
        backwater.timer[14] = 0; backwater.phase[14] = 0;
        invincible_units[raynor] = 0;
        if (!load_game() || backwater.phase[14] != 1 ||
            backwater.timer[14] != timer_before || invincible_units[raynor] != invincible_before)
            return 0;

        for (i = 0; i < 500 && backwater.outcome == SC64_BACKWATER_RUNNING; ++i)
            race_campaign_tick();
        if (backwater.outcome != SC64_BACKWATER_WON || !won || lost ||
            race_campaign.phase != SC64_RACE_PHASE_COMPLETE ||
            !(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) ||
            !(race_completed & (1 << SC64_RACE_MISSION_T2)) || !u[raynor].alive)
            return 0;

        race_completed = 0x3f;
        if (!save_game()) return 0;
        race_completed = 0;
        if (!load_game() || race_completed != 0x3f) return 0;
    }
    return 1;
}

int main(void)
{
    int mission;
    for (mission = 0; mission < 6; ++mission) {
        if (!roundtrip_mission(mission)) {
            fprintf(stderr, "Unified campaign: mission %d failed screen=%d selected=%d campaign=%d race=%d/%d mode=%d\n",
                    mission, screen, race_selected_mission, race_campaign.mission,
                    race_human, mission >= 4 ? R_TER : mission >= 2 ? R_PROTOSS : R_ZERG,
                    race_mode);
            return 1;
        }
    }
    if (!terran_objective_fixtures()) return fail("Terran mission objective fixture failed");
    puts("Unified campaign: six mission menu/SRAM paths and Terran objectives passed.");
    return 0;
}
