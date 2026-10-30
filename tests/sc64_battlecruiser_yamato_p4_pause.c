/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Host-seeded P4 source pause; no ordinary-gameplay launch claim. */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void) {
    game_init(); game_update(KEY_A,KEY_A);game_update(KEY_RIGHT,KEY_RIGHT);
    game_update(KEY_DOWN,KEY_DOWN);game_update(KEY_A,KEY_A);game_update(KEY_START,KEY_START);
    assert(screen==SCREEN_PLAY && race_p4_active());
    int a=spawn_unit(ENEMY,unit_raw(12),500,500),t=spawn_unit(SIDE,unit_raw(0),640,500);
    assert(a>=0 && t>=0);u[a].raw_owner=0;u[a].work_timer=64<<3;u[a].carry=200*256;
    sc64_battlecruiser_tech_bits=SC64_BC_TECH_YAMATO;
    assert(race_bc_yamato_issue(a,t)); if(!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(race_bc_yamato_tick_caster(a));
    for(int n=0;n<55;++n) {++frame;assert(race_bc_yamato_tick_caster(a));}
    assert(race_bc_yamato_launch_resolved(0,64,0,1280,1000));
    /* Host seeding advanced frame directly; reconcile the campaign clock
     * before testing the real paused-update and save paths. */
    race_campaign.elapsed_frames=(u32)frame;
    Sc64BcYamatoShot shot=sc64_bc_shots[0]; int energy=u[a].carry, timer=u[a].work_timer;
    int due=sc64_battlecruiser_source_tick_due(); unsigned old_offset=sc64_bc_pause_offset;
    int before_frame=frame; game_update(KEY_START,KEY_START); assert(screen==SCREEN_PAUSE);
    for(int n=0;n<7;++n) {
        game_update(0,0);
        assert(frame==before_frame && sc64_bc_pause_offset==old_offset &&
               !memcmp(&shot,&sc64_bc_shots[0],sizeof shot) && u[a].carry==energy && u[a].work_timer==timer);
    }
    game_update(KEY_START,KEY_START); assert(screen==SCREEN_PLAY && frame==before_frame);
    for(int n=0;n<7;++n) {
        race_p4.source_paused=1; int old_frame=frame;game_update(0,0);
        assert(frame==old_frame+1 && sc64_battlecruiser_source_tick_due()==due);
        assert(!memcmp(&shot,&sc64_bc_shots[0],sizeof shot) && u[a].carry==energy && u[a].work_timer==timer);
    }
    assert(sc64_bc_pause_offset==(old_offset+7u)%5u);
    assert(race_save_game());sc64_bc_pause_offset=0;memset(sc64_bc_shots,0,sizeof(sc64_bc_shots));
    int loaded=race_load_game();
    fprintf(stderr,"loaded=%d marker=%x offset=%u expected=%u due=%d expected_due=%d frame=%d\n",loaded,(unsigned)race_data.state[28],sc64_bc_pause_offset,(old_offset+7u)%5u,sc64_battlecruiser_source_tick_due(),due,frame);
    assert(loaded && !memcmp(&shot,&sc64_bc_shots[0],sizeof shot) &&
           sc64_bc_pause_offset==(old_offset+7u)%5u && sc64_battlecruiser_source_tick_due()==due);
    int saved_frame=frame,saved_energy=u[a].carry; unsigned saved_offset=sc64_bc_pause_offset;
    RaceSave corrupt=race_data; corrupt.state[28]=0x42440005;
    save_write(&corrupt,sizeof(corrupt),SAVE_VERSION);
    assert(!race_load_game() && frame==saved_frame && u[a].carry==saved_energy &&
           sc64_bc_pause_offset==saved_offset && !memcmp(&shot,&sc64_bc_shots[0],sizeof shot));
    puts("private P4 pause freezes BC clock and rejects invalid phase atomically");
}
