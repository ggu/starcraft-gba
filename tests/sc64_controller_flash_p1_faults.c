/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored HOST I/O faults against the real P1 game journal path. */
#define SC64_Q8_FLASH_NO_FIXTURE_MAIN
#include "sc64_q8_flash_game.c"
static int mode, fired, erases, programs;
static u32 mutate_at, fault_at;
static u8 after_external[131072];
static int fault_read(void *c,u32 p,void *v,u32 n)
{
    if(mode==1 && !fired && sc64_q8_flash_has_previous && !sc64_q8_flash_snapshot.plan) {
        /* Model a concurrent, checksum-preserving sequence update after selection. */
        gba_fj_put32(device.bytes+mutate_at,sc64_q8_flash_previous.sequence+10);
        memcpy(after_external,device.bytes,sizeof after_external);fired=1;
    }
    if(mode==2 && !fired) {fired=1;return 0;}
    if(mode==3 && !fired && p==fault_at && n==8) {fired=1;return 0;}
    return io_read(c,p,v,n);
}
static int fault_erase(void *c,u32 p){++erases;return io_erase(c,p);}
static int fault_program(void *c,u32 p,const void *v,u32 n){++programs;return io_write(c,p,v,n);}
int main(int argc,char **argv)
{
    assert(argc==3);gba_init();game_init();race_mode=0;
    race_selected_mission=SC64_RACE_MISSION_P1;init_state();
    file_read(argv[2],device.bytes,sizeof device.bytes);assert(race_load_game());
    GbaFlashJournalRecord a,b,*newer;
    assert(gba_fj_probe(&flash_io,2,&a)==GBA_FJ_OK && gba_fj_probe(&flash_io,3,&b)==GBA_FJ_OK);
    newer=gba_fj_newer(a.sequence,b.sequence)?&a:&b;
    static Unit units[MAX_UNITS];static Building buildings[MAX_BUILDINGS];
    static u8 controllers[MAX_UNITS],building_controllers[MAX_BUILDINGS],debts[MAX_UNITS+MAX_BUILDINGS],before[131072];
    Sc64ProtossResearchLevels levels=sc64_experimental_research_levels;
    memcpy(units,u,sizeof u);memcpy(buildings,b,sizeof b);
    memcpy(controllers,sc64_current_unit,sizeof controllers);memcpy(building_controllers,sc64_current_building,sizeof building_controllers);
    memcpy(debts,sc64_pr_hp_debt,sizeof debts);memcpy(before,device.bytes,sizeof before);
    int prior_minerals=minerals,prior_gas=gas,prior_frame=frame;
    flash_io.read=fault_read;flash_io.erase_sector=fault_erase;flash_io.program=fault_program;
    sc64_q8_flash_has_previous=0;
    if(!strcmp(argv[1],"stale-selected") || !strcmp(argv[1],"stale-epoch")) {
        GbaFlashJournalRecord *victim=!strcmp(argv[1],"stale-selected")?newer:(newer==&a?&b:&a);
        mutate_at=gba_fj_metadata(victim->index)+12;mode=1;assert(!race_save_game());
        assert(fired && !memcmp(after_external,device.bytes,sizeof after_external));
    } else if(!strcmp(argv[1],"save-read")) {
        mode=2;assert(!race_save_game());assert(fired && !memcmp(before,device.bytes,sizeof before));
    } else if(!strcmp(argv[1],"load-read")) {
        fault_at=gba_fj_metadata(newer->index)+16+newer->length-SC64_CONTROLLER_FLASH_BYTES;
        mode=3;assert(!race_load_game());assert(fired && !memcmp(before,device.bytes,sizeof before));
    } else assert(0);
    assert(!erases && !programs && minerals==prior_minerals && gas==prior_gas && frame==prior_frame);
    assert(!memcmp(units,u,sizeof u) && !memcmp(buildings,b,sizeof b));
    assert(!memcmp(controllers,sc64_current_unit,sizeof controllers) && !memcmp(building_controllers,sc64_current_building,sizeof building_controllers));
    assert(!memcmp(debts,sc64_pr_hp_debt,sizeof debts) && !memcmp(&levels,&sc64_experimental_research_levels,sizeof levels));
    puts("P1 Flash actual-path fault: PASS");return 0;
}
