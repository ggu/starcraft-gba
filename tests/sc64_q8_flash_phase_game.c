/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual game processes and unmodified baseline writer; authored scenes. */
#define main sc64_unused_flash_fixture_main
#include "sc64_q8_flash_game.c"
#undef main
#include <stdlib.h>
#define PHASE_UNITS 160
#define PHASE_BUILDINGS 100

static void phase_scene(int buildings,int base)
{
    int count=buildings?PHASE_BUILDINGS:PHASE_UNITS;
    int limit=buildings?320:1280;
    if(count>limit-base)count=limit-base;
    for(int n=0;n<count;++n) {
        if(buildings) {
            int i=spawn_build(ENEMY,B_FORGE,100+(n%10)*64,100+(n/10)*64,0);
            assert(i>=0);b[i].shield=sc64_race_building_shields(b[i].type)-1;
            b[i].shield_remainder=base+n;
        } else {
            int i=spawn_unit(ENEMY,U_DRAGOON,300+n,600);assert(i>=0);
            u[i].shield=sc64_race_unit_rule(u[i].type)->shields-1;
            u[i].shield_remainder=base+n;
        }
    }
    int marine=spawn_unit(ENEMY,U_MARINE,700,800);
    int burn=spawn_build(ENEMY,B_BARRACKS,800,800,0);
    assert(marine>=0&&burn>=0);u[marine].shield_remainder=9;
    b[burn].hp=b[burn].maxhp-1;b[burn].shield_remainder=ENEMY*256+37;
}
#ifdef SC64_Q8_FLASH_HOST_STAGE
static void phase_verify(int buildings,int base,int advanced)
{
    int count=0,marine=0,burn=0;
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive) {
        if(u[i].type==U_MARINE) {assert(u[i].shield_remainder==9);++marine;}
        if(!buildings && u[i].type==U_DRAGOON && u[i].owner==ENEMY && u[i].y==600) {
            int n=u[i].x-300;assert(n>=0&&n<PHASE_UNITS);
            unsigned value=sc64_race_unit_rule(u[i].type)->shields-1,rem=(unsigned)(base+n);
            for(int tick=0;tick<advanced;++tick)sc64_p7_battery_passive_shield_tick(&value,80,&rem);
            assert((unsigned)u[i].shield==value && (unsigned)u[i].shield_remainder==rem);++count;
        }
    }
    for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive) {
        if(b[i].type==B_BARRACKS) {
            assert(b[i].shield_remainder==ENEMY*256+37);
            unsigned hp;assert(sc64_pr_q8_actor_hp(MAX_UNITS+i,&hp) && hp==(unsigned)(b[i].maxhp-1)*256u-37u);
            assert(!sc64_pr_hp_debt[MAX_UNITS+i]);++burn;
        }
        if(buildings && b[i].type==B_FORGE && b[i].owner==ENEMY) {
            int n=((b[i].y-100)/64)*10+(b[i].x-100)/64;
            assert(n>=0&&n<PHASE_BUILDINGS);
            unsigned short value=(unsigned short)(sc64_race_building_shields(b[i].type)-1),rem=(unsigned short)(base+n);
            for(int tick=0;tick<advanced;++tick)sc64_race_shield_tick_30fps(&value,sc64_race_building_shields(b[i].type),&rem);
            assert(b[i].shield==value && b[i].shield_remainder==(int)rem*4);++count;
        }
    }
    int expected=buildings?PHASE_BUILDINGS:PHASE_UNITS,limit=buildings?320:1280;
    if(expected>limit-base)expected=limit-base;
    assert(count==expected && marine==1 && burn==1);
}
#endif
int main(int argc,char **argv)
{
    assert(argc==5);int buildings=atoi(argv[3]),base=atoi(argv[4]);
    assert((buildings==0||buildings==1)&&base>=0&&base<(buildings?320:1280));start();
#ifdef SC64_Q8_FLASH_BASELINE
    assert(!strcmp(argv[1],"write"));phase_scene(buildings,base);
    assert(race_save_game());file_write(argv[2],host_save,sizeof host_save);
#else
    memset(device.bytes,0xff,sizeof device.bytes);
    if(!strcmp(argv[1],"reject-build-phase")) {
        /* Deliberately forge a checksum-valid ambiguous old building phase;
         * this negative is not represented as a genuine producer output. */
        static u8 prior[131072];static Unit old_units[MAX_UNITS];
        static Building old_buildings[MAX_BUILDINGS];u8 old_debt[sizeof sc64_pr_hp_debt];
        file_read(argv[2],device.bytes,32768);GbaFlashJournalRecord rec;
        assert(gba_fj_probe(&flash_io,0,&rec)==GBA_FJ_OK);
        assert(rec.length<=sizeof race_impact_record);
        assert(gba_fj_read_payload(&flash_io,&rec,0,&race_impact_record,rec.length)==GBA_FJ_OK);
        int changed=0;
        for(unsigned i=0;i<MAX_BUILDINGS&&!changed;++i) {
            Building q;assert(race_battery_save_building(&race_data,i,&q));
            if(q.alive&&q.type==B_FORGE) {
                q.shield_remainder=320;
                assert(race_battery_save_encode_building(&race_data,i,2048,2048,&q));changed=1;
            }
        }
        assert(changed);u32 meta=gba_fj_metadata(0);
        memcpy(device.bytes+meta+16,&race_impact_record,rec.length);
        gba_fj_put32(device.bytes+meta+8,gba_fj_hash(2166136261u,device.bytes+meta+16,rec.length));
        memcpy(prior,device.bytes,sizeof prior);memcpy(old_units,u,sizeof u);
        memcpy(old_buildings,b,sizeof b);memcpy(old_debt,sc64_pr_hp_debt,sizeof old_debt);
        int old_minerals=minerals,old_gas=gas;
        assert(!race_load_game()&&!race_save_game());
        assert(!memcmp(prior,device.bytes,sizeof prior)&&!memcmp(old_units,u,sizeof u)&&
               !memcmp(old_buildings,b,sizeof b)&&!memcmp(old_debt,sc64_pr_hp_debt,sizeof old_debt)&&
               minerals==old_minerals&&gas==old_gas);
    } else if(!strcmp(argv[1],"migrate")) {
        static u8 old[32768];file_read(argv[2],old,sizeof old);memcpy(device.bytes,old,sizeof old);
        assert(race_load_game());phase_verify(buildings,base,0);
        assert(race_save_game() && !memcmp(old,device.bytes,sizeof old));
        file_write(argv[2],device.bytes,sizeof device.bytes);
    } else {
        assert(!strcmp(argv[1],"read"));file_read(argv[2],device.bytes,sizeof device.bytes);
        assert(race_load_game());phase_verify(buildings,base,0);
        for(int tick=1;tick<=47;++tick) {race_tick_state();phase_verify(buildings,base,tick);}
    }
#endif
    puts("actual legacy shield phase process: PASS");return 0;
}
