/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Isolated engine movement checks, not campaign qualification. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void travel(int type,int raw,int diagonal,int low,int high)
{
    game_init(); race_mode=1; race_human=R_TER; race_bots=1;
    race_skirmish_init();
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); memset(m,0,sizeof(m));
    memset(path_walkable,0xff,sizeof(path_walkable)); path_grid_valid=1;
    sc64_unit_buckets_end(); sc64_target_index_active=0;
    int slot=spawn_unit(SIDE,type,128,128);
    assert(slot>=0); u[slot].raw_type=raw;
    for (frame=0;frame<128;++frame)
        move_toward(&u[slot],WORLD_W-32,diagonal?WORLD_H-32:128);
    int distance=u[slot].x-128;
    assert(distance>=low && distance<=high);
    assert(u[slot].y==(diagonal?u[slot].x:128));
}

int main(void)
{
    gba_init();
    travel(U_MARINE,0,0,204,205); /* 4 * 24/30 / 2 = 1.6 px/tick. */
    travel(U_MARINE,0,1,144,145);
    travel(U_FIREBAT,32,0,204,205); /* No invented Firebat speed boost. */
    travel(U_ZERGLING,37,0,280,282);
    travel(U_OVERLORD,42,0,42,43);
    travel(U_SIEGE_TANK,30,0,0,0);
    travel(U_RAYNOR,19,0,327,328); /* Objective alias still uses Vulture speed. */
    puts("Engine uses source nominal speeds and normalizes diagonal travel");
    return 0;
}
