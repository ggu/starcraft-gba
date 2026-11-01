/* SPDX-License-Identifier: GPL-3.0-or-later
 * Real-engine shared Shield Battery behavior in custom games. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/game.c"

static int find_unit(int owner,int raw)
{
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive && u[i].owner==owner && sc64_race_raw_unit(u[i].type)==raw) return i;
    return -1;
}
static int build_battery(int worker)
{
    int type=sc64_race_building_from_raw(172);
    assert(type>=0);
    for(int y=96;y<WORLD_H-64;y+=16) for(int x=96;x<WORLD_W-64;x+=16){
        int sx=x,sy=y;
        if(!race_build_request_valid(SIDE,worker,type,&sx,&sy,0)) continue;
        u[worker].x=(short)(sx+80);u[worker].y=(short)sy;
        int before=minerals,oldgas=gas;
        assert(race_start_build(SIDE,worker,type,sx,sy));
        assert(minerals==before && gas==oldgas);
        u[worker].x=(short)sx;u[worker].y=(short)sy;
        race_pending_build_tick(worker);
        for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive && b[i].type==type && b[i].under_construction){
            const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
            assert(rule && minerals==before-rule->minerals && gas==oldgas-rule->gas);
            for(int tick=0;tick<building_build_time(type)+8 && !building_complete(i);++tick){
                update_orders();race_tick_buildings();
            }
            assert(building_complete(i));return i;
        }
    }
    return -1;
}
int main(void)
{
    game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;init_state();
    assert(screen==SCREEN_PLAY && race_mode && race_player[SIDE]==R_PROTOSS);
    minerals=gas=5000;
    int worker=find_unit(SIDE,64);assert(worker>=0);
    int pylon=spawn_build(SIDE,sc64_race_building_from_raw(156),240,240,0);
    int gateway=spawn_build(SIDE,sc64_race_building_from_raw(160),280,240,0);
    assert(pylon>=0 && gateway>=0);
    int battery=build_battery(worker);assert(battery>=0);
    /* Use a powered site without changing its ownership or source identity. */
    b[pylon].x=b[battery].x;b[pylon].y=b[battery].y;
    assert(race_battery_fields_valid(battery));
    assert(b[battery].queue_progress==100*256);
    assert(race_battery_powered_live(battery));
    assert(race_battery_site_powered(SIDE,b[battery].x,b[battery].y));
    b[battery].queue_progress=0;b[battery].level=0;
    race_tick_state();
    assert(b[battery].queue_progress>0);
    int marine=spawn_unit(SIDE,U_ZEALOT,b[battery].x,b[battery].y);
    assert(marine>=0);u[marine].shield=0;u[marine].shield_remainder=0;
    b[battery].queue_progress=100*256;b[battery].level=0;
    selected_building=battery;
    for(int i=0;i<MAX_UNITS;++i) u[i].selected=0;
    assert(race_battery_command_available());
    int row=(int)sc64_skirmish_menu_count((unsigned)race_human);
    assert(field_menu_count()>row);
    menu_open=1;
    choose_command(row);
    assert(mode==MODE_RECHARGE && !menu_open);
    cx=u[marine].x;cy=u[marine].y;
    int energy=b[battery].queue_progress;
    game_update(KEY_A,KEY_A); /* Target through the ordinary field input. */
    assert(screen==SCREEN_PLAY && mode==MODE_NORMAL);
    assert(u[marine].order==ORDER_RECHARGE && u[marine].target==battery);
    assert(u[marine].shield>0 || u[marine].shield_remainder>0);
    assert(b[battery].queue_progress<energy);
    int shield_before=u[marine].shield;
    u[marine].shield_remainder=1000;
    race_tick_state();
    assert(u[marine].shield==shield_before);
    assert(u[marine].shield_remainder==1028);
    int enemy=spawn_unit(ENEMY,U_ZEALOT,b[battery].x,b[battery].y);
    assert(enemy>=0);u[enemy].shield=0;
    assert(!race_battery_unit_valid(enemy,battery));
    int own_raw=u[marine].raw_type;
    u[marine].raw_type=64; /* Zealot cannot masquerade as a Probe. */
    assert(!race_battery_unit_valid(marine,battery));
    u[marine].raw_type=own_raw;
    int enemy_battery=spawn_build(ENEMY,sc64_race_building_from_raw(172),600,600,0);
    int enemy_pylon=spawn_build(ENEMY,sc64_race_building_from_raw(156),600,600,0);
    assert(enemy_battery>=0 && enemy_pylon>=0);
    b[enemy_battery].queue_progress=100*256;b[enemy_battery].level=0;
    u[enemy].x=u[enemy].y=600;
    assert(race_battery_target_valid(enemy_battery));
    assert(race_battery_unit_valid(enemy,enemy_battery));
    assert(!race_battery_unit_valid(marine,enemy_battery));
    unsigned char removed[MAX_BUILDINGS]={0};
    for(int i=0;i<MAX_BUILDINGS;++i)
        if(i!=pylon && b[i].alive && b[i].owner==SIDE &&
           sc64_race_raw_building(b[i].type)==156) { removed[i]=1;b[i].alive=0; }
    b[pylon].owner=ENEMY;
    assert(!race_battery_powered_live(battery));
    b[pylon].owner=SIDE;
    for(int i=0;i<MAX_BUILDINGS;++i) if(removed[i]) b[i].alive=1;
    u[marine].order=race_battery_recharge_order();u[marine].target=battery;
    b[battery].alive=0;
    int reused=spawn_build(SIDE,sc64_race_building_from_raw(156),700,700,0);
    assert(reused==battery);
    assert(u[marine].order==ORDER_IDLE && u[marine].target==-1);
    /* Non-P7 campaign: rescued SIDE actors keep their original source slot;
     * nonhuman owners with source slots must still match exactly. */
    race_mode=0;race_selected_mission=0;
    b[enemy_battery].owner=SIDE;b[enemy_battery].raw_owner=0;
    b[enemy_pylon].owner=SIDE;b[enemy_pylon].raw_owner=7;
    u[enemy].owner=SIDE;u[enemy].raw_owner=4;u[enemy].shield=0;
    assert(race_battery_target_valid(enemy_battery));
    assert(race_battery_unit_valid(enemy,enemy_battery));
    /* Campaign menu row must override Terran advanced-command labels. */
    int saved_human=race_human;
    race_human=R_TER; selected_building=enemy_battery;
    assert(field_menu_count()==MENU_STOP+2);
    assert(!strcmp(race_menu_label(MENU_STOP+1),"RECHARGE"));
    menu_open=1; choose_command(MENU_STOP+1);
    assert(mode==MODE_RECHARGE && !menu_open);
    mode=MODE_NORMAL; race_human=saved_human;
    b[enemy_battery].owner=ENEMY;b[enemy_battery].raw_owner=0;
    b[enemy_pylon].owner=ENEMY;b[enemy_pylon].raw_owner=4;
    u[enemy].owner=ENEMY;u[enemy].raw_owner=4;
    assert(!race_battery_target_valid(enemy_battery));
    assert(!race_battery_unit_valid(enemy,enemy_battery));
    b[enemy_pylon].raw_owner=0;u[enemy].raw_owner=0;
    assert(race_battery_target_valid(enemy_battery));
    assert(race_battery_unit_valid(enemy,enemy_battery));
#ifdef SC64_P7_RUNTIME
    race_selected_mission=SC64_P7_GLOBAL_ID;
    b[enemy_battery].owner=SIDE;b[enemy_battery].raw_owner=7;b[enemy_battery].raw_type=172;
    b[enemy_pylon].owner=SIDE;b[enemy_pylon].raw_owner=7;b[enemy_pylon].raw_type=156;
    u[enemy].owner=SIDE;u[enemy].raw_owner=7;u[enemy].raw_type=65;
    assert(race_battery_p7_context());
    assert(race_battery_recharge_order()==ORDER_P7_RECHARGE);
    assert(race_battery_target_valid(enemy_battery));
    assert(race_battery_unit_valid(enemy,enemy_battery));
    selected_building=enemy_battery;
    assert(field_menu_count()==MENU_STOP+2);
    assert(!strcmp(race_menu_label(MENU_STOP+1),"RECHARGE"));
    menu_open=1; choose_command(MENU_STOP+1);
    assert(mode==MODE_RECHARGE && !menu_open);
    mode=MODE_NORMAL;
    u[enemy].order=ORDER_P7_RECHARGE;u[enemy].target=enemy_battery;
    int legacy_energy=b[enemy_battery].queue_progress;
    assert(race_battery_tick_unit(enemy));
    assert(b[enemy_battery].queue_progress<legacy_energy);
    u[enemy].raw_owner=4;
    assert(!race_battery_unit_valid(enemy,enemy_battery));
#endif
    puts("shared Battery paid build, power, transfer and cleanup: ok");
    return 0;
}
