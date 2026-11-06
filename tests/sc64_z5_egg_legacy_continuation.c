/* SPDX-License-Identifier: GPL-3.0-or-later
 * Genuine old-tag Z5 HOST save, fresh staged reader and bounded input ticks. */
#define main sc64_z9_existing_fixture_main
#include "sc64_z9_game.c"
#undef main
int fixture_import_host_save(const char *path);
int fixture_host_save_equals(const char *path);
int main(int argc,char **argv)
{
    assert(argc==2);game_init();assert(fixture_import_host_save(argv[1]));
    RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_TRANSPORT_SAVE_VERSION && !(plan.egg_auth.features&SC64_STAGE_SAVE_EGG));
    assert(race_load_game() && !race_mode && race_selected_mission==SC64_Z5_GLOBAL_ID && race_z5_active());
    assert(!race_shared_egg_context());
    for(int i=0;i<MAX_UNITS;++i)assert(!u[i].alive || u[i].type!=U_Z7_EGG);
    int advanced=0,paused=0,other=0,ack=0;held_keys=0;
    for(int i=0;i<180;++i) {
        int before=frame;
        if(screen==SCREEN_PLAY && race_z5_world_paused()) {press(KEY_A);++ack;}
        else input(0);
        if(frame>before)++advanced;else if(race_z5_world_paused())++paused;else ++other;
    }
    assert(advanced>0 && race_selected_mission==SC64_Z5_GLOBAL_ID);
    assert(fixture_host_save_equals(argv[1]));
    printf("Z5 old-tag staged fresh continuation: advancing=%d paused=%d other=%d acknowledgments=%d\n",advanced,paused,other,ack);
    return 0;
}
