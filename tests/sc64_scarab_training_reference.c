/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Isolated reference of pinned resume_building_unit's non-cheat branch.
 * This is scheduler arithmetic, not an engine clock or gameplay test. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_scarab_lifecycle.h"
typedef struct { unsigned remaining, hp_q8; int complete; } Reference;
typedef struct { int completed; } Fixture;
static int allocate(void *ctx,int *slot){(void)ctx;(void)slot;return 0;}
static int complete(void *ctx,int slot){Fixture *f=ctx;assert(slot==3);++f->completed;return 1;}
static const Sc64ScarabOps ops={allocate,complete,0,0};
/* OpenBW adds construction HP first, returns while remaining was nonzero,
 * and completes only on the next call; set_unit_hp clamps each addition. */
static void reference_tick(Reference *r) {
 const unsigned max_hp = 20u * 256u;
 const unsigned gain = (max_hp - max_hp / 10u + 104u) / 105u;
 assert(gain == 44u);
 r->hp_q8 += gain;
 if(r->hp_q8 > 20u*256u) r->hp_q8=20u*256u;
 if(r->remaining){--r->remaining;return;}
 r->complete=1;
}
int main(void) {
 Reference ref={105,2u*256u,0};Fixture fixture={0};
 Sc64ScarabParent p={3,0,1,0};Sc64ScarabCounts counts={0,0};
 for(unsigned call=1;call<=106;call++) {
  reference_tick(&ref);
  int result=sc64_scarab_production_tick(&p,counts,0,&ops,&fixture);
  if(!ref.complete) {
   unsigned hp=0;assert(result==SC64_SCARAB_PROGRESS);assert(p.active_slot==3 && p.queued==1);
   assert(p.progress==call && ref.remaining==105-call);
   assert(sc64_scarab_training_hp_q8(p.progress,&hp));
   unsigned expected=2u*256u+44u*call; if(expected>20u*256u)expected=20u*256u;
   assert(hp==expected && hp==ref.hp_q8);
  } else {assert(call==106 && result==SC64_SCARAB_COMPLETED && fixture.completed==1);assert(p.active_slot==-1 && p.queued==0 && p.progress==0);}
 }
 unsigned hp=123;assert(!sc64_scarab_training_hp_q8(106,&hp) && hp==123);
 puts("Scarab independent source-tick reference: 105 pending, 106 complete");return 0;
}
