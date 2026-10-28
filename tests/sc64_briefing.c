/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/sc64_frontend.h"

static int press(Sc64Frontend *f, unsigned key) {
    int action = sc64_frontend_update(f, key);
    (void)sc64_frontend_update(f, 0);
    return action;
}

static void test_static_page_model(void) {
    static const Sc64BriefingPage pages[] = {
        {"ADMIRAL", "Hold this position.\nReinforcements are coming.", 3},
        {"OBJECTIVES", "Destroy the enemy command center.", -1}
    };
    const Sc64Briefing briefing = {pages, 2};

    assert(briefing.pages == pages && briefing.page_count == 2);
    assert(strcmp(briefing.pages[0].speaker, "ADMIRAL") == 0);
    assert(briefing.pages[0].portrait == 3);
    assert(strcmp(briefing.pages[1].speaker, "OBJECTIVES") == 0);
    assert(briefing.pages[1].portrait == -1);
    assert(SC64_BRIEFING_COLUMNS == 108u && SC64_BRIEFING_ROWS == 6u);
}

static void test_reveal_advance_and_wrap(void) {
    Sc64Frontend f;
    unsigned start_tick;

    sc64_frontend_reset(&f, 0, SC_FRONT_RACE_TERRAN, 1, -1);
    assert(f.briefing_page == 0 && f.briefing_pages == 1);
    assert(f.briefing_reveal_length == 0 && f.briefing_revealed == 0);

    f.page = SC_FRONT_BRIEFING;
    sc64_frontend_briefing_configure(&f, 2, 5);
    start_tick = f.ticks;
    assert(sc64_frontend_briefing_visible(&f) == 0);
    (void)sc64_frontend_update(&f, 0);
    assert(sc64_frontend_briefing_visible(&f) == 2);
    assert(press(&f, KEY_A) == SC_FRONT_NONE); /* reveal remaining text */
    assert(sc64_frontend_briefing_visible(&f) == 5);

    assert(press(&f, KEY_A) == SC_FRONT_NONE); /* advance */
    assert(f.briefing_page == 1 && f.briefing_page_tick > start_tick);
    assert(f.briefing_revealed == 0);
    sc64_frontend_briefing_configure(&f, 2, 4);
    assert(sc64_frontend_briefing_visible(&f) == 2); /* release frame elapsed */
    (void)sc64_frontend_update(&f, 0);
    assert(sc64_frontend_briefing_is_revealed(&f));
    assert(press(&f, KEY_A) == SC_FRONT_START_CAMPAIGN);

    /* Explicit page navigation is bounded and restarts typewriter playback. */
    f.page = SC_FRONT_BRIEFING;
    f.briefing_page = 0;
    sc64_frontend_briefing_configure(&f, 2, 8);
    (void)press(&f, KEY_LEFT);
    assert(f.briefing_page == 0);
    (void)press(&f, KEY_RIGHT);
    assert(f.briefing_page == 1 && f.briefing_revealed == 0);
    assert(sc64_frontend_briefing_visible(&f) == 2); /* release frame elapsed */
    (void)press(&f, KEY_RIGHT);
    assert(f.briefing_page == 1);
    (void)press(&f, KEY_LEFT);
    assert(f.briefing_page == 0 && f.briefing_revealed == 0);
}

static void test_start_back_and_limits(void) {
    Sc64Frontend f;
    sc64_frontend_reset(&f, 0, SC_FRONT_RACE_TERRAN, 1, -1);
    f.page = SC_FRONT_BRIEFING;
    sc64_frontend_briefing_configure(&f, 0, SC64_BRIEFING_MAX_TEXT_BYTES + 100u);
    assert(f.briefing_pages == 1);
    assert(f.briefing_reveal_length == SC64_BRIEFING_MAX_TEXT_BYTES);
    assert(press(&f, KEY_L) == SC_FRONT_LOAD); /* global quick-load still works */
    assert(f.page == SC_FRONT_BRIEFING);
    assert(press(&f, KEY_START) == SC_FRONT_START_CAMPAIGN);

    f.page = SC_FRONT_BRIEFING;
    f.briefing_page = 0;
    sc64_frontend_briefing_configure(&f, 2, 100);
    assert(press(&f, KEY_B) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_MISSION && f.briefing_page == 0);
}

int main(void) {
    test_static_page_model();
    test_reveal_advance_and_wrap();
    test_start_back_and_limits();
    puts("sc64 briefing page model: ok");
    return 0;
}
