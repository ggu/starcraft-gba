/* SPDX-License-Identifier: GPL-3.0-or-later
 * Real candidate-envelope graph validation; no gameplay/namespace claim. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_scarab_unit_adapter.h"
#include "sc64_save_candidate_fixture.h"
static RaceSave good,bad;
static Unit before_units[MAX_UNITS];
static Building before_buildings[MAX_BUILDINGS];
static void encode(int slot,const Unit *q){
#if defined(SC64_ZP_FULL_STORAGE)
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
 assert(sc64_p7_typed_unit_encode(&bad.units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#elif defined(SC64_Z7_DERIVED_SAVE)
 assert(sc64_derived_unit_encode(&bad.units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#else
 assert(sc64_zp_unit_save_encode(&bad.units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#endif
#elif defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
 assert(sc64_unit_save_encode(&bad.units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#else
 for(unsigned j=0;j<RACE_UNIT_WORDS;j++){int n;memcpy(&n,(const u8*)q+j*sizeof(int),sizeof(n));bad.units[slot][j]=(s16)n;}
#endif
}
static void valid_record(void){
 assert(race_save_game());fixture_capture_save(&good);
 assert(race_scarab_save_candidate_valid(&good,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
 assert(race_load_game());
}
static void rejected(const char *what){
 int before_minerals=minerals,before_gas=gas,before_frame=frame;u32 before_seed=seed;
 assert(!race_scarab_save_candidate_valid(&bad,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
 memcpy(before_units,u,sizeof(u));memcpy(before_buildings,b,sizeof(b));
 write_candidate_record(&bad);assert(!race_load_game());
 assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)));
 assert(before_minerals==minerals && before_gas==gas && before_frame==frame && before_seed==seed);
 printf("rejected %s atomically\n",what);
}
int main(void){
 gba_init();game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;race_skirmish_map=0;race_bot_races[0]=R_ZERG;race_skirmish_init();screen=SCREEN_PLAY;minerals=500;
 int parent=spawn_unit(SIDE,U_REAVER,128,128);assert(parent>=0&&race_scarab_parent_init(parent));assert(race_scarab_enqueue(parent,0));assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_ALLOCATED);
 Sc64ScarabParent p;Sc64ScarabCounts counts;assert(race_scarab_parent_read(parent,0,&p,&counts));int child=p.active_slot;
 for(int i=0;i<105;i++)assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_PROGRESS);
 /* Script invincibility overrides remain legal live state. */
 invincible_units[child]=0;valid_record();assert(invincible_units[child]==0);
 Unit q,par;assert(race_save_decode_unit(&good,child,(unsigned)race_cols*16u,(unsigned)race_rows*16u,&q));assert(race_save_decode_unit(&good,parent,(unsigned)race_cols*16u,(unsigned)race_rows*16u,&par));
 bad=good;q.raw_type=84;encode(child,&q);rejected("raw type");q.raw_type=85;
 bad=good;q.owner=ENEMY;encode(child,&q);rejected("owner");q.owner=SIDE;
 bad=good;q.raw_owner=q.raw_owner==-1?0:-1;encode(child,&q);rejected("source owner");q.raw_owner=par.raw_owner;
 bad=good;q.selected=1;encode(child,&q);rejected("selected hidden child");q.selected=0;
 bad=good;q.hp=19;encode(child,&q);rejected("training health");q.hp=20;
 bad=good;par.raw_type=84;encode(parent,&par);rejected("parent raw type");par.raw_type=u[parent].raw_type;
 bad=good;par.carry-=1;encode(parent,&par);rejected("active queue shape");par.carry=u[parent].carry;
 bad=good;par.work_timer=255;encode(parent,&par);rejected("reserved timer shape");par.work_timer=u[parent].work_timer;
 int other=-1;for(int i=0;i<MAX_UNITS;i++)if(!u[i].alive){other=i;break;}assert(other>=0);
 bad=good;encode(other,&q);rejected("duplicate training");
 bad=good;{
   Unit inside=q;int c,t;Sc64ScarabChildFields f={(unsigned)parent,1,0,0};
   assert(sc64_scarab_child_fields_pack(&f,&c,&t));inside.carry=c;inside.work_timer=t;
   int added=0;for(int i=0;i<MAX_UNITS && added<10;i++)if(!u[i].alive){encode(i,&inside);added++;}
   assert(added==10);
 }rejected("overfull inside plus paid queue");
 bad=good;par.alive=0;encode(parent,&par);rejected("dead parent");
 assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_COMPLETED);valid_record();
 assert(race_scarab_launch_child(parent,0,child));valid_record();
 assert(race_save_decode_unit(&good,child,(unsigned)race_cols*16u,(unsigned)race_rows*16u,&q));
 bad=good;encode(other,&q);rejected("duplicate outside");
 bad=good;q.work_timer=0;encode(child,&q);rejected("expired outside child");
 assert(race_scarab_child_set(child,SC64_SCARAB_FIELD_NO_SLOT,SC64_SCARAB_CHILD_OUTSIDE,89,0));valid_record();
 /* Actual full actor pool: unrelated enemy bodies coexist with the detached
  * child and the saved parent, without imposing an artificial reserve. */
 while(spawn_unit(ENEMY,U_MARINE,500,500)>=0) {}
 for(int i=0;i<MAX_UNITS;i++) assert(u[i].alive);
 valid_record();
 puts("Scarab production-envelope candidate graph checks passed");return 0;
}
