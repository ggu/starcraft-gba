/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static u16 previous;

static void controller(u16 held)
{
    game_update(held, held & (u16)~previous);
    previous = held;
}

static void press(u16 key)
{
    controller(key);
    controller(0);
}

int main(void)
{
    gba_init();
    game_init();
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
#ifdef SC64_SHOWCASE_ENABLED
    controller(KEY_R);
    controller(KEY_L | KEY_R);
    controller(KEY_L | KEY_R | KEY_SELECT);
    controller(0);
    assert(race_front.page == SC_FRONT_SHOWCASE);
    press(KEY_B);
#endif
    press(KEY_A);
#ifndef SC64_CAMPAIGN_RACE
    assert(race_front.page == SC_FRONT_CAMPAIGN);
    press(KEY_A);
#endif
    assert(race_front.page == SC_FRONT_MISSION);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING);
    int mission = race_front.mission;
    press(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && race_selected_mission == mission);
    game_init();

    press(KEY_DOWN);
    press(KEY_RIGHT);
    assert(race_front.page == SC_FRONT_OPTIONS);
    press(KEY_RIGHT);
    press(KEY_DOWN);
    press(KEY_A);
    press(KEY_B);
    assert(race_front.page == SC_FRONT_HOME && race_front.row == 1);
    assert(race_front.settings.cursor_delta == 1 && race_front.settings.quiet_menu);

    press(KEY_A);
    assert(race_front.page == SC_FRONT_SKIRMISH);
    int first_map = race_front.map;
    press(KEY_SELECT);
    assert(race_front.page == SC_FRONT_MAP_BROWSER);
    press(KEY_DOWN);
    press(KEY_B);
    assert(race_front.page == SC_FRONT_SKIRMISH && race_front.map == first_map);
    press(KEY_SELECT);
    press(KEY_DOWN);
    press(KEY_A);
    int map = race_front.map;
    assert(race_front.page == SC_FRONT_SKIRMISH && map != first_map);

    press(KEY_DOWN);
    press(KEY_RIGHT);
    int human = race_front.race;
    press(KEY_DOWN);
    press(KEY_RIGHT);
    assert(race_front.bots == 3);
    for (int row = 2; row < SC_FRONT_SKIRMISH_LAUNCH_ROW; ++row) press(KEY_DOWN);
    press(KEY_A);
    assert(screen == SCREEN_PLAY && race_mode && race_human == human);
    assert(race_bots == 3 && race_skirmish_map == map);

    int cursor_x = cx;
    press(KEY_RIGHT);
    assert(cx == cursor_x + 3);
    press(KEY_START);
    assert(screen == SCREEN_PAUSE);
    int saved_frame = frame;
    press(KEY_B);
    assert((pause_notice & 0xc0) == UI_NOTICE_SAVED);
    press(KEY_RIGHT);
    press(KEY_A);
    assert(field_pause_page == FIELD_PAUSE_OBJECTIVES && frame == saved_frame);
    press(KEY_B);
    press(KEY_DOWN);
    press(KEY_A);
    assert(field_pause_page == FIELD_PAUSE_SETTINGS);
    press(KEY_LEFT);
    press(KEY_LEFT);
    assert(race_front.settings.cursor_delta == -1);
    press(KEY_START);
    assert(screen == SCREEN_PLAY);
    cursor_x = cx;
    press(KEY_RIGHT);
    assert(cx == cursor_x + 1);

    press(KEY_START);
    assert(field_pause_page == FIELD_PAUSE_ROOT && screen == SCREEN_PAUSE);
    press(KEY_RIGHT);
    press(KEY_UP);
    press(KEY_UP);
    press(KEY_A);
    assert(field_pause_page == FIELD_PAUSE_CONFIRM_EXIT && field_pause_row == 0);
    press(KEY_A);
    assert(screen == SCREEN_PAUSE && field_pause_page == FIELD_PAUSE_MORE);
    press(KEY_A);
    press(KEY_DOWN);
    controller(KEY_A);
    controller(KEY_A);
    controller(0);
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    assert(race_front.settings.cursor_delta == -1 && race_front.settings.quiet_menu);

    press(KEY_DOWN);
    press(KEY_DOWN);
    controller(KEY_A);
    assert(screen == SCREEN_PLAY && frame == saved_frame);
    controller(0);
    assert(race_mode && race_human == human && race_bots == 3 && race_skirmish_map == map);
    assert(race_front.settings.cursor_delta == -1 && race_front.settings.quiet_menu);
    puts("SC64 menu session, pause and saved skirmish: ok");
    return 0;
}
