/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Mixed typed slots and family resets use the actual game storage. */
#define main previous_defiler_test
#include "sc64_defiler_executor_game.c"
#undef main
#ifndef SC64_SHARED_ORDINARY_STAGE
#error "mixed payload fixture needs ordinary stage"
#endif
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    u32 swarm=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_SWARM,0);
    u32 ordinary=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_IMPACT,7);
    assert(swarm && ordinary);
    unsigned s=(swarm&255u)-1u,o=(ordinary&255u)-1u;
    sc64_shared_payloads[s].swarm=(Sc64DefilerProjectile){swarm,23,42,3};
    sc64_shared_payloads[o].ordinary=(Sc64OrdinaryBullet){.handle=ordinary,.target=4,.reservation_owner=7};
    assert(sc64_defiler_projectile_at(s) && !sc64_ordinary_bullet_at(s));
    assert(sc64_ordinary_bullet_at(o) && !sc64_defiler_projectile_at(o));
    race_shared_ordinary_forget_target(4);assert(sc64_ordinary_bullet_at(o)->target==-1);
    race_shared_ordinary_reset();
    assert(!sc64_ordinary_bullet_at(o) && sc64_defiler_projectile_at(s));
    assert(sc64_defiler_projectile_at(s)->x==23 && sc64_shared_projectiles.active==1);
    assert(sc64_shared_projectile_release(&sc64_shared_projectiles,swarm,SC64_SHARED_PROJECTILE_SWARM,0));
    u32 replacement=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_IMPACT,7);
    assert((replacement&255u)==s+1 && replacement!=swarm);
    /* An old swarm payload cannot masquerade as the newly reserved ordinary row. */
    assert(!sc64_defiler_projectile_at(s) && !sc64_ordinary_bullet_at(s));
    sc64_shared_payloads[s].ordinary=(Sc64OrdinaryBullet){.handle=replacement,.target=8,.reservation_owner=7};
    assert(sc64_ordinary_bullet_at(s));
    sc64_shared_payloads[s].ordinary.handle=swarm;
    assert(!sc64_ordinary_bullet_at(s)); /* old generation */
    sc64_shared_payloads[s].ordinary.handle=replacement;
    race_shared_ordinary_reset();assert(!sc64_shared_projectiles.active);
    swarm=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_SWARM,0);
    assert(swarm);s=(swarm&255u)-1u;
    sc64_shared_payloads[s].swarm=(Sc64DefilerProjectile){swarm,23,42,3};
    game_init();assert(!sc64_shared_projectiles.active && !sc64_defiler_projectile_at(s));
    puts("mixed typed payload reuse, generation and reset passed");return 0;
}
