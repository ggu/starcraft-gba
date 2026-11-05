/* SPDX-License-Identifier: GPL-3.0-or-later
 * Same-update mission message + outcome regression, using the actual engine.
 * Default asserts the desired deferred publication. SC64_OBSERVE_BUG records
 * the pre-fix failure without declaring it correct. Each case runs in a fresh
 * process because the HOST save journal is intentionally retained in memory. */
#define main sc64_original_text_box_fixture_main
#include "sc64_text_box_game.c"
#undef main

static int journal_present(void)
{
    static u8 bytes[16368]; u32 len=0;
#if defined(SC64_SHARED_TRANSPORT) && defined(SC64_GUARDIAN_COCOON_STAGE)
    return save_read_latest(bytes,sizeof(bytes),SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,&len);
#elif defined(SC64_SHARED_TRANSPORT)
    return save_read_latest(bytes,sizeof(bytes),SC64_TRANSPORT_SAVE_VERSION,&len);
#else
    return save_read_latest(bytes,sizeof(bytes),SAVE_VERSION,&len);
#endif
}

static void seed_t1_defeat(void)
{
    launch_t1();
    for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].type==U_RAYNOR) u[i].alive=0;
}

static void seed_p2_victory(void)
{
    game_init();held_keys=0;race_mode=0;race_selected_mission=SC64_RACE_MISSION_P2;
    race_human=R_PROTOSS;init_state();
    assert(screen==SCREEN_PLAY && !race_mode && race_campaign.mission==SC64_RACE_MISSION_P2);
    int survivor=-1;
    for(int i=0;i<MAX_UNITS;i++) {
        if(u[i].alive && u[i].owner==ENEMY && u[i].raw_type==151)u[i].alive=0;
        else if(survivor<0 && u[i].alive && u[i].owner==SIDE)survivor=i;
    }
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(b[i].alive && b[i].owner==ENEMY && b[i].raw_type==151)b[i].alive=0;
    assert(survivor>=0);race_campaign.rescue_unit=(unsigned short)survivor;
    race_campaign.objective_flags|=SC64_RACE_OBJECTIVE_FENIX_READY;
}

int main(int argc,char **argv)
{
    int victory, reset_case, failed_load, successful_load;
    assert(argc==2);victory=!strcmp(argv[1],"p2-victory");
    reset_case=!strcmp(argv[1],"t1-new-game");
    failed_load=!strcmp(argv[1],"t1-failed-load");
    successful_load=!strcmp(argv[1],"t1-load-success");
    assert(victory || reset_case || failed_load || successful_load || !strcmp(argv[1],"t1-defeat"));
    gba_init();
    if(victory)seed_p2_victory();
    else if(successful_load) {
        launch_t1();assert(save_game());
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].type==U_RAYNOR)u[i].alive=0;
    } else seed_t1_defeat();
    assert(journal_present()==successful_load);int completed_before=race_completed; (void)completed_before;
    unsigned phase_before=race_campaign.phase, objectives_before=race_campaign.objective_flags;
    (void)phase_before; (void)objectives_before;
    input(0);
    printf("case=%s queue=%u screen=%d won=%d lost=%d completed=%lu journal=%d frame=%d\n",
           argv[1],sc64_text_box_count(),screen,won,lost,(unsigned long)race_completed,
           journal_present(),frame);fflush(stdout);
    assert(sc64_text_box_count()>0);
#ifdef SC64_OBSERVE_BUG
    assert(screen==SCREEN_RESULT && (victory?won:lost) && journal_present());
    if(victory)assert(race_completed & (1u<<race_selected_mission));
    return 0;
#else
    assert(screen==SCREEN_PLAY && !won && !lost && race_completed==completed_before &&
           journal_present()==successful_load);
    assert(race_campaign.phase==phase_before && race_campaign.objective_flags==objectives_before);
    u32 frozen=world_digest();int frozen_frame=frame;
    assert(!save_game() && journal_present()==successful_load);
    if(failed_load) {
        assert(!load_game());
        assert(sc64_text_box_count() && sc64_text_box_outcome.result &&
               screen==SCREEN_PLAY && frame==frozen_frame && world_digest()==frozen);
    }
    if(successful_load) {
        assert(load_game());
        assert(!sc64_text_box_count() && !sc64_text_box_outcome.result && !won && !lost);
        input(0);assert(screen!=SCREEN_RESULT && !won && !lost);
        return 0;
    }
    if(reset_case) {
        game_init();
        assert(!sc64_text_box_count() && !sc64_text_box_outcome.result && !journal_present());
        input(0);assert(screen!=SCREEN_RESULT && !won && !lost);
        return 0;
    }
    for(int n=0;n<120;n++) {input(0);assert(frame==frozen_frame && world_digest()==frozen);}
    while(sc64_text_box_count()) {
        input(KEY_A);assert(sc64_text_box_count() && frame==frozen_frame && world_digest()==frozen);
        input(0);
        if(sc64_text_box_count())assert(screen==SCREEN_PLAY && !won && !lost &&
                                        race_completed==completed_before && !journal_present() &&
                                        frame==frozen_frame && world_digest()==frozen);
    }
    assert(frame==frozen_frame && screen==SCREEN_RESULT && (victory?won:lost) && journal_present());
    assert(race_campaign.phase==(victory?SC64_RACE_PHASE_COMPLETE:SC64_RACE_PHASE_FAILED));
    if(victory)assert(race_completed & (1u<<race_selected_mission));
    return 0;
#endif
}
