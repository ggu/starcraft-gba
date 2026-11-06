/* SPDX-License-Identifier: GPL-3.0-or-later
 * Separate baseline producer / experimental consumer compatibility witness. */
#define main sc64_existing_mission_main
#ifdef SC64_RESEARCH_TEST_Z9
#include "../common/gba.c"
#include "sc64_z9_game.c"
#define SNAPSHOT_DIGEST() world_digest()
#else
#include "sc64_p10_mission.c"
#define SNAPSHOT_DIGEST() state_digest()
#endif
#undef main
#include <stdio.h>
#include <stdlib.h>
typedef struct {
 u32 digest,mode,mission,frame,weapon,armor,count;
 Sc64ImpactEvent impacts[8];
} Sc64CompatWitness;
static u8 sram_before[32768];
static void launch(int custom) {
 if(custom) {
  game_init(); race_mode=1;race_human=R_PROTOSS;race_bots=1;race_skirmish_map=0;
  race_skirmish_init();screen=SCREEN_PLAY;
 } else {
#ifdef SC64_RESEARCH_TEST_Z9
  launch_z9_from_normal_menu();
#else
  start_p10();
#endif
  game_update(0,0);
 }
}
static void snapshot(Sc64CompatWitness *w) {
 memset(w,0,sizeof(*w));w->digest=SNAPSHOT_DIGEST();w->mode=race_mode;w->mission=race_selected_mission;
 w->frame=frame;w->weapon=weapons_level;w->armor=armor_level;w->count=race_impact_count;
 assert(w->count==8);memcpy(w->impacts,race_impact_rows,sizeof(w->impacts));
}
static void put(const char *file,const void *buf,size_t n) {
 FILE *f=fopen(file,"wb");assert(f && fwrite(buf,1,n,f)==n && fclose(f)==0);
}
static void get(const char *file,void *buf,size_t n) {
 FILE *f=fopen(file,"rb");assert(f && fread(buf,1,n,f)==n && fgetc(f)==EOF && fclose(f)==0);
}
int main(int argc,char **argv) {
 assert(argc==4);int custom=atoi(argv[1]);assert(custom==0 || custom==1);gba_init();
#ifdef SC64_RESEARCH_COMPAT_PRODUCER
 launch(custom);memset(host_save,0,sizeof(host_save));weapons_level=1;armor_level=2;
 int target=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive){target=i;break;}assert(target>=0);
 race_impact_reset();assert(RACE_IMPACT_SAVE_CAPACITY>=8);
 for(unsigned i=0;i<8;i++)race_impact_launch(target,1,1,SIDE,0,U_DRAGOON,20+i);
 assert(race_impact_count==8);assert(race_save_game());
 assert(sram_get32(host_save)==SC64_TRANSPORT_SAVE_VERSION && valid_record(host_save,0,SC64_TRANSPORT_SAVE_VERSION));
 Sc64CompatWitness w;snapshot(&w);put(argv[2],host_save,sizeof(host_save));put(argv[3],&w,sizeof(w));
 printf("baseline producer mode=%d tag=%08x length=%u digest=%08x impacts=%u upgrades=%u/%u\n",custom,(unsigned)sram_get32(host_save),(unsigned)sram_get32(host_save+4),(unsigned)w.digest,(unsigned)w.count,(unsigned)w.weapon,(unsigned)w.armor);
#else
 Sc64CompatWitness expected,actual;game_init();get(argv[2],host_save,sizeof(host_save));get(argv[3],&expected,sizeof(expected));
 assert(sram_get32(host_save)==SC64_TRANSPORT_SAVE_VERSION && valid_record(host_save,0,SC64_TRANSPORT_SAVE_VERSION));
 memcpy(sram_before,host_save,sizeof(sram_before));assert(race_load_game());assert(!memcmp(sram_before,host_save,sizeof(sram_before)));
 snapshot(&actual);assert(!memcmp(&expected,&actual,sizeof(actual)));
 Sc64ProtossResearchLevels zero={0};assert(!memcmp(&zero,&sc64_experimental_research_levels,sizeof(zero)));
 printf("experimental consumer mode=%d digest=%08x impacts=%u upgrades=%u/%u SRAM unchanged\n",custom,(unsigned)actual.digest,(unsigned)actual.count,(unsigned)actual.weapon,(unsigned)actual.armor);
#endif
 return 0;
}
