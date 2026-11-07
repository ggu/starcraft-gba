/* Exercise the live field-menu input path with real held/pressed edges. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static u16 previous;
static void tick(u16 held)
{
    game_update(held, held & (u16)~previous);
    previous = held;
}
static void release(void) { tick(0); }

int main(void)
{
    game_init();
    init_state();
    screen = SCREEN_PLAY;
    mode = MODE_NORMAL;
    previous = 0;
    tick(KEY_B); assert(menu_open && menu_cursor == 0);
    release(); tick(KEY_DOWN); assert(menu_cursor == 1);
    for (int i = 0; i < 17; ++i) tick(KEY_DOWN);
    assert(menu_cursor == 1);
    tick(KEY_DOWN); assert(menu_cursor == 2);
    for (int i = 0; i < 4; ++i) tick(KEY_DOWN);
    assert(menu_cursor == 2);
    tick(KEY_DOWN); assert(menu_cursor == 3);
    release(); tick(KEY_UP); assert(menu_cursor == 2);
    release(); tick(KEY_UP); assert(menu_cursor == 1);
    release(); tick(KEY_UP); assert(menu_cursor == 0);
    release(); tick(KEY_UP); assert(menu_cursor == field_menu_count() - 1);
    release(); tick(KEY_DOWN);
    for (int i = 0; i < 17; ++i) tick(KEY_DOWN);
    tick(KEY_B | KEY_DOWN); assert(!menu_open);
    release(); tick(KEY_B); assert(menu_open && menu_cursor == 0);
    tick(KEY_DOWN); assert(menu_cursor == 1);
    for (int i = 0; i < 17; ++i) tick(KEY_DOWN);
    assert(menu_cursor == 1);
    tick(KEY_DOWN); assert(menu_cursor == 2);
    release(); tick(KEY_B); assert(!menu_open);
    release(); tick(KEY_B | KEY_DOWN); assert(menu_open && menu_cursor == 1);
    release(); tick(KEY_DOWN | KEY_UP); assert(menu_cursor == 1);
    release(); tick(KEY_A); assert(!menu_open);
    for (int i = 0; i < 25; ++i) tick(KEY_A);
    assert(!menu_open);
    field_menu_repeat_direction = KEY_DOWN; field_menu_repeat_age = 17;
    game_init(); assert(!field_menu_repeat_direction && !field_menu_repeat_age);
    puts("field menu held repeat and input edges: ok");
    return 0;
}
