/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored HOST P1 paid research + controller Flash experiment. */
#define SC64_Q8_FLASH_NO_FIXTURE_MAIN
#include "sc64_q8_flash_game.c"
static void setup_p1(void)
{
    gba_init();game_init();race_mode=0;race_selected_mission=SC64_RACE_MISSION_P1;init_state();
    assert(screen==SCREEN_PLAY && race_human==R_PROTOSS && sc64_controller_human_source()==7);
}
int main(int argc,char **argv)
{
    assert(argc==3 || argc==4);setup_p1();
    if(!strcmp(argv[1],"write")) {
        int p=spawn_build(SIDE,B_PYLON,240,240,0),f=spawn_build(SIDE,B_FORGE,240,240,0);
        int u0=spawn_unit(SIDE,U_DRAGOON,300,300);assert(p>=0 && f>=0 && u0>=0);
        b[p].raw_owner=b[f].raw_owner=7;u[u0].raw_owner=7;
        sc64_current_building[p]=sc64_current_building[f]=sc64_current_unit[u0]=7;
        minerals=gas=1000;assert(sc64_pr_stage_begin_slot(f,SC64_PR_GROUND_WEAPONS));
        u[u0].shield=0;u[u0].shield_remainder=0;
        hit_target_impl_ex(u0,128,U_MARINE,ENEMY,SC64_STOCK_DMG_IGNORE_ARMOR,SC64_HIT_PACKET_Q8);
        memset(device.bytes,0xff,sizeof device.bytes);
        assert(race_save_game());minerals=889;
        assert(race_save_game());file_write(argv[2],device.bytes,sizeof device.bytes);
    } else if(!strcmp(argv[1],"read")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);assert(race_load_game());
        int f=-1,a=-1;unsigned hp;
        for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].type==B_FORGE&&b[i].level)f=i;
        for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].type==U_DRAGOON&&u[i].x==300)a=i;
        assert(f>=0 && a>=0 && minerals==889 && sc64_controller_building(f)==7 && sc64_controller_unit(a)==7);
        assert(sc64_pr_q8_actor_hp(a,&hp)&&hp==(unsigned)u[a].maxhp*256u-128u);
        Sc64PrPaidReceipt paid;assert(sc64_pr_stage_receipt(f,&paid)&&paid.payer==SIDE);
        unsigned left=b[f].queue_progress;assert(left);for(unsigned i=0;i<left;++i)sc64_pr_stage_tick();
        assert(!b[f].level && sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]==1);
    } else if(!strcmp(argv[1],"legacy")) {
        assert(argc==4);memset(device.bytes,0xff,sizeof device.bytes);
        file_read(argv[2],device.bytes,32768);static u8 legacy[32768];memcpy(legacy,device.bytes,sizeof legacy);
        assert(race_load_game() && race_selected_mission==SC64_RACE_MISSION_P1);
        int f=-1;for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].type==B_FORGE&&b[i].level)f=i;
        assert(f>=0 && sc64_controller_building(f)==7);
        Sc64PrPaidReceipt paid;assert(sc64_pr_stage_receipt(f,&paid)&&paid.payer==SIDE);
        assert(race_save_game() && !memcmp(legacy,device.bytes,sizeof legacy));
        file_write(argv[3],device.bytes,sizeof device.bytes);
    } else if(!strcmp(argv[1],"legacy-read")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);assert(race_load_game());
        int f=-1;for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].type==B_FORGE&&b[i].level)f=i;
        assert(f>=0 && sc64_controller_building(f)==7);
        Sc64PrPaidReceipt paid;assert(sc64_pr_stage_receipt(f,&paid)&&paid.payer==SIDE);
    } else if(!strcmp(argv[1],"fallback") || !strcmp(argv[1],"only-bad")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);
        GbaFlashJournalRecord r2,r3,*newer,*older;
        assert(gba_fj_probe(&flash_io,2,&r2)==GBA_FJ_OK && gba_fj_probe(&flash_io,3,&r3)==GBA_FJ_OK);
        newer=gba_fj_newer(r2.sequence,r3.sequence)?&r2:&r3;older=newer==&r2?&r3:&r2;
        /* Deliberately malformed, checksum-valid test record; never call it
         * a genuine producer or alter any predecessor bytes. */
        u32 meta=gba_fj_metadata(newer->index),tail=newer->length-SC64_CONTROLLER_FLASH_BYTES;
        device.bytes[meta+16+tail+8]=254;
        gba_fj_put32(device.bytes+meta+8,gba_fj_hash(2166136261u,device.bytes+meta+16,newer->length));
        if(!strcmp(argv[1],"only-bad")) memset(device.bytes+gba_fj_base(older->index),0xff,4);
        static u8 frozen[131072];memcpy(frozen,device.bytes,sizeof frozen);
        int prior=minerals, priorframe=frame;
        if(!strcmp(argv[1],"fallback")) {
            assert(race_load_game() && minerals==900);
        } else {
            assert(!race_load_game() && minerals==prior && frame==priorframe);
        }
        assert(!memcmp(frozen,device.bytes,sizeof frozen));
    } else assert(0);
    puts("P1 current controller Flash paid job: PASS");return 0;
}
