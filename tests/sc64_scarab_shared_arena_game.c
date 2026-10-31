/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real Scarab bullet with other arena occupants; synthetic host scenario. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
#ifndef SC64_SHARED_PROJECTILE_STAGE
#error "requires shared projectile stage"
#endif
static int launch_paid_reaver(int *target)
{
    minerals=1000;
    int parent=spawn_unit(SIDE,U_REAVER,128,100);
    *target=spawn_unit(ENEMY,U_MARINE,180,100);
    assert(parent>=0 && *target>=0);
    u[*target].hp=u[*target].maxhp=200;
    assert(race_scarab_parent_init(parent) && race_scarab_enqueue(parent,0));
    assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent p;Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(parent,0,&p,&counts));
    for(int i=0;i<105;i++)assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_COMPLETED);
    u[parent].selected=1;
    assert(issue_order(ORDER_ATTACK_MOVE,u[*target].x,u[*target].y));
    return p.active_slot;
}
int main(void)
{
    setup();
    int target=spawn_unit(ENEMY,U_MARINE,160,160);
    assert(target>=0);
    u[target].hp=u[target].maxhp=200;
    u32 other[99];
    for(unsigned i=0;i<99;i++) {
        other[i]=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_SWARM,i);
        assert(other[i]);
    }
    Unit q={0};q.x=160;q.y=160;q.target=target;q.owner=SIDE;
    assert(race_scarab_probe_bullet_create(&q));
    assert(sc64_shared_projectiles.active==100 && race_impact_count==1);
    assert(!race_scarab_probe_bullet_create(&q));
    assert(sc64_shared_projectiles.active==100 && race_impact_count==1);
    /* A lost explicit target does not delete the position splash bullet. */
    race_impact_clear_target(target);
    assert(sc64_shared_projectiles.active==100 && race_impact_count==1);
    race_impact_rows[0].steps=(u8)((race_impact_rows[0].steps&15u)|16u);
    frame=0;screen=SCREEN_PLAY;game_update(0,0);
    assert(race_impact_count==0 && sc64_shared_projectiles.active==100 && race_impact_scarab_tail_count==1);
    assert(!race_scarab_probe_bullet_create(&q));
    for(int n=0;n<5 && race_impact_scarab_tail_count;n++) {++frame;race_impact_tick();}
    assert(!race_impact_scarab_tail_count && sc64_shared_projectiles.active==99);
    for(unsigned i=0;i<99;i++)assert(sc64_shared_projectile_release(&sc64_shared_projectiles,other[i],SC64_SHARED_PROJECTILE_SWARM,i));
    assert(sc64_shared_projectiles.active==0);
    assert(race_scarab_probe_bullet_create(&q));
    assert(sc64_shared_projectiles.active==1);
    clear_tracers();
    assert(race_impact_count==0 && sc64_shared_projectiles.active==0);
    /* An ordinary row removed ahead of two Scarab rows must not detach
     * either reservation from its packet when the shared queue compacts. */
    setup();
    int a=spawn_unit(ENEMY,U_MARINE,160,160), b=spawn_unit(ENEMY,U_MARINE,180,160);
    assert(a>=0 && b>=0);
    race_impact_launch(a,5,1,SIDE,SC64_STOCK_DMG_NORMAL,U_MARINE,9);
    q.x=160;q.y=160;q.target=a;q.owner=SIDE;
    assert(race_scarab_probe_bullet_create(&q));
    q.x=180;q.target=b;assert(race_scarab_probe_bullet_create(&q));
    assert(race_impact_count==3 && sc64_shared_projectiles.active==2);
    race_impact_clear_target(a);
    assert(race_impact_count==2 && sc64_shared_projectiles.active==2);
    race_impact_rows[0].steps=(u8)((race_impact_rows[0].steps&15u)|16u);
    frame=0;screen=SCREEN_PLAY;game_update(0,0);
    assert(race_impact_count==1 && sc64_shared_projectiles.active==2 && race_impact_scarab_tail_count==1);
    for(int n=0;n<5 && race_impact_scarab_tail_count;n++) {++frame;race_impact_tick();}
    assert(sc64_shared_projectiles.active==1);
    clear_tracers();assert(!race_impact_count && !sc64_shared_projectiles.active);
    /* Fully occupied source pool: ordinary paid parent attack still consumes
     * the child when the projectile allocator fails, without causing damage. */
    setup();
    u32 full[100];
    for(unsigned i=0;i<100;i++) {
        full[i]=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_SWARM,i);
        assert(full[i]);
    }
    int victim;int child=launch_paid_reaver(&victim);int health=u[victim].hp;
    for(int i=0;i<200 && u[child].alive;i++) {screen=SCREEN_PLAY;game_update(0,0);}
    assert(!u[child].alive && u[victim].hp==health && race_impact_count==0);
    assert(sc64_shared_projectiles.active==100);
    for(unsigned i=0;i<100;i++)assert(sc64_shared_projectile_release(&sc64_shared_projectiles,full[i],SC64_SHARED_PROJECTILE_SWARM,i));
    puts("Scarab reservation, capacity, paid child cleanup, retained splash and reset passed");
    return 0;
}
