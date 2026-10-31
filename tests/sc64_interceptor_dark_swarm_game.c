/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic launched child; actual pad-created field and projectile adapter. */
#define main previous_defiler_test
#include "sc64_defiler_executor_game.c"
#undef main
#ifndef SC64_SHARED_CARRIER_EXPERIMENT
#error "requires experimental shared Carrier"
#endif
#ifndef SC64_SHARED_DARK_SWARM_PROBE
#error "requires staged Dark Swarm query"
#endif
static Sc64ProjectileRow only_row(void)
{
    u8 id=sc64_pool_active_head(&race_p7_projectiles);Sc64ProjectileRow row;
    assert(id!=SC64_END && sc64_pool_get_row(&race_p7_projectiles,id,&row));
    assert(row.next_link==SC64_END);return row;
}
static void launch_case(int shooter,int target,int initially_inside,int expect_miss)
{
    u[target].x=(s16)(initially_inside?400:500);u[target].y=320;
    u[target].hp=u[target].maxhp=1000;
    assert(sc64_world_dark_swarm_affects_target(target)==initially_inside);
    assert(race_p7_projectile_fire(shooter,target,64,u[shooter].x*512,u[shooter].y*512));
    Sc64ProjectileRow row=only_row();assert((row.hit_flags_low2&1u)==(unsigned)expect_miss);
    /* Crossing the field edge after launch must not change the stored miss. */
    u[target].x=(s16)(initially_inside?500:400);
    for(int n=0;n<700 && sc64_pool_active_head(&race_p7_projectiles)!=SC64_END;n++)race_p7_projectile_source_tick();
    assert(!race_p7_projectile_fault && sc64_pool_active_head(&race_p7_projectiles)==SC64_END);
    if(expect_miss)assert(u[target].hp==1000);else assert(u[target].hp<1000);
}
int main(void)
{
    game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    int caster=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(caster>=0);test_phase=0;click_cast(caster,400,320);
    for(int n=0;n<40 && !fields();n++)game_update(0,0);assert(fields()==1);
    int parent=spawn_unit(SIDE,SC64_CARRIER_TYPE,360,300);
    int shooter=spawn_unit(SIDE,SC64_INTERCEPTOR_TYPE,380,300);
    int target=spawn_unit(ENEMY,U_MARINE,400,320);assert(parent>=0 && shooter>=0 && target>=0);
    /* This seeds the parent link and launched-child state; no paid launch claim. */
    u[shooter].build=(s16)parent;
    assert(!SC64_CARRIER_HANGAR_HIDDEN(&u[shooter]));
    launch_case(shooter,target,1,1);launch_case(shooter,target,0,0);
    u[target].raw_type=255;u[target].x=400;u[target].y=320;
    assert(sc64_world_dark_swarm_affects_target(target)==-1);
    unsigned active=sc64_shared_projectiles.active;
    assert(!race_p7_projectile_fire(shooter,target,64,u[shooter].x*512,u[shooter].y*512));
    assert(sc64_shared_projectiles.active==active && sc64_pool_active_head(&race_p7_projectiles)==SC64_END);
    u[target].raw_type=sc64_race_raw_unit(u[target].type);
    u[shooter].build=-1;launch_case(shooter,target,1,1);
    puts("Interceptor field decision persists from launch to impact");return 0;
}
