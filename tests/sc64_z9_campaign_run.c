/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary-input Z9 "The Invasion of Aiur" campaign controller.
 * The controller may inspect live state to choose targets; every mutation is
 * an ordinary HOME/menu/gameplay input delivered through game_update(). */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include "sc64_zerg_late_controller.h"
#include <assert.h>

#ifndef SC64_Z9_RUNTIME
#error "Compile this controller with the zerg-z9 profile flags"
#endif
#ifndef Z9_RUN_LIMIT
#define Z9_RUN_LIMIT 200000u
#endif
#ifdef Z9_SWEEP
static int knob(const char*n,int d){const char*v=getenv(n);return v?atoi(v):d;}
#else
#define knob(n,d) (d)
#endif

enum { HOME_X=288, HOME_Y=152, RAW_NEXUS=154, RAW_CRYSTAL=219 };

/* The only way out of the home plateau, past the first town and down the
 * middle of the map to the crystal formation. */
static const int road[][2]={{640,260},{760,280},{900,264},{1040,280},{1056,400},{1050,520},
    {1000,620},{968,760},{904,860},{904,1000},{904,1170},{904,1336}};
enum { ROAD_LEGS=sizeof(road)/sizeof(road[0]) };

static int crystal(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==RAW_CRYSTAL)return i;
    return -1;
}

static int crystal_drone(void)
{
    int c=crystal();
    if(c<0)return -1;
    if(race_zl_crystal_carrier!=SC64_ZL_NO_CARRIER)return race_zl_crystal_carrier;
    return -1;
}

static int hydras_near(int x,int y,int r)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&dist2(u[i].x,u[i].y,x,y)<r*r;
    return n;
}

static void census(const char *label,int leg)
{
    int enemy=0,eb=0;
    for(int i=0;i<MAX_BUILDINGS;i++)eb+=b[i].alive&&b[i].owner==ENEMY;
    enemy=count_units(ENEMY,-1);
    fprintf(trace,"{\"kind\":\"checkpoint\",\"label\":\"%s\",\"frame\":%u,\"call_index\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,"
        "\"side_units\":%d,\"hydras\":%d,\"drones\":%d,\"enemy_units\":%d,\"enemy_buildings\":%d,\"leg\":%d,\"bank\":[%d,%d],"
        "\"supply\":[%d,%d],\"cursor\":[%d,%d],\"countdown\":%u,\"towns\":%u,\"fault\":%u}\n",
        label,(unsigned)frame,host_tick,screen,won,lost,count_units(SIDE,-1),count_units(SIDE,U_HYDRALISK),drone_count(),
        enemy,eb,leg,minerals,gas,race_owner_supply(SIDE),race_owner_supply_cap(SIDE),cx,cy,
        (unsigned)race_zl_sched.countdown,(unsigned)race_zl_ai.started,(unsigned)race_zl_sched.core.fault);
    fflush(trace);
}

int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:".local/traces/z9-controller.jsonl";
    unsigned limit_ticks=argc>2?(unsigned)strtoul(argv[2],0,10):Z9_RUN_LIMIT;
    trace=fopen(path,"w");if(!trace){perror(path);return 2;}
    fprintf(trace,"{\"kind\":\"metadata\",\"schema\":1,\"mission\":%d,\"profile\":\"ZERG_Z9_CANONICAL_FLAGS\",\"input_policy\":\"ordinary game_update held/pressed only\"}\n",SC64_Z9_GLOBAL_ID);
    game_init();prior=0;
    launch_catalog_mission(SC64_Z9_GLOBAL_ID);
    assert(screen==SCREEN_PLAY&&!won&&!lost&&race_selected_mission==SC64_Z9_GLOBAL_ID);
    census("launch",0);
    int saved=0,turn=0,leg=0,rush=0,next_checkpoint=2000,next_mine=0,next_train=0,next_build=100,next_gas=400,next_hatch=600,next_rally=1500,next_order=0,next_save_build=0,next_stream=0,next_runner=0,runner=-1,runner_leg=0,taken=0,next_defend=0;
    const int drone_goal=knob("Z9_DRONES",16),hatch_goal=knob("Z9_HATCH",7),gas_drones=3,hydra_cap=knob("Z9_CAP",110);
    while(host_tick<limit_ticks&&screen==SCREEN_PLAY&&!won&&!lost){
#ifdef Z9_SWEEP
        {static int lastf,lastt;if(getenv("Z9_SLOW")&&frame-lastf>=atoi(getenv("Z9_SLOW")))fprintf(stderr,"SLOW f%d..%u turn %d->%d cur(%d,%d)\n",lastf,(unsigned)frame,lastt,turn,cx,cy);lastf=frame;lastt=turn;}
#endif
        if(menu_open)KEY_TAP(KEY_B);
        int mark=frame,ax=HOME_X,ay=HOME_Y;
        int carrier=crystal_drone();
        /* The crystal's Drone runs home along the road, however the army fares. */
        if(carrier>=0){
            int best=-1,bd=0x7fffffff;
            for(int k=0;k<ROAD_LEGS;k++){int d=dist2(u[carrier].x,u[carrier].y,road[k][0],road[k][1]);if(d<bd){bd=d;best=k;}}
            int tx=best>0?road[best-1][0]:264,ty=best>0?road[best-1][1]:240;
            if(best==0&&dist2(u[carrier].x,u[carrier].y,264,240)<40*40){tx=264;ty=240;}
            if(u[carrier].order==ORDER_IDLE||dist2(u[carrier].tx,u[carrier].ty,tx,ty)>24*24){
                (void)unit_order(carrier,MENU_MOVE,tx,ty);close_menus();
            }
            input(0);continue;
        }
        /* The army advances along the road, leg by leg. */
        if(frame>=next_order){
            next_order=frame+150;
            int home=hydras_near(road[0][0],road[0][1],300);
            if(!rush&&home>=knob("Z9_GO",50)&&frame>=knob("Z9_GOF",8000)){rush=1;fprintf(trace,"{\"kind\":\"assault\",\"frame\":%u,\"army\":%d}\n",(unsigned)frame,home);}
            if(rush){
                int n=0,sx=0,sy=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&dist2(u[i].x,u[i].y,HOME_X,HOME_Y)>=300*300){
                    int c=0,x2=0,y2=0;
                    for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&u[k].type==U_HYDRALISK&&dist2(u[k].x,u[k].y,u[i].x,u[i].y)<240*240){++c;x2+=u[k].x;y2+=u[k].y;}
                    if(c>n){n=c;sx=x2;sy=y2;}
                }
                if(n<knob("Z9_RETREAT",12)){rush=0;leg=0;}
                else {
                    int gx,gy,ax2=sx/n,ay2=sy/n;
                    while(leg<ROAD_LEGS-1){
                        int there=hydras_near(road[leg][0],road[leg][1],170);
                        if(there*100<n*knob("Z9_LEGP",50))break;
                        ++leg;
                    }
                    gx=road[leg][0];gy=road[leg][1];
                    group_area_x=ax2;group_area_y=ay2;group_area_r=320;group_rounds=8;
                    (void)group_command(U_HYDRALISK,MENU_ATTACK,gx,gy,0);
                    group_area_x=-1;group_rounds=6;
                    fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"leg\":%d,\"army\":%d,\"at\":[%d,%d]}\n",(unsigned)frame,gx,gy,leg,n,ax2,ay2);
                    next_order=frame+knob("Z9_ORD",300);
                }
            }
        }
        if(frame-mark>=20){input(0);continue;}
#define COST(n) if(frame-mark>=20){turn=n;input(0);continue;} case n:;
        switch(turn){ case 0:;
        if(!saved&&frame>=knob("Z9_SAVE",6000)){
            int pick=free_drone(-1,-1);
            if(under_construction()&&pick>=0){
                if(pause_menu_save(path,pick)){saved=1;census("saved",leg);}
            } else if(frame>=next_save_build&&minerals>=75){
                (void)build_near(B_CREEP_COLONY,MENU_Z7_CREEP,ax-40,ay+110,96);
                next_save_build=frame+300;
            }
        }
        COST(10)
        /* Whatever is at home answers anything that comes for the hives. */
        if(frame>=next_defend){
            int foe=-1,fd=460*460;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&sc64_race_unit_damage(u[i].type)>0){
                int d=dist2(u[i].x,u[i].y,HOME_X,HOME_Y+60);
                if(d<fd){fd=d;foe=i;}
            }
            if(foe>=0&&!rush){
                group_area_x=ax;group_area_y=ay+60;group_area_r=560;group_rounds=2;
                (void)group_command(U_HYDRALISK,MENU_ATTACK,u[foe].x,u[foe].y,1);
                group_area_x=-1;group_rounds=6;
            }
            next_defend=frame+knob("Z9_DEF",200);
        }
        COST(1)
        if(frame>=next_mine){(void)order_idle_drones(3);next_mine=frame+400;}
        COST(2)
        if(frame>=next_train){
            int done=0,hatch=-1;
            for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&sc64_race_is_zerg_townhall(b[i].type)&&dist2(b[i].x,b[i].y,HOME_X,HOME_Y)<64*64)hatch=i;
            for(int i=0;i<MAX_BUILDINGS;i++){
                if(!b[i].alive||b[i].owner!=SIDE||!sc64_race_is_zerg_townhall(b[i].type)||!b[i].larva||
                   !building_complete(i)||dist2(b[i].x,b[i].y,ax,ay)>=500*500)continue;
                while(b[i].larva>0){
                    int spare=race_owner_supply_cap(SIDE)-race_owner_supply(SIDE),type=-1;
                    if(drone_count()+queued_units(U_DRONE)<drone_goal&&minerals>=50)type=U_DRONE;
                    else if(spare<14&&minerals>=100&&queued_units(U_OVERLORD)<3)type=U_OVERLORD;
                    else if(count_buildings(B_HYDRALISK_DEN,1)&&minerals>=75&&gas>=25&&spare>=2&&count_units(SIDE,U_HYDRALISK)<hydra_cap)type=U_HYDRALISK;
                    else if(count_buildings(B_SPAWNING_POOL,1)&&minerals>=150&&frame>=2000&&spare>=2&&(!count_buildings(B_HYDRALISK_DEN,1)||gas<25))type=U_ZERGLING;
#ifdef Z9_SWEEP
                    {int ok=type>=0?train_at(i,type):-1;if(getenv("Z9_DUMP"))fprintf(stderr,"T f%u h%d lv%d type%d ok%d spare%d bank%d/%d sel%d tt%d drones%d q%d\n",(unsigned)frame,i,b[i].larva,type,ok,spare,minerals,gas,selected_building,train_type,drone_count(),queued_units(U_DRONE));if(ok<=0)break;}
#else
                    if(type<0||!train_at(i,type))break;
#endif
                    ++done;
                }
            }
            (void)hatch;
            next_train=frame+(done?60:120);
        }
        COST(3)
        if(frame>=next_build){
            if(!count_buildings(B_HYDRALISK_DEN,0)&&minerals>=100&&gas>=50&&drone_count()>=5)
                (void)build_near(B_HYDRALISK_DEN,MENU_ACADEMY,ax-90,ay+60,128);
            else if(!count_buildings(B_EXTRACTOR,0)&&minerals>=50&&drone_count()>=6)
                (void)build_extractor_near(416,240);
            next_build=frame+240;
        }
        COST(4)
        if(frame>=next_hatch&&minerals>=knob("Z9_HMIN",500)&&drone_count()>=6&&count_buildings(B_HATCHERY,0)<hatch_goal){
            static const int spots[][2]={{200,60},{110,120},{90,240},{180,300},{300,320},{420,340}};
            int k=count_buildings(B_HATCHERY,0)-1;
            if(k>=0&&k<6)(void)build_near(B_HATCHERY,MENU_DEPOT,spots[k][0],spots[k][1],120);
            next_hatch=frame+300;
        }
        COST(5)
        if(frame>=next_gas){(void)assign_gas_drone(gas_drones);next_gas=frame+600;}
        COST(6)
        if(frame>=next_rally&&!rush){
            group_area_x=ax;group_area_y=ay+60;group_area_r=480;group_rounds=4;
            /* Stand between the hives and the way in until the army is big enough
               to leave; Scouts come over the cliffs. */
            int forward=count_units(SIDE,U_HYDRALISK)>=knob("Z9_FWD",40);
            int rx=forward?road[0][0]:knob("Z9_RX",360),ry=forward?road[0][1]:knob("Z9_RY",250);
            (void)group_command(U_HYDRALISK,MENU_ATTACK,rx,ry,0);
            (void)group_command(U_ZERGLING,MENU_ATTACK,rx,ry,0);
            group_area_x=-1;group_rounds=6;
            next_rally=frame+500;
        }
        COST(7)
        /* Fresh Hydralisks follow the army up the road in stages. */
        if(rush&&frame>=next_stream){
            group_area_x=ax;group_area_y=ay+60;group_area_r=480;group_rounds=3;
            (void)group_command(U_HYDRALISK,MENU_ATTACK,road[0][0],road[0][1],0);
            for(int k=0;k+1<=leg&&k<ROAD_LEGS-1;k+=3){
                group_area_x=road[k][0];group_area_y=road[k][1];group_area_r=120;group_rounds=2;
                (void)group_command(U_HYDRALISK,MENU_ATTACK,road[k+3<leg?k+3:leg][0],road[k+3<leg?k+3:leg][1],0);
            }
            group_area_x=-1;group_rounds=6;
            next_stream=frame+knob("Z9_STREAM",800);
        }
        COST(8)
        /* Once the army holds the middle of the road, a Drone runs past it to
           the formation. */
        if(rush&&leg>=knob("Z9_RUN",ROAD_LEGS-1)&&!race_zl_sched.countdown&&crystal()<0&&!taken&&frame>=next_runner){
            if(runner<0||!u[runner].alive||u[runner].type!=U_DRONE){runner=free_drone(-1,-1);runner_leg=0;}
            if(runner>=0){
                while(runner_leg<ROAD_LEGS&&dist2(u[runner].x,u[runner].y,road[runner_leg][0],road[runner_leg][1])<60*60)++runner_leg;
                int tx=runner_leg<ROAD_LEGS?road[runner_leg][0]:960,ty=runner_leg<ROAD_LEGS?road[runner_leg][1]:1430;
                (void)unit_order(runner,MENU_MOVE,tx,ty);close_menus();
            }
            next_runner=frame+200;
        }
        COST(9)
        }
        turn=0;
#ifdef Z9_SWEEP
        if(getenv("Z9_DUMP")&&frame>=next_checkpoint){
            int nh=0,nl=0,nq=0,nun=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&sc64_race_is_zerg_townhall(b[i].type)){nh++;nl+=b[i].larva;nq+=b[i].queue_count;nun+=!building_complete(i);}
            int mine=0,gasd=0,idle=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE){mine+=u[i].order==ORDER_MINE;gasd+=u[i].order==ORDER_GAS;idle+=u[i].order==ORDER_IDLE;}
            fprintf(stderr,"f%u bank %d/%d HATCH n%d larva%d q%d uc%d den%d/%d pool%d/%d ext%d drones m%d g%d i%d enemy_near_home %d\n",(unsigned)frame,minerals,gas,nh,nl,nq,nun,count_buildings(B_HYDRALISK_DEN,1),count_buildings(B_HYDRALISK_DEN,0),count_buildings(B_SPAWNING_POOL,1),count_buildings(B_SPAWNING_POOL,0),count_buildings(B_EXTRACTOR,0),mine,gasd,idle,({int e=0;for(int i=0;i<MAX_UNITS;i++)e+=u[i].alive&&u[i].owner==ENEMY&&dist2(u[i].x,u[i].y,HOME_X,HOME_Y)<420*420;e;}));
        }
#endif
        if(frame>=next_checkpoint){census("progress",leg);next_checkpoint=frame+2000;}
        input(0);
    }
    census("final",leg);
    fclose(trace);
    printf("z9 controller: frame=%u calls=%u won=%d lost=%d screen=%d fault=%u drones=%d hydras=%d leg=%d countdown=%u crystal=%d\n",
        (unsigned)frame,host_tick,won,lost,screen,(unsigned)race_zl_sched.core.fault,drone_count(),count_units(SIDE,U_HYDRALISK),leg,(unsigned)race_zl_sched.countdown,crystal()>=0);
    return won?0:1;
}
