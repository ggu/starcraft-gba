/* Asset-free UI/catalog coverage for the opt-in combined T3/T4 campaign. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_frontend.h"

static int press(Sc64Frontend *f, unsigned key)
{
    int action = sc64_frontend_update(f, key);
    (void)sc64_frontend_update(f, 0);
    return action;
}

static void test_catalog_gate(void)
{
    assert(sc64_catalog_get(7)->faction == SC64_FACTION_TERRAN);
    assert(sc64_catalog_get(7)->chapter == 4);
    assert(sc64_catalog_mission_count(SC64_FACTION_TERRAN) == 10);
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) ==
#ifdef SC64_T4_CAMPAIGN_ENTRY
           4
#else
           3
#endif
    );
#ifdef SC64_T4_CAMPAIGN_ENTRY
    assert(sc64_catalog_is_playable(7));
    assert(sc64_catalog_mission_allowed(7, SC64_FACTION_TERRAN));
    assert(!sc64_catalog_mission_allowed(7, SC64_FACTION_ZERG));
    assert(sc64_catalog_playable_at(SC64_FACTION_TERRAN, 3) == 7);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_TERRAN, 7) == 3);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 6) == 7);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 7) == -1);
#else
    assert(!sc64_catalog_is_playable(7));
    assert(!sc64_catalog_mission_allowed(7, SC64_FACTION_TERRAN));
    assert(sc64_catalog_playable_at(SC64_FACTION_TERRAN, 3) == -1);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_TERRAN, 7) == -1);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 6) == -1);
#endif
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 2);
    assert(sc64_catalog_playable_count(SC64_FACTION_PROTOSS) == 2);
}

static void test_four_mission_paging_and_launch(void)
{
    Sc64Frontend f;
    sc64_frontend_reset(&f, 4, SC_FRONT_RACE_TERRAN, 1, 5);
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 4);
    assert(f.completed == 5);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_MISSION);
    assert(f.mission_page == 0 && f.row == 0);
    assert(sc64_front_mission_rows(&f) == 2);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE);
    assert(f.mission_page == 1 && f.row == 0);
#ifdef SC64_T4_CAMPAIGN_ENTRY
    assert(sc64_front_mission_rows(&f) == 2);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 7);
    assert(press(&f, KEY_B) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 1 && f.row == 1);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_BRIEFING);
    assert(f.mission == 7);
    assert(press(&f, KEY_START) == SC_FRONT_START_CAMPAIGN);
    assert(f.mission == 7 && f.completed == 5);
    assert(press(&f, KEY_B) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 1 && f.row == 1);
    sc64_frontend_reset(&f, 7, SC_FRONT_RACE_TERRAN, 1, 99);
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 7 && f.completed == 7);
#else
    assert(sc64_front_mission_rows(&f) == 1);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 0);
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 6);
    assert(press(&f, KEY_B) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 1 && f.row == 0);
    sc64_frontend_reset(&f, 7, SC_FRONT_RACE_TERRAN, 1, 99);
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 4 && f.completed == 6);
#endif
    /* The internal replay path remains separately gated from campaign entry. */
    sc64_frontend_reset(&f, 4, SC_FRONT_RACE_TERRAN, 1, -1);
    assert(sc64_catalog_is_playable(7) ==
#ifdef SC64_T4_CAMPAIGN_ENTRY
           1
#else
           0
#endif
    );
}

#ifdef SC64_IMPORTED_SKIRMISH
static void test_skirmish_selection_unchanged(void)
{
    Sc64Frontend f;
    sc64_frontend_reset(&f, 4, SC_FRONT_RACE_TERRAN, 1, -1);
    f.row = 1;
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_SKIRMISH);
    assert(f.map == SC64_MAP_OUTPOST);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.map == SC64_MAP_BLOOD_BATH);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.race == SC_FRONT_RACE_ZERG);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.race == SC_FRONT_RACE_PROTOSS);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.race == SC_FRONT_RACE_TERRAN);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.race == SC_FRONT_RACE_ZERG);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 2);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bots == 2);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bots == 3);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 3);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bot_races[0] == 2);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 4);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bot_races[1] == 0);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 5);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bot_races[2] == 2);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 6);
    assert(press(&f, KEY_A) == SC_FRONT_START_SKIRMISH);
    assert(f.race == SC_FRONT_RACE_ZERG && f.bots == 3);
}
#endif

int main(void)
{
    test_catalog_gate();
    test_four_mission_paging_and_launch();
#ifdef SC64_IMPORTED_SKIRMISH
    test_skirmish_selection_unchanged();
#endif
    puts("combined T3/T4 frontend catalog checks passed");
    return 0;
}
