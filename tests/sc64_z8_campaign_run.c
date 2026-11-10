/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary-input Z8 "Eye for an Eye" campaign controller.
 * The controller may inspect live state to choose targets; every mutation is
 * an ordinary HOME/menu/gameplay input delivered through game_update(). */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include "sc64_zerg_late_controller.h"
#include <assert.h>

#ifndef SC64_Z8_RUNTIME
#error "Compile this controller with the zerg-z8 profile flags"
#endif
#ifndef Z8_RUN_LIMIT
#define Z8_RUN_LIMIT 200000u
#endif

enum { RAW_NEXUS=154, RAW_HERO_DT=74, RAW_KERRIGAN=51, RAW_OVERLORD=42, RAW_NYDUS=134 };

/* Three human hive clusters and where each guard waits for raids. Raids
 * outrun Zerglings, so a guard waits just short of its defeat rectangle. */
typedef struct { int hatch_x,hatch_y,guard_x,guard_y; } Guard;
enum { GUARD_NE, GUARD_NW, GUARD_SE };
static Guard guards[3]={
    {1712,168,1950,330},
    {352,136,90,240},
    {1824,1832,1300,1860},
};

typedef struct { int at,in_x,in_y,out_x,out_y,g,state,since; } Crossing;

static int hatch_at(int x,int y)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        sc64_race_is_zerg_townhall(b[i].type)&&dist2(b[i].x,b[i].y,x,y)<64*64)return i;
    return -1;
}

static int nexus(int raw_owner)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==RAW_NEXUS&&b[i].raw_owner==raw_owner)return i;
    return -1;
}

static int kerrigan(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==RAW_KERRIGAN&&u[i].owner==SIDE)return i;
    return -1;
}

static int raid_unit(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==RAW_HERO_DT&&u[i].owner==ENEMY)return i;
    return -1;
}

static int isqrt_approx(int v)
{
    int r=0;
    while((r+1)*(r+1)<=v)++r;
    return r;
}

static int army_near(int x,int y,int radius)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&
        (u[i].type==U_HYDRALISK||u[i].type==U_ZERGLING)&&dist2(u[i].x,u[i].y,x,y)<radius*radius;
    return n;
}

static int army_count(void)
{
    return count_units(SIDE,U_HYDRALISK)+count_units(SIDE,U_ZERGLING);
}

/* Combat helpers. A Reaver outranges and out-sees a Hydralisk, so a group that waits
 * in front of a garrison is shot down piecemeal; the army attack-moves into it at
 * once. One L-drag box orders every Hydralisk in it with a single command, where
 * double-taps reach about a screen of them. */
static int isqrt_i(int v){int lo=0,hi=46340;while(lo<hi){int mid=(lo+hi+1)/2;if(mid*mid<=v)lo=mid;else hi=mid-1;}return lo;}
static int foe_combat(int raw){return raw==65||raw==66||raw==67||raw==68||raw==70||raw==71||raw==72||raw==83;}
static int nearest_foe(int x,int y,int *dd){
    int best=-1,bd=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&foe_combat(u[i].raw_type)&&!SC64_TRANSPORT_LOADED(&u[i])){int d=dist2(u[i].x,u[i].y,x,y);if(d<bd){bd=d;best=i;}}
    if(dd)*dd=best>=0?isqrt_i(bd):9999;
    return best;
}
static int hydra_box_attack(int x,int y,int r,int tx,int ty,int force_rebox)
{
    int near=0,sel=0,x0=0x7fffffff,y0=0x7fffffff,x1=-1,y1=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&dist2(u[i].x,u[i].y,x,y)<r*r){
        ++near;sel+=u[i].selected;
        if(u[i].x<x0)x0=u[i].x;if(u[i].y<y0)y0=u[i].y;if(u[i].x>x1)x1=u[i].x;if(u[i].y>y1)y1=u[i].y;
    }
    if(!near)return 0;
    /* A box takes everything in it: leave Kerrigan, Overlords and Drones out of it. */
    int foreign=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type!=U_HYDRALISK&&
        u[i].x>=x0-24&&u[i].x<=x1+24&&u[i].y>=y0-24&&u[i].y<=y1+24)++foreign;
    if(foreign){
        group_area_x=x;group_area_y=y;group_area_r=r;group_rounds=8;
        int g=group_command(U_HYDRALISK,MENU_ATTACK,tx,ty,1);
        group_area_x=-1;group_rounds=6;
        return g?near:0;
    }
    if(force_rebox||selected_building>=0||sel*100<near*75||selected_count()>near+6){
        int n=box_select(x0-24,y0-24,x1+24,y1+24);
        if(!n)return 0;
    }
    return command_selected(MENU_ATTACK,tx,ty)?near:0;
}
/* Top up every finished Extractor to `wanted` gas Drones. */
static int assign_gas_all(int wanted)
{
    for(int r=0;r<MAX_BUILDINGS;r++)if(b[r].alive&&b[r].owner==SIDE&&b[r].type==B_EXTRACTOR&&building_complete(r)){
        int have=0,worker=-1;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE){
            if(u[i].order==ORDER_GAS&&u[i].target==r)++have;
            else if(worker<0&&u[i].order==ORDER_MINE&&!u[i].carry)worker=i;
        }
        if(have>=wanted||worker<0||!select_single(worker))continue;
        move_cursor(b[r].x,b[r].y);KEY_TAP(KEY_A);
        return u[worker].order==ORDER_GAS;
    }
    return 0;
}

static void census(const char *label)
{
    int enemy_b=0,k=kerrigan(),n1=nexus(1),n2=nexus(2);
    for(int i=0;i<MAX_BUILDINGS;i++)enemy_b+=b[i].alive&&b[i].owner==ENEMY;
    fprintf(trace,"{\"kind\":\"checkpoint\",\"label\":\"%s\",\"frame\":%u,\"call_index\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,"
        "\"side_units\":%d,\"army\":%d,\"drones\":%d,\"enemy_units\":%d,\"enemy_buildings\":%d,"
        "\"nexus\":[%d,%d],\"kerrigan\":%d,\"bank\":[%d,%d],\"supply\":[%d,%d],\"cursor\":[%d,%d],"
        "\"guards\":[%d,%d,%d],\"fault\":%u}\n",
        label,(unsigned)frame,host_tick,screen,won,lost,count_units(SIDE,-1),army_count(),drone_count(),
        count_units(ENEMY,-1),enemy_b,n1>=0?b[n1].hp+b[n1].shield:-1,n2>=0?b[n2].hp+b[n2].shield:-1,k>=0?u[k].hp:-1,
        minerals,gas,race_owner_supply(SIDE),race_owner_supply_cap(SIDE),cx,cy,
        army_near(guards[0].guard_x,guards[0].guard_y,280),army_near(guards[1].guard_x,guards[1].guard_y,280),
        army_near(guards[2].guard_x,guards[2].guard_y,280),(unsigned)race_z8_sched.fault);
    fflush(trace);
}

/* Kerrigan's death ends the mission. Waves aim at the first living Hatchery by
 * slot, so the southeastern hive is attacked last: walk her into the Nydus
 * beside the northeastern Hatchery, which exits there. */
static int kerrigan_to_nydus(void)
{
    int k=kerrigan(),nydus=-1,bd=0x7fffffff;
    if(k<0)return 0;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].raw_type==RAW_NYDUS){
        int d=dist2(b[i].x,b[i].y,1712,288);
        if(d<bd){bd=d;nydus=i;}
    }
    if(nydus<0||!select_single(k))return 0;
    move_cursor(b[nydus].x,b[nydus].y);KEY_TAP(KEY_A);
    return u[k].order==ORDER_NYDUS_ENTER;
}

/* Trip to a far hive: train what its larva allow, then rally to its guard. */
static void visit_hive(int g,int unit_type)
{
    Guard *q=&guards[g];
    int h=hatch_at(q->hatch_x,q->hatch_y),overlord=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==RAW_OVERLORD&&
        dist2(u[i].x,u[i].y,q->hatch_x,q->hatch_y)<420*420)overlord=i;
    if(overlord<0&&h>=0&&minerals>=100&&(g==GUARD_NW||!b[h].queue_count))(void)train_at(h,U_OVERLORD);
    for(int n=0;n<3&&h>=0&&train_at(h,unit_type);n++){}
    close_menus();clear_selected_building();
    group_area_x=q->hatch_x;group_area_y=q->hatch_y;group_area_r=240;
    (void)group_command(unit_type,MENU_ATTACK,q->guard_x,q->guard_y,0);
    group_area_x=-1;
}

/* Idle units ignore a raid walking past. Attack-moving in place lets them
 * acquire it, and staying put keeps them ahead of it. */
static int intercept_raid(int raid)
{
    int sx=0,sy=0,n=0,acted=0;
    for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&u[k].type!=U_DRONE&&
        dist2(u[k].x,u[k].y,u[raid].x,u[raid].y)<320*320){sx+=u[k].x;sy+=u[k].y;++n;}
    if(!n)return 0;
    sx/=n;sy/=n;
    if(n>10){
        int gx=sx,gy=sy,gd=0x7fffffff;
        for(int g=0;g<3;g++){
            int d=dist2(sx,sy,guards[g].guard_x,guards[g].guard_y);
            if(d<gd){gd=d;gx=guards[g].guard_x;gy=guards[g].guard_y;}
        }
        group_area_x=gx;group_area_y=gy;group_area_r=280;group_rounds=1;
        acted+=group_command(U_HYDRALISK,MENU_ATTACK,gx,gy,1);
        acted+=group_command(U_ZERGLING,MENU_ATTACK,gx,gy,1);
        group_area_x=-1;group_rounds=6;
    } else if(box_select(sx-48,sy-36,sx+48,sy+36))acted+=attack_moving_unit(raid);
    return acted;
}

int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:".local/traces/z8-controller.jsonl";
    unsigned limit_ticks=argc>2?(unsigned)strtoul(argv[2],0,10):Z8_RUN_LIMIT;
    trace=fopen(path,"w");if(!trace){perror(path);return 2;}
    fprintf(trace,"{\"kind\":\"metadata\",\"schema\":1,\"mission\":%d,\"profile\":\"ZERG_Z8_CANONICAL_FLAGS\",\"input_policy\":\"ordinary game_update held/pressed only\"}\n",SC64_Z8_GLOBAL_ID);
    game_init();prior=0;
    launch_catalog_mission(SC64_Z8_GLOBAL_ID);
    assert(screen==SCREEN_PLAY&&!won&&!lost&&race_selected_mission==SC64_Z8_GLOBAL_ID);
    census("launch");

    int saved=0,kerrigan_sent=0,opened=0,ne_overlord=0;
    int next_checkpoint=2000,next_mine=0,next_train=0,next_build=120,next_gas=600,next_order=600;
    int next_hatch=1500,next_rally=3000,next_visit[3]={0,4400,7000};
    int next_se_eco=0,next_defend=0,rush=0,rush_west=0,army_x=0,army_y=0,army_r=0,posted=0,turn=0,army_leg=0,mustered=0,next_status=0,next_detector=0,next_save_build=0,raid_guard=-1,raid_last_x=0,raid_last_y=0,raid_still_since=0,fields=0,fields_since=0,next_research=0;
    /* Upgrades, the raid-quiet window and the charge: see the notes at each use. */
    const int tech_after=14000,quiet_lead=2400,gas_per_extractor=3,contact=320,box_reach=420,rebox_period=300,fight_period=90,hold_lead=2600;
    int last_box=-100000;
    const int drone_goal=16,hatch_goal=8,gas_drones=3;
    while(host_tick<limit_ticks&&screen==SCREEN_PLAY&&!won&&!lost){
        if(menu_open)KEY_TAP(KEY_B);
        int ne=hatch_at(guards[GUARD_NE].hatch_x,guards[GUARD_NE].hatch_y);
        int ax=guards[GUARD_NE].hatch_x,ay=guards[GUARD_NE].hatch_y;
        int stage_x=1680,stage_y=520;
        int mark=frame,raid=raid_unit();
        /* Raid spawn frames follow the source's elapsed-time triggers, and each
           raid heads for a known guard. */
        static const struct { int at,g,town; } raids[]={{1845,GUARD_SE,1},{10448,GUARD_NE,2},{12850,GUARD_SE,2},
            {20872,GUARD_NW,1},{26930,GUARD_NW,2},{34170,GUARD_SE,1},{46230,GUARD_SE,2},{55880,GUARD_NW,1},{68340,GUARD_NW,2},{80000,GUARD_SE,1}};
        enum { RAID_ROWS=sizeof(raids)/sizeof(raids[0]) };
        /* Upgrades and gas Drones move the cursor away from a guard: they wait out the
           stretch from 2,400 frames before a raid to 900 frames after its spawn. */
        int quiet=1;
        for(int r=0;r<RAID_ROWS;r++)if((int)frame+quiet_lead>=raids[r].at&&(int)frame<raids[r].at+900)quiet=0;
        if(raid>=0){
            if(FAST_ABS(u[raid].x-raid_last_x)>8||FAST_ABS(u[raid].y-raid_last_y)>8){
                raid_last_x=u[raid].x;raid_last_y=u[raid].y;raid_still_since=frame;
            }
            raid_guard=GUARD_SE;
            /* The spawn clock runs a little ahead of the frame count, and further
               when menus held the game: take the row nearest the frame. */
            for(unsigned r=0;r<RAID_ROWS;r++)if(frame>=raids[r].at-600)raid_guard=raids[r].g;
        }
        /* A live raid takes every input once the cursor must leave for its
           guard: the cursor crosses at 4 pixels a frame, the raid at 2. */
        if(raid>=0&&opened){
            Guard *q=&guards[raid_guard];
            int trip=isqrt_approx(dist2(cx,cy,q->guard_x,q->guard_y));
            /* After the opening nothing else can pay for the trip back: wait out
               the raid at its guard, unless it has stopped somewhere for good. */
            int lead=frame<3000?trip/2+300:(frame-raid_still_since<900?0x3fff:trip/2+500);
            int to_guard=dist2(u[raid].x,u[raid].y,q->guard_x,q->guard_y);
            if(to_guard<lead*lead){
                int bd=0x7fffffff;
                for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&
                    (u[k].type==U_HYDRALISK||u[k].type==U_ZERGLING||u[k].raw_type==RAW_KERRIGAN)){
                    int d=dist2(u[k].x,u[k].y,u[raid].x,u[raid].y);
                    if(d<bd)bd=d;
                }
                int hidden=race_z8_unit_hidden_from_side(raid);
                if(!hidden&&bd<320*320)(void)intercept_raid(raid);
                else if(hidden&&bd<160*160&&frame>=next_detector&&frame-raid_still_since>=90){
                    /* Stopped against the squad and unseen: fetch an Overlord. */
                    int o=-1,od=0x7fffffff;
                    for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&u[k].raw_type==RAW_OVERLORD){
                        int d=dist2(u[k].x,u[k].y,u[raid].x,u[raid].y);
                        if(d<od){od=d;o=k;}
                    }
                    if(o>=0)(void)unit_order(o,MENU_MOVE,u[raid].x,u[raid].y);
                    next_detector=frame+600;
                }
                else if(dist2(cx,cy,q->guard_x,q->guard_y)>64*64){
                    close_menus();
                    move_cursor(q->guard_x,q->guard_y);
                }
                input(0);
                continue;
            }
        }
        /* The box order that stopped a raid also moved the squad's Overlord and
           Kerrigan. Put them back over the guard before the next raid. */
        if(raid<0&&raid_guard>=0){
            Guard *q=&guards[raid_guard];
            /* Raids reach this rectangle by two roads, north and west. After the
               first one, wait at the corner where they meet. */
            if(raid_guard==GUARD_SE){q->guard_x=1240;q->guard_y=1930;}
            int o=-1,od=0x7fffffff,k=kerrigan();
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==RAW_OVERLORD){
                int d=dist2(u[i].x,u[i].y,q->guard_x,q->guard_y);
                if(d<od){od=d;o=i;}
            }
            /* The southern Overlord sits up the road both raid routes share,
               so the squad sees a raid coming. */
            int ov_x=raid_guard==GUARD_SE?1250:q->guard_x,ov_y=raid_guard==GUARD_SE?1880:q->guard_y;
            if(o>=0&&(dist2(u[o].x,u[o].y,ov_x,ov_y)>48*48||dist2(u[o].tx,u[o].ty,ov_x,ov_y)>48*48))
                (void)unit_order(o,MENU_MOVE,ov_x,ov_y);
            if(k>=0&&raid_guard==GUARD_SE&&dist2(u[k].x,u[k].y,q->guard_x+24,q->guard_y+24)>64*64)
                (void)unit_order(k,MENU_MOVE,q->guard_x+24,q->guard_y+24);
            close_menus();
            raid_guard=-1;
        }
        /* Squads cross by Nydus: one reinforces the south before the third raid,
           one the corner before its Nydus exit falls to the waves. */
        for(int t=0;t<3;t++){
            static Crossing crossing[3]={{10600,1712,288,1904,1888,GUARD_SE,0,0},
                {9000000,1712,288,1904,1888,GUARD_SE,0,0},{9000000,1712,288,1904,1888,GUARD_SE,0,0}};
            Crossing *c=&crossing[t];
            /* With the east Nexus gone, the army crosses south for the road west. */
            if(t&&nexus(2)<0&&c->at==9000000)c->at=frame+(t-1)*600;
            if(raid>=0)break;
            if(c->state==0&&frame>=c->at){
                int in=-1,out=-1,lead=-1,ld=0x7fffffff;
                for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].raw_type==RAW_NYDUS){
                    if(dist2(b[i].x,b[i].y,c->in_x,c->in_y)<32*32)in=i;
                    if(dist2(b[i].x,b[i].y,c->out_x,c->out_y)<32*32)out=i;
                }
                /* The middle of the largest Hydralisk clump away from the south. */
                ld=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&
                    dist2(u[i].x,u[i].y,guards[GUARD_SE].guard_x,guards[GUARD_SE].guard_y)>=400*400){
                    int d=army_near(u[i].x,u[i].y,64);
                    if(d>ld&&dist2(u[i].x,u[i].y,290,230)>=400*400){ld=d;lead=i;}
                }
                c->state=3;
                if(in>=0&&out>=0&&lead>=0){
                    int bx=64,n=box_select(u[lead].x-bx,u[lead].y-bx,u[lead].x+bx,u[lead].y+bx);
                    if(n){move_cursor(b[in].x,b[in].y);KEY_TAP(KEY_A);}
                    fprintf(trace,"{\"kind\":\"crossing\",\"frame\":%u,\"guard\":%d,\"selected\":%d}\n",(unsigned)frame,c->g,n);
                    c->state=1;c->since=frame;
                }
                break;
            }
            if(t==1&&c->state==2&&nexus(2)<0&&frame-c->since>=800&&!rush){c->state=0;c->at=frame;}
            if(c->state==1&&frame-c->since>=(c->g==GUARD_NW?250:400)){
                group_area_x=c->out_x;group_area_y=c->out_y;group_area_r=240;
                (void)group_command(U_HYDRALISK,MENU_ATTACK,guards[c->g].guard_x,guards[c->g].guard_y,1);
                group_area_x=-1;
                c->state=2;
                break;
            }
        }
        if(frame-mark>=20){input(0);continue;}
        /* Wait at the guard a raid will reach instead of leaving on a long trip. */
        {
            int waiting=0;
            for(unsigned r=1;r<RAID_ROWS;r++)
                if(frame>=raids[r].at-420&&frame<raids[r].at&&nexus(raids[r].town)>=0){
                    Guard *q=&guards[raids[r].g];
                    if(dist2(cx,cy,q->guard_x,q->guard_y)>96*96){close_menus();move_cursor(q->guard_x,q->guard_y);}
                    /* Chasing the last raid can leave a squad off its post. */
                    if(posted<(int)r){
                        close_menus();
                        group_area_x=q->guard_x;group_area_y=q->guard_y;group_area_r=560;group_rounds=2;
                        (void)group_command(U_HYDRALISK,MENU_MOVE,q->guard_x,q->guard_y,0);
                        group_area_x=-1;group_rounds=6;
                        posted=(int)r;
                    }
                    waiting=1;
                }
            if(waiting){input(0);continue;}
        }
        /* Two Scouts sit over the southern fields and kill whatever mines there.
           The first army south clears them on its way, so Drones can mine again. */
        if(raid<0&&nexus(2)<0&&!rush&&!fields&&frame<76000&&
           army_near(guards[GUARD_SE].guard_x,guards[GUARD_SE].guard_y,460)>=24){
            group_area_x=guards[GUARD_SE].guard_x;group_area_y=guards[GUARD_SE].guard_y;group_area_r=460;group_rounds=3;
            (void)group_command(U_HYDRALISK,MENU_ATTACK,1860,1650,1);
            group_area_x=-1;group_rounds=6;
            fields=1;fields_since=frame;
            fprintf(trace,"{\"kind\":\"fields\",\"frame\":%u}\n",(unsigned)frame);
        }
        if(fields==1&&raid<0&&frame-fields_since>=900){
            group_area_x=1860;group_area_y=1650;group_area_r=420;group_rounds=3;
            (void)group_command(U_HYDRALISK,MENU_ATTACK,guards[GUARD_SE].guard_x,guards[GUARD_SE].guard_y,1);
            group_area_x=-1;group_rounds=6;
            fields=2;
        }
        if(frame-mark>=20){input(0);continue;}
        if(frame>=next_order&&raid<0){
            /* Only the two Nexuses decide the mission, and each stops its town's
               raids. The army walks fixed legs along the road the raids use: a
               far order gives up, and the shortest open path can wander round a
               town's wall of buildings. */
            static const int legs_e[][2]={{1660,700},{1700,1120},{1712,1208}};
            static const int legs_w[][2]={{1320,1700},{1320,1560},{1000,1540},{640,1560},{470,1620},{400,1704}};
            int target=nexus(2)>=0?nexus(2):nexus(1);
            int west=target>=0&&nexus(2)<0;
            int leg_count=west?6:3;
            const int (*legs)[2]=west?legs_w:legs_e;
            next_order=frame+300;
            /* Once the east falls the army turns west. A raid of a standing town (at 34,170
               the southern hive) pins its guard squad, so the march waits for it to pass. */
            int raid_soon=0;
            for(int r=0;r<RAID_ROWS;r++)if(nexus(raids[r].town)>=0&&(int)frame+hold_lead>=raids[r].at&&(int)frame<raids[r].at+1200)raid_soon=1;
            if(west&&raid_soon)target=-1;
            if(target>=0&&!rush){
                int from_x=west?guards[GUARD_SE].guard_x:stage_x,from_y=west?guards[GUARD_SE].guard_y:stage_y;
                int home=army_near(from_x,from_y,west?460:700);
                /* The south has a raid at about frame 34,200, then no raid reaches
                   it or the east again until the Nexus is gone or far later. */
                if(home>=(west?(frame<52000?40:28):(frame<28000?50:26))&&frame>=(west?34800:21900)&&fields!=1){
                    rush=1;army_leg=0;army_x=from_x;army_y=from_y;army_r=west?460:700;
                    rush_west=west;
                    fprintf(trace,"{\"kind\":\"assault\",\"frame\":%u,\"army\":%d,\"target\":%d}\n",(unsigned)frame,home,target);
                }
            }
            if(target>=0&&rush){
                /* The east has fallen with the army at its gates: go straight on
                   west, and the southern squad falls in. */
                if(rush_west!=west){
                    rush_west=west;
                    army_leg=1;
                    group_area_x=guards[GUARD_SE].guard_x;group_area_y=guards[GUARD_SE].guard_y;group_area_r=460;group_rounds=3;
                    (void)group_command(U_HYDRALISK,MENU_ATTACK,legs[1][0],legs[1][1],1);
                    group_area_x=-1;group_rounds=6;
                }
                /* The army is its largest clump: it outruns any fixed circle, and
                   the far squads and fresh Hydralisks at home are not part of it. */
                int n=0,sx=0,sy=0;
                for(int i=0;i<MAX_UNITS&&rush;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&
                    (west||dist2(u[i].x,u[i].y,guards[GUARD_SE].guard_x,guards[GUARD_SE].guard_y)>=340*340)&&
                    dist2(u[i].x,u[i].y,guards[GUARD_NW].guard_x,guards[GUARD_NW].guard_y)>=340*340){
                    int c=0,ax2=0,ay2=0;
                    for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&u[k].type==U_HYDRALISK&&
                        dist2(u[k].x,u[k].y,u[i].x,u[i].y)<240*240){++c;ax2+=u[k].x;ay2+=u[k].y;}
                    if(c>n){n=c;sx=ax2;sy=ay2;}
                }
                if(!west&&n&&sy/n<560&&n<20)rush=0;
                if(n<8)rush=0;
                else if(rush){
                    army_x=sx/n;army_y=sy/n;army_r=240+80;
                    /* Hydralisks arriving in a line die to Reavers one by one. Most of
                       them reach each leg before the next starts. */
                    /* Back at the start after a retreat. */
                    if(!west&&army_y<560)army_leg=0;
                    while(army_leg<leg_count-1){
                        int there=0;
                        for(int i=0;i<MAX_UNITS;i++)there+=u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK&&
                            dist2(u[i].x,u[i].y,legs[army_leg][0],legs[army_leg][1])<170*170;
                        if(there*100<n*60)break;
                        ++army_leg;
                    }
                    int gx=legs[army_leg][0],gy=legs[army_leg][1];
                    /* In reach the Nexus is the only target worth the shots: its
                       death ends the town's raids whatever else still stands. */
                    {int dr=280;
                    if(dist2(army_x,army_y,b[target].x,b[target].y)<dr*dr&&currently_visible(b[target].x,b[target].y)){gx=b[target].x;gy=b[target].y;}}
                    /* One selection reaches about a screen: repeat around whoever
                       has not got this leg's order yet. */
                    int fd=9999,foe=nearest_foe(army_x,army_y,&fd);
                    if(foe>=0&&fd<contact&&dist2(army_x,army_y,guards[GUARD_NW].guard_x,guards[GUARD_NW].guard_y)>=340*340){
                        /* In reach of the garrison: everything charges at once, Reavers first. */
                        int fx=0,fy=0,fn=0,rv=-1,rbd=360*360;
                        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&foe_combat(u[i].raw_type)&&!SC64_TRANSPORT_LOADED(&u[i])){
                            if(dist2(u[i].x,u[i].y,u[foe].x,u[foe].y)<200*200){fx+=u[i].x;fy+=u[i].y;++fn;}
                            if(u[i].raw_type==83){int dd=dist2(u[i].x,u[i].y,army_x,army_y);if(dd<rbd){rbd=dd;rv=i;}}
                        }
                        gx=fx/fn;gy=fy/fn;
                        if(rv>=0){gx=u[rv].x;gy=u[rv].y;}
                        (void)hydra_box_attack(army_x,army_y,box_reach,gx,gy,frame-last_box>=rebox_period);
                        if(frame-last_box>=rebox_period)last_box=frame;
                        fprintf(trace,"{\"kind\":\"charge\",\"frame\":%u,\"target\":[%d,%d],\"foe\":%d,\"fd\":%d,\"army\":%d}\n",(unsigned)frame,gx,gy,foe,fd,n);
                        next_order=frame+fight_period;
                    } else {
                    group_area_x=army_x;group_area_y=army_y;group_area_r=army_r;group_rounds=8;
                    (void)group_command(U_HYDRALISK,MENU_ATTACK,gx,gy,gx==b[target].x&&gy==b[target].y);
                    group_area_x=-1;group_rounds=6;
                    fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"leg\":%d,\"army\":%d,\"at\":[%d,%d]}\n",(unsigned)frame,gx,gy,army_leg,n,army_x,army_y);
                    /* Count from the order's end: a far army's order can outlast
                       a fixed period and starve production. */
                    next_order=frame+300;
                    }
                }
            }
        }
        if(frame-mark>=20){input(0);continue;}
/* A long cursor trip ends the pass, so the raid checks above run often, and
   the next pass resumes at the next job, so no job starves another. */
#define COST(n) if(frame-mark>=20){turn=n;input(0);continue;} case n:;
        switch(turn){ case 0:;
        /* One ordinary pause-menu save while something is under construction,
           so the record carries active morphs. */
        if(!saved&&frame>=16000&&raid<0){
            int pick=free_drone(-1,-1);
            if(under_construction()&&pick>=0){
                if(pause_menu_save(path,pick)){saved=1;census("saved");}
            } else if(frame>=next_save_build&&minerals>=75){
                (void)build_near(B_CREEP_COLONY,MENU_Z7_CREEP,ax+40,ay+110,96);
                next_save_build=frame+300;
            }
        }
        COST(1)
        /* Whatever is still at home answers anything that comes for the base. */
        if(frame>=next_defend&&raid<0){
            int foe=-1,fd=420*420;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&sc64_race_unit_damage(u[i].type)>0){
                int d=dist2(u[i].x,u[i].y,ax-30,ay+100);
                if(d<fd){fd=d;foe=i;}
            }
            if(foe>=0&&army_near(stage_x,stage_y,700)>0){
                group_area_x=stage_x;group_area_y=stage_y-120;group_area_r=700;group_rounds=2;
                (void)group_command(U_HYDRALISK,MENU_ATTACK,u[foe].x,u[foe].y,1);
                group_area_x=-1;group_rounds=6;
            }
            next_defend=frame+300;
        }
        COST(50)
        if(frame>=next_mine){
            idle_reach=800;(void)order_idle_drones(3);next_mine=frame+(minerals>=1500?900:400);
            /* A Drone with no field left near it takes the Nydus to the southern
               fields, where the idle pass above picks it up. */
            int in=-1,stranded=-1;
            for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].raw_type==RAW_NYDUS&&
                dist2(b[i].x,b[i].y,1712,288)<32*32)in=i;
            for(int i=0;in>=0&&i<MAX_UNITS&&stranded<0;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE&&
                u[i].order==ORDER_IDLE&&!u[i].carry&&u[i].y<900){
                int near=0;
                for(int p=0;p<MAX_MINERALS&&!near;p++)near=m[p].amount>0&&dist2(u[i].x,u[i].y,m[p].x,m[p].y)<360*360&&currently_visible(m[p].x,m[p].y);
                if(!near)stranded=i;
            }
            if(stranded>=0&&raid<0&&select_single(stranded)){move_cursor(b[in].x,b[in].y);KEY_TAP(KEY_A);close_menus();}
        }
        COST(2)
        /* Pool first: its 200 minerals are in the starting bank. */
        if(!count_buildings(B_SPAWNING_POOL,0)&&minerals>=200&&drone_count()>=2&&frame>=next_build){
            (void)build_near(B_SPAWNING_POOL,MENU_BARRACKS,ax-96,ay+80,128);
            next_build=frame+120;
        }
        COST(3)
        /* Kerrigan and the middle squad wait for the first raid, which crosses
           the south at about frame 2,400. */
        if(!kerrigan_sent&&frame>=60){kerrigan_sent=kerrigan_to_nydus()?1:-1;}
        COST(4)
        if(!opened&&kerrigan_sent){
            int n=box_select(1284,1480,1376,1590);
            fprintf(trace,"{\"kind\":\"station\",\"frame\":%u,\"selected\":%d}\n",(unsigned)frame,n);
            if(n)(void)command_selected(MENU_MOVE,guards[GUARD_SE].guard_x,guards[GUARD_SE].guard_y);
            opened=1;
        }
        COST(5)
        if(kerrigan_sent==1){
            int k=kerrigan();
            if(k>=0&&dist2(u[k].x,u[k].y,1904,1888)<160*160){
                /* She stands by the exit, where A would send a selected squad
                   back down the Nydus; an empty L box drops the selection. */
                (void)box_select(1860,1760,1876,1776);
                if(unit_order(k,MENU_ATTACK,guards[GUARD_SE].guard_x+24,guards[GUARD_SE].guard_y+24))kerrigan_sent=2;
            }else if(k>=0&&opened&&frame<1800){
                if(dist2(cx,cy,1880,1880)>64*64)move_cursor(1880,1880);
                input(0);
                continue;
            }
        }
        COST(6)
        if(frame>=next_train&&ne>=0){
            /* One pass spends every larva the bank can pay for: hopping between
               nearby Hatcheries is far cheaper than coming back for each unit. */
            int done=0;
            for(int i=0;i<MAX_BUILDINGS;i++){
                if(!b[i].alive||b[i].owner!=SIDE||!sc64_race_is_zerg_townhall(b[i].type)||!b[i].larva||
                   !building_complete(i)||dist2(b[i].x,b[i].y,ax,ay)>=400*400)continue;
                while(b[i].larva>0){
                    int spare=race_owner_supply_cap(SIDE)-race_owner_supply(SIDE);
                    /* The army must not spend the bank a replacement Drone needs. */
                    int keep=frame>=18000?150:0;
                    int type=-1;
                    if(drone_count()+queued_units(U_DRONE)<drone_goal&&minerals>=50&&i==ne)type=U_DRONE;
                    else if(spare<14&&minerals>=100+keep&&queued_units(U_OVERLORD)<3)type=U_OVERLORD;
                    else if(count_buildings(B_HYDRALISK_DEN,1)&&minerals>=75+keep&&gas>=25&&spare>=2&&
                        count_units(SIDE,U_HYDRALISK)<100)type=U_HYDRALISK;
                    else if(count_buildings(B_SPAWNING_POOL,1)&&minerals>=50+keep&&spare>=2&&
                        count_units(SIDE,U_HYDRALISK)>=100)type=U_ZERGLING;
                    else if(count_buildings(B_SPAWNING_POOL,1)&&minerals>=150&&frame>=2600&&spare>=2&&
                        (!count_buildings(B_HYDRALISK_DEN,0)||gas<25))type=U_ZERGLING;
                    if(type<0||!train_at(i,type))break;
                    ++done;
                }
            }
            next_train=frame+(done?60:120);
        }
        COST(7)
        if(frame>=next_build&&opened&&raid<0){
            if(count_buildings(B_SPAWNING_POOL,1)&&!count_buildings(B_HYDRALISK_DEN,0)&&minerals>=100&&gas>=50&&drone_count()>=5)
                (void)build_near(B_HYDRALISK_DEN,MENU_ACADEMY,ax+64,ay+96,128);
            /* Not before frame 14,000: an Evolution Chamber at frame 4,900 delayed the first
               Hatchery by two thousand frames and the second raid reached its hive. */
            else if(frame>=tech_after&&count_buildings(B_HYDRALISK_DEN,1)&&!count_buildings(B_EVOLUTION_CHAMBER,0)&&minerals>=100&&drone_count()>=6)
                (void)build_near(B_EVOLUTION_CHAMBER,MENU_ENG_BAY,ax+96,ay+96,128);
            else if(count_buildings(B_SPAWNING_POOL,1)&&!count_buildings(B_EXTRACTOR,0)&&minerals>=50&&drone_count()>=6)
                (void)build_extractor_near(ax,ay);
            next_build=frame+240;
        }
        COST(8)
        /* Larva cap production at one unit per 428 frames per Hatchery, and a
           new Hatchery starts with three. The first goes beside the fields. */
        if(frame>=next_hatch&&opened&&raid<0&&minerals>=300&&drone_count()>=5&&count_buildings(B_SPAWNING_POOL,0)&&
           count_buildings(B_HATCHERY,0)<hatch_goal){
            int extra=count_buildings(B_HATCHERY,0)-3;
            if(extra<=0)(void)build_near(B_HATCHERY,MENU_DEPOT,1640,150,64);
            else if(frame>=9000)(void)build_near(B_HATCHERY,MENU_DEPOT,ax-40,ay+90,224);
            next_hatch=frame+300;
        }
        COST(9)
        if(frame>=next_gas&&raid<0&&(frame<tech_after||quiet)){if(frame<tech_after)(void)assign_gas_drone(gas_drones);else (void)assign_gas_all(gas_per_extractor);next_gas=frame+(frame<tech_after?600:200);}
        COST(30)
        if(frame>=tech_after&&frame>=next_research&&raid<0&&quiet&&count_buildings(B_EVOLUTION_CHAMBER,1)){
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
        COST(10)
        /* An Overlord for the eastern guard, started early because it is slow. */
        if(!ne_overlord&&opened&&frame>=3000&&raid<0){
            int o=-1,od=0x7fffffff;
            for(int k=0;k<MAX_UNITS;k++)if(u[k].alive&&u[k].owner==SIDE&&u[k].raw_type==RAW_OVERLORD){
                int d=dist2(u[k].x,u[k].y,guards[GUARD_NE].guard_x,guards[GUARD_NE].guard_y);
                if(d<od){od=d;o=k;}
            }
            if(o>=0)(void)unit_order(o,MENU_MOVE,guards[GUARD_NE].guard_x,guards[GUARD_NE].guard_y);
            ne_overlord=1;
        }
        COST(11)
        /* Feed the far guards from their own Hatcheries' larva. */
        /* Waves draw on the northwestern Hatchery, but raids run past it for the
           corner. While that Hatchery lives, it trains the corner's squad, and
           its Overlord goes with them. */
        if(raid<0&&frame>=next_visit[GUARD_NW]&&minerals>=150&&count_buildings(B_HYDRALISK_DEN,1)&&gas>=75&&
           hatch_at(guards[GUARD_NW].hatch_x,guards[GUARD_NW].hatch_y)>=0&&
           army_near(guards[GUARD_NW].guard_x,guards[GUARD_NW].guard_y,300)<5){
            visit_hive(GUARD_NW,U_HYDRALISK);
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==RAW_OVERLORD&&
                dist2(u[i].x,u[i].y,guards[GUARD_NW].hatch_x,guards[GUARD_NW].hatch_y)<420*420&&
                dist2(u[i].x,u[i].y,guards[GUARD_NW].guard_x,guards[GUARD_NW].guard_y)>=96*96){
                (void)unit_order(i,MENU_MOVE,guards[GUARD_NW].guard_x+16,guards[GUARD_NW].guard_y-16);
                close_menus();
                break;
            }
            next_visit[GUARD_NW]=frame+1000;
        }
        /* The fields beside the northeastern Hatchery run out at about frame
           30,000. Open the southern fields first: an Overlord to see them, then
           Drones from the Hatchery beside them. */
        if(raid<0&&fields==2&&frame>=next_se_eco&&hatch_at(guards[GUARD_SE].hatch_x,guards[GUARD_SE].hatch_y)>=0){
            int h=hatch_at(guards[GUARD_SE].hatch_x,guards[GUARD_SE].hatch_y),o=-1,seen=0,here=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==RAW_OVERLORD){
                if(dist2(u[i].x,u[i].y,1880,1680)<64*64||dist2(u[i].tx,u[i].ty,1880,1680)<64*64)seen=1;
                else if(dist2(u[i].x,u[i].y,1824,1832)<200*200||(o<0&&dist2(u[i].x,u[i].y,1250,1880)>=160*160&&dist2(u[i].x,u[i].y,90,240)>=240*240&&dist2(u[i].x,u[i].y,352,136)>=420*420))o=i;
            }
            for(int i=0;i<MAX_UNITS;i++)here+=u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE&&
                dist2(u[i].x,u[i].y,1880,1720)<320*320;
            if(!seen){
                if(o>=0)(void)unit_order(o,MENU_MOVE,1880,1680);
                else (void)train_at(h,U_OVERLORD);
                close_menus();
            } else if(here<8){
                for(int n=0;n<3&&b[h].larva>0&&minerals>=50&&here+n<8;n++)(void)train_at(h,U_DRONE);
                close_menus();
            }
            next_se_eco=frame+600;
        }
        COST(60)
        /* Feed the southern guard from its own Hatchery's larva. */
        if(raid<0&&frame>=next_visit[GUARD_SE]&&minerals>=150&&count_buildings(B_SPAWNING_POOL,1)&&
           hatch_at(guards[GUARD_SE].hatch_x,guards[GUARD_SE].hatch_y)>=0){
            int type=count_buildings(B_HYDRALISK_DEN,1)&&gas>=75?U_HYDRALISK:U_ZERGLING;
            if(army_near(guards[GUARD_SE].guard_x,guards[GUARD_SE].guard_y,300)<10)visit_hive(GUARD_SE,type);
            next_visit[GUARD_SE]=frame+6000;
        }
        COST(12)
        /* Muster everything at the eastern guard just before the second raid. */
        if(!mustered&&frame>=9000&&raid<0){
            group_area_x=ax;group_area_y=ay+60;group_area_r=460;group_rounds=4;
            (void)group_command(U_HYDRALISK,MENU_ATTACK,guards[GUARD_NE].guard_x,guards[GUARD_NE].guard_y,0);
            group_area_x=-1;group_rounds=6;
            mustered=1;
        }
        COST(17)
        /* Not during an assault: this would call its Hydralisks back. */
        if(frame>=next_rally&&raid<0&&!rush){
            /* New units gather at the eastern guard until the second raid has
               passed, then where the road south leaves the base. */
            int rx=frame<11100?guards[GUARD_NE].guard_x:stage_x;
            int ry=frame<11100?guards[GUARD_NE].guard_y:stage_y;
            if(nexus(2)<0&&!rush){rx=1690;ry=350;}
            group_area_x=ax;group_area_y=ay+60;group_area_r=nexus(2)<0&&!rush?720:420;group_rounds=nexus(2)<0?4:2;
            (void)group_command(U_HYDRALISK,MENU_ATTACK,rx,ry,0);
            (void)group_command(U_ZERGLING,MENU_ATTACK,rx,ry,0);
            group_area_x=-1;group_rounds=6;
            next_rally=frame+600;
        }
        COST(18)
        }
        turn=0;
        if(frame>=next_checkpoint){census("progress");next_checkpoint=frame+2000;}
        input(0);
    }
    census("final");
    fclose(trace);
    printf("z8 controller: frame=%u calls=%u won=%d lost=%d screen=%d fault=%u drones=%d army=%d kerrigan=%d nexus1=%d nexus2=%d\n",
        (unsigned)frame,host_tick,won,lost,screen,(unsigned)race_z8_sched.fault,drone_count(),army_count(),kerrigan()>=0,
        nexus(1)>=0?b[nexus(1)].hp+b[nexus(1)].shield:0,nexus(2)>=0?b[nexus(2)].hp+b[nexus(2)].shield:0);
    return won?0:1;
}
