/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Read-only recorded-input witness; the private event header is supplied by the caller. */
#include "../starcraft/game.c"
#include <stdio.h>
#include "sc64_z3_assault_events.h"
typedef struct { int alive,x,y,hp,type,order,target,tx,ty; } Prev;
static Prev old[MAX_UNITS];
static int oldcd[MAX_UNITS];
static void report(void) {
 int side=0,enemy=0,bcount=0;
 for(int i=0;i<MAX_UNITS;i++)if(u[i].alive){if(u[i].owner==SIDE&&!sc64_race_is_worker(u[i].type))side++;if(u[i].owner==ENEMY)enemy++;}
 for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY)bcount++;
 printf("SNAP frame%d hostside%d enemy%d buildings%d minerals%d gas%d\n",frame,side,enemy,bcount,minerals,gas);
 for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!sc64_race_is_worker(u[i].type))
  printf(" S %d type%d hp%d xy%d,%d ord%d target%d dest%d,%d path%d,%d retry%d\n",i,u[i].type,u[i].hp,u[i].x,u[i].y,u[i].order,u[i].target,u[i].tx,u[i].ty,u[i].path_x,u[i].path_y,u[i].path_retry);
 for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY)
  printf(" E %d type%d raw%d hp%d xy%d,%d ord%d target%d\n",i,u[i].type,u[i].raw_type,u[i].hp,u[i].x,u[i].y,u[i].order,u[i].target);
 for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY)
  printf(" B %d type%d raw%d hp%d xy%d,%d\n",i,b[i].type,b[i].raw_type,b[i].hp,b[i].x,b[i].y);
}
int main(void) {
 unsigned idx=0;u16 held=0,prior=0;gba_init();game_init();
 for(unsigned tick=1;tick<75000;tick++) {
  if(idx<sizeof(events)/sizeof(events[0])&&events[idx].tick==tick) held=events[idx++].held;
  for(int i=0;i<MAX_UNITS;i++)old[i]=(Prev){u[i].alive&&u[i].owner==SIDE&&!sc64_race_is_worker(u[i].type),u[i].x,u[i].y,u[i].hp,u[i].type,u[i].order,u[i].target,u[i].tx,u[i].ty};
  for(int i=0;i<MAX_UNITS;i++) oldcd[i]=u[i].cd;
  game_update(held,held & (u16)~prior);prior=held;
  if(frame>=32000 && frame<=70500) {
   for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==30&&oldcd[i]<=1&&u[i].cd>1)
    printf("TANK_FIRE frame%d slot%d xy%d,%d cd%d\n",frame,i,u[i].x,u[i].y,u[i].cd);
   for(int i=0;i<MAX_UNITS;i++)if(old[i].alive&&!u[i].alive) {
    int best=-1,bd=0x7fffffff;
    for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==ENEMY) {int d=dist2(old[i].x,old[i].y,u[j].x,u[j].y);if(d<bd){bd=d;best=j;}}
    printf("DEATH frame%d side%d type%d hp%d xy%d,%d ord%d target%d dest%d,%d nearest%d d2%d etype%d eraw%d exy%d,%d\n",frame,i,old[i].type,old[i].hp,old[i].x,old[i].y,old[i].order,old[i].target,old[i].tx,old[i].ty,best,bd,best<0?-1:u[best].type,best<0?-1:u[best].raw_type,best<0?-1:u[best].x,best<0?-1:u[best].y);
   }
   if(frame%600==0) report();
  }
  if(screen==SCREEN_RESULT)break;
 }
 return 0;
}
