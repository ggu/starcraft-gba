/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only Z9 candidate binding: no staged Egg journal admission here. */
#define main sc64_z9_existing_fixture_main
#include "sc64_z9_game.c"
#undef main
#ifndef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
#error "This fixture exercises the stage-specific detached mission binding"
#endif
int main(void)
{
    fixture_clear_candidate_records();launch_z9_from_normal_menu();assert(race_save_game());
    static RaceImpactRecord envelope;static RaceSave candidate;u32 length=0;
    assert(save_read_latest(&envelope,sizeof(envelope),SC64_TRANSPORT_SAVE_VERSION,&length));
    assert(length>=sizeof(RaceSave));candidate=envelope.save;
    int width=race_cols,height=race_rows;
    assert(race_zl_save_candidate_world_valid_for(&candidate,width,height,0));
    race_selected_mission=SC64_Z7_GLOBAL_ID;race_mode=1;
    static Sc64ZlSchedulerState scratch;
    const Sc64ZlMission *mission=race_zl_mission_for(candidate.state[2]);
    assert(mission && race_zl_save_candidate_world_valid_for(&candidate,width,height,0));
    assert(race_zl_unpack_candidate_for(&candidate.zl,&scratch,mission));
    assert(race_selected_mission==SC64_Z7_GLOBAL_ID && race_mode==1);
    assert(race_load_game());assert(!race_mode && race_selected_mission==SC64_Z9_GLOBAL_ID);
    puts("detached Z9 candidate mission binding passed");return 0;
}
