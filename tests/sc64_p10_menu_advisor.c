/* SPDX-License-Identifier: GPL-3.0-or-later
 * Real menu inputs and imported advisor observer in P10 custom game. */
#include <assert.h>
#define main sc64_game_audio_other_main
#include "game_audio_run.c"
#undef main
static void research_row(void)
{
    tick(KEY_B); tick(0);
    assert(menu_open);
    for (int guard=0; menu_cursor!=MENU_WEAPONS && guard<64; ++guard) {
#ifdef SC64_COMMAND_ICONS
        if (card_fits() && card_command_slot(menu_cursor)%CARD_COLUMNS !=
                           card_command_slot(MENU_WEAPONS)%CARD_COLUMNS) tick(KEY_RIGHT);
        else
#endif
            tick(KEY_DOWN);
        tick(0);
    }
    assert(menu_cursor==MENU_WEAPONS);
}
int main(void)
{
    gba_init(); game_init(); race_mode=1; race_human=R_PROTOSS;
    race_bots=1; race_bot_races[0]=R_TER; race_skirmish_map=0; init_state();
    assert(screen==SCREEN_PLAY && race_mode && race_human==R_PROTOSS);
    int forge=spawn_build(SIDE,B_FORGE,race_base_x[0]+50,race_base_y[0]+50,0);
    assert(forge>=0); b[forge].raw_type=166;
    minerals=0; gas=1000; previous_held=0;
    tick(0); clear_events(); research_row();
    int old_frame=frame;
    tick(KEY_A);
    assert(frame==old_frame && toast_id==1 && events(GAME_AUDIO_ADVISOR)==1);
    game_render(); assert(events(GAME_AUDIO_ADVISOR)==1);
    tick(0); minerals=1000; gas=0; research_row(); old_frame=frame;
    tick(KEY_A);
    assert(frame==old_frame && toast_id==12 && events(GAME_AUDIO_ADVISOR)==2);
    game_render(); assert(events(GAME_AUDIO_ADVISOR)==2);
    puts("PASS P10 custom menu advisor cues");
    return 0;
}
