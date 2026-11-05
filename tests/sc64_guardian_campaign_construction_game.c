/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST world; this is not a guest or SRAM qualification. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

int main(void)
{
    game_init(); memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    race_mode=0; race_human=R_ZERG; race_player[SIDE]=R_ZERG;
    race_selected_mission=SC64_Z7_GLOBAL_ID;
    race_z7_map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    minerals=5000; gas=5000; selected_building=-1;
    assert(race_guardian_campaign_allowed(SIDE,3,138));
    assert(race_guardian_campaign_allowed(SIDE,3,133));
    assert(race_guardian_campaign_allowed(SIDE,3,137));
    assert(!race_guardian_campaign_allowed(SIDE,4,138));
    assert(!race_guardian_campaign_allowed(ENEMY,3,138));
    race_selected_mission=SC64_RACE_MISSION_Z4;
    assert(!race_guardian_campaign_allowed(SIDE,3,138));
    race_selected_mission=SC64_Z7_GLOBAL_ID;
    int lair=spawn_build(SIDE,B_LAIR,200,200,0);
    int drone=spawn_unit(SIDE,U_DRONE,260,200);
    assert(lair>=0 && drone>=0 && b[lair].raw_owner==3 && u[drone].raw_owner==3);
    assert(sc64_race_building_rule(B_ZP_QUEENS_NEST)->worker==U_DRONE);
    assert(!strcmp(race_menu_label(MENU_ZP_QUEENS_NEST),"QUEENS NEST"));
    choose_command(MENU_ZP_QUEENS_NEST);
    assert(mode==MODE_BUILD && menu==B_ZP_QUEENS_NEST);
    int x=0,y=0,found=0;
    for (int yy=120;yy<=280 && !found;yy+=8) for(int xx=120;xx<=280;xx+=8) {
        int tx=xx,ty=yy;
        if (race_build_request_valid(SIDE,drone,B_ZP_QUEENS_NEST,&tx,&ty,0)) {
            x=tx; y=ty; found=1; break;
        }
    }
    assert(found);
    int ore=minerals, vespene=gas;
    u[drone].raw_owner=4;
    assert(!race_start_build(SIDE,drone,B_ZP_QUEENS_NEST,x,y));
    assert(minerals==ore && gas==vespene && u[drone].order!=ORDER_BUILD_PENDING);
    u[drone].raw_owner=3;
    assert(race_start_build(SIDE,drone,B_ZP_QUEENS_NEST,x,y));
    assert(minerals==ore && gas==vespene); /* travel has not committed */
    u[drone].x=x;u[drone].y=y;race_pending_build_tick(drone);
    assert(minerals==ore-150 && gas==vespene-100 && !u[drone].alive);
    int nest=-1;
    for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive && b[i].type==B_ZP_QUEENS_NEST) nest=i;
    assert(nest>=0 && b[nest].raw_owner==3 && b[nest].under_construction);
    race_complete_building(nest);
    assert(spawn_unit(SIDE,U_OVERLORD,100,100)>=0);
    b[lair].larva=2; b[lair].larva_timer=123; b[lair].hp=900;
    assert(race_queue_unit(SIDE,lair,U_DRONE));
    int egg=-1;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].type==U_Z7_EGG) egg=i;
    assert(egg>=0 && b[lair].larva==1);
    b[lair].shield_remainder=17; b[lair].queue_count=1;
    ore=minerals;vespene=gas; selected_building=lair;
    assert(!race_guardian_campaign_morph(lair,B_HIVE));
    assert(minerals==ore && gas==vespene && b[lair].type==B_LAIR);
    b[lair].queue_count=0;
    b[lair].raw_owner=4;
    assert(!race_guardian_campaign_morph(lair,B_HIVE));
    assert(minerals==ore && gas==vespene && b[lair].type==B_LAIR);
    b[lair].raw_owner=3;
    choose_command(MENU_ZP_HIVE);
    assert(b[lair].type==B_HIVE && b[lair].under_construction);
    assert(minerals==ore-200 && gas==vespene-150);
    assert(b[lair].hp==900 && b[lair].larva==1 && b[lair].larva_timer==123 && b[lair].shield_remainder==17);
    Building retired=b[lair];
    assert(race_guardian_paid_morph_active(&retired));
    race_guardian_paid_morph_retire(&retired);
    assert(!(retired.flags & SC64_GUARDIAN_PAID_BUILDING_MORPH));
    Building unrelated=b[lair];unrelated.type=B_PYLON;unrelated.raw_type=205;unrelated.flags=0x2080;
    race_guardian_paid_morph_retire(&unrelated);
    assert(unrelated.flags==0x2080);
    assert(u[egg].alive && u[egg].type==U_Z7_EGG && u[egg].target==U_DRONE);
    /* Leaf codec shape only: no selected-journal or SRAM claim. */
    Sc64PackedBuildingSave packed; Building restored;
    assert(sc64_building_save_encode(&packed,&b[lair],2048,2048));
    assert(sc64_building_save_decode(&restored,&packed));
    assert(restored.type==B_HIVE && restored.under_construction && restored.flags==b[lair].flags &&
           restored.hp==900 && restored.raw_type==133 && restored.raw_owner==3 &&
           restored.larva==1 && restored.larva_timer==123 && restored.shield_remainder==17);
    assert(!strcmp(race_menu_label(MENU_ZP_HIVE),"CANCEL MORPH"));
    b[lair].hp=0; int old_ore=minerals, old_gas=gas;
    assert(!race_guardian_campaign_cancel(lair));
    assert(minerals==old_ore && gas==old_gas && b[lair].type==B_HIVE);
    b[lair].hp=800; choose_command(MENU_ZP_HIVE);
    assert(b[lair].type==B_LAIR && !b[lair].under_construction && b[lair].hp==800);
    assert(minerals==ore-50 && gas==vespene-38);
    assert(b[lair].larva==1 && b[lair].larva_timer==123 && b[lair].shield_remainder==17);
    assert(!race_guardian_campaign_cancel(lair));
    assert(race_guardian_campaign_morph(lair,B_HIVE));
    int cap=race_owner_supply_cap(SIDE);
    race_complete_building(lair);
    assert(b[lair].type==B_HIVE && !b[lair].under_construction && b[lair].hp==1500);
    assert(b[lair].larva==1 && b[lair].larva_timer==123 && b[lair].shield_remainder==17);
    assert(race_owner_supply_cap(SIDE)==cap);
    int spire=spawn_build(SIDE,B_SPIRE,350,200,0);assert(spire>=0);
    selected_building=spire; ore=minerals;vespene=gas;
    choose_command(MENU_ZP_GREATER_SPIRE);
    assert(b[spire].type==B_GREATER_SPIRE && b[spire].under_construction);
    assert(minerals==ore-100 && gas==vespene-150);
    choose_command(MENU_ZP_HIVE);
    assert(b[spire].type==B_SPIRE && !b[spire].under_construction);
    assert(minerals==ore-25 && gas==vespene-38);
    int pylon=spawn_build(SIDE,B_PYLON,500,200,1);assert(pylon>=0);
    b[pylon].raw_type=205;b[pylon].flags=0x2080;
    race_complete_building(pylon);
    assert(b[pylon].flags==0x2080); /* unrelated P6-style source bits survive */
    puts("Guardian campaign construction HOST stage passed");
    return 0;
}
