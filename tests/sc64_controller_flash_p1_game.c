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
        int visitor=-1,nexus=-1;
        for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE){visitor=i;break;}
        for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive && b[i].owner==RESCUABLE && b[i].raw_owner==1 && b[i].type==B_NEXUS && b[i].x>=224 && b[i].x<=784 && b[i].y>=848 && b[i].y<=1200){nexus=i;break;}
        assert(visitor>=0 && nexus>=0);u[visitor].x=400;u[visitor].y=900;race_campaign_tick();
        assert(b[nexus].owner==SIDE && b[nexus].raw_owner==1 && sc64_controller_building(nexus)==7);
        int p=spawn_build(SIDE,B_PYLON,240,240,0),f=spawn_build(SIDE,B_FORGE,240,240,0);
        int u0=spawn_unit(SIDE,U_DRAGOON,300,300);assert(p>=0 && f>=0 && u0>=0);
        b[p].raw_owner=b[f].raw_owner=7;u[u0].raw_owner=7;
        sc64_current_building[p]=sc64_current_building[f]=sc64_current_unit[u0]=7;
        minerals=gas=1000;assert(sc64_pr_stage_begin_slot(f,SC64_PR_GROUND_WEAPONS));
        u[u0].shield=0;u[u0].shield_remainder=0;
        hit_target_impl_ex(u0,128,U_MARINE,ENEMY,SC64_STOCK_DMG_IGNORE_ARMOR,SC64_HIT_PACKET_Q8);
        memset(device.bytes,0xff,sizeof device.bytes);
        static u8 blank[131072];memcpy(blank,device.bytes,sizeof blank);
        sc64_current_building[f]=254;assert(!race_save_game() && !memcmp(blank,device.bytes,sizeof blank));
        sc64_current_building[f]=7;
        assert(race_save_game());minerals=889;
        assert(race_save_game());file_write(argv[2],device.bytes,sizeof device.bytes);
    } else if(!strcmp(argv[1],"read")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);assert(race_load_game());
        int f=-1,a=-1;unsigned hp;
        for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].type==B_FORGE&&b[i].level)f=i;
        for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].type==U_DRAGOON&&u[i].x==300)a=i;
        assert(f>=0 && a>=0 && minerals==889 && sc64_controller_building(f)==7 && sc64_controller_unit(a)==7);
        int nexus=-1;for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].type==B_NEXUS&&b[i].raw_owner==1&&b[i].owner==SIDE){nexus=i;break;}
        assert(nexus>=0 && sc64_controller_building(nexus)==7);
        assert(sc64_pr_q8_actor_hp(a,&hp)&&hp==(unsigned)u[a].maxhp*256u-128u);
        Sc64PrPaidReceipt paid;assert(sc64_pr_stage_receipt(f,&paid)&&paid.payer==SIDE);
        unsigned left=b[f].queue_progress;assert(left);for(unsigned i=0;i<left;++i)sc64_pr_stage_tick();
        assert(!b[f].level && sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]==1);
    } else if(!strcmp(argv[1],"legacy") || !strcmp(argv[1],"legacy-empty")) {
        assert(argc==4);memset(device.bytes,0xff,sizeof device.bytes);
        file_read(argv[2],device.bytes,32768);static u8 legacy[32768];memcpy(legacy,device.bytes,sizeof legacy);
        assert(race_load_game() && race_selected_mission==SC64_RACE_MISSION_P1);
        if(!strcmp(argv[1],"legacy")) {
            int f=-1;for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].type==B_FORGE&&b[i].level)f=i;
            assert(f>=0 && sc64_controller_building(f)==7);
            Sc64PrPaidReceipt paid;assert(sc64_pr_stage_receipt(f,&paid)&&paid.payer==SIDE);
        } else assert(minerals==1000 && gas==1000);
        assert(race_save_game() && !memcmp(legacy,device.bytes,sizeof legacy));
        file_write(argv[3],device.bytes,sizeof device.bytes);
    } else if(!strcmp(argv[1],"legacy-empty-read")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);assert(race_load_game());
        assert(race_selected_mission==SC64_RACE_MISSION_P1 && minerals==1000 && gas==1000);
    } else if(!strcmp(argv[1],"legacy-read")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);assert(race_load_game());
        int f=-1;for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].type==B_FORGE&&b[i].level)f=i;
        assert(f>=0 && sc64_controller_building(f)==7);
        Sc64PrPaidReceipt paid;assert(sc64_pr_stage_receipt(f,&paid)&&paid.payer==SIDE);
    } else if(!strcmp(argv[1],"legacy-foreign-hash") || !strcmp(argv[1],"legacy-foreign-length") || !strcmp(argv[1],"legacy-known-malformed")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);
        u32 tag=SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION;
        if(strcmp(argv[1],"legacy-known-malformed")) tag=(tag&~SC64_SAVE_TAG_NAMESPACE_MASK)|0x004a0000u;
        gba_fj_put32(device.bytes,tag);
        gba_fj_put32(device.bytes+4,!strcmp(argv[1],"legacy-foreign-length")?0u:1u);
        gba_fj_put32(device.bytes+8,0u);gba_fj_put32(device.bytes+12,1u);device.bytes[16]=42;
        static u8 legacy_before[32768],full_before[131072];memcpy(legacy_before,device.bytes,sizeof legacy_before);
        assert(race_load_game());memcpy(full_before,device.bytes,sizeof full_before);
        if(!strcmp(argv[1],"legacy-known-malformed")) {
            assert(race_save_game() && !memcmp(legacy_before,device.bytes,sizeof legacy_before));
        } else assert(!race_save_game() && !memcmp(full_before,device.bytes,sizeof full_before));
    } else if(!strcmp(argv[1],"unknown-new") || !strcmp(argv[1],"fallback-legacy")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);
        GbaFlashJournalRecord r2,r3,*newer=0;
        int s2=gba_fj_probe(&flash_io,2,&r2),s3=gba_fj_probe(&flash_io,3,&r3);
        if(s2==GBA_FJ_OK)newer=&r2;
        if(s3==GBA_FJ_OK && (!newer || gba_fj_newer(r3.sequence,newer->sequence)))newer=&r3;
        assert(newer);u32 meta=gba_fj_metadata(newer->index);
        if(!strcmp(argv[1],"unknown-new")) {
            u32 unknown=(newer->tag&~SC64_SAVE_TAG_NAMESPACE_MASK)|0x004a0000u;
            gba_fj_put32(device.bytes+gba_fj_base(newer->index),unknown);gba_fj_put32(device.bytes+meta,unknown);
        } else {
            u32 tail=newer->length-SC64_CONTROLLER_FLASH_BYTES;
            device.bytes[meta+16+tail+8]=254;
            gba_fj_put32(device.bytes+meta+8,gba_fj_hash(2166136261u,device.bytes+meta+16,newer->length));
        }
        static u8 frozen_unknown[131072];memcpy(frozen_unknown,device.bytes,sizeof frozen_unknown);
        assert(race_load_game() && race_selected_mission==SC64_RACE_MISSION_P1);
        if(!strcmp(argv[1],"unknown-new")) assert(minerals==900 && !race_save_game());
        else {int f=-1;for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].type==B_FORGE&&b[i].level)f=i;
              assert(f>=0 && sc64_controller_building(f)==7);}
        assert(!memcmp(frozen_unknown,device.bytes,sizeof frozen_unknown));
    } else if(!strcmp(argv[1],"fallback") || !strcmp(argv[1],"only-bad")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);
        GbaFlashJournalRecord r2,r3,*newer,*older;
        assert(gba_fj_probe(&flash_io,2,&r2)==GBA_FJ_OK && gba_fj_probe(&flash_io,3,&r3)==GBA_FJ_OK);
        newer=gba_fj_newer(r2.sequence,r3.sequence)?&r2:&r3;older=newer==&r2?&r3:&r2;
        /* Deliberately malformed, checksum-valid test record; never call it
         * a genuine producer or alter any predecessor bytes. */
        u32 meta=gba_fj_metadata(newer->index),tail=newer->length-SC64_CONTROLLER_FLASH_BYTES;
        unsigned changed=0;
        for(unsigned i=0;i<MAX_UNITS+MAX_BUILDINGS;++i) if(device.bytes[meta+16+tail+8+i]==7) {
            device.bytes[meta+16+tail+8+i]=254;changed=1;break;
        }
        assert(changed);
        gba_fj_put32(device.bytes+meta+8,gba_fj_hash(2166136261u,device.bytes+meta+16,newer->length));
        if(!strcmp(argv[1],"only-bad")) memset(device.bytes+gba_fj_base(older->index),0xff,4);
        static u8 frozen[131072];memcpy(frozen,device.bytes,sizeof frozen);
        static Unit before_units[MAX_UNITS];static Building before_buildings[MAX_BUILDINGS];
        static u8 before_controllers[MAX_UNITS],before_building_controllers[MAX_BUILDINGS];
        static u8 before_debts[MAX_UNITS+MAX_BUILDINGS];Sc64ProtossResearchLevels before_levels=sc64_experimental_research_levels;
        memcpy(before_units,u,sizeof u);memcpy(before_buildings,b,sizeof b);
        memcpy(before_controllers,sc64_current_unit,sizeof sc64_current_unit);
        memcpy(before_building_controllers,sc64_current_building,sizeof sc64_current_building);
        memcpy(before_debts,sc64_pr_hp_debt,sizeof sc64_pr_hp_debt);
        int prior=minerals, priorframe=frame;
        if(!strcmp(argv[1],"fallback")) {
            assert(race_load_game() && minerals==900);
        } else {
            assert(!race_load_game() && minerals==prior && frame==priorframe);
            assert(!memcmp(before_units,u,sizeof u) && !memcmp(before_buildings,b,sizeof b));
            assert(!memcmp(before_controllers,sc64_current_unit,sizeof sc64_current_unit));
            assert(!memcmp(before_building_controllers,sc64_current_building,sizeof sc64_current_building));
            assert(!memcmp(before_debts,sc64_pr_hp_debt,sizeof sc64_pr_hp_debt));
            assert(!memcmp(&before_levels,&sc64_experimental_research_levels,sizeof before_levels));
        }
        assert(!memcmp(frozen,device.bytes,sizeof frozen));
    } else assert(0);
    puts("P1 current controller Flash paid job: PASS");return 0;
}
