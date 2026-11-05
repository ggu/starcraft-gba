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

/* Pending live Eggs are already paid products, in addition to producer queues. */
static int z10_pending_products(int type)
{
    int n = queued_units(type) * sc64_race_product_count(type);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE &&
            u[i].type == SC64_SHARED_EGG_TYPE && u[i].target == type)
            n += sc64_race_product_count(type);
    return n;
}

/* Controller-only roles: keep existing retreat-sized force at home. */
static u8 z10_home_reserve[MAX_UNITS];
static void z10_reserve_home(void)
{
    int need=knob("Z10_RETREAT",12),have=0;
    for(int i=0;i<MAX_UNITS;i++){
        if(z10_home_reserve[i]&&(!u[i].alive||u[i].owner!=SIDE||u[i].type!=U_HYDRALISK))z10_home_reserve[i]=0;
        have+=z10_home_reserve[i]!=0;
    }
    while(have<need){
        int best=-1,bd=300*300;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&!z10_home_reserve[i]){
            int d=dist2(u[i].x,u[i].y,HOME_X,HOME_Y);if(d<bd){bd=d;best=i;}
        }
        if(best<0)break;
        z10_home_reserve[best]=1;have++;
    }
}

/* Double-select may include reserve units even when the anchor is elsewhere.
 * Validate the actual selection; fall back to one ordinary selected unit. */
static int z10_forward_group(int x,int y,int force)
{
    static u8 ordered[MAX_UNITS];int groups=0;
    for(int i=0;i<MAX_UNITS;i++)ordered[i]=z10_home_reserve[i]||
        (group_area_x>=0&&dist2(u[i].x,u[i].y,group_area_x,group_area_y)>group_area_r*group_area_r)||
        (!force&&((u[i].order==ORDER_ATTACK_MOVE&&dist2(u[i].tx,u[i].ty,x,y)<96*96)||u[i].order==ORDER_ATTACK_TARGET));
    for(int round=0;round<group_rounds;round++){
        int anchor=-1,bd=0x7fffffff;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&!ordered[i]){
            int d=dist2(u[i].x,u[i].y,x,y);if(d<bd){bd=d;anchor=i;}
        }
        if(anchor<0)break;
        close_menus();clear_selected_building();move_cursor(u[anchor].x,u[anchor].y);KEY_TAP(KEY_A);KEY_TAP(KEY_A);
        int bad=0,count=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected){count++;if(u[i].type!=U_HYDRALISK||z10_home_reserve[i]||(group_area_x>=0&&dist2(u[i].x,u[i].y,group_area_x,group_area_y)>group_area_r*group_area_r))bad=1;}
        if(!count||bad){
            ordered[anchor]=1;
            if(select_single(anchor)&&command_selected(MENU_ATTACK,x,y))groups++;
            continue;
        }
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected)ordered[i]=1;
        if(command_selected(MENU_ATTACK,x,y))groups++;
    }
    return groups;
}

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

static int hydras_near(int x,int y,int r)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&!z10_home_reserve[i]&&dist2(u[i].x,u[i].y,x,y)<r*r;
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
        "\"side_units\":%d,\"hydras\":%d,\"drones\":%d,\"enemy_units\":%d,\"enemy_buildings\":%d,\"leg\":%d,\"bank\":[%d,%d],"
        "\"supply\":[%d,%d],\"cursor\":[%d,%d],\"countdown\":0,\"towns\":%u,\"fault\":%u}\n",
        label,(unsigned)frame,host_tick,screen,won,lost,count_units(SIDE,-1),count_units(SIDE,U_HYDRALISK),drone_count(),
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
    int next_observe=0;
    int saved=0,turn=0,leg=0,rush=0,next_checkpoint=2000,next_mine=0,next_train=0,next_build=100,next_gas=400,next_hatch=100,next_rally=1500,next_order=0,next_save_build=0,next_stream=0,next_defend=0,next_runner=0,runner=-1,next_static=0;
    const int drone_goal=knob("Z10_DRONES",16),hatch_goal=knob("Z10_HATCH",7),gas_drones=3,hydra_cap=knob("Z10_CAP",110);
    while(host_tick<limit_ticks&&screen==SCREEN_PLAY&&!won&&!lost){
        if(frame >= next_observe) {
            fprintf(stderr,"OBS frame=%u town=%u switch10=%d bank=%d/%d",(unsigned)frame,(unsigned)race_zl_ai.started,sc64_z8_switch_get(&race_zl_sched.core,10),minerals,gas);
            for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive && b[i].owner==SIDE && (b[i].type==B_SUNKEN_COLONY||b[i].type==B_CREEP_COLONY))
                fprintf(stderr," B%d:type%d,xy%d/%d,hp%d,under%d,flags%d",i,b[i].type,b[i].x,b[i].y,b[i].hp,b[i].under_construction,b[i].flags);
            for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].owner==ENEMY && dist2(u[i].x,u[i].y,HOME_X,HOME_Y)<520*520)
                fprintf(stderr," E%d:raw%d,owner%d,hp%d,xy%d/%d",i,u[i].raw_type,u[i].raw_owner,u[i].hp,u[i].x,u[i].y);
            int larvae=0,eggs=0,droneegg=0,overe=0,hydraegg=0,queue=0,halls=0,completehalls=0;
            for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&sc64_race_is_zerg_townhall(b[i].type)){larvae+=b[i].larva;queue+=b[i].queue_count;halls++;completehalls+=building_complete(i);}
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_Z7_EGG){eggs++;droneegg+=u[i].target==U_DRONE;overe+=u[i].target==U_OVERLORD;hydraegg+=u[i].target==U_HYDRALISK;}
            fprintf(stderr," PROD supply=%d/%d den=%d halls=%d completehalls=%d hatch=%d larvae=%d queue=%d eggs=%d droneegg=%d overegg=%d hydraegg=%d overload=%d\n",race_owner_supply(SIDE),race_owner_supply_cap(SIDE),count_buildings(B_HYDRALISK_DEN,1),halls,completehalls,count_buildings(B_HATCHERY,0),larvae,queue,eggs,droneegg,overe,hydraegg,count_units(SIDE,U_OVERLORD)); next_observe=frame+500;
        }
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
            int home=count_units(SIDE,U_HYDRALISK);
            int goal=frame<knob("Z10_L2",30000)?knob("Z10_GO",70):knob("Z10_GO2",55);
            int free_slots=0;
            for(int i=0;i<MAX_UNITS;i++)free_slots+=!u[i].alive;
            int pending=z10_pending_products(U_HYDRALISK);
            int pool_limited=home+pending+free_slots<goal;
            if(!rush&&frame>=knob("Z10_GOF",8000)&&
               (home>=goal||(pool_limited&&home>=knob("Z10_GO2",55)))){
                rush=1;leg=0;z10_reserve_home();
                fprintf(trace,"{\"kind\":\"assault\",\"frame\":%u,\"army\":%d,\"pending\":%d,\"free_slots\":%d,\"pool_limited\":%d}\n",(unsigned)frame,home,pending,free_slots,pool_limited);
            }
            if(rush){
                int n=0,sx=0,sy=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&!z10_home_reserve[i]){
                    int k2=0,x2=0,y2=0;
                    for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&u[k].type==U_HYDRALISK&&!z10_home_reserve[k]&&dist2(u[k].x,u[k].y,u[i].x,u[i].y)<240*240){++k2;x2+=u[k].x;y2+=u[k].y;}
                    if(k2>n){n=k2;sx=x2;sy=y2;}
                }
                if(n<knob("Z10_RETREAT",12)){rush=0;leg=0;}
                else {
                    int ax2=sx/n,ay2=sy/n,nearest=nearest_leg(ax2,ay2);
                    if(nearest<leg-2)leg=nearest;
                    int cannon=road_cannon(ax2,ay2,knob("Z10_CREACH",220));
                    while(leg<ROAD_LEGS-1&&cannon<0){
                        int there=hydras_near(road[leg][0],road[leg][1],170);
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
                    group_area_x=ax2;group_area_y=ay2;group_area_r=320;group_rounds=MAX_UNITS;
                    (void)z10_forward_group(gx,gy,force);
                    group_area_x=-1;group_rounds=6;
                    fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"leg\":%d,\"army\":%d,\"at\":[%d,%d]}\n",(unsigned)frame,gx,gy,leg,n,ax2,ay2);
                    { int nearest_overlord=-1,home_hydras=0,foes[256]={0};
                      for(int i=0;i<MAX_UNITS;i++)if(u[i].alive){
                          if(u[i].owner==SIDE&&u[i].type==U_OVERLORD){int d=dist2(u[i].x,u[i].y,ax2,ay2);if(nearest_overlord<0||d<nearest_overlord)nearest_overlord=d;}
                          if(u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&dist2(u[i].x,u[i].y,HOME_X,HOME_Y)<300*300)home_hydras++;
                          if(u[i].owner==ENEMY&&dist2(u[i].x,u[i].y,ax2,ay2)<400*400)foes[u[i].raw_type&255]++;
                      }
                      fprintf(stderr,"ASSAULT frame=%u leg=%d at=%d/%d target=%d/%d group=%d homehydras=%d nearest_overlord_dist2=%d foes:",(unsigned)frame,leg,ax2,ay2,gx,gy,n,home_hydras,nearest_overlord);
                      for(int k=0;k<256;k++)if(foes[k])fprintf(stderr," raw%d=%d",k,foes[k]);
                      fprintf(stderr,"\n"); }

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
            if(foe>=0&&rush){
                z10_reserve_home();
                for(int i=0;i<MAX_UNITS;i++)if(z10_home_reserve[i]&&u[i].alive&&
                    dist2(u[i].x,u[i].y,HOME_X,HOME_Y)<400*400){
                    if(select_single(i)) (void)command_selected(MENU_ATTACK,u[foe].x,u[foe].y);
                }
            } else if(foe>=0){
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
                    if(drone_count()+z10_pending_products(U_DRONE)<(hy>=knob("Z10_HFIRST",16)?drone_goal:knob("Z10_DR0",9))&&minerals>=50)type=U_DRONE;
                    else if(spare<14&&minerals>=100&&z10_pending_products(U_OVERLORD)<3)type=U_OVERLORD;
                    else if(count_buildings(B_HYDRALISK_DEN,1)&&minerals>=75&&gas>=25&&spare>=2&&count_units(SIDE,U_HYDRALISK)+z10_pending_products(U_HYDRALISK)<hydra_cap)type=U_HYDRALISK;
                    else if(count_buildings(B_SPAWNING_POOL,1)&&minerals>=150&&frame>=2000&&spare>=2&&(!count_buildings(B_HYDRALISK_DEN,1)||gas<25))type=U_ZERGLING;
                    if(type<0)break;
                    { int larva_before=b[i].larva, bank_before=minerals;
                      int accepted=train_at(i,type);
                      fprintf(stderr,"TRAIN frame=%u hall=%d type=%d accepted=%d larva=%d/%d minerals=%d/%d\n",(unsigned)frame,i,type,accepted,larva_before,b[i].larva,bank_before,minerals);
                      if(!accepted)break; }
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
            next_build=frame+240;
        }
        COST(11)
        /* Build and morph ordinary creep colonies to defend the hive approach. */
        if(frame>=next_static && count_buildings(B_SPAWNING_POOL,1)) {
            int active=count_buildings(B_SUNKEN_COLONY,0),colony=-1,pending=0;
            for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive && b[i].owner==SIDE && b[i].type==B_CREEP_COLONY) {
                if(b[i].flags&SC64_Z7_SUNKEN_MORPH_FLAG) active++;
                else if(building_complete(i)) colony=i;
                else pending++;
            }
            if(active<2 && colony>=0 && minerals>=50 && select_building(colony)) {
                int accepted=choose_row(MENU_STOP+1);
                fprintf(trace,"{\"kind\":\"defense\",\"frame\":%u,\"colony\":%d,\"accepted\":%d,\"flags\":%d}\n",(unsigned)frame,colony,accepted,b[colony].flags); close_menus();
            } else if(active<2 && colony<0 && !pending && minerals>=75 && drone_count()>=3) {
                (void)build_near(B_CREEP_COLONY,MENU_Z7_CREEP,ax-80,ay-140,110);
            }
            next_static=frame+240;
        }
        COST(4)
        if(frame>=next_hatch&&minerals>=knob("Z10_HMIN",500)&&drone_count()>=6&&count_buildings(B_HATCHERY,0)<hatch_goal&&
           (count_buildings(B_HATCHERY,0)<knob("Z10_H0",4)||count_units(SIDE,U_HYDRALISK)>=knob("Z10_HFIRST",16))){
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
            group_area_x=-1;group_rounds=6;
            next_rally=frame+500;
        }
        COST(7)
        /* Fresh Hydralisks follow the army up the road in stages. */
        if(rush&&frame>=next_stream){
            group_area_x=ax;group_area_y=ay-60;group_area_r=560;group_rounds=3;
            (void)z10_forward_group(road[0][0],road[0][1],0);
            for(int k=0;k+1<=leg&&k<ROAD_LEGS-1;k+=2){
                int to=k+2<leg?k+2:leg;
                group_area_x=road[k][0];group_area_y=road[k][1];group_area_r=120;group_rounds=2;
                (void)z10_forward_group(road[to][0],road[to][1],0);
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
