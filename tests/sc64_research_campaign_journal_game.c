/* SPDX-License-Identifier: GPL-3.0-or-later
 * Authored HOST world; separate processes exchange genuine game SRAM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/game.c"
static void transfer(const char *path,int write_out) {
    FILE *f=fopen(path,write_out?"wb":"rb"); assert(f);
    if (write_out) assert(fwrite(host_save,1,sizeof host_save,f)==sizeof host_save);
    else { assert(fread(host_save,1,sizeof host_save,f)==sizeof host_save); assert(fgetc(f)==EOF); }
    assert(!fclose(f));
}
int main(int argc,char **argv) {
    assert(argc==3); gba_init(); game_init(); race_mode=0;
    race_selected_mission=SC64_RACE_MISSION_P1; init_state();
    if (!strcmp(argv[1],"write")) {
        int p=spawn_build(SIDE,B_PYLON,240,240,0);
        int f=spawn_build(SIDE,B_FORGE,240,240,0); assert(p>=0&&f>=0);
        minerals=gas=1000;
        assert(sc64_pr_stage_begin_slot(f,SC64_PR_GROUND_WEAPONS));
        assert(minerals==900&&gas==900);
        assert(race_save_game()); transfer(argv[2],1);
    } else if (!strcmp(argv[1],"load")) {
        transfer(argv[2],0); assert(race_load_game());
        int f=-1; for(int i=0;i<MAX_BUILDINGS;++i) {
            Sc64PrPaidReceipt paid;
            if(sc64_pr_stage_receipt(i,&paid)) { assert(f<0);f=i;assert(paid.payer==SIDE); }
        }
        assert(f>=0&&minerals==900&&gas==900);
        unsigned remaining=b[f].queue_progress; assert(remaining>0);
        for(unsigned i=0;i<remaining;++i) sc64_pr_stage_tick();
        assert(!b[f].level && sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]==1);
    } else assert(0);
    puts("campaign journal process: PASS"); return 0;
}
