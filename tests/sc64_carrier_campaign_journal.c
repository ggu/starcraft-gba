/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Boot each ordinary mission through the frontend. Write binary HOST payloads
 * for comparison between the standard and selector-on products. */
#include "../starcraft/game.c"
#define PROGRESSION_FACTION SC64_CAMPAIGN_RACE
#define PROGRESSION_MISSION 0
#define PROGRESSION_NEXT 1
#include "sc64_campaign_progression.h"

#ifndef SC64_TRANSPORT_SAVE_VERSION
#error "Use the canonical Terran or Z10 SRAM journal"
#endif

static u8 before[RACE_IMPACT_RECORD_MAX], after[RACE_IMPACT_RECORD_MAX];

static void reset_journal(void)
{
    for (int i=0;i<2;++i) {
        save_test_corrupt_latest(0,SC64_TRANSPORT_SAVE_VERSION);
        save_test_corrupt_latest(0,SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION);
    }
}

int main(int argc,char **argv)
{
    assert(argc==2);
    FILE *out=fopen(argv[1],"wb");
    assert(out);
    gba_init();
    const int count=sc64_catalog_playable_count(PROGRESSION_FACTION);
    assert(count==10);
    for (int ordinal=0;ordinal<count;++ordinal) {
        const int mission=sc64_catalog_playable_at(PROGRESSION_FACTION,ordinal);
        reset_journal();
        progression_launch_from_menu(mission);
        for (int i=0;i<30;++i) progression_input(0);
        progression_press(KEY_START);
        assert(screen==SCREEN_PAUSE);
        progression_press(KEY_B);
        u32 length,reloaded;
        assert(save_read_latest(before,sizeof(before),SC64_TRANSPORT_SAVE_VERSION,&length));
        assert(length>=sizeof(RaceSave)-SC64_BC_SAVE_EXTRA_BYTES);
        assert(fwrite(&length,1,sizeof(length),out)==sizeof(length));
        assert(fwrite(before,1,length,out)==length);
        const int saved_frame=frame;
        const u32 world=progression_world_digest();

        game_init(); progression_held=0;
        progression_input(KEY_L);
        assert(screen==SCREEN_PLAY && !race_mode && race_selected_mission==mission);
        assert(frame==saved_frame && progression_world_digest()==world);
        progression_press(KEY_START);
        assert(screen==SCREEN_PAUSE);
        progression_press(KEY_B);
        assert(save_read_latest(after,sizeof(after),SC64_TRANSPORT_SAVE_VERSION,&reloaded));
        assert(reloaded==length && !memcmp(before,after,length));
#if defined(SC64_TERRAN_CARRIER_PROBE)
        assert(!save_read_latest(after,sizeof(after),SC64_TERRAN_CARRIER_TAG,&reloaded));
#elif defined(SC64_ZERG_CARRIER_PROBE)
        assert(!save_read_latest(after,sizeof(after),SC64_ZERG_CARRIER_TAG,&reloaded));
#endif
        printf("Campaign mission %d: ordinary tag %08lx, %lu bytes, exact re-save\n",
               mission,(unsigned long)SC64_TRANSPORT_SAVE_VERSION,(unsigned long)length);
    }
    assert(fclose(out)==0);
    return 0;
}
