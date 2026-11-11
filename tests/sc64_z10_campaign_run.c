/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary-input Z10 "Full Circle" campaign controller.
 * The controller may inspect live state to choose targets; every mutation is
 * an ordinary HOME/menu/gameplay input delivered through game_update().
 *
 * The mission: the human wins when Force 1 has killed the Protoss Temple (raw
 * 174), which creates a Drone and the Khaydarin Crystal (raw 219) beside the
 * hive, and a carrier brings the crystal to the Temple's site (location 4);
 * it loses when it commands no unit. The three Protoss towns keep 80 to 170
 * units in the shared 240-unit pool and send scripted waves at the first
 * Hatchery (6253: 8 Zealots, a Scout and a Reaver from the south-west and
 * east plus 2 Zealots and 2 Archons from the north, then every 9,600 to
 * 10,000 updates). Reavers outrange a Hydralisk, so the controller never
 * leaves the hive's ground:
 *   1. Three Hatcheries early (larvae are the scarce input), a Spawning Pool,
 *      a Hydralisk Den, 17 Drones and six of them on the one Extractor.
 *   2. A fortress of Sunken Colonies north of the hive (the two Creep Colonies
 *      it starts with plus six more) and up to 30 Hydralisks that answer only
 *      what comes within reach of it; macro work waits while a raid is on.
 *   3. After the second wave, a Lair, a Spire and a fleet of Mutalisks
 *      (Zealots and Reavers cannot hit air). The fleet walks the Drone's route
 *      (hive, east side of the central massif, the Temple), killing the
 *      anti-air first, then flattens the Temple.
 *   4. The crystal Drone follows two legs behind the fleet. */
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

/* The way the Drone walks, as the path finder takes it: from the hive up the east
 * side of the central massif and through the middle to the Temple's site. */
static const int route[][2]={{824,1728},{1096,1272},{1040,1020},{900,880},{790,740},{730,590}};
enum { ROUTE_LEGS=sizeof(route)/sizeof(route[0]) };
enum { SITE_X=712, SITE_Y=486 };

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
    for(int k=0;k<ROUTE_LEGS;k++){int d=dist2(x,y,route[k][0],route[k][1]);if(d<bd){bd=d;best=k;}}
    return best;
}

static int ultra_type(void){for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==39)return u[i].type;return -2;}
/* One L-drag box around every unit of `type` within r of (x,y), then one order. */
static int box_command(int type,int x,int y,int r,int which,int tx,int ty,int force_rebox)
{
    int near=0,sel=0,x0=0x7fffffff,y0=0x7fffffff,x1=-1,y1=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type&&dist2(u[i].x,u[i].y,x,y)<r*r){
        ++near;sel+=u[i].selected;
        if(u[i].x<x0)x0=u[i].x;if(u[i].y<y0)y0=u[i].y;if(u[i].x>x1)x1=u[i].x;if(u[i].y>y1)y1=u[i].y;
    }
    if(!near)return 0;
    if(force_rebox||selected_building>=0||sel*100<near*75||selected_count()>near+6){
        int n=box_select(x0-24,y0-24,x1+24,y1+24);
        if(!n)return 0;
        int bad=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].type!=type)bad=1;
        if(bad){
            /* The box caught something else: fall back to the double tap. */
            close_menus();clear_selected_building();
            group_area_x=x;group_area_y=y;group_area_r=r;group_rounds=3;
            int g=group_command(type,which,tx,ty,1);
            group_area_x=-1;group_rounds=6;
            return g?near:0;
        }
    }
    return command_selected(which,tx,ty)?near:0;
}


/* An Extractor on the free Geyser nearest the hive. */
static int build_extractor_free(void)
{
    int best=-1,bd=0x7fffffff;
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(B_EXTRACTOR);
    if(!rule||minerals<(int)rule->minerals)return 0;
    for(int g=0;g<geyser_count;g++){
        int taken=0;
        for(int k=0;k<MAX_BUILDINGS;k++)if(b[k].alive&&b[k].owner==SIDE&&b[k].type==B_EXTRACTOR&&dist2(b[k].x,b[k].y,geysers[g].x,geysers[g].y)<48*48)taken=1;
        if(taken)continue;
        int d=dist2(geysers[g].x,geysers[g].y,HOME_X,HOME_Y);
        if(d<bd){bd=d;best=g;}
    }
    if(best<0||bd>520*520)return 0;
    int worker=free_drone(geysers[best].x,geysers[best].y);
    if(worker<0||!select_single(worker)||!choose_row(MENU_REFINERY))return 0;
    if(mode!=MODE_BUILD||menu!=B_EXTRACTOR){close_menus();return 0;}
    move_cursor(geysers[best].x,geysers[best].y);KEY_TAP(KEY_A);
    if(mode!=MODE_NORMAL)close_menus();
    return SC64_BUILD_WORKER_ORDER(u[worker].order);
}

/* Fill every finished Extractor to `per` Drones. */
static int assign_gas_multi(int per)
{
    for(int r=0;r<MAX_BUILDINGS;r++){
        if(!b[r].alive||b[r].owner!=SIDE||b[r].type!=B_EXTRACTOR||!building_complete(r))continue;
        int have=0,worker=-1;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE){
            if(u[i].order==ORDER_GAS&&u[i].target==r)++have;
            else if(worker<0&&u[i].order==ORDER_MINE&&!u[i].carry)worker=i;
        }
        if(have>=per||worker<0)continue;
        if(!select_single(worker))return 0;
        move_cursor(b[r].x,b[r].y);KEY_TAP(KEY_A);
        return u[worker].order==ORDER_GAS;
    }
    return 0;
}


/* build_near with a reason code for the trace: 1 built, 0 no resources, -1 no site, -2 no worker, -3 select, -4 menu, -5 order refused. */
static int build_once(int type,int menu_row,int anchor_x,int anchor_y,int radius,int *why)
{
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
    int best=0x7fffffff,site_x=-1,site_y=-1;
    if(!rule||minerals<(int)rule->minerals||gas<(int)rule->gas){*why=0;return 0;}
    for(int dy=-radius;dy<=radius;dy+=16)for(int dx=-radius;dx<=radius;dx+=16){
        int x=anchor_x+dx,y=anchor_y+dy,d=dx*dx+dy*dy;
        if(d>=best||x<32||x>=WORLD_W-32||y<32||y>=WORLD_H-32||
           !race_can_site(SIDE,type,x,y)||!can_place(type,x,y)||!site_clear(type,x,y))continue;
        best=d;site_x=x;site_y=y;
    }
    if(site_x<0){*why=-1;return 0;}
    int worker=free_drone(site_x,site_y);
    if(worker<0){*why=-2;return 0;}
    if(!select_single(worker)){*why=-3;return 0;}
    if(!choose_row(menu_row)){*why=-4;return 0;}
    if(mode!=MODE_BUILD||menu!=type){close_menus();*why=-4;return 0;}
    move_cursor(site_x,site_y);KEY_TAP(KEY_A);
    if(mode!=MODE_NORMAL)close_menus();
    if(!SC64_BUILD_WORKER_ORDER(u[worker].order)){
        if(refused_count<64){refused_x[refused_count]=site_x;refused_y[refused_count++]=site_y;}
        *why=-5;return 0;
    }
    fprintf(trace,"{\"kind\":\"build\",\"frame\":%u,\"type\":%d,\"site\":[%d,%d]}\n",(unsigned)frame,type,site_x,site_y);
    *why=1;return 1;
}

static int build_try(int type,int menu_row,int anchor_x,int anchor_y,int radius,int tries,int *why)
{
    int w=0;
    for(int t=0;t<tries;t++){
        if(build_once(type,menu_row,anchor_x,anchor_y,radius,&w))return *why=1;
        if(w!=-5&&w!=-3)break;
    }
    *why=w;return 0;
}

static int next_defend_g,last_box_g,peak_units_g,peak_buildings_g;
static void pool_peaks(void)
{
    int au=0,ab=0;
    for(int i=0;i<MAX_UNITS;i++)au+=u[i].alive!=0;
    for(int i=0;i<MAX_BUILDINGS;i++)ab+=b[i].alive!=0;
    if(au>peak_units_g)peak_units_g=au;
    if(ab>peak_buildings_g)peak_buildings_g=ab;
}
static int raid_since_g=-1,raid_off_until_g;
static int raid_now(void)
{
    int on=0;
    for(int i=0;i<MAX_UNITS&&!on;i++)if(u[i].alive&&u[i].owner==ENEMY&&sc64_race_unit_damage(u[i].type)>0&&!sc64_race_is_worker(u[i].type)&&
        (u[i].order==ORDER_ATTACK_MOVE||u[i].order==ORDER_ATTACK_TARGET)&&dist2(u[i].x,u[i].y,640,1820)<knob("Z10_RAIDR",640)*knob("Z10_RAIDR",640))on=1;
    if(!on){raid_since_g=-1;return 0;}
    if(raid_since_g<0)raid_since_g=(int)frame;
    /* A raid that will not end does not freeze the economy for good. */
    if((int)frame>=raid_off_until_g&&(int)frame-raid_since_g>knob("Z10_RAIDMAX",1500)){raid_off_until_g=(int)frame+knob("Z10_RAIDOFF",500);raid_since_g=-1;}
    if((int)frame<raid_off_until_g)return 0;
    return 1;
}
/* Whatever is at home answers anything that comes for the hives. */
static void defend_now(int def_reach,int fight_period)
{
    int ax=HOME_X,ay=HOME_Y;
    if(frame<next_defend_g)return;
    {
            int foe=-1,fd=def_reach*def_reach,pr=99;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&sc64_race_unit_damage(u[i].type)>0&&!sc64_race_is_worker(u[i].type)){
                int d=dist2(u[i].x,u[i].y,HOME_X,HOME_Y-60);
                int rw=u[i].raw_type;
                int p=knob("Z10_PRIO",1)?(rw==83?0:rw==68?1:rw==66?2:rw==70?2:3):3;
                /* A Reaver shoots from far: reach it from further out than the rest. */
                int lim=(rw==83)?(def_reach+knob("Z10_RVX",60))*(def_reach+knob("Z10_RVX",60)):def_reach*def_reach;
                if(d>=lim)continue;
                if(d>=knob("Z10_NEARD",300)*knob("Z10_NEARD",300)&&u[i].order!=ORDER_ATTACK_MOVE&&u[i].order!=ORDER_ATTACK_TARGET)continue;
                if(p<pr||(p==pr&&d<fd)){pr=p;fd=d;foe=i;}
            }
            if(foe>=0){
                int tx=u[foe].x,ty=u[foe].y;
                int nh=box_command(U_HYDRALISK,ax,ay-80,knob("Z10_DBOX",520),MENU_ATTACK,tx,ty,frame-last_box_g>=300);
                if(nh&&frame-last_box_g>=300)last_box_g=frame;
                group_area_x=ax;group_area_y=ay-80;group_area_r=520;group_rounds=2;
                (void)group_command(ultra_type(),MENU_ATTACK,tx,ty,1);
                (void)group_command(U_ZERGLING,MENU_ATTACK,tx,ty,1);
                group_area_x=-1;group_rounds=6;
                fprintf(trace,"{\"kind\":\"defend\",\"frame\":%u,\"foe\":[%d,%d],\"raw\":%d,\"hydras\":%d}\n",(unsigned)frame,tx,ty,u[foe].raw_type,nh);
                next_defend_g=frame+fight_period;
            } else next_defend_g=frame+200;
    }
}

static int ck_hatch(void){int n=0;for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&sc64_race_is_zerg_townhall(b[i].type)&&building_complete(i);return n;}
static int ck_larva(void){int n=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&sc64_race_is_zerg_townhall(b[i].type))n+=b[i].larva;return n;}
static int ck_queued(void){int n=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&sc64_race_is_zerg_townhall(b[i].type))n+=b[i].queue_count;return n;}
static void census(const char *label,int leg)
{
    int eb=0;
    for(int i=0;i<MAX_BUILDINGS;i++)eb+=b[i].alive&&b[i].owner==ENEMY;
    fprintf(trace,"{\"kind\":\"checkpoint\",\"label\":\"%s\",\"frame\":%u,\"call_index\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,"
        "\"side_units\":%d,\"hydras\":%d,\"mutas\":%d,\"hatch\":%d,\"larva\":%d,\"queued\":%d,\"sunk\":%d,\"drones\":%d,\"enemy_units\":%d,\"enemy_buildings\":%d,\"leg\":%d,\"bank\":[%d,%d],"
        "\"supply\":[%d,%d],\"cursor\":[%d,%d],\"countdown\":0,\"towns\":%u,\"fault\":%u,\"peak_units\":%d,\"peak_buildings\":%d}\n",
        label,(unsigned)frame,host_tick,screen,won,lost,count_units(SIDE,-1),count_units(SIDE,U_HYDRALISK),count_units(SIDE,U_MUTALISK),ck_hatch(),ck_larva(),ck_queued(),count_buildings(B_SUNKEN_COLONY,1),drone_count(),
        count_units(ENEMY,-1),eb,leg,minerals,gas,race_owner_supply(SIDE),race_owner_supply_cap(SIDE),cx,cy,
        (unsigned)race_zl_ai.started,(unsigned)race_zl_sched.core.fault,peak_units_g,peak_buildings_g);
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
    int saved=0,turn=0,front=0,next_checkpoint=2000,next_mine=0,next_train=0,next_build=60,next_gas=500,next_hatch=300,next_rally=900,
        next_save_build=0,next_runner=0,runner=-1,next_lair=0,next_muta=0,muta_go=0,next_mgather=0,
        next_colony=0,next_fort=0,next_reinforce=0,taken=0;
    const int hatch_goal=knob("Z10_HATCH",6),gas_drones=3,hydra_cap=knob("Z10_CAP",30),muta_cap=knob("Z10_MUTAS",40),
        muta_go_count=knob("Z10_MGO",34),muta_floor=knob("Z10_MFLOOR",5),muta_reach=knob("Z10_MREACH",300),
        def_reach=knob("Z10_DEFR",260),fight_period=90;
    int raid=0;
    while(host_tick<limit_ticks&&screen==SCREEN_PLAY&&!won&&!lost){
        pool_peaks();
        if(!taken&&crystal()>=0){taken=1;fprintf(trace,"{\"kind\":\"crystal\",\"frame\":%u,\"mutas\":%d}\n",(unsigned)frame,count_units(SIDE,U_MUTALISK));}
        if(menu_open)KEY_TAP(KEY_B);
        int mark=frame,ax=HOME_X,ay=HOME_Y;
        int c=crystal(),carrier=c>=0&&race_zl_crystal_carrier!=SC64_ZL_NO_CARRIER?race_zl_crystal_carrier:-1;
        /* The crystal walks the route, never past the Mutalisk front. */
        if(carrier>=0){
            int at=nearest_leg(u[carrier].x,u[carrier].y);
            int to=at+1<ROUTE_LEGS?at+1:ROUTE_LEGS-1;
            if(to>front)to=front;
            if(to<at)to=at;
            int tx=route[to][0],ty=route[to][1];
            if(at==ROUTE_LEGS-1||(to==ROUTE_LEGS-1&&dist2(u[carrier].x,u[carrier].y,route[ROUTE_LEGS-1][0],route[ROUTE_LEGS-1][1])<120*120)){tx=SITE_X;ty=SITE_Y;}
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
        defend_now(def_reach,fight_period);
        raid=raid_now();
        if(c>=0&&carrier>=0){
            /* The Mutalisks stay two legs ahead of the Drone. */
            int at=nearest_leg(u[carrier].x,u[carrier].y);
            front=at+2<ROUTE_LEGS?at+2:ROUTE_LEGS-1;
        }
#define COST(n) if(frame-mark>=20){turn=n;input(0);continue;} case n: defend_now(def_reach,fight_period);
        switch(turn){ case 0:;
        if(!saved&&frame>=knob("Z10_SAVE",2500)){
            int pick=free_drone(-1,-1);
            if(under_construction()&&pick>=0){
                if(clear_menu_last(pick)&&pause_menu_save(path,pick)){saved=1;census("saved",front);}
            } else if(frame>=next_save_build&&minerals>=75){
                (void)build_near(B_CREEP_COLONY,MENU_Z7_CREEP,ax-60,ay-90,96);
                next_save_build=frame+300;
            }
        }
        COST(10)
        COST(1)
        if(frame>=next_mine&&!(raid&&knob("Z10_RAIDGATE",1))){if(minerals<2000&&c<0)(void)order_idle_drones(3);next_mine=frame+400;}
        COST(2)
        if(frame>=next_train){
            int done=0;
            for(int i=0;i<MAX_BUILDINGS;i++){
                if(!b[i].alive||b[i].owner!=SIDE||!sc64_race_is_zerg_townhall(b[i].type)||!b[i].larva||
                   !building_complete(i)||dist2(b[i].x,b[i].y,ax,ay)>=520*520)continue;
                while(b[i].larva>0){
                    int spare=race_owner_supply_cap(SIDE)-race_owner_supply(SIDE),type=-1;
                    int dgoal=frame<1500?knob("Z10_D1",9):frame<4000?knob("Z10_D2",13):knob("Z10_D3",17);
                    int spire=count_buildings(B_SPIRE,1),mutas=count_units(SIDE,U_MUTALISK)+queued_units(U_MUTALISK);
                    int hy=count_units(SIDE,U_HYDRALISK)+queued_units(U_HYDRALISK);
                    int hyneed=count_buildings(B_HYDRALISK_DEN,1)&&hy<knob("Z10_HYF",8)&&frame>=knob("Z10_HYFT",3500)&&drone_count()>=knob("Z10_HYFD",10);
                    if(spare<knob("Z10_OVL",28)&&minerals>=100&&queued_units(U_OVERLORD)<knob("Z10_OVLQ",2))type=U_OVERLORD;
                    else if(drone_count()+queued_units(U_DRONE)<dgoal&&minerals>=50&&!hyneed&&spare>=2)type=U_DRONE;
                    else if(spire&&minerals>=100&&gas>=100&&spare>=4&&mutas<muta_cap)type=U_MUTALISK;
                    else if(count_buildings(B_HYDRALISK_DEN,1)&&minerals>=75&&gas>=25+(spire?0:knob("Z10_GFLOOR",0))&&spare>=2&&hy<hydra_cap)type=U_HYDRALISK;
                    if(type<0||!train_at(i,type))break;
                    ++done;
                }
            }
            next_train=frame+(done?60:120);
        }
        COST(3)
        if(frame>=next_build&&!(raid&&knob("Z10_RAIDGATE",1))){
            int poolc=count_buildings(B_SPAWNING_POOL,1),lairc=count_buildings(B_LAIR,1)+count_buildings(B_HIVE,1);
            int ext=count_buildings(B_EXTRACTOR,0),dr=drone_count();
            /* In order of want: the first item whose needs stand but whose price is short
               holds the rest back. */
            enum { W_POOL,W_EXT1,W_LAIR,W_SPIRE,W_DEN,W_COUNT };
            for(int w=0;w<W_COUNT;w++){
                int need_m=0,need_g=0,ready=0,done=0;
                switch(w){
                case W_POOL: done=count_buildings(B_SPAWNING_POOL,0)>0;need_m=200;ready=dr>=2&&frame>=knob("Z10_POOL",60);break;
                case W_EXT1: done=ext>=1;need_m=50;ready=dr>=knob("Z10_E1D",5)&&frame>=knob("Z10_EXT",7000);break;
                case W_LAIR: done=count_buildings(B_LAIR,0)>0;need_m=150;need_g=100;ready=poolc&&frame>=next_lair&&frame>=knob("Z10_LAIRT",13000);break;
                case W_SPIRE: done=count_buildings(B_SPIRE,0)>0;need_m=200;need_g=150;ready=lairc&&dr>=6&&frame>=knob("Z10_SPIRET",0);break;
                case W_DEN: done=count_buildings(B_HYDRALISK_DEN,0)>0;need_m=100;need_g=50;ready=poolc&&dr>=5&&frame>=knob("Z10_DEN",1450);break;
                }
                if(done||!ready)continue;
                if(minerals<need_m||gas<need_g)break;
                int res=0;
                switch(w){
                case W_POOL: {int w;res=build_try(B_SPAWNING_POOL,MENU_BARRACKS,ax+60,ay-110,128,knob("Z10_TRIES",5),&w);};break;
                case W_EXT1: res=build_extractor_free();break;
                case W_DEN: {int w;res=build_try(B_HYDRALISK_DEN,MENU_ACADEMY,ax-90,ay-90,128,knob("Z10_TRIES",5),&w);};break;
                case W_SPIRE: {int why=0;
                    static const int sa[][2]={{836,1688},{744,1756},{900,1800},{600,1700}};
                    for(unsigned q=0;q<4&&!res;q++)res=build_try(B_SPIRE,MENU_ZP_SPIRE,sa[q][0],sa[q][1],100,2,&why);break;}
                case W_LAIR: {
                    int h=-1,bd=0x7fffffff;
                    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_HATCHERY&&building_complete(i)&&!b[i].queue_count){int d=knob("Z10_LAIRNEW",1)?-i:dist2(b[i].x,b[i].y,ax,ay);if(d<bd){bd=d;h=i;}}
                    next_lair=frame+(h>=0?400:60);
                    if(h>=0&&select_building(h)){
                        int ok=choose_row(MENU_ZP_LAIR);
                        close_menus();clear_selected_building();
                        fprintf(trace,"{\"kind\":\"lair\",\"frame\":%u,\"ok\":%d,\"lair\":%d}\n",(unsigned)frame,ok,count_buildings(B_LAIR,0));
                    }
                    res=1;break;}
                }
                if(!res&&knob("Z10_SKIPFAIL",1))continue;
                break;
            }
            next_build=frame+knob("Z10_BI",115);
        }
        COST(4)
        {
            int nh=count_buildings(B_HATCHERY,0)+count_buildings(B_LAIR,0)+count_buildings(B_HIVE,0);
            if(frame>=next_hatch&&!(raid&&knob("Z10_RAIDGATE",1))&&minerals>=knob("Z10_HMIN",300)&&drone_count()>=knob("Z10_HDR",1)&&nh<hatch_goal&&count_buildings(B_SPAWNING_POOL,0)&&
               frame>=(nh<2?0:nh<3?knob("Z10_H3T",0):nh<4?knob("Z10_H4T",4000):nh<5?knob("Z10_H5T",7000):knob("Z10_H6T",10000))){
                /* North of the hive, in the open: walled-in Hatcheries trap their larvae. */
                static const int spots[][2]={{560,1890},{850,1890},{704,1760},{980,1800},{420,1800},{540,1740}};
                int k=nh-1;
                if(k>=0&&k<6){int w;(void)build_try(B_HATCHERY,MENU_DEPOT,spots[k][0],spots[k][1],120,knob("Z10_TRIES",5),&w);}
                next_hatch=frame+300;
            }
        }
        COST(5)
        if(frame>=next_gas&&!(raid&&knob("Z10_RAIDGATE",1))&&frame>=knob("Z10_GASAT",900)){(void)assign_gas_multi(knob("Z10_GASPER",6));next_gas=frame+300;}
        COST(7)
        /* The two starting Creep Colonies become Sunken Colonies once the pool stands. */
        if(frame>=next_colony&&!(raid&&knob("Z10_RAIDGATE",1))&&count_buildings(B_SPAWNING_POOL,1)){
            next_colony=frame+120;
            int colony=-1;
            for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_CREEP_COLONY&&building_complete(i)&&
                !(b[i].flags&(SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG)))colony=i;
            if(colony>=0&&minerals>=50&&frame>=knob("Z10_SUNK",0)&&select_building(colony)){
                int ok=race_colony_morph_available_for(colony,B_SUNKEN_COLONY)&&choose_row(MENU_STOP+1);
                close_menus();clear_selected_building();
                fprintf(trace,"{\"kind\":\"sunken\",\"frame\":%u,\"ok\":%d}\n",(unsigned)frame,ok);
            }
        }
        COST(12)
        /* More Creep Colonies north of the hive, for the colony step to turn into Sunken Colonies. */
        if(frame>=next_fort&&!(raid&&knob("Z10_RAIDGATE",1))&&count_buildings(B_SPAWNING_POOL,1)&&frame>=knob("Z10_SUNKT",1400)){
            next_fort=frame+knob("Z10_FORTI",120);
            int have=0;for(int i=0;i<MAX_BUILDINGS;i++)have+=b[i].alive&&b[i].owner==SIDE&&(b[i].type==B_CREEP_COLONY||b[i].type==B_SUNKEN_COLONY);
            static const int fort[][2]={{704,1820},{640,1830},{770,1830},{580,1850},{830,1850},{704,1770},{640,1780},{770,1780},{510,1830},{900,1830},{704,1720},{580,1760},{830,1770},{450,1880},{960,1880},{700,1680}};
            int want=2+knob("Z10_SUNKS",6);
            int hyall=count_units(SIDE,U_HYDRALISK)+queued_units(U_HYDRALISK);
            if(have<want&&drone_count()>=knob("Z10_FORTD",5)&&minerals>=knob("Z10_FORTM",175)&&(hyall>=knob("Z10_FORTH",6)||minerals>=knob("Z10_FORTBIG",700))){
                int r=0,tried=0,kk=-1;
                for(unsigned k=0;k<sizeof(fort)/sizeof(fort[0])&&!r&&tried<knob("Z10_FORTTRY",4);k++){
                    int occ=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&(b[i].type==B_CREEP_COLONY||b[i].type==B_SUNKEN_COLONY)&&dist2(b[i].x,b[i].y,fort[k][0],fort[k][1])<44*44)occ=1;
                    if(occ)continue;
                    ++tried;kk=(int)k;
                    {int w;r=build_try(B_CREEP_COLONY,MENU_Z7_CREEP,fort[k][0],fort[k][1],40,knob("Z10_TRIES",5),&w);}
                }
                fprintf(trace,"{\"kind\":\"fort\",\"frame\":%u,\"k\":%d,\"have\":%d,\"ok\":%d,\"m\":%d}\n",(unsigned)frame,kk,have,r,minerals);
                if(!r)next_fort=frame+knob("Z10_FORTRETRY",600);
            }
        }
        COST(8)
        /* Idle fighters wait between the hives. */
        if(frame>=next_rally&&!(raid&&knob("Z10_RAIDGATE",1))){
            group_area_x=ax;group_area_y=ay-60;group_area_r=560;group_rounds=3;
            (void)group_command(U_HYDRALISK,MENU_ATTACK,640,1790,0);
            (void)group_command(U_ZERGLING,MENU_ATTACK,640,1790,0);
            (void)group_command(ultra_type(),MENU_ATTACK,640,1790,0);
            group_area_x=-1;group_rounds=6;
            next_rally=frame+500;
        }
        COST(9)
        /* The Mutalisk fleet. */
        if(frame>=next_muta){
            next_muta=frame+fight_period;
            int nm=0,bn=0,bx=0,by=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_MUTALISK){
                nm++;
                int n=0,sx=0,sy=0;
                for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&u[k].type==U_MUTALISK&&dist2(u[k].x,u[k].y,u[i].x,u[i].y)<260*260){++n;sx+=u[k].x;sy+=u[k].y;}
                if(n>bn){bn=n;bx=sx/n;by=sy/n;}
            }
            if(!muta_go&&bn>=muta_go_count&&frame>=knob("Z10_MGOT",0)){muta_go=1;fprintf(trace,"{\"kind\":\"muta_go\",\"frame\":%u,\"mutas\":%d,\"fleet\":%d}\n",(unsigned)frame,nm,bn);}
            if(muta_go&&nm<muta_floor){muta_go=0;front=0;fprintf(trace,"{\"kind\":\"muta_stop\",\"frame\":%u,\"mutas\":%d}\n",(unsigned)frame,nm);}
            if(nm&&frame>=next_mgather&&(!muta_go||(c<0&&bn<nm))){
                /* Stragglers and the fleet in waiting gather at the hive's rally point. */
                next_mgather=frame+(muta_go?600:900);
                int tx=muta_go?bx:660,ty=muta_go?by:1760;
                for(int round=0;round<3;round++){
                    int an=-1;
                    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_MUTALISK&&dist2(u[i].x,u[i].y,tx,ty)>(muta_go?420:200)*(muta_go?420:200)){an=i;break;}
                    if(an<0)break;
                    (void)box_command(U_MUTALISK,u[an].x,u[an].y,260,MENU_MOVE,tx,ty,1);
                }
            }
            if(muta_go&&nm&&bn){
                int mx=bx,my=by;
                /* The front walks the route: kill whatever could threaten the Drone near it. */
                static const int prioB[]={83,66,70,71,72,68,67,65,69,64};
                static const int prioA[]={66,70,68,67,71,72};
                const int *prio=c>=0?prioB:prioA;
                const unsigned nprio=c>=0?sizeof(prioB)/sizeof(prioB[0]):sizeof(prioA)/sizeof(prioA[0]);
                const unsigned cpos=c>=0?7:6;
                int fx=route[front][0],fy=route[front][1],tgt=-1,tb=-1,bd=0x7fffffff,RM=muta_reach;
                for(unsigned pi=0;pi<=nprio&&tgt<0&&tb<0;pi++){
                    if(pi==cpos)for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&b[i].raw_type==RAW_CANNON&&dist2(b[i].x,b[i].y,fx,fy)<(RM-100)*(RM-100)){
                        int d=dist2(b[i].x,b[i].y,mx,my);if(d<bd){bd=d;tb=i;}
                    }
                    if(tb>=0)break;
                    if(pi<nprio)for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==prio[pi]&&!SC64_TRANSPORT_LOADED(&u[i])&&dist2(u[i].x,u[i].y,fx,fy)<RM*RM){
                        int d=dist2(u[i].x,u[i].y,mx,my);if(d<bd){bd=d;tgt=i;}
                    }
                }
                int gx=-1,gy=-1,gt=0;
                if(tgt>=0){gx=u[tgt].x;gy=u[tgt].y;gt=u[tgt].raw_type;}
                else if(tb>=0){gx=b[tb].x;gy=b[tb].y;gt=RAW_CANNON;}
                int rebox=frame-last_box_g>=300;
                if(gx>=0){
                    int r=box_command(U_MUTALISK,mx,my,300,MENU_ATTACK,gx,gy,rebox);
                    if(rebox)last_box_g=frame;
                    fprintf(trace,"{\"kind\":\"muta_order\",\"frame\":%u,\"mutas\":%d,\"fleet\":%d,\"target\":[%d,%d],\"type\":%d,\"front\":%d,\"r\":%d}\n",(unsigned)frame,nm,bn,gx,gy,gt,front,r);
                } else if(c<0){
                    if(front<ROUTE_LEGS-1&&dist2(mx,my,fx,fy)<260*260){++front;fprintf(trace,"{\"kind\":\"muta_front\",\"frame\":%u,\"front\":%d,\"mutas\":%d,\"fleet\":%d}\n",(unsigned)frame,front,nm,bn);}
                    int t=temple();
                    if(front==ROUTE_LEGS-1&&t>=0&&dist2(mx,my,route[front][0],route[front][1])<200*200){
                        (void)box_command(U_MUTALISK,mx,my,300,MENU_ATTACK,b[t].x,b[t].y,1);
                                        fprintf(trace,"{\"kind\":\"strike\",\"frame\":%u,\"mutas\":%d,\"hp\":%d}\n",(unsigned)frame,bn,b[t].hp);
                    } else {int r=box_command(U_MUTALISK,mx,my,300,MENU_ATTACK,route[front][0],route[front][1],rebox);if(rebox)last_box_g=frame;}
                } else {
                    (void)box_command(U_MUTALISK,mx,my,300,MENU_ATTACK,route[front][0],route[front][1],frame-last_box_g>=300);
                }
            }
        }
        COST(11)
        }
        turn=0;
        if(frame>=next_checkpoint){census("progress",front);next_checkpoint=frame+2000;}
        input(0);
    }
    census("final",front);
    fclose(trace);
    printf("z10 controller: frame=%u calls=%u won=%d lost=%d screen=%d fault=%u drones=%d hydras=%d mutas=%d front=%d temple=%d crystal=%d peak_units=%d peak_buildings=%d\n",
        (unsigned)frame,host_tick,won,lost,screen,(unsigned)race_zl_sched.core.fault,drone_count(),count_units(SIDE,U_HYDRALISK),count_units(SIDE,U_MUTALISK),front,temple()>=0,crystal()>=0,peak_units_g,peak_buildings_g);
    return won?0:1;
}
