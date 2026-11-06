/* SPDX-License-Identifier: GPL-3.0-or-later
 * Separate processes exchange genuine journal bytes written by race_save_game.
 * Test-only constructed starting buildings; no retagging or outcome injection. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/game.c"
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS
static Sc64PrPaidReceipt test_receipt(int slot)
{
    Sc64PrPaidReceipt result={0};
    (void)sc64_pr_stage_receipt(slot,&result);
    return result;
}
#endif

static void start_custom(void)
{
    gba_init(); game_init(); race_mode=1; race_human=R_PROTOSS;
    race_bots=1; race_bot_races[0]=R_PROTOSS; race_skirmish_map=0; init_state();
    minerals=gas=1000;
}
static void read_sram(const char *path)
{
    FILE *f=fopen(path,"rb"); assert(f);
    assert(fread(host_save,1,sizeof host_save,f)==sizeof host_save);
    assert(fgetc(f)==EOF); assert(!fclose(f));
}
static void write_sram(const char *path)
{
    FILE *f=fopen(path,"wb"); assert(f);
    assert(fwrite(host_save,1,sizeof host_save,f)==sizeof host_save);
    assert(!fclose(f));
}
int main(int argc,char **argv)
{
    assert(argc==3); start_custom();
    if (!strcmp(argv[1],"old-write")) {
#if defined(SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL)
        assert(0 && "old producer must not enable job journal");
#else
        weapons_level=1; armor_level=2;
        int power=spawn_build(SIDE,B_PYLON,240,240,0);
        int gate=spawn_build(SIDE,B_GATEWAY,240,240,0);
        assert(power>=0 && gate>=0);
        supply_cap=200;
        assert(race_queue_unit(SIDE,gate,U_ZEALOT));
        assert(b[gate].queue_count==1 && b[gate].queue[0]==U_ZEALOT);
        destroy_building(gate,ENEMY);
        int live_forge=spawn_build(SIDE,B_FORGE,240,240,0);
        assert(live_forge==gate && !b[gate].queue_count && b[gate].queue[0]==U_ZEALOT);
        int archive=spawn_build(SIDE,B_P7_TEMPLAR_ARCHIVES,240,240,0);
        assert(archive>=0 && sc64_race_raw_building(b[archive].type)==165);
        int gate2=spawn_build(SIDE,B_GATEWAY,240,240,0);
        assert(gate2>=0 && race_queue_unit(SIDE,gate2,U_ZEALOT));
        destroy_building(gate2,ENEMY);
        int dead_forge=spawn_build(SIDE,B_FORGE,240,240,0);
        assert(dead_forge==gate2 && b[dead_forge].queue[0]==U_ZEALOT);
        destroy_building(dead_forge,ENEMY);
        assert(!b[dead_forge].alive && b[dead_forge].queue[0]==U_ZEALOT);
        assert(race_save_game()); write_sram(argv[2]);
#endif
    } else if (!strcmp(argv[1],"new-write") || !strcmp(argv[1],"new-write-over-old")) {
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
        if (!strcmp(argv[1],"new-write-over-old")) read_sram(argv[2]);
        int pylon=spawn_build(SIDE,B_PYLON,240,240,0);
        int forge=spawn_build(SIDE,B_FORGE,240,240,0);
        assert(pylon>=0 && forge>=0);
        for(int i=0;i<MAX_UNITS;++i) u[i].selected=0;
        selected_building=forge; menu_open=0;
#ifdef SC_FIELD_FEEDBACK
        menu_last=0;
#endif
        game_update(KEY_B,KEY_B); assert(menu_open);
        game_update(0,0); game_update(KEY_A,KEY_A);
        assert(test_receipt(forge).live && minerals==900 && gas==900);
        assert(race_save_game()); write_sram(argv[2]);
#else
        assert(0 && "new producer requires job journal");
#endif
    } else if (!strcmp(argv[1],"new-load")) {
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
        read_sram(argv[2]); assert(race_load_game());
        int found=-1;
        for(int i=0;i<MAX_BUILDINGS;++i) if(test_receipt(i).live) { assert(found<0); found=i; }
        assert(found>=0 && test_receipt(found).payer==SIDE &&
               test_receipt(found).family==SC64_PR_GROUND_WEAPONS &&
               minerals==900 && gas==900 && b[found].queue_progress>0);
        unsigned before=b[found].queue_progress; sc64_pr_stage_tick();
        assert(b[found].queue_progress==before-1);
        assert(sc64_pr_stage_cancel_slot(found,0));
        assert(minerals==1000 && gas==1000 && !test_receipt(found).live);
#else
        assert(0 && "new consumer requires job journal");
#endif
    } else if (!strcmp(argv[1],"old-load")) {
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
        read_sram(argv[2]); assert(race_load_game());
        assert(weapons_level==1 && armor_level==2);
        int live=-1,dead=-1;
        for(int i=0;i<MAX_BUILDINGS;++i) {
            assert(!test_receipt(i).live);
            if (b[i].type==B_FORGE && b[i].queue[0]==U_ZEALOT && !b[i].queue_count && !b[i].level) {
                if (b[i].alive) live=i; else dead=i;
            }
        }
        assert(live>=0 && dead>=0);
        int old_ore=minerals, old_gas=gas;
        assert(sc64_pr_stage_begin_slot(live,SC64_PR_GROUND_WEAPONS));
        assert(b[live].queue[1]==2 && minerals==old_ore-150 && gas==old_gas-150);
        assert(!b[dead].alive && b[dead].queue[0]==U_ZEALOT);
#else
        assert(0 && "consumer requires job journal");
#endif
    } else assert(0 && "unknown test mode");
    puts("research job journal process: ok"); return 0;
}
