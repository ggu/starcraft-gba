/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <string.h>
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
static void tap(unsigned k) { game_update((u16)k,(u16)k); game_update(0,0); }
static void choose_row(int row) {
    tap(KEY_B); assert(menu_open);
    for (int guard=0;menu_cursor!=row && guard<field_menu_count();++guard) tap(KEY_DOWN);
    assert(menu_cursor==row); tap(KEY_A); assert(!menu_open);
}
int main(void)
{
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    minerals=gas=5000;
    int x=-1,y=-1;
    for (int yy=100;yy<WORLD_H-100 && x<0;yy+=16)
        for (int xx=100;xx<WORLD_W-160;xx+=16)
            if (can_place(building_raw(116),xx,yy) &&
                can_place(building_raw(118),xx+48,yy+8) &&
                !terrain_blocked((xx-8)/16,(yy-8)/16) &&
                !terrain_blocked((xx+8)/16,(yy-8)/16) &&
                !terrain_blocked((xx-8)/16,(yy+8)/16) &&
                !terrain_blocked((xx+8)/16,(yy+8)/16)) {x=xx;y=yy;break;}
    assert(x>=0);
    int sf=fixture_building_raw(SIDE,116,x,y);
    int lab=fixture_building_raw(SIDE,118,x+48,y+8);
    assert(sf>=0 && lab>=0 && race_bc_lab_start(lab,SC64_BC_TECH_YAMATO));
    selected_building=sf;
    int row=race_bc_menu_base();
    assert(field_menu_count()==row+1 && !strcmp(race_menu_label(row),"LIFT OFF"));
    choose_row(row);
    assert(race_bc_facility_airborne(sf) && b[lab].owner==NEUTRAL && !b[lab].level);
    assert(!race_can_attack(U_ZERGLING,MAX_UNITS+sf));
    assert(race_can_attack(U_MARINE,MAX_UNITS+sf));
    assert(!strcmp(race_menu_label(row),"LAND"));
    assert(!race_skirmish_has_raw(SIDE,116));
    assert(race_save_game());
    b[sf].flags=0;
    assert(race_load_game() && race_bc_facility_airborne(sf));
#ifdef SC64_SHARED_BATTERY
    assert(race_save_game());
    Building invalid=b[sf]; invalid.level=2;
    assert(race_battery_save_encode_building(&race_data,(unsigned)sf,
                                             race_cols*16u,race_rows*16u,&invalid));
    save_write(&race_data,sizeof(race_data),SAVE_VERSION);
    save_write(&race_data,sizeof(race_data),SAVE_VERSION);
    int saved_x=b[sf].x, saved_energy=minerals;
    assert(!race_load_game());
    assert(race_bc_facility_airborne(sf) && b[sf].x==saved_x && minerals==saved_energy);
    assert(race_save_game());
#endif
    selected_building=sf;
    choose_row(row); assert(mode==MODE_LAND);
    cx=x;cy=y; tap(KEY_A);
    assert(!race_bc_facility_airborne(sf) || b[sf].level==1);
    for (int i=0;i<8 && race_bc_facility_airborne(sf);++i) game_update(0,0);
    assert(!race_bc_facility_airborne(sf) && b[lab].owner==SIDE);
    /* Save and resume an actual nonzero flight toward another valid site. */
    assert(race_bc_facility_lift(sf));
    int tx=-1,ty=-1;
    for (int yy=80;yy<WORLD_H-80 && tx<0;yy+=16)
        for (int xx=80;xx<WORLD_W-80;xx+=16)
            if (dist2(x,y,xx,yy)>80*80 && race_bc_facility_landing_site(sf,xx,yy))
                {tx=xx;ty=yy;break;}
    assert(tx>=0 && race_bc_facility_land(sf,tx,ty));
    for (int i=0;i<10;++i) { ++frame; race_bc_facility_flight_tick(); }
    int at_x=b[sf].x, at_y=b[sf].y;
    assert(at_x!=x || at_y!=y);
    assert(race_save_game()); b[sf].x=x; b[sf].y=y;
    assert(race_load_game() && b[sf].x==at_x && b[sf].y==at_y && b[sf].level==1);
    for (int i=0;i<10000 && race_bc_facility_airborne(sf);++i) {
        ++frame; race_bc_facility_flight_tick();
    }
    assert(!race_bc_facility_airborne(sf) && b[sf].x==tx && b[sf].y==ty);
    /* A player-owned Facility in an existing campaign context uses the same
     * shared actor service; the fixture does not start or win a mission. */
    race_mode=0;
#ifdef SC64_CAMPAIGN_RACE
    race_human=SC64_CAMPAIGN_RACE;
#else
    race_human=R_PROTOSS;
#endif
    race_selected_mission=sc64_frontend_first_mission(race_human);
    Sc64RaceCampaignState campaign_before=race_campaign;
    selected_building=sf;
    assert(race_bc_facility_menu_available());
    assert(race_bc_facility_lift(sf) && race_bc_facility_airborne(sf));
    assert(!memcmp(&campaign_before,&race_campaign,sizeof(campaign_before)));
    /* A parent cannot lift during construction of its add-on. On parent
     * destruction the paid unfinished Lab is cancelled for a 75% refund. */
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    minerals=gas=950;
    sf=fixture_building_raw(SIDE,116,400,400);
    lab=spawn_build(SIDE,building_raw(118),448,408,1);
    assert(sf>=0 && lab>=0 && b[lab].under_construction);
    assert(!race_bc_facility_lift(sf) && b[lab].alive && minerals==950 && gas==950);
    destroy_building(sf,ENEMY);
    assert(!b[sf].alive && !b[lab].alive && minerals==987 && gas==987);
    puts("experimental shared facility lift, save, landing and Lab reconnect pass");
    return 0;
}
