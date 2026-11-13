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

enum { HOME_X=288, HOME_Y=152, RAW_NEXUS=154, RAW_CRYSTAL=219 };

/* The only way out of the home plateau, past the first town and down the
 * middle of the map to the crystal formation. */
static const int road[][2]={{640,260},{760,280},{900,264},{1040,280},{1056,400},{1050,520},
    {1000,620},{968,760},{904,860},{904,1000},{904,1170},{904,1336},{870,1390}};
enum { ROAD_LEGS=sizeof(road)/sizeof(road[0]) };

static int near_road(int x,int y,int reach)
{
    for(int k=0;k+1<ROAD_LEGS;k++)for(int t=0;t<=4;t++){
        int px=road[k][0]+(road[k+1][0]-road[k][0])*t/4;
        int py=road[k][1]+(road[k+1][1]-road[k][1])*t/4;
        if(dist2(x,y,px,py)<reach*reach)return 1;
    }
    return 0;
}

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
            close=dist2(b[i].x,b[i].y,x,y)<150*150;
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
            if(dist2(b[i].x,b[i].y,x,y)<150*150)return 0;
        }
    return 1;
}

static int hydras_near(int x,int y,int r)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&dist2(u[i].x,u[i].y,x,y)<r*r;
    return n;
}

static int isqrt_i(int v){int lo=0,hi=46340;while(lo<hi){int mid=(lo+hi+1)/2;if(mid*mid<=v)lo=mid;else hi=mid-1;}return lo;}
static int foe_combat(int raw){return raw==65||raw==66||raw==67||raw==68||raw==70||raw==71||raw==72||raw==83;}
static int nearest_foe(int x,int y,int *dd){
    int best=-1,bd=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&foe_combat(u[i].raw_type)){int d=dist2(u[i].x,u[i].y,x,y);if(d<bd){bd=d;best=i;}}
    if(dd)*dd=best>=0?isqrt_i(bd):9999;
    return best;
}
/* One L-drag box around every Hydralisk within r of (x,y), then one attack order. */
static int hydra_box_attack(int x,int y,int r,int tx,int ty,int force_rebox)
{
    int near=0,sel=0,x0=0x7fffffff,y0=0x7fffffff,x1=-1,y1=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&dist2(u[i].x,u[i].y,x,y)<r*r){
        ++near;sel+=u[i].selected;
        if(u[i].x<x0)x0=u[i].x;if(u[i].y<y0)y0=u[i].y;if(u[i].x>x1)x1=u[i].x;if(u[i].y>y1)y1=u[i].y;
    }
    if(!near)return 0;
    if(force_rebox||selected_building>=0||sel*100<near*75||selected_count()>near+6){
        int n=box_select(x0-24,y0-24,x1+24,y1+24);
        if(!n)return 0;
    }
    return command_selected(MENU_ATTACK,tx,ty)?near:0;
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
    int saved=0,turn=0,leg=0,rush=0,next_checkpoint=2000,next_mine=0,next_train=0,next_build=100,next_gas=400,next_hatch=600,next_rally=1500,next_order=0,next_save_build=0,next_stream=0,next_runner=0,runner=-1,runner_leg=0,taken=0,escort=-1,next_defend=0,next_research=0,leg_since=0,leg_last=-1,last_box=-100000,next_muta=0,muta_go=0,next_lair=0,next_mgather=0,mfront=2,next_wait=0,next_mwait=0;
    const int drone_goal=16,hatch_goal=7,gas_drones=3,hydra_cap=16;
    /* The garrisons are Reaver-heavy and a Hydralisk cannot answer one from the ground, so
       Mutalisks (air-only targets for Reavers and Zealots) clear the road ahead of the Drone
       while a small Hydralisk guard holds the rally point. Upgrades come from the starting
       Evolution Chamber. */
    const int hold_after=36000,gather_percent=80,gather_wait=1500,upgrades=2,contact=320,fight_period=90,rebox_period=300,box_reach=420,
        muta_cap=40,muta_go_count=14,lair_after=2500,muta_reach=520,hydra_go_count=12,return_lead=150;
    while(host_tick<limit_ticks&&screen==SCREEN_PLAY&&!won&&!lost){
        if(!taken&&race_zl_sched.countdown){taken=1;fprintf(trace,"{\"kind\":\"taken\",\"frame\":%u,\"hydras\":%d,\"leg\":%d}\n",(unsigned)frame,count_units(SIDE,U_HYDRALISK),leg);}
        if(taken==1&&crystal()>=0){taken=2;fprintf(trace,"{\"kind\":\"crystal\",\"frame\":%u,\"hydras\":%d}\n",(unsigned)frame,count_units(SIDE,U_HYDRALISK));}
        if(menu_open)KEY_TAP(KEY_B);
        int mark=frame,ax=HOME_X,ay=HOME_Y;
        int carrier=crystal_drone();
        int waiting=race_zl_sched.countdown>return_lead;
        /* The crystal's Drone walks home a leg behind the army, or alone once
           there is no army. */
        if(carrier>=0){
            int best=0,bd=0x7fffffff;
            for(int k=0;k<ROAD_LEGS;k++){int d=dist2(u[carrier].x,u[carrier].y,road[k][0],road[k][1]);if(d<bd){bd=d;best=k;}}
            int to=rush&&escort>=0?escort:best-1;
            if(to>best)to=best;
            int tx=to>=0&&to<ROAD_LEGS?road[to][0]:264,ty=to>=0&&to<ROAD_LEGS?road[to][1]:240;
            if(to==0&&best==0&&(!rush||escort==0))tx=264,ty=240;
            if(u[carrier].order==ORDER_IDLE||dist2(u[carrier].tx,u[carrier].ty,tx,ty)>24*24){
                (void)unit_order(carrier,MENU_MOVE,tx,ty);close_menus();
            }
        }
        /* Once the army is past the first town, a Drone follows it down the road.
           It waits a leg behind, so its next order must not wait for other jobs. */
        int loose=crystal()>=0&&carrier<0;
        if(frame>=next_runner&&(loose||(rush&&leg>=6&&!race_zl_sched.countdown&&crystal()<0&&road_clear(leg)))){
            if(runner<0||!u[runner].alive||u[runner].type!=U_DRONE){runner=free_drone(-1,-1);runner_leg=0;}
            if(runner>=0){
                while(runner_leg<ROAD_LEGS&&dist2(u[runner].x,u[runner].y,road[runner_leg][0],road[runner_leg][1])<60*60)++runner_leg;
                int to=runner_leg;
                if(to>=leg&&leg<ROAD_LEGS-1)to=leg-1;
                if(to>=ROAD_LEGS&&runner_leg<ROAD_LEGS)to=ROAD_LEGS-1;
                if(to<runner_leg)to=runner_leg;
                int tx=to<ROAD_LEGS?road[to][0]:930,ty=to<ROAD_LEGS?road[to][1]:1420;
                if(loose){
                    /* A dropped crystal: fetch it. Its leg is the last one needed. */
                    int c=crystal(),last=0,bd=0x7fffffff;
                    for(int k=0;k<ROAD_LEGS;k++){int d=dist2(u[c].x,u[c].y,road[k][0],road[k][1]);if(d<bd){bd=d;last=k;}}
                    to=runner_leg;
                    if(to>last)to=last;
                    tx=road[to][0];ty=road[to][1];
                    if(runner_leg>=last||dist2(u[runner].x,u[runner].y,u[c].x,u[c].y)<200*200){tx=u[c].x;ty=u[c].y;}
                }
                if(u[runner].order==ORDER_IDLE||dist2(u[runner].tx,u[runner].ty,tx,ty)>24*24){(void)unit_order(runner,MENU_MOVE,tx,ty);close_menus();}
            }
            next_runner=frame+60;
        }
        if(frame-mark>=20){input(0);continue;}
        /* The army advances along the road, leg by leg. */
        if(frame>=next_order){
            next_order=frame+150;
            int home=count_units(SIDE,U_HYDRALISK);
            int upgraded=(weapons_level>=upgrades&&armor_level>=upgrades)||frame>=hold_after;
            {int rv=0;for(int i=0;i<MAX_UNITS;i++)rv+=u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==83&&u[i].y<1000&&u[i].x>700&&u[i].x<1400;
                if(rv>1&&frame<hold_after)upgraded=0;}
            if(!rush&&(muta_go&&home>=hydra_go_count)&&frame>=8000&&upgraded){rush=1;fprintf(trace,"{\"kind\":\"assault\",\"frame\":%u,\"army\":%d}\n",(unsigned)frame,home);}
            if(rush){
                int n=0,sx=0,sy=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK){
                    int c=0,x2=0,y2=0;
                    for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&u[k].type==U_HYDRALISK&&dist2(u[k].x,u[k].y,u[i].x,u[i].y)<240*240){++c;x2+=u[k].x;y2+=u[k].y;}
                    if(c>n){n=c;sx=x2;sy=y2;}
                }
                if(n<12){rush=0;leg=0;}
                else {
                    int gx,gy,ax2=sx/n,ay2=sy/n;
                    /* A new clump starts from the leg it stands by. */
                    int nearest=0,bd=0x7fffffff;
                    for(int k=0;k<ROAD_LEGS;k++){int dd=dist2(ax2,ay2,road[k][0],road[k][1]);if(dd<bd){bd=dd;nearest=k;}}
                    if(nearest<leg-2)leg=nearest;
                    if(muta_go&&crystal()<0){leg=mfront>0?mfront-1:0;}
                    else while(crystal()<0&&leg<ROAD_LEGS-1&&road_cannon(ax2,ay2,220)<0){
                        int there=hydras_near(road[leg][0],road[leg][1],200);
                        int total=0;for(int i=0;i<MAX_UNITS;i++)total+=u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&dist2(u[i].x,u[i].y,HOME_X,HOME_Y+60)>350*350;
                        if(leg!=leg_last){leg_last=leg;leg_since=frame;}
                        if(there*100<total*gather_percent&&frame-leg_since<gather_wait)break;
                        ++leg;
                    }
                    /* A far order gives up, so a clump behind walks to the next leg. */
                    int to_leg=leg<nearest+1?leg:nearest+1;
                    /* With the crystal out, the army goes first on the way home. */
                    if(crystal()>=0){
                        if(escort<0)escort=leg;
                        int there=hydras_near(road[escort][0],road[escort][1],170);
                        if(carrier>=0&&escort>0&&there*100>=n*35&&
                           dist2(u[carrier].x,u[carrier].y,road[escort][0],road[escort][1])<140*140)--escort;
                        /* A dropped crystal calls the army back to it. */
                        if(carrier<0){
                            int c=crystal(),bd2=0x7fffffff;
                            for(int k=0;k<ROAD_LEGS;k++){int d=dist2(u[c].x,u[c].y,road[k][0],road[k][1]);if(d<bd2){bd2=d;escort=k;}}
                        }
                        to_leg=escort;
                    }
                    gx=road[to_leg][0];gy=road[to_leg][1];
                    int cannon=road_cannon(ax2,ay2,220);
                    if(cannon>=0){gx=b[cannon].x;gy=b[cannon].y;}
                    /* A Reaver's splash spends a crowd fastest: shoot those first. */
                    int reaver=-1;
                    {int rd=300;rd*=rd;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==83&&dist2(u[i].x,u[i].y,road[to_leg][0],road[to_leg][1])<260*260){int dd=dist2(u[i].x,u[i].y,ax2,ay2);if(dd<rd){rd=dd;reaver=i;}}}
                    if(reaver>=0){gx=u[reaver].x;gy=u[reaver].y;}
                    if(muta_go&&crystal()<0){to_leg=0;gx=road[to_leg][0];gy=road[to_leg][1];}
                    int fd=9999,foe=crystal()<0?nearest_foe(ax2,ay2,&fd):-1;
                    if(waiting){
                        if(frame>=next_wait){(void)hydra_box_attack(ax2,ay2,box_reach+200,road[2][0],road[2][1],1);next_wait=frame+600;}
                        next_order=frame+300;
                    } else if(crystal()<0){
                        if(foe>=0&&fd<contact){
                            /* In reach of the garrison: everything charges at once. */
                            int fx=0,fy=0,fn=0;
                            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&foe_combat(u[i].raw_type)&&dist2(u[i].x,u[i].y,u[foe].x,u[foe].y)<200*200){fx+=u[i].x;fy+=u[i].y;++fn;}
                            gx=fx/fn;gy=fy/fn;
                            if(reaver>=0&&dist2(u[reaver].x,u[reaver].y,ax2,ay2)<360*360){gx=u[reaver].x;gy=u[reaver].y;}
                            (void)hydra_box_attack(ax2,ay2,box_reach,gx,gy,frame-last_box>=rebox_period);
                            if(frame-last_box>=rebox_period)last_box=frame;
                            fprintf(trace,"{\"kind\":\"charge\",\"frame\":%u,\"target\":[%d,%d],\"foe\":%d,\"fd\":%d,\"army\":%d}\n",(unsigned)frame,gx,gy,foe,fd,n);
                            next_order=frame+fight_period;
                        } else {
                            (void)hydra_box_attack(ax2,ay2,box_reach,gx,gy,1);
                            last_box=frame;
                            fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"leg\":%d,\"army\":%d,\"at\":[%d,%d],\"fd\":%d}\n",(unsigned)frame,gx,gy,leg,n,ax2,ay2,fd);
                            next_order=frame+300;
                        }
                    } else {
                    group_area_x=ax2;group_area_y=ay2;group_area_r=320;group_rounds=8;
                    (void)group_command(U_HYDRALISK,MENU_ATTACK,gx,gy,cannon>=0||reaver>=0);
                    group_area_x=-1;group_rounds=6;
                    fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"leg\":%d,\"army\":%d,\"at\":[%d,%d]}\n",(unsigned)frame,gx,gy,leg,n,ax2,ay2);
                    next_order=frame+(reaver>=0?60:300);
                    }
                }
            }
        }
        if(frame-mark>=20){input(0);continue;}
#define COST(n) if(frame-mark>=20){turn=n;input(0);continue;} case n:;
        switch(turn){ case 0:;
        if(!saved&&frame>=6000){
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
            next_defend=frame+200;
        }
        COST(1)
        /* The unit pool fills before the bank empties: no need to chase idle
           Drones across the field once there is plenty. */
        if(frame>=next_mine){if(minerals<2000&&crystal()<0)(void)order_idle_drones(3);next_mine=frame+400;}
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
                    else if(count_buildings(B_SPIRE,1)&&minerals>=100&&gas>=100&&spare>=4&&count_units(SIDE,U_MUTALISK)+queued_units(U_MUTALISK)<muta_cap)type=U_MUTALISK;
                    else if(count_buildings(B_HYDRALISK_DEN,1)&&minerals>=75&&gas>=25&&spare>=2&&count_units(SIDE,U_HYDRALISK)<hydra_cap)type=U_HYDRALISK;
                    else if(count_buildings(B_SPAWNING_POOL,1)&&minerals>=150&&frame>=2000&&spare>=2&&(!count_buildings(B_HYDRALISK_DEN,1)||gas<25))type=U_ZERGLING;
                    if(type<0||!train_at(i,type))break;
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
            else if(!count_buildings(B_EVOLUTION_CHAMBER,0)&&count_buildings(B_HYDRALISK_DEN,1)&&minerals>=100&&drone_count()>=6)
                (void)build_near(B_EVOLUTION_CHAMBER,MENU_ENG_BAY,ax-90,ay+150,128);
            else if(frame>=lair_after&&frame>=next_lair&&count_buildings(B_SPAWNING_POOL,1)&&!count_buildings(B_LAIR,0)&&minerals>=150&&gas>=100){
                int h=-1,bd=0x7fffffff;
                for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_HATCHERY&&building_complete(i)&&!b[i].queue_count){int d=dist2(b[i].x,b[i].y,296,328);if(d<bd){bd=d;h=i;}}
                next_lair=frame+(h>=0?600:100);
                if(h>=0&&select_building(h)){
                    int ok=choose_row(MENU_ZP_LAIR);
                    close_menus();clear_selected_building();
                    fprintf(trace,"{\"kind\":\"lair\",\"frame\":%u,\"ok\":%d,\"lair\":%d}\n",(unsigned)frame,ok,count_buildings(B_LAIR,0));
                }
            }
            else if(count_buildings(B_LAIR,1)&&!count_buildings(B_SPIRE,0)&&minerals>=200&&gas>=150&&drone_count()>=6)
                (void)build_near(B_SPIRE,MENU_ZP_SPIRE,ax+90,ay+150,128);
            else if(!count_buildings(B_EXTRACTOR,0)&&minerals>=50&&drone_count()>=6)
                (void)build_extractor_near(416,240);
            next_build=frame+240;
        }
        COST(4)
        if(frame>=next_hatch&&minerals>=500&&drone_count()>=6&&count_buildings(B_HATCHERY,0)<hatch_goal){
            /* South and east of the first hive, in the open: Hatcheries placed
               behind it wall their own larvae in. */
            static const int spots[][2]={{300,340},{440,360},{170,340},{560,180},{560,340},{300,500}};
            int k=count_buildings(B_HATCHERY,0)-1;
            if(k>=0&&k<6)(void)build_near(B_HATCHERY,MENU_DEPOT,spots[k][0],spots[k][1],120);
            next_hatch=frame+300;
        }
        COST(5)
        if(frame>=next_gas){(void)assign_gas_drone(gas_drones);next_gas=frame+600;}
        COST(12)
        if(frame>=next_muta&&crystal()<0){
            next_muta=frame+fight_period;
            int nm=0,mx=0,my=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_MUTALISK){nm++;mx+=u[i].x;my+=u[i].y;}
            if(!muta_go&&nm>=muta_go_count){muta_go=1;fprintf(trace,"{\"kind\":\"muta_go\",\"frame\":%u,\"mutas\":%d}\n",(unsigned)frame,nm);}
            if(muta_go&&nm<muta_go_count/3)muta_go=0;
            if(!muta_go&&nm>=4&&frame>=next_mgather){
                next_mgather=frame+900;
                group_area_x=mx/nm;group_area_y=my/nm;group_area_r=700;group_rounds=3;
                (void)group_command(U_MUTALISK,MENU_MOVE,560,250,0);
                group_area_x=-1;group_rounds=6;
            }
            if(muta_go&&nm&&waiting){
                mx/=nm;my/=nm;
                if(frame>=next_mwait){
                    group_area_x=mx;group_area_y=my;group_area_r=420;group_rounds=3;
                    (void)group_command(U_MUTALISK,MENU_ATTACK,road[3][0],road[3][1],1);
                    group_area_x=-1;group_rounds=6;
                    next_mwait=frame+600;
                }
            } else if(muta_go&&nm){
                mx/=nm;my/=nm;
                /* The front walks down the road: clear what threatens the air or the Drone near it. */
                static const int prio[]={83,66,70,71,72,68,67,65,69,84,64};
                int fx=road[mfront][0],fy=road[mfront][1],tgt=-1,tb=-1,bd=0x7fffffff,RM=muta_reach;
                for(unsigned pi=0;pi<sizeof(prio)/sizeof(prio[0])&&tgt<0&&tb<0;pi++){
                    /* The Drone needs the road, not every Cannon in the area.
                       A distant emplacement can trap the sweep on a flank. */
                    if(pi==7)for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&b[i].raw_type==162&&
                        dist2(b[i].x,b[i].y,fx,fy)<RM*RM&&near_road(b[i].x,b[i].y,340)){
                        int d=dist2(b[i].x,b[i].y,mx,my);if(d<bd){bd=d;tb=i;}
                    }
                    if(tb>=0)break;
                    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==prio[pi]&&
                        !SC64_TRANSPORT_LOADED(&u[i])&&dist2(u[i].x,u[i].y,fx,fy)<RM*RM&&near_road(u[i].x,u[i].y,340)){
                        int d=dist2(u[i].x,u[i].y,mx,my);if(d<bd){bd=d;tgt=i;}
                    }
                }
                int gx=-1,gy=-1,gt=0;
                if(tgt>=0){gx=u[tgt].x;gy=u[tgt].y;gt=u[tgt].raw_type;}
                else if(tb>=0){gx=b[tb].x;gy=b[tb].y;gt=162;}
                if(gx>=0){
                    group_area_x=mx;group_area_y=my;group_area_r=420;group_rounds=3;
                    int groups=group_command(U_MUTALISK,MENU_ATTACK,gx,gy,1);
                    /* A dispersed air group can have no selectable cluster
                       near its average position. Try the remaining anchors. */
                    if(!groups){
                        group_area_x=-1;group_rounds=8;
                        groups=group_command(U_MUTALISK,MENU_ATTACK,gx,gy,1);
                    }
                    group_area_x=-1;group_rounds=6;
                    fprintf(trace,"{\"kind\":\"muta_order\",\"frame\":%u,\"mutas\":%d,\"target\":[%d,%d],\"type\":%d,\"front\":%d,\"groups\":%d}\n",(unsigned)frame,nm,gx,gy,gt,mfront,groups);
                } else {
                    if(nm>=muta_go_count/2&&mfront<ROAD_LEGS-1){++mfront;fprintf(trace,"{\"kind\":\"muta_front\",\"frame\":%u,\"front\":%d,\"mutas\":%d}\n",(unsigned)frame,mfront,nm);}
                    group_area_x=mx;group_area_y=my;group_area_r=420;group_rounds=3;
                    (void)group_command(U_MUTALISK,MENU_ATTACK,fx,fy,0);
                    group_area_x=-1;group_rounds=6;
                }
            }
        }
        COST(11)
        if(frame>=next_research&&count_buildings(B_EVOLUTION_CHAMBER,1)){
            next_research=frame+240;
            for(int w=0;w<2;w++){
                int which=w?MENU_ARMOR:MENU_WEAPONS;
                int lvl=w?armor_level:weapons_level,busy=w?armor_research:weapons_research;
                if(lvl<2&&!busy&&minerals>=100&&gas>=100){
                    int pick=free_drone(-1,-1);
                    if(pick>=0&&select_single(pick)){
                        (void)choose_row(which);
                        close_menus();
                        fprintf(trace,"{\"kind\":\"research\",\"frame\":%u,\"which\":%d,\"w\":%d,\"a\":%d}\n",(unsigned)frame,w,weapons_level,armor_level);
                    }
                }
            }
        }
        COST(6)
        if(frame>=next_rally&&!rush){
            group_area_x=ax;group_area_y=ay+60;group_area_r=480;group_rounds=4;
            /* Stand between the hives and the way in until the army is big enough
               to leave; Scouts come over the cliffs. */
            int forward=count_units(SIDE,U_HYDRALISK)>=40;
            int rx=forward?road[0][0]:360,ry=forward?road[0][1]:250;
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
            next_stream=frame+800;
        }
        COST(8)
        COST(9)
        }
        turn=0;
        if(frame>=next_checkpoint){census("progress",leg);next_checkpoint=frame+2000;}
        input(0);
    }
    census("final",leg);
    fclose(trace);
    printf("z9 controller: frame=%u calls=%u won=%d lost=%d screen=%d fault=%u drones=%d hydras=%d leg=%d countdown=%u crystal=%d\n",
        (unsigned)frame,host_tick,won,lost,screen,(unsigned)race_zl_sched.core.fault,drone_count(),count_units(SIDE,U_HYDRALISK),leg,(unsigned)race_zl_sched.countdown,crystal()>=0);
    return won?0:1;
}
