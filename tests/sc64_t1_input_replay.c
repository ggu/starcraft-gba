/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Replays a strictly validated host-generated tape using only game_update. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
typedef struct { unsigned tick; u16 held; } T1Event;
typedef struct { unsigned tick; int frame,minerals,marines,scv,zerg,raynor_owner,raynor_alive; u32 hash; } T1Checkpoint;
typedef struct { int won,lost,marines,scv,minerals,frame; u32 hash; } T1Expected;
#include "sc64_t1_replay_tape.h"
static int count_type(int owner,int type) {int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==owner&&u[i].type==type;return n;}
static int hero_slot(void) {for(int i=0;i<MAX_UNITS;i++)if(u[i].type==U_RAYNOR)return i;return -1;}
static u32 world_hash(void) {
 const u8 *parts[]={(const u8 *)u,(const u8 *)b,(const u8 *)m};
 const size_t sizes[]={sizeof(u),sizeof(b),sizeof(m)};u32 h=2166136261u;
 for(unsigned p=0;p<3;p++)for(size_t i=0;i<sizes[p];i++)h=(h^parts[p][i])*16777619u;
 h=(h^(u32)minerals)*16777619u;h=(h^(u32)gas)*16777619u;h=(h^(u32)frame)*16777619u;
 return (h^(u32)mission_objective_stage)*16777619u;
}
int main(void) {
 game_init();unsigned event=0,checkpoint=0;u16 held=0,previous=0;
 for(unsigned tick=0;tick<T1_TOTAL_TICKS;tick++) {
  if(event<sizeof(t1_events)/sizeof(t1_events[0]) && t1_events[event].tick==tick)held=t1_events[event++].held;
  game_update(held,held & (u16)~previous);previous=held;
  if(checkpoint<sizeof(t1_checkpoints)/sizeof(t1_checkpoints[0]) && t1_checkpoints[checkpoint].tick==tick+1) {
   const T1Checkpoint *c=&t1_checkpoints[checkpoint++];int h=hero_slot();
   if(frame!=c->frame||minerals!=c->minerals||count_type(SIDE,U_MARINE)!=c->marines||count_type(SIDE,U_SCV)!=c->scv||count_type(ENEMY,U_ZERGLING)!=c->zerg||
      (h>=0?u[h].owner:-1)!=c->raynor_owner||(h>=0?u[h].alive:0)!=c->raynor_alive||world_hash()!=c->hash) {
    fprintf(stderr,"T1 replay checkpoint mismatch tick=%u frame=%d expected=%d hash=%08x expected=%08x\n",tick+1,frame,c->frame,world_hash(),c->hash);return 1;
   }
  }
 }
 assert(event==sizeof(t1_events)/sizeof(t1_events[0]) && checkpoint==sizeof(t1_checkpoints)/sizeof(t1_checkpoints[0]));
 assert(won==t1_expected.won && lost==t1_expected.lost && count_type(SIDE,U_MARINE)==t1_expected.marines && count_type(SIDE,U_SCV)==t1_expected.scv && minerals==t1_expected.minerals && frame==t1_expected.frame && world_hash()==t1_expected.hash);
 printf("T1 input replay matched %u checkpoints and final world hash at %u updates\n",checkpoint,T1_TOTAL_TICKS);return 0;
}
