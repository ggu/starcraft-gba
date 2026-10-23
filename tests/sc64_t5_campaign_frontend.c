/* Asset-free catalog/frontend checks for the gated T5 campaign entry. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_frontend.h"

static int press(Sc64Frontend *f, unsigned key)
{
    int action = sc64_frontend_update(f, key);
    (void)sc64_frontend_update(f, 0);
    return action;
}

static void test_catalog_and_clamping(void)
{
    Sc64Frontend f;
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) ==
#ifdef SC64_T5_CAMPAIGN_ENTRY
           5
#else
           4
#endif
    );
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 2);
    assert(sc64_catalog_playable_count(SC64_FACTION_PROTOSS) == 2);
#ifdef SC64_T5_CAMPAIGN_ENTRY
    assert(sc64_catalog_is_playable(8));
    assert(sc64_catalog_mission_allowed(8, SC64_FACTION_TERRAN));
    assert(sc64_catalog_playable_at(SC64_FACTION_TERRAN, 4) == 8);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_TERRAN, 8) == 4);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 7) == 8);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 8) == -1);
    sc64_frontend_reset(&f, 8, SC_FRONT_RACE_TERRAN, 1, 99);
    assert(f.mission == 8 && f.completed == 8 && f.faction == SC_FRONT_TERRAN);
    assert(f.mission_page == 2 && f.row == 0);
    sc64_frontend_reset(&f, 29, SC_FRONT_RACE_TERRAN, 1, 99);
    assert(f.mission == 4 && f.completed == 8); /* invalid faction mission clamps to first */
#else
    assert(!sc64_catalog_is_playable(8));
    assert(!sc64_catalog_mission_allowed(8, SC64_FACTION_TERRAN));
    assert(sc64_catalog_playable_at(SC64_FACTION_TERRAN, 4) == -1);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 7) == -1);
    sc64_frontend_reset(&f, 8, SC_FRONT_RACE_TERRAN, 1, 99);
    assert(f.mission == 4 && f.completed == 7 && f.faction == SC_FRONT_TERRAN);
#endif
}

static void test_five_mission_paging_and_launch(void)
{
    Sc64Frontend f;
    sc64_frontend_reset(&f, 7, SC_FRONT_RACE_TERRAN, 1, 7);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_MISSION);
    assert(f.mission_page == 1 && f.row == 1);
    assert(sc64_front_mission_rows(&f) == 2);
#ifdef SC64_T5_CAMPAIGN_ENTRY
    press(&f, KEY_RIGHT);
    assert(f.mission_page == 2 && f.row == 0);
    assert(sc64_front_mission_rows(&f) == 1);
    press(&f, KEY_DOWN);
    assert(f.row == 0); /* final page has one selectable row */
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 8);
    assert(press(&f, KEY_B) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 2 && f.row == 0);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_BRIEFING);
    assert(f.mission == 8);
    assert(press(&f, KEY_START) == SC_FRONT_START_CAMPAIGN);
    assert(f.mission == 8 && f.completed == 7);
    assert(press(&f, KEY_B) == SC_FRONT_NONE && f.page == SC_FRONT_MISSION);
    assert(f.mission_page == 2 && f.row == 0);
#else
    assert(f.mission_page == 1 && f.row == 1);
    assert(sc64_front_mission_rows(&f) == 2);
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 7);
    assert(press(&f, KEY_B) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 1 && f.row == 1);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_BRIEFING);
    assert(f.mission == 7);
    assert(press(&f, KEY_START) == SC_FRONT_START_CAMPAIGN);
#endif
}

#ifdef SC64_IMPORTED_SKIRMISH
static void test_skirmish_is_unchanged(void)
{
    Sc64Frontend f;
    sc64_frontend_reset(&f, 4, SC_FRONT_RACE_TERRAN, 1, -1);
    press(&f, KEY_DOWN);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_SKIRMISH);
    assert(f.map == SC64_MAP_OUTPOST && f.race == SC_FRONT_RACE_TERRAN && f.bots == 1);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.map == SC64_MAP_BLOOD_BATH);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.race == SC_FRONT_RACE_ZERG);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 2);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bots == 2);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 3);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bot_races[0] == SC_FRONT_RACE_PROTOSS);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 4);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.row == 5);
    assert(press(&f, KEY_A) == SC_FRONT_START_SKIRMISH);
    assert(f.page == SC_FRONT_SKIRMISH && f.map == SC64_MAP_BLOOD_BATH && f.bots == 2);
}
#endif

int main(void)
{
    test_catalog_and_clamping();
    test_five_mission_paging_and_launch();
#ifdef SC64_IMPORTED_SKIRMISH
    test_skirmish_is_unchanged();
#endif
    puts("T5 campaign frontend gate checks passed");
    return 0;
}
