/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real projectile families; actor setup is synthetic. */
#define main previous_yamato_interceptor_test
#include "sc64_shared_yamato_interceptor_game.c"
#undef main
#ifndef SC64_SCARAB_WEAPON_PROBE
#error "fixture needs Scarab projectile probe"
#endif
int main(void)
{
    int shooter,enemy,bc,victim;
    setup(&shooter,&enemy,&bc,&victim);
    u[enemy].hp=u[enemy].maxhp=500;
    for(int i=0;i<99;i++)assert(race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    assert(sc64_shared_projectiles.active==99);
    Unit scarab={0};scarab.x=224;scarab.y=64;scarab.target=enemy;scarab.owner=SIDE;
    assert(race_scarab_probe_bullet_create(&scarab));
    assert(sc64_shared_projectiles.active==100 && race_impact_count==1);
    assert(!race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    assert(!race_scarab_probe_bullet_create(&scarab));
    race_impact_rows[0].steps=(u8)((race_impact_rows[0].steps&15u)|16u);
    frame=0;race_impact_tick();
    assert(!race_impact_count && sc64_shared_projectiles.active==99);
    assert(race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    assert(sc64_shared_projectiles.active==100);
    race_p7_projectile_reset();assert(!sc64_shared_projectiles.active);
    puts("real Scarab and Interceptor contention and retirement passed");return 0;
}
