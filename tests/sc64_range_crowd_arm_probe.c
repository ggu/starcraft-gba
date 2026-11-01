/* SPDX-License-Identifier: GPL-3.0-or-later
 * Authored crowded skirmish scene for paired real-ARM whole-frame sampling.
 * Default map and AI run normally; actors are added and ordered only at boot. */
#ifndef CROWD_COUNT
#error "CROWD_COUNT required"
#endif
#ifndef CROWD_HUMAN
#error "CROWD_HUMAN required"
#endif
#ifndef CROWD_BOTS
#error "CROWD_BOTS required"
#endif
#define game_init crowd_engine_init
#define game_update crowd_engine_update
#include "../starcraft/game.c"
#undef game_init
#undef game_update

volatile u32 probe_tick, probe_fault, probe_world_hash;
volatile u32 probe_alive, probe_moving, probe_moves, probe_attacks, probe_damage;
volatile u32 probe_initial_alive, probe_owner0, probe_owner1, probe_owner2, probe_owner3;
static u16 old_x[MAX_UNITS], old_y[MAX_UNITS], old_cd[MAX_UNITS];
static int old_hp_sum;
static int total_alive(void) { int n=0; for(int i=0;i<MAX_UNITS;++i) if(u[i].alive)++n; return n; }
static int owner_at(int i) { return i ? race_bot_owner_ids[i-1] : SIDE; }
static int race_unit(int r) { return r==R_TER ? U_MARINE : r==R_ZERG ? U_HYDRALISK : U_ZEALOT; }
static int free_spot(int x,int y) {
 if(x<16 || y<16 || x>=WORLD_W-16 || y>=WORLD_H-16 || !is_traversable(x/16,y/16,-1))return 0;
 for(int j=0;j<MAX_UNITS;++j) if(u[j].alive && dist2(x,y,u[j].x,u[j].y)<24*24)return 0;
 for(int j=0;j<MAX_BUILDINGS;++j) if(b[j].alive && dist2(x,y,b[j].x,b[j].y)<48*48)return 0;
 return 1;
}
static int add_actor(int owner,int base) {
 for(int ring=1;ring<=12;++ring)
  for(int dy=-ring;dy<=ring;++dy)for(int dx=-ring;dx<=ring;++dx) {
   if(FAST_ABS(dx)!=ring && FAST_ABS(dy)!=ring)continue;
   int x=race_base_x[base]+dx*24,y=race_base_y[base]+dy*24;
   if(free_spot(x,y)) {
    int slot=spawn_unit(owner,race_unit(race_player[owner]),x,y);
    if(slot<0)return 0;
    u[slot].order=ORDER_ATTACK_MOVE;
    u[slot].tx=race_base_x[base ? 0 : 1];
    u[slot].ty=race_base_y[base ? 0 : 1];
    return 1;
   }
  }
 return 0;
}
static u32 mix(u32 h,u32 v) { return (h^v)*16777619u; }
void game_init(void) {
 crowd_engine_init();
 race_mode=1;race_human=CROWD_HUMAN;race_bots=CROWD_BOTS;race_skirmish_map=0;
 for(int i=0;i<3;++i)race_bot_races[i]=(CROWD_HUMAN+i+1)%3;
 init_state();
 if(screen!=SCREEN_PLAY || !race_mode || race_human!=CROWD_HUMAN || race_bots!=CROWD_BOTS || CROWD_COUNT>MAX_UNITS || total_alive()>CROWD_COUNT){probe_fault|=1;return;}
 build_path_grid();
 for(int i=0;total_alive()<CROWD_COUNT;++i) {
  int base=i%(1+CROWD_BOTS);
  if(!add_actor(owner_at(base),base)){probe_fault|=2;return;}
 }
 probe_initial_alive=(u32)total_alive();
 u32 h=2166136261u;
 h=mix(h,race_skirmish_map);h=mix(h,race_human);h=mix(h,race_bots);h=mix(h,seed);h=mix(h,minerals);h=mix(h,gas);
 for(int i=0;i<5;++i){h=mix(h,race_minerals[i]);h=mix(h,race_gas[i]);}
 for(int i=0;i<3;++i)h=mix(h,race_bot_races[i]);
 for(int i=0;i<MAX_UNITS;++i) {
  if(!u[i].alive){old_x[i]=0xffff;continue;}
  h=mix(h,i);h=mix(h,u[i].owner);h=mix(h,u[i].type);h=mix(h,u[i].x);h=mix(h,u[i].y);h=mix(h,u[i].hp);h=mix(h,u[i].order);h=mix(h,u[i].tx);h=mix(h,u[i].ty);
  old_x[i]=(u16)u[i].x;old_y[i]=(u16)u[i].y;old_cd[i]=(u16)u[i].cd;old_hp_sum+=u[i].hp+u[i].shield;
 }
 for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive){h=mix(h,i);h=mix(h,b[i].owner);h=mix(h,b[i].type);h=mix(h,b[i].x);h=mix(h,b[i].y);h=mix(h,b[i].hp);old_hp_sum+=b[i].hp;}
 probe_world_hash=h;
}
void game_update(u16 held,u16 pressed) {
 if(probe_fault)return;
 ++probe_tick;crowd_engine_update(held,pressed);
 if(screen!=SCREEN_PLAY || frame!=(int)probe_tick){probe_fault|=4;return;}
 int sum=0,alive=0,moving=0,owner[4]={0,0,0,0};
 for(int i=0;i<MAX_UNITS;++i) {
  if(!u[i].alive){old_x[i]=0xffff;continue;}
  ++alive;sum+=u[i].hp+u[i].shield;
  for(int k=0;k<=CROWD_BOTS;++k)if(u[i].owner==owner_at(k))++owner[k];
  if(old_x[i]!=0xffff && (old_x[i]!=u[i].x || old_y[i]!=u[i].y)){++moving;++probe_moves;}
  if(old_x[i]!=0xffff && u[i].cd>old_cd[i])++probe_attacks;
  old_x[i]=(u16)u[i].x;old_y[i]=(u16)u[i].y;old_cd[i]=(u16)u[i].cd;
 }
 for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive)sum+=b[i].hp;
 if(sum<old_hp_sum)probe_damage+=(u32)(old_hp_sum-sum);
 old_hp_sum=sum;probe_alive=alive;probe_moving=moving;
 probe_owner0=owner[0];probe_owner1=owner[1];probe_owner2=owner[2];probe_owner3=owner[3];
}
