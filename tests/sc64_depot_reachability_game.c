/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual selector and cargo return through the imported race engine. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
int main(void)
{
    gba_init(); game_init(); memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); memset(m,0,sizeof(m));
    race_mode=1; race_human=R_PROTOSS; race_bots=0;
    memset(race_player,0xff,sizeof(race_player)); race_player[SIDE]=R_PROTOSS;
    minerals=gas=0; supply_used=0; supply_cap=200; frame=1; won=lost=0;
    b[0].alive=b[1].alive=1; b[0].owner=b[1].owner=SIDE;
    b[0].type=b[1].type=B_NEXUS; b[0].x=320; b[0].y=320; b[1].x=720; b[1].y=320;
    b[0].hp=b[0].maxhp=b[1].hp=b[1].maxhp=1000;
    m[0].x=800; m[0].y=320; m[0].amount=500; invalidate_path_grid();
    int w=spawn_unit(SIDE,U_PROBE,760,320); assert(w>=0);
    u[w].order=ORDER_MINE; u[w].target=0; u[w].carry=6; u[w].build=-1;
    update_orders(); assert(u[w].build==1 && u[w].carry==6 && minerals==0);
    for (int tick=0;tick<64 && u[w].carry;++tick) update_orders();
    assert(u[w].carry==0 && minerals==6 && u[w].build==-1);
    puts("actual worker selects nearest depot and credits carried minerals once");
    return 0;
}
