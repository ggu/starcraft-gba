/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real previous feature5 HOST writer; constructed private SRAM only. */
#include <assert.h>
#include <stdio.h>
#include SC64_PRIOR_GAME
#include SC64_PRIOR_SCARAB_ADAPTER
extern void guardian_test_reset(void);
extern void guardian_test_snapshot(void *);
extern void guardian_test_restore(const void *);
static unsigned char sram[32768];
int main(int argc,char **argv)
{
    assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fread(sram,1,sizeof(sram),f)==sizeof(sram));assert(!fclose(f));
    guardian_test_reset();game_init();guardian_test_restore(sram);assert(race_load_game());
    int p=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].type==U_REAVER && u[i].x==300)p=i;
    assert(p>=0);minerals=100;assert(race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED && race_save_game());
    RaceLoadPlan plan;GbaSaveSelection selected;assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_ACTOR_TRANSPORT_SAVE_VERSION &&
           plan.egg_auth.features==(SC64_STAGE_SAVE_EGG|SC64_STAGE_SAVE_SCARAB));
    guardian_test_snapshot(sram);f=fopen(argv[2],"wb");assert(f);
    assert(fwrite(sram,1,sizeof(sram),f)==sizeof(sram));assert(!fclose(f));
    puts("genuine feature5 HOST writer created paid Scarab journal");return 0;
}
