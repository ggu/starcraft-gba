/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#if !defined(SC64_DIAGNOSTIC_COMBAT_LEAVES) || !defined(SC64_CONTACT_CACHE)
#error "This fixture requires the combat leaf and contact diagnostics"
#endif
static void exercise(void)
{
    int nx,ny;
    (void)direct_path_clear(320,320,336,320);
    (void)unit_step_clear(&u[0],321,320);
    (void)path_next_tile(320,320,400,320,&nx,&ny);
    u[0].x=320;u[0].y=320;u[0].path_x=400;u[0].path_y=320;
    u[0].path_timer=24;u[0].path_retry=0;sc64_unit_buckets_begin();
    move_toward(&u[0],400,320);sc64_unit_buckets_end();
}
int main(void)
{
    gba_init();game_init();race_mode=1;race_human=R_PROTOSS;race_bots=0;race_skirmish_map=0;race_skirmish_init();
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));invalidate_path_grid();
    assert(spawn_unit(SIDE,U_MARINE,320,320)==0);
    assert(spawn_unit(SIDE,U_MARINE,322,320)==1);
    sc64_combat_leaf_active=0;exercise();
    assert(!sc64_combat_direct_count && !sc64_combat_step_count && !sc64_combat_contact_count && !sc64_combat_route_count);
    sc64_combat_leaf_active=1;exercise();sc64_combat_leaf_active=0;
    assert(sc64_combat_direct_count>=1 && sc64_combat_step_count>=2 && sc64_combat_contact_count>=1 && sc64_combat_route_count>=1);
    puts("combat leaf interval: outside zero; direct, step, contact and route observed inside");return 0;
}
