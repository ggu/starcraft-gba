/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "race_rules.h"
#include "sc64_game_actor_facts.h"
int main(void)
{
    int l,u,r,d;
    assert(sc64_fact_sight_radius_runtime(U_SHARED_DEFILER,0)==160);
    assert(sc64_fact_sight_radius_runtime(U_SHARED_UNCLEAN_ONE,0)==160);
    assert(sc64_game_unit_collision_radius(U_SHARED_UNCLEAN_ONE)==7);
    assert(sc64_fact_p7_rect_extents(46,0,&l,&u,&r,&d));
    assert(l==13 && u==12 && r==13 && d==12);
    assert(sc64_fact_p7_rect_extents(52,0,&l,&u,&r,&d));
    assert(l==14 && u==14 && r==14 && d==14);
    return 0;
}
