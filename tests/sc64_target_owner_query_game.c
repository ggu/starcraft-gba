/* SPDX-License-Identifier: GPL-3.0-or-later
 * Real-engine differential at the update_orders owner-list boundary.
 *
 * With prepared canonical assets, run `make test-target-owner-query` using
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
static unsigned long queries, probes, hits;
static unsigned hash_bytes(const void *v, unsigned n) { const unsigned char *p=v; unsigned h=2166136261u; while(n--)h=(h^*p++)*16777619u; return h; }
static void sc64_owner_lists_test_probe(void) {
 if (!target_lists_valid) return;
 unsigned uh=hash_bytes(u,sizeof(u)),bh=hash_bytes(b,sizeof(b)),before=seed;
 for(int k=0;k<8;k++) {
  unsigned r=(unsigned)(frame*1103515245u+k*2654435761u); int x=(int)(r%(unsigned)WORLD_W);int y=(int)((r>>9)%(unsigned)WORLD_H);int owner=k%5;int radius=(int[]){1,52,128,320,512,1024,2048,64}[k];
  int attacker=-1,raw=-1;for(int j=0;j<MAX_UNITS;j++)if(u[j].alive && u[j].owner==owner){if(k&1)attacker=u[j].type;raw=u[j].raw_owner;break;}
  sc64_target_index_begin();
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
  sc64_target_hostility_snapshot();
#endif
  int old=nearest_enemy_typed(x,y,owner,radius,attacker SC64_P7_SOURCE_OWNER_ARG(raw));
  sc64_target_index_begin_owner_lists();
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
  sc64_target_hostility_snapshot();
#endif
  int now=nearest_enemy_typed(x,y,owner,radius,attacker SC64_P7_SOURCE_OWNER_ARG(raw));
  if(old!=now){fprintf(stderr,"mismatch frame=%d k=%d old=%d new=%d xy=%d,%d owner=%d radius=%d attacker=%d raw=%d\n",frame,k,old,now,x,y,owner,radius,attacker,raw);abort();} ++queries; if (old >= 0) ++hits;
 }
 if(uh!=hash_bytes(u,sizeof(u))||bh!=hash_bytes(b,sizeof(b))||before!=seed){fputs("query mutated actors or rng\n",stderr);abort();} ++probes;
}
int main(void){gba_init();game_init();race_mode=1;race_human=R_TER;race_bots=2;race_skirmish_map=0;race_bot_races[0]=R_ZERG;race_bot_races[1]=R_PROTOSS;init_state();for(int i=0;i<500;i++)game_update(0,0);printf("real engine frames=%lu target queries=%lu nonempty=%lu\n",probes,queries,hits);return probes == 500 && queries == 4000 && hits > 0 ? 0 : 1;}
