/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST journal test; never use these records as guest saves. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
extern void guardian_test_reset(void);
extern void guardian_test_snapshot(void *);

static void start_custom(void)
{
    guardian_test_reset();game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();
    assert(screen==SCREEN_PLAY && race_mode);
}
static int add_legacy_egg(void)
{
    int slot=spawn_unit(SIDE,U_Z7_EGG,140,100);assert(slot>=0);
    u[slot].raw_type=36;u[slot].raw_owner=-1;
    u[slot].target=U_DRONE;u[slot].work_timer=1;
    return slot;
}
int main(void)
{
    static u8 before[32768],after[32768];
    static Unit old_units[MAX_UNITS];static Building old_buildings[MAX_BUILDINGS];
    start_custom();int old=spawn_unit(SIDE,U_OVERLORD,100,100);assert(old>=0);
    u[old].raw_type=36;Unit overlord=u[old];
    assert(race_save_game() && race_load_game());
    assert(!memcmp(&overlord,&u[old],sizeof(overlord)));

    start_custom();add_legacy_egg();assert(race_save_game());
    memcpy(old_units,u,sizeof(u));memcpy(old_buildings,b,sizeof(b));
    int old_frame=frame,old_ore=minerals,old_gas=gas;
    guardian_test_snapshot(before);assert(!race_load_game());
    guardian_test_snapshot(after);assert(!memcmp(before,after,sizeof(before)));
    assert(!memcmp(old_units,u,sizeof(u)) && !memcmp(old_buildings,b,sizeof(b)));
    assert(frame==old_frame && minerals==old_ore && gas==old_gas);

    start_custom();old=spawn_unit(SIDE,U_OVERLORD,100,100);assert(old>=0);
    u[old].raw_type=36;overlord=u[old];assert(race_save_game());
    int egg=add_legacy_egg();assert(race_save_game());u[old].hp=1;
    guardian_test_snapshot(before);assert(race_load_game());
    guardian_test_snapshot(after);assert(!memcmp(before,after,sizeof(before)));
    assert(!memcmp(&overlord,&u[old],sizeof(overlord)) && !u[egg].alive);
    puts("Guardian-only journal preserves ordinary actors and rejects custom legacy Eggs");
    return 0;
}
