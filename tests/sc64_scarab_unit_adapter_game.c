/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_scarab_unit_adapter.h"

static int fresh_reaver(int owner)
{
    int slot=spawn_unit(owner,U_REAVER,128,128);
    assert(slot>=0 && race_scarab_parent_init(slot));
    return slot;
}
static void queue_wait_cancel_and_retry(void)
{
    memset(u,0,sizeof(u)); minerals=100;
    int p=fresh_reaver(SIDE); assert(race_scarab_enqueue(p,0));
    assert(minerals==85);
    for(int i=0;i<MAX_UNITS;i++)if(i!=p)u[i].alive=1;
    Unit before=u[p];
    for(int i=0;i<3;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_WAIT);
    assert(!memcmp(&before,&u[p],sizeof(before)) && minerals==85);
    int free_slot=(p==1)?2:1; memset(&u[free_slot],0,sizeof(u[free_slot]));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    assert(u[free_slot].alive && u[free_slot].type==U_SCARAB && u[free_slot].raw_type==85);
    assert(u[free_slot].hp==20 && u[free_slot].maxhp==20 && u[free_slot].shield==10 && u[free_slot].armor==0);
    assert(invincible_units[free_slot] && u[free_slot].owner==u[p].owner && u[free_slot].raw_owner==u[p].raw_owner);
    int parent; unsigned phase; assert(race_scarab_child_lookup(free_slot,&parent,&phase));
    assert(parent==p && phase==SC64_SCARAB_CHILD_TRAINING);
    for(int i=0;i<104;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED);
    assert(race_scarab_child_lookup(free_slot,&parent,&phase) && phase==SC64_SCARAB_CHILD_INSIDE);
    assert(race_scarab_enqueue(p,0)); assert(minerals==70);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_WAIT); /* no slot */
    assert(race_scarab_cancel(p,0,0)); assert(minerals==85);
    assert(!race_scarab_cancel(p,0,0) && minerals==85);
}
static void active_cancel_and_loss(void)
{
    memset(u,0,sizeof(u)); race_minerals[ENEMY]=100; int p=fresh_reaver(ENEMY);
    assert(race_scarab_enqueue(p,0) && race_minerals[ENEMY]==85);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent state; Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(p,0,&state,&counts)); int child=state.active_slot;
    u[child].raw_owner=4; Unit parent_before=u[p];
    assert(!race_scarab_cancel(p,0,0));
    assert(!memcmp(&u[p],&parent_before,sizeof(parent_before)) && race_minerals[ENEMY]==85 && u[child].alive);
    u[child].raw_owner=u[p].raw_owner;
    race_minerals[ENEMY]=65530; parent_before=u[p]; Unit child_before=u[child];
    assert(!race_scarab_cancel(p,0,0));
    assert(!memcmp(&u[p],&parent_before,sizeof(Unit)) && !memcmp(&u[child],&child_before,sizeof(Unit)) && race_minerals[ENEMY]==65530);
    race_minerals[ENEMY]=85;
    assert(race_scarab_cancel(p,0,0) && !u[child].alive && race_minerals[ENEMY]==100);
    assert(race_scarab_enqueue(p,0) && race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    assert(race_scarab_parent_read(p,0,&state,&counts)); child=state.active_slot;
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    u[child].alive=0; u[child].raw_owner=4; parent_before=u[p];
    assert(!race_scarab_child_lost(p,0,child));
    assert(!memcmp(&u[p],&parent_before,sizeof(Unit)));
    u[child].raw_owner=u[p].raw_owner;
    assert(race_scarab_child_lost(p,0,child));
    assert(!race_scarab_child_lost(p,0,child));
    assert(race_minerals[ENEMY]==85 && race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    assert(race_minerals[ENEMY]==85);
    assert(race_scarab_parent_lost(p,0)==1);
    assert(race_scarab_parent_read(p,0,&state,&counts)==0); /* parent must now be retired by caller */
}
static void stop_signal_and_outside_parent_loss(void)
{
    for(int signal=0;signal<2;signal++) {
        memset(u,0,sizeof(u)); minerals=100; int p=fresh_reaver(SIDE);
        assert(race_scarab_enqueue(p,0)); assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
        Sc64ScarabParent state; Sc64ScarabCounts counts; assert(race_scarab_parent_read(p,0,&state,&counts)); int child=state.active_slot;
        for(int i=0;i<104;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
        assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED);
        assert(race_scarab_launch_child(p,0,child)); assert(race_scarab_enqueue(p,0));
        Unit parent_before=u[p]; int parent; unsigned phase;
        if(signal) {
            assert(race_scarab_stop_signal(p,0));
            assert(race_scarab_child_lookup(child,&parent,&phase) && phase==SC64_SCARAB_CHILD_SELF_DESTRUCT);
            assert(!memcmp(&u[p],&parent_before,sizeof(Unit)) && u[child].alive);
        }
        assert(race_scarab_parent_read(p,0,&state,&counts) && state.cooldown==60 && state.queued==1);
        for(int i=0;i<60;i++)assert(race_scarab_cooldown_tick(p,0));
        assert(race_scarab_parent_read(p,0,&state,&counts) && state.cooldown==0 && state.queued==1);
        assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
        assert(race_scarab_parent_read(p,0,&state,&counts)); int active=state.active_slot;
        u[child].target=17; Unit child_before=u[child]; int bank_before=minerals;
        assert(race_scarab_parent_lost(p,0)==1 && u[child].alive && !u[active].alive && minerals==bank_before);
        assert(race_scarab_child_lookup(child,&parent,&phase) && parent==-1 &&
               phase==(signal?SC64_SCARAB_CHILD_SELF_DESTRUCT:SC64_SCARAB_CHILD_OUTSIDE));
        assert(u[child].owner==child_before.owner && u[child].raw_owner==child_before.raw_owner &&
               u[child].target==17 && u[child].work_timer==child_before.work_timer);
        u[p].alive=0; int reused=spawn_unit(SIDE,U_REAVER,128,128); assert(reused==p);
        assert(race_scarab_parent_init(reused));
        assert(race_scarab_child_lookup(child,&parent,&phase) && parent==-1);
    }
}
static void cancel_all_atomic(void)
{
    memset(u,0,sizeof(u)); minerals=100; int p=fresh_reaver(SIDE);
    assert(race_scarab_enqueue(p,0) && race_scarab_enqueue(p,0) && minerals==70);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent state; Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(p,0,&state,&counts)); int child=state.active_slot;
    minerals=65510; Unit parent_before=u[p],child_before=u[child];
    assert(!race_scarab_cancel_all(p,0));
    assert(!memcmp(&u[p],&parent_before,sizeof(Unit)) && !memcmp(&u[child],&child_before,sizeof(Unit)) && minerals==65510);
    minerals=70; assert(race_scarab_cancel_all(p,0));
    assert(minerals==100 && !u[child].alive && race_scarab_parent_read(p,0,&state,&counts) && state.queued==0);
    assert(race_scarab_cancel_all(p,0) && minerals==100);
}
static void malformed_and_slot_reuse(void)
{
    memset(u,0,sizeof(u)); minerals=100; int p=fresh_reaver(SIDE);
    int saved=u[p].carry; u[p].carry=2147483647; Unit before=u[p];
    assert(!race_scarab_enqueue(p,0) && minerals==100 && !memcmp(&before,&u[p],sizeof(Unit)));
    u[p].carry=saved;
    assert(race_scarab_enqueue(p,0)); assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent state; Sc64ScarabCounts counts; assert(race_scarab_parent_read(p,0,&state,&counts)); int child=state.active_slot;
    int second=spawn_unit(SIDE,U_SCARAB,128,128); assert(second>=0);
    u[second].raw_type=85; u[second].raw_owner=u[p].raw_owner; u[second].build=-1;
    assert(race_scarab_child_set(second,(unsigned)p,SC64_SCARAB_CHILD_TRAINING,0,0));
    before=u[p]; assert(!race_scarab_enqueue(p,0)); assert(!memcmp(&before,&u[p],sizeof(Unit)) && minerals==85);
    u[second].alive=0;
    /* A recycled parent slot may not be initialized over a surviving child. */
    u[p].alive=0; assert(!race_scarab_child_lookup(child,0,0));
    int reused=spawn_unit(SIDE,U_REAVER,128,128); assert(reused==p);
    assert(!race_scarab_parent_init(reused));
}
int main(void)
{
    gba_init(); game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1; init_state();
    queue_wait_cancel_and_retry(); active_cancel_and_loss(); stop_signal_and_outside_parent_loss(); cancel_all_atomic(); malformed_and_slot_reuse();
    puts("Scarab actual Unit adapter: paid queue, allocation, cancellation, loss and STOP signal pass");
    return 0;
}
