/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST player controller: game state changes only through game_update(). */
#define main sc64_skirmish_controller_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main
#include <string.h>
extern void guardian_test_reset(void);
static int observe_egg(void)
{
    for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].owner==SIDE &&
        u[i].type==SC64_EGG_ACTOR && u[i].target==U_DRONE) return i;
    return -1;
}
static int menu_to(int command)
{
    if(menu_open)tap(KEY_B);
    tap(KEY_B);
    for(int n=0;menu_open && menu_cursor!=command && n<64;n++) {
#ifdef SC64_COMMAND_ICONS
        if(card_fits()) {
            int slot=card_command_slot(menu_cursor),col=card_command_slot(command)%CARD_COLUMNS;
            int row_length=field_menu_count()-(slot/CARD_COLUMNS)*CARD_COLUMNS;
            if(row_length>CARD_COLUMNS)row_length=CARD_COLUMNS;
            if(slot%CARD_COLUMNS!=col)tap(col>=row_length?KEY_UP:KEY_RIGHT);
            else tap(KEY_DOWN);
        } else
#endif
            tap(KEY_DOWN);
    }
    return menu_open && menu_cursor==command;
}
int main(void)
{
    guardian_test_reset();option_map=0;option_human=R_ZERG;option_bots=1;
    option_bot_races[0]=R_TER;match_limit=30000;
    assert(launch_match());
    int hall=-1;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive && b[i].owner==SIDE &&
        sc64_race_is_zerg_townhall(b[i].type)){hall=i;break;}
    assert(hall>=0 && b[hall].larva>0 && minerals>=sc64_race_unit_rule(U_DRONE)->minerals);
    printf("start tick=%u bank=%d larva=%d hall=%d\n",host_ticks,minerals,b[hall].larva,hall);
    assert(select_building(hall));
    int found=0;for(int i=0;i<16;i++) {
        if(train_type==U_DRONE){found=1;break;}tap(KEY_R);
    }
    assert(found && selected_building==hall);int ore=minerals,larva=b[hall].larva;
    assert(observe_egg()<0);tap(KEY_A);int egg=observe_egg();
    assert(egg>=0 && b[hall].larva==larva-1 && minerals<=ore-sc64_race_unit_rule(U_DRONE)->minerals);
    printf("paid tick=%u egg=%d timer=%d bank=%d\n",host_ticks,egg,u[egg].work_timer,minerals);
    tap(KEY_START);assert(screen==SCREEN_PAUSE);tap(KEY_B);
    assert((pause_notice&0xc0)==UI_NOTICE_SAVED);tap(KEY_START);assert(screen==SCREEN_PLAY);
    int saved_timer=u[egg].work_timer;
    tap(KEY_SELECT);assert(selected_building<0);assert(select_unit(egg));
    int row=race_context_command_base(RACE_CONTEXT_EGG);
    assert(row<race_skirmish_command_count() && !strcmp(race_menu_label(row),"CANCEL EGG"));
    assert(menu_to(row));int before=minerals;tap(KEY_A);
    assert(!u[egg].alive && minerals>=before+sc64_race_unit_rule(U_DRONE)->minerals);
    printf("canceled tick=%u bank=%d\n",host_ticks,minerals);
    tap(KEY_START);assert(screen==SCREEN_PAUSE);tap(KEY_L);
    assert(screen==SCREEN_PLAY && u[egg].alive && u[egg].type==SC64_EGG_ACTOR &&
           u[egg].work_timer==saved_timer);
    unsigned guard=0;while(u[egg].alive && u[egg].type==SC64_EGG_ACTOR && guard++<2048)input(0);
    assert(u[egg].alive && u[egg].type==U_DRONE);
    printf("loaded and born tick=%u egg=%d\n",host_ticks,egg);
    puts("ordinary HOST input: select, pay, save, cancel, load, birth PASS");return 0;
}
