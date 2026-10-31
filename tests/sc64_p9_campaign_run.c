/* Ordinary-input P9 "Shadow Hunters" campaign controller.
 * The controller may inspect live state to choose targets; every mutation is
 * an ordinary HOME/menu/gameplay input delivered through game_update(). */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef P9_CONTROLLER_PROFILE
#define P9_CONTROLLER_PROFILE "PROTOSS_P9_FLAGS"
#endif
#ifndef P9_RUN_LIMIT
#define P9_RUN_LIMIT 340000u
#endif
/* One ordinary pause-menu save; the replay tools resume from this record. */
#ifndef P9_SAVE_FRAME
#define P9_SAVE_FRAME 30000
#endif
#define KEY_TAP(k) do { input((k)); input(0); } while (0)

static FILE *trace;
static u16 prior;
static unsigned host_tick;

static void input(u16 held)
{
    /* Once the mission ends, later calls are not inputs: the trace stops at
     * the deciding call, so a replay ends there too. */
    if(screen==SCREEN_RESULT||won||lost)return;
    u16 pressed=(u16)(held & (u16)~prior);
    int changed=held!=prior;
    unsigned call_index=host_tick;
    unsigned frame_before=(unsigned)frame;
    game_update(held,pressed);
    prior=held;
    ++host_tick;
    if(trace && changed)
        fprintf(trace,"{\"kind\":\"input\",\"call_index\":%u,\"frame_before\":%u,\"frame_after\":%u,\"held\":%u,\"pressed\":%u}\n",
            call_index,frame_before,(unsigned)frame,(unsigned)held,(unsigned)pressed);
}

/* Read-only route query over the engine's current walkability plane. */
static int waypoint_to(int sx,int sy,int tx,int ty,int *wx,int *wy,int *distance_out)
{
    enum { WAYPOINT_STEPS = 10 };
    static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    static int prev[MAP_CELLS],queue[MAP_CELLS],path[MAP_CELLS];
    if(!path_grid_valid)return 0;
    sx=limit(sx/16,0,MAP_COLS-1);sy=limit(sy/16,0,MAP_ROWS-1);
    int start=sx+sy*MAP_COLS,gx=limit(tx/16,0,MAP_COLS-1),gy=limit(ty/16,0,MAP_ROWS-1);
    int head=0,tail=0,best=-1;
    for(int i=0;i<MAP_COLS*MAP_ROWS;i++)prev[i]=-2;
    prev[start]=-1;queue[tail++]=start;
    while(head<tail){
        int cell=queue[head++],col=cell%MAP_COLS,row=cell/MAP_COLS;
        if(FAST_ABS(col-gx)<=4&&FAST_ABS(row-gy)<=4){best=cell;break;}
        for(int k=0;k<4;k++){
            int nc=col+dx[k],nr=row+dy[k];
            if(nc<0||nr<0||nc>=MAP_COLS||nr>=MAP_ROWS)continue;
            int next=nc+nr*MAP_COLS;
            if(prev[next]!=-2||(!SC64_GRID_GET(path_walkable,next)&&next!=start))continue;
            prev[next]=cell;queue[tail++]=next;
        }
    }
    if(best<0)return 0;
    int len=0;
    for(int c=best;c>=0&&len<MAP_CELLS;c=prev[c])path[len++]=c;
    int distance=len-1;
    if(distance_out)*distance_out=distance;
    int pick=distance>WAYPOINT_STEPS?distance-WAYPOINT_STEPS:0;
    *wx=(path[pick]%MAP_COLS)*16+8;*wy=(path[pick]/MAP_COLS)*16+8;
    return 1;
}

static int hero(int raw)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==raw)return i;
    return -1;
}

/* The next Cerebrate to hunt: Baelrog (slot 6, location 1) first. */
static int cerebrate(int *location)
{
    static const int slot[2]={6,5},loc[2]={1,2};
    for(int k=0;k<2;k++){
        if(race_p9.switches&(1<<k))continue;
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==151&&b[i].raw_owner==slot[k]){
            *location=loc[k];return i;
        }
    }
    return -1;
}

static int under_construction(void)
{
    int n=0;for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&b[i].under_construction;
    return n;
}

static int home_nexus(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_NEXUS)return i;
    return -1;
}

static int army_cursor_watch, army_cursor_emergency;

static void move_cursor(int x,int y)
{
    unsigned guard=0;
    x=limit(x,0,WORLD_W-1);y=limit(y,0,WORLD_H-1);
    x=cx+((x-cx)>=0?((x-cx+2)/4):((x-cx-2)/4))*4;
    y=cy+((y-cy)>=0?((y-cy+2)/4):((y-cy-2)/4))*4;
    while((cx!=x||cy!=y)&&screen==SCREEN_PLAY&&!won&&!lost&&guard++<12000u){
        u16 held=KEY_R;
        if(cx<x)held|=KEY_RIGHT;else if(cx>x)held|=KEY_LEFT;
        if(cy<y)held|=KEY_DOWN;else if(cy>y)held|=KEY_UP;
        input(held);
        if(army_cursor_watch)for(int i=0;i<MAX_UNITS;i++)
            if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==75&&u[i].shield<380&&
               SC64_P7_ARBITER_CACHE_VALID(u[i].weapon)&&
               (SC64_P7_ARBITER_DETECTED_MASK(u[i].weapon)&~(1u<<7))) {
                army_cursor_emergency=1;break;
            }
        if(army_cursor_emergency)break;
    }
    input(0);
    if(!army_cursor_emergency&&(cx!=x||cy!=y))
        fprintf(stderr,"cursor guard frame=%d wanted=%d,%d got=%d,%d\n",frame,x,y,cx,cy);
}

static void clear_selected_building(void)
{
    if(selected_building<0)return;
    move_cursor(900,700);input(KEY_L);
    unsigned guard=0;
    while((cx<916||cy<716)&&guard++<64){
        u16 held=KEY_L;
        if(cx<916)held|=KEY_RIGHT;
        if(cy<716)held|=KEY_DOWN;
        input(held);
    }
    input(0);
}

static void close_menus(void)
{
    if(menu_open)KEY_TAP(KEY_B);
    if(mode!=MODE_NORMAL)KEY_TAP(KEY_B);
}

static int command_selected(int which,int x,int y)
{
    KEY_TAP(KEY_B);
    for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=which;n++)
        KEY_TAP(KEY_DOWN);
    if(!menu_open||menu_cursor!=which){
        fprintf(stderr,"command row frame=%d want=%d cursor=%d rows=%d sel=%d\n",
                frame,which,menu_cursor,field_menu_count(),selected_count());
        close_menus();return 0;
    }
    KEY_TAP(KEY_A);
    if(mode!=(which==MENU_ATTACK?MODE_ATTACK:MODE_MOVE)){close_menus();return 0;}
    move_cursor(x,y);
    if(army_cursor_emergency){close_menus();return 0;}
    KEY_TAP(KEY_A);
    return 1;
}

static int hero_command(int slot,int x,int y);

/* Distance from (x,y) to the nearest armed enemy (Overlords are harmless
 * detectors), capped at 1024 pixels. */
static int enemy_clearance(int x,int y)
{
    int best=1024*1024;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type!=42){
        int d=dist2(u[i].x,u[i].y,x,y);
        if(d<best)best=d;
    }
    int r=0;
    while((r+1)*(r+1)<=best)++r;
    return r;
}

/* The nearest live Sunken Colony guarding a Cerebrate, or -1. */
static int guarding_sunken(int c)
{
    int best=-1,bd=200*200;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&b[i].raw_type==146){
        int d=dist2(b[i].x,b[i].y,b[c].x,b[c].y);
        if(d<bd){bd=d;best=i;}
    }
    return best;
}
static int fenix_home_x=-1,fenix_home_y=-1;
static int zeratul_entering;

/* Order the army by type groups: a double tap on a Zealot or Dragoon
 * selects that type on screen, which never includes Fenix or Zeratul (R's
 * whole-army shortcut would). Repeat for off-screen clusters. */
/* Anchors farther than this from the order point are left alone (0: no
 * limit), so frequent assault orders do not walk the cursor home. */
static int army_anchor_limit;

static int army_command(int which,int x,int y)
{
    static u8 ordered[MAX_UNITS];
    int groups=0;
    /* Units already carrying this order, or already fighting during an
     * attack, need no new one; cursor travel is the controller's main cost. */
    for(int i=0;i<MAX_UNITS;i++){
        int same=u[i].order==(which==MENU_ATTACK?ORDER_ATTACK_MOVE:ORDER_MOVE)&&
            dist2(u[i].tx,u[i].ty,x,y)<96*96;
        ordered[i]=same||(which==MENU_ATTACK&&u[i].order==ORDER_ATTACK_TARGET)||
            (army_anchor_limit&&dist2(u[i].x,u[i].y,x,y)>army_anchor_limit*army_anchor_limit);
    }
    for(int round=0;round<6;round++){
        int anchor=-1,bd=0x7fffffff;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!ordered[i]&&
            (u[i].type==U_ZEALOT||u[i].type==U_DRAGOON)){
            int d=dist2(u[i].x,u[i].y,x,y);
            if(d<bd){bd=d;anchor=i;}
        }
        if(anchor<0)break;
        close_menus();
        clear_selected_building();
        move_cursor(u[anchor].x,u[anchor].y);
        if(army_cursor_emergency)break;
        KEY_TAP(KEY_A);KEY_TAP(KEY_A);
        int bad=0,count=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected){
            ++count;
            if(u[i].type!=U_ZEALOT&&u[i].type!=U_DRAGOON)bad=1;
        }
        /* Whatever happened, do not retry this anchor. */
        ordered[anchor]=1;
        if(getenv("P9_ALOG"))fprintf(stderr,"group f=%d anchor=%d type=%d at=%d,%d count=%d bad=%d cur=%d,%d\n",
            frame,anchor,u[anchor].type,u[anchor].x,u[anchor].y,count,bad,cx,cy);
        if(!count||bad)continue;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected)ordered[i]=1;
        if(command_selected(which,x,y))++groups;
        if(army_cursor_emergency)break;
    }
    return groups>0;
}

/* Tap one hero to select it alone, then give it one command. */
/* Tap one actor to select it alone. A tap can pick an overlapping unit; an
 * L tap cycles the single selection through the side's units. */
static int hero_select(int slot)
{
    if(slot<0||!u[slot].alive||u[slot].owner!=SIDE)return 0;
    close_menus();
    clear_selected_building();
    move_cursor(u[slot].x,u[slot].y);KEY_TAP(KEY_A);
    for(int n=0;n<40&&(!u[slot].selected||selected_count()!=1);n++)KEY_TAP(KEY_L);
    return u[slot].selected&&selected_count()==1;
}

static int hero_order(int slot,int which,int x,int y)
{
    if(!hero_select(slot))return 0;
    return command_selected(which,x,y);
}

static int hero_command(int slot,int x,int y)
{ return hero_order(slot,MENU_MOVE,x,y); }

static int live_units_owner(int owner)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==owner;return n;
}

static int probe_count(void)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE;return n;
}

static int army_count(void)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&!IS_WORKER(u[i].type)&&
        u[i].raw_type!=75&&u[i].raw_type!=78&&u[i].raw_type!=71;
    return n;
}

static int count_type(int type,int complete_only)
{
    int n=0;for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&b[i].type==type&&
        (!complete_only||building_complete(i));
    return n;
}

static int queue_probe(void)
{
    int home=home_nexus();
    if(home<0||!building_complete(home)||probe_count()+b[home].queue_count>=14||
       b[home].queue_count>=2||minerals<50||race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<2)return 0;
    close_menus();
    if(selected_building!=home){clear_selected_building();move_cursor(b[home].x,b[home].y);KEY_TAP(KEY_A);}
    if(selected_building!=home)return 0;
    int before=b[home].queue_count;
    KEY_TAP(KEY_A);
    if(b[home].queue_count<=before)return 0;
    fprintf(trace,"{\"kind\":\"production\",\"frame\":%u,\"unit\":\"probe\",\"bank_after\":%d}\n",(unsigned)frame,minerals);
    return 1;
}

static int queue_combat_unit(void)
{
    int wanted=count_type(B_CYBERNETICS_CORE,1)&&gas>=50&&minerals>=125?U_DRAGOON:U_ZEALOT;
    const Sc64RaceUnitRule *rule=sc64_race_unit_rule(wanted);
    if(!rule||minerals<(int)rule->minerals||gas<(int)rule->gas||
       race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<(int)rule->supply)return 0;
    int producer=-1,best_queue=4;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_GATEWAY&&building_complete(i)&&b[i].queue_count<best_queue){
        producer=i;best_queue=b[i].queue_count;
    }
    if(producer<0)return 0;
    close_menus();
    move_cursor(b[producer].x,b[producer].y);KEY_TAP(KEY_A);
    if(selected_building!=producer)return 0;
    for(int n=0;n<4&&train_type!=wanted;n++)KEY_TAP(KEY_R);
    if(train_type!=wanted)return 0;
    int before=b[producer].queue_count;
    KEY_TAP(KEY_A);
    if(b[producer].queue_count<=before)return 0;
    fprintf(trace,"{\"kind\":\"production\",\"frame\":%u,\"unit\":\"%s\",\"bank_after\":[%d,%d]}\n",
        (unsigned)frame,wanted==U_DRAGOON?"dragoon":"zealot",minerals,gas);
    return 1;
}

/* A builder Probe that a single tap can select: the free Probe farthest
 * from any other unit of the side. */
static int free_builder(void)
{
    int best=-1,best_gap=-1;
    for(int pass=0;pass<2&&best<0;pass++)
        for(int i=0;i<MAX_UNITS;i++){
            if(!u[i].alive||u[i].owner!=SIDE||u[i].type!=U_PROBE||
               SC64_BUILD_WORKER_ORDER(u[i].order)||u[i].order==ORDER_GAS)continue;
            if(!pass&&(u[i].order!=ORDER_MINE||u[i].carry))continue;
            int gap=0x7fffffff;
            for(int j=0;j<MAX_UNITS;j++)if(j!=i&&u[j].alive&&u[j].owner==SIDE){
                int d=dist2(u[i].x,u[i].y,u[j].x,u[j].y);
                if(d<gap)gap=d;
            }
            if(gap>best_gap){best_gap=gap;best=i;}
        }
    return best;
}

/* Keep a 16-pixel walkable lane between a new structure's pinned BWAPI tile
 * footprint and every structure and mineral patch, and keep the base's
 * north ramp clear, so production exits never seal off. */
/* Sites the game refused once are not tried again. */
static int refused_x[64],refused_y[64],refused_count;

static int site_keeps_lanes(int type,int x,int y)
{
    for(int k=0;k<refused_count;k++)
        if(FAST_ABS(refused_x[k]-x)<16&&FAST_ABS(refused_y[k]-y)<16)return 0;
    int raw=sc64_race_raw_building(type);
    if(raw<0||raw>=228)return 0;
    int hw=siege_bounds[raw][0]*8,hh=siege_bounds[raw][1]*8;
    if(x>=1360&&x<=1630&&y>=1600&&y<=1776)return 0;
    for(int k=0;k<MAX_BUILDINGS;k++)if(b[k].alive&&b[k].owner==SIDE&&b[k].raw_type>=0&&b[k].raw_type<228){
        int kw=siege_bounds[b[k].raw_type][0]*8,kh=siege_bounds[b[k].raw_type][1]*8;
        if(FAST_ABS(b[k].x-x)<hw+kw+16&&FAST_ABS(b[k].y-y)<hh+kh+16)return 0;
    }
    for(int k=0;k<MAX_MINERALS;k++)if(m[k].amount>0&&
        FAST_ABS(m[k].x-x)<hw+16+16&&FAST_ABS(m[k].y-y)<hh+8+16)return 0;
    return 1;
}

/* Place `type` through the Probe build menu at the nearest legal site. */
static int build_structure(int type,int menu_row,int min_r,int max_r,int fixed_x,int fixed_y)
{
    int home=home_nexus(),best=0x7fffffff,site_x=fixed_x,site_y=fixed_y;
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
    if(home<0||!rule||minerals<(int)rule->minerals||gas<(int)rule->gas)return 0;
    if(site_x<0){
        /* A coarse lattice keeps walkable lanes between structures, so the
         * base cannot wall its own production in. */
        int step=16;
        for(int dy=-max_r;dy<=max_r;dy+=step)for(int dx=-max_r;dx<=max_r;dx+=step){
            int x=b[home].x+dx,y=b[home].y+dy,d=dx*dx+dy*dy;
            if(d<min_r*min_r||x<32||x>=WORLD_W-32||y<32||y>=WORLD_H-32||d>=best||
               !race_can_site(SIDE,type,x,y)||!can_place(type,x,y))continue;
            if(!site_keeps_lanes(type,x,y))continue;
            /* Units standing on a site make the placement fail. */
            int occupied=0;
            for(int i=0;i<MAX_UNITS&&!occupied;i++)occupied=u[i].alive&&dist2(u[i].x,u[i].y,x,y)<32*32;
            if(occupied)continue;
            best=d;site_x=x;site_y=y;
        }
    }
    if(site_x<0){
        if(getenv("P9_BLOG"))fprintf(stderr,"no site f=%d type=%d home=%d,%d bank=%d\n",frame,type,b[home].x,b[home].y,minerals);
        return 0;
    }
    int worker=free_builder();
    if(worker<0){
        if(getenv("P9_BLOG"))fprintf(stderr,"no worker f=%d type=%d\n",frame,type);
        return 0;
    }
    close_menus();
    clear_selected_building();
    move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    for(int n=0;n<40&&(!u[worker].selected||selected_count()!=1);n++)KEY_TAP(KEY_L);
    if(!u[worker].selected||selected_count()!=1){
        if(getenv("P9_BLOG"))fprintf(stderr,"builder tap fail f=%d type=%d sel=%d count=%d\n",
            frame,type,u[worker].selected,selected_count());
        return 0;
    }
    KEY_TAP(KEY_B);
    for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=menu_row;n++)KEY_TAP(KEY_DOWN);
    if(!menu_open||menu_cursor!=menu_row){
        if(getenv("P9_BLOG"))fprintf(stderr,"build row fail f=%d type=%d cursor=%d rows=%d open=%d\n",
            frame,type,menu_cursor,field_menu_count(),menu_open);
        close_menus();return 0;
    }
    KEY_TAP(KEY_A);
    if(mode!=MODE_BUILD||menu!=type){
        fprintf(stderr,"build menu frame=%d type=%d mode=%d menu=%d\n",frame,type,mode,menu);
        close_menus();return 0;
    }
    move_cursor(site_x,site_y);KEY_TAP(KEY_A);
    if(mode!=MODE_NORMAL)close_menus();
    if(!SC64_BUILD_WORKER_ORDER(u[worker].order)){
        fprintf(stderr,"build refused frame=%d type=%d site=%d,%d order=%d\n",frame,type,site_x,site_y,u[worker].order);
        if(refused_count<64){refused_x[refused_count]=site_x;refused_y[refused_count++]=site_y;}
        return 0;
    }
    fprintf(trace,"{\"kind\":\"build\",\"frame\":%u,\"type\":%d,\"site\":[%d,%d]}\n",(unsigned)frame,type,site_x,site_y);
    return 1;
}

static int build_assimilator(void)
{
    int home=home_nexus(),best=-1,bd=0x7fffffff;
    if(home<0||count_type(B_ASSIMILATOR,0))return 0;
    for(int g=0;g<geyser_count;g++){
        int d=dist2(geysers[g].x,geysers[g].y,b[home].x,b[home].y);
        if(d<bd){bd=d;best=g;}
    }
    if(best<0||bd>320*320)return 0;
    return build_structure(B_ASSIMILATOR,MENU_REFINERY,0,0,geysers[best].x,geysers[best].y);
}

/* A selected Probe exposes NEXUS as the row after STOP. */
static int build_nexus(int near_x,int near_y)
{
    int best=0x7fffffff,site_x=-1,site_y=-1,worker=free_builder();
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(B_NEXUS);
    if(!rule||minerals<(int)rule->minerals||worker<0)return 0;
    for(int dy=-160;dy<=160;dy+=8)for(int dx=-160;dx<=160;dx+=8){
        int x=near_x+dx,y=near_y+dy,d=dx*dx+dy*dy,mineral=0x7fffffff;
        if(x<48||x>=WORLD_W-48||y<48||y>=WORLD_H-48||
           !race_can_site(SIDE,B_NEXUS,x,y)||!can_place(B_NEXUS,x,y))continue;
        for(int k=0;k<MAX_MINERALS;k++)if(m[k].amount>0){
            int md=dist2(x,y,m[k].x,m[k].y);if(md<mineral)mineral=md;
        }
        if(mineral<56*56)continue;
        d+=mineral/4;
        if(d<best){best=d;site_x=x;site_y=y;}
    }
    if(site_x<0){if(getenv("P9_BLOG"))fprintf(stderr,"nexus no site f=%d\n",frame);return 0;}
    close_menus();clear_selected_building();
    move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    for(int n=0;n<40&&(!u[worker].selected||selected_count()!=1);n++)KEY_TAP(KEY_L);
    if(!u[worker].selected||selected_count()!=1){if(getenv("P9_BLOG"))fprintf(stderr,"nexus select fail f=%d\n",frame);return 0;}
    KEY_TAP(KEY_B);
    for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=MENU_STOP+1;n++)KEY_TAP(KEY_DOWN);
    if(!menu_open||menu_cursor!=MENU_STOP+1){close_menus();return 0;}
    KEY_TAP(KEY_A);
    if(mode!=MODE_BUILD||menu!=B_NEXUS){close_menus();return 0;}
    move_cursor(site_x,site_y);KEY_TAP(KEY_A);
    if(mode!=MODE_NORMAL)close_menus();
    if(!SC64_BUILD_WORKER_ORDER(u[worker].order))return 0;
    fprintf(trace,"{\"kind\":\"build\",\"frame\":%u,\"type\":%d,\"site\":[%d,%d]}\n",(unsigned)frame,B_NEXUS,site_x,site_y);
    return 1;
}

/* Photon Cannons around Fenix's shelter: the Probe's CANNON row follows
 * NEXUS (STOP + 2). */
static int build_cannon(int near_x,int near_y)
{
    int best=0x7fffffff,site_x=-1,site_y=-1,worker=free_builder();
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(B_PHOTON_CANNON);
    if(!rule||minerals<(int)rule->minerals||worker<0)return 0;
    for(int dy=-160;dy<=160;dy+=8)for(int dx=-160;dx<=160;dx+=8){
        int x=near_x+dx,y=near_y+dy,d=dx*dx+dy*dy;
        if(d<24*24||d>=best||x<48||x>=WORLD_W-48||y<48||y>=WORLD_H-48||
           !race_can_site(SIDE,B_PHOTON_CANNON,x,y)||!can_place(B_PHOTON_CANNON,x,y)||
           !site_keeps_lanes(B_PHOTON_CANNON,x,y))continue;
        best=d;site_x=x;site_y=y;
    }
    if(site_x<0){if(getenv("P9_BLOG"))fprintf(stderr,"cannon no site f=%d near=%d,%d\n",frame,near_x,near_y);return 0;}
    close_menus();clear_selected_building();
    move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    for(int n=0;n<40&&(!u[worker].selected||selected_count()!=1);n++)KEY_TAP(KEY_L);
    if(!u[worker].selected||selected_count()!=1)return 0;
    KEY_TAP(KEY_B);
    for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=MENU_STOP+2;n++)KEY_TAP(KEY_DOWN);
    if(!menu_open||menu_cursor!=MENU_STOP+2){
        if(getenv("P9_BLOG"))fprintf(stderr,"cannon row f=%d open=%d cursor=%d rows=%d\n",frame,menu_open,menu_cursor,field_menu_count());
        close_menus();return 0;
    }
    KEY_TAP(KEY_A);
    if(mode!=MODE_BUILD||menu!=B_PHOTON_CANNON){close_menus();return 0;}
    move_cursor(site_x,site_y);KEY_TAP(KEY_A);
    if(mode!=MODE_NORMAL)close_menus();
    if(!SC64_BUILD_WORKER_ORDER(u[worker].order)){
        if(getenv("P9_BLOG"))fprintf(stderr,"cannon refused f=%d site=%d,%d\n",frame,site_x,site_y);
        if(refused_count<64){refused_x[refused_count]=site_x;refused_y[refused_count++]=site_y;}
        return 0;
    }
    fprintf(trace,"{\"kind\":\"build\",\"frame\":%u,\"type\":%d,\"site\":[%d,%d]}\n",(unsigned)frame,B_PHOTON_CANNON,site_x,site_y);
    return 1;
}

/* Frame at which each Probe's pending build order was first seen. */
static int pending_since[MAX_UNITS];

/* Return one idle Probe to the nearest mineral patch near the Nexus. A
 * pending build stuck for 900 frames (its site occupied) is abandoned. */
static int remine_idle_probe(int home)
{
    static const int dx[] = {0,8,-8,0,0,8,-8,8,-8};
    static const int dy[] = {0,0,0,8,-8,8,8,-8,-8};
    int worker=-1;
    unsigned char tried[MAX_MINERALS] = {0};
    for(int i=0;i<MAX_UNITS;i++){
        if(!(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&SC64_PENDING_BUILD_ORDER(u[i].order))){
            pending_since[i]=-1;continue;
        }
        if(pending_since[i]<0)pending_since[i]=frame;
        if(worker<0&&frame-pending_since[i]>900){
            worker=i;
            if(refused_count<64){refused_x[refused_count]=u[i].tx;refused_y[refused_count++]=u[i].ty;}
        }
    }
    for(int i=0;i<MAX_UNITS&&worker<0;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE){
        if(u[i].order==ORDER_IDLE && !u[i].carry){worker=i;break;}
        if(u[i].order==ORDER_MINE&&!u[i].carry&&u[i].target>=0&&u[i].target<MAX_MINERALS){
            int near=0;
            for(int n=0;n<MAX_BUILDINGS;n++)if(b[n].alive&&b[n].owner==SIDE&&b[n].type==B_NEXUS&&
                dist2(b[n].x,b[n].y,m[u[i].target].x,m[u[i].target].y)<320*320)near=1;
            if(!near)worker=i;
        }
    }
    if(worker<0||home<0)return 0;
    close_menus();clear_selected_building();
    move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    for(int n=0;n<40&&(!u[worker].selected||selected_count()!=1);n++)KEY_TAP(KEY_L);
    if(!u[worker].selected||selected_count()!=1)return 0;
    /* Try nearby visible fields in distance order, including hit-box offsets:
     * another unit can intercept selection at a mineral field's center. */
    for(int attempt=0;attempt<MAX_MINERALS;attempt++) {
        int patch=-1,best=0x7fffffff;
        for(int n=0;n<MAX_BUILDINGS;n++)if(b[n].alive&&b[n].owner==SIDE&&b[n].type==B_NEXUS&&
            building_complete(n))for(int k=0;k<MAX_MINERALS;k++)if(!tried[k]&&m[k].amount>0&&currently_visible(m[k].x,m[k].y)) {
            int d=dist2(m[k].x,m[k].y,b[n].x,b[n].y);
            if(d<best){best=d;patch=k;}
        }
        if(patch<0)break;
        tried[patch]=1;
        for(unsigned k=0;k<sizeof(dx)/sizeof(dx[0]);k++) {
            int x=m[patch].x+dx[k],y=m[patch].y+dy[k];
            move_cursor(x,y);
            if(!u[worker].alive||!u[worker].selected||selected_count()!=1)return 0;
            if(absi(cx-x)>2||absi(cy-y)>2||!currently_visible(m[patch].x,m[patch].y)||
               field_ui_a_action(selected_count())!=UI_A_MINE||selected_worker_mineral_target()!=patch)continue;
            KEY_TAP(KEY_A);
            if(u[worker].order==ORDER_MINE&&u[worker].target==patch)return 1;
        }
    }
    return 0;
}

static int assign_gas_worker(void)
{
    int refinery=-1,gas_workers=0,worker=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_ASSIMILATOR&&building_complete(i)){refinery=i;break;}
    if(refinery<0)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE){
        if(u[i].order==ORDER_GAS)++gas_workers;
        else if(worker<0&&u[i].order==ORDER_MINE&&!u[i].carry)worker=i;
    }
    if(gas_workers>=3||worker<0)return 0;
    close_menus();
    clear_selected_building();move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    if(!u[worker].selected)return 0;
    move_cursor(b[refinery].x,b[refinery].y);KEY_TAP(KEY_A);
    return u[worker].order==ORDER_GAS;
}

/* Shared-service evidence: most enemy source slots detecting Zeratul at once,
 * and most human units under the human Arbiter's aura at once. */
static int detect_peak,aura_peak;
static void service_sample(void)
{
    int z=hero(75),aura=0;
    if(z>=0&&SC64_P7_ARBITER_CACHE_VALID(u[z].weapon)){
        unsigned m=SC64_P7_ARBITER_DETECTED_MASK(u[z].weapon)&~(1u<<7),n=0;
        for(;m;m&=m-1)++n;
        if((int)n>detect_peak)detect_peak=(int)n;
    }
    for(int i=0;i<MAX_UNITS;i++)aura+=u[i].alive&&u[i].owner==SIDE&&
        SC64_P7_ARBITER_CACHE_VALID(u[i].weapon)&&(u[i].weapon&SC64_P7_ARBITER_AURA);
    if(aura>aura_peak)aura_peak=aura;
}

static void census(const char *label)
{
    int z=hero(75),f=hero(78),loc=0,c=cerebrate(&loc);
    int enemy=0,enemy_b=0;
    for(int i=0;i<MAX_UNITS;i++)enemy+=u[i].alive&&u[i].owner==ENEMY;
    for(int i=0;i<MAX_BUILDINGS;i++)enemy_b+=b[i].alive&&b[i].owner==ENEMY;
    fprintf(trace,"{\"kind\":\"checkpoint\",\"label\":\"%s\",\"frame\":%u,\"call_index\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,"
        "\"side_units\":%d,\"army\":%d,\"probes\":%d,\"enemy_units\":%d,\"enemy_buildings\":%d,"
        "\"cerebrate\":[%d,%d],\"zeratul\":[%d,%d,%d,%d,%d],\"fenix\":[%d,%d,%d],\"bank\":[%d,%d],\"supply\":[%d,%d],"
        "\"nexus\":%d,\"gateways\":%d,\"switches\":%u,\"done\":[%u,%u,%u,%u],\"outcome\":%u,"
        "\"towns\":[%u,%u,%u],\"waves\":%u,"
        "\"zeratul_cloaked\":%d,\"zeratul_detected_by\":%u,\"detect_peak\":%d,\"aura_peak\":%d}\n",
        label,(unsigned)frame,host_tick,screen,won,lost,
        live_units_owner(SIDE),army_count(),probe_count(),enemy,enemy_b,
        loc,c>=0?b[c].hp:-1,z>=0?u[z].owner:-1,z>=0?u[z].hp:-1,z>=0?u[z].shield:-1,z>=0?u[z].x:-1,z>=0?u[z].y:-1,
        f>=0?u[f].owner:-1,f>=0?u[f].hp:-1,f>=0?u[f].shield:-1,minerals,gas,
        race_owner_supply(SIDE),race_owner_supply_cap(SIDE),count_type(B_NEXUS,0),count_type(B_GATEWAY,0),
        race_p9.switches,race_p9.runner[0].done,race_p9.runner[1].done,race_p9.runner[2].done,race_p9.runner[3].done,
        race_p9.runner[0].outcome,race_p9.towns[0].script,race_p9.towns[1].script,race_p9.towns[2].script,
        race_p9.towns[0].wave,
        z>=0?sc64_p7_observer_target_is_cloaked(z):-1,
        z>=0&&SC64_P7_ARBITER_CACHE_VALID(u[z].weapon)?SC64_P7_ARBITER_DETECTED_MASK(u[z].weapon):0u,
        detect_peak,aura_peak);
    fflush(trace);
}

int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:".local/traces/p9-controller.jsonl";
    unsigned limit_ticks=argc>2?(unsigned)strtoul(argv[2],0,10):P9_RUN_LIMIT;
    trace=fopen(path,"w");if(!trace){perror(path);return 2;}
    fprintf(trace,"{\"kind\":\"metadata\",\"schema\":1,\"mission\":28,\"profile\":\"%s\",\"input_policy\":\"ordinary game_update held/pressed only\"}\n",P9_CONTROLLER_PROFILE);
    game_init();prior=0;
    assert(screen==SCREEN_TITLE&&race_front.page==SC_FRONT_HOME);
    /* Campaign, fifth page of the two-entry mission list, first row, briefing. */
    KEY_TAP(KEY_A);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);
    KEY_TAP(KEY_A);KEY_TAP(KEY_START);
    assert(screen==SCREEN_PLAY&&!won&&!lost&&race_selected_mission==28);
    census("launch");
    for(int i=0;i<MAX_UNITS;i++)pending_since[i]=-1;
    int start_x=0,start_y=0,n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE){start_x+=u[i].x;start_y+=u[i].y;++n;}
    if(n){start_x/=n;start_y/=n;}

    int saved=0;
    int next_checkpoint=2000;
    int next_mine=30;
    int next_probe=60,next_supply=400,next_build=200,next_gas=3000,next_production=900;
    int next_order=60,next_hero=600;
    int attacking=0,front_distance=-1,next_zeratul=0,front_unit=-1,next_expand=8000,next_stage=3000;
    int next_save_build=0,escort_home=0,next_escort=0,cover_until=0;
    unsigned regrouped_switches=0;
    int saving_for_nexus=0,regroup_until=0,striking=0;
    while(host_tick<limit_ticks&&screen==SCREEN_PLAY&&!won&&!lost){
        static int stuck_reported;
        if(menu_open&&!stuck_reported){
            stuck_reported=1;
            fprintf(stderr,"menu left open frame=%d mode=%d cursor=%d\n",frame,mode,menu_cursor);
        }
        if(menu_open)KEY_TAP(KEY_B);
        int home=home_nexus();
        int z=hero(75),f=hero(78);
        /* While the assault is close, keep control near the objective so a
         * long trip to a home worker cannot delay Zeratul's next order. */
        int critical_assault=attacking&&front_distance>=0&&front_distance<30;
        /* Fenix, with an enemy within 256 pixels, walks to the reachable
         * spot near the Nexus whose path keeps farthest from every enemy;
         * Protoss hit points do not regenerate. Checked at the start of
         * every step, since a step can spend hundreds of frames. */
        static int next_evade;
        if(!(!saved&&frame>=P9_SAVE_FRAME)&&home>=0&&f>=0&&u[f].owner==SIDE&&frame>=next_evade&&
           enemy_clearance(u[f].x,u[f].y)<256){
            static const int ox[16]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
            static const int oy[16]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
            int ex=-1,ey=-1,best=enemy_clearance(u[f].x,u[f].y);
            for(int r=96;r<=224;r+=64)for(int a=0;a<16;a++){
                int x=limit(b[home].x+ox[a]*r/16,48,WORLD_W-48),y=limit(b[home].y+oy[a]*r/16,48,WORLD_H-48);
                int cell=limit(x/16,0,MAP_COLS-1)+limit(y/16,0,MAP_ROWS-1)*MAP_COLS,wx,wy,distance=-1;
                if(!path_grid_valid||!SC64_GRID_GET(path_walkable,cell))continue;
                /* Score the walk once under way, not just the destination. */
                int clear=1024;
                for(int t=2;t<=8;t++){
                    int c=enemy_clearance(u[f].x+(x-u[f].x)*t/8,u[f].y+(y-u[f].y)*t/8);
                    if(c<clear)clear=c;
                }
                if(clear<=best)continue;
                if(!waypoint_to(u[f].x,u[f].y,x,y,&wx,&wy,&distance)||distance>32)continue;
                best=clear;ex=x;ey=y;
            }
            if(ex>=0&&dist2(u[f].tx,u[f].ty,ex,ey)>48*48){
                (void)hero_command(f,ex,ey);
                next_hero=frame+180;
            }
            next_evade=frame+120;
        }
        if(home<0&&frame>=next_build){
            int pending=0;
            for(int i=0;i<MAX_UNITS;i++)pending|=u[i].alive&&u[i].owner==SIDE&&SC64_BUILD_WORKER_ORDER(u[i].order);
            /* Beside the start's own mineral line (source 2994,3974). */
            if(!pending)(void)build_nexus(1470,1930);
            next_build=frame+300;
        }
        if(!critical_assault&&home>=0&&frame>=next_mine){
            for(int k=0;k<8&&remine_idle_probe(home);k++){}
            next_mine=frame+180;
        }
        if(!critical_assault&&home>=0&&frame>=next_probe){(void)queue_probe();next_probe=frame+300;}
        if(!critical_assault&&home>=0&&count_type(B_NEXUS,0)<2&&frame>=next_expand){
            int left=0;
            for(int k=0;k<MAX_MINERALS;k++)if(m[k].amount>0&&dist2(m[k].x,m[k].y,b[home].x,b[home].y)<320*320)
                left+=m[k].amount;
            /* Expand to the large western field (source 136,3864), reachable
             * by ground, before the start field runs out; production waits
             * for the Nexus money. */
            static int expand_attempts;
            saving_for_nexus=left<6000&&expand_attempts<3;
            next_expand=frame+300;
            if(saving_for_nexus&&build_nexus(150,1890)){
                /* One builder at a time: give it time to arrive and warp. */
                saving_for_nexus=0;
                ++expand_attempts;
                next_expand=frame+3000;
            }
        }
        if(!critical_assault&&home>=0&&frame>=next_supply){
            /* Count Pylons already rising; a builder still walking to its
             * site is covered by the cooldown after each Pylon order. */
            static int last_pylon_order=-100000;
            int pending=0;
            for(int i=0;i<MAX_BUILDINGS;i++)pending+=b[i].alive&&b[i].owner==SIDE&&
                b[i].type==B_PYLON&&b[i].under_construction;
            if(pending<2&&frame-last_pylon_order>=300&&
               race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<=12&&
               build_structure(B_PYLON,MENU_DEPOT,64,448,-1,-1))last_pylon_order=frame;
            else if(getenv("P9_BLOG")&&race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<=12)
                fprintf(stderr,"pylon skip f=%d pending=%d since=%d bank=%d\n",frame,pending,frame-last_pylon_order,minerals);
            next_supply=frame+150;
        }
        if(!critical_assault&&home>=0&&frame>=next_build){
            int want_gateway=count_type(B_GATEWAY,0)<1||(probe_count()>=10&&count_type(B_GATEWAY,0)<6);
            int built=0;
            if(want_gateway){
                built=build_structure(B_GATEWAY,MENU_BARRACKS,80,224,-1,-1);
                /* No powered site: widen the Pylon field first. */
                if(!built)built=build_structure(B_PYLON,MENU_DEPOT,96,448,-1,-1);
            }
            if(!built&&!count_type(B_CYBERNETICS_CORE,0)&&count_type(B_GATEWAY,1))
                built=build_structure(B_CYBERNETICS_CORE,MENU_ACADEMY,80,224,-1,-1);
            if(!built&&!count_type(B_FORGE,0)&&count_type(B_GATEWAY,0)>=2)
                built=build_structure(B_FORGE,MENU_ENG_BAY,80,224,-1,-1);
            if(!built&&count_type(B_FORGE,1)&&count_type(B_PHOTON_CANNON,0)<4&&fenix_home_x>=0)
                built=build_cannon(fenix_home_x,fenix_home_y);
            /* Two more by the expansion Nexus once it stands. */
            if(!built&&count_type(B_FORGE,1)&&count_type(B_PHOTON_CANNON,0)<6)
                for(int n=0;n<MAX_BUILDINGS&&!built;n++)if(n!=home&&b[n].alive&&b[n].owner==SIDE&&
                    b[n].type==B_NEXUS&&building_complete(n))built=build_cannon(b[n].x,b[n].y-48);
            next_build=frame+(minerals>=400?240:600);
        }
        if(!critical_assault&&frame>=next_gas){
            (void)assign_gas_worker();
            for(int k=0;k<4&&remine_idle_probe(home);k++){}
            next_gas=frame+700;
        }
        /* After a Cerebrate falls, Zeratul (60 hit points) travels home
         * inside the regrouping army, never alone. */
        if(escort_home&&!attacking&&frame>=next_escort&&z>=0&&u[z].owner==SIDE&&fenix_home_x>=0){
            int ax=0,ay=0,an=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
                (u[i].type==U_ZEALOT||u[i].type==U_DRAGOON)&&
                dist2(u[i].x,u[i].y,u[z].x,u[z].y)<300*300){ax+=u[i].x;ay+=u[i].y;++an;}
            if(dist2(u[z].x,u[z].y,fenix_home_x+24,fenix_home_y)<200*200)escort_home=0;
            else if(an>=6){
                ax/=an;ay/=an;
                if(dist2(u[z].tx,u[z].ty,ax,ay)>48*48)(void)hero_command(z,ax,ay);
            } else (void)hero_command(z,fenix_home_x+24,fenix_home_y);
            next_hero=frame+180;
            next_escort=frame+60;
        }
        /* While the save waits for a structure to warp in, skip the slow
         * army and hero blocks so the short window is not missed. */
        int saving=!saved&&frame>=P9_SAVE_FRAME;
        if(!critical_assault&&frame>=next_production&&!(saving_for_nexus&&minerals<500)){
            int queued=0;
            while(queued<6&&queue_combat_unit())++queued;
            next_production=frame+60;
        }
        /* Rescue Zeratul and Fenix, then keep Fenix home; Zeratul goes with
         * the army only for an assault. */
        if(!saving&&frame>=next_hero){
            int rx=start_x,ry=start_y;
            if(home>=0){rx=b[home].x;ry=b[home].y+48;}
            fenix_home_x=rx;fenix_home_y=ry;
            if((z>=0&&u[z].owner==RESCUABLE)||(f>=0&&u[f].owner==RESCUABLE)){
                int t=z>=0&&u[z].owner==RESCUABLE?z:f;
                (void)army_command(MENU_MOVE,u[t].x,u[t].y);
            } else {
                if(f>=0&&u[f].owner==SIDE&&dist2(u[f].x,u[f].y,rx,ry)>48*48)
                    (void)hero_command(f,rx,ry);
                if(!attacking&&z>=0&&u[z].owner==SIDE&&dist2(u[z].x,u[z].y,rx+24,ry)>48*48)
                    (void)hero_command(z,rx+24,ry);
            }
            next_hero=frame+180;
        }
        /* Between assaults the army waits west of the build area. */
        if(!saving&&!attacking&&home>=0&&frame>=next_stage){
            int off=0;
            for(int i=0;i<MAX_UNITS;i++)off+=u[i].alive&&u[i].owner==SIDE&&
                (u[i].type==U_ZEALOT||u[i].type==U_DRAGOON)&&dist2(u[i].x,u[i].y,1180,1760)>96*96;
            if(off>=4)(void)army_command(MENU_MOVE,1180,1760);
            next_stage=frame+1200;
        }
        if(!saving&&frame>=next_order&&home>=0&&!attacking){
            /* Defend every Nexus, the expansion included. Overlords are
             * harmless detectors the Zealots cannot reach, and a threat the
             * army has not killed after five orders is out of its reach. */
            static int last_threat=-1,threat_orders;
            static u8 unreachable[MAX_UNITS];
            int threat=-1,bd=360*360;
            for(int i=0;i<MAX_UNITS;i++)if(!u[i].alive)unreachable[i]=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type!=42&&!unreachable[i])
                for(int n=0;n<MAX_BUILDINGS;n++)if(b[n].alive&&b[n].owner==SIDE&&b[n].type==B_NEXUS){
                    int d=dist2(u[i].x,u[i].y,b[n].x,b[n].y);
                    if(d<bd){bd=d;threat=i;}
                }
            if(threat>=0){
                threat_orders=threat==last_threat?threat_orders+1:0;
                last_threat=threat;
                if(threat_orders>=5)unreachable[threat]=1;
                (void)army_command(MENU_ATTACK,u[threat].x,u[threat].y);
                fprintf(trace,"{\"kind\":\"defend\",\"frame\":%u,\"target\":[%d,%d]}\n",
                    (unsigned)frame,u[threat].x,u[threat].y);
                next_hero=frame+30;
                next_order=frame+180;
            }
        }
        /* Zeratul waits at home until the army's front is near the
         * Cerebrate, then follows his own route into its location. */
        if(!saving&&attacking&&frame>=next_zeratul&&z>=0&&u[z].owner==SIDE){
            int loc=0,c=cerebrate(&loc);
            static int last_target=-1;
            if(loc!=last_target){last_target=loc;front_distance=-1;}
            int guards=0,escort=0;
            if(c>=0){
                for(int i=0;i<MAX_UNITS;i++)escort+=u[i].alive&&u[i].owner==SIDE&&
                    (u[i].type==U_ZEALOT||u[i].type==U_DRAGOON)&&dist2(u[i].x,u[i].y,b[c].x,b[c].y)<160*160;
                const Sc64P9SourceLocation *gl=&sc64_p9_source_locations[loc-1];
                int gx0=(int)(gl->left+gl->right)/4,gy0=(int)(gl->top+gl->bottom)/4;
                for(int i=0;i<MAX_UNITS;i++)guards+=u[i].alive&&u[i].owner==ENEMY&&
                    dist2(u[i].x,u[i].y,gx0,gy0)<120*120;
            }
            int was_entering=zeratul_entering;
            zeratul_entering=0;
            if(army_count()<16&&fenix_home_x>=0){
                /* Too small an army to shelter him: wait at home. */
                if(dist2(u[z].x,u[z].y,fenix_home_x+24,fenix_home_y)>48*48)
                    (void)hero_command(z,fenix_home_x+24,fenix_home_y);
            } else if((u[z].shield<300||(was_entering&&escort<6))&&fenix_home_x>=0&&c>=0){
                /* Step away from the Cerebrate until the shields recharge. */
                int vx=u[z].x-b[c].x,vy=u[z].y-b[c].y,len=FAST_ABS(vx)+FAST_ABS(vy);
                if(len>0)(void)hero_command(z,u[z].x+vx*96/len,u[z].y+vy*96/len);
            } else if(c>=0&&front_distance>=0&&front_distance<14&&guarding_sunken(c)<0&&
                      b[c].hp<500&&guards<=2&&u[z].shield>=300&&escort>=10){
                const Sc64P9SourceLocation *l=&sc64_p9_source_locations[loc-1];
                int lx=(int)(l->left+l->right)/4,ly=(int)(l->top+l->bottom)/4;
                int zx=lx,zy=ly+20,zd=-1;
                (void)waypoint_to(u[z].x,u[z].y,lx,ly,&zx,&zy,&zd);
                if(zd>=0&&zd<10){zx=lx;zy=ly+20;}
                zeratul_entering=1;
                (void)hero_command(z,zx,zy);
            } else if(c>=0&&front_unit>=0&&u[front_unit].alive&&u[front_unit].owner==SIDE&&
                      front_distance>=0&&front_distance<30){
                /* Cloaked, he waits at home until the front is close, then
                 * trails it: its centre, 96 pixels back from the Cerebrate. */
                int ax=0,ay=0,an=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
                    (u[i].type==U_ZEALOT||u[i].type==U_DRAGOON)&&
                    dist2(u[i].x,u[i].y,u[front_unit].x,u[front_unit].y)<160*160){ax+=u[i].x;ay+=u[i].y;++an;}
                if(an>=6){
                    ax/=an;ay/=an;
                    int vx=ax-b[c].x,vy=ay-b[c].y,len=FAST_ABS(vx)+FAST_ABS(vy);
                    if(len>0){ax+=vx*96/len;ay+=vy*96/len;}
                    if(dist2(u[z].x,u[z].y,ax,ay)>64*64)(void)hero_command(z,ax,ay);
                }
            } else if(fenix_home_x>=0&&dist2(u[z].x,u[z].y,fenix_home_x+24,fenix_home_y)>48*48)
                (void)hero_command(z,fenix_home_x+24,fenix_home_y);
            next_zeratul=frame+30;
        }
        if(!saving&&frame>=next_order){
            int loc=0,c=cerebrate(&loc);
            if(attacking&&race_p9.switches!=regrouped_switches){
                /* A Cerebrate fell: Zeratul leaves first while the army
                 * holds and covers him, then all regroup at home. */
                regrouped_switches=race_p9.switches;
                regroup_until=frame+3900;
                escort_home=1;
                attacking=0;
                cover_until=frame+900;
                if(z>=0&&u[z].owner==SIDE&&fenix_home_x>=0)
                    (void)hero_command(z,fenix_home_x+24,fenix_home_y);
                next_escort=frame+900;
            }
            if(cover_until&&frame>=cover_until){
                cover_until=0;
                if(home>=0)(void)army_command(MENU_MOVE,b[home].x,b[home].y-96);
            }
            /* After the first Cerebrate, attack at 56 units, or with 40 once
             * 60,000 frames pass without reaching that. */
            if(!attacking&&frame>=regroup_until&&
               (army_count()>=(race_p9.switches?56:32)||
                (race_p9.switches&&frame>=regroup_until+60000&&army_count()>=40))&&
               (count_type(B_PHOTON_CANNON,1)>=3||frame>=60000))attacking=1;
            if(attacking&&c>=0&&z>=0&&u[z].owner==SIDE){
                const Sc64P9SourceLocation *l=&sc64_p9_source_locations[loc-1];
                int lx=(int)(l->left+l->right)/4,ly=(int)(l->top+l->bottom)/4;
                /* Route from the frontline unit nearest the Cerebrate; newly
                 * trained units at home would skew an army average. Zeratul
                 * (no weapon flag, so outside R's group) trails the group
                 * around that unit and enters the Cerebrate's location only
                 * once it is nearly dead. */
                int front=-1,fd=0x7fffffff,gx=0,gy=0,gn=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!IS_WORKER(u[i].type)&&
                    u[i].raw_type!=75&&u[i].raw_type!=78){
                    int d=dist2(u[i].x,u[i].y,b[c].x,b[c].y);
                    if(d<fd){fd=d;front=i;}
                }
                if(front<0)front=z;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!IS_WORKER(u[i].type)&&
                    u[i].raw_type!=75&&u[i].raw_type!=78&&dist2(u[i].x,u[i].y,u[front].x,u[front].y)<160*160){
                    gx+=u[i].x;gy+=u[i].y;++gn;
                }
                int wx=b[c].x,wy=b[c].y,distance=-1,sunken=guarding_sunken(c);
                /* A unit inside a building pocket has no grid route; try the
                 * next nearest units before falling back to a direct order. */
                {
                    static u8 tried[MAX_UNITS];
                    for(int i=0;i<MAX_UNITS;i++)tried[i]=0;
                    int origin=front;
                    for(int k=0;k<8&&!waypoint_to(u[origin].x,u[origin].y,b[c].x,b[c].y,&wx,&wy,&distance);k++){
                        int nd=0x7fffffff;tried[origin]=1;origin=-1;
                        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!tried[i]&&
                            (u[i].type==U_ZEALOT||u[i].type==U_DRAGOON)){
                            int d=dist2(u[i].x,u[i].y,b[c].x,b[c].y);
                            if(d<nd){nd=d;origin=i;}
                        }
                        if(origin<0)break;
                        distance=-1;
                    }
                    if(origin>=0&&distance>=0)front=origin;
                }
                /* Gather about 25 cells out until most of the army has
                 * closed up, then strike together. Near the Cerebrate,
                 * clear its Sunken Colonies first. */
                int total=0,near_front=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
                    (u[i].type==U_ZEALOT||u[i].type==U_DRAGOON)){
                    ++total;
                    near_front+=dist2(u[i].x,u[i].y,u[front].x,u[front].y)<200*200;
                }
                if(!striking&&distance>=0&&distance<=25){
                    if(near_front*10>=total*8)striking=1;
                    else{wx=u[front].x;wy=u[front].y;}
                }
                if(striking&&distance>25)striking=0;
                if(sunken>=0&&distance>=0&&distance<16){wx=b[sunken].x;wy=b[sunken].y;}
                /* Reinforcements far behind are sent only every fifth order. */
                static int order_round;
                army_anchor_limit=(order_round++%5)?480:0;
                army_cursor_watch=1;army_cursor_emergency=0;
                (void)army_command(MENU_ATTACK,wx,wy);
                army_cursor_watch=0;
                if(army_cursor_emergency&&z>=0&&u[z].alive&&fenix_home_x>=0){
                    army_cursor_emergency=0;
                    (void)hero_command(z,fenix_home_x+24,fenix_home_y);
                    next_zeratul=frame+30;
                }
                army_anchor_limit=0;
                front_distance=distance;
                front_unit=front;
                (void)gn;(void)gx;(void)gy;
                fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"route\":%d,\"army\":%d,\"cerebrate\":%d,\"cerebrate_hp\":%d}\n",
                    (unsigned)frame,wx,wy,distance,army_count(),loc,b[c].hp);
                next_hero=frame+60;
                if(army_count()<8)attacking=0;
            }
            next_order=frame+(attacking?300:600);
        }
        /* Save while a structure is still warping in, so the record carries
         * active construction: start a Pylon if nothing is, and fall back
         * after a bounded wait. */
        if(!saved&&frame>=P9_SAVE_FRAME&&!under_construction()&&frame>=next_save_build){
            (void)build_structure(B_PYLON,MENU_DEPOT,64,448,-1,-1);
            next_save_build=frame+300;
        }
        if(!saved&&frame>=P9_SAVE_FRAME&&(under_construction()||frame>=P9_SAVE_FRAME+6000)){
            int building=under_construction();
            /* The building selection, the orders menu's remembered command
             * and an open orders menu are UI state, not saved: a fresh
             * process starts with none of them. Return to that state with
             * ordinary inputs, so replayed keys mean the same thing after a
             * load. B from a targeting mode reopens the menu, so close
             * everything last. */
            close_menus();
            if(menu_last!=0){
                /* Any of the side's units opens the orders menu; try a few
                 * (a mining Probe can be hard to tap). */
                int ok=0;
                for(int i=0,tries=0;i<MAX_UNITS&&!ok&&tries<4;i++)
                    if(u[i].alive&&u[i].owner==SIDE&&(u[i].type==U_ZEALOT||u[i].type==U_DRAGOON||
                       u[i].type==U_PROBE)){++tries;ok=hero_select(i);}
                if(ok){
                    KEY_TAP(KEY_B);
                    for(int n=0;n<16&&menu_open&&menu_cursor!=0;n++)KEY_TAP(KEY_UP);
                    if(menu_open&&menu_cursor==0)KEY_TAP(KEY_A);
                    close_menus();
                }
            }
            clear_selected_building();
            for(int k=0;k<4&&(menu_open||mode!=MODE_NORMAL);k++)KEY_TAP(KEY_B);
            unsigned first=host_tick;
            KEY_TAP(KEY_START);
            int paused=screen==SCREEN_PAUSE;
            KEY_TAP(KEY_B);
            KEY_TAP(KEY_START);
            saved=1;
            char save_path[512];
            snprintf(save_path,sizeof(save_path),"%s.sav",path);
            FILE *sf=fopen(save_path,"wb");
            if(!sf||fwrite(host_save,1,sizeof(host_save),sf)!=sizeof(host_save)){perror(save_path);return 2;}
            fclose(sf);
            fprintf(trace,"{\"kind\":\"save\",\"frame\":%u,\"first_call\":%u,\"resume_call\":%u,\"paused\":%d,\"screen\":%d,\"under_construction\":%d,\"menu_last\":%d}\n",
                (unsigned)frame,first,host_tick,paused,screen,building,menu_last);
            census("saved");
        }
        service_sample();
        if(frame>=next_checkpoint){census("progress");next_checkpoint=frame+2000;}
        input(0);
    }
    census("final");
    fclose(trace);
    printf("p9 controller: frame=%u calls=%u won=%d lost=%d screen=%d switches=%u\n",
           (unsigned)frame,host_tick,won,lost,screen,race_p9.switches);
    return won?0:1;
}
