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
    int reaver=spawn_unit(SIDE,U_REAVER,300,300);assert(reaver>=0);
    int shuttle_type=sc64_race_unit_from_raw(69);assert(shuttle_type>=0);
    int shuttle=spawn_unit(SIDE,shuttle_type,320,300);assert(shuttle>=0);
    assert(egg>=0 && race_save_game());transfer(argv[1],1);
#else
    transfer(argv[1],0);assert(race_load_game());int egg=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].type==SC64_SHARED_EGG_TYPE){egg=i;break;}
    assert(egg>=0 && u[egg].raw_type==36 && u[egg].target==U_DRONE);
    int reaver=-1,shuttle=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive) {
        if(u[i].type==U_REAVER && u[i].x==300) reaver=i;
        if(u[i].type==sc64_race_unit_from_raw(69) && u[i].x==320) shuttle=i;
    }
    assert(reaver>=0 && shuttle>=0 && sc64_race_raw_unit(u[shuttle].type)==69);
    Sc64ScarabParentFields f;
    assert(sc64_scarab_parent_fields_unpack(u[reaver].carry,u[reaver].work_timer,&f));
    assert(f.queued==0 && f.active_slot==SC64_SCARAB_FIELD_NO_SLOT);
#endif
    printf("legacy bound=%d Egg=%d\n",SC_RU_LEGACY_COUNT,SC64_SHARED_EGG_TYPE);
    return 0;
}
