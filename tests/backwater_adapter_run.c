/* Adapter integration fixture over the generated private Backwater scenario. */
#include <stdio.h>
#include "../starcraft/game.c"

static int
find_unit(int owner, int type)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == owner && u[i].type == type) return i;
    return -1;
}

static int
find_building(int owner, int type)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) return i;
    return -1;
}

static int
fail(const char *message)
{
    fprintf(stderr, "Backwater adapter: %s\n", message);
    return 1;
}

int
main(void)
{
    int hero, marine, bunker, firebats, i, mobile, buildings, minerals_count;
    game_init();
    game_update(KEY_A, KEY_A);
    if (minerals != 200 || gas != 200) return fail("initial ore/gas grant");
    hero = find_unit(NEUTRAL, U_RAYNOR);
    marine = find_unit(SIDE, U_MARINE);
    if (hero < 0 || marine < 0) return fail("imported hero or human squad missing");
    mobile = bw_count(0, SC64_BW_ALL_PLAYERS, SC64_BW_MEN,
                      SC64_BW_LOC_ANYWHERE);
    buildings = bw_count(0, SC64_BW_ALL_PLAYERS, SC64_BW_BUILDINGS,
                         SC64_BW_LOC_ANYWHERE);
    minerals_count = 0;
    for (i = 0; i < MAX_MINERALS; i++) if (m[i].amount > 0) minerals_count++;
    if (mobile != 61 || buildings != 17 || minerals_count != 13 || geyser_count != 2)
        return fail("imported Backwater placement counts");
    if (bw_count(0, SC64_BW_ALL_PLAYERS, SC64_BW_RAYNOR_VULTURE,
                 SC64_BW_LOC_ANYWHERE) != 1 ||
        bw_count(0, SC64_BW_FORCE_2, SC64_BW_INFESTED_CC,
                 SC64_BW_LOC_ANYWHERE) != 1 || mission_unsupported_count)
        return fail("owner, infested Command Center, or unit mapping");

    for (i = 0; i < 300; i++) game_update(0, 0);
    if (won || lost || sc64_backwater_outcome(&backwater) != SC64_BACKWATER_RUNNING)
        return fail("mission ended before the original trigger gate");
    hero = find_unit(SIDE, U_RAYNOR);
    if (hero < 0) hero = find_unit(NEUTRAL, U_RAYNOR);
    if (hero < 0) return fail("Raynor was lost during the opening");

    /* Proximity transfers the authored neutral hero and updates supply use. */
    u[marine].x = u[hero].x;
    u[marine].y = u[hero].y;
    backwater_update();
    if (u[hero].owner != SIDE) return fail("Raynor did not join the player");

    /* Reach the first Firebat bunker; its source trigger creates two allies. */
    bunker = find_building(RESCUABLE, B_BUNKER);
    if (bunker < 0) return fail("rescuable bunker missing");
    {
        int left, top, right, bottom;
        if (!bw_location_bounds(SC64_BW_LOC_FIREBAT_BUNKER_2,
                                &left, &top, &right, &bottom))
            return fail("generated region lookup failed");
        b[bunker].x = (left + right) / 2;
        b[bunker].y = (top + bottom) / 2;
    }
    u[marine].x = b[bunker].x;
    u[marine].y = b[bunker].y;
    backwater_update();
    firebats = bw_count(0, SC64_BW_OUTPOST, SC64_BW_FIREBAT,
                        SC64_BW_LOC_FIREBAT_BUNKER_2);
    if (firebats != 2) return fail("bunker rescue did not create two Firebats");
    backwater_update();
    if (bw_count(0, SC64_BW_SIDE, SC64_BW_FIREBAT,
                 SC64_BW_LOC_FIREBAT_BUNKER_2) != 2)
        return fail("rescued Firebats did not transfer to the player");

    /* Rescuing the settlement Command Center adds supply and persists. */
    bunker = find_building(RESCUABLE, B_COMMAND);
    marine = find_unit(SIDE, U_MARINE);
    if (bunker < 0 || marine < 0) return fail("settlement Command Center missing");
    u[marine].x = b[bunker].x;
    u[marine].y = b[bunker].y;
    i = supply_cap;
    backwater_update();
    if (b[bunker].owner != SIDE || supply_cap != i + 20)
        return fail("settlement Command Center rescue/supply");
    if (!save_game()) return fail("saving rescued Command Center");
    init_state();
    if (!load_game()) return fail("loading rescued Command Center");
    if (find_building(SIDE, B_COMMAND) < 0 || supply_cap != i + 20)
        return fail("rescued Command Center did not survive save/load");

    /* Destroying the actual Force 2 Infested Command Center starts the source
     * ending wait; no victory is awarded while it remains alive. */
    bunker = find_building(ENEMY, B_INFESTED_COMMAND);
    if (bunker < 0) return fail("enemy Infested Command Center missing");
    hit_target(MAX_UNITS + bunker, 5000, U_MARINE, SIDE);
    if (won || lost) return fail("mission outcome bypassed the ending sequence");
    for (i = 0; i < 5000; i++) backwater_update();
    if (!won || lost || !sc64_backwater_objective(&backwater))
        return fail("source ending did not produce victory");

    /* Result presentation flags are transient; the mission outcome is saved.
     * The first update after loading must restore the completed result screen. */
    if (!save_game()) return fail("saving completed victory");
    init_state();
    if (!load_game()) return fail("loading completed victory");
    backwater_update();
    if (!won || lost || screen != SCREEN_RESULT)
        return fail("loaded victory did not restore the result screen");

    /* Exercise the same persistence path for a valid defeated mission state. */
    backwater.objective = 0;
    backwater.outcome = SC64_BACKWATER_LOST;
    if (!save_game()) return fail("saving completed defeat");
    init_state();
    if (!load_game()) return fail("loading completed defeat");
    backwater_update();
    if (won || !lost || screen != SCREEN_RESULT)
        return fail("loaded defeat did not restore the result screen");

    return 0;
}
