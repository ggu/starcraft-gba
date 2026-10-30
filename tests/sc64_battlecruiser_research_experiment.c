/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <string.h>

#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
static void tap_input(unsigned key)
{ game_update((u16)key,(u16)key); game_update(0,0); }
static void choose_by_buttons(int row)
{
    tap_input(KEY_B);
    assert(menu_open);
    for (int guard=0; menu_cursor!=row && guard<field_menu_count(); ++guard)
        tap_input(KEY_DOWN);
    assert(menu_cursor==row);
    tap_input(KEY_A);
    assert(!menu_open);
}
int main(void)
{
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    minerals=gas=5000;
    int lab=fixture_building_raw(SIDE,118,700,700);
    int lab2=fixture_building_raw(SIDE,118,800,700);
    assert(lab>=0 && lab2>=0);
    selected_building=lab;
    int menu_base=race_bc_menu_base();
    assert(field_menu_count()==menu_base+2);
    assert(!strcmp(race_menu_label(menu_base),"YAMATO GUN"));
    assert(!strcmp(race_menu_label(menu_base+1),"COLOSSUS REACTOR"));
    choose_by_buttons(menu_base);
    assert(b[lab].level==SC64_BC_TECH_YAMATO);
    assert(field_menu_count()==menu_base+1);
    assert(!strcmp(race_menu_label(menu_base),"CANCEL RESEARCH"));
    assert(minerals==4900 && gas==4900);
    choose_by_buttons(menu_base);
    assert(!b[lab].level && minerals==5000 && gas==5000);
    choose_by_buttons(menu_base);
    assert(b[lab].level==SC64_BC_TECH_YAMATO && minerals==4900 && gas==4900);
    assert(!race_bc_lab_start(lab2,SC64_BC_TECH_YAMATO));
    int expected=b[lab].queue_progress;
    for (int i=0;i<100;i++) {
        ++frame;
        if ((unsigned)frame % 5u != 0u) --expected;
        race_tick_buildings();
    }
    int remaining=b[lab].queue_progress;
    assert(remaining==expected);
    assert(race_save_game());
#ifdef SC64_SHARED_BATTERY
    /* A structurally encodable duplicate active job must be rejected before
       any live unit, building, bank or completed-tech state is committed. */
    Building bad=b[lab2]; bad.level=SC64_BC_TECH_YAMATO; bad.queue_progress=5;
    assert(race_battery_save_encode_building(&race_data,(unsigned)lab2,
                                             race_cols*16u,race_rows*16u,&bad));
    save_write(&race_data,sizeof(race_data),SAVE_VERSION);
    save_write(&race_data,sizeof(race_data),SAVE_VERSION);
    int original_remaining=b[lab].queue_progress, original_ore=minerals;
    unsigned original_bits=sc64_battlecruiser_tech_bits;
    assert(!race_load_game());
    assert(b[lab].queue_progress==original_remaining && !b[lab2].level &&
           minerals==original_ore && sc64_battlecruiser_tech_bits==original_bits);
    assert(race_save_game());
#endif
    b[lab2].level=SC64_BC_TECH_YAMATO; b[lab2].queue_progress=2;
    assert(!race_save_game()); /* duplicate research cannot be serialized */
    b[lab2].level=0; b[lab2].queue_progress=0;
    b[lab].level=0; b[lab].queue_progress=0;
    assert(race_load_game());
    assert(b[lab].level==SC64_BC_TECH_YAMATO && b[lab].queue_progress==remaining);
    for (int i=0;i<2200 && b[lab].level;i++) { ++frame; race_tick_buildings(); }
    assert(!b[lab].level && !b[lab].queue_progress);
    assert(sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,SIDE,SC64_BC_TECH_YAMATO));
    assert(!sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,ENEMY,SC64_BC_TECH_YAMATO));
    assert(!race_bc_lab_start(lab2,SC64_BC_TECH_YAMATO));
    assert(race_save_game()); sc64_battlecruiser_tech_bits=0;
    assert(race_load_game());
    assert(sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,SIDE,SC64_BC_TECH_YAMATO));
    /* Parent loss detaches its Lab and returns full unfinished research. */
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    minerals=gas=1000;
    int facility=fixture_building_raw(SIDE,116,400,400);
    lab=fixture_building_raw(SIDE,118,448,408);
    assert(facility>=0 && lab>=0 && race_bc_lab_start(lab,SC64_BC_TECH_YAMATO));
    destroy_building(facility,ENEMY);
    assert(!b[facility].alive && b[lab].alive && b[lab].owner==NEUTRAL);
    assert(!b[lab].level && !b[lab].queue_progress && minerals==1000 && gas==1000);
    /* A direct Lab death refunds three quarters and does not unlock. */
    lab=fixture_building_raw(SIDE,118,600,400);
    assert(lab>=0 && race_bc_lab_start(lab,SC64_BC_TECH_REACTOR));
    destroy_building(lab,ENEMY);
    assert(!b[lab].alive && minerals==962 && gas==962);
    assert(!sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,SIDE,SC64_BC_TECH_REACTOR));
#ifdef SC64_TERRAN_LATE_RUNTIME
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    minerals=gas=1000;
    race_mode=0; race_selected_mission=11;
    int fx=-1, fy=-1;
    for (int y=100;y<WORLD_H-100 && fx<0;y+=16)
        for (int x=100;x<WORLD_W-160;x+=16)
            if (can_place(building_raw(116),x,y) && can_place(building_raw(118),x+48,y+8) &&
                !terrain_blocked((x-8)/16,(y-8)/16) && !terrain_blocked((x+8)/16,(y+8)/16) &&
                !terrain_blocked((x-8)/16,(y+8)/16) && !terrain_blocked((x+8)/16,(y-8)/16))
                { fx=x; fy=y; break; }
    assert(fx>=0);
    facility=fixture_building_raw(SIDE,116,fx,fy);
    lab=fixture_building_raw(SIDE,118,fx+48,fy+8);
    b[facility].raw_owner=0; b[lab].raw_owner=0;
    memset(race_late.flight_goals,0,sizeof(race_late.flight_goals));
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && !sc64_race_unit_flying(u[i].type) &&
            dist2(u[i].x,u[i].y,fx,fy)<20*20) { u[i].x=fx+100; u[i].y=fy+100; }
    assert(race_late_active() && race_bc_lab_start(lab,SC64_BC_TECH_YAMATO));
    assert(race_late_lift(facility));
    assert(b[lab].owner==NEUTRAL && !b[lab].level && minerals==1000 && gas==1000);
    int lx=b[facility].x, ly=b[facility].y;
    fprintf(stderr,"landing x=%d y=%d airborne=%d owner=%d world=%d,%d can=%d site=%d\n",lx,ly,race_late_airborne(facility),b[facility].owner,WORLD_W,WORLD_H,can_place(b[facility].type,lx,ly),race_late_landing_site(facility,lx,ly));
    assert(race_late_flight_order(facility,lx,ly,1));
    race_late_flight_tick();
    assert(!race_late_airborne(facility) && b[lab].owner==SIDE && b[lab].raw_owner==0);
#endif
    puts("experimental paid Lab research, source timing, save/load and death pass");
    return 0;
}
