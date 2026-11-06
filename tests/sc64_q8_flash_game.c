/* SPDX-License-Identifier: GPL-3.0-or-later
 * Explicit HOST fake-device experiment with actual game save/load. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef SC64_Q8_FLASH_HOST_STAGE
#define main sc64_unused_flash_model_main
#include "gba_flash1m_model.c"
#undef main
#endif
#ifdef SC64_Q8_FLASH_BASELINE
#include <gba.c>
#include <game.c>
#else
#include "../common/gba.c"
#include "../starcraft/game.c"
#endif
#ifdef SC64_Q8_FLASH_HOST_STAGE
static Model device;
static GbaFlash1m flash={&device,rd,wr,20,0,0};
static int io_read(void *c,uint32_t p,void *v,uint32_t n){return gba_flash1m_read(c,p,v,n);}
static int io_write(void *c,uint32_t p,const void *v,uint32_t n){return gba_flash1m_program(c,p,v,n);}
static int io_erase(void *c,uint32_t p){return gba_flash1m_erase_sector(c,p);}
static GbaFlashJournalIo flash_io={&flash,io_read,io_erase,io_write};
const GbaFlashJournalIo *sc64_q8_flash_host_io(void){return &flash_io;}
#endif
static void file_read(const char *p,void *buf,size_t n)
{FILE *f=fopen(p,"rb");assert(f && fread(buf,1,n,f)==n && fgetc(f)==EOF && !fclose(f));}
static void file_write(const char *p,const void *buf,size_t n)
{FILE *f=fopen(p,"wb");assert(f && fwrite(buf,1,n,f)==n && !fclose(f));}
static void start(void)
{
    gba_init();game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;
    race_bot_races[0]=R_PROTOSS;race_skirmish_map=0;init_state();minerals=gas=1000;
}
#ifdef SC64_Q8_FLASH_HOST_STAGE
static void create_paid_fractional(void)
{
    int p=spawn_build(SIDE,B_PYLON,240,240,0),f=spawn_build(SIDE,B_FORGE,240,240,0);
    int a=spawn_unit(SIDE,U_DRAGOON,300,300),s=spawn_unit(SIDE,U_DRAGOON,340,300);
    unsigned hp,shield;assert(p>=0&&f>=0&&a>=0&&s>=0);
    assert(sc64_pr_stage_begin_slot(f,SC64_PR_GROUND_WEAPONS));
    u[a].shield=0;u[a].shield_remainder=0;
    /* Actual shared Ignore Armor packets create both persisted fractions. */
    hit_target_impl_ex(a,128,U_MARINE,ENEMY,SC64_STOCK_DMG_IGNORE_ARMOR,SC64_HIT_PACKET_Q8);
    hit_target_impl_ex(s,128,U_MARINE,ENEMY,SC64_STOCK_DMG_IGNORE_ARMOR,SC64_HIT_PACKET_Q8);
    assert(sc64_pr_q8_actor_hp(a,&hp)&&hp==(unsigned)u[a].maxhp*256u-128u);
    assert(sc64_pr_q8_actor_shield(s,&shield)&&shield==80u*256u-128u);
    /* The row codec can encode values the detached preflight rejects. Never
     * write a new Flash record for those invalid live Q8 states. */
    static u8 prior[131072];memcpy(prior,device.bytes,sizeof prior);
    int old_hp=u[a].hp;u8 old_debt=sc64_pr_hp_debt[a];
    u[a].hp=0;sc64_pr_hp_debt[a]=0;
    assert(!race_save_game() && !memcmp(prior,device.bytes,sizeof prior));
    u[a].hp=u[a].maxhp+1;
    assert(!race_save_game() && !memcmp(prior,device.bytes,sizeof prior));
    u[a].hp=old_hp;sc64_pr_hp_debt[a]=old_debt;
    /* The packed row admits target -1, but the actual outer preflight must
     * reject a mining order without a valid resource target. */
    int old_order=u[a].order,old_target=u[a].target;
    u[a].order=ORDER_MINE;u[a].target=-1;
    assert(!race_save_game() && !memcmp(prior,device.bytes,sizeof prior));
    u[a].order=old_order;u[a].target=old_target;
}
static void verify_loaded(void)
{
    int a=-1,s=-1,f=-1;unsigned hp,shield;
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].type==U_DRAGOON) {
        if(u[i].x==300)a=i;else if(u[i].x==340)s=i;
    }
    for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].type==B_FORGE&&b[i].level)f=i;
    assert(a>=0&&s>=0&&f>=0&&minerals==900&&gas==900);
    assert(sc64_pr_q8_actor_hp(a,&hp)&&hp==(unsigned)u[a].maxhp*256u-128u);
    assert(sc64_pr_q8_actor_shield(s,&shield)&&shield==80u*256u-128u);
    hit_target_impl_ex(a,128,U_MARINE,ENEMY,SC64_STOCK_DMG_IGNORE_ARMOR,SC64_HIT_PACKET_Q8);
    assert(sc64_pr_q8_actor_hp(a,&hp)&&hp==(unsigned)u[a].maxhp*256u-256u);
    Sc64PrPaidReceipt paid;assert(sc64_pr_stage_receipt(f,&paid)&&paid.payer==SIDE);
    unsigned left=b[f].queue_progress;assert(left);
    for(unsigned i=0;i<left;++i)sc64_pr_stage_tick();
    assert(!b[f].level&&sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]==1);
}
#endif
#ifndef SC64_Q8_FLASH_NO_FIXTURE_MAIN
int main(int argc,char **argv)
{
    assert(argc==3);start();
    if(!strcmp(argv[1],"old-write") || !strcmp(argv[1],"old-phase-write")) {
#ifndef SC64_Q8_FLASH_HOST_STAGE
        if(!strcmp(argv[1],"old-phase-write")) {
            int a=spawn_unit(SIDE,U_DRAGOON,300,300);assert(a>=0 && u[a].shield>0);
            --u[a].shield;u[a].shield_remainder=1;
        }
        assert(race_save_game());file_write(argv[2],host_save,sizeof host_save);
#else
        assert(0);
#endif
    }
#ifdef SC64_Q8_FLASH_HOST_STAGE
    else if(!strcmp(argv[1],"write")||!strcmp(argv[1],"migrate")) {
        memset(device.bytes,0xff,sizeof device.bytes);static u8 prior[32768];
        if(!strcmp(argv[1],"migrate")) {
            file_read(argv[2],device.bytes,32768);memcpy(prior,device.bytes,32768);
            assert(race_load_game());
        } else memcpy(prior,device.bytes,32768);
        create_paid_fractional();assert(race_save_game());
        assert(!memcmp(prior,device.bytes,32768));file_write(argv[2],device.bytes,sizeof device.bytes);
    } else if(!strcmp(argv[1],"read")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);
        assert(race_load_game());verify_loaded();
    } else if(!strcmp(argv[1],"fallback")) {
        file_read(argv[2],device.bytes,sizeof device.bytes);u8 legacy[32768];
        memcpy(legacy,device.bytes,sizeof legacy);
        GbaFlashJournalRecord newest;assert(gba_fj_probe(&flash_io,2,&newest)==GBA_FJ_OK);
        assert(newest.tag==SC64_Q8_FLASH_TAG && newest.length>SC64_Q8_FLASH_TAIL_BYTES);
        /* Corrupt the version marker and recompute the checksum only in the
         * negative HOST fixture. The older producer bytes stay untouched. */
        u32 meta=gba_fj_metadata(2),base_length=newest.length-SC64_Q8_FLASH_TAIL_BYTES;
        device.bytes[meta+16+base_length]^=1;
        gba_fj_put32(device.bytes+meta+8,gba_fj_hash(2166136261u,device.bytes+meta+16,newest.length));
        assert(race_load_game() && !memcmp(legacy,device.bytes,sizeof legacy));
        assert(minerals==1000 && gas==1000);
        for(unsigned i=0;i<sizeof sc64_pr_hp_debt;++i)assert(!sc64_pr_hp_debt[i]);
    } else if(!strcmp(argv[1],"read-old-phase")) {
        static u8 prior[32768];
        memset(device.bytes,0xff,sizeof device.bytes);file_read(argv[2],device.bytes,32768);
        memcpy(prior,device.bytes,sizeof prior);
        assert(race_load_game());int a=-1;
        for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].type==U_DRAGOON)a=i;
        assert(a>=0 && u[a].shield==79 && u[a].shield_remainder==1);
        assert(race_save_game() && !memcmp(prior,device.bytes,sizeof prior));
    } else if(!strcmp(argv[1],"unknown-hash") || !strcmp(argv[1],"unknown-length") ||
              !strcmp(argv[1],"mixed-47")) {
        static u8 prior[131072];
        memset(device.bytes,0xff,sizeof device.bytes);file_read(argv[2],device.bytes,32768);
        if(!strcmp(argv[1],"mixed-47")) {
            u32 at=16384u,tag=SC64_SAVE_TAG_CAMPAIGN_JOBS(SC64_TRANSPORT_SAVE_VERSION);
            gba_fj_put32(device.bytes+at,tag);gba_fj_put32(device.bytes+at+4,0);
        } else {
            u32 at=gba_fj_base(2),meta=gba_fj_metadata(2);
            u32 tag=(SC64_TRANSPORT_SAVE_VERSION&~SC64_SAVE_TAG_NAMESPACE_MASK)|0x00460000u;
            gba_fj_put32(device.bytes+at,tag);gba_fj_put32(device.bytes+meta,tag);
            gba_fj_put32(device.bytes+meta+4,!strcmp(argv[1],"unknown-length")?0u:1u);
            gba_fj_put32(device.bytes+meta+8,0u);gba_fj_put32(device.bytes+meta+12,2u);
        }
        memcpy(prior,device.bytes,sizeof prior);
        /* Read the supported predecessor, but refuse to overwrite or hide
         * an unsupported committed layout when writing a new epoch. */
        assert(race_load_game());assert(!race_save_game());
        assert(!memcmp(prior,device.bytes,sizeof prior));
    } else if(!strcmp(argv[1],"race-ownership")) {
        static u8 prior[131072];
        memset(device.bytes,0xff,sizeof device.bytes);file_read(argv[2],device.bytes,32768);
        assert(race_q8_flash_begin());
        u32 at=gba_fj_base(2),meta=gba_fj_metadata(2);
        u32 tag=(SC64_TRANSPORT_SAVE_VERSION&~SC64_SAVE_TAG_NAMESPACE_MASK)|0x00460000u;
        gba_fj_put32(device.bytes+at,tag);gba_fj_put32(device.bytes+meta,tag);
        gba_fj_put32(device.bytes+meta+4,0);
        memcpy(prior,device.bytes,sizeof prior);
        Sc64Q8FlashGuard guard={&flash_io,0};
        assert(!race_q8_flash_guard_erase(&guard,gba_fj_base(2)));
        assert(!memcmp(prior,device.bytes,sizeof prior));
    } else if(!strcmp(argv[1],"reject-new")) {
        static u8 prior[131072];static Unit old_units[MAX_UNITS];static Building old_buildings[MAX_BUILDINGS];
        file_read(argv[2],device.bytes,sizeof device.bytes);GbaFlashJournalRecord rec;
        assert(gba_fj_probe(&flash_io,2,&rec)==GBA_FJ_OK && rec.tag==SC64_Q8_FLASH_TAG);
        u32 meta=gba_fj_metadata(2),base_length=rec.length-SC64_Q8_FLASH_TAIL_BYTES;
        device.bytes[meta+16+base_length]^=1;
        gba_fj_put32(device.bytes+meta+8,gba_fj_hash(2166136261u,device.bytes+meta+16,rec.length));
        memcpy(prior,device.bytes,sizeof prior);memcpy(old_units,u,sizeof u);memcpy(old_buildings,b,sizeof b);
        assert(!race_load_game() && !race_save_game());
        assert(!memcmp(prior,device.bytes,sizeof prior) && !memcmp(old_units,u,sizeof u) && !memcmp(old_buildings,b,sizeof b));
    } else assert(0);
#endif
    puts("Q8 Flash actual game process: PASS");return 0;
}
#endif
