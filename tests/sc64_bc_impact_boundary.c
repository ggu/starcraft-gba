/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_unused_main
#include "sc64_skirmish_runtime.c"
#undef main
static RaceImpactRecord saved;
int main(void)
{
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    int a=spawn_unit(ENEMY,unit_raw(12),500,500),t=spawn_unit(SIDE,unit_raw(0),640,500);
    int a2=spawn_unit(ENEMY,unit_raw(12),500,600),t2=spawn_unit(SIDE,unit_raw(0),640,600);
    assert(a>=0&&t>=0&&a2>=0&&t2>=0);
    u[a].work_timer=u[a2].work_timer=64<<3; u[a].carry=u[a2].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(a,t)&&race_bc_yamato_issue(a2,t2));
    if (!sc64_battlecruiser_source_tick_due()) ++frame;
    assert(race_bc_yamato_tick_caster(a)&&race_bc_yamato_tick_caster(a2));
    const unsigned base=sizeof(RaceSave)-SC64_BC_SAVE_EXTRA_BYTES;
    const unsigned rows=(RACE_IMPACT_RECORD_MAX-base-SC64_BC_SAVE_EXTRA_BYTES-sizeof(Sc64ImpactSaveHeader))/sizeof(Sc64ImpactEvent);
    unsigned limit=rows<RACE_IMPACT_CAPACITY?rows:RACE_IMPACT_CAPACITY;
    for(unsigned i=0;i<limit;++i)race_impact_launch(t,1,1,ENEMY,0,u[a].type,200);
    assert(race_impact_count==limit && race_save_game());
    u32 len=0; assert(save_read_latest(&saved,sizeof(saved),SAVE_VERSION,&len));
    if(limit<RACE_IMPACT_CAPACITY){
        race_impact_launch(t,1,1,ENEMY,0,u[a].type,200);
        assert(!race_save_game() && race_impact_count==limit+1);
        RaceImpactRecord check;u32 len2=0;
        assert(save_read_latest(&check,sizeof(check),SAVE_VERSION,&len2));
        assert(len2==len&&!memcmp(&saved,&check,len));
    }
    printf("base=%u both-shot impact capacity=%u runtime=%u\n",base,limit,RACE_IMPACT_CAPACITY);
    return 0;
}
