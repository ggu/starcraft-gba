/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_scarab_lifecycle.h"

enum { EMPTY, BUILDING, INSIDE, OUTSIDE };
typedef struct { int phase, parent, owner; } Actor;
typedef struct { Actor actors[16]; int parent, owner, allocations, completions, launches, cancels; } World;
static int allocate(void *ctx, int *slot)
{
    World *w=ctx;
    for (int i=0;i<16;i++) if (w->actors[i].phase==EMPTY) {
        w->actors[i]=(Actor){BUILDING,w->parent,w->owner};
        ++w->allocations; *slot=i; return 1;
    }
    return 0;
}
static int complete(void *ctx, int slot)
{
    World *w=ctx;
    if (slot<0 || slot>=16 || w->actors[slot].phase!=BUILDING ||
        w->actors[slot].parent!=w->parent || w->actors[slot].owner!=w->owner) return 0;
    w->actors[slot].phase=INSIDE; ++w->completions; return 1;
}
static int launch(void *ctx, int slot)
{
    World *w=ctx;
    if (slot<0 || slot>=16 || w->actors[slot].phase!=INSIDE ||
        w->actors[slot].parent!=w->parent || w->actors[slot].owner!=w->owner) return 0;
    w->actors[slot].phase=OUTSIDE; ++w->launches; return 1;
}
static int cancel_active(void *ctx, int slot)
{
    World *w=ctx;
    if (slot<0 || slot>=16 || w->actors[slot].phase!=BUILDING ||
        w->actors[slot].parent!=w->parent || w->actors[slot].owner!=w->owner) return 0;
    w->actors[slot].phase=EMPTY; ++w->cancels; return 1;
}
static const Sc64ScarabOps ops={allocate,complete,launch,cancel_active};
static Sc64ScarabCounts counts(const World *w)
{
    Sc64ScarabCounts c={0,0};
    for(int i=0;i<16;i++) if(w->actors[i].parent==w->parent && w->actors[i].owner==w->owner) {
        if(w->actors[i].phase==INSIDE) ++c.inside;
        if(w->actors[i].phase==OUTSIDE) ++c.outside;
    }
    return c;
}
static void check_queue_and_payment(void)
{
    Sc64ScarabParent p; World w={0}; int minerals=100;
    w.parent=3; w.owner=2; sc64_scarab_parent_init(&p);
    int insufficient=14;
    assert(!sc64_scarab_enqueue(&p,counts(&w),0,&insufficient));
    assert(insufficient==14 && p.queued==0 && w.allocations==0);
    for(int i=0;i<5;i++) assert(sc64_scarab_enqueue(&p,counts(&w),0,&minerals));
    assert(minerals==25 && p.queued==5 && w.allocations==0);
    Sc64ScarabParent before=p; int saved=minerals;
    assert(!sc64_scarab_enqueue(&p,counts(&w),0,&minerals));
    assert(!memcmp(&p,&before,sizeof(p)) && minerals==saved && !w.allocations);
    /* Full actor pool cannot consume or allocate any future paid entry. */
    for(int i=0;i<16;i++) w.actors[i]=(Actor){INSIDE,99,99};
    for(int i=0;i<4;i++) assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_WAIT);
    assert(p.queued==5 && p.active_slot==-1 && p.progress==0 && minerals==25 && !w.allocations);
    w.actors[4].phase=EMPTY;
    assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_ALLOCATED);
    assert(p.active_slot==4 && p.queued==5 && p.progress==0 && w.allocations==1);
    for(int i=0;i<105;i++) assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_PROGRESS);
    assert(p.progress==105 && p.queued==5 && p.active_slot==4 && w.completions==0);
    /* Wrong owner cannot complete or mutate the queue; retry recovers. */
    w.actors[4].owner=1; before=p;
    assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_WAIT);
    assert(!memcmp(&before,&p,sizeof(p)) && w.completions==0);
    w.actors[4].owner=2;
    assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_COMPLETED);
    assert(p.queued==4 && p.active_slot==-1 && counts(&w).inside==1 && w.completions==1);
    assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_WAIT);
    assert(w.allocations==1);
}
static void check_capacity_and_launch(void)
{
    Sc64ScarabParent p; World w={0}; int minerals=1000; Sc64ScarabFlight flight={0,0};
    w.parent=3; w.owner=2; sc64_scarab_parent_init(&p);
    for(int i=0;i<4;i++) w.actors[i]=(Actor){INSIDE,3,2};
    w.actors[4]=(Actor){OUTSIDE,3,2};
    /* Outside does not count toward the 5 inside+queued stock cap. */
    assert(sc64_scarab_enqueue(&p,counts(&w),0,&minerals));
    assert(minerals==985 && p.queued==1);
    assert(!sc64_scarab_enqueue(&p,counts(&w),0,&minerals));
    assert(!sc64_scarab_launch(&p,counts(&w),0,&ops,&w,0,&flight));
    w.actors[4].phase=EMPTY;
    /* Foreign-parent and wrong-owner actors never leave their owner. */
    w.actors[7]=(Actor){INSIDE,4,2}; w.actors[8]=(Actor){INSIDE,3,1};
    assert(!sc64_scarab_launch(&p,counts(&w),0,&ops,&w,7,&flight));
    assert(!sc64_scarab_launch(&p,counts(&w),0,&ops,&w,8,&flight));
    assert(p.cooldown==0 && flight.life==0);
    assert(sc64_scarab_launch(&p,counts(&w),0,&ops,&w,0,&flight));
    assert(w.actors[0].phase==OUTSIDE && counts(&w).outside==1 && p.cooldown==60);
    assert(flight.life==90 && flight.no_collision==7 && sc64_scarab_flight_valid(&flight));
    for(int i=0;i<6;i++) {assert(!sc64_scarab_flight_tick(&flight)); assert(flight.no_collision==6-i);}
    assert(!sc64_scarab_flight_tick(&flight) && flight.no_collision==0 && flight.life==83);
    for(int i=0;i<52;i++) sc64_scarab_cooldown_tick(&p);
    assert(p.cooldown==8); for(int i=0;i<8;i++)sc64_scarab_cooldown_tick(&p); assert(p.cooldown==0);
    assert(!sc64_scarab_launch(&p,counts(&w),0,&ops,&w,1,&flight)); /* one outside */
    for(int i=0;i<82;i++)assert(!sc64_scarab_flight_tick(&flight));
    assert(sc64_scarab_flight_tick(&flight)); /* caller retires actual child */
    assert(!sc64_scarab_flight_valid(&flight));
    w.actors[0].phase=EMPTY;
    assert(sc64_scarab_launch(&p,counts(&w),0,&ops,&w,1,&flight));
}
static void check_training_hp(void)
{
    unsigned hp=99;
    assert(sc64_scarab_training_hp_q8(0,&hp) && hp==512);
    assert(sc64_scarab_training_hp_q8(104,&hp) && hp==5088);
    assert(sc64_scarab_training_hp_q8(105,&hp) && hp==5120);
    hp=99; assert(!sc64_scarab_training_hp_q8(106,&hp) && hp==99);
    assert(!sc64_scarab_training_hp_q8(0,0));
}
static void check_upgrade_and_invalid(void)
{
    Sc64ScarabParent p; World w={0}; int minerals=75; w.parent=3; w.owner=2;
    sc64_scarab_parent_init(&p); for(int i=0;i<5;i++)w.actors[i]=(Actor){INSIDE,3,2};
    assert(!sc64_scarab_enqueue(&p,counts(&w),0,&minerals));
    for(int i=0;i<5;i++)assert(sc64_scarab_enqueue(&p,counts(&w),1,&minerals));
    assert(minerals==0 && p.queued==5);
    assert(!sc64_scarab_enqueue(&p,counts(&w),1,&minerals));
    /* Model tolerates an above-current-capacity shape without claiming an
     * actual transfer path; source transfer first cancels the paid queue. */
    assert(sc64_scarab_parent_valid(&p,counts(&w),0));
    minerals=100;
    assert(!sc64_scarab_enqueue(&p,counts(&w),0,&minerals));
    assert(minerals==100);
    assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_ALLOCATED);
    assert(p.queued==5 && p.active_slot>=0);
    sc64_scarab_parent_init(&p); p.active_slot=4;
    assert(!sc64_scarab_parent_valid(&p,(Sc64ScarabCounts){0,0},0));
    p.active_slot=-1; p.progress=1;
    assert(!sc64_scarab_parent_valid(&p,(Sc64ScarabCounts){0,0},0));
    sc64_scarab_parent_init(&p);
    assert(!sc64_scarab_parent_valid(&p,(Sc64ScarabCounts){0,2},0));
    assert(!sc64_scarab_parent_valid(&p,(Sc64ScarabCounts){0,0},2));
    assert(!sc64_scarab_flight_valid(&(Sc64ScarabFlight){91,0}));
    assert(!sc64_scarab_flight_valid(&(Sc64ScarabFlight){10,8}));
    assert(!sc64_scarab_flight_valid(&(Sc64ScarabFlight){1,2}));
}
static void check_cancellation_and_loss(void)
{
    Sc64ScarabParent p; World w={0}; int minerals=100;
    w.parent=3; w.owner=2; sc64_scarab_parent_init(&p);
    for(int i=0;i<3;i++)assert(sc64_scarab_enqueue(&p,counts(&w),0,&minerals));
    assert(minerals==55 && p.queued==3);
    assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_ALLOCATED);
    int slot=p.active_slot;
    for(int i=0;i<10;i++)assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_PROGRESS);
    /* A later entry refunds without touching the active child. */
    assert(sc64_scarab_cancel(&p,counts(&w),0,&ops,&w,1,&minerals,65535));
    assert(minerals==70 && p.queued==2 && p.active_slot==slot && p.progress==10 && !w.cancels);
    Sc64ScarabParent before=p; int previous=minerals;
    assert(!sc64_scarab_cancel(&p,counts(&w),0,&ops,&w,2,&minerals,65535));
    assert(!sc64_scarab_cancel(&p,counts(&w),0,&ops,&w,0,&minerals,84)); /* refund bound */
    int near_int_max=INT_MAX-14;
    assert(!sc64_scarab_cancel(&p,counts(&w),0,&ops,&w,0,&near_int_max,INT_MAX));
    assert(near_int_max==INT_MAX-14);
    assert(!memcmp(&before,&p,sizeof(p)) && minerals==previous && !w.cancels);
    w.actors[slot].owner=1; /* callback refusal must be atomic */
    assert(!sc64_scarab_cancel(&p,counts(&w),0,&ops,&w,0,&minerals,65535));
    assert(!memcmp(&before,&p,sizeof(p)) && minerals==previous && !w.cancels);
    w.actors[slot].owner=2;
    assert(sc64_scarab_cancel(&p,counts(&w),0,&ops,&w,0,&minerals,65535));
    assert(minerals==85 && p.queued==1 && p.active_slot==-1 && p.progress==0 && w.cancels==1);
    assert(sc64_scarab_cancel(&p,counts(&w),0,&ops,&w,0,&minerals,65535));
    assert(minerals==100 && !p.queued);
    assert(!sc64_scarab_cancel(&p,counts(&w),0,&ops,&w,0,&minerals,65535));
    assert(minerals==100 && w.cancels==1);
    assert(sc64_scarab_enqueue(&p,counts(&w),0,&minerals));
    assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_ALLOCATED);
    slot=p.active_slot; assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_PROGRESS);
    assert(!sc64_scarab_active_lost(&p,counts(&w),0,slot+1));
    w.actors[slot].phase=EMPTY;
    assert(sc64_scarab_active_lost(&p,counts(&w),0,slot));
    assert(p.queued==1 && p.active_slot==-1 && p.progress==0 && minerals==85);
    assert(!sc64_scarab_active_lost(&p,counts(&w),0,slot));
    assert(sc64_scarab_production_tick(&p,counts(&w),0,&ops,&w)==SC64_SCARAB_ALLOCATED);
    assert(p.queued==1 && minerals==85); /* paid entry retries, no refund */
}
int main(void)
{
    check_queue_and_payment(); check_capacity_and_launch(); check_training_hp(); check_upgrade_and_invalid(); check_cancellation_and_loss();
    puts("Scarab source-tick lifecycle: queue, slots, capacity, ownership and timers passed");
    return 0;
}
