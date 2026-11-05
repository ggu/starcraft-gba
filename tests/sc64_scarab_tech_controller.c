/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST controller: one real custom opponent, buttons and a constructed bank.
 * No prerequisite building, Reaver or target is spawned by this fixture. */
#ifndef SC64_SCARAB_TECH_PATH_HOST_PROBE
#error "Use the explicit full-service HOST tech-path probe"
#endif
#define SC64_CATALOG_BUTTONS_ENTRY sc64_catalog_buttons_unused_main
#include "sc64_skirmish_catalog_buttons.c"
#undef SC64_CATALOG_BUTTONS_ENTRY

extern void guardian_test_snapshot(void *);
extern void guardian_test_restore(const void *);
static unsigned char controller_sram[32768];

static void setup_options(void)
{
    option_map=0;option_human=R_PROTOSS;option_bots=1;option_bot_races[0]=R_ZERG;
    match_limit=90000;gba_init();assert(launch_match());
}

static int sole_reaver(void)
{
    int found=-1;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE &&
                                      sc64_race_raw_unit(u[i].type)==83) {
        assert(found<0);found=i;
    }
    return found;
}

static void write_progress(const char *path)
{
    setup_options();
    /* Explicit setup only: ample synthetic funds and initially idle workers.
     * All subsequent world changes pass through button input/game_update. */
    minerals=gas=5000;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE &&
                                      sc64_race_is_worker(u[i].type))u[i].order=ORDER_IDLE;
    assert(build_by_buttons(156,0)>=0); /* Pylon */
    assert(build_by_buttons(160,0)>=0); /* Gateway */
    assert(build_by_buttons(164,0)>=0); /* Cybernetics Core */
    int facility=build_by_buttons(155,0);assert(facility>=0);
    assert(build_by_buttons(171,0)>=0); /* Robotics Support Bay */
    produce_by_buttons(facility,83);
    int parent=sole_reaver();assert(parent>=0 && select_unit(parent));
    Sc64ScarabParent p;Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(parent,0,&p,&counts) && !p.queued && !counts.inside);
    int row=race_context_command_base(RACE_CONTEXT_SCARAB);
    assert(!strcmp(race_menu_label(row),"BUILD SCARAB") && choose_menu_command(row));
    assert(race_scarab_parent_read(parent,0,&p,&counts) && p.queued==1);
    for(int i=0;i<1500 && !counts.inside;++i) {
        assert(screen==SCREEN_PLAY && !won && !lost);input(0);
        assert(race_scarab_parent_read(parent,0,&p,&counts));
    }
    assert(counts.inside==1 && !p.queued);
    tap(KEY_START);assert(screen==SCREEN_PAUSE);tap(KEY_B);
    assert((pause_notice & 0xc0)==UI_NOTICE_SAVED);
    guardian_test_snapshot(controller_sram);
    FILE *f=fopen(path,"wb");assert(f && fwrite(controller_sram,1,sizeof(controller_sram),f)==sizeof(controller_sram));
    assert(!fclose(f));
    printf("buttons built tech, produced Reaver and stocked Scarab; saved at tick %u\n",host_ticks);
}

static void read_progress(const char *path)
{
    FILE *f=fopen(path,"rb");assert(f && fread(controller_sram,1,sizeof(controller_sram),f)==sizeof(controller_sram));
    assert(!fclose(f));setup_options();guardian_test_restore(controller_sram);
    tap(KEY_START);assert(screen==SCREEN_PAUSE);tap(KEY_L);assert(screen==SCREEN_PLAY);
    assert(race_mode && race_human==R_PROTOSS && race_bots==1);
    assert(find_raw_building(156)>=0 && find_raw_building(160)>=0 &&
           find_raw_building(164)>=0 && find_raw_building(155)>=0 && find_raw_building(171)>=0);
    int parent=sole_reaver();Sc64ScarabParent p;Sc64ScarabCounts counts;
    assert(parent>=0 && race_scarab_parent_read(parent,0,&p,&counts));
    assert(counts.inside==1 && !p.queued);
    for(int i=0;i<60;++i) { assert(screen==SCREEN_PLAY && !won && !lost);input(0); }
    assert(race_scarab_parent_read(parent,0,&p,&counts) && counts.inside==1);
    puts("fresh HOST process loaded tech and paid ammunition and continued");
}

int main(int argc,char **argv)
{
    assert(argc==3);
    if(!strcmp(argv[1],"write"))write_progress(argv[2]);
    else {assert(!strcmp(argv[1],"read"));read_progress(argv[2]);}
    return 0;
}
