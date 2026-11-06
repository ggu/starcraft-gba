/* SPDX-License-Identifier: GPL-3.0-or-later
 * Verify Cannon power with both cached and uncached Observer profiles. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
int main(void)
{
    memset(b,0,sizeof(b)); race_mode=0; race_selected_mission=SC64_P7_GLOBAL_ID;
    int p=spawn_build(ENEMY,B_PYLON,240,160,0);
    int c=spawn_build(ENEMY,B_PHOTON_CANNON,240,160,0);
    assert(p>=0 && c>=0);
    b[p].raw_type=156; b[p].raw_owner=4;
    b[c].raw_type=162; b[c].raw_owner=4;
    b[c].y=b[p].y+79; assert(sc64_p7_observer_pylon_powers(c,4,ENEMY));
    b[c].y=b[p].y+80; assert(!sc64_p7_observer_pylon_powers(c,4,ENEMY));
    b[c].x=b[p].x+96; b[c].y=b[p].y+64;
    assert(!sc64_p7_observer_pylon_powers(c,4,ENEMY));
    b[c].x=b[p].x; b[c].y=b[p].y;
    b[p].raw_owner=0; assert(!sc64_p7_observer_pylon_powers(c,4,ENEMY));
    b[p].raw_owner=4; b[p].under_construction=1;
    assert(!sc64_p7_observer_pylon_powers(c,4,ENEMY));
    puts("PASS Cannon psi source owner and boundary");
    return 0;
}
