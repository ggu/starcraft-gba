/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Verify actual update ordering when a Scarab tail and unit release coincide. */
#define main previous_yamato_interceptor_test
#include "sc64_shared_yamato_interceptor_game.c"
#undef main
#if !defined(SC64_SHARED_ORDINARY_STAGE) || !defined(SC64_SCARAB_WEAPON_PROBE)
#error "fixture requires both disabled projectile stages"
#endif
int main(void)
{
    int shooter,enemy,bc,victim;setup(&shooter,&enemy,&bc,&victim);
    u[enemy].hp=u[enemy].maxhp=1000;
    for(int i=0;i<99;i++)assert(race_p7_projectile_fire(shooter,enemy,64,64*512,64*512));
    Unit scarab={0};scarab.x=224;scarab.y=64;scarab.target=enemy;scarab.owner=SIDE;
    assert(race_scarab_probe_bullet_create(&scarab));
    race_impact_rows[0].steps=(u8)((race_impact_rows[0].steps&15u)|16u);
    race_mode=1;frame=1;race_impact_tick();
    assert(!race_impact_count && race_impact_scarab_tail_count==1 && sc64_shared_projectiles.active==100);
    int actor=spawn_unit(SIDE,sc64_race_unit_from_raw(0),160,160);
    int target=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),220,160);
    assert(actor>=0 && target>=0);u[target].hp=u[target].maxhp=1000;
    u[actor].raw_type=0;u[actor].order=ORDER_ATTACK_TARGET;u[actor].target=target;
    tick_combat(&u[actor],target);assert(SC64_ORDINARY_PENDING(actor).active);
    while(SC64_ORDINARY_PENDING(actor).wait[0]>1)race_shared_ordinary_tick();
    assert(sc64_shared_projectiles.active==100 && race_impact_scarab_tail_count==1);
    int hp=u[target].hp;screen=SCREEN_PLAY;game_update(0,0);
    assert(!SC64_ORDINARY_PENDING(actor).active && u[target].hp==hp);
    assert(!race_impact_scarab_tail_count && sc64_shared_projectiles.active==99);
    race_p7_projectile_reset();assert(!sc64_shared_projectiles.active);
    puts("unit release precedes same-update Scarab retirement");return 0;
}
