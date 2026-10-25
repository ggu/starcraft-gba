#include <assert.h>
#include <stdio.h>
#ifdef SC64_SHOWCASE_ENABLED
#include <string.h>
#endif
#include "../starcraft/sc64_frontend.h"

static int press(Sc64Frontend *f, unsigned key) {
    int action = sc64_frontend_update(f, key);
    (void)sc64_frontend_update(f, 0);
    return action;
}

#ifdef SC64_SHOWCASE_ENABLED
static void test_hidden_showcase(void) {
    Sc64Frontend f;
    sc64_frontend_reset(&f, 0, SC_FRONT_RACE_TERRAN, 1, -1);

    /* Hold R, add L, then tap SELECT. The staged chord takes precedence over
       L quick-load on HOME. */
    assert(sc64_frontend_update(&f, KEY_R) == SC_FRONT_NONE);
    (void)sc64_frontend_update(&f, 0);
    assert(sc64_frontend_update(&f, KEY_R | KEY_L) == SC_FRONT_NONE);
    (void)sc64_frontend_update(&f, 0);
    assert(sc64_frontend_update(&f, KEY_L | KEY_R | KEY_SELECT) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_SHOWCASE && f.row == 0);
    (void)sc64_frontend_update(&f, 0);
    assert(press(&f, KEY_L) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_SHOWCASE);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_UP) == SC_FRONT_NONE && f.row == 0);
    assert(press(&f, KEY_B) == SC_FRONT_NONE && f.page == SC_FRONT_HOME);

    /* The chord is HOME-only, and all eight scene indices wrap cleanly. */
    f.page = SC_FRONT_SHOWCASE;
    f.row = 7;
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 0);
    assert(press(&f, KEY_UP) == SC_FRONT_NONE && f.row == 7);
    assert(press(&f, KEY_A) == SC_FRONT_START_SHOWCASE && f.row == 7);
    assert(strcmp(sc64_front_showcase_name(0), "CARRIER APPROACH") == 0);

    sc64_frontend_reset(&f, 0, SC_FRONT_RACE_TERRAN, 1, -1);
    assert(press(&f, KEY_L | KEY_SELECT) == SC_FRONT_LOAD);
    assert(f.page == SC_FRONT_HOME);
}
#endif

#ifndef SC64_CAMPAIGN_RACE
static void test_reset_and_home(void) {
    Sc64Frontend f;
    sc64_frontend_reset(&f, 3, 8, 0, 99);
    assert(f.page == SC_FRONT_HOME && f.row == 0);
    assert(f.mission == 3 && f.faction ==
#ifdef SC64_CAMPAIGN_RACE
           SC_FRONT_TERRAN);
#else
           SC_FRONT_PROTOSS);
#endif
    assert(f.race == SC_FRONT_RACE_PROTOSS && f.bots == 1 &&
#ifdef SC64_UNIFIED
#ifdef SC64_T3_ENABLED
           f.completed == 6);
#else
           f.completed == 5);
#endif
#else
           f.completed == 3);
#endif
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
#ifdef SC64_CAMPAIGN_RACE
    assert(f.page == SC_FRONT_MISSION && f.faction == SC_FRONT_TERRAN);
#else
    assert(f.page == SC_FRONT_CAMPAIGN && f.faction == SC_FRONT_PROTOSS);
#endif
    assert(press(&f, KEY_B) == SC_FRONT_NONE && f.page == SC_FRONT_HOME);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_SKIRMISH);
    assert(press(&f, KEY_B) == SC_FRONT_NONE && f.page == SC_FRONT_HOME && f.row == 1);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 2);
    assert(press(&f, KEY_A) == SC_FRONT_LOAD);
    assert(press(&f, KEY_B) == SC_FRONT_LOAD);
}

static void test_campaign_pages_and_retention(void) {
    Sc64Frontend f;
    sc64_frontend_reset(&f, 2, SC_FRONT_RACE_ZERG, 2, 0);
    assert(press(&f, KEY_A) == SC_FRONT_NONE); /* home -> faction */
    assert(f.page == SC_FRONT_CAMPAIGN && f.row == SC_FRONT_PROTOSS);
    assert(press(&f, KEY_LEFT) == SC_FRONT_NONE);
    assert(f.faction == SC_FRONT_ZERG && f.row == SC_FRONT_ZERG);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_MISSION);
    assert(f.faction == SC_FRONT_ZERG && f.row == 0);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 1);
    assert(press(&f, KEY_B) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION && f.row == 1 && f.mission == 1);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_BRIEFING);
    assert(press(&f, KEY_A) == SC_FRONT_START_CAMPAIGN);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 1); /* stable at launch */
}

static void test_skirmish_setup_and_quickload(void) {
    Sc64Frontend f;
    sc64_frontend_reset(&f, 0, SC_FRONT_RACE_TERRAN, 3, -1);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_SKIRMISH);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_LEFT) == SC_FRONT_NONE && f.bots == 2);
    assert(press(&f, KEY_UP) == SC_FRONT_NONE && f.row == 0);
    assert(press(&f, KEY_LEFT) == SC_FRONT_NONE && f.race == SC_FRONT_RACE_PROTOSS);
    assert(press(&f, KEY_UP) == SC_FRONT_NONE && f.row == 2);
    assert(press(&f, KEY_A) == SC_FRONT_START_SKIRMISH);
    assert(f.page == SC_FRONT_SKIRMISH && f.race == SC_FRONT_RACE_PROTOSS && f.bots == 2);
    assert(press(&f, KEY_L) == SC_FRONT_LOAD);
}

static void test_wrap_edges_and_ticks(void) {
    Sc64Frontend f;
    unsigned before;
    sc64_frontend_reset(&f, -9, -3, 9, -5);
    assert(f.mission == 0 && f.faction == SC_FRONT_ZERG);
    assert(f.race == 0 && f.bots == 3 && f.completed == -1);
    assert(sc64_frontend_update(&f, KEY_DOWN) == SC_FRONT_NONE);
    assert(f.row == 1);
    before = f.ticks;
    assert(sc64_frontend_update(&f, KEY_DOWN) == SC_FRONT_NONE);
    assert(f.row == 1); /* held direction does not auto-repeat */
    assert(f.ticks == before + 1);
    (void)sc64_frontend_update(&f, 0);
    assert(sc64_frontend_update(&f, KEY_DOWN) == SC_FRONT_NONE);
    assert(f.row == 2);
    assert(press(&f, KEY_UP) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_UP) == SC_FRONT_NONE && f.row == 0);
}
#endif

static void test_campaign_catalog_contract(void) {
    assert(SC64_BASE_MISSION_COUNT == 30);
    assert(sc64_catalog_mission_faction(0) == SC64_FACTION_ZERG);
    assert(sc64_catalog_mission_faction(1) == SC64_FACTION_ZERG);
    assert(sc64_catalog_mission_faction(2) == SC64_FACTION_PROTOSS);
    assert(sc64_catalog_mission_faction(3) == SC64_FACTION_PROTOSS);
    assert(sc64_catalog_mission_faction(4) == SC64_FACTION_TERRAN);
    assert(sc64_catalog_mission_faction(5) == SC64_FACTION_TERRAN);
    assert(sc64_catalog_mission_at(SC64_FACTION_TERRAN, 2) == 6);
    assert(sc64_catalog_mission_at(SC64_FACTION_ZERG, 2) == 14);
    assert(sc64_catalog_mission_at(SC64_FACTION_PROTOSS, 2) == 22);
    assert(sc64_catalog_mission_count(SC64_FACTION_TERRAN) == 10);
    assert(sc64_catalog_is_playable(5));
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    assert(sc64_catalog_is_playable(6));
    assert(sc64_catalog_mission_allowed(6, SC64_FACTION_TERRAN));
    assert(sc64_catalog_get(6)->title[0] == 'D');
#ifdef SC64_T4_CAMPAIGN_ENTRY
#ifdef SC64_TERRAN_FULL_CAMPAIGN_ENTRY
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) == 10);
    for (int ordinal = 0; ordinal < 10; ++ordinal) {
        assert(sc64_catalog_playable_at(SC64_FACTION_TERRAN, ordinal) == ordinal + 4);
        assert(sc64_catalog_playable_ordinal(SC64_FACTION_TERRAN, ordinal + 4) == ordinal);
    }
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 12) == 13);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 13) == -1);
#elif defined(SC64_T6_CAMPAIGN_ENTRY)
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) == 6);
    for (int ordinal = 0; ordinal < 6; ++ordinal) {
        assert(sc64_catalog_playable_at(SC64_FACTION_TERRAN, ordinal) == ordinal + 4);
        assert(sc64_catalog_playable_ordinal(SC64_FACTION_TERRAN, ordinal + 4) == ordinal);
    }
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 8) == 9);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 9) == -1);
#else
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) == 4);
    assert(sc64_catalog_playable_at(SC64_FACTION_TERRAN, 3) == 7);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_TERRAN, 7) == 3);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 6) == 7);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 7) == -1);
    assert(sc64_catalog_mission_allowed(7, SC64_FACTION_TERRAN));
#endif
#else
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) == 3);
    assert(sc64_catalog_playable_at(SC64_FACTION_TERRAN, 2) == 6);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_TERRAN, 6) == 2);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 5) == 6);
#endif
#else
    assert(!sc64_catalog_is_playable(6));
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) == 2);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_TERRAN, 6) == -1);
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, 5) == -1);
#endif
    assert(sc64_catalog_mission_allowed(4, SC64_FACTION_TERRAN));
    assert(!sc64_catalog_mission_allowed(4, SC64_FACTION_ZERG));
    assert(sc64_catalog_available_next(SC64_FACTION_ZERG, 0) == 1);
    assert(sc64_catalog_available_next(SC64_FACTION_ZERG, 1) == -1);
}

#if defined(SC64_UNIFIED) && !defined(SC64_CAMPAIGN_RACE)
static void test_unified_campaign_mission_mapping(void) {
    Sc64Frontend f;
    const int expected_faction[6]={SC_FRONT_ZERG,SC_FRONT_ZERG,
        SC_FRONT_PROTOSS,SC_FRONT_PROTOSS,SC_FRONT_TERRAN,SC_FRONT_TERRAN};
    for (int mission=0;mission<6;mission++) {
        sc64_frontend_reset(&f, mission, SC_FRONT_RACE_TERRAN, 1, mission);
        assert(f.mission==mission);
        assert(f.faction==expected_faction[mission]);
        assert(f.completed==mission);
        assert(sc64_frontend_mission_faction(mission)==expected_faction[mission]);
    }
    assert(sc64_frontend_first_mission(SC_FRONT_ZERG)==0);
    assert(sc64_frontend_first_mission(SC_FRONT_PROTOSS)==2);
    assert(sc64_frontend_first_mission(SC_FRONT_TERRAN)==4);

    /* Enter each campaign from HOME, open both missions, and exercise B. */
    for (int mission=0;mission<6;mission++) {
        sc64_frontend_reset(&f, mission, SC_FRONT_RACE_TERRAN, 1, -1);
        press(&f, KEY_A);
        assert(f.page==SC_FRONT_CAMPAIGN);
        press(&f, KEY_A);
        assert(f.page==SC_FRONT_MISSION &&
               f.faction==expected_faction[mission]);
        assert(f.row==mission-sc64_frontend_first_mission(expected_faction[mission]));
        press(&f, KEY_A);
        assert(f.page==SC_FRONT_BRIEFING && f.mission==mission);
        press(&f, KEY_B);
        assert(f.page==SC_FRONT_MISSION && f.mission==mission);
        press(&f, KEY_B);
        assert(f.page==SC_FRONT_CAMPAIGN &&
               f.row==sc64_front_campaign_row(expected_faction[mission]));
    }

    /* Campaign rows are Terran, Zerg, Protoss, while IDs remain stable. */
    sc64_frontend_reset(&f, 0, SC_FRONT_RACE_TERRAN, 3, -1);
    press(&f, KEY_A);
    assert(f.page==SC_FRONT_CAMPAIGN && f.row==1);
    press(&f, KEY_LEFT);
    assert(f.row==0 && f.faction==SC_FRONT_TERRAN);
    press(&f, KEY_A);
    assert(f.page==SC_FRONT_MISSION && f.faction==SC_FRONT_TERRAN && f.row==0);
    press(&f, KEY_DOWN);
    press(&f, KEY_A);
    assert(f.page==SC_FRONT_BRIEFING && f.mission==5);
    press(&f, KEY_B);
    assert(f.page==SC_FRONT_MISSION && f.row==1);
    press(&f, KEY_B);
    assert(f.page==SC_FRONT_CAMPAIGN && f.row==0);
}

static void test_unified_skirmish_all_races(void) {
    Sc64Frontend f;
    sc64_frontend_reset(&f, 4, SC_FRONT_RACE_TERRAN, 1, -1);
    press(&f, KEY_DOWN);
    press(&f, KEY_A);
    assert(f.page==SC_FRONT_SKIRMISH);
    for (int race=SC_FRONT_RACE_TERRAN;race<=SC_FRONT_RACE_PROTOSS;race++) {
        assert(f.race==race);
        press(&f, KEY_RIGHT);
    }
    assert(f.race==SC_FRONT_RACE_TERRAN);
}
#endif

#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
static void test_t3_paging_retention_and_reset(void) {
    Sc64Frontend f;
#ifndef SC64_CAMPAIGN_RACE
    sc64_frontend_reset(&f, 5, SC_FRONT_RACE_TERRAN, 1, 5);
    assert(f.mission == 5 && f.mission_page == 0);
    press(&f, KEY_A);
    assert(f.page == SC_FRONT_CAMPAIGN && f.faction == SC_FRONT_TERRAN);
    press(&f, KEY_A);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 0 && f.row == 1);
    press(&f, KEY_RIGHT);
    assert(f.mission_page == 1 && f.row == 0);
    assert(sc64_front_mission_rows(&f) == 1);
    assert(sc64_catalog_playable_at(SC64_FACTION_TERRAN,
                                    f.mission_page * 2 + f.row) == 6);
    press(&f, KEY_DOWN);
    assert(f.row == 0);
    press(&f, KEY_A);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 6);
    press(&f, KEY_B);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 1 && f.row == 0);
    press(&f, KEY_A);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 6);
    assert(press(&f, KEY_A) == SC_FRONT_START_CAMPAIGN);
    press(&f, KEY_B);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 1 && f.row == 0);
    press(&f, KEY_B);
    assert(f.page == SC_FRONT_CAMPAIGN && f.row == SC_FRONT_TERRAN);
#endif

    sc64_frontend_reset(&f, 6, SC_FRONT_RACE_TERRAN, 1, 6);
#ifdef SC64_CAMPAIGN_RACE
#if SC64_CAMPAIGN_RACE == 0
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 6 && f.mission_page == 1);
#ifdef SC64_T4_CAMPAIGN_ENTRY
    /* T4 follows T3 by catalog ordinal and occupies row 1 on page 1. */
    assert(sc64_catalog_available_next(SC64_FACTION_TERRAN, f.mission) == 7);
    press(&f, KEY_A);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 1 && f.row == 0);
    press(&f, KEY_DOWN);
    assert(f.row == 1);
    press(&f, KEY_A);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 7);
    press(&f, KEY_B);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 1 && f.row == 1);
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_BRIEFING);
    assert(press(&f, KEY_A) == SC_FRONT_START_CAMPAIGN);
    assert(f.mission == 7);
#endif
#ifdef SC64_T6_CAMPAIGN_ENTRY
    sc64_frontend_reset(&f, 9, SC_FRONT_RACE_TERRAN, 1, 9);
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 9 && f.mission_page == 2);
    press(&f, KEY_A);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 2 && f.row == 1);
    press(&f, KEY_A);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 9);
    assert(press(&f, KEY_A) == SC_FRONT_START_CAMPAIGN && f.mission == 9);
#endif
#ifdef SC64_TERRAN_FULL_CAMPAIGN_ENTRY
    sc64_frontend_reset(&f, 13, SC_FRONT_RACE_TERRAN, 1, 13);
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 13 && f.mission_page == 4);
    press(&f, KEY_A);
    assert(f.page == SC_FRONT_MISSION && f.mission_page == 4 && f.row == 1);
    press(&f, KEY_UP);
    assert(f.row == 0);
    press(&f, KEY_A);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 12);
    assert(press(&f, KEY_A) == SC_FRONT_START_CAMPAIGN && f.mission == 12);
#endif
    sc64_frontend_reset(&f, 30, SC_FRONT_RACE_TERRAN, 1, 99);
#ifdef SC64_TERRAN_FULL_CAMPAIGN_ENTRY
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 4 && f.completed == 13);
#elif defined(SC64_T6_CAMPAIGN_ENTRY)
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 4 && f.completed == 9);
#elif defined(SC64_T4_CAMPAIGN_ENTRY)
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 4 && f.completed == 7);
#else
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 4 && f.completed == 6);
#endif
#elif SC64_CAMPAIGN_RACE == 1
    assert(f.faction == SC_FRONT_ZERG && f.mission == 0);
#elif SC64_CAMPAIGN_RACE == 2
    assert(f.faction == SC_FRONT_PROTOSS && f.mission == 2);
#endif
#else
    assert(f.faction == SC_FRONT_TERRAN && f.mission == 6 && f.mission_page == 1);
    sc64_frontend_reset(&f, 30, SC_FRONT_RACE_TERRAN, 1, 99);
    assert(f.faction == SC_FRONT_ZERG && f.mission == 0 && f.completed == 6);
#endif
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 2);
    assert(sc64_catalog_playable_count(SC64_FACTION_PROTOSS) == 2);
}
#endif

int main(void) {
#ifdef SC64_SHOWCASE_ENABLED
    test_hidden_showcase();
#endif
#ifdef SC64_CAMPAIGN_RACE
    Sc64Frontend f;
    const int profile_faction = SC64_CAMPAIGN_RACE == 0 ? SC_FRONT_TERRAN :
                                SC64_CAMPAIGN_RACE == 1 ? SC_FRONT_ZERG : SC_FRONT_PROTOSS;
    sc64_frontend_reset(&f, 0, SC_FRONT_RACE_TERRAN, 3, -1);
    assert(f.faction == profile_faction);
    assert(sc64_front_campaign_count() == 1);
    assert(sc64_front_campaign_faction_at(0) == profile_faction);
    assert(f.mission == sc64_frontend_first_mission(profile_faction));
    assert(press(&f, KEY_A) == SC_FRONT_NONE && f.page == SC_FRONT_MISSION);
    assert(f.faction == profile_faction);
#ifdef SC64_IMPORTED_SKIRMISH
    assert(press(&f, KEY_B) == SC_FRONT_NONE && f.page == SC_FRONT_CAMPAIGN);
    assert(f.row == 0 && f.faction == profile_faction);
#endif
#ifdef SC64_IMPORTED_SKIRMISH
    f.page = SC_FRONT_SKIRMISH; f.race = SC_FRONT_RACE_TERRAN; f.bots = 3; f.row = 0;
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.map == 1);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 1);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.race == 1);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 2);
    assert(press(&f, KEY_LEFT) == SC_FRONT_NONE && f.bots == 2);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bots == 3);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 3);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bot_races[0] == 2);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 4);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bot_races[1] == 0);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 5);
    assert(press(&f, KEY_RIGHT) == SC_FRONT_NONE && f.bot_races[2] == 2);
    assert(press(&f, KEY_DOWN) == SC_FRONT_NONE && f.row == 6);
    assert(press(&f, KEY_A) == SC_FRONT_START_SKIRMISH);
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    test_t3_paging_retention_and_reset();
#endif
#else
    test_reset_and_home();
    test_campaign_pages_and_retention();
    test_skirmish_setup_and_quickload();
    test_wrap_edges_and_ticks();
#ifdef SC64_IMPORTED_SKIRMISH
    {
        Sc64Frontend f;
        sc64_frontend_reset(&f,0,0,1,-1);
        press(&f,KEY_DOWN); press(&f,KEY_A);
        assert(f.page==SC_FRONT_SKIRMISH && f.map==SC64_MAP_OUTPOST);
        assert(press(&f,KEY_RIGHT)==SC_FRONT_NONE && f.map==SC64_MAP_BLOOD_BATH);
        assert(press(&f,KEY_RIGHT)==SC_FRONT_NONE && f.map==SC64_MAP_OUTPOST);
    }
#endif
#endif
    test_campaign_catalog_contract();
#if defined(SC64_UNIFIED) && !defined(SC64_CAMPAIGN_RACE)
    test_unified_campaign_mission_mapping();
    test_unified_skirmish_all_races();
#if defined(SC64_T3_ENABLED)
    test_t3_paging_retention_and_reset();
#endif
#endif
    puts("sc64 frontend input: ok");
    return 0;
}
