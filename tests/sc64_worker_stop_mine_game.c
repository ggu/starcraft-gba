/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Seed a worker and field, then exercise selection, STOP and mining by input. */
#define main sc64_worker_stop_embedded_main
#include "sc64_skirmish_runtime.c"
#undef main
static void tap(unsigned key){game_update((u16)key,(u16)key);game_update(0,0);}
int main(void)
{
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();screen=SCREEN_PLAY;
    int worker=spawn_unit(SIDE,U_SCV,600,600);assert(worker>=0);
    memset(m,0,sizeof(m));
    m[0].x=632;m[0].y=600;m[0].amount=1500;
    u[worker].order=ORDER_MINE;u[worker].target=0;u[worker].carry=0;
    clear_selection();cx=600;cy=600;update_exploration();
    tap(KEY_A);assert(u[worker].selected && selected_count()==1);
    tap(KEY_B);assert(menu_open);
    int stop=-1;for(int i=0;i<field_menu_count();i++)if(!strcmp(race_menu_label(i),"STOP / HOLD"))stop=i;
    assert(stop>=0);
    for(int i=0;i<field_menu_count() && menu_cursor!=stop;i++)tap(KEY_DOWN);
    assert(menu_cursor==stop);tap(KEY_A);assert(u[worker].order==ORDER_IDLE);
    int x=u[worker].x,y=u[worker].y;
    for(int i=0;i<120;i++){game_update(0,0);assert(u[worker].alive&&u[worker].order==ORDER_IDLE);}
    assert(u[worker].x==x&&u[worker].y==y);
    for(int guard=0;cx<640 && guard<40;guard++)game_update(KEY_RIGHT,0);
    game_update(0,0);assert(dist2(cx,cy,m[0].x,m[0].y)<16*16);
    assert(currently_visible(m[0].x,m[0].y));
    assert(field_ui_a_action(selected_count())==UI_A_MINE);tap(KEY_A);
    assert(u[worker].order==ORDER_MINE&&u[worker].target==0);
    /* Explicit input cannot inspect or assign an unseen field. */
    u[worker].order=ORDER_IDLE;u[worker].target=-1;u[worker].carry=0;
    SC64_GRID_SET(visible,m[0].x/16+(m[0].y/16)*MAP_COLS,0);
    tap(KEY_A);assert(u[worker].order!=ORDER_MINE);
    /* A loaded gas trip cannot be retyped as mineral cargo or discarded. */
    int refinery=spawn_build(SIDE,B_REFINERY,700,700,0);assert(refinery>=0);
    cx=u[worker].x;cy=u[worker].y;tap(KEY_A);assert(u[worker].selected);
    cx=m[0].x+8;cy=m[0].y;
    u[worker].order=ORDER_GAS;u[worker].target=refinery;u[worker].carry=8;
    SC64_GRID_SET(visible,m[0].x/16+(m[0].y/16)*MAP_COLS,1);
    assert(currently_visible(m[0].x,m[0].y));
    tap(KEY_A);assert(u[worker].order==ORDER_GAS && u[worker].target==refinery && u[worker].carry==8);
    /* Mixed group: loaded gas worker is preserved, empty worker is assigned. */
    u[worker].x=500;u[worker].y=600;
    int second=spawn_unit(SIDE,U_SCV,520,600);assert(second>=0);
    u[second].selected=1;u[second].order=ORDER_IDLE;u[second].carry=0;
    assert(u[worker].selected && u[worker].carry==8);
    tap(KEY_A);
    assert(u[worker].order==ORDER_GAS && u[worker].target==refinery && u[worker].carry==8);
    assert(u[second].order==ORDER_MINE && u[second].target==0);
    /* Armed attack command keeps its own dispatch priority over the patch. */
    clear_selection();u[second].selected=1;tap(KEY_B);assert(menu_open);
    int attack=-1;for(int i=0;i<field_menu_count();i++)if(!strcmp(race_menu_label(i),"ATTACK"))attack=i;
    assert(attack>=0);for(int i=0;i<field_menu_count() && menu_cursor!=attack;i++)tap(KEY_DOWN);
    assert(menu_cursor==attack);tap(KEY_A);assert(mode==MODE_ATTACK);
    assert(field_ui_a_action(selected_count())==UI_A_ATTACK_HERE);
    cx=m[0].x+8;cy=m[0].y;tap(KEY_A);assert(u[second].order==ORDER_ATTACK_MOVE);
    /* A friendly actor on a patch keeps existing selection priority. */
    int overlapped=spawn_unit(SIDE,U_MARINE,m[0].x,m[0].y);assert(overlapped>=0);
    clear_selection();u[second].selected=1;cx=m[0].x;cy=m[0].y;tap(KEY_A);
    assert(u[overlapped].selected && !u[second].selected && u[second].order==ORDER_ATTACK_MOVE);
    assert(field_ui_a_action(selected_count())==UI_A_SELECT);
    /* A selected non-worker does not get a mineral job. */
    int marine=spawn_unit(SIDE,U_MARINE,800,800);assert(marine>=0);
    clear_selection();cx=800;cy=800;tap(KEY_A);assert(u[marine].selected);
    cx=m[0].x;cy=m[0].y;update_exploration();tap(KEY_A);
    assert(u[marine].order!=ORDER_MINE);
    puts("empty worker STOP holds 120 updates; visible A resumes mining; hidden, cargo and nonworker cases pass");return 0;
}
