/* Integration checks for the privately generated two-mission campaign. */
#include <stdio.h>
#include "../starcraft/game.c"

static int
count(int owner, int type)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].type == type) ++n;
    return n;
}

static int
buildings(int owner, int type)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) ++n;
    return n;
}

static int
fail(const char *message)
{
    fprintf(stderr, "Campaign: %s\n", message);
    return 1;
}

int
main(void)
{
    int raynor = -1, i, blocked = 0;
    game_init();
    if (SC64_CAMPAIGN_COUNT != 2 || SC64_CAMPAIGN_T1_UNIT_COUNT != 46)
        return fail("generated mission descriptor size");
    for (i = 0; i < MAP_CELLS; ++i) blocked += (sc64_campaign_maps[0].blocked[i] & 1) != 0;
    if (!blocked)
        return fail("campaign collision grid unavailable");

    game_update(KEY_A, KEY_A);
    if (campaign_mission != 0 || buildings(SIDE, B_COMMAND) != 1 ||
        count(SIDE, U_SCV) < 4 || count(NEUTRAL, U_RAYNOR) != 1 ||
        supply_cap < 20 || mission_unsupported_count != 0)
        return fail("Wasteland starts with expected scenario and supply");
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_RAYNOR) raynor = i;
    if (raynor < 0) return fail("Raynor imported");
    frame = 30;
    update_imported_mission();
    if (mission_objective_stage != 1 || minerals != 40)
        return fail("Wasteland trigger grants opening resources");
    u[raynor].owner = SIDE;
    for (i = 0; i < MAX_UNITS && count(SIDE, U_MARINE) < 10; ++i)
        if (!u[i].alive) spawn_unit(SIDE, U_MARINE, 100 + (i % 3) * 8, 120 + (i / 3) * 8);
    if (!buildings(SIDE, B_BARRACKS)) spawn_build(SIDE, B_BARRACKS, 130, 130, 0);
    check_imported_mission_outcome();
    if (!won) return fail("Wasteland victory criteria remain active");

    screen = SCREEN_RESULT;
    game_update(KEY_A, KEY_A);
    if (campaign_mission != 1 || !(campaign_completed & 1) || screen != SCREEN_PLAY)
        return fail("A advances from Wasteland to Backwater");
    frame = 1;
    update_imported_mission();
    if (mission_unsupported_count || buildings(SIDE, B_COMMAND) < 1 ||
        (count(SIDE, U_RAYNOR) + count(NEUTRAL, U_RAYNOR)) != 1 ||
        backwater.outcome != SC64_BACKWATER_RUNNING) {
        return fail("Backwater mission initialized from its own descriptor");
    }
    if (!save_game()) return fail("campaign autosave fixture");
    campaign_mission = 0;
    campaign_completed = 0;
    if (!load_game() || campaign_mission != 1 || !(campaign_completed & 1))
        return fail("save/load preserves current mission and campaign progress");

    game_init();
    if (campaign_mission != 1 || !(campaign_completed & 1) || screen != SCREEN_TITLE)
        return fail("startup restores campaign selection and progress to title");
    campaign_mission = 0;
    init_state();
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_RAYNOR) { u[i].alive = 0; break; }
    lost = 0;
    check_imported_mission_outcome();
    if (!lost) return fail("Wasteland loss when Raynor is gone");
    puts("Campaign integration passed: map descriptors, T1 triggers/outcome, T2 dispatch, save/load, startup restore.");
    return 0;
}
