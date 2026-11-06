/* SPDX-License-Identifier: GPL-3.0-or-later */
/* P10 custom-game command card, driven through game_update input edges. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#ifndef SC64_COMMAND_ICONS
#error "This fixture exercises the imported P10 command card"
#endif
static u16 previous;
static void tick(u16 held) { game_update(held, held & (u16)~previous); previous=held; }
static void release(void) { tick(0); }
int main(void)
{
    gba_init(); game_init(); race_mode=1; race_human=R_PROTOSS;
    race_bots=1; race_bot_races[0]=R_TER; race_skirmish_map=0; init_state();
    int producer=-1;
    for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive && b[i].owner==SIDE && b[i].type==B_NEXUS) { producer=i; break; }
    assert(producer>=0); selected_building=producer; previous=0;
    tick(KEY_B); assert(menu_open && card_fits() && selected_building==producer);
    int initial=card_command_slot(menu_cursor); assert(initial==0);
    release(); tick(KEY_DOWN); assert(card_command_slot(menu_cursor)==3);
    for(int i=0;i<17;i++) tick(KEY_DOWN);
    assert(card_command_slot(menu_cursor)==3);
    tick(KEY_DOWN); assert(card_command_slot(menu_cursor)==6);
    release(); tick(KEY_UP); assert(card_command_slot(menu_cursor)==3);
    release(); tick(KEY_RIGHT); assert(card_command_slot(menu_cursor)==4);
    for(int i=0;i<17;i++) tick(KEY_RIGHT);
    assert(card_command_slot(menu_cursor)==4);
    tick(KEY_RIGHT); assert(card_command_slot(menu_cursor)==5);
    tick(KEY_LEFT); assert(card_command_slot(menu_cursor)==4);
    for(int i=0;i<17;i++) tick(KEY_LEFT);
    assert(card_command_slot(menu_cursor)==4);
    tick(KEY_LEFT); assert(card_command_slot(menu_cursor)==3);
    release(); tick(KEY_UP); assert(card_command_slot(menu_cursor)==0);
    release(); tick(KEY_UP); assert(card_command_slot(menu_cursor)==(field_menu_count()-1)/3*3);
    release(); tick(KEY_DOWN); assert(card_command_slot(menu_cursor)==0);
    release(); tick(KEY_B); assert(!menu_open);
    release(); tick(KEY_B); assert(menu_open && card_command_slot(menu_cursor)==0);
    release(); tick(KEY_A); assert(!menu_open);
    for(int i=0;i<25;i++) tick(KEY_A);
    assert(!menu_open);
    field_menu_repeat_direction=KEY_DOWN; field_menu_repeat_age=17;
    init_state(); assert(!field_menu_repeat_direction && !field_menu_repeat_age);
    puts("P10 custom producer command card repeat and edges: ok");
    return 0;
}
