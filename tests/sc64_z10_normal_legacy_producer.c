/* SPDX-License-Identifier: GPL-3.0-or-later
 * Fixture compiled against an explicitly supplied unmodified producer tree. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef SC64_Z10_OLD_GAME
#error "Specify the verified original game.c"
#endif
#ifndef SC64_Z10_OLD_GBA
#error "Specify the verified original common/gba.c"
#endif
#include SC64_Z10_OLD_GBA
#include SC64_Z10_OLD_GAME
#if !defined(GBA_HOST) || !defined(SC64_Z10_RUNTIME) || !defined(SC64_WORKER_CARGO_KIND) || \
    defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT) || defined(SC64_SCARAB_RUNTIME)
#error "Use the original normal Z10 producer vector"
#endif
_Static_assert(SAVE_VERSION==0x52435a61u && SC64_TRANSPORT_SAVE_VERSION==0x56435a61u,
               "unexpected normal Z10 generation-49 tags");
static unsigned held;
static void press(unsigned key)
{
    game_update((u16)key,(u16)(key&~held));held=key;
    game_update(0,0);held=0;
}
static void launch(void)
{
    game_init();held=0;assert(race_front.page==SC_FRONT_HOME);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z10_GLOBAL_ID)==9);
    press(KEY_A);assert(race_front.page==SC_FRONT_MISSION && race_front.faction==SC_FRONT_ZERG);
    for(int i=0;i<9;++i)press(KEY_DOWN);
    assert(race_front.mission==SC64_Z10_GLOBAL_ID);press(KEY_START);
    assert(screen==SCREEN_PLAY && !race_mode && race_selected_mission==SC64_Z10_GLOBAL_ID && race_zl_active());
}
static void transfer(const char *path,int write)
{
    FILE *f=fopen(path,write?"wb":"rb");assert(f);
    if(write)assert(fwrite(host_save,1,sizeof host_save,f)==sizeof host_save);
    else assert(fread(host_save,1,sizeof host_save,f)==sizeof host_save && fgetc(f)==EOF);
    assert(!fclose(f));
}
static void report(void)
{
    int reavers=0,shuttles=0;
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive){
        if(u[i].type==U_REAVER){assert(u[i].raw_type==83 && u[i].raw_owner>=0);++reavers;}
        if(u[i].type==U_SHARED_SHUTTLE)++shuttles;
    }
    assert(reavers>0);
    printf("mission=%d frame=%d minerals=%d gas=%d reavers=%d shuttles=%d inner=%08x outer=%08x unit_count=%d shuttle=%d building_count=%d racesave=%zu impacts=%d unitrow=%zu buildrow=%zu stateoff=%zu unitoff=%zu buildoff=%zu\n",
        race_selected_mission,frame,minerals,gas,reavers,shuttles,(unsigned)SAVE_VERSION,
        (unsigned)SC64_TRANSPORT_SAVE_VERSION,SC_RU_COUNT,U_SHARED_SHUTTLE,SC_RB_COUNT,
        sizeof(RaceSave),RACE_IMPACT_SAVE_CAPACITY,sizeof(race_data.units[0]),sizeof(race_data.buildings[0]),
        offsetof(RaceSave,state),offsetof(RaceSave,units),offsetof(RaceSave,buildings));
}
int main(int argc,char **argv)
{
    assert(argc==3);gba_init();
    if(!strcmp(argv[1],"write")) {
        launch();report();assert(race_save_game());transfer(argv[2],1);
    } else if(!strcmp(argv[1],"read")) {
        game_init();transfer(argv[2],0);assert(race_load_game());
        assert(!race_mode && race_selected_mission==SC64_Z10_GLOBAL_ID && race_human==R_ZERG);
        report();
    } else assert(0);
    puts("normal generation-49 Z10 fresh journal: PASS");return 0;
}
