/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "../starcraft/game.c"

static void press(u16 key)
{
    game_update(key, key);
    game_update(0, 0);
}

static void test_saved_repeat_command(int human)
{
    game_init();
    press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_SKIRMISH);
    press(KEY_DOWN);
    for (int i = 0; i < 3 && race_front.race != human; ++i) press(KEY_RIGHT);
    assert(race_front.race == human);
    press(KEY_DOWN);
    for (int i = 0; i < 3 && race_front.bots != 1; ++i) press(KEY_RIGHT);
    assert(race_front.bots == 1);
    press(KEY_A);
    press(KEY_A);
    assert(screen == SCREEN_PLAY && race_mode && race_human == human);
    for (int step = 0; step < 60; ++step) game_update(0, 0);

    int base = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (b[i].alive && b[i].owner == SIDE) { base = i; break; }
    }
    assert(base >= 0);

    /* Exercise the genuine production writer for a legacy zero UI word. */
    assert(race_save_game() && race_data.state[SC64_SAVE_UI_STATE_INDEX] == 0);
    menu_last = MENU_ATTACK;
    assert(race_load_game() && menu_last == MENU_MOVE);

    selected_building = base;
    menu_cursor = MENU_STOP;
    menu_last = MENU_ATTACK;
    assert(race_save_game());
    assert((unsigned)race_data.state[SC64_SAVE_UI_STATE_INDEX] ==
           sc64_save_ui_encode(-1, MAX_BUILDINGS, 0, MENU_ATTACK));
    menu_cursor = 0;
    menu_last = 0;
    assert(race_load_game());
    assert(selected_building == -1 && !menu_open && menu_cursor == 0 && menu_last == MENU_ATTACK);
    game_update(KEY_B, KEY_B);
    assert(menu_open && menu_cursor == MENU_ATTACK);

    /* The codec accepts prior full-context tags and bounds the command. */
    assert(sc64_save_ui_valid(sc64_save_ui_encode(
        base, MAX_BUILDINGS, MENU_STOP, MENU_ATTACK), MAX_BUILDINGS));
    assert(!sc64_save_ui_valid(SC64_SAVE_UI_TAG | (64u << 16), MAX_BUILDINGS));
}

int main(void)
{
    gba_init();
    for (int race = R_TER; race <= R_PROTOSS; ++race)
        test_saved_repeat_command(race);
    return 0;
}
