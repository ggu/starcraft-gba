/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary-input Z10 "Full Circle" campaign controller.
 * The controller may inspect live state to choose targets; every mutation is
 * an ordinary HOME/menu/gameplay input delivered through game_update(). */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include "sc64_zerg_late_controller.h"
#include <assert.h>

#ifndef SC64_Z10_RUNTIME
#error "Compile this controller with the zerg-z10 profile flags"
#endif
#ifndef Z10_RUN_LIMIT
#define Z10_RUN_LIMIT 200000u
#endif
#ifdef Z10_SWEEP
static int knob(const char*n,int d){const char*v=getenv(n);return v?atoi(v):d;}
#else
#define knob(n,d) (d)
#endif

enum { HOME_X=704, HOME_Y=1896, RAW_TEMPLE=174, RAW_CRYSTAL=219, RAW_CANNON=162 };

/* The west road round the central massif, from the hive to the Temple. */
static const int road[][2]={{560,1800},{424,1640},{408,1432},{408,1208},{488,1064},{568,920},{632,760},{664,600},{700,520}};
enum { ROAD_LEGS=sizeof(road)/sizeof(road[0]) };

static int crystal(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==RAW_CRYSTAL)return i;
    return -1;
}

static int temple(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==RAW_TEMPLE)return i;
    return -1;
}

static int nearest_leg(int x,int y)
{
    int best=0,bd=0x7fffffff;
    for(int k=0;k<ROAD_LEGS;k++){int d=dist2(x,y,road[k][0],road[k][1]);if(d<bd){bd=d;best=k;}}
    return best;
}

static int army_type(int type){return type==U_HYDRALISK||type==U_MUTALISK;}
#define Z10_READY(ground,air,live,limit,goal) ((air)>=10 && ((ground)+(air)>=(goal) || ((ground)+(air)>=40 && (live)>=(limit)-1)))
_Static_assert(Z10_READY(30,10,240,240,70), "mixed force can launch at pool capacity");
_Static_assert(!Z10_READY(30,9,240,240,70), "air minimum remains required");
_Static_assert(!Z10_READY(30,10,238,240,70), "pool-limited launch requires pool pressure");

static int army_near(int x,int y,int r)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&army_type(u[i].type)&&dist2(u[i].x,u[i].y,x,y)<r*r;
    return n;
}

/* The nearest Photon Cannon that reaches the road and is within reach of the army. */
static int road_cannon(int ax,int ay,int reach)
{
    int best=-1,bd=reach*reach;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&b[i].raw_type==RAW_CANNON){
        int d=dist2(b[i].x,b[i].y,ax,ay);
        if(d>=bd)continue;
        int close=0;
        for(int k=0;k+1<ROAD_LEGS&&!close;k++)for(int t=0;t<=4&&!close;t++){
            int x=road[k][0]+(road[k+1][0]-road[k][0])*t/4,y=road[k][1]+(road[k+1][1]-road[k][1])*t/4;
            close=dist2(b[i].x,b[i].y,x,y)<knob("Z10_CR",150)*knob("Z10_CR",150);
        }
        if(close){bd=d;best=i;}
    }
    return best;
}

static void census(const char *label,int leg)
{
    int eb=0;
    for(int i=0;i<MAX_BUILDINGS;i++)eb+=b[i].alive&&b[i].owner==ENEMY;
    fprintf(trace,"{\"kind\":\"checkpoint\",\"label\":\"%s\",\"frame\":%u,\"call_index\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,"
        "\"side_units\":%d,\"hydras\":%d,\"mutalisks\":%d,\"drones\":%d,\"enemy_units\":%d,\"enemy_buildings\":%d,\"leg\":%d,\"bank\":[%d,%d],"
        "\"supply\":[%d,%d],\"cursor\":[%d,%d],\"countdown\":0,\"towns\":%u,\"fault\":%u}\n",
        label,(unsigned)frame,host_tick,screen,won,lost,count_units(SIDE,-1),count_units(SIDE,U_HYDRALISK),count_units(SIDE,U_MUTALISK),drone_count(),
        count_units(ENEMY,-1),eb,leg,minerals,gas,race_owner_supply(SIDE),race_owner_supply_cap(SIDE),cx,cy,
        (unsigned)race_zl_ai.started,(unsigned)race_zl_sched.core.fault);
    fflush(trace);
}

int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:".local/traces/z10-controller.jsonl";
    unsigned limit_ticks=argc>2?(unsigned)strtoul(argv[2],0,10):Z10_RUN_LIMIT;
    trace=fopen(path,"w");if(!trace){perror(path);return 2;}
    fprintf(trace,"{\"kind\":\"metadata\",\"schema\":1,\"mission\":%d,\"profile\":\"ZERG_Z10_CANONICAL_FLAGS\",\"input_policy\":\"ordinary game_update held/pressed only\"}\n",SC64_Z10_GLOBAL_ID);
    game_init();prior=0;
    launch_catalog_mission(SC64_Z10_GLOBAL_ID);
    assert(screen==SCREEN_PLAY&&!won&&!lost&&race_selected_mission==SC64_Z10_GLOBAL_ID);
    census("launch",0);
    int saved=0,turn=0,leg=0,rush=0,next_checkpoint=2000,next_mine=0,next_train=0,next_build=100,next_gas=400,next_hatch=100,next_rally=1500,next_order=0,next_save_build=0,next_stream=0,next_defend=0,next_runner=0,runner=-1;
    const int drone_goal=knob("Z10_DRONES",16),hatch_goal=knob("Z10_HATCH",7),gas_drones=3,hydra_cap=knob("Z10_CAP",110);
    while(host_tick<limit_ticks&&screen==SCREEN_PLAY&&!won&&!lost){
#ifdef Z10_SWEEP
        if(getenv("Z10_DUMP")&&frame>=next_checkpoint){
            int nh=0,nl=0,nq=0,nun=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&sc64_race_is_zerg_townhall(b[i].type)){nh++;nl+=b[i].larva;nq+=b[i].queue_count;nun+=!building_complete(i);}
            int mine=0,gasd=0,idle=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE){mine+=u[i].order==ORDER_MINE;gasd+=u[i].order==ORDER_GAS;idle+=u[i].order==ORDER_IDLE;}
            int ne[256]={0};for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&dist2(u[i].x,u[i].y,HOME_X,HOME_Y)<520*520)ne[u[i].raw_type&255]++;
            fprintf(stderr,"f%u bank %d/%d HATCH n%d larva%d q%d uc%d den%d/%d pool%d/%d ext%d drones m%d g%d i%d foes:",(unsigned)frame,minerals,gas,nh,nl,nq,nun,count_buildings(B_HYDRALISK_DEN,1),count_buildings(B_HYDRALISK_DEN,0),count_buildings(B_SPAWNING_POOL,1),count_buildings(B_SPAWNING_POOL,0),count_buildings(B_EXTRACTOR,0),mine,gasd,idle);
            for(int k=0;k<256;k++)if(ne[k])fprintf(stderr," r%d:%d",k,ne[k]);fprintf(stderr,"\n");
        }
        {static int ll;if(getenv("Z10_ARMY")&&rush&&frame>=ll){ll=frame+atoi(getenv("Z10_ARMY"));int gx=road[leg][0],gy=road[leg][1];int ord[24]={0},t=0,nl=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK){ord[u[i].order&23]++;t++;nl+=dist2(u[i].x,u[i].y,gx,gy)<300*300;}
        fprintf(stderr,"A f%u leg%d hy%d nearleg%d temple%d o:",(unsigned)frame,leg,t,nl,temple()>=0?b[temple()].hp:-1);for(int k=0;k<24;k++)if(ord[k])fprintf(stderr," %d:%d",k,ord[k]);
        int ne[256]={0};for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&dist2(u[i].x,u[i].y,gx,gy)<340*340)ne[u[i].raw_type&255]++;fprintf(stderr," foes:");for(int k=0;k<256;k++)if(ne[k])fprintf(stderr," r%d:%d",k,ne[k]);fprintf(stderr,"\n");}}
#endif
        if(menu_open)KEY_TAP(KEY_B);
        int mark=frame,ax=HOME_X,ay=HOME_Y;
        int c=crystal(),carrier=c>=0&&race_zl_crystal_carrier!=SC64_ZL_NO_CARRIER?race_zl_crystal_carrier:-1;
        /* The crystal goes up the road to the Temple's site. */
        if(carrier>=0){
            int at=nearest_leg(u[carrier].x,u[carrier].y);
            int to=at+1,tx,ty;
            if(to<ROAD_LEGS){tx=road[to][0];ty=road[to][1];}else{tx=road[ROAD_LEGS-1][0];ty=road[ROAD_LEGS-1][1];}
            if(at==ROAD_LEGS-1){tx=712;ty=506;}
            if(u[carrier].order==ORDER_IDLE||dist2(u[carrier].tx,u[carrier].ty,tx,ty)>24*24){
                (void)unit_order(carrier,MENU_MOVE,tx,ty);close_menus();
            }
        }
        /* A dropped crystal: another Drone fetches it. */
        if(c>=0&&carrier<0&&frame>=next_runner){
            if(runner<0||!u[runner].alive||u[runner].type!=U_DRONE)runner=free_drone(-1,-1);
            if(runner>=0){(void)unit_order(runner,MENU_MOVE,u[c].x,u[c].y);close_menus();}
            next_runner=frame+200;
        }
        if(frame-mark>=20){input(0);continue;}
        if(frame>=next_order){
            next_order=frame+150;
            int ground=count_units(SIDE,U_HYDRALISK), air=count_units(SIDE,U_MUTALISK), home=ground+air;
            int live=0;for(int i=0;i<MAX_UNITS;i++)live+=u[i].alive!=0;
            /* Attack when the shared actor pool prevents further recruitment. */
            if(!rush&&Z10_READY(ground,air,live,MAX_UNITS,(frame<knob("Z10_L2",30000)?knob("Z10_GO",70):knob("Z10_GO2",55)))&&frame>=knob("Z10_GOF",8000)){rush=1;leg=0;fprintf(trace,"{\"kind\":\"assault\",\"frame\":%u,\"army\":%d,\"mutalisks\":%d}\n",(unsigned)frame,home,count_units(SIDE,U_MUTALISK));}
            if(rush){
                int n=0,sx=0,sy=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&army_type(u[i].type)){
                    int k2=0,x2=0,y2=0;
                    for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&army_type(u[k].type)&&dist2(u[k].x,u[k].y,u[i].x,u[i].y)<240*240){++k2;x2+=u[k].x;y2+=u[k].y;}
                    if(k2>n){n=k2;sx=x2;sy=y2;}
                }
                if(n<knob("Z10_RETREAT",12)){rush=0;leg=0;}
                else {
                    int ax2=sx/n,ay2=sy/n,nearest=nearest_leg(ax2,ay2);
                    if(nearest<leg-2)leg=nearest;
                    int cannon=road_cannon(ax2,ay2,knob("Z10_CREACH",220));
                    while(leg<ROAD_LEGS-1&&cannon<0){
                        int there=army_near(road[leg][0],road[leg][1],170);
                        if(there*100<n*knob("Z10_LEGP",50))break;
                        ++leg;
                    }
                    int to=leg<nearest+1?leg:nearest+1;
                    int gx=road[to][0],gy=road[to][1],force=0;
                    if(cannon>=0){gx=b[cannon].x;gy=b[cannon].y;force=1;}
                    /* A Reaver's splash spends a crowd fastest: shoot those first. */
                    {int rd=knob("Z10_RV",300),reaver=-1;rd*=rd;
                     for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==83&&dist2(u[i].x,u[i].y,road[to][0],road[to][1])<260*260){int dd=dist2(u[i].x,u[i].y,ax2,ay2);if(dd<rd){rd=dd;reaver=i;}}
                     if(reaver>=0){gx=u[reaver].x;gy=u[reaver].y;force=1;}}
                    /* In reach, the Temple is the only target that matters. */
                    int t=temple();
                    if(t>=0&&dist2(ax2,ay2,b[t].x,b[t].y)<knob("Z10_TR",240)*knob("Z10_TR",240)){gx=b[t].x;gy=b[t].y;force=1;}
                    group_area_x=ax2;group_area_y=ay2;group_area_r=320;group_rounds=8;
                    (void)group_command(U_HYDRALISK,MENU_ATTACK,gx,gy,force);
                    (void)group_command(U_MUTALISK,MENU_ATTACK,gx,gy,force);
                    group_area_x=-1;group_rounds=6;
                    fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"leg\":%d,\"army\":%d,\"at\":[%d,%d],\"mutalisks\":%d}\n",(unsigned)frame,gx,gy,leg,n,ax2,ay2,count_units(SIDE,U_MUTALISK));
                    next_order=frame+knob("Z10_ORD",300);
                }
            }
        }
        if(frame-mark>=20){input(0);continue;}
#define COST(n) if(frame-mark>=20){turn=n;input(0);continue;} case n:;
        switch(turn){ case 0:;
        if(!saved&&frame>=knob("Z10_SAVE",6000)){
            int pick=free_drone(-1,-1);
            if(under_construction()&&pick>=0){
                if(clear_menu_last(pick)&&pause_menu_save(path,pick)){saved=1;census("saved",leg);}
            } else if(frame>=next_save_build&&minerals>=75){
                (void)build_near(B_CREEP_COLONY,MENU_Z7_CREEP,ax-60,ay-90,96);
                next_save_build=frame+300;
            }
        }
        COST(10)
        /* Whatever is at home answers anything that comes for the hives. */
        if(frame>=next_defend){
            int foe=-1,fd=knob("Z10_DEFR",330)*knob("Z10_DEFR",330);
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&sc64_race_unit_damage(u[i].type)>0){
                int d=dist2(u[i].x,u[i].y,HOME_X,HOME_Y-60);
                if(d<fd){fd=d;foe=i;}
            }
            if(foe>=0&&!rush){
                group_area_x=ax;group_area_y=ay-60;group_area_r=600;group_rounds=2;
                (void)group_command(U_HYDRALISK,MENU_ATTACK,u[foe].x,u[foe].y,1);
                group_area_x=-1;group_rounds=6;
            }
            next_defend=frame+200;
        }
        COST(1)
        /* The unit pool fills before the bank empties: no need to chase idle
           Drones across the field once there is plenty. */
        if(frame>=next_mine){if(minerals<2000&&c<0)(void)order_idle_drones(3);next_mine=frame+400;}
        COST(2)
        if(frame>=next_train){
            int done=0;
            for(int i=0;i<MAX_BUILDINGS;i++){
                if(!b[i].alive||b[i].owner!=SIDE||!sc64_race_is_zerg_townhall(b[i].type)||!b[i].larva||
                   !building_complete(i)||dist2(b[i].x,b[i].y,ax,ay)>=520*520)continue;
                while(b[i].larva>0){
                    int spare=race_owner_supply_cap(SIDE)-race_owner_supply(SIDE),type=-1;
                    int hy=count_units(SIDE,U_HYDRALISK);
                    /* The first attacks come early: an army before the full economy. */
                    if(spare<14&&minerals>=100&&queued_units(U_OVERLORD)<3)type=U_OVERLORD;
                    else if(spare>=2&&drone_count()+queued_units(U_DRONE)<(hy>=knob("Z10_HFIRST",16)?drone_goal:knob("Z10_DR0",9))&&minerals>=50)type=U_DRONE;
                    else if(count_buildings(B_SPIRE,1)&&minerals>=100&&gas>=100&&spare>=4&&count_units(SIDE,U_MUTALISK)<16)type=U_MUTALISK;
                    else if(count_buildings(B_HYDRALISK_DEN,1)&&minerals>=75&&gas>=25&&spare>=2&&count_units(SIDE,U_HYDRALISK)<(count_units(SIDE,U_MUTALISK)<10?30:hydra_cap))type=U_HYDRALISK;
                    else if(count_buildings(B_SPAWNING_POOL,1)&&minerals>=150&&frame>=2000&&spare>=2&&(!count_buildings(B_HYDRALISK_DEN,1)||gas<25))type=U_ZERGLING;
                    if(type<0||!train_at(i,type))break;
                    ++done;
                }
            }
            next_train=frame+(done?60:120);
        }
        COST(3)
        if(frame>=next_build){
            if(!count_buildings(B_SPAWNING_POOL,0)&&minerals>=200&&drone_count()>=2)
                (void)build_near(B_SPAWNING_POOL,MENU_BARRACKS,ax+60,ay-110,128);
            else if(count_buildings(B_SPAWNING_POOL,1)&&!count_buildings(B_HYDRALISK_DEN,0)&&minerals>=100&&gas>=50&&drone_count()>=5)
                (void)build_near(B_HYDRALISK_DEN,MENU_ACADEMY,ax-90,ay-90,128);
            else if(count_buildings(B_SPAWNING_POOL,1)&&!count_buildings(B_EXTRACTOR,0)&&minerals>=50&&drone_count()>=6)
                (void)build_extractor_near(608,1712);
            else if(count_buildings(B_LAIR,1)&&!count_buildings(B_SPIRE,0)&&minerals>=200&&gas>=150&&drone_count()>=5)
                (void)build_near(B_SPIRE,MENU_ZP_SPIRE,ax+100,ay-100,128);
            else if(count_buildings(B_SPAWNING_POOL,1)&&!count_buildings(B_LAIR,0)&&minerals>=150&&gas>=100){
                for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_HATCHERY&&building_complete(i)&&!b[i].queue_count){
                    if(select_building(i)) (void)choose_row(MENU_ZP_LAIR);
                    break;
                }
            }
            next_build=frame+240;
        }
        COST(4)
        if(frame>=next_hatch&&minerals>=knob("Z10_HMIN",500)&&drone_count()>=6&&count_buildings(B_HATCHERY,0)<hatch_goal&&
           (count_buildings(B_HATCHERY,0)<knob("Z10_H0",2)||count_units(SIDE,U_HYDRALISK)>=knob("Z10_HFIRST",16))){
            /* North of the hive, in the open: walled-in Hatcheries trap their larvae. */
            static const int spots[][2]={{560,1880},{540,1740},{700,1740},{860,1740},{980,1700},{420,1800}};
            int k=count_buildings(B_HATCHERY,0)-1;
            if(k>=0&&k<6)(void)build_near(B_HATCHERY,MENU_DEPOT,spots[k][0],spots[k][1],120);
            next_hatch=frame+300;
        }
        COST(5)
        if(frame>=next_gas){(void)assign_gas_drone(gas_drones);next_gas=frame+600;}
        COST(6)
        if(frame>=next_rally&&!rush){
            group_area_x=ax;group_area_y=ay-60;group_area_r=560;group_rounds=4;
            (void)group_command(U_HYDRALISK,MENU_ATTACK,road[0][0],road[0][1],0);
            (void)group_command(U_ZERGLING,MENU_ATTACK,road[0][0],road[0][1],0);
            (void)group_command(U_MUTALISK,MENU_ATTACK,road[0][0],road[0][1],0);
            group_area_x=-1;group_rounds=6;
            next_rally=frame+500;
        }
        COST(7)
        /* Fresh Hydralisks follow the army up the road in stages. */
        if(rush&&frame>=next_stream){
            group_area_x=ax;group_area_y=ay-60;group_area_r=560;group_rounds=3;
            (void)group_command(U_HYDRALISK,MENU_ATTACK,road[0][0],road[0][1],0);
            (void)group_command(U_MUTALISK,MENU_ATTACK,road[0][0],road[0][1],0);
            for(int k=0;k+1<=leg&&k<ROAD_LEGS-1;k+=2){
                int to=k+2<leg?k+2:leg;
                group_area_x=road[k][0];group_area_y=road[k][1];group_area_r=120;group_rounds=2;
                (void)group_command(U_HYDRALISK,MENU_ATTACK,road[to][0],road[to][1],0);
                (void)group_command(U_MUTALISK,MENU_ATTACK,road[to][0],road[to][1],0);
            }
            group_area_x=-1;group_rounds=6;
            next_stream=frame+knob("Z10_STREAM",800);
        }
        COST(9)
        }
        turn=0;
        if(frame>=next_checkpoint){census("progress",leg);next_checkpoint=frame+2000;}
        input(0);
    }
    census("final",leg);
    fclose(trace);
    printf("z10 controller: frame=%u calls=%u won=%d lost=%d screen=%d fault=%u drones=%d hydras=%d leg=%d temple=%d crystal=%d\n",
        (unsigned)frame,host_tick,won,lost,screen,(unsigned)race_zl_sched.core.fault,drone_count(),count_units(SIDE,U_HYDRALISK),leg,temple()>=0,crystal()>=0);
    return won?0:1;
}
