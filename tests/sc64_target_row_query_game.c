/* SPDX-License-Identifier: GPL-3.0-or-later
 * Real-engine differential against the bb55a170 nearest-target query.
 *
 * With prepared canonical assets, run `make test-target-row-query` using
 * the same frozen inputs and weapon source override as the product builds.
 * The test hook is host-only. It advances 500 updates from a fixed custom
 * skirmish setup and compares eight reproducible target queries per update.
 * This is not a controller replay or a campaign qualification.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void sc64_owner_lists_test_probe(void);
#define SC64_OWNER_LIST_TEST 1
#include "../starcraft/game.c"
#include "sc64_target_nearest_reference.h"
static unsigned long queries, probes, hits;
static void sc64_owner_lists_test_probe(void) {
 if (!target_lists_valid) return;
 static Unit saved_units[MAX_UNITS];
 static Building saved_buildings[MAX_BUILDINGS];
 memcpy(saved_units,u,sizeof(u)); memcpy(saved_buildings,b,sizeof(b)); unsigned before=seed;
 for(int k=0;k<8;k++) {
  unsigned r=(unsigned)(frame*1103515245u+k*2654435761u); int x=(int)(r%(unsigned)WORLD_W);int y=(int)((r>>9)%(unsigned)WORLD_H);int owner=k%5;int radius=(int[]){1,52,128,320,512,1024,2048,64}[k];
  int attacker=-1,raw=-1;for(int j=0;j<MAX_UNITS;j++)if(u[j].alive && u[j].owner==owner){if(k&1)attacker=u[j].type;raw=u[j].raw_owner;break;}
  sc64_target_index_begin_owner_lists();
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
  sc64_target_hostility_snapshot();
#endif
  int old=sc64_target_nearest_reference(x,y,owner,radius,attacker SC64_P7_SOURCE_OWNER_ARG(raw));
  int now=sc64_target_nearest(x,y,owner,radius,attacker SC64_P7_SOURCE_OWNER_ARG(raw));
  if(old!=now){fprintf(stderr,"mismatch frame=%d k=%d old=%d new=%d xy=%d,%d owner=%d radius=%d attacker=%d raw=%d\n",frame,k,old,now,x,y,owner,radius,attacker,raw);abort();} ++queries; if (old >= 0) ++hits;
 }
 if(memcmp(saved_units,u,sizeof(u)) || memcmp(saved_buildings,b,sizeof(b)) || before!=seed){fputs("query mutated actors or rng\n",stderr);abort();} ++probes;
}
static unsigned random_state = 0x5a17c0deu;
static unsigned next_random(void) { random_state = random_state * 1664525u + 1013904223u; return random_state; }
static int build_lists_for_fixture(void) {
 target_lists_valid = 0; sc64_target_index_end();
 for (int p=0; p<5; ++p) target_unit_head[p]=target_building_head[p]=-1;
 for (int i=MAX_UNITS-1; i>=0; --i) if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i])) {
  int p=u[i].owner; if (p<0 || p>=5) return 0;
  target_unit_next[i]=target_unit_head[p]; target_unit_head[p]=i;
 }
 for (int i=MAX_BUILDINGS-1; i>=0; --i) if (b[i].alive) {
  int p=b[i].owner; if (p<0 || p>=5) return 0;
  target_building_next[i]=target_building_head[p]; target_building_head[p]=i;
 }
 target_lists_valid=1; return 1;
}
static unsigned random_queries, random_hits, fallback_checks;
static void compare_random_queries(void) {
 static Unit units_before[MAX_UNITS]; static Building buildings_before[MAX_BUILDINGS];
 memcpy(units_before,u,sizeof(u)); memcpy(buildings_before,b,sizeof(b)); unsigned seed_before=seed;
 for (int k=0;k<32;k++) {
  int x=(k==0)?2047:(int)(next_random()%2048), y=(k==0)?2047:(int)(next_random()%2048);
  int owner=k%5, radius=(int[]){1,52,128,320,512,1024,2048,64}[k%8];
  int old=sc64_target_nearest_reference(x,y,owner,radius,-1 SC64_P7_SOURCE_OWNER_ARG(-1));
  int now=sc64_target_nearest(x,y,owner,radius,-1 SC64_P7_SOURCE_OWNER_ARG(-1));
  if(old!=now){fprintf(stderr,"synthetic query mismatch %d != %d, %d %d %d\n",old,now,x,y,owner);abort();}
  ++random_queries; if(old>=0) ++random_hits;
 }
 if(memcmp(units_before,u,sizeof(u)) || memcmp(buildings_before,b,sizeof(b)) || seed_before!=seed)abort();
}
static void synthetic_worlds(void) {
 int first_unit=-1,first_building=-1;
 for(int i=0;i<MAX_UNITS;i++)if(u[i].alive){first_unit=i;break;}
 for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive){first_building=i;break;}
 if(first_unit<0||first_building<0)abort();
 Unit unit=u[first_unit]; Building building=b[first_building];
 race_cols=128; race_rows=128; memset(visible,0xff,sizeof(visible));
 /* Explicit equal-distance tie at row/column 31, including loaded/dead and
  * changed-owner rebuilds. The smallest slot must continue to win. */
 memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
 u[0]=unit; u[5]=unit; u[0].alive=u[5].alive=1;
 u[0].owner=u[5].owner=SIDE; u[0].x=u[5].x=1984; u[0].y=u[5].y=1984;
 u[0].order=u[5].order=ORDER_IDLE;
 if(!owners_hostile(ENEMY,SIDE) || !build_lists_for_fixture())abort();
 sc64_target_index_begin_owner_lists();
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
 sc64_target_hostility_snapshot();
#endif
 int tie0=sc64_target_nearest_reference(1984,1952,ENEMY,65,-1 SC64_P7_SOURCE_OWNER_ARG(-1));
 int tie1=sc64_target_nearest(1984,1952,ENEMY,65,-1 SC64_P7_SOURCE_OWNER_ARG(-1));
 if(tie0!=0 || tie1!=tie0)abort();
#ifdef SC64_TRANSPORT_RUNTIME
 u[0].order=ORDER_TRANSPORTED; if(!build_lists_for_fixture())abort(); sc64_target_index_begin_owner_lists();
 if(sc64_target_nearest_reference(1984,1952,ENEMY,65,-1 SC64_P7_SOURCE_OWNER_ARG(-1))!=5 ||
    sc64_target_nearest(1984,1952,ENEMY,65,-1 SC64_P7_SOURCE_OWNER_ARG(-1))!=5)abort();
 u[0].order=ORDER_IDLE;
#endif
 u[0].alive=0; if(!build_lists_for_fixture())abort(); sc64_target_index_begin_owner_lists();
 if(sc64_target_nearest_reference(1984,1952,ENEMY,65,-1 SC64_P7_SOURCE_OWNER_ARG(-1))!=5 ||
    sc64_target_nearest(1984,1952,ENEMY,65,-1 SC64_P7_SOURCE_OWNER_ARG(-1))!=5)abort();
 u[0].alive=1; u[0].owner=ENEMY; if(!build_lists_for_fixture())abort(); sc64_target_index_begin_owner_lists();
 if(sc64_target_nearest_reference(1984,1952,ENEMY,65,-1 SC64_P7_SOURCE_OWNER_ARG(-1))!=5 ||
    sc64_target_nearest(1984,1952,ENEMY,65,-1 SC64_P7_SOURCE_OWNER_ARG(-1))!=5)abort();
 for(int world=0;world<250;world++) {
  memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
  for(int i=0;i<24;i++) { u[i]=unit; u[i].alive=1; u[i].owner=i%5; u[i].x=(int)(next_random()%2048); u[i].y=(int)(next_random()%2048); }
  for(int i=0;i<12;i++) { b[i]=building; b[i].alive=1; b[i].owner=i%5; b[i].x=(int)(next_random()%2048); b[i].y=(int)(next_random()%2048); }
#ifdef SC64_TRANSPORT_RUNTIME
  u[1].order=ORDER_TRANSPORTED;
#endif
  u[2].alive=0;
  if(!build_lists_for_fixture())abort(); sc64_target_index_begin_owner_lists();
  if(!sc64_target_index_active)abort();
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
  sc64_target_hostility_snapshot();
#endif
  compare_random_queries();
  /* Move across rows; old row occupancy must remain a harmless stale positive. */
  int oldrow=u[0].y>>6; u[0].y=(oldrow==31)?64:1984; sc64_target_index_move(&u[0]);
  if(!sc64_target_index_active)abort();
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
  if(!(sc64_target_owner_rows[u[0].owner] & ((u32)1u << oldrow)))abort();
#endif
  u[3].alive=0; compare_random_queries();
  /* A changed owner requires fresh owner lists, exactly as on the next tick. */
  u[4].owner=(u[4].owner+1)%5; if(!build_lists_for_fixture())abort(); sc64_target_index_begin_owner_lists();
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
  sc64_target_hostility_snapshot();
#endif
  compare_random_queries();
  /* Invalid position disables the index and production takes the scan path. */
  u[0].x=-1; if(!build_lists_for_fixture())abort(); sc64_target_index_begin_owner_lists(); if(sc64_target_index_active)abort();
  int a=nearest_enemy_typed(1024,1024,0,320,-1 SC64_P7_SOURCE_OWNER_ARG(-1));
  int b0=nearest_enemy_scan(1024,1024,0,320,-1 SC64_P7_SOURCE_OWNER_ARG(-1)); if(a!=b0)abort(); ++fallback_checks;
  u[0].owner=5; if(build_lists_for_fixture())abort();
  a=nearest_enemy_typed(1024,1024,0,320,-1 SC64_P7_SOURCE_OWNER_ARG(-1));
  b0=nearest_enemy_scan(1024,1024,0,320,-1 SC64_P7_SOURCE_OWNER_ARG(-1)); if(a!=b0)abort(); ++fallback_checks;
 }
 printf("synthetic worlds=250 comparisons=%u hits=%u fallback=%u\n",random_queries,random_hits,fallback_checks);
 if(random_queries!=24000 || random_hits==0 || fallback_checks!=500)abort();
}
int main(void){gba_init();game_init();race_mode=1;race_human=R_TER;race_bots=2;race_skirmish_map=0;race_bot_races[0]=R_ZERG;race_bot_races[1]=R_PROTOSS;init_state();for(int i=0;i<500;i++)game_update(0,0);printf("real engine frames=%lu target queries=%lu nonempty=%lu\n",probes,queries,hits); if(probes != 500 || queries != 4000 || hits == 0) return 1; synthetic_worlds(); return 0;}
