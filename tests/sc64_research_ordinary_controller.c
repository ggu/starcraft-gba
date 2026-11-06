/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary-button Protoss custom research checkpoint controller.
 * The controller observes game state to choose controls; mutations go through
 * game_update, except loading or copying the save-device backing file. */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef P10_CONTROLLER_PROFILE
#define P10_CONTROLLER_PROFILE "PROTOSS_P10_FLAGS"
#endif
#ifndef P10_RUN_LIMIT
#define P10_RUN_LIMIT 220000u
#endif
/* One ordinary pause-menu save; the replay tools resume from this record. */
#ifndef P10_ATTACK_ARMY
#define P10_ATTACK_ARMY 60
#endif
#ifndef P10_SAVE_FRAME
#define P10_SAVE_FRAME 30000
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
    if (host_tick >= 45000u) { fprintf(stderr, "INPUT_CAP %u\n", host_tick); exit(2); }
    u16 pressed=(u16)(held & (u16)~prior);
    int changed=held!=prior;
    unsigned call_index=host_tick;
    unsigned frame_before=(unsigned)frame;
    game_update(held,pressed);
    game_render();
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

/* The army unit with the shortest walk to (tx,ty), skipping units in skip[].
 * Straight-line distance picks a unit on the far side of a cliff, and the whole
 * army then waits for it. */
static int nearest_by_route(int tx,int ty,const u8 *skip)
{
    static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    static short dist[MAP_CELLS];
    static int queue[MAP_CELLS];
    if(!path_grid_valid)return -1;
    int head=0,tail=0,gx=limit(tx/16,0,MAP_COLS-1),gy=limit(ty/16,0,MAP_ROWS-1);
    for(int i=0;i<MAP_COLS*MAP_ROWS;i++)dist[i]=-1;
    /* The goal is a building, so seed the walkable cells around it. */
    for(int r=-4;r<=4;r++)for(int c=-4;c<=4;c++){
        int nc=gx+c,nr=gy+r;
        if(nc<0||nr<0||nc>=MAP_COLS||nr>=MAP_ROWS)continue;
        int cell=nc+nr*MAP_COLS;
        if(SC64_GRID_GET(path_walkable,cell)){dist[cell]=0;queue[tail++]=cell;}
    }
    while(head<tail){
        int cell=queue[head++],col=cell%MAP_COLS,row=cell/MAP_COLS;
        for(int k=0;k<4;k++){
            int nc=col+dx[k],nr=row+dy[k];
            if(nc<0||nr<0||nc>=MAP_COLS||nr>=MAP_ROWS)continue;
            int next=nc+nr*MAP_COLS;
            if(dist[next]>=0||!SC64_GRID_GET(path_walkable,next))continue;
            dist[next]=(short)(dist[cell]+1);queue[tail++]=next;
        }
    }
    int best=-1,bd=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!skip[i]&&
        (u[i].type==U_MARINE||u[i].type==U_FIREBAT)){
        int d=dist[limit(u[i].x/16,0,MAP_COLS-1)+limit(u[i].y/16,0,MAP_ROWS-1)*MAP_COLS];
        if(d>=0&&d<bd){bd=d;best=i;}
    }
    return best;
}

static int hero(int raw)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==raw)return i;
    return -1;
}

/* Distance from (x,y) to the nearest enemy unit, capped at 1024 pixels. */
static int enemy_clearance(int x,int y)
{
    int best=1024*1024;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY){
        int d=dist2(u[i].x,u[i].y,x,y);
        if(d<best)best=d;
    }
    int r=0;
    while((r+1)*(r+1)<=best)++r;
    return r;
}

static int overmind(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==147)return i;
    return -1;
}

static int under_construction(void)
{
    int n=0;for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&b[i].under_construction;
    return n;
}

static int home_center(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_NEXUS)return i;
    return -1;
}

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
    }
    input(0);
    if(cx!=x||cy!=y)
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
    move_cursor(x,y);KEY_TAP(KEY_A);
    return 1;
}

/* Order the army by type groups: a double tap on a Marine or Firebat
 * selects that type on screen, which never includes Zeratul, the Hyperion or
 * the Carrier (R's whole-army shortcut would). Units already carrying this
 * order are skipped; repeat for off-screen clusters. */
static int army_force;  /* Nonzero: re-order every group. */
static int army_only;   /* Nonzero: order only this unit type. */
static int army_command(int which,int x,int y)
{
    static u8 ordered[MAX_UNITS];
    int groups=0;
    for(int i=0;i<MAX_UNITS;i++)
        ordered[i]=!army_force&&((u[i].order==(which==MENU_ATTACK?ORDER_ATTACK_MOVE:ORDER_MOVE)&&
            dist2(u[i].tx,u[i].ty,x,y)<96*96)||
            (which==MENU_ATTACK&&u[i].order==ORDER_ATTACK_TARGET));
    for(int round=0;round<6;round++){
        int anchor=-1,bd=0x7fffffff;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!ordered[i]&&
            (u[i].type==U_MARINE||u[i].type==U_FIREBAT)&&(!army_only||u[i].type==army_only)){
            int d=dist2(u[i].x,u[i].y,x,y);
            if(d<bd){bd=d;anchor=i;}
        }
        if(anchor<0)break;
        close_menus();
        clear_selected_building();
        move_cursor(u[anchor].x,u[anchor].y);
        KEY_TAP(KEY_A);KEY_TAP(KEY_A);
        int bad=0,count=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected){
            ++count;
            if(u[i].type!=U_MARINE&&u[i].type!=U_FIREBAT)bad=1;
        }
        ordered[anchor]=1;
        if(!count||bad)continue;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected)ordered[i]=1;
        if(command_selected(which,x,y))++groups;
    }
    return groups>0;
}

/* Tap one actor to select it alone. A tap can pick an overlapping unit; an
 * L tap cycles the single selection through the side's units. */
static int select_single(int slot)
{
    if(slot<0||!u[slot].alive||u[slot].owner!=SIDE)return 0;
    close_menus();
    clear_selected_building();
    move_cursor(u[slot].x,u[slot].y);KEY_TAP(KEY_A);
    for(int n=0;n<40&&(!u[slot].selected||selected_count()!=1);n++)KEY_TAP(KEY_L);
    return u[slot].selected&&selected_count()==1;
}

/* Select at least the SCV under the cursor. Idle SCVs stack on a spent
 * patch, where a tap cannot pick one out, so any selection of workers there
 * will do: the mining order goes to every empty one. */
static int select_workers_at(int slot)
{
    if(slot<0||!u[slot].alive||u[slot].owner!=SIDE)return 0;
    close_menus();
    clear_selected_building();
    move_cursor(u[slot].x,u[slot].y);KEY_TAP(KEY_A);
    if(selected_building>=0||find_selected_worker()<0)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&!sc64_race_is_worker(u[i].type))return 0;
    return 1;
}

/* As in sc64_visible_worker_orders.h, issue mining through normal field
 * input only for idle empty workers and currently visible mineral patches.
 * Patches are spread by how many SCVs already work them, so a group does not
 * drain one patch and go idle together. */
static int order_idle_scvs_to_visible_minerals(void)
{
    static const int dx[]={0,8,-8,0,0,8,-8,8,-8};
    static const int dy[]={0,0,0,8,-8,8,8,-8,-8};
    int assigned=0;
    for(int w=0;w<MAX_UNITS && screen==SCREEN_PLAY;w++) {
        if(!u[w].alive || u[w].owner!=SIDE || u[w].type!=U_PROBE ||
           u[w].order!=ORDER_IDLE ||
#ifndef SC64_WORKER_CARGO_KIND
           u[w].carry ||
#endif
           !select_workers_at(w)) continue;
        /* Raynor's patches lie just past the base's sight: walk toward the
         * nearest one first, and never cross the map for a visible patch. */
        int target=-1,best=0x7fffffff,nearest=-1,near_d=0x7fffffff;
        for(int p=0;p<MAX_MINERALS;p++) if(m[p].amount>0) {
            int reach=dist2(u[w].x,u[w].y,m[p].x,m[p].y);
            if(reach<near_d){near_d=reach;nearest=p;}
            if(!currently_visible(m[p].x,m[p].y)||reach>400*400)continue;
            int workers=0;
            for(int j=0;j<MAX_UNITS;j++)workers+=u[j].alive&&u[j].owner==SIDE&&u[j].order==ORDER_MINE&&u[j].target==p;
            int d=reach/256+workers*64;
            if(d<best){best=d;target=p;}
        }
        if(target<0&&nearest>=0&&near_d<=600*600){
            (void)command_selected(MENU_MOVE,(u[w].x+m[nearest].x)/2,(u[w].y+m[nearest].y)/2);
            continue;
        }
        if(target<0) continue;
        for(unsigned k=0;k<sizeof(dx)/sizeof(dx[0]);k++) {
            int x=m[target].x+dx[k],y=m[target].y+dy[k];
            move_cursor(x,y);
            if(abs(cx-x)>2 || abs(cy-y)>2 || find_selected_worker()<0)continue;
            if(field_ui_a_action(selected_count())!=UI_A_MINE)continue;
            KEY_TAP(KEY_A);
            if(u[find_selected_worker()].order==ORDER_MINE)assigned++;
            break;
        }
    }
    return assigned;
}

static int unit_order(int slot,int which,int x,int y)
{ return select_single(slot)&&command_selected(which,x,y); }

static int hero_command(int slot,int x,int y)
{ return unit_order(slot,MENU_MOVE,x,y); }

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
        u[i].raw_type!=28&&u[i].raw_type!=78&&u[i].raw_type!=72;
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
    int home=home_center();
    if(home<0||!building_complete(home)||probe_count()+b[home].queue_count>=18||
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

/* Minerals production leaves unspent, so the save-time Depot can start. */
static int mineral_reserve;

static int queue_combat_unit(void)
{
    /* One Firebat in four: Marines shoot air and cost no gas. */
    int firebats=0,marines=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE){firebats+=u[i].type==U_FIREBAT;marines+=u[i].type==U_MARINE;}
    int wanted=count_type(B_ACADEMY,1)&&gas>=25&&minerals>=50&&firebats*3<marines?U_FIREBAT:U_MARINE;
    const Sc64RaceUnitRule *rule=sc64_race_unit_rule(wanted);
    if(!rule||minerals<(int)rule->minerals+mineral_reserve||gas<(int)rule->gas||
       race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<(int)rule->supply)return 0;
    int producer=-1,best_queue=4;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_BARRACKS&&building_complete(i)&&b[i].queue_count<best_queue){
        producer=i;best_queue=b[i].queue_count;
    }
    if(producer<0)return 0;
    close_menus();
    /* Marines crowd the Barracks, and a tap on one selects it instead. */
    static const int tx[5]={0,-16,16,0,0},ty[5]={0,0,0,-16,16};
    for(int k=0;k<5&&selected_building!=producer;k++){
        move_cursor(b[producer].x+tx[k],b[producer].y+ty[k]);KEY_TAP(KEY_A);
    }
    if(selected_building!=producer)return 0;
    for(int n=0;n<4&&train_type!=wanted;n++)KEY_TAP(KEY_R);
    if(train_type!=wanted)return 0;
    int before=b[producer].queue_count;
    KEY_TAP(KEY_A);
    if(b[producer].queue_count<=before)return 0;
    fprintf(trace,"{\"kind\":\"production\",\"frame\":%u,\"unit\":\"%s\",\"bank_after\":[%d,%d]}\n",
        (unsigned)frame,wanted==U_FIREBAT?"dragoon":"zealot",minerals,gas);
    return 1;
}

static int free_builder(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&
        u[i].order==ORDER_MINE&&!u[i].carry)return i;
    /* An idle SCV holding cargo cannot be sent back to minerals afterward. */
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&
        !SC64_BUILD_WORKER_ORDER(u[i].order)&&u[i].order!=ORDER_GAS&&!u[i].carry)return i;
    return -1;
}

/* Sites the game refused once are not tried again. */
static int refused_x[64],refused_y[64],refused_count;

/* Keep a 16-pixel walkable lane between a new structure's pinned BWAPI tile
 * footprint and every structure and mineral patch, so the base cannot wall
 * its own army in. Refused and occupied sites are skipped. */
static int site_ok(int type,int x,int y)
{
    for(int k=0;k<refused_count;k++)
        if(FAST_ABS(refused_x[k]-x)<16&&FAST_ABS(refused_y[k]-y)<16)return 0;
    int raw=sc64_race_raw_building(type);
    if(raw<0||raw>=228)return 0;
    int hw=siege_bounds[raw][0]*8,hh=siege_bounds[raw][1]*8;
    for(int k=0;k<MAX_BUILDINGS;k++)if(b[k].alive&&b[k].owner==SIDE&&b[k].raw_type>=0&&b[k].raw_type<228){
        int kw=siege_bounds[b[k].raw_type][0]*8,kh=siege_bounds[b[k].raw_type][1]*8;
        if(FAST_ABS(b[k].x-x)<hw+kw+16&&FAST_ABS(b[k].y-y)<hh+kh+16)return 0;
    }
    for(int k=0;k<MAX_MINERALS;k++)if(m[k].amount>0&&
        FAST_ABS(m[k].x-x)<hw+16+16&&FAST_ABS(m[k].y-y)<hh+8+16)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&dist2(u[i].x,u[i].y,x,y)<32*32)return 0;
    return 1;
}

/* Place `type` through the SCV build menu at the nearest legal site. */
static int build_structure(int type,int menu_row,int min_r,int max_r,int fixed_x,int fixed_y)
{
    int home=home_center(),best=0x7fffffff,site_x=fixed_x,site_y=fixed_y;
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
    if(home<0||!rule||minerals<(int)rule->minerals||gas<(int)rule->gas)return 0;
    if(site_x<0){
        for(int dy=-max_r;dy<=max_r;dy+=16)for(int dx=-max_r;dx<=max_r;dx+=16){
            int x=b[home].x+dx,y=b[home].y+dy,d=dx*dx+dy*dy;
            if(d<min_r*min_r||x<32||x>=WORLD_W-32||y<32||y>=WORLD_H-32||d>=best||
               !race_can_site(SIDE,type,x,y)||!can_place(type,x,y))continue;
            if(!site_ok(type,x,y))continue;
            best=d;site_x=x;site_y=y;
        }
    }
    if(site_x<0)return 0;
    int worker=free_builder();
    if(worker<0)return 0;
    close_menus();
    clear_selected_building();
    move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    for(int n=0;n<40&&(!u[worker].selected||selected_count()!=1);n++)KEY_TAP(KEY_L);
    if(!u[worker].selected||selected_count()!=1)return 0;
    KEY_TAP(KEY_B);
    for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=menu_row;n++)KEY_TAP(KEY_DOWN);
    if(!menu_open||menu_cursor!=menu_row){close_menus();return 0;}
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
    int home=home_center(),best=-1,bd=0x7fffffff;
    if(home<0||count_type(B_ASSIMILATOR,0))return 0;
    for(int g=0;g<geyser_count;g++){
        int d=dist2(geysers[g].x,geysers[g].y,b[home].x,b[home].y);
        if(d<bd){bd=d;best=g;}
    }
    if(best<0||bd>320*320)return 0;
    return build_structure(B_ASSIMILATOR,MENU_REFINERY,0,0,geysers[best].x,geysers[best].y);
}

/* A builder that cannot reach its site stays pending; after 900 frames send
 * it back to the nearest mineral patch and never try that site again. */
static int pending_since[MAX_UNITS];
static int remine_stuck_builder(void)
{
    int worker=-1,home=home_center(),patch=-1,best=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++){
        if(!(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&SC64_PENDING_BUILD_ORDER(u[i].order))){
            pending_since[i]=-1;continue;
        }
        if(pending_since[i]<0)pending_since[i]=frame;
        if(worker<0&&frame-pending_since[i]>900)worker=i;
    }
    if(worker<0||home<0)return 0;
    for(int k=0;k<MAX_MINERALS;k++)if(m[k].amount>0){
        int d=dist2(m[k].x,m[k].y,b[home].x,b[home].y);
        if(d<best){best=d;patch=k;}
    }
    if(patch<0)return 0;
    if(refused_count<64){refused_x[refused_count]=u[worker].tx;refused_y[refused_count++]=u[worker].ty;}
    if(!select_single(worker))return 0;
    move_cursor(m[patch].x,m[patch].y);KEY_TAP(KEY_A);
    pending_since[worker]=-1;
    return u[worker].order==ORDER_MINE;
}

/* A builder can pick up cargo between being chosen and placing its building,
 * and an idle SCV holding cargo refuses the mineral order. Tapping the
 * Refinery gives it a gas order, which drops the cargo; STOP then leaves it
 * idle and empty for the mining pass. */
static int recover_loaded_probe(void)
{
#ifdef SC64_WORKER_CARGO_KIND
    /* The normal mineral order accepts loaded workers; the mining pass handles them. */
    return 0;
#else
    int refinery=-1,worker=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_ASSIMILATOR&&building_complete(i)){refinery=i;break;}
    for(int i=0;i<MAX_UNITS&&worker<0;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&
        u[i].order==ORDER_IDLE&&u[i].carry)worker=i;
    if(refinery<0||worker<0||!select_workers_at(worker))return 0;
    move_cursor(b[refinery].x,b[refinery].y);KEY_TAP(KEY_A);
    if(u[worker].order!=ORDER_GAS)return 0;
    KEY_TAP(KEY_B);
    for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=MENU_STOP;n++)KEY_TAP(KEY_DOWN);
    if(menu_open&&menu_cursor==MENU_STOP)KEY_TAP(KEY_A);
    close_menus();
    return u[worker].order==ORDER_IDLE;
#endif
}

#ifndef P10_GAS_BANK
#define P10_GAS_BANK 300
#endif
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
    /* Gas beyond what Firebats can spend costs mineral income: STOP an SCV and
     * let the mining pass send it to a patch. */
    if(gas>=P10_GAS_BANK){
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&
            u[i].order==ORDER_GAS&&!u[i].carry&&select_workers_at(i)){
            KEY_TAP(KEY_B);
            for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=MENU_STOP;n++)KEY_TAP(KEY_DOWN);
            if(menu_open&&menu_cursor==MENU_STOP)KEY_TAP(KEY_A);
            close_menus();
            return 1;
        }
        return 0;
    }
    if(gas_workers>=3||worker<0)return 0;
    close_menus();
    clear_selected_building();move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    for(int n=0;n<40&&(!u[worker].selected||selected_count()!=1);n++)KEY_TAP(KEY_L);
    if(!u[worker].selected||selected_count()!=1)return 0;
    move_cursor(b[refinery].x,b[refinery].y);KEY_TAP(KEY_A);
    return u[worker].order==ORDER_GAS;
}

static void census(const char *label)
{
    int cell=overmind(),z=hero(75),g=hero(82),h=hero(28);
    int enemy=0,enemy_b=0;
    for(int i=0;i<MAX_UNITS;i++)enemy+=u[i].alive&&u[i].owner==ENEMY;
    for(int i=0;i<MAX_BUILDINGS;i++)enemy_b+=b[i].alive&&b[i].owner==ENEMY;
    fprintf(trace,"{\"kind\":\"checkpoint\",\"label\":\"%s\",\"frame\":%u,\"call_index\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,"
        "\"side_units\":%d,\"army\":%d,\"scvs\":%d,\"enemy_units\":%d,\"enemy_buildings\":%d,"
        "\"overmind_hp\":%d,\"zeratul\":[%d,%d],\"gantrithor\":[%d,%d],\"hyperion\":[%d,%d],\"bank\":[%d,%d],\"supply\":[%d,%d],"
        "\"gateways\":%d,\"cursor\":[%d,%d]}\n",
        label,(unsigned)frame,host_tick,screen,won,lost,
        live_units_owner(SIDE),army_count(),probe_count(),enemy,enemy_b,
        cell>=0?b[cell].hp:-1,z>=0?u[z].owner:-1,z>=0?u[z].hp:-1,g>=0?u[g].owner:-1,g>=0?u[g].hp:-1,
        h>=0?u[h].owner:-1,h>=0?u[h].hp:-1,minerals,gas,
        race_owner_supply(SIDE),race_owner_supply_cap(SIDE),count_type(B_BARRACKS,0),cx,cy);
    fflush(trace);
}

static int completed(int type)
{
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].owner==SIDE && b[i].type==type && !b[i].under_construction) return i;
    return -1;
}
static int select_building(int slot)
{
    static const int dx[]={0,-8,8,0,0},dy[]={0,0,0,-8,8};
    close_menus();
    for (unsigned k=0;k<5 && selected_building!=slot;++k) {
        move_cursor(b[slot].x+dx[k],b[slot].y+dy[k]); KEY_TAP(KEY_A);
    }
    return selected_building==slot && selected_count()==0;
}
static int prefix(const char *path)
{
    FILE *f=fopen(path,"r"); char line[512]; unsigned ticks[4096],holds[4096],frames[4096],n=0;
    if (!f) return 0;
    while (fgets(line,sizeof line,f)) {
        unsigned t,before,after,held,pressed;
        if (sscanf(line,"{\"kind\":\"input\",\"call_index\":%u,\"frame_before\":%u,\"frame_after\":%u,\"held\":%u,\"pressed\":%u}",&t,&before,&after,&held,&pressed)==5) {
            if(n>=4096 || t>=816) { fclose(f); return 0; }
            ticks[n]=t;holds[n]=held;frames[n]=before;++n;
        }
    }
    fclose(f); unsigned pos=0,held=0;
    for (unsigned t=0;t<816;++t) {
        if(pos<n && ticks[pos]==t) { if((unsigned)frame!=frames[pos]) return 0; held=holds[pos++]; }
        input((u16)held);
    }
    return pos==n && screen==SCREEN_PLAY && race_mode && race_human==R_PROTOSS && race_bots==1;
}
int main(int argc,char **argv)
{
    if(argc!=4) return 2;
    trace=fopen(argv[2],"w"); if(!trace) return 2;
    gba_init();game_init();prior=0;
    if(!strcmp(argv[1],"resume")) {
        FILE *f=fopen(argv[3],"rb"); if(!f || fread(host_save,1,sizeof host_save,f)!=sizeof host_save || fgetc(f)!=EOF) return 3;
        fclose(f); KEY_TAP(KEY_L);
        if(screen!=SCREEN_PLAY) return 4;
        int slot=-1;for(int i=0;i<MAX_BUILDINGS;++i) if(sc64_pr_stage_job_marked(&b[i])) {if(slot>=0)return 5;slot=i;}
        Sc64PrPaidReceipt paid;
        if(slot<0 || !sc64_pr_stage_receipt(slot,&paid) || paid.payer!=SIDE || paid.family!=SC64_PR_GROUND_WEAPONS) return 6;
        fprintf(stderr,"RESUMED frame=%d timer=%d bank=%d,%d bots=%d\n",frame,b[slot].queue_progress,minerals,gas,race_bots);
        for(int i=0;i<8000 && screen==SCREEN_PLAY && !lost && !won && b[slot].queue_progress;++i) input(0);
        fprintf(stderr,"END frame=%d screen=%d lost=%d level=%u timer=%d\n",frame,screen,lost,sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS],b[slot].queue_progress);
        fclose(trace);
        return sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]==1 ? 0:7;
    }
    if(strcmp(argv[1],"prefix") || !prefix(argv[3])) return 8;
    fprintf(stderr,"PREFIX frame=%d ticks=%u bank=%d,%d pylon=%d bots=%d\n",frame,host_tick,minerals,gas,completed(B_PYLON),race_bots);
    for(int cycle=0;cycle<500 && host_tick<44000 && screen==SCREEN_PLAY && !lost && !won;++cycle) {
        int refinery=completed(B_ASSIMILATOR),forge=completed(B_FORGE);
        (void)order_idle_scvs_to_visible_minerals();
        if(refinery>=0) (void)assign_gas_worker();
        if(!under_construction() && completed(B_PYLON)>=0) {
            if(refinery<0 && !count_type(B_ASSIMILATOR,0) && minerals>=100) (void)build_assimilator();
            else if(forge<0 && !count_type(B_FORGE,0) && minerals>=150) (void)build_structure(B_FORGE,MENU_ENG_BAY,64,320,-1,-1);
        }
        if(forge>=0 && minerals>=100 && gas>=100 && select_building(forge)) {
            KEY_TAP(KEY_B); if(menu_open && menu_cursor==0) KEY_TAP(KEY_A);
            Sc64PrPaidReceipt paid;
            if(sc64_pr_stage_receipt(forge,&paid) && paid.payer==SIDE && paid.family==SC64_PR_GROUND_WEAPONS) {
                fprintf(stderr,"PAID tick=%u frame=%d timer=%d bank=%d,%d enemy=%d\n",host_tick,frame,b[forge].queue_progress,minerals,gas,live_units_owner(ENEMY));
                KEY_TAP(KEY_START); KEY_TAP(KEY_B);
                if(screen!=SCREEN_PAUSE || (pause_notice&0xc0)!=UI_NOTICE_SAVED) return 9;
                FILE *f=fopen(".local/research-ordinary/host-save.sav","wb"); if(!f || fwrite(host_save,1,sizeof host_save,f)!=sizeof host_save || fclose(f)) return 10;
                fprintf(stderr,"SAVED tick=%u frame=%d timer=%d\n",host_tick,frame,b[forge].queue_progress); fclose(trace);return 0;
            }
        }
        if(cycle%10==0)fprintf(stderr,"CENSUS tick=%u frame=%d bank=%d,%d pylon=%d assimilator=%d forge=%d enemies=%d\n",host_tick,frame,minerals,gas,completed(B_PYLON),refinery,forge,live_units_owner(ENEMY));
        for(int t=0;t<80 && screen==SCREEN_PLAY && !lost && !won;++t) input(0);
    }
    fprintf(stderr,"INCOMPLETE tick=%u frame=%d screen=%d won=%d lost=%d bank=%d,%d\n",host_tick,frame,screen,won,lost,minerals,gas);fclose(trace);return 11;
}
