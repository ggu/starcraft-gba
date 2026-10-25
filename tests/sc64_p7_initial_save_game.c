/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual-engine regression: compare immediate save/resume with cold title load. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

static void tap(u16 key) { game_update(key,key); game_update(0,0); }

int main(void)
{
    static Unit resumed_units[MAX_UNITS];
    static Building resumed_buildings[MAX_BUILDINGS];
    static Sc64P7PersistentState resumed_p7;
    static int resumed_frame;
    static u32 resumed_seed;

    game_init();
    tap(KEY_A); /* HOME -> Protoss missions */
    tap(KEY_RIGHT); tap(KEY_RIGHT); tap(KEY_RIGHT); /* P7 page */
    tap(KEY_A); /* briefing */
    game_update(KEY_A,KEY_A); /* launch without a PLAY tick */
    assert(screen==SCREEN_PLAY && frame==0);
    assert(!sc64_p7_arbiter_cache_initialized);

    tap(KEY_START);
    assert(screen==SCREEN_PAUSE);
    tap(KEY_B); /* save before the first ordinary play tick */
    assert(screen==SCREEN_PAUSE);
    assert(frame==0 && save_read(&race_data,sizeof(race_data),SAVE_VERSION));
    tap(KEY_A); /* resume; key release performs the first PLAY tick */
    assert(screen==SCREEN_PLAY && sc64_p7_arbiter_cache_initialized);
    memcpy(resumed_units,u,sizeof(u));
    memcpy(resumed_buildings,b,sizeof(b));
    resumed_p7=race_p7; resumed_frame=frame; resumed_seed=seed;

    /* Clear mission/cache process state while preserving host SRAM. */
    game_init();
    memset(&race_p7,0,sizeof(race_p7));
    memset(&race_p7_world,0,sizeof(race_p7_world));
    memset(&race_p7_ops,0,sizeof(race_p7_ops));
    memset(&race_data,0,sizeof(race_data));
    race_p7_map=0;
    sc64_p7_arbiter_cache_initialized=0;
    sc64_p7_arbiter_cache_due=0;
    assert(screen==SCREEN_TITLE);
    game_update(KEY_L,KEY_L); /* ordinary title-menu cold load */
    assert(screen==SCREEN_PLAY && frame==0);
    game_update(0,0); /* same first ordinary PLAY tick */
    assert(screen==SCREEN_PLAY && sc64_p7_arbiter_cache_initialized);
    assert(!memcmp(u,resumed_units,sizeof(u)));
    assert(!memcmp(b,resumed_buildings,sizeof(b)));
    assert(!memcmp(&race_p7,&resumed_p7,sizeof(race_p7)));
    assert(frame==resumed_frame && seed==resumed_seed);
    printf("P7 immediate-save first-tick equivalence clock=%u phase=%u frame=%d\n",
           (unsigned)race_p7.reserved[1] |
               ((unsigned)race_p7.reserved[2]<<8),
           (unsigned)race_p7.reserved[0],frame);
    return 0;
}
