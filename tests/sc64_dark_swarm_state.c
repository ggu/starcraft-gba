/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_dark_swarm_state.h"
int main(void)
{
    Sc64DarkSwarmEffect e={0};
    assert(!sc64_dark_swarm_tick(&e));
    sc64_dark_swarm_created(&e,123,456);
    assert(e.x==123 && e.y==456 && e.remaining==900);
    for (int i=0;i<899;++i) assert(!sc64_dark_swarm_tick(&e));
    assert(e.remaining==1 && sc64_dark_swarm_tick(&e));
    assert(!e.remaining && !sc64_dark_swarm_tick(&e));
    assert(sc64_dark_swarm_remaining_valid(0));
    assert(sc64_dark_swarm_remaining_valid(900));
    assert(!sc64_dark_swarm_remaining_valid(901));
    puts("Dark Swarm effect lifetime passed");
    return 0;
}
