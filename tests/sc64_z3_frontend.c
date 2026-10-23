#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_frontend.h"

static int press(Sc64Frontend *f, unsigned key) {
    int action = sc64_frontend_update(f, key);
    (void)sc64_frontend_update(f, 0);
    return action;
}

static int profile_faction(void) {
#ifdef SC64_CAMPAIGN_RACE
    return SC64_CAMPAIGN_RACE;
#else
    return -1;
#endif
}

static void test_catalog_mask_and_gaps(void) {
    int profile = profile_faction();
    unsigned all = sc64_catalog_playable_mask(-1);
    unsigned filtered = sc64_catalog_playable_mask(profile);
    int expected_terran = 2;
#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)
    expected_terran = 3;
#endif
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    assert(sc64_catalog_is_playable(14));
    assert(sc64_catalog_mission_allowed(14, SC64_FACTION_ZERG));
    assert(sc64_catalog_available_next(SC64_FACTION_ZERG, 1) == 14);
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 3);
    assert(sc64_catalog_playable_at(SC64_FACTION_ZERG, 2) == 14);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_ZERG, 14) == 2);
#else
    assert(!sc64_catalog_is_playable(14));
    assert(sc64_catalog_available_next(SC64_FACTION_ZERG, 1) == -1);
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 2);
#endif
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) == expected_terran);
    assert(sc64_catalog_playable_count(SC64_FACTION_PROTOSS) == 2);

    for (int id = 7; id <= 13; ++id) {
        assert(!sc64_catalog_is_playable(id));
        assert(!(all & (1u << id)));
        assert(!(filtered & (1u << id)));
    }
    for (int id = 15; id < SC64_BASE_MISSION_COUNT; ++id)
        assert(!(all & (1u << id)));

#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    assert((all & (1u << 14)) != 0);
    if (profile == SC64_FACTION_TERRAN || profile == SC64_FACTION_PROTOSS)
        assert(!(filtered & (1u << 14)));
    else {
        assert((filtered & (1u << 14)) != 0);
        assert(sc64_catalog_mission_allowed(14, profile));
    }
#else
    assert(!(all & (1u << 14)));
#endif
}

static void test_zerg_page_two_and_briefing(void) {
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    Sc64Frontend f;
    sc64_frontend_reset(&f, 0, SC_FRONT_RACE_TERRAN, 2, 1);

#ifdef SC64_CAMPAIGN_RACE
#if SC64_CAMPAIGN_RACE == 1
    assert(f.faction == SC_FRONT_ZERG);
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION);
#else
    /* A fixed non-Zerg profile cannot enter or select the Zerg mission. */
    sc64_frontend_reset(&f, 14, SC_FRONT_RACE_ZERG, 2, 14);
    assert(f.mission != 14 && f.mission == sc64_frontend_first_mission(f.faction));
    assert(f.faction == SC64_CAMPAIGN_RACE);
    assert(sc64_catalog_playable_ordinal(SC64_CAMPAIGN_RACE, 14) == -1);
    return;
#endif
#else
    assert(press(&f, KEY_A) == SC_FRONT_NONE); /* HOME -> campaign selector */
    assert(f.page == SC_FRONT_CAMPAIGN && f.faction == SC_FRONT_ZERG);
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION && f.faction == SC_FRONT_ZERG);
#endif

    assert(f.mission_page == 0);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 1 && f.row == 0);
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 14);
    assert(press(&f, KEY_B) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION && f.mission == 14 &&
           f.mission_page == 1 && f.row == 0);
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 14);
    assert(press(&f, KEY_A) == SC_FRONT_START_CAMPAIGN);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 14);
#endif
}

static void test_reset_clamps_and_completion_retention(void) {
    Sc64Frontend f;
#ifdef SC64_CAMPAIGN_RACE
#if SC64_CAMPAIGN_RACE == 0
    const int first_mission = 4;
#elif SC64_CAMPAIGN_RACE == 1
    const int first_mission = 0;
#else
    const int first_mission = 2;
#endif
#else
    const int first_mission = 0;
#endif
    sc64_frontend_reset(&f, -1, SC_FRONT_RACE_TERRAN, 2, 14);
    assert(f.mission == first_mission);
    assert(f.mission_page == 0 && f.row == 0);

    sc64_frontend_reset(&f, 99, SC_FRONT_RACE_PROTOSS, 2, 14);
    assert(f.mission == first_mission);
    assert(f.mission_page == 0 && f.row == 0);
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    assert(f.completed == 14);
#elif defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)
    assert(f.completed == 6);
#else
    assert(f.completed == 5);
#endif

#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    sc64_frontend_reset(&f, 14, SC_FRONT_RACE_ZERG, 2, 14);
#ifdef SC64_CAMPAIGN_RACE
#if SC64_CAMPAIGN_RACE == 1
    assert(f.mission == 14 && f.faction == SC_FRONT_ZERG);
#else
    assert(f.mission == sc64_frontend_first_mission(f.faction));
#endif
#else
    assert(f.mission == 14 && f.faction == SC_FRONT_ZERG);
#endif
    assert(f.completed == 14);
#endif
}

static void test_skirmish_choices_remain_independent(void) {
    Sc64Frontend f;
    sc64_frontend_reset(&f, 0, SC_FRONT_RACE_TERRAN, 1, -1);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE);
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_SKIRMISH);

#ifdef SC64_IMPORTED_SKIRMISH
    assert(f.row == 0);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.race == SC_FRONT_RACE_ZERG);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 2);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bots == 2);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 3);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE);
    assert(f.bot_races[0] >= 0 && f.bot_races[0] <= 2);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 4);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 5);
    assert(press(&f, KEY_A) == SC_FRONT_START_SKIRMISH);
#else
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.race == SC_FRONT_RACE_ZERG);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bots == 2);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 2);
    assert(press(&f, KEY_A) == SC_FRONT_START_SKIRMISH);
#endif
    assert(f.page == SC_FRONT_SKIRMISH && f.race == SC_FRONT_RACE_ZERG && f.bots == 2);
}

int main(void) {
    test_reset_clamps_and_completion_retention();
    test_catalog_mask_and_gaps();
    test_zerg_page_two_and_briefing();
    test_skirmish_choices_remain_independent();
    puts("sc64 Z3 frontend tests passed");
    return 0;
}
