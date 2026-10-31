/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#ifndef SC64_SHARED_PROJECTILE_STAGE
#error "compile this fixture with the experimental shared projectile gate"
#endif
#ifndef SC64_P7_PROJECTILES_ENABLED
#error "fixture needs the existing Interceptor projectile family"
#endif
int main(void)
{
    u32 handles[99];
    game_init(); race_mode=0; race_selected_mission=SC64_P7_GLOBAL_ID; init_state();
    assert(screen==SCREEN_PLAY && sc64_shared_projectiles.active==0);
    int shooter=spawn_unit(SIDE,U_P7_INTERCEPTOR,64,64);
    int target=spawn_unit(ENEMY,U_MARINE,224,64);
    assert(shooter>=0 && target>=0);
    u[target].raw_type=0;u[target].raw_owner=0;
    for(unsigned i=0;i<99;i++) {
        handles[i]=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_YAMATO,i);
        assert(handles[i]);
    }
    assert(race_p7_projectile_fire(shooter,target,64,64*512,64*512));
    assert(sc64_shared_projectiles.active==100);
    assert(!race_p7_projectile_fire(shooter,target,64,64*512,64*512));
    assert(sc64_pool_validate(&race_p7_projectiles));
    assert(!race_save_game() && !race_load_game());
    assert(!sc64_shared_projectile_release(&sc64_shared_projectiles,handles[0],SC64_SHARED_PROJECTILE_SWARM,0));
    assert(sc64_shared_projectile_release(&sc64_shared_projectiles,handles[0],SC64_SHARED_PROJECTILE_YAMATO,0));
    assert(race_p7_projectile_fire(shooter,target,64,64*512,64*512));
    assert(sc64_shared_projectiles.active==100);
    assert(!sc64_shared_projectile_release(&sc64_shared_projectiles,handles[0],SC64_SHARED_PROJECTILE_YAMATO,0));
    /* Source actor death invalidates references, not in-flight reservations. */
    u[shooter].alive=0; race_p7_projectile_clear_references(shooter);
    assert(sc64_shared_projectiles.active==100);
    for(unsigned i=1;i<99;i++)assert(sc64_shared_projectile_release(&sc64_shared_projectiles,handles[i],SC64_SHARED_PROJECTILE_YAMATO,i));
    for(int i=0;i<400 && sc64_shared_projectiles.active;i++)race_p7_projectile_source_tick();
    assert(!race_p7_projectile_fault && sc64_shared_projectiles.active==0);
    u32 old=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_SWARM,7);
    assert(old);sc64_shared_projectile_reset(&sc64_shared_projectiles);
    u32 replacement=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_SWARM,7);
    assert(replacement && replacement!=old);
    assert(!sc64_shared_projectile_release(&sc64_shared_projectiles,old,SC64_SHARED_PROJECTILE_SWARM,7));
    assert(sc64_shared_projectile_release(&sc64_shared_projectiles,replacement,SC64_SHARED_PROJECTILE_SWARM,7));
    /* A validated old P7 packet can reconstruct typed reservations when the
     * stage arena is otherwise empty. Saving a live staged row is still refused. */
    shooter=spawn_unit(SIDE,U_P7_INTERCEPTOR,64,64); assert(shooter>=0);
    assert(race_p7_projectile_fire(shooter,target,64,64*512,64*512));
    u8 packet[SC64_CODEC_BYTES]; assert(race_p7_projectile_encode(packet));
    race_p7_projectile_reset(); assert(sc64_shared_projectiles.active==0);
    assert(race_p7_projectile_restore(packet)); assert(sc64_shared_projectiles.active==1);
    race_p7_projectile_reset(); assert(sc64_shared_projectiles.active==0);
    puts("typed contention and real Interceptor lifecycle passed");return 0;
}
