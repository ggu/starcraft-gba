/* Source-driven normal-input Z6 campaign controller experiment. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Z6_LIMIT 120000u
static u16 prior;
static unsigned host_ticks;
static FILE *controller_trace;
static int trace_event_started;
static int trace_carrier_slot=-1;
static int hero_track_slot=-2,hero_track_hp,hero_track_shield,hero_track_x,hero_track_y;
static int z6_storms_cast;
static int kerrigan(void);
static void z6_log_hero_hit(int hero,int old_x,int old_y,int old_hp,int old_shield);
static int select_slot(int slot);
static int assign_gas_worker(int worker);
static int order_group(int attack,int x,int y);
static int box_select(int left,int top,int right,int bottom);
static int best_storm_point(int slot,int *out_x,int *out_y);
static int controller_cast_storm_pair(int hero,int x,int y,int cluster_count);
static void trace_storm_candidates(int slot)
{
    if(slot<0)return;
    int count=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive) {
        int dx=u[i].x-u[slot].x,dy=u[i].y-u[slot].y;
        if(dx*dx+dy*dy<=144*144&&owners_hostile(SIDE,u[i].owner)) {
            count++;
            fprintf(stderr,"  Storm candidate slot%d raw%d owner%d source%d xy%d,%d d2%d\n",
                i,u[i].raw_type,u[i].owner,u[i].raw_owner,u[i].x,u[i].y,dx*dx+dy*dy);
        }
    }
    fprintf(stderr,"  Storm range scan Kerrigan owner%d rawowner%d xy%d,%d candidates%d hostileTest%d\n",
        u[slot].owner,u[slot].raw_owner,u[slot].x,u[slot].y,count,
        owners_hostile(SIDE,ENEMY));
}

static int z6_hold_safely(void);
/* Set while Kerrigan rests for Storm energy; input() then keeps her clear of
 * attackers between the controller's own steps. */
static int z6_rest_guard,z6_rest_guard_busy;
static void input(u16 held)
{
    u16 pressed=(u16)(held & (u16)~prior);
    int changed=held!=prior;
    if (screen==SCREEN_RESULT || won || lost) return;
    game_update(held,pressed);
    prior=held;
    ++host_ticks;
    /* A retreat can recursively advance input; record this tick first. */
    if(controller_trace&&changed) {
        fprintf(controller_trace,"%s{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}",
            trace_event_started?",":"",host_ticks,(unsigned)held,(unsigned)pressed);
        trace_event_started=1;
    }
    int hero=kerrigan();
    if(hero_track_slot==-2) {
        if(hero>=0) {
            hero_track_slot=hero;
            hero_track_hp=u[hero].hp;hero_track_shield=u[hero].shield;
            hero_track_x=u[hero].x;hero_track_y=u[hero].y; }
    } else if(hero_track_slot>=0) {
        if(hero<0) {
            z6_log_hero_hit(hero_track_slot,hero_track_x,hero_track_y,hero_track_hp,hero_track_shield);
            hero_track_slot=-1;
        } else {
            if(u[hero].hp<hero_track_hp||u[hero].shield<hero_track_shield)
                z6_log_hero_hit(hero,hero_track_x,hero_track_y,hero_track_hp,hero_track_shield);
            hero_track_slot=hero;hero_track_hp=u[hero].hp;hero_track_shield=u[hero].shield;
            hero_track_x=u[hero].x;hero_track_y=u[hero].y;
        }
    }
    if(z6_rest_guard&&!z6_rest_guard_busy&&
       mode==MODE_NORMAL&&!menu_open) {
        z6_rest_guard_busy=1;
        (void)z6_hold_safely();
        z6_rest_guard_busy=0;
    }

}
static void tap(u16 key) { input(key); input(0); }
static void move_cursor(int x,int y);
static int choose_menu_row(int row)
{
    if(!menu_open)tap(KEY_B);
    int count=field_menu_count();
    if(!menu_open||row<0||row>=count) { if(menu_open)tap(KEY_B); return 0; }
    for(int i=0;i<count&&menu_cursor!=row&&screen==SCREEN_PLAY&&!won&&!lost;i++)tap(KEY_DOWN);
    if(menu_cursor!=row) { if(menu_open)tap(KEY_B); return 0; }
    tap(KEY_A);
    return 1;
}

static int kerrigan(void)
{
    for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive && u[i].raw_type==51 && u[i].owner==SIDE) return i;
    return -1;
}
static int owner_count(int owner)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==owner;return n;
}
static int side_raw_count(int raw)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==raw;return n;
}
static int side_gas_workers(void)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type==41&&u[i].order==ORDER_GAS;
    return n;
}
static int assign_available_gas_workers(int wanted)
{
    int n=side_gas_workers();
    for(int i=0;i<MAX_UNITS&&n<wanted;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type==41&&u[i].order==ORDER_MINE&&assign_gas_worker(i))n++;
    return n;
}
static int side_combat_count(void)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type!=41&&u[i].raw_type!=42)n++;
    return n;
}
static int idle_side_combat_count(void)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type!=41&&u[i].raw_type!=42&&u[i].order==ORDER_IDLE)n++;
    return n;
}
static int order_fresh_combat_group(int attack,int x,int y)
{
    if(menu_open)tap(KEY_B);if(mode!=MODE_NORMAL)tap(KEY_B);
    (void)box_select(900,700,916,716); /* clear a selected producer through ordinary input */
    return order_group(attack,x,y);
}
static u32 hostile_health_total(void)
{
    u32 total=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&owners_hostile(SIDE,u[i].owner))
        total+=(u32)(u[i].hp>0?u[i].hp:0)+(u32)(u[i].shield>0?u[i].shield:0);
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner))
        total+=(u32)(b[i].hp>0?b[i].hp:0);
    return total;
}
static int raw_building(int raw,int owner)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==raw&&b[i].owner==owner)return i;
    return -1;
}
static int owned_building_count(int type)
{
    int n=0;for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&b[i].type==type;return n;
}
static int nearest_force2_in_region(int left,int top,int right,int bottom,int *out_x,int *out_y)
{
    int ax=0,ay=0,n=0,best=0x7fffffff,bx=-1,by=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type!=41&&u[i].raw_type!=42){
        ax+=u[i].x;ay+=u[i].y;++n;
    }
    if(n){ax/=n;ay/=n;}else{ax=(left+right)/2;ay=(top+bottom)/2;}
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].x>=left&&u[i].x<right&&
        u[i].y>=top&&u[i].y<bottom&&
        sc64_z6_world_owner_matches(&race_z6_world,19,u[i].raw_owner,u[i].owner)>0){
        int d=dist2(ax,ay,u[i].x,u[i].y);if(d<best){best=d;bx=u[i].x;by=u[i].y;}
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].x>=left&&b[i].x<right&&
        b[i].y>=top&&b[i].y<bottom&&
        sc64_z6_world_owner_matches(&race_z6_world,19,b[i].raw_owner,b[i].owner)>0){
        int d=dist2(ax,ay,b[i].x,b[i].y);if(d<best){best=d;bx=b[i].x;by=b[i].y;}
    }
    if(bx<0)return 0;
    *out_x=bx;*out_y=by;return 1;
}
static int select_building_slot(int slot)
{
    if(slot<0||slot>=MAX_BUILDINGS||!b[slot].alive||b[slot].owner!=SIDE)return 0;
    if(menu_open)tap(KEY_B);if(mode!=MODE_NORMAL)tap(KEY_B);
    move_cursor(b[slot].x,b[slot].y);tap(KEY_A);
    return selected_building==slot;
}
static int box_select(int left,int top,int right,int bottom)
{
    if(menu_open)tap(KEY_B);if(mode!=MODE_NORMAL)tap(KEY_B);
    move_cursor(left,top);input(KEY_L);
    unsigned guard=0;
    while((cx<right||cy<bottom)&&screen==SCREEN_PLAY&&!won&&!lost&&guard++<6000u){u16 h=KEY_L;if(cx<right)h|=KEY_RIGHT;if(cy<bottom)h|=KEY_DOWN;input(h);}
    input(0);return selected_count();
}
static int queue_hatchery_type(int raw,int count)
{
    int hatch=-1,target=raw==41?U_DRONE:raw==42?U_OVERLORD:
        raw==43?U_MUTALISK:raw==38?U_HYDRALISK:U_ZERGLING;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        (b[i].type==B_HATCHERY||b[i].type==B_LAIR||b[i].type==B_HIVE)&&
        building_complete(i)&&b[i].queue_count<5&&b[i].larva>0){hatch=i;break;}
    if(hatch<0||!select_building_slot(hatch))return 0;
    for(int n=0;n<5&&train_type!=target;n++)tap(KEY_R);
    if(train_type!=target)return 0;
    int before=b[hatch].queue_count;
    for(int n=0;n<count;n++)tap(KEY_A);
    return b[hatch].queue_count>before;
}
static int build_spire(void)
{
    int worker=-1,site_x=-1,site_y=-1,hatch=raw_building(131,SIDE);
    if(raw_building(141,SIDE)>=0||hatch<0)return raw_building(141,SIDE)>=0;
    for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41&&
           u[i].order!=ORDER_BUILD){worker=i;break;}
    if(worker<0)return 0;
    int best=0x7fffffff;
    for(int y=360;y<720;y+=32)for(int x=480;x<880;x+=32)if(can_place(B_SPIRE,x,y)){
        int dx=x-b[hatch].x,dy=y-b[hatch].y,d=dx*dx+dy*dy;
        if(d<best){best=d;site_x=x;site_y=y;}
    }
    if(site_x<0)return 0;
    if(menu_open)tap(KEY_B);if(mode!=MODE_NORMAL)tap(KEY_B);
    move_cursor(u[worker].x,u[worker].y);tap(KEY_A);
    if(selected_count()!=1||!u[worker].selected)return 0;
    if(!choose_menu_row(MENU_ZP_SPIRE))return 0;
    if(mode!=MODE_BUILD||menu!=B_SPIRE)return 0;
    move_cursor(site_x,site_y);tap(KEY_A);
    fprintf(stderr,"spire order site%d,%d worker%d order%d mode%d menu%d ownerminerals%d gas%d\n",site_x,site_y,worker,u[worker].order,mode,menu,minerals,gas);
    return u[worker].order==ORDER_BUILD;
}
static int build_extractor(void)
{
    int worker=-1,geyser=-1;
    if(raw_building(149,SIDE)>=0)return 1;
    for(int i=0;i<geyser_count;i++) {
        int used=0;
        for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE&&
            b[j].type==B_EXTRACTOR&&dist2(geysers[i].x,geysers[i].y,b[j].x,b[j].y)<36*36)used=1;
        if(!used){geyser=i;break;}
    }
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41&&
        u[i].order!=ORDER_BUILD){worker=i;break;}
    if(worker<0||geyser<0){fprintf(stderr,"extractor prereq fail worker%d geyser%d count%d\n",worker,geyser,geyser_count);return 0;}
    /* A Drone moves while the cursor is steered, so capture the base group
     * with a broad ordinary drag. start_build consumes only its first worker. */
    if(box_select(240,80,500,280)<=0){fprintf(stderr,"extractor group select failed mode%d cursor%d,%d\n",mode,cx,cy);return 0;}
    if(!choose_menu_row(MENU_REFINERY))return 0;
    if(mode!=MODE_BUILD||menu!=B_EXTRACTOR){fprintf(stderr,"extractor menu fail mode%d menu%d row%d\n",mode,menu,menu_cursor);return 0;}
    move_cursor(geysers[geyser].x,geysers[geyser].y);tap(KEY_A);
    fprintf(stderr,"extractor attempt geyser%d xy%d,%d worker%d order%d building%d mode%d bank%d/%d\n",
        geyser,geysers[geyser].x,geysers[geyser].y,worker,u[worker].order,
        raw_building(149,SIDE),mode,minerals,gas);
    return raw_building(149,SIDE)>=0;
}
static int build_extra_hatchery(void)
{
    int worker=-1,site_x=-1,site_y=-1,hatch=raw_building(132,SIDE),best=-1;
    if(hatch<0)hatch=raw_building(131,SIDE);
    if(owned_building_count(B_HATCHERY)>=2)return 1;
    if(hatch<0||minerals<300)return 0;
    for(int y=120;y<500;y+=16)for(int x=400;x<720;x+=16)
        if(race_can_site(SIDE,B_HATCHERY,x,y)&&can_place(B_HATCHERY,x,y)){
            int d=dist2(x,y,b[hatch].x,b[hatch].y);
            if(d>80*80&&(best<0||d<best)){best=d;site_x=x;site_y=y;}
        }
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41&&
        (u[i].order==ORDER_MINE||u[i].order==ORDER_IDLE)){worker=i;break;}
    if(worker<0||site_x<0){fprintf(stderr,"extra Hatchery site/worker unavailable worker%d site%d,%d\n",worker,site_x,site_y);return 0;}
    if(!select_slot(worker))return 0;
    if(!choose_menu_row(MENU_DEPOT))return 0;
    if(mode!=MODE_BUILD||menu!=B_HATCHERY)return 0;
    move_cursor(site_x,site_y);tap(KEY_A);
    fprintf(stderr,"extra Hatchery attempt site%d,%d worker%d order%d buildingcount%d bank%d/%d\n",site_x,site_y,worker,u[worker].order,owned_building_count(B_HATCHERY),minerals,gas);
    return owned_building_count(B_HATCHERY)>0;
}
static int assign_gas_worker(int worker)
{
    int extractor=raw_building(149,SIDE);
    if(extractor<0||!building_complete(extractor)||worker<0||!u[worker].alive||
       u[worker].raw_type!=41||u[worker].owner!=SIDE)return 0;
    if(!select_slot(worker))return 0;
    move_cursor(b[extractor].x,b[extractor].y);tap(KEY_A);
    return u[worker].order==ORDER_GAS;
}
static void move_cursor(int x,int y)
{
    unsigned guard=0;
    x=limit(cx+((x-cx)>=0?((x-cx+2)/4):((x-cx-2)/4))*4,0,WORLD_W-1);
    y=limit(cy+((y-cy)>=0?((y-cy+2)/4):((y-cy-2)/4))*4,0,WORLD_H-1);
    while((cx!=x || cy!=y) && screen==SCREEN_PLAY && !won && !lost && guard++<6000u) {
        u16 h=KEY_R;
        if(cx<x)h|=KEY_RIGHT; if(cx>x)h|=KEY_LEFT;
        if(cy<y)h|=KEY_DOWN; if(cy>y)h|=KEY_UP;
        input(h);
    }
    input(0);
    if(cx!=x||cy!=y)fprintf(stderr,"cursor stalled target=%d,%d current=%d,%d screen=%d mode=%d host=%u\n",x,y,cx,cy,screen,mode,host_ticks);
}
#ifdef SC64_DIRECT_ATTACK
static int controller_attackmove_member_reachable(int sx,int sy,int px,int py,
                                                   int flying)
{
    int nx,ny;
    return flying || path_next_tile(sx,sy,px,py,&nx,&ny);
}

static int controller_attackmove_group_reachable(int px,int py)
{
    for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].selected&&u[i].owner==SIDE&&
           !SC64_TRANSPORT_LOADED(&u[i])&&
           controller_attackmove_member_reachable(u[i].x,u[i].y,px,py,
                                                   sc64_race_unit_flying(u[i].type)))
            return 1;
    return 0;
}


/* The engine's direct-target picker uses dist2 < 31^2 for units and
 * dist2 < 26^2 for buildings (game.c: sc64_direct_attack_find_target).
 * Stay 9/17 pixels outside the applicable edge, still within the 64px
 * Hydralisk range used by this controller. A click on open ground keeps
 * the normal Attack-Move semantics. */
static int controller_attackmove_click_point(int sx,int sy,int x,int y,
                                              int *out_x,int *out_y)
{
    static const int cardinal[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
    static const int diagonal[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}};
    int target=sc64_direct_attack_find_target(x,y);
    int pick_radius=target>=0 ? (target<MAX_UNITS ? 31 : 26) : 31;
    unsigned out_of_bounds=0,targeted=0,blocked=0,unreachable=0;
    /* Search 40/48px cardinal offsets, plus 32x32 diagonals (~45px). The
     * source picker bounds are 31px for units and 26px for buildings; all
     * points stay outside that bound and within the 64px Hydra range. */
    for(int extra=9;extra<=17;extra+=8) for(int d=0;d<4;d++) {
        int radius=pick_radius+extra;
        int px=x+cardinal[d][0]*radius,py=y+cardinal[d][1]*radius,nx,ny;
        if(px<0||py<0||px>=WORLD_W||py>=WORLD_H) { ++out_of_bounds; continue; }
        if(sc64_direct_attack_find_target(px,py)>=0) { ++targeted; continue; }
        if(!is_traversable(px/16,py/16,-1)) { ++blocked; continue; }
        if(!controller_attackmove_group_reachable(px,py)) { ++unreachable; continue; }
        *out_x=px;*out_y=py;return 1;
    }
    for(int d=0;d<4;d++) {
        int px=x+diagonal[d][0]*32,py=y+diagonal[d][1]*32,nx,ny;
        if(px<0||py<0||px>=WORLD_W||py>=WORLD_H) { ++out_of_bounds; continue; }
        if(sc64_direct_attack_find_target(px,py)>=0) { ++targeted; continue; }
        if(!is_traversable(px/16,py/16,-1)) { ++blocked; continue; }
        if(!controller_attackmove_group_reachable(px,py)) { ++unreachable; continue; }
        *out_x=px;*out_y=py;return 1;
    }
    /* Diagnostics only: report nearest live hostile, rejection totals, and
     * selected group coordinates. This does not affect candidate choice. */
    {
        int entity=-1,best=0x7fffffff,tx=-1,ty=-1,raw=-1,owner=-1;
        for(int i=0;i<MAX_UNITS;i++) if(u[i].alive&&owners_hostile(SIDE,u[i].owner)) {
            int d=dist2(x,y,u[i].x,u[i].y); if(d<best) {best=d;entity=i;tx=u[i].x;ty=u[i].y;raw=u[i].raw_type;owner=u[i].owner;}
        }
        for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive&&owners_hostile(SIDE,b[i].owner)) {
            int d=dist2(x,y,b[i].x,b[i].y); if(d<best) {best=d;entity=MAX_UNITS+i;tx=b[i].x;ty=b[i].y;raw=b[i].raw_type;owner=b[i].owner;}
        }
        fprintf(stderr,"no Attack-Move point objective=(%d,%d) nearest=%d raw%d owner%d xy%d,%d d2%d group-centroid=%d,%d rejects bounds=%u targeted=%u blocked=%u unreachable=%u\n",
            x,y,entity,raw,owner,tx,ty,best,sx,sy,out_of_bounds,targeted,blocked,unreachable);
        for(int i=0;i<MAX_UNITS;i++) if(u[i].alive&&u[i].selected&&u[i].owner==SIDE)
            fprintf(stderr,"  selected slot%d raw%d xy%d,%d order%d tx%d,%d\\n",i,u[i].raw_type,u[i].x,u[i].y,u[i].order,u[i].tx,u[i].ty);
    }
    return 0;
}

static void controller_cancel_armed_attack(void)
{
    if(mode==MODE_ATTACK) tap(KEY_B);
    if(menu_open) tap(KEY_B);
    if(mode!=MODE_NORMAL||menu_open)
        fprintf(stderr,"Attack-Move cancel cleanup incomplete mode%d menu%d\\n",mode,menu_open);
}

static int controller_click_attackmove(int sx,int sy,int x,int y)
{
    int px=x,py=y;
    /* Moving the cursor advances the simulation. Re-plan from the original
     * objective whenever a hostile enters the point's pick radius. Never
     * issue a direct-target order as a fallback for a controller Attack-Move. */
    for(int retry=0;retry<4;retry++) {
        if(!controller_attackmove_click_point(sx,sy,x,y,&px,&py)) {
            controller_cancel_armed_attack();
            return 0;
        }
        move_cursor(px,py);
        int target_now=sc64_direct_attack_find_target(cx,cy);
        if(target_now<0) {
            fprintf(stderr,"attackmove click try%d pre-A mode%d screen%d menu%d point%d,%d\n",
                retry,mode,screen,menu_open,cx,cy);
            tap(KEY_A);
            fprintf(stderr,"attackmove click post-A mode%d screen%d menu%d selected%d\n",
                mode,screen,menu_open,selected_count());
            return 1;
        }
    }
    controller_cancel_armed_attack();
    return 0;
}

static int controller_selected_attackmove(void)
{
    int count=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].owner==SIDE) {
        ++count;
        if(u[i].order==ORDER_ATTACK_TARGET) return 0;
        if(u[i].order!=ORDER_ATTACK_MOVE &&
           (u[i].order!=ORDER_IDLE || dist2(u[i].x,u[i].y,u[i].tx,u[i].ty)>16*16))
            return 0;
    }
    return count>0;
}
#endif

static int order_group(int attack,int x,int y)
{
    if(menu_open) tap(KEY_B);
    if(mode!=MODE_NORMAL) tap(KEY_B);
    /* Establish a unit selection before R: when the sole selected unit is
     * Kerrigan, B+R means Storm. R first expands to the normal whole group;
     * then B+R safely removes Kerrigan before issuing the army order. */
    int seed=kerrigan();
    if(seed<0)for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type!=41&&u[i].raw_type!=42){seed=i;break;}
    if(seed<0||!select_slot(seed))return 0;
    tap(KEY_R);
    if(selected_count()<=1&&seed==kerrigan()) {
        fprintf(stderr,"R group contains only Kerrigan; refusing bulk route at%u\n",host_ticks);
        return 0;
    }
    tap(KEY_B|KEY_R); /* Z6's ordinary escort-group shortcut excludes Kerrigan */
    if(!selected_count()) {
        fprintf(stderr,"B+R selected no combat group at%u cursor%d,%d screen%d mode%d building%d\n",
            host_ticks,cx,cy,screen,mode,selected_building);
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE)
            fprintf(stderr,"  SIDE live slot%d raw%d xy%d,%d selectable%d loaded%d\n",i,u[i].raw_type,
                u[i].x,u[i].y,sc64_game_unit_selectable(u[i].type),SC64_TRANSPORT_LOADED(&u[i]));
        return 0;
    }
    fprintf(stderr,"B+R selected%d units before command at%u:",selected_count(),host_ticks);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].selected)
        fprintf(stderr," %d/raw%d@%d,%d",i,u[i].raw_type,u[i].x,u[i].y);
    fputc('\n',stderr);
    int command_row=attack?MENU_ATTACK:MENU_MOVE;
    if(!choose_menu_row(command_row) ||
       mode!=(attack?MODE_ATTACK:MODE_MOVE)) {
        fprintf(stderr,"B+R attack command mode failed at%u mode%d menuopen%d row%d selected%d\n",
            host_ticks,mode,menu_open,menu_cursor,selected_count());
        return 0;
    }
#ifdef SC64_DIRECT_ATTACK
    if(attack) {
        int sx=0,sy=0,n=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected) {sx+=u[i].x;sy+=u[i].y;++n;}
        int clicked=0;
        if(n) {
            int guard_was_busy=z6_rest_guard_busy;
            z6_rest_guard_busy=1;
            clicked=controller_click_attackmove(sx/n,sy/n,x,y);
            z6_rest_guard_busy=guard_was_busy;
        }
        if(!n||!clicked) {
            controller_cancel_armed_attack();
            fprintf(stderr,"Attack-Move controller found no stable safe click for %d,%d at%u\n",x,y,host_ticks);
            return 0;
        }
        if(!controller_selected_attackmove()) {
            fprintf(stderr,"Attack-Move acceptance mismatch at%u cursor%d,%d selected%d mode%d menu%d screen%d\n",
                host_ticks,cx,cy,selected_count(),mode,menu_open,screen);
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected)
                fprintf(stderr,"  accepted order slot%d raw%d order%d target%d tx%d,%d\n",
                    i,u[i].raw_type,u[i].order,u[i].target,u[i].tx,u[i].ty);
            assert(controller_selected_attackmove());
        }
    } else
#endif
    { move_cursor(x,y);tap(KEY_A); }
    fprintf(stderr,"B+R command destination%d,%d at%u frame%d screen%d mode%d menuopen%d selected%d:",
        cx,cy,host_ticks,frame,screen,mode,menu_open,selected_count());
    for(int i=0;i<MAX_UNITS;i++)if(u[i].selected)
        fprintf(stderr," %d/raw%d/o%d->%d,%d",i,u[i].raw_type,u[i].order,u[i].tx,u[i].ty);
    fputc('\n',stderr);
    return 1;
}
static int order_raw_group(unsigned raw,int x,int y)
{
    int picked=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==(int)raw){picked=i;break;}
    if(picked<0)return 0;
    if(mode!=MODE_NORMAL)tap(KEY_B);
    if(menu_open)tap(KEY_B);
    move_cursor(u[picked].x,u[picked].y);tap(KEY_A);tap(KEY_A);
    if(!selected_count())return 0;
    if(!choose_menu_row(MENU_ATTACK)||mode!=MODE_ATTACK)return 0;
    move_cursor(x,y);tap(KEY_A);
    return 1;
}
/* All changes below use the same selection/menu/target inputs as a player.
 * Keep Storm casts separate from group orders: the source hero is the only
 * caster and selecting a mixed group intentionally hides the ability row. */
static int select_slot(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE)
        return 0;
    if (menu_open) tap(KEY_B);
    if (mode != MODE_NORMAL) tap(KEY_B);
    /* A-clicks while a building remains selected act on that building instead
     * of selecting the unit under the cursor. Clear it with a normal empty
     * drag before issuing an individual unit click. */
    if (selected_building >= 0) (void)box_select(900,700,916,716);
    int want_x=u[slot].x,want_y=u[slot].y;
    move_cursor(want_x,want_y);
    tap(KEY_A);
    if(selected_count()==1 && u[slot].selected)return 1;
    fprintf(stderr,"select_slot miss slot%d raw%d wanted%d,%d cursor%d,%d sel%d count%d mode%d screen%d\n",
        slot,u[slot].raw_type,want_x,want_y,cx,cy,u[slot].selected,selected_count(),mode,screen);
    fprintf(stderr,"  selection_target=%d type=%d actorflags=%u selectable=%d alive=%d owner=%d loaded=%d\n",
        selection_target(),u[slot].type,sc64_game_unit_actor_flags(u[slot].type),
        sc64_game_unit_selectable(u[slot].type),u[slot].alive,u[slot].owner,
        SC64_TRANSPORT_LOADED(&u[slot]));
    move_cursor(u[slot].x,u[slot].y);tap(KEY_A);
    return selected_count()==1 && u[slot].selected;
}
static int hero_command(int row,int x,int y)
{
    int k=kerrigan();
    if(k<0||!select_slot(k)||selected_count()!=1||!u[k].selected)return 0;
    if(row==MENU_STOP+2&&u[k].order!=ORDER_IDLE) {
        tap(KEY_B);
        int count=field_menu_count();
        if(!menu_open||MENU_STOP>=count) { if(menu_open)tap(KEY_B); return 0; }
        for(int i=0;i<count&&menu_cursor!=MENU_STOP;i++)tap(KEY_DOWN);
        if(menu_cursor!=MENU_STOP) { tap(KEY_B); return 0; }
        tap(KEY_A);
        if(u[k].order!=ORDER_IDLE||mode!=MODE_NORMAL)return 0;
        if(!select_slot(k)||selected_count()!=1)return 0;
    }
    tap(KEY_B);
    /* Menu movement wraps, so never chase an unavailable ability row. */
    int count=field_menu_count();
    if(!menu_open||row<0||row>=count) { if(menu_open)tap(KEY_B); return 0; }
    for(int i=0;i<count&&menu_cursor!=row;i++)tap(KEY_DOWN);
    if(menu_cursor!=row) { tap(KEY_B); return 0; }
    tap(KEY_A);
    if (row==MENU_STOP+1) return mode==MODE_NORMAL; /* cloak toggle */
    if (row!=MENU_STOP+2 || mode!=MODE_P4_STORM) return 0;
    move_cursor(x,y);
    int energy_before=u[k].carry;
    tap(KEY_A);
    int paid=energy_before-u[k].carry;
    if(mode==MODE_NORMAL&&u[k].alive&&paid>=75*256-16&&paid<=75*256+16)
        fprintf(stderr,"normal-input Kerrigan Storm energy debit %d Q8 (regen-adjusted)\n",paid);
    return mode==MODE_NORMAL&&u[k].alive&&paid>=75*256-16&&paid<=75*256+16;
}
static int hero_cloak(void) { return hero_command(MENU_STOP+1,0,0); }
static int hero_storm(int x,int y) { return hero_command(MENU_STOP+2,x,y); }
static int hero_move(int x,int y)
{
    int k=kerrigan();
    if (k<0||!select_slot(k)||selected_count()!=1||!u[k].selected) {
        fprintf(stderr,"hero_move selection failed slot%d mode%d count%d\n",k,mode,selected_count()); return 0;
    }
    tap(KEY_B);
    int count=field_menu_count();
    if(!menu_open||MENU_MOVE>=count) { if(menu_open)tap(KEY_B); return 0; }
    for(int i=0;i<count&&menu_cursor!=MENU_MOVE;i++)tap(KEY_DOWN);
    if(menu_cursor!=MENU_MOVE) { tap(KEY_B); return 0; }
    tap(KEY_A);
    if(mode!=MODE_MOVE) { fprintf(stderr,"hero_move mode failed mode%d menu%d row%d\n",mode,menu_open,menu_cursor); return 0; }
    move_cursor(x,y);tap(KEY_A);
    fprintf(stderr,"hero_move issued selected%d mode%d order%d dest%d,%d xy%d,%d cursor%d,%d\n",selected_count(),mode,u[k].order,u[k].tx,u[k].ty,u[k].x,u[k].y,cx,cy);
    return mode==MODE_NORMAL&&u[k].order==ORDER_MOVE;
}
static int hero_attack_move(int x,int y)
{
    int k=kerrigan();
    if(k<0||!select_slot(k)||selected_count()!=1||!u[k].selected)return 0;
    tap(KEY_B);
    int count=field_menu_count();
    if(!menu_open||MENU_ATTACK>=count) { if(menu_open)tap(KEY_B); return 0; }
    for(int i=0;i<count&&menu_cursor!=MENU_ATTACK;i++)tap(KEY_DOWN);
    if(menu_cursor!=MENU_ATTACK) { tap(KEY_B); return 0; }
    tap(KEY_A);
    if(mode!=MODE_ATTACK) {
#ifdef SC64_DIRECT_ATTACK
        controller_cancel_armed_attack();
#endif
        return 0;
    }
#ifdef SC64_DIRECT_ATTACK
    int guard_was_busy=z6_rest_guard_busy;
    z6_rest_guard_busy=1;
    int clicked=controller_click_attackmove(u[k].x,u[k].y,x,y);
    z6_rest_guard_busy=guard_was_busy;
    if(!clicked) {
        controller_cancel_armed_attack();
        return 0;
    }
    assert(u[k].order!=ORDER_ATTACK_TARGET);
#else
    move_cursor(x,y);tap(KEY_A);
#endif
    fprintf(stderr,"normal-input Kerrigan attack-move selected%d order%d dest%d,%d xy%d,%d\n",
        selected_count(),u[k].order,u[k].tx,u[k].ty,u[k].x,u[k].y);
    return mode==MODE_NORMAL&&u[k].alive&&u[k].order==ORDER_ATTACK_MOVE;
}
static int z6_detector_sees_at(int slot,int x,int y)
{
    if(slot<0||slot>=MAX_UNITS||!u[slot].alive)return 1;
    Unit probe=u[slot];
    Sc64Z5DetectBox box;
    probe.x=x;probe.y=y;
    return !race_z6_cloak_target_box(&probe,&box)||race_z6_cloak_enemy_detector_sees(&box);
}
/* True when a hostile attacker could hit Kerrigan resting at x,y. Ranges use
 * the stock weapon table, plus a margin for closing movement; a cloak
 * no detector covers keeps her untargetable, as in the source. */
static int z6_point_threatened(int hero,int x,int y,int margin)
{
    if(hero<0||!u[hero].alive)return 1;
    if(race_z6.cloak.active&&!z6_detector_sees_at(hero,x,y))return 0;
    for(int i=0;i<MAX_UNITS;i++) {
        int range;
        if(!u[i].alive||!owners_hostile(SIDE,u[i].owner))continue;
        range=unit_range(u[i].type);
        /* Reaver scarabs are outside the generic weapon table that
         * race_can_attack consults; use the per-type damage instead. */
        if(range<=0||unit_damage(u[i].type)<=0)continue;
        if(dist2(x,y,u[i].x,u[i].y)<=(range+margin)*(range+margin))return 1;
    }
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(building_complete(i)&&b[i].hp>0&&b[i].raw_type==162&&
           owners_hostile(SIDE,b[i].owner)&&
           dist2(x,y,b[i].x,b[i].y)<=(112+margin)*(112+margin))return 1;
    return 0;
}
/* The rest point behind the cleared northern base can fall inside a
 * Reaver's 8-tile range. Move to the nearest walkable cell within 320 px
 * that no attacker reaches; the ordinary Move order routes there. */
static int z6_hold_safely(void)
{
    int hero=kerrigan(),best_x=-1,best_y=-1,best_d=0x7fffffff;
    if(hero<0||!u[hero].alive||SC64_TRANSPORT_LOADED(&u[hero])||
       !z6_point_threatened(hero,u[hero].x,u[hero].y,48))return 0;
    for(int dy=-320;dy<=320;dy+=16)for(int dx=-320;dx<=320;dx+=16) {
        int x=u[hero].x+dx,y=u[hero].y+dy,d=dx*dx+dy*dy,col=x/16,row=y/16;
        if(d<48*48||d>320*320||d>=best_d||x<0||y<0||col>=MAP_COLS||row>=MAP_ROWS||
           !SC64_GRID_GET(path_walkable,col+row*MAP_COLS)||
           z6_point_threatened(hero,x,y,80))continue;
        best_x=x;best_y=y;best_d=d;
    }
    if(best_x<0||!hero_move(best_x,best_y))return 0;
    unsigned start=host_ticks;
    while(hero>=0&&u[hero].alive&&dist2(u[hero].x,u[hero].y,best_x,best_y)>24*24&&
          !z6_point_threatened(hero,best_x,best_y,80)&&
          host_ticks-start<900u&&screen==SCREEN_PLAY&&!won&&!lost)input(0);
    if(hero>=0&&u[hero].alive&&z6_point_threatened(hero,best_x,best_y,80)) {
        fprintf(stderr,"normal-input Kerrigan safe-point invalidated; replan target%d,%d xy%d,%d tick%u\n",
            best_x,best_y,u[hero].x,u[hero].y,host_ticks);
        return 1;
    }
    fprintf(stderr,"normal-input Kerrigan held clear of attackers at%d,%d xy%d,%d hp%d tick%u\n",
        best_x,best_y,hero>=0?u[hero].x:-1,hero>=0?u[hero].y:-1,hero>=0?u[hero].hp:-1,host_ticks);
    return 1;
}

/* Unaided Overlords are slow. Wait while the flight keeps closing on its
 * point: stop after 900 ticks without getting closer, or 20000 in total. */
static int z6_wait_closing(int mover,int guard,int x,int y,int radius)
{
    unsigned start=host_ticks,last_progress=host_ticks;
    int best=dist2(u[mover].x,u[mover].y,x,y);
    while(host_ticks-start<20000u&&host_ticks-last_progress<900u&&
          screen==SCREEN_PLAY&&!won&&!lost&&u[mover].alive&&
          (guard<0||u[guard].alive)&&best>radius*radius) {
        input(0);
        int d=dist2(u[mover].x,u[mover].y,x,y);
        if(d<best) { best=d; last_progress=host_ticks; }
        if(host_ticks%500u==0)
            fprintf(stderr,"flight tick%u mover xy%d,%d order%d d2%d\n",
                host_ticks,u[mover].x,u[mover].y,u[mover].order,d);
    }
    return u[mover].alive&&(guard<0||u[guard].alive)&&
        dist2(u[mover].x,u[mover].y,x,y)<=radius*radius;
}
static void z6_log_detectors(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==84&&owners_hostile(SIDE,u[i].owner))
        fprintf(stderr,"Z6 detector Observer slot%d owner%d xy%d,%d range144\n",i,u[i].owner,u[i].x,u[i].y);
    for(int i=0;i<MAX_BUILDINGS;i++)if(building_complete(i)&&b[i].hp>0&&
        b[i].raw_type==162&&owners_hostile(SIDE,b[i].owner))
        fprintf(stderr,"Z6 detector PhotonCannon slot%d owner%d xy%d,%d range112\n",
            i,b[i].owner,b[i].x,b[i].y);
}
static int z6_safe_storm_rally(int hero,int *out_x,int *out_y,int *out_targets)
{
    int best_targets=0,best_travel=0x7fffffff,best_x=-1,best_y=-1;
    if(hero<0||!out_x||!out_y||!out_targets)return 0;
    for(int y=1760;y<=1968;y+=16)for(int x=1664;x<=1952;x+=16) {
        int col=x/16,row=y/16,cell=col+row*MAP_COLS;
        Unit probe=u[hero];Sc64Z5DetectBox box;int targets=0,nearest=0x7fffffff;
        if(col<0||row<0||col>=MAP_COLS||row>=MAP_ROWS||
           !SC64_GRID_GET(path_walkable,cell))continue;
        probe.x=x;probe.y=y;
        if(!race_z6_cloak_target_box(&probe,&box)||race_z6_cloak_enemy_detector_sees(&box))continue;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&owners_hostile(SIDE,u[i].owner)) {
            int d=dist2(x,y,u[i].x,u[i].y);
            if(d<nearest)nearest=d;
            if(d<=144*144)targets++;
        }
        /* Keep Kerrigan out of point-blank range while still supporting a
         * Storm cast. The final comparison favors the best safe cluster. */
        if(targets>0&&nearest>=72*72) {
            int travel=dist2(x,y,u[hero].x,u[hero].y);
            if(targets>best_targets||(targets==best_targets&&travel<best_travel)) {
                best_targets=targets;best_travel=travel;best_x=x;best_y=y;
            }
        }
    }
    if(best_x<0)return 0;
    *out_x=best_x;*out_y=best_y;*out_targets=best_targets;
    return 1;
}
static void z6_log_hero_hit(int hero,int old_x,int old_y,int old_hp,int old_shield)
{
    int live=kerrigan();
    if(live>=0) {
        fprintf(stderr,"Kerrigan damage xy%d,%d ->%d,%d hp%d->%d shield%d->%d energy%d cloak%d detected%d targetable%d storms%d tick%u\n",
            old_x,old_y,u[live].x,u[live].y,old_hp,u[live].hp,old_shield,u[live].shield,
            u[live].carry/256,race_z6.cloak.active,z6_detector_sees_at(live,u[live].x,u[live].y),
            race_z6_cloaked_target_attackable(live),z6_storms_cast,host_ticks);
    } else fprintf(stderr,"Kerrigan death from slot%d xy%d,%d lastHP%d shield%d storms%d cloak%d carry%d tick%u\n",
        hero,old_x,old_y,old_hp,old_shield,z6_storms_cast,race_z6.cloak.active,
        hero>=0?u[hero].carry/256:-1,host_ticks);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&owners_hostile(SIDE,u[i].owner)) {
        int d=dist2(old_x,old_y,u[i].x,u[i].y),range=unit_range(u[i].type);
        if(u[i].target==hero||d<=(range+64)*(range+64))
            fprintf(stderr,"  nearby/target attacker slot%d raw%d owner%d src%d xy%d,%d d%d order%d target%d range%d cd%d\n",
                i,u[i].raw_type,u[i].owner,u[i].raw_owner,u[i].x,u[i].y,d,u[i].order,
                u[i].target,range,u[i].cd);
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner)&&
        (b[i].raw_type==162||dist2(old_x,old_y,b[i].x,b[i].y)<220*220))
        fprintf(stderr,"  nearby building slot%d raw%d owner%d xy%d,%d hp%d complete%d d%d\n",
            i,b[i].raw_type,b[i].owner,b[i].x,b[i].y,b[i].hp,building_complete(i),
            dist2(old_x,old_y,b[i].x,b[i].y));
}
static int morph_starting_hatchery_to_lair(void)
{
    int slot=raw_building(131,SIDE);
    if(slot<0)return raw_building(132,SIDE)>=0;
    if(!building_complete(slot)||b[slot].queue_count||!select_building_slot(slot))return 0;
    if(!choose_menu_row(MENU_ZP_LAIR))return 0;
    fprintf(stderr,"normal-input Lair morph slot%d raw%d building%d progress%d bank%d/%d\n",
        slot,b[slot].raw_type,b[slot].under_construction,b[slot].build_progress,minerals,gas);
    return b[slot].alive&&b[slot].raw_type==132&&b[slot].under_construction;
}
static int start_ventral_sacs(void)
{
    int slot=raw_building(132,SIDE);
    if(slot<0||!building_complete(slot)||!select_building_slot(slot))return 0;
    if(!choose_menu_row(MENU_STOP+1))return 0;
    fprintf(stderr,"normal-input Ventral Sacs result remaining%u researched%u bank%d/%d\n",
        race_z6.transport.ticks_remaining,race_z6.transport.researched,minerals,gas);
    return race_z6.transport.ticks_remaining!=0;
}
static int wait_research_and_stay_alive(unsigned max_ticks)
{
    unsigned start=host_ticks;
    unsigned last_storm=0;
    while(host_ticks-start<max_ticks&&screen==SCREEN_PLAY&&!won&&!lost&&
          race_z6.transport.ticks_remaining) {
        input(0);
        if(host_ticks-last_storm>=240u) {
            int k=kerrigan(),sx=0,sy=0,n=best_storm_point(k,&sx,&sy);
            if(k>=0&&n>=4&&u[k].carry>=75*256) {
                int cast=hero_storm(sx,sy);
                if(cast)fprintf(stderr,"defensive Kerrigan Storm cluster%d at%d,%d tick%u\n",n,sx,sy,host_ticks);
                last_storm=host_ticks;
            }
        }
    }
    return screen==SCREEN_PLAY&&!won&&!lost&&!race_z6.transport.ticks_remaining&&
        race_z6.transport.researched==1;
}
static int issue_selected_unit_move(int slot,int x,int y)
{
    if(!select_slot(slot))return 0;
    if(!choose_menu_row(MENU_MOVE))return 0;
    if(mode!=MODE_MOVE)return 0;
    move_cursor(x,y);tap(KEY_A);
    return mode==MODE_NORMAL&&u[slot].order==ORDER_MOVE&&
        dist2(u[slot].tx,u[slot].ty,x,y)<=4;
}
static int route_waypoint(int sx,int sy,int tx,int ty,int steps,int *out_x,int *out_y)
{
    int x=sx,y=sy,nx=0,ny=0;
    if(steps<1)steps=1;
    for(int i=0;i<steps;i++) {
        if(!path_next_tile(x,y,tx,ty,&nx,&ny))return 0;
        if(nx==x&&ny==y)break;
        x=nx;y=ny;
        if(dist2(x,y,tx,ty)<=24*24)break;
    }
    *out_x=x;*out_y=y;
    return x!=sx||y!=sy;
}
static int board_kerrigan_to_overlord(int carrier,int passenger)
{
    if(carrier<0||passenger<0||!select_slot(passenger))return 0;
    move_cursor(u[carrier].x,u[carrier].y);tap(KEY_A);
    fprintf(stderr,"normal-input board request k%d order%d target%d carrier%d,%d\n",
        passenger,u[passenger].order,u[passenger].target,u[carrier].x,u[carrier].y);
    return (u[passenger].order==ORDER_BOARD||u[passenger].order==ORDER_TRANSPORTED)&&
        u[passenger].target==carrier;
}
static int unload_selected_overlord_at(int carrier,int x,int y)
{
    if(!select_slot(carrier))return 0;
    if(!choose_menu_row(MENU_STOP+1))return 0;
    if(mode!=MODE_UNLOAD)return 0;
    move_cursor(x,y);tap(KEY_A);
    fprintf(stderr,"normal-input unload request carrier%d order%d target%d at%d,%d\n",
        carrier,u[carrier].order,u[carrier].target,x,y);
    return mode==MODE_NORMAL&&u[carrier].order==ORDER_UNLOAD&&u[carrier].target==-1;
}
static int clear_force2_locations(void)
{
    int support_prepared=0,support_x=-1,support_y=-1,support_targets=0;
    int support_casts=0,casts_since_retreat=0,support_waiting=0;
    (void)support_casts; /* Also compile under canonical Z6 flags without the DirectAttack controller. */
    for(unsigned loc=4;loc<=5&&screen==SCREEN_PLAY&&!won&&!lost;loc++) {
        int ll,tt,rr,bb;u32 before=0,after=0;unsigned rounds=0;
        if(!sc64_z6_world_location(&race_z6_world,loc,&ll,&tt,&rr,&bb)||
           !sc64_z6_world_count(&race_z6_world,19,229,loc,&before)) {
            fprintf(stderr,"source-force clearance location%u unavailable\n",loc);return 0;
        }
        fprintf(stderr,"normal-input Force2 clear loc%u bounds%d,%d-%d,%d initial%u\n",
            loc,ll,tt,rr,bb,before);
        if(loc==5) {
            z6_log_detectors();
            int hero=kerrigan();
            if(z6_safe_storm_rally(hero,&support_x,&support_y,&support_targets))
                fprintf(stderr,"normal-input safe Kerrigan support point%d,%d targets%d detector%d\n",
                    support_x,support_y,support_targets,z6_detector_sees_at(hero,support_x,support_y));
            else fprintf(stderr,"no safe Storm support point found for live loc5 units\n");
        }
        unsigned max_rounds=loc==4?10u:72u,last_production=host_ticks,stalled_rounds=0;
        u32 hp_before=hostile_health_total();
        while(before&&rounds++<max_rounds&&screen==SCREEN_PLAY&&!won&&!lost) {
            int wx,wy;
            if(!nearest_force2_in_region(ll,tt,rr,bb,&wx,&wy)) {
                /* The last target may have died since the count was read. */
                (void)sc64_z6_world_count(&race_z6_world,19,229,loc,&before);
                if(!before)break;
                if(loc==5&&before<=1) {
                    fprintf(stderr,"  no nearest Force2 target despite before=%u bounds%d,%d-%d,%d\n",
                        before,ll,tt,rr,bb);
                    for(int bi=0;bi<MAX_BUILDINGS;bi++)if(b[bi].alive)
                        fprintf(stderr,"  alive building%d raw%d owner%d rawowner%d xy%d,%d hp%d complete%d inregion%d match%d hostile%d\n",
                            bi,b[bi].raw_type,b[bi].owner,b[bi].raw_owner,b[bi].x,b[bi].y,b[bi].hp,
                            building_complete(bi),b[bi].x>=ll&&b[bi].x<rr&&b[bi].y>=tt&&b[bi].y<bb,
                            sc64_z6_world_owner_matches(&race_z6_world,19,b[bi].raw_owner,b[bi].owner),
                            owners_hostile(SIDE,b[bi].owner));
                }
                if(loc==5&&rounds<max_rounds&&host_ticks<Z6_LIMIT-2000u) {
                    for(unsigned wait=0;wait<1200u&&side_combat_count()<=1&&
                        screen==SCREEN_PLAY&&!won&&!lost;wait++)input(0);
                    if(side_combat_count()>1)continue;
                }
                break;
            }
            int final_building=-1;
            if(before<=1)for(int bi=0;bi<MAX_BUILDINGS;bi++)
                if(b[bi].alive&&b[bi].x>=ll&&b[bi].x<rr&&b[bi].y>=tt&&b[bi].y<bb&&
                   sc64_z6_world_owner_matches(&race_z6_world,19,b[bi].raw_owner,b[bi].owner)>0) {
                    /* Attack-move to an open cell beside the final building;
                     * its center is blocked terrain and can leave the whole
                     * group with an unreachable route goal. */
                    /* Keep the waypoint inside Hydra firing distance while
                     * remaining outside the structure's blocked footprint. */
                    wx=limit(b[bi].x-48,0,WORLD_W-1);wy=b[bi].y;
                    final_building=bi;
                    fprintf(stderr,"  final source building raw%d at%d,%d; approach waypoint%d,%d\n",
                        b[bi].raw_type,b[bi].x,b[bi].y,wx,wy);
                    break;
                }
            /* With only the last Force2 actors left, reinforcements trickle in
             * from the main base one at a time and can stall the round budget.
             * If no escort is near the target, Kerrigan finishes it with an
             * ordinary attack-move instead; victory conditions are unchanged. */
            int escort_near=0;
            for(int ei=0;ei<MAX_UNITS;ei++)
                if(u[ei].alive&&u[ei].owner==SIDE&&u[ei].raw_type!=41&&u[ei].raw_type!=42&&
                   u[ei].raw_type!=51&&dist2(u[ei].x,u[ei].y,wx,wy)<=320*320)escort_near=1;
            int hero_finish=loc==5&&before<=2&&!escort_near&&kerrigan()>=0;
            if(hero_finish)
                fprintf(stderr,"  no escort within 320px of final Force2 actors; Kerrigan finishes\n");
            if(before<=1&&!hero_finish) {
                int in_range=0,stepped=0;
                for(int ui=0;ui<MAX_UNITS;ui++)if(u[ui].alive&&u[ui].owner==SIDE&&u[ui].raw_type==38) {
                    if(dist2(u[ui].x,u[ui].y,wx,wy)<=64*64) { in_range++; continue; }
                    int px=0,py=0;
                    if(route_waypoint(u[ui].x,u[ui].y,wx,wy,16,&px,&py)) {
                        int moved=issue_selected_unit_move(ui,px,py);
                        fprintf(stderr,"  final Pylon Hydra slot%d at%d,%d route waypoint%d,%d issued%d order%d\n",
                            ui,u[ui].x,u[ui].y,px,py,moved,u[ui].order);
                        stepped+=moved;
                    }
                }
                if(!in_range&&stepped) {
                    unsigned step_start=host_ticks;
                    while(host_ticks-step_start<1800u&&screen==SCREEN_PLAY&&!won&&!lost)input(0);
                    continue;
                }
            }
            fprintf(stderr,"  focus Force2 target at%d,%d\n",wx,wy);
            if(hero_finish||!order_fresh_combat_group(1,wx,wy)) {
                if(loc==5&&before<=2&&kerrigan()>=0) {
                    int hero=kerrigan();
                    int px=0,py=0;
                    if(dist2(u[hero].x,u[hero].y,wx,wy)>80*80&&
                       route_waypoint(u[hero].x,u[hero].y,wx,wy,16,&px,&py)) {
                        int moved=hero_move(px,py);
                        fprintf(stderr,"  escort gone; Kerrigan route waypoint%d,%d issued%d\n",px,py,moved);
                        if(moved) {
                            unsigned solo_start=host_ticks;
                            while(host_ticks-solo_start<1800u&&screen==SCREEN_PLAY&&!won&&!lost)input(0);
                            continue;
                        }
                    }
                    fprintf(stderr,"  no escort group remains; using Kerrigan's ordinary attack-move on final target\n");
                    if(hero_attack_move(wx,wy)) {
                        unsigned solo_start=host_ticks;
                        /* Wait until a remaining Force2 actor dies (or the
                         * final building, when only it remains). */
                        while(host_ticks-solo_start<2400u&&screen==SCREEN_PLAY&&!won&&!lost) {
                            u32 left=before;
                            if(sc64_z6_world_count(&race_z6_world,19,229,loc,&left)&&left<before)break;
                            if(final_building>=0&&final_building<MAX_BUILDINGS&&
                               (!b[final_building].alive||b[final_building].hp<=0))break;
                            input(0);
                        }
                        (void)sc64_z6_world_count(&race_z6_world,19,229,loc,&after);
                        before=after;
                        if(!after)break;
                        continue;
                    }
                }
                fprintf(stderr,"normal-input attack-move to loc%u failed\n",loc);return 0;
            }
            if(loc==4&&rounds==1&&owned_building_count(B_HATCHERY)<1) {
                unsigned bank_start=host_ticks;
                while(minerals<300&&host_ticks-bank_start<1800u&&screen==SCREEN_PLAY&&!won&&!lost)
                    input(0);
                if(minerals>=300) {
                    int built=build_extra_hatchery();
                    fprintf(stderr,"normal-input second Hatchery order%d count%d bank%d/%d\n",
                        built,owned_building_count(B_HATCHERY),minerals,gas);
                } else fprintf(stderr,"normal-input Hatchery skipped bank%d/%d\n",minerals,gas);
            }
            if(loc==5&&!support_prepared&&support_x>=0) {
                int hero=kerrigan();unsigned move_start=host_ticks;
                /* Stay behind the cleared base while approaching from the
                 * north. Cloak only for the final leg into live AA coverage. */
                if(hero>=0&&hero_move(1728,1456)) {
                    while(hero>=0&&u[hero].alive&&dist2(u[hero].x,u[hero].y,1728,1456)>64*64&&
                          host_ticks-move_start<6000u&&screen==SCREEN_PLAY&&!won&&!lost)input(0);
                    hero=kerrigan();
                    if(hero>=0&&u[hero].alive&&dist2(u[hero].x,u[hero].y,1728,1456)<=64*64) {
                        if(!race_z6.cloak.active) (void)hero_cloak();
                        if(race_z6.cloak.active&&hero_move(support_x,support_y))
                            fprintf(stderr,"normal-input Kerrigan cloaked support move target%d,%d tick%u\n",
                                support_x,support_y,host_ticks);
                        else fprintf(stderr,"Kerrigan support final move unavailable cloaked%d energy%d\n",
                            race_z6.cloak.active,u[hero].carry/256);
                    } else fprintf(stderr,"Kerrigan did not reach north staging point; xy%d,%d\n",
                        hero>=0?u[hero].x:-1,hero>=0?u[hero].y:-1);
                } else fprintf(stderr,"Kerrigan north staging move failed\n");
                support_prepared=1;
            }
            unsigned started=host_ticks,last_report=started,last_cast=started;
            unsigned last_group_refresh=started;
            int frame_started=frame;
            fprintf(stderr,"  wait begin host%u frame%d screen%d mode%d menuopen%d\n",
                started,frame,screen,mode,menu_open);
            while(host_ticks-started<2500u&&screen==SCREEN_PLAY&&!won&&!lost) {
                input(0);
                if(support_waiting&&(host_ticks-started)%60u==0u)(void)z6_hold_safely();
                int resting_hero=kerrigan();
                if(support_waiting&&resting_hero>=0&&u[resting_hero].carry>=240*256&&
                   !z6_detector_sees_at(resting_hero,u[resting_hero].x,u[resting_hero].y)) {
                    if(!race_z6.cloak.active)(void)hero_cloak();
                    resting_hero=kerrigan();
                    if(resting_hero>=0&&race_z6.cloak.active&&hero_move(support_x,support_y)) {
                        unsigned advance_start=host_ticks;
                        while(resting_hero>=0&&u[resting_hero].alive&&
                              dist2(u[resting_hero].x,u[resting_hero].y,support_x,support_y)>64*64&&
                              host_ticks-advance_start<2400u&&screen==SCREEN_PLAY&&!won&&!lost)input(0);
                        resting_hero=kerrigan();
                        if(resting_hero>=0&&dist2(u[resting_hero].x,u[resting_hero].y,
                                                  support_x,support_y)<=64*64) {
                            support_waiting=0;
                            z6_rest_guard=0;
                            fprintf(stderr,"normal-input Kerrigan rejoined safe Storm point xy%d,%d energy%d tick%u\n",
                                u[resting_hero].x,u[resting_hero].y,u[resting_hero].carry/256,host_ticks);
                        } else fprintf(stderr,"Kerrigan failed safe-point return xy%d,%d detector%d tick%u\n",
                            resting_hero>=0?u[resting_hero].x:-1,resting_hero>=0?u[resting_hero].y:-1,
                            resting_hero>=0?z6_detector_sees_at(resting_hero,u[resting_hero].x,u[resting_hero].y):1,
                            host_ticks);
                    }
                }
                if(host_ticks-last_production>=600u) {
                    int hydras=side_raw_count(38),queued=0,workers=0;
                    for(int wi=0;wi<MAX_UNITS;wi++)workers+=u[wi].alive&&u[wi].owner==SIDE&&u[wi].raw_type==41;
                    if(workers<8&&owner_count(SIDE)<48&&minerals>=50)
                        queued=queue_hatchery_type(41,1);
                    else if(hydras<30&&owner_count(SIDE)<48&&minerals>=75&&gas>=25)
                        queued=queue_hatchery_type(38,1);
                    if(queued)fprintf(stderr,"normal-input ongoing production at%u Hydras%d workers%d bank%d/%d\n",
                        host_ticks,hydras,workers,minerals,gas);
                    last_production=host_ticks;
                }
                if(side_gas_workers()<2) (void)assign_available_gas_workers(2);
                /* Units created after an order are idle until a later player
                 * command. Reissue the current attack-move only when at least
                 * one newly available combat unit is idle, and rate-limit it. */
                int idle_combat=idle_side_combat_count();
                if(idle_combat>0&&host_ticks-last_group_refresh>=900u) {
                    fprintf(stderr,"normal-input route idle reinforcements%d at%u toward%d,%d\n",
                        idle_combat,host_ticks,wx,wy);
                    if(order_group(1,wx,wy))last_group_refresh=host_ticks;
                }
                if(host_ticks-last_cast>=300u) {
                    int hero=kerrigan(),sx=0,sy=0,n=best_storm_point(hero,&sx,&sy);
                    if(hero>=0&&n>=2&&u[hero].carry>=150*256) {
                        int fired=controller_cast_storm_pair(hero,sx,sy,n);
                        if(fired>0) {
                            support_casts+=fired;
                            casts_since_retreat++;
                            z6_storms_cast+=fired;
                            fprintf(stderr,"normal-input Kerrigan Storm pair result%d initialcluster%d at%d,%d tick%u\n",
                                fired,n,sx,sy,host_ticks);
                        if(casts_since_retreat>=1) {
                            hero=kerrigan();
                            if(hero>=0&&u[hero].alive&&hero_move(1728,1456)) {
                                unsigned retreat_start=host_ticks;
                                while(hero>=0&&u[hero].alive&&
                                      dist2(u[hero].x,u[hero].y,1728,1456)>64*64&&
                                      host_ticks-retreat_start<1800u&&screen==SCREEN_PLAY&&!won&&!lost)
                                    input(0);
                                hero=kerrigan();
                                if(hero>=0&&u[hero].alive&&race_z6.cloak.active&&
                                   !z6_detector_sees_at(hero,u[hero].x,u[hero].y))
                                    (void)hero_cloak();
                                fprintf(stderr,"normal-input Kerrigan retreat after Storm sortie xy%d,%d hp%d energy%d cloak%d\n",
                                    hero>=0?u[hero].x:-1,hero>=0?u[hero].y:-1,
                                    hero>=0?u[hero].hp:-1,hero>=0?u[hero].carry/256:-1,
                                    race_z6.cloak.active);
                            }
                            casts_since_retreat=0;
                            support_waiting=hero>=0&&u[hero].alive;
                            z6_rest_guard=support_waiting;
                            if(support_waiting)(void)z6_hold_safely();
                        }
                    }
                    }
                    last_cast=host_ticks;
                }
                if(host_ticks-last_report>=1000u) {
                    (void)sc64_z6_world_count(&race_z6_world,19,229,loc,&after);
                    int hero=kerrigan(),sx=0,sy=0,clusters=best_storm_point(hero,&sx,&sy);
                    fprintf(stderr,"  loc%u clearance tick%u frame%d(+%d) screen%d mode%d menuopen%d force2units%u hostilehp%u sidecombat%d hero%d@%d,%d energy%d cloak%d detected%d storm%d@%d,%d result%d\n",
                        loc,host_ticks,frame,frame-frame_started,screen,mode,menu_open,after,hostile_health_total(),side_combat_count(),hero,
                        hero>=0?u[hero].x:-1,hero>=0?u[hero].y:-1,hero>=0?u[hero].carry:0,
                        race_z6.cloak.active,hero>=0?z6_detector_sees_at(hero,u[hero].x,u[hero].y):0,
                        clusters,sx,sy,race_z6.runtime.model.outcome);
                    if(hero>=0&&clusters==0)trace_storm_candidates(hero);
                    last_report=host_ticks;
                }
                if(!sc64_z6_world_count(&race_z6_world,19,229,loc,&after)||!after)break;
            }
            (void)sc64_z6_world_count(&race_z6_world,19,229,loc,&after);
            u32 hp_after=hostile_health_total();
            if(after>=before&&hp_after>=hp_before) {
                if(++stalled_rounds>=4u) {
                fprintf(stderr,"bounded clear stalled loc%u force2units%u (started%u)\n",loc,after,before);
                fprintf(stderr,"  source bank%d/%d sideunits%d\n",minerals,gas,side_combat_count());
                if(loc==5&&before<=1) {
                    int hero=kerrigan();
                    if(hero>=0) {
                        int q=nearest_enemy_typed(u[hero].x,u[hero].y,SIDE,256,u[hero].type);
                        fprintf(stderr,"  final target audit hero slot%d raw%d owner%d xy%d,%d query%d attackable%d\n",
                            hero,u[hero].raw_type,u[hero].owner,u[hero].x,u[hero].y,q,
                            q>=0&&SC64_CAN_ATTACK_FROM(u[hero].type,q,SIDE));
                        if(q>=MAX_UNITS)fprintf(stderr,"  query building%d raw%d own%d rawown%d xy%d,%d hp%d flags%x complete%d\n",
                            q-MAX_UNITS,b[q-MAX_UNITS].raw_type,b[q-MAX_UNITS].owner,
                            b[q-MAX_UNITS].raw_owner,b[q-MAX_UNITS].x,b[q-MAX_UNITS].y,
                            b[q-MAX_UNITS].hp,b[q-MAX_UNITS].flags,building_complete(q-MAX_UNITS));
                    }
                }
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].x>=ll&&u[i].x<rr&&u[i].y>=tt&&u[i].y<bb&&
                    sc64_z6_world_owner_matches(&race_z6_world,19,u[i].raw_owner,u[i].owner)>0)
                    fprintf(stderr,"  Force2 unit slot%d raw%d xy%d,%d hp%d sh%d order%d target%d\n",
                        i,u[i].raw_type,u[i].x,u[i].y,u[i].hp,u[i].shield,u[i].order,u[i].target);
                for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].x>=ll&&b[i].x<rr&&b[i].y>=tt&&b[i].y<bb&&
                    sc64_z6_world_owner_matches(&race_z6_world,19,b[i].raw_owner,b[i].owner)>0)
                    fprintf(stderr,"  Force2 building slot%d raw%d xy%d,%d hp%d complete%d\n",
                        i,b[i].raw_type,b[i].x,b[i].y,b[i].hp,building_complete(i));
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
                    u[i].raw_type!=41&&u[i].raw_type!=42) {
                    int nx=-1,ny=-1,has=path_next_tile(u[i].x,u[i].y,u[i].tx,u[i].ty,&nx,&ny);
                    int clear=has?unit_step_clear(&u[i],nx,ny):0;
                    fprintf(stderr,"  SIDE combat slot%d raw%d xy%d,%d order%d target%d tx%d,%d cd%d nearest%d range%d dmg%d\n",
                        i,u[i].raw_type,u[i].x,u[i].y,u[i].order,u[i].target,u[i].tx,u[i].ty,u[i].cd,
                        nearest_enemy_typed(u[i].x,u[i].y,u[i].owner,unit_range(u[i].type)>64?unit_range(u[i].type)+18:64,u[i].type),
                        unit_range(u[i].type),unit_damage(u[i].type));
                    fprintf(stderr,"    route has%d next%d,%d clear%d path%d,%d timer%d retry%d cellwalk%d\n",
                        has,nx,ny,clear,u[i].path_x,u[i].path_y,u[i].path_timer,u[i].path_retry,
                        SC64_GRID_GET(path_walkable,(u[i].x/16)+(u[i].y/16)*MAP_COLS));
                }
                return 0;
                }
            } else stalled_rounds=0;
            if(after>before) {
                fprintf(stderr,"  loc%u bounded enemy reinforcement %u -> %u; retargeting\n",loc,before,after);
            } else if(after==before) {
                fprintf(stderr,"  loc%u source count stable at%u but enemy damage recorded\n",loc,after);
            }
            if(after<before) fprintf(stderr,"  loc%u source-count progress %u -> %u; bounded retry\n",loc,before,after);
            before=after;
            hp_before=hp_after;
        }
        if(before) {
            fprintf(stderr,"clearance stopped loc%u remaining%u rounds%u screen%d won%d lost%d result%d hero%d\n",
                loc,before,rounds,screen,won,lost,race_z6.runtime.model.outcome,kerrigan());
            if(loc==5) {
                int hero=kerrigan();
                if(hero>=0) {
                    int q=nearest_enemy_typed(u[hero].x,u[hero].y,SIDE,256,u[hero].type);
                    fprintf(stderr,"  final census hero%d raw%d xy%d,%d query%d attackable%d\n",
                        hero,u[hero].raw_type,u[hero].x,u[hero].y,q,
                        q>=0&&SC64_CAN_ATTACK_FROM(u[hero].type,q,SIDE));
                }
                for(int bi=0;bi<MAX_BUILDINGS;bi++)if(b[bi].alive&&
                    b[bi].x>=ll&&b[bi].x<rr&&b[bi].y>=tt&&b[bi].y<bb)
                    fprintf(stderr,"  final region building%d raw%d own%d rawown%d xy%d,%d hp%d flags%x complete%d match%d hostile%d\n",
                        bi,b[bi].raw_type,b[bi].owner,b[bi].raw_owner,b[bi].x,b[bi].y,b[bi].hp,
                        b[bi].flags,building_complete(bi),
                        sc64_z6_world_owner_matches(&race_z6_world,19,b[bi].raw_owner,b[bi].owner),
                        owners_hostile(SIDE,b[bi].owner));
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE)
                    fprintf(stderr,"  final SIDE unit%d raw%d xy%d,%d hp%d order%d target%d\n",
                        i,u[i].raw_type,u[i].x,u[i].y,u[i].hp,u[i].order,u[i].target);
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&owners_hostile(SIDE,u[i].owner))
                    fprintf(stderr,"  final hostile unit%d raw%d own%d rawown%d xy%d,%d hp%d\n",
                        i,u[i].raw_type,u[i].owner,u[i].raw_owner,u[i].x,u[i].y,u[i].hp);
                for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner))
                    fprintf(stderr,"  final hostile building%d raw%d own%d rawown%d xy%d,%d hp%d\n",
                        i,b[i].raw_type,b[i].owner,b[i].raw_owner,b[i].x,b[i].y,b[i].hp);
            }
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].x>=ll&&u[i].x<rr&&u[i].y>=tt&&u[i].y<bb&&
                sc64_z6_world_owner_matches(&race_z6_world,19,u[i].raw_owner,u[i].owner)>0)
                fprintf(stderr,"  remaining Force2 unit slot%d raw%d xy%d,%d hp%d sh%d order%d target%d\n",
                    i,u[i].raw_type,u[i].x,u[i].y,u[i].hp,u[i].shield,u[i].order,u[i].target);
            return 0;
        }
    }
    return screen==SCREEN_PLAY&&!won&&!lost;
}
static int z6_transport_campaign(void)
{
    fprintf(stderr,"Z6 transport start tick%u resources%d/%d\n",host_ticks,minerals,gas);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE)
        fprintf(stderr,"SIDE start slot%d raw%d xy%d,%d hp%d order%d\n",
            i,u[i].raw_type,u[i].x,u[i].y,u[i].hp,u[i].order);
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE)
        fprintf(stderr,"building slot%d raw%d xy%d,%d complete%d queue%d\n",
            i,b[i].raw_type,b[i].x,b[i].y,building_complete(i),b[i].queue_count);

    /* The authenticated map already contains the Spawning Pool prerequisite.
     * Morph the starting Hatchery and buy Ventral Sacs through normal UI. */
    if(raw_building(132,SIDE)<0&&!morph_starting_hatchery_to_lair()) {
        fprintf(stderr,"normal-input Lair morph failed\n"); return 0;
    }
    int lair=raw_building(132,SIDE);
    unsigned phase_start=host_ticks,last_storm=0;
    while(lair>=0&&b[lair].alive&&!building_complete(lair)&&
          host_ticks-phase_start<6000u&&screen==SCREEN_PLAY&&!won&&!lost) {
        input(0);
        if(host_ticks-last_storm>=240u) {
            int k=kerrigan(),sx=0,sy=0,n=best_storm_point(k,&sx,&sy);
            if(k>=0&&n>=4&&u[k].carry>=75*256&&hero_storm(sx,sy))
                fprintf(stderr,"Lair defense Storm cluster%d tick%u\n",n,host_ticks);
            last_storm=host_ticks;
        }
    }
    if(lair<0||!b[lair].alive||!building_complete(lair)) {
        fprintf(stderr,"Lair incomplete tick%u\n",host_ticks); return 0;
    }
    if(raw_building(149,SIDE)<0&&!build_extractor()) {
        fprintf(stderr,"could not place source-economy Extractor (geysers%d)\n",geyser_count);
        return 0;
    }
    int extractor=raw_building(149,SIDE);
    unsigned gas_setup_start=host_ticks;
    while(extractor>=0&&b[extractor].alive&&!building_complete(extractor)&&
          host_ticks-gas_setup_start<5000u&&screen==SCREEN_PLAY&&!won&&!lost)
        input(0);
    int gas_workers=assign_available_gas_workers(2);
    unsigned assign_start=host_ticks;
    while(gas_workers<2&&host_ticks-assign_start<1800u&&screen==SCREEN_PLAY&&!won&&!lost) {
        input(0);
        gas_workers=assign_available_gas_workers(2);
    }
    fprintf(stderr,"normal-input Extractor slot%d complete%d assigned%d workers bank%d/%d geysers%d\n",
        extractor,extractor>=0?building_complete(extractor):0,gas_workers,
        minerals,gas,geyser_count);
    if(extractor<0||!building_complete(extractor)||gas_workers==0) {
        fprintf(stderr,"no working gas economy after normal inputs\n"); return 0;
    }
    /* Keep the initial minerals for the mainland fight; buy the island ferry
     * upgrade only after both authored Force2 unit regions are cleared. */
    int hydras_before=side_raw_count(38);
    if(minerals>=150&&gas>=50&&queue_hatchery_type(38,2)) {
        unsigned train_start=host_ticks;
        while(side_raw_count(38)<hydras_before+2&&host_ticks-train_start<2400u&&
              screen==SCREEN_PLAY&&!won&&!lost)input(0);
        fprintf(stderr,"normal-input pre-assault Hydralisks %d -> %d bank%d/%d\n",
            hydras_before,side_raw_count(38),minerals,gas);
    }
    if(!clear_force2_locations()) {
        fprintf(stderr,"source Force2 zones were not cleared within bounded normal-input attempts\n");return 0;
    }
    z6_rest_guard=0; /* Storm support ends with the Force2 zones. */

    while((minerals<(int)SC64_Z6_TRANSPORT_MINERAL_COST||
           gas<(int)SC64_Z6_TRANSPORT_GAS_COST)&&host_ticks-phase_start<20000u&&
          screen==SCREEN_PLAY&&!won&&!lost)input(0);
    if(minerals<(int)SC64_Z6_TRANSPORT_MINERAL_COST||
       gas<(int)SC64_Z6_TRANSPORT_GAS_COST||!start_ventral_sacs()) {
        fprintf(stderr,"Ventral Sacs unavailable bank%d/%d tick%u\n",minerals,gas,host_ticks);
        return 0;
    }
    if(!wait_research_and_stay_alive(10000u)) {
        fprintf(stderr,"Ventral Sacs incomplete tick%u remaining%u researched%u outcome%u\n",
            host_ticks,race_z6.transport.ticks_remaining,race_z6.transport.researched,
            race_z6.runtime.model.outcome);
        return 0;
    }
    fprintf(stderr,"Ventral Sacs complete tick%u frame%u\n",host_ticks,(unsigned)frame);

    int k=kerrigan(),carrier=-1,best=0x7fffffff;
    if(k<0) { fprintf(stderr,"Kerrigan dead before boarding\n"); return 0; }
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_owner==3&&u[i].raw_type==42) {
        int d=dist2(u[i].x,u[i].y,u[k].x,u[k].y);
        if(d<best){best=d;carrier=i;}
    }
    if(carrier>=0&&best>28*28) {
        int bx=u[k].x,by=u[k].y;
        fprintf(stderr,"normal-input ferry rendezvous carrier%d xy%d,%d Kerrigan%d xy%d,%d\n",
            carrier,u[carrier].x,u[carrier].y,k,bx,by);
        if(!issue_selected_unit_move(carrier,bx,by)) {
            fprintf(stderr,"normal-input Overlord rendezvous order failed\n");return 0;
        }
        (void)z6_wait_closing(carrier,k,bx,by,28);
        best=dist2(u[carrier].x,u[carrier].y,u[k].x,u[k].y);
        fprintf(stderr,"normal-input ferry rendezvous end tick%u carrier%d,%d Kerrigan%d,%d d2%d\n",
            host_ticks,u[carrier].x,u[carrier].y,u[k].x,u[k].y,best);
    }
    if(carrier<0||best>28*28||!board_kerrigan_to_overlord(carrier,k)) {
        fprintf(stderr,"normal-input board failed carrier%d\n",carrier); return 0;
    }
    trace_carrier_slot=carrier;
    unsigned board_start=host_ticks;
    while(host_ticks-board_start<3000u&&screen==SCREEN_PLAY&&!won&&!lost&&
          u[k].alive&&u[k].order!=ORDER_TRANSPORTED) input(0);
    if(!u[k].alive||u[k].order!=ORDER_TRANSPORTED||u[k].target!=carrier) {
        fprintf(stderr,"Kerrigan not loaded tick%u alive%d order%d target%d\n",
            host_ticks,u[k].alive,u[k].order,u[k].target); return 0;
    }

    int left=0,top=0,right=0,bottom=0;
    if(!sc64_z6_world_location(&race_z6_world,1,&left,&top,&right,&bottom)) {
        fprintf(stderr,"source island location unavailable\n"); return 0;
    }
    int goal_x=(left+right)/2,goal_y=(top+bottom)/2;
    fprintf(stderr,"Overlord ferry carrier%d to loc1 %d,%d bounds%d,%d-%d,%d\n",
        carrier,goal_x,goal_y,left,top,right,bottom);
    if(!issue_selected_unit_move(carrier,goal_x,goal_y)) {
        fprintf(stderr,"normal-input Overlord move failed order%d mode%d selected%d count%d target%d dest%d,%d xy%d,%d\n",
            u[carrier].order,mode,u[carrier].selected,selected_count(),u[carrier].target,
            u[carrier].tx,u[carrier].ty,u[carrier].x,u[carrier].y); return 0;
    }
    (void)z6_wait_closing(carrier,k,goal_x,goal_y,28);
    if(!u[carrier].alive||!u[k].alive||
       dist2(u[carrier].x,u[carrier].y,goal_x,goal_y)>28*28) {
        fprintf(stderr,"Overlord flight failed at tick%u\n",host_ticks); return 0;
    }
    if(!unload_selected_overlord_at(carrier,goal_x,goal_y)) {
        fprintf(stderr,"normal-input unload failed mode%d cargo%d\n",
            mode,race_transport_cargo_space(carrier)); return 0;
    }
    unsigned unload_start=host_ticks;
    while(host_ticks-unload_start<300u&&screen==SCREEN_PLAY&&!won&&!lost&&
          u[k].alive&&u[k].order==ORDER_TRANSPORTED) input(0);
    fprintf(stderr,"unloaded tick%u Kerrigan alive%d order%d owner%d xy%d,%d loc1=%d "
        "started%x fault%u\n",host_ticks,u[k].alive,u[k].order,u[k].owner,
        u[k].x,u[k].y,u[k].x>=left&&u[k].x<=right&&u[k].y>=top&&u[k].y<=bottom,
        race_z6.runtime.model.started,race_z6.runtime.fault);

    /* The source's follow-up condition counts player units in a smaller
     * island region. Leave Kerrigan there alone after unloading. */
    if(!issue_selected_unit_move(carrier,1120,900)) {
        fprintf(stderr,"normal-input Overlord clear-island move failed order%d dest%d,%d\n",
            u[carrier].order,u[carrier].tx,u[carrier].ty); return 0;
    }

    /* Source trigger 2 requires Force 2 to have no units in locations 4 and
     * 5. Attack-move the ordinary combat group through those actual regions;
     * stop if either bounded clearance attempt makes no source-count progress. */
    for(unsigned loc=4;loc<=5&&screen==SCREEN_PLAY&&!won&&!lost;loc++) {
        int ll,tt,rr,bb;u32 before=0,after=0;
        if(!sc64_z6_world_location(&race_z6_world,loc,&ll,&tt,&rr,&bb)||
           !sc64_z6_world_count(&race_z6_world,19,229,loc,&before)) {
            fprintf(stderr,"source-force clearance location%u unavailable\n",loc);return 0;
        }
        fprintf(stderr,"normal-input Force2 clear loc%u bounds%d,%d-%d,%d initial%u\n",
            loc,ll,tt,rr,bb,before);
        while(before&&screen==SCREEN_PLAY&&!won&&!lost) {
            int wx,wy;
            if(!nearest_force2_in_region(ll,tt,rr,bb,&wx,&wy))break;
            fprintf(stderr,"  focus Force2 target at%d,%d\n",wx,wy);
            if(!order_fresh_combat_group(1,wx,wy)) {
                fprintf(stderr,"normal-input attack-move to loc%u failed\n",loc);return 0;
            }
            unsigned started=host_ticks,last_report=started;
            unsigned last_cast=started;
            while(host_ticks-started<2500u&&screen==SCREEN_PLAY&&!won&&!lost) {
                input(0);
                if(host_ticks-last_cast>=300u) {
                    int hero=kerrigan(),sx=0,sy=0,n=best_storm_point(hero,&sx,&sy);
                    if(hero>=0&&n>=4&&u[hero].carry>=75*256&&hero_storm(sx,sy))
                        fprintf(stderr,"normal-input Kerrigan Storm cluster%d at%d,%d tick%u\n",n,sx,sy,host_ticks);
                    last_cast=host_ticks;
                }
                if(host_ticks-last_report>=1000u) {
                    (void)sc64_z6_world_count(&race_z6_world,19,229,loc,&after);
                    fprintf(stderr,"  loc%u clearance tick%u force2units%u hostilehp%u sidecombat%d\n",
                        loc,host_ticks,after,hostile_health_total(),side_combat_count());
                    last_report=host_ticks;
                }
                if(!sc64_z6_world_count(&race_z6_world,19,229,loc,&after)||!after)break;
            }
            (void)sc64_z6_world_count(&race_z6_world,19,229,loc,&after);
            if(after&&after>=before) {
                fprintf(stderr,"bounded clear stalled loc%u force2units%u (started%u)\n",loc,after,before);
                fprintf(stderr,"  source bank%d/%d sideunits%d\n",minerals,gas,side_combat_count());
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].x>=ll&&u[i].x<rr&&u[i].y>=tt&&u[i].y<bb&&
                    sc64_z6_world_owner_matches(&race_z6_world,19,u[i].raw_owner,u[i].owner)>0)
                    fprintf(stderr,"  Force2 unit slot%d raw%d xy%d,%d hp%d sh%d order%d target%d\n",
                        i,u[i].raw_type,u[i].x,u[i].y,u[i].hp,u[i].shield,u[i].order,u[i].target);
                for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].x>=ll&&b[i].x<rr&&b[i].y>=tt&&b[i].y<bb&&
                    sc64_z6_world_owner_matches(&race_z6_world,19,b[i].raw_owner,b[i].owner)>0)
                    fprintf(stderr,"  Force2 building slot%d raw%d xy%d,%d hp%d complete%d\n",
                        i,b[i].raw_type,b[i].x,b[i].y,b[i].hp,building_complete(i));
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
                    u[i].raw_type!=41&&u[i].raw_type!=42)
                    fprintf(stderr,"  SIDE combat slot%d raw%d xy%d,%d order%d target%d tx%d,%d cd%d nearest%d range%d dmg%d\n",
                        i,u[i].raw_type,u[i].x,u[i].y,u[i].order,u[i].target,u[i].tx,u[i].ty,u[i].cd,
                        nearest_enemy_typed(u[i].x,u[i].y,u[i].owner,unit_range(u[i].type)>64?unit_range(u[i].type)+18:64,u[i].type),
                        unit_range(u[i].type),unit_damage(u[i].type));
                return 0;
            }
            if(after) fprintf(stderr,"  loc%u source-count progress %u -> %u; bounded retry\n",loc,before,after);
            before=after;
        }
    }

    unsigned objective_start=host_ticks;
    while(host_ticks-objective_start<6000u&&screen==SCREEN_PLAY&&!won&&!lost) {
        input(0);
        if(host_ticks%500u==0) {
            fprintf(stderr,"objective tick%u frame%u result%d started%x Kerrigan%d at%d,%d carrier%d at%d,%d switch0%x switch1%x fault%u\n",
                host_ticks,(unsigned)frame,race_z6.runtime.model.outcome,
                race_z6.runtime.model.started,kerrigan(),
                kerrigan()>=0?u[kerrigan()].x:-1,kerrigan()>=0?u[kerrigan()].y:-1,
                carrier,u[carrier].x,u[carrier].y,
                race_z6.runtime.switch_bits[0]&1u,(race_z6.runtime.switch_bits[0]>>1)&1u,
                race_z6.runtime.fault);
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==79)
                fprintf(stderr,"  raw79 slot%d owner%d rawowner%d xy%d,%d hp%d order%d\n",
                    i,u[i].owner,u[i].raw_owner,u[i].x,u[i].y,u[i].hp,u[i].order);
        }
    }
    fprintf(stderr,"final tick%u frame%u won%d lost%d screen%d result%d started%x fault%u "
        "Kerrigan%d carrier%d cargo%d research%u\n",host_ticks,(unsigned)frame,
        won,lost,screen,race_z6.runtime.model.outcome,race_z6.runtime.model.started,
        race_z6.runtime.fault,kerrigan(),carrier,
        carrier>=0?race_transport_cargo_space(carrier):-1,race_z6.transport.researched);
    for(unsigned t=0;t<SC64_Z6_SOURCE_TRIGGER_COUNT;t++) {
        const Sc64Z6SourceTrigger *tr=&sc64_z6_source_triggers[t];
        for(unsigned c=0;c<tr->condition_count;c++) {
            const Sc64Z6SourceCondition *row=&sc64_z6_source_conditions[tr->first_condition+c];
            if(t==2||t==4||t==5)
                fprintf(stderr,"  cond t%u.%u opcode%u player%u unit%u loc%u cmp%u amount%u =>%d\n",
                    t,c,row->opcode,row->player,row->unit,row->location,row->comparison,row->amount,
                    sc64_z6_world_condition(&race_z6_world,&race_z6.runtime,row));
        }
    }
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].x>=896&&u[i].x<1008&&u[i].y>=960&&u[i].y<1072)
        fprintf(stderr,"  island SIDE slot%d raw%d xy%d,%d order%d\n",i,u[i].raw_type,u[i].x,u[i].y,u[i].order);
    return 0;
}
static int best_storm_point(int slot,int *out_x,int *out_y)
{
    int best=-1,best_score=0,best_count=0,best_d2=0x7fffffff;
    if(slot<0)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&!SC64_TRANSPORT_LOADED(&u[i])&&
        owners_hostile(SIDE,u[i].owner)&&
        currently_visible(u[i].x,u[i].y)) {
        int dx=u[i].x-u[slot].x,dy=u[i].y-u[slot].y,d2=dx*dx+dy*dy;
        int count=0,friendly=0;
        if(d2>144*144)continue;
        for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&!SC64_TRANSPORT_LOADED(&u[j])&&
            owners_hostile(SIDE,u[j].owner)&&
            currently_visible(u[j].x,u[j].y)) {
            int sx=u[j].x-u[i].x,sy=u[j].y-u[i].y;
            if(sx*sx+sy*sy<=24*24)count++;
        }
        for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE) {
            int sx=u[j].x-u[i].x,sy=u[j].y-u[i].y;
            if(sx*sx+sy*sy<=24*24)friendly++;
        }
        /* Storm has source-faithful friendly fire. Prefer empty splash zones,
         * but accept a dense-enemy trade when it is clearly favorable. */
        int score=count-2*friendly;
        if(count>=2&&(score>best_score||(score==best_score&&d2<best_d2))) {
            best=i;best_score=score;best_count=count;best_d2=d2;
        }
    }
    if(best<0)return 0;
    *out_x=u[best].x;*out_y=u[best].y;return best_count;
}

typedef struct {
    int valid,raw_type,raw_owner,owner,hp,shield;
} ControllerStormTarget;

static void controller_storm_snapshot(int x,int y,ControllerStormTarget *before)
{
    memset(before,0,sizeof(ControllerStormTarget)*MAX_UNITS);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&!SC64_TRANSPORT_LOADED(&u[i])&&
        owners_hostile(SIDE,u[i].owner)&&
        currently_visible(u[i].x,u[i].y)&&dist2(x,y,u[i].x,u[i].y)<=24*24) {
        before[i].valid=1;before[i].raw_type=u[i].raw_type;
        before[i].raw_owner=u[i].raw_owner;before[i].owner=u[i].owner;
        before[i].hp=u[i].hp;before[i].shield=u[i].shield;
    }
}

static void controller_storm_damage_report(const char *label,int x,int y,
                                             const ControllerStormTarget *before)
{
    for(int i=0;i<MAX_UNITS;i++)if(before[i].valid) {
        int same=u[i].raw_type==before[i].raw_type&&u[i].raw_owner==before[i].raw_owner&&
                 u[i].owner==before[i].owner;
        int hp=same&&u[i].alive?u[i].hp:0;
        int shield=same&&u[i].alive?u[i].shield:0;
        fprintf(stderr,"Storm %s target slot%d raw%d owner%d rawowner%d center%d,%d hp%d->%d shield%d->%d identity%d alive%d\n",
            label,i,before[i].raw_type,before[i].owner,before[i].raw_owner,x,y,
            before[i].hp,hp,before[i].shield,shield,same,same&&u[i].alive);
    }
}

static int controller_wait_storm_expire(int effect)
{
    unsigned start=host_ticks;
    while(effect>=0&&effect<SC64_STORM_MAX_ACTIVE&&
          race_z6.storm.storm[effect].active&&host_ticks-start<120u&&
          screen==SCREEN_PLAY&&!won&&!lost)input(0);
    return effect>=0&&effect<SC64_STORM_MAX_ACTIVE&&
           !race_z6.storm.storm[effect].active;
}

static int controller_cast_storm_pair(int hero,int x,int y,int cluster_count)
{
    ControllerStormTarget before[MAX_UNITS];
    int effect=-1,cast_x,cast_y,second_x=0,second_y=0,second_count=0,overlap=0;
    unsigned cast_start=host_ticks;
    if(hero<0||!u[hero].alive||u[hero].carry<150*256||cluster_count<2||
       !currently_visible(x,y)||dist2(u[hero].x,u[hero].y,x,y)>144*144) return 0;
    if(!hero_storm(x,y)) {
        fprintf(stderr,"observed Storm1 command rejected target%d,%d latency%u energy%d\n",
            x,y,host_ticks-cast_start,u[hero].carry/256);
        return 0;
    }
    cast_x=cx;cast_y=cy;
    for(int i=0;i<SC64_STORM_MAX_ACTIVE;i++)
        if(race_z6.storm.storm[i].active&&race_z6.storm.storm[i].x==cast_x&&
           race_z6.storm.storm[i].y==cast_y) {effect=i;break;}
    fprintf(stderr,"observed Storm1 cast cluster%d requested%d,%d actual%d,%d latency%u energy%d effect%d\n",
        cluster_count,x,y,cast_x,cast_y,host_ticks-cast_start,u[hero].carry/256,effect);
    if(effect<0)return 1;
    controller_storm_snapshot(cast_x,cast_y,before);
    if(!controller_wait_storm_expire(effect)) {
        fprintf(stderr,"observed Storm1 did not expire within120 ticks\n");return 1;
    }
    controller_storm_damage_report("1",cast_x,cast_y,before);
    hero=kerrigan();
    second_count=best_storm_point(hero,&second_x,&second_y);
    if(hero>=0)for(int i=0;i<MAX_UNITS;i++)if(before[i].valid&&u[i].alive&&
        u[i].raw_type==before[i].raw_type&&u[i].raw_owner==before[i].raw_owner&&
        u[i].owner==before[i].owner&&currently_visible(u[i].x,u[i].y)&&
        dist2(second_x,second_y,u[i].x,u[i].y)<=24*24)overlap++;
    if(hero<0||!u[hero].alive||u[hero].carry<75*256||second_count<2||overlap<2||
       dist2(x,y,second_x,second_y)>48*48||
       dist2(u[hero].x,u[hero].y,second_x,second_y)>144*144||
       !currently_visible(second_x,second_y)) {
        fprintf(stderr,"observed Storm2 skipped: cluster%d overlap%d point%d,%d from%d,%d energy%d\n",
            second_count,overlap,second_x,second_y,x,y,hero>=0?u[hero].carry/256:-1);
        return 1;
    }
    cast_start=host_ticks;
    if(!hero_storm(second_x,second_y)) {
        fprintf(stderr,"observed Storm2 command rejected point%d,%d latency%u energy%d\n",
            second_x,second_y,host_ticks-cast_start,u[hero].carry/256);
        return 1;
    }
    int actual_second_x=cx,actual_second_y=cy;
    effect=-1;
    for(int i=0;i<SC64_STORM_MAX_ACTIVE;i++)
        if(race_z6.storm.storm[i].active&&race_z6.storm.storm[i].x==actual_second_x&&
           race_z6.storm.storm[i].y==actual_second_y) {effect=i;break;}
    fprintf(stderr,"observed Storm2 cast cluster%d overlap%d requested%d,%d actual%d,%d latency%u energy%d effect%d\n",
        second_count,overlap,second_x,second_y,actual_second_x,actual_second_y,host_ticks-cast_start,
        u[hero].carry/256,effect);
    if(effect>=0)controller_storm_snapshot(actual_second_x,actual_second_y,before);
    if(effect>=0&&controller_wait_storm_expire(effect))
        controller_storm_damage_report("2",actual_second_x,actual_second_y,before);
    else fprintf(stderr,"observed Storm2 effect did not expire within120 ticks\n");
    return 2;
}
static void launch(void)
{
    game_init(); prior=0;
    assert(screen==SCREEN_TITLE);
    tap(KEY_A);
    assert(race_front.page==SC_FRONT_MISSION && race_front.faction==SC_FRONT_ZERG);
    tap(KEY_RIGHT);tap(KEY_RIGHT);
    assert(race_front.mission_page==2);
    tap(KEY_DOWN);
    tap(KEY_A);
    fprintf(stderr,"briefing page=%u mission=%d row=%u ordinal=%d count=%d idord=%d\n",race_front.page,race_front.mission,race_front.row,sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z6_GLOBAL_ID),sc64_catalog_playable_count(SC64_FACTION_ZERG),sc64_catalog_playable_at(SC64_FACTION_ZERG,5));
    assert(race_front.page==SC_FRONT_BRIEFING && race_front.mission==SC64_Z6_GLOBAL_ID);
    tap(KEY_A);
    assert(screen==SCREEN_PLAY && race_selected_mission==SC64_Z6_GLOBAL_ID && race_z6_active());
}

static void trace_array_u8(const u8 *values,unsigned count)
{
    fputc('[',controller_trace);
    for(unsigned i=0;i<count;i++)fprintf(controller_trace,"%s%u",i?",":"",values[i]);
    fputc(']',controller_trace);
}
static void trace_array_u16(const u16 *values,unsigned count)
{
    fputc('[',controller_trace);
    for(unsigned i=0;i<count;i++)fprintf(controller_trace,"%s%u",i?",":"",values[i]);
    fputc(']',controller_trace);
}
static void trace_model_event(const char *kind)
{
    int k=kerrigan();
    fprintf(controller_trace,
        "%s{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,\"screen\":%d,"
        "\"won\":%d,\"lost\":%d,\"outcome\":%u,\"started\":%u,\"fired\":%u,"
        "\"action_cursors\":",trace_event_started?",":"",kind,host_ticks,(unsigned)frame,
        screen,won,lost,race_z6.runtime.model.outcome,race_z6.runtime.model.started,
        race_z6.runtime.model.started);
    trace_event_started=1;
    trace_array_u8(race_z6.runtime.model.action_cursor,SC64_Z6_TRIGGER_COUNT);
    fputs(",\"action_ticks\":",controller_trace);
    trace_array_u16(race_z6.runtime.action_ticks,SC64_Z6_TRIGGER_COUNT);
    fputs(",\"switch_bits\":[",controller_trace);
    for(unsigned i=0;i<8;i++)fprintf(controller_trace,"%s%u",i?",":"",race_z6.runtime.switch_bits[i]);
    fprintf(controller_trace,"],\"fault\":%u,\"kerrigan_alive\":%d",
        race_z6.runtime.fault,k>=0&&u[k].owner==SIDE);
    if(strcmp(kind,"start")!=0) {
        u32 loc4=0,loc5=0;
        (void)sc64_z6_world_count(&race_z6_world,19,229,4,&loc4);
        (void)sc64_z6_world_count(&race_z6_world,19,229,5,&loc5);
        fprintf(controller_trace,
            ",\"kerrigan\":{\"alive\":%d,\"raw_type\":%d,\"raw_owner\":%d,\"owner\":%d,\"x\":%d,\"y\":%d,\"order\":%d,\"target\":%d,\"carrier_target\":%d},"
            "\"transport\":{\"carrier_slot\":%d,\"cargo_space\":%d,\"researched\":%u,\"ticks_remaining\":%u},"
            "\"force2_loc4\":%u,\"force2_loc5\":%u",
            k>=0&&u[k].owner==SIDE,k>=0?u[k].raw_type:-1,k>=0?u[k].raw_owner:-1,
            k>=0?u[k].owner:-1,k>=0?u[k].x:-1,k>=0?u[k].y:-1,
            k>=0?u[k].order:-1,k>=0?u[k].target:-1,k>=0?u[k].target:-1,trace_carrier_slot,
            trace_carrier_slot>=0?race_transport_cargo_space(trace_carrier_slot):-1,
            race_z6.transport.researched,race_z6.transport.ticks_remaining,loc4,loc5);
    }
    fputc('}',controller_trace);
}

int main(int argc,char **argv)
{
    const char *trace_path=".local/sc64/z6-controller-attempt.json";
    if(argc==3&&strcmp(argv[1],"--trace")==0&&argv[2][0])trace_path=argv[2];
    else if(argc!=1) {
        fprintf(stderr,"usage: %s [--trace PATH]\n",argv[0]);
        return 2;
    }
    controller_trace=fopen(trace_path,"w");
    if(!controller_trace) { perror("open Z6 controller trace"); return 2; }
    fputs("{\"schema\":1,\"profile\":\"zerg-z6\",\"mission\":17,"
          "\"policy\":\"normal-input bounded Z6 mainland clearance and Ventral Sacs ferry\",\"events\":[",
          controller_trace);
    launch();
    trace_model_event("start");
    int result=z6_transport_campaign();
    const char *terminal=(screen==SCREEN_RESULT&&won&&race_z6.runtime.model.outcome==SC64_Z6_WON)?
        "won":(screen==SCREEN_RESULT&&lost)?"lost":"limit";
    trace_model_event(terminal);
    fputs("]}\n",controller_trace);
    if(fclose(controller_trace)!=0) { perror("write Z6 controller trace"); return 2; }
    controller_trace=NULL;
    /* Helpers historically returned zero for both early aborts and the
     * completed route. Only an observed source victory may pass this target. */
    return result == 0 && strcmp(terminal,"won") == 0 ? 0 : 1;
}
