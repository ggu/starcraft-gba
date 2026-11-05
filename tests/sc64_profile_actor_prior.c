/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Two HOST processes; predecessor binary uses its actual checkout source. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef SC64_PRIOR_WRITER
#include SC64_PRIOR_GAME
#else
#include "../starcraft/game.c"
#endif
extern void guardian_test_reset(void);
extern void guardian_test_snapshot(void *);
extern void guardian_test_restore(const void *);
static unsigned char sram[32768];
static void transfer(const char *path,int write)
{
    FILE *f=fopen(path,write?"wb":"rb");assert(f);
    if(write) { guardian_test_snapshot(sram);assert(fwrite(sram,1,sizeof(sram),f)==sizeof(sram)); }
    else { assert(fread(sram,1,sizeof(sram),f)==sizeof(sram));guardian_test_restore(sram); }
    assert(!fclose(f));
}
static void check_public_ids(const char *path,int write)
{
    char name[512];assert(snprintf(name,sizeof(name),"%s.ids",path)>0);
    FILE *f=fopen(name,write?"w":"r");assert(f);
    int count=SC_RU_LEGACY_COUNT,egg=SC64_SHARED_EGG_TYPE;
    if(write) assert(fprintf(f,"%d %d\n",count,egg)>0);
    else { int old_count,old_egg;assert(fscanf(f,"%d %d\n",&old_count,&old_egg)==2);
           assert(old_count==count && old_egg==egg); }
    for(int i=0;i<count;i++) {
        int raw=sc64_race_raw_unit(i);const char *label=sc64_race_unit_name(i);
        if(write) assert(fprintf(f,"%d %d %s\n",i,raw,label)>0);
        else { int old_id,old_raw;char old_label[128];
            assert(fscanf(f,"%d %d %127[^\n]\n",&old_id,&old_raw,old_label)==3);
            assert(old_id==i && old_raw==raw && !strcmp(old_label,label)); }
    }
    assert(!fclose(f));
}
static void start(void)
{
    guardian_test_reset();game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();assert(screen==SCREEN_PLAY);
}
int main(int argc,char **argv)
{
    assert(argc==2);start();
#ifdef SC64_PRIOR_WRITER
    int hall=-1;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive && b[i].owner==SIDE && sc64_race_is_zerg_townhall(b[i].type)){hall=i;break;}
    assert(hall>=0);b[hall].larva=2;minerals=1000;gas=1000;
    assert(race_queue_unit(SIDE,hall,U_DRONE));
    int egg=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].type==SC64_SHARED_EGG_TYPE){egg=i;break;}
    /* Choose a real unobstructed route on this profile's loaded map. */
    int px=-1,py=-1;
    for(int y=96;y+80<race_rows*16 && py<0;y+=16)
      for(int x=96;x+32<race_cols*16 && py<0;x+=16) {
        int clear=!terrain_blocked(x/16,y/16) && !terrain_blocked(x/16,(y+60)/16) &&
                  direct_path_clear(x,y,x,y+60);
        for(int j=0;j<MAX_BUILDINGS && clear;++j) if(b[j].alive &&
            dist2(x,y,b[j].x,b[j].y)<80*80) clear=0;
        for(int j=0;j<MAX_UNITS && clear;++j) if(u[j].alive &&
            dist2(x,y,u[j].x,u[j].y)<60*60) clear=0;
        if(clear) {px=x;py=y;}
      }
    assert(px>=0 && py>=0);
    int legacy=spawn_unit(SIDE,U_OVERLORD,100,100);assert(legacy>=0);
    u[legacy].raw_type=36;u[legacy].raw_owner=-1;
    int reaver=spawn_unit(SIDE,U_REAVER,px,py);assert(reaver>=0);
    int shuttle_type=sc64_race_unit_from_raw(69);assert(shuttle_type>=0);
    int shuttle=spawn_unit(SIDE,shuttle_type,px+20,py);assert(shuttle>=0);
    assert(egg>=0 && race_save_game());transfer(argv[1],1);check_public_ids(argv[1],1);
#else
    check_public_ids(argv[1],0);transfer(argv[1],0);assert(race_load_game());int egg=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].type==SC64_SHARED_EGG_TYPE){egg=i;break;}
    assert(egg>=0 && u[egg].raw_type==36 && u[egg].target==U_DRONE);
    int reaver=-1,shuttle=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive) {
        if(u[i].type==U_REAVER) reaver=i;
        if(u[i].type==sc64_race_unit_from_raw(69)) shuttle=i;
    }
    assert(reaver>=0 && shuttle>=0 && sc64_race_raw_unit(u[shuttle].type)==69);
    Sc64ScarabParentFields f;
    assert(sc64_scarab_parent_fields_unpack(u[reaver].carry,u[reaver].work_timer,&f));
    assert(f.queued==0 && f.active_slot==SC64_SCARAB_FIELD_NO_SLOT);
#endif
    printf("legacy bound=%d Egg=%d\n",SC_RU_LEGACY_COUNT,SC64_SHARED_EGG_TYPE);
    return 0;
}
