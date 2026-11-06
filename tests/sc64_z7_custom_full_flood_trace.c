/* SPDX-License-Identifier: GPL-3.0-or-later
 * Bounded HOST custom games launched through ordinary controller menus.
 * Fingerprints are diagnostic; they are not complete-state proof. */
#include "../common/gba.c"
#define main skirmish_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main
#include <stdint.h>

static uint64_t digest(uint64_t h,const void *p,size_t n)
{
    const u8 *b=p;
    while(n--) h=(h^*b++)*UINT64_C(1099511628211);
    return h;
}
#define ADD(h, x) ((h)=digest((h),&(x),sizeof(x)))

int main(int argc,char **argv)
{
    int map,bots,initial_frame;
    if(argc!=3) return 2;
    map=atoi(argv[1]); bots=atoi(argv[2]);
    if(map<0 || map>1 || (bots!=1 && bots!=3)) return 2;
    option_map=map; option_human=R_ZERG; option_bots=bots; match_limit=10000;
    for(int i=0;i<bots;i++) option_bot_races[i]=(R_ZERG+1+i)%3;
    gba_init();
    if(!launch_match()) {fputs("normal custom launch failed\n",stderr);return 3;}
    if(race_skirmish_map!=map || race_human!=R_ZERG || race_bots!=bots) return 4;
    for(int i=0;i<bots;i++)
        if(race_bot_races[i]!=option_bot_races[i]) return 5;
    initial_frame=frame;
    printf("identity map=%d human=%d bots=%d owners=",map,race_human,bots);
    for(int i=0;i<bots;i++) printf("%s%d:%d",i?",":"",race_bot_owner_ids[i],race_bot_races[i]);
    putchar('\n');
    for(int step=1;step<=4000;step++) {
        uint64_t actors=UINT64_C(14695981039346656037), world=actors, display=actors, route=actors;
        if(screen!=SCREEN_PLAY || won || lost) {fprintf(stderr,"unexpected end step=%d screen=%d\n",step,screen);return 6;}
        input(0); audio_update(); game_render();
        if(step%100 != 0 && step!=1) continue;
        ADD(actors,u); ADD(actors,b); ADD(actors,m); ADD(actors,geysers); ADD(actors,tracers);
        ADD(world,race_campaign); ADD(world,visible); ADD(world,explored); ADD(world,seed);
        ADD(world,minerals); ADD(world,gas); ADD(world,supply_used); ADD(world,supply_cap);
        ADD(world,race_player); ADD(world,race_minerals); ADD(world,race_gas);
        ADD(world,race_bot_races); ADD(world,race_bot_owner_ids); ADD(world,race_skirmish_map);
        ADD(world,path_walkable);
        ADD(route,sc64_route_valid); ADD(route,sc64_route_goal); ADD(route,sc64_route_age);
        ADD(route,sc64_route_clock); ADD(route,sc64_route_field);
        ADD(display,host_fb[front]); ADD(display,host_palette);
        printf("step=%d frame=%d units=%d buildings=%d minerals=%d gas=%d actor=%016llx world=%016llx route=%016llx display=%016llx",
            step,frame,count_units(SIDE,-1),count_buildings(SIDE,-1),minerals,gas,
            (unsigned long long)actors,(unsigned long long)world,(unsigned long long)route,(unsigned long long)display);
        for(int i=0;i<bots;i++) {
            int owner=race_bot_owner_ids[i];
            printf(" bot%d=%d:%d:%d:%d",owner,count_units(owner,-1),count_buildings(owner,-1),
                   race_owner_minerals(owner),race_owner_gas(owner));
        }
        putchar('\n');
    }
    if(frame-initial_frame < 3900) {fputs("insufficient advancing custom gameplay\n",stderr);return 7;}
    return 0;
}
#undef ADD
