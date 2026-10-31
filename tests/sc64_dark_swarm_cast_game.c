/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_dark_swarm_cast_probe.h"
int main(void)
{
    unsigned energy=100u*256u; Sc64DarkSwarmEffect effect={0};
    game_init();
    int caster=spawn_unit(SIDE,U_SK_DEFILER,320,320);
    assert(caster>=0 && sc64_race_raw_unit(u[caster].type)==46);
    assert(!sc64_dark_swarm_probe_begin(&u[caster],1,&energy));
    assert(energy==100u*256u);
    assert(sc64_dark_swarm_probe_begin(&u[caster],0,&energy) && energy==0);
    assert(!sc64_dark_swarm_probe_begin(&u[caster],0,&energy));
    assert(!effect.remaining); /* animation start is not projectile hit */
    assert(!sc64_dark_swarm_probe_place(0,0,0,2048,2048));
    assert(energy==0); /* allocation failure cannot refund energy */
    assert(sc64_dark_swarm_probe_place(&effect,0,2047,2048,2048));
    assert(effect.x==80 && effect.y==1936 && effect.remaining==900);
    /* Feed the actual shared source-cadence predicate, not every game update. */
    int source_ticks=0,expired=0;
    race_mode=1;
    for (frame=0;frame<1125;frame++) if(sc64_battlecruiser_source_tick_due()) {
        ++source_ticks; expired+=sc64_dark_swarm_source_tick(&effect);
        if(source_ticks<900) assert(!expired && effect.remaining==900-source_ticks);
    }
    assert(source_ticks==900 && expired==1 && !effect.remaining);
    puts("Dark Swarm actual-actor cast/placement probe passed");
    return 0;
}
