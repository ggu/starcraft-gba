/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exercises the real custom-game runtime and ordinary save/load path. */
#define main sc64_resume_baseline_main
#include "sc64_skirmish_resume.c"
#undef main

#ifndef SC64_SHARED_TRANSPORT
#error "compile with the staged shared transport capability"
#endif

static void check_jobs(int map)
{
    static const int owners[4] = { SIDE, ENEMY, RESCUABLE, 4 };
    int lair[4];
    launch(map, R_ZERG, 3, R_ZERG);
    for (int i = 0; i < 4; ++i) {
        lair[i] = spawn_build(owners[i], B_LAIR,
                              race_base_x[i] + 60, race_base_y[i] + 60, 0);
        assert(lair[i] >= 0);
        if (i == 0) { minerals = 1000; gas = 1000; }
        else { race_minerals[owners[i]] = 1000; race_gas[owners[i]] = 1000; }
        assert(race_sk_transport_start_owner(owners[i], lair[i]));
    }
    for (int i = 0; i < 60; ++i) race_sk_transport_research_tick();
    assert(race_save_game());
    memset(&race_skirmish_transport_tech, 0, sizeof(race_skirmish_transport_tech));
    assert(race_load_game());
    for (int i = 0; i < 4; ++i) {
        assert(race_skirmish_transport_tech.owner[i].remaining == 2940);
        assert(race_skirmish_transport_tech.owner[i].producer_plus_one == lair[i] + 1);
    }
    for (int i = 0; i < 2940; ++i) race_sk_transport_research_tick();
    for (int i = 0; i < 4; ++i) assert(race_skirmish_transport_tech.owner[i].researched);
    assert(race_save_game());
    assert(race_load_game());
    for (int i = 0; i < 4; ++i) assert(race_skirmish_transport_tech.owner[i].researched);
}

int main(void)
{
    gba_init();
    for (int map = 0; map <= SC64_SKIRMISH_IMPORTED_COUNT; ++map) check_jobs(map);
    puts("shared custom transport research/save: ok");
    return 0;
}
