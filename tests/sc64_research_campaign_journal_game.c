/* SPDX-License-Identifier: GPL-3.0-or-later
 * Authored HOST world; separate processes exchange genuine game SRAM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef SC64_CAMPAIGN_BASELINE_WRITER
#include <gba.c>
#include <game.c>
#else
#include "../common/gba.c"
#include "../starcraft/game.c"
#endif
static void transfer(const char *path,int write_out) {
    FILE *f=fopen(path,write_out?"wb":"rb"); assert(f);
    if (write_out) assert(fwrite(host_save,1,sizeof host_save,f)==sizeof host_save);
    else { assert(fread(host_save,1,sizeof host_save,f)==sizeof host_save); assert(fgetc(f)==EOF); }
    assert(!fclose(f));
}
int main(int argc,char **argv) {
    assert(argc==3); gba_init(); game_init(); race_mode=0;
    race_selected_mission=SC64_RACE_MISSION_P1;
    if (!strncmp(argv[1],"custom-",7)) {
        race_mode=1; race_human=R_PROTOSS; race_bots=1;
        race_bot_races[0]=R_PROTOSS; race_skirmish_map=0;
    }
    init_state();
    if (!strcmp(argv[1],"old-write")) {
#if defined(SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL)
        assert(0 && "old writer must have no research flags");
#else
        minerals=gas=1000; assert(race_save_game()); transfer(argv[2],1);
#endif
    } else if (!strcmp(argv[1],"write") || !strcmp(argv[1],"over-old") || !strcmp(argv[1],"custom-write") || !strcmp(argv[1],"core-write")) {
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
        if (!strcmp(argv[1],"over-old")) { transfer(argv[2],0); assert(race_load_game()); }

        int core=!strcmp(argv[1],"core-write");
        int p=spawn_build(SIDE,B_PYLON,240,240,0);
        int f=spawn_build(SIDE,core?B_CYBERNETICS_CORE:B_FORGE,240,240,0); assert(p>=0&&f>=0);
        minerals=gas=1000;
        assert(sc64_pr_stage_begin_slot(f,core?SC64_PR_SINGULARITY:SC64_PR_GROUND_WEAPONS));
        assert(minerals==(core?850:900)&&gas==minerals);
        u8 before[sizeof host_save]; memcpy(before,host_save,sizeof before);
        b[f].queue[0]=ENEMY;
        assert(!race_save_game() && !memcmp(before,host_save,sizeof before));
        b[f].queue[0]=SIDE;
        assert(race_save_game());
        if (!strcmp(argv[1],"custom-write")) {
            assert(sram_get32(host_save)==SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION ||
                   sram_get32(host_save+SAVE_SLOT_SIZE)==SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION);
        }
        transfer(argv[2],1);
#else
        assert(0 && "new writer requires campaign journal");
#endif
    } else if (!strcmp(argv[1],"old-load") || !strcmp(argv[1],"fallback") || !strcmp(argv[1],"reject")) {
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
        transfer(argv[2],0);
        if (!strcmp(argv[1],"fallback") || !strcmp(argv[1],"reject")) {
            int slot=-1;
            for(int i=0;i<2;++i) if(sram_get32(host_save+i*SAVE_SLOT_SIZE)==SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION) slot=i;
            assert(slot>=0); u8 *row=host_save+slot*SAVE_SLOT_SIZE;
            u32 n=sram_get32(row+4); assert(n>offsetof(RaceSave,state)+2*sizeof(int));
            /* Deliberate checksum-valid bad context, only a negative fixture. */
            int bad=R_ZERG; memcpy(row+16+offsetof(RaceSave,state)+sizeof(int),&bad,sizeof bad);
            sram_put32(row+8,hash_bytes(row+16,n));
        }
        u8 unchanged[sizeof host_save]; memcpy(unchanged,host_save,sizeof unchanged);
        if (!strcmp(argv[1],"reject")) {
            int old_minerals=minerals,old_gas=gas,old_mission=race_selected_mission;
            Sc64ProtossResearchLevels old_levels=sc64_experimental_research_levels;
            static Unit old_units[MAX_UNITS]; static Building old_buildings[MAX_BUILDINGS];
            memcpy(old_units,u,sizeof old_units); memcpy(old_buildings,b,sizeof old_buildings);
            assert(!race_load_game());
            assert(!memcmp(old_units,u,sizeof old_units) && !memcmp(old_buildings,b,sizeof old_buildings));
            assert(minerals==old_minerals && gas==old_gas && race_selected_mission==old_mission &&
                   !memcmp(&old_levels,&sc64_experimental_research_levels,sizeof old_levels));
            assert(!memcmp(unchanged,host_save,sizeof unchanged));
            puts("campaign journal atomic reject: PASS"); return 0;
        }
        assert(race_load_game()); assert(!memcmp(unchanged,host_save,sizeof unchanged));
        assert(minerals==1000&&gas==1000);
        for(int i=0;i<MAX_BUILDINGS;++i) { Sc64PrPaidReceipt paid; assert(!sc64_pr_stage_receipt(i,&paid)); }
#else
        assert(0 && "consumer requires campaign journal");
#endif
    } else if (!strcmp(argv[1],"load") || !strcmp(argv[1],"custom-load") || !strcmp(argv[1],"core-load")) {
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
        transfer(argv[2],0); assert(race_load_game());
        int f=-1; for(int i=0;i<MAX_BUILDINGS;++i) {
            Sc64PrPaidReceipt paid;
            if(sc64_pr_stage_receipt(i,&paid)) { assert(f<0);f=i;assert(paid.payer==SIDE); }
        }
        int core=!strcmp(argv[1],"core-load");
        assert(f>=0&&minerals==(core?850:900)&&gas==minerals);
        unsigned remaining=b[f].queue_progress; assert(remaining>0);
        for(unsigned i=0;i<remaining;++i) sc64_pr_stage_tick();
        assert(!b[f].level && sc64_experimental_research_levels.level[SIDE][core?SC64_PR_SINGULARITY:SC64_PR_GROUND_WEAPONS]==1);
#else
        assert(0 && "consumer requires campaign journal");
#endif
    } else assert(0);
    puts("campaign journal process: PASS"); return 0;
}
