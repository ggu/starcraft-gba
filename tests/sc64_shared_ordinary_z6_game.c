/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
/* Defensive test for the existing hallucination no-damage rule. */
#define main original_z6_hallucination_test
#include "sc64_z6_hallucination.c"
#undef main
#ifndef SC64_SHARED_ORDINARY_STAGE
#error "fixture needs the ordinary stage"
#endif
int main(void)
{
    init_test();
    int actor=spawn_unit(SIDE,sc64_race_unit_from_raw(0),160,160);
    int target=spawn_unit(ENEMY,U_OVERLORD,220,160);
    assert(actor>=0 && target>=0);
    /* Deliberately synthetic flagged Marine: no staged source family may
     * bypass the existing hallucination guard even on malformed live data. */
    u[actor].raw_type=0;u[actor].flags|=SC64_Z6_HALLUCINATION_FLAG;
    u[actor].order=ORDER_ATTACK_TARGET;u[actor].target=target;
    int hp=u[target].hp;assert(!race_z6_hallucination_can_deal_damage(actor));
    tick_combat(&u[actor],target);
    assert(!SC64_ORDINARY_PENDING(actor).active && !sc64_shared_projectiles.active);
    assert(u[target].hp==hp);
    puts("Z6 hallucination cannot reserve or damage through ordinary stage");return 0;
}
