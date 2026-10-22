#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_frontend.h"

static int press(Sc64Frontend *f, unsigned key) {
    int action = sc64_frontend_update(f, key);
    (void)sc64_frontend_update(f, 0);
    return action;
}

static void test_reset_and_home(void) {
    Sc64Frontend f;
    sc64_frontend_reset(&f, 3, 8, 0, 99);
    assert(f.page == SC_FRONT_HOME && f.row == 0);
    assert(f.mission == 3 && f.faction == SC_FRONT_PROTOSS);
    assert(f.race == SC_FRONT_RACE_PROTOSS && f.bots == 1 &&
#ifdef SC64_UNIFIED
           f.completed == 5);
#else
           f.completed == 3);
#endif
    assert(press(&f, KEY_A) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_CAMPAIGN && f.faction == SC_FRONT_PROTOSS);
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

#ifdef SC64_UNIFIED
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
    sc64_frontend_reset(&f, 0, SC_FRONT_RACE_TERRAN, 1, -1);
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

int main(void) {
    test_reset_and_home();
    test_campaign_pages_and_retention();
    test_skirmish_setup_and_quickload();
    test_wrap_edges_and_ticks();
#ifdef SC64_UNIFIED
    test_unified_campaign_mission_mapping();
    test_unified_skirmish_all_races();
#endif
    puts("sc64 frontend input: ok");
    return 0;
}
