/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST journals only. Constructed states are never copied into guest SRAM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef SC64_EGG_AUTH_GAME_SOURCE
#define SC64_EGG_AUTH_GAME_SOURCE "../starcraft/game.c"
#endif
#include SC64_EGG_AUTH_GAME_SOURCE
extern void sc64_test_sram_snapshot(void *);
extern int sc64_test_sram_matches(const void *);
static unsigned char journal[32768];
static void start_custom(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();
    assert(race_mode && screen==SCREEN_PLAY);
}
static int add_legacy_overlord(void)
{
    int i=spawn_unit(SIDE,U_OVERLORD,128,128);assert(i>=0);
    u[i].raw_type=36;return i; /* historical raw 36 did not always mean Egg */
}
static int add_unbacked_egg(int raw_owner)
{
    int i=spawn_unit(SIDE,U_Z7_EGG,140,100);assert(i>=0);
    u[i].raw_type=36;u[i].raw_owner=raw_owner;u[i].target=U_DRONE;
    u[i].work_timer=1;return i;
}
static void invalid_only(void)
{
    static Unit before[MAX_UNITS];static Building before_b[MAX_BUILDINGS];
    memcpy(before,u,sizeof(u));memcpy(before_b,b,sizeof(b));
    const int ore=minerals,vespene=gas,old_frame=frame;const u32 old_seed=seed;
    sc64_test_sram_snapshot(journal);
#ifdef SC64_BASELINE_EXPECT_ACCEPT
    assert(race_load_game());
    puts("baseline accepted a non-Z7 Egg journal");
#else
    assert(!race_load_game());
    assert(!memcmp(before,u,sizeof(u)) && !memcmp(before_b,b,sizeof(b)));
    assert(ore==minerals && vespene==gas && old_frame==frame && old_seed==seed);
    assert(sc64_test_sram_matches(journal));
    puts("non-Z7 Egg journal rejected without mutation");
#endif
}
int main(int argc,char **argv)
{
    assert(argc==2);
#ifdef SC64_Z9_RUNTIME
    if (!strcmp(argv[1],"z9")) {
        game_init();race_mode=0;race_human=R_ZERG;race_selected_mission=SC64_Z9_GLOBAL_ID;
        init_state();assert(!race_mode && screen==SCREEN_PLAY && race_zl_active());
        (void)add_unbacked_egg(3);assert(race_save_game());invalid_only();return 0;
    }
#endif
    start_custom();int old=add_legacy_overlord();Unit saved=u[old];
    if (!strcmp(argv[1],"legacy")) {
        assert(race_save_game());u[old].raw_type=42;
        assert(race_load_game() && !memcmp(&u[old],&saved,sizeof(saved)));
        puts("ordinary custom legacy raw36 Overlord restored");return 0;
    }
    if (!strcmp(argv[1],"fallback")) assert(race_save_game());
    else assert(!strcmp(argv[1],"reject"));
    (void)add_unbacked_egg(-1);assert(race_save_game());
    if (!strcmp(argv[1],"fallback")) {
        sc64_test_sram_snapshot(journal);assert(race_load_game());
        assert(sc64_test_sram_matches(journal) && !memcmp(&u[old],&saved,sizeof(saved)));
        puts("invalid newer Egg falls back to ordinary custom journal");
    } else invalid_only();
    return 0;
}
