/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Existing Z7 predecessor journal: trace the candidate's unmodified gates.
 * Source view is generated under ignored local storage; no guest image changes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include SC64_BOUNDARY_GAME
extern void guardian_test_snapshot(void *dst);
extern void guardian_test_restore(const void *src);
static unsigned char boundary_seed[32768], boundary_before[32768], boundary_after[32768];
int main(int argc,char **argv)
{
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fread(boundary_seed,1,sizeof(boundary_seed),f)==sizeof(boundary_seed));
    assert(fgetc(f)==EOF && !ferror(f));assert(!fclose(f));
    gba_init();game_init();guardian_test_restore(boundary_seed);guardian_test_snapshot(boundary_before);
    u32 tag,length,sequence;
    memcpy(&tag,boundary_seed,4);memcpy(&length,boundary_seed+4,4);
    memcpy(&sequence,boundary_seed+12,4);
    fprintf(stderr,"BOUNDARY envelope tag=%08lx len=%lu sequence=%lu capacity=%lu\n",
            (unsigned long)tag,(unsigned long)length,(unsigned long)sequence,
            (unsigned long)sizeof(race_impact_record));
    fprintf(stderr,"BOUNDARY accepted legacy=%08lx transport=%08lx egg=%08lx flight=%08lx\n",
            (unsigned long)SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,
            (unsigned long)SC64_TRANSPORT_SAVE_VERSION,
            (unsigned long)SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,
            (unsigned long)SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION);
    /* Independently identify both pre-callback selector barriers. This is a
     * baseline rejection witness, not permission to retag or truncate. */
    assert(tag==0x56435a54u && length==16312u && sequence==1u);
    assert(tag!=SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION &&
           tag!=SC64_TRANSPORT_SAVE_VERSION &&
           tag!=SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION &&
           tag!=SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION);
    assert(length>sizeof(race_impact_record));
    fprintf(stderr,"BOUNDARY BEGIN legacy Z7 load\n");
    int loaded=race_load_game();
    guardian_test_snapshot(boundary_after);
    fprintf(stderr,"BOUNDARY load=%d sram_unchanged=%d\n",loaded,!memcmp(boundary_before,boundary_after,sizeof(boundary_before)));
    assert(!loaded && !memcmp(boundary_before,boundary_after,sizeof(boundary_before)));
    race_mode=0;race_human=R_ZERG;race_selected_mission=SC64_Z7_GLOBAL_ID;
    init_state();assert(race_z7_active() && screen==SCREEN_PLAY);
    guardian_test_snapshot(boundary_before);
    fprintf(stderr,"BOUNDARY BEGIN current Z7 save\n");
    int saved=race_save_game();
    guardian_test_snapshot(boundary_after);
    fprintf(stderr,"BOUNDARY save=%d sram_unchanged=%d\n",saved,!memcmp(boundary_before,boundary_after,sizeof(boundary_before)));
    assert(!saved && !memcmp(boundary_before,boundary_after,sizeof(boundary_before)));
    return 0;
}
