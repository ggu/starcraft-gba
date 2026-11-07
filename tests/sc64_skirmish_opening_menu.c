/* Normal-menu skirmish openings in each product and every lobby combination. */
#ifndef SC64_IMPORTED_SKIRMISH
#define SC64_SKIRMISH_IMPORTED_COUNT 0
#endif
#define main sc64_skirmish_controller_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main
#include <assert.h>

static void check_owner(int owner, int race)
{
    int workers=0, halls=0, extra=0, overlords=0;
    int worker=race==R_TER?U_SCV:race==R_ZERG?U_DRONE:U_PROBE;
    int hall=race==R_TER?B_COMMAND:race==R_ZERG?B_HATCHERY:B_NEXUS;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==owner) {
        if(u[i].type==worker) { ++workers; assert(u[i].order==ORDER_MINE); }
        if(u[i].type==U_OVERLORD) ++overlords;
    }
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive && b[i].owner==owner) {
        if(b[i].type==hall) ++halls;
        if(b[i].type==B_DEPOT || b[i].type==B_PYLON) ++extra;
    }
    assert(workers==4 && halls==1 && extra==0 && overlords==(race==R_ZERG));
    assert(race_owner_supply(owner)==8);
    assert(race_owner_supply_cap(owner)==(race==R_TER?20:18));
    assert((owner==SIDE?minerals:race_minerals[owner])==50);
    assert((owner==SIDE?gas:race_gas[owner])==0);
}
int main(void)
{
    int cases=0;
    for(option_map=0;option_map<SC64_FRONT_MAP_COUNT;++option_map)
      for(option_human=R_TER;option_human<=R_PROTOSS;++option_human)
       for(option_bots=1;option_bots<=3;++option_bots) {
        int combinations=option_bots==1?3:option_bots==2?9:27;
        for(int c=0;c<combinations;++c) {
            int value=c;for(int i=0;i<option_bots;++i){option_bot_races[i]=value%3;value/=3;}
            gba_init();assert(launch_match());check_owner(SIDE,option_human);
            for(int i=0;i<option_bots;++i)check_owner(race_bot_owner_ids[i],option_bot_races[i]);
            ++cases;
        }
       }
    assert(cases==117 * SC64_FRONT_MAP_COUNT);printf("Normal-menu skirmish openings: %d configurations passed\n",cases);
    return 0;
}
