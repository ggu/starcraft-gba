/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST-only legacy record: no guest SRAM is seeded. */
#define main sc64_z9_existing_fixture_main
#include "sc64_z9_game.c"
#undef main
#ifdef SC64_SHARED_EGG_HOST_STAGE
#error "This baseline must use the unchanged legacy journal"
#endif
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_ZERG;init_state();
    assert(screen==SCREEN_PLAY && race_mode);
    int slot=spawn_unit(SIDE,U_OVERLORD,100,100);assert(slot>=0);
    u[slot].raw_type=36;u[slot].raw_owner=-1;
    fixture_clear_candidate_records();assert(race_save_game());
    u[slot].raw_type=-1;assert(race_load_game());
    assert(u[slot].alive && u[slot].type==U_OVERLORD && u[slot].raw_type==36);
    puts("legacy custom ordinary/raw36 journal roundtrip passed");return 0;
}
