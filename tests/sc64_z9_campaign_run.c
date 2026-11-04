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
    {1000,620},{968,760},{904,860},{904,1000},{904,1170},{904,1336},{870,1390}};
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

/* The nearest Photon Cannon that reaches the road and that the army is close
 * enough to kill. A Drone won't pass a live one. */
static int road_cannon(int ax,int ay,int reach)
{
    int best=-1,bd=reach*reach;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&b[i].raw_type==162){
        int d=dist2(b[i].x,b[i].y,ax,ay);
        if(d>=bd)continue;
        int close=0;
        for(int k=0;k+1<ROAD_LEGS&&!close;k++)for(int t=0;t<=4&&!close;t++){
            int x=road[k][0]+(road[k+1][0]-road[k][0])*t/4,y=road[k][1]+(road[k+1][1]-road[k][1])*t/4;
            close=dist2(b[i].x,b[i].y,x,y)<knob("Z9_CR",150)*knob("Z9_CR",150);
        }
        if(close){bd=d;best=i;}
    }
    return best;
}

static int road_clear(int upto)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&b[i].raw_type==162)
        for(int k=0;k<upto&&k+1<ROAD_LEGS;k++)for(int t=0;t<=4;t++){
            int x=road[k][0]+(road[k+1][0]-road[k][0])*t/4,y=road[k][1]+(road[k+1][1]-road[k][1])*t/4;
            if(dist2(b[i].x,b[i].y,x,y)<knob("Z9_CR",150)*knob("Z9_CR",150))return 0;
        }
    return 1;
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
#ifdef Z9_SWEEP
        {static int ll;if(getenv("Z9_RUNNER")&&rush&&frame>=ll){ll=frame+atoi(getenv("Z9_RUNNER"));
            fprintf(stderr,"R f%u leg%d runner %d alive%d at(%d,%d) o%d t(%d,%d) rleg%d clear%d nh%d cannons:",(unsigned)frame,leg,runner,runner>=0&&u[runner].alive,runner>=0?u[runner].x:-1,runner>=0?u[runner].y:-1,runner>=0?u[runner].order:-1,runner>=0?u[runner].tx:-1,runner>=0?u[runner].ty:-1,runner_leg,road_clear(leg),count_units(SIDE,U_HYDRALISK));
            for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&b[i].raw_type==162&&b[i].y<1400&&road_cannon(b[i].x,b[i].y,2)==i)fprintf(stderr," (%d,%d)",b[i].x,b[i].y);fprintf(stderr,"\n");}}
#endif
        if(!taken&&race_zl_sched.countdown){taken=1;fprintf(trace,"{\"kind\":\"taken\",\"frame\":%u,\"hydras\":%d,\"leg\":%d}\n",(unsigned)frame,count_units(SIDE,U_HYDRALISK),leg);}
        if(taken==1&&crystal()>=0){taken=2;fprintf(trace,"{\"kind\":\"crystal\",\"frame\":%u,\"hydras\":%d}\n",(unsigned)frame,count_units(SIDE,U_HYDRALISK));}
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
#ifdef Z9_SWEEP
        {static int ll;if(getenv("Z9_ARMY")&&rush&&frame>=ll){ll=frame+atoi(getenv("Z9_ARMY"));int gx=road[leg][0],gy=road[leg][1];int ord[24]={0},t=0,sx=0,sy=0,nl=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK){ord[u[i].order&23]++;t++;if(dist2(u[i].x,u[i].y,gx,gy)<300*300){sx+=u[i].x;sy+=u[i].y;nl++;}}
        fprintf(stderr,"A f%u leg%d hy%d nearleg%d at(%d,%d) o:",(unsigned)frame,leg,t,nl,nl?sx/nl:0,nl?sy/nl:0);for(int k=0;k<24;k++)if(ord[k])fprintf(stderr," %d:%d",k,ord[k]);
        int ne[256]={0},nb=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&dist2(u[i].x,u[i].y,gx,gy)<340*340)ne[u[i].raw_type&255]++;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&b[i].raw_type==162&&dist2(b[i].x,b[i].y,gx,gy)<340*340)nb++;{int cell[16][16]={{0}};for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK)cell[u[i].y/128][u[i].x/128]++;fprintf(stderr," cells:");for(int y=0;y<16;y++)for(int x=0;x<16;x++)if(cell[y][x]>=2)fprintf(stderr," (%d,%d):%d",x*128,y*128,cell[y][x]);}fprintf(stderr," cannons%d foes:",nb);for(int k=0;k<256;k++)if(ne[k])fprintf(stderr," r%d:%d",k,ne[k]);fprintf(stderr,"\n");}}
#endif
        /* The army advances along the road, leg by leg. */
        if(frame>=next_order){
            next_order=frame+150;
            int home=count_units(SIDE,U_HYDRALISK);
            if(!rush&&home>=knob("Z9_GO",50)&&frame>=knob("Z9_GOF",8000)){rush=1;fprintf(trace,"{\"kind\":\"assault\",\"frame\":%u,\"army\":%d}\n",(unsigned)frame,home);}
            if(rush){
                int n=0,sx=0,sy=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK){
                    int c=0,x2=0,y2=0;
                    for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&u[k].type==U_HYDRALISK&&dist2(u[k].x,u[k].y,u[i].x,u[i].y)<240*240){++c;x2+=u[k].x;y2+=u[k].y;}
                    if(c>n){n=c;sx=x2;sy=y2;}
                }
                if(n<knob("Z9_RETREAT",12)){rush=0;leg=0;}
                else {
                    int gx,gy,ax2=sx/n,ay2=sy/n;
                    /* A new clump starts from the leg it stands by. */
                    int nearest=0,bd=0x7fffffff;
                    for(int k=0;k<ROAD_LEGS;k++){int dd=dist2(ax2,ay2,road[k][0],road[k][1]);if(dd<bd){bd=dd;nearest=k;}}
                    if(nearest<leg-2)leg=nearest;
                    while(leg<ROAD_LEGS-1&&road_cannon(ax2,ay2,knob("Z9_CREACH",280))<0){
                        int there=hydras_near(road[leg][0],road[leg][1],170);
                        if(there*100<n*knob("Z9_LEGP",50))break;
                        ++leg;
                    }
                    /* A far order gives up, so a clump behind walks to the next leg. */
                    {int to=leg<nearest+1?leg:nearest+1;gx=road[to][0];gy=road[to][1];}
                    int cannon=road_cannon(ax2,ay2,knob("Z9_CREACH",280));
                    if(cannon>=0){gx=b[cannon].x;gy=b[cannon].y;}
                    /* A Reaver's splash spends a crowd fastest: shoot those first. */
                    int reaver=-1;
                    {int rd=knob("Z9_RV",300);rd*=rd;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==83){int dd=dist2(u[i].x,u[i].y,ax2,ay2);if(dd<rd){rd=dd;reaver=i;}}}
                    if(reaver>=0){gx=u[reaver].x;gy=u[reaver].y;}
                    group_area_x=ax2;group_area_y=ay2;group_area_r=320;group_rounds=8;
                    (void)group_command(U_HYDRALISK,MENU_ATTACK,gx,gy,cannon>=0||reaver>=0);
                    group_area_x=-1;group_rounds=6;
                    fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"leg\":%d,\"army\":%d,\"at\":[%d,%d]}\n",(unsigned)frame,gx,gy,leg,n,ax2,ay2);
                    next_order=frame+(reaver>=0?knob("Z9_ORDR",60):knob("Z9_ORD",300));
                }
            }
        }
        if(frame-mark>=20){input(0);continue;}
#define COST(n) if(frame-mark>=20){turn=n;input(0);continue;} case n:;
        switch(turn){ case 0:;
        if(!saved&&frame>=knob("Z9_SAVE",6000)){
            int pick=free_drone(-1,-1);
            if(under_construction()&&pick>=0){
                if(clear_menu_last(pick)&&pause_menu_save(path,pick)){saved=1;census("saved",leg);}
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
            /* South and east of the first hive, in the open: Hatcheries placed
               behind it wall their own larvae in. */
            static const int spots[][2]={{300,340},{440,360},{170,340},{560,180},{560,340},{300,500}};
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
        /* Once the army has cleared the gate, a Drone follows it down the road. */
        if(rush&&leg>=knob("Z9_RUN",4)&&!race_zl_sched.countdown&&crystal()<0&&frame>=next_runner&&road_clear(leg)){
            if(runner<0||!u[runner].alive||u[runner].type!=U_DRONE){runner=free_drone(-1,-1);runner_leg=0;}
            if(runner>=0){
                while(runner_leg<ROAD_LEGS&&dist2(u[runner].x,u[runner].y,road[runner_leg][0],road[runner_leg][1])<60*60)++runner_leg;
                /* Stay a leg behind the army until it holds the far end. */
                int to=runner_leg;
                if(to>=leg&&leg<ROAD_LEGS-1)to=leg-1;
                int tx=to<ROAD_LEGS?road[to][0]:930,ty=to<ROAD_LEGS?road[to][1]:1420;
                if(dist2(u[runner].tx,u[runner].ty,tx,ty)>24*24||u[runner].order==ORDER_IDLE){(void)unit_order(runner,MENU_MOVE,tx,ty);close_menus();}
            }
            next_runner=frame+knob("Z9_RUNP",150);
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
