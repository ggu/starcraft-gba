/* Ordinary-input P8 "The Trial of Tassadar" campaign controller.
 * The controller may inspect live state to choose targets; every mutation is
 * an ordinary HOME/menu/gameplay input delivered through game_update(). */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef P8_CONTROLLER_PROFILE
#define P8_CONTROLLER_PROFILE "PROTOSS_P8_FLAGS"
#endif
#ifndef P8_RUN_LIMIT
#define P8_RUN_LIMIT 160000u
#endif
/* One ordinary pause-menu save; the replay tools resume from this record. */
#ifndef P8_SAVE_FRAME
#define P8_SAVE_FRAME 30000
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

static int stasis_cell(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==168)return i;
    return -1;
}

/* The human Carrier (source slot 7) that the shared services serve. */
static int human_carrier(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==72&&
        u[i].raw_owner==7)return i;
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

/* Order the army by type groups: a double tap on a Zealot or Dragoon
 * selects that type on screen, which never includes Fenix, the Hyperion or
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
            (u[i].type==U_ZEALOT||u[i].type==U_DRAGOON)&&(!army_only||u[i].type==army_only)){
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
            if(u[i].type!=U_ZEALOT&&u[i].type!=U_DRAGOON)bad=1;
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

/* As in sc64_visible_worker_orders.h, issue mining through normal field
 * input only for idle empty workers and currently visible mineral patches. */
static int order_idle_probes_to_visible_minerals(void)
{
    static const int dx[]={0,8,-8,0,0,8,-8,8,-8};
    static const int dy[]={0,0,0,8,-8,8,8,-8,-8};
    int assigned=0;
    for(int w=0;w<MAX_UNITS && screen==SCREEN_PLAY;w++) {
        if(!u[w].alive || u[w].owner!=SIDE || u[w].type!=U_PROBE ||
           u[w].order!=ORDER_IDLE || u[w].carry || !select_single(w)) continue;
        int target=-1,best=0x7fffffff;
        for(int p=0;p<MAX_MINERALS;p++) if(m[p].amount>0 && currently_visible(m[p].x,m[p].y)) {
            int d=dist2(u[w].x,u[w].y,m[p].x,m[p].y);
            if(d<best){best=d;target=p;}
        }
        if(target<0) continue;
        for(unsigned k=0;k<sizeof(dx)/sizeof(dx[0]);k++) {
            int x=m[target].x+dx[k],y=m[target].y+dy[k];
            move_cursor(x,y);
            if(abs(cx-x)>2 || abs(cy-y)>2 || !u[w].alive || !u[w].selected)continue;
            if(field_ui_a_action(selected_count())!=UI_A_MINE)continue;
            KEY_TAP(KEY_A);
            if(u[w].alive && u[w].order==ORDER_MINE && u[w].target==target)assigned++;
            break;
        }
    }
    return assigned;
}

static int unit_order(int slot,int which,int x,int y)
{ return select_single(slot)&&command_selected(which,x,y); }

static int hero_command(int slot,int x,int y)
{ return unit_order(slot,MENU_MOVE,x,y); }

/* The human Carrier's BUILD INTERCEPTOR row (shared Protoss service). */
static int train_interceptors(int carrier,int wanted)
{
    int queued=0;
    for(int k=0;k<wanted&&minerals>=SC64_P7_INTERCEPTOR_COST;k++){
        if(!select_single(carrier))break;
        /* A paid order can start training at once, moving from the
         * parent's queue into its hangar count. */
        int before=u[carrier].carry+(int)race_p7_carrier_training_child_count(carrier);
        KEY_TAP(KEY_B);
        for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=MENU_STOP+1;n++)KEY_TAP(KEY_DOWN);
        if(!menu_open||menu_cursor!=MENU_STOP+1){close_menus();break;}
        KEY_TAP(KEY_A);
        close_menus();
        if(u[carrier].carry+(int)race_p7_carrier_training_child_count(carrier)<=before)break;
        ++queued;
    }
    if(queued)fprintf(trace,"{\"kind\":\"interceptors\",\"frame\":%u,\"queued\":%d,\"hangar\":%u}\n",
        (unsigned)frame,queued,race_p7_carrier_training_child_count(carrier));
    return queued;
}

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

/* Minerals production leaves unspent, so the save-time Pylon can start. */
static int mineral_reserve;

static int queue_combat_unit(void)
{
    int wanted=count_type(B_CYBERNETICS_CORE,1)&&gas>=50&&minerals>=125?U_DRAGOON:U_ZEALOT;
    const Sc64RaceUnitRule *rule=sc64_race_unit_rule(wanted);
    if(!rule||minerals<(int)rule->minerals+mineral_reserve||gas<(int)rule->gas||
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

static int free_builder(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&
        u[i].order==ORDER_MINE&&!u[i].carry)return i;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&
        !SC64_BUILD_WORKER_ORDER(u[i].order)&&u[i].order!=ORDER_GAS)return i;
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

/* Place `type` through the Probe build menu at the nearest legal site. */
static int build_structure(int type,int menu_row,int min_r,int max_r,int fixed_x,int fixed_y)
{
    int home=home_nexus(),best=0x7fffffff,site_x=fixed_x,site_y=fixed_y;
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

/* A Photon Cannon through the Probe menu's CANNON row (MENU_STOP+2): it
 * defends the base and detects cloaked raiders. */
static int build_cannon(int near_x,int near_y)
{
    int best=0x7fffffff,site_x=-1,site_y=-1,worker=free_builder();
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(B_PHOTON_CANNON);
    if(!rule||minerals<(int)rule->minerals||worker<0)return 0;
    for(int dy=-96;dy<=96;dy+=8)for(int dx=-96;dx<=96;dx+=8){
        int x=near_x+dx,y=near_y+dy,d=dx*dx+dy*dy;
        if(d<32*32||d>=best||x<48||x>=WORLD_W-48||y<48||y>=WORLD_H-48||
           !race_can_site(SIDE,B_PHOTON_CANNON,x,y)||!can_place(B_PHOTON_CANNON,x,y)||
           !site_ok(B_PHOTON_CANNON,x,y))continue;
        best=d;site_x=x;site_y=y;
    }
    if(site_x<0)return 0;
    close_menus();clear_selected_building();
    move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    for(int n=0;n<40&&(!u[worker].selected||selected_count()!=1);n++)KEY_TAP(KEY_L);
    if(!u[worker].selected||selected_count()!=1)return 0;
    KEY_TAP(KEY_B);
    for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=MENU_STOP+2;n++)KEY_TAP(KEY_DOWN);
    if(!menu_open||menu_cursor!=MENU_STOP+2){close_menus();return 0;}
    KEY_TAP(KEY_A);
    if(mode!=MODE_BUILD||menu!=B_PHOTON_CANNON){close_menus();return 0;}
    move_cursor(site_x,site_y);KEY_TAP(KEY_A);
    if(mode!=MODE_NORMAL)close_menus();
    if(!SC64_BUILD_WORKER_ORDER(u[worker].order)){
        if(refused_count<64){refused_x[refused_count]=site_x;refused_y[refused_count++]=site_y;}
        return 0;
    }
    fprintf(trace,"{\"kind\":\"build\",\"frame\":%u,\"type\":%d,\"site\":[%d,%d]}\n",(unsigned)frame,B_PHOTON_CANNON,site_x,site_y);
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

/* A builder that cannot reach its site stays pending; after 900 frames send
 * it back to the nearest mineral patch and never try that site again. */
static int pending_since[MAX_UNITS];
static int remine_stuck_builder(void)
{
    int worker=-1,home=home_nexus(),patch=-1,best=0x7fffffff;
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
    for(int n=0;n<40&&(!u[worker].selected||selected_count()!=1);n++)KEY_TAP(KEY_L);
    if(!u[worker].selected||selected_count()!=1)return 0;
    move_cursor(b[refinery].x,b[refinery].y);KEY_TAP(KEY_A);
    return u[worker].order==ORDER_GAS;
}

static int launch_peak;  /* Most human Interceptors outside at once. */
static void census(const char *label)
{
    int cell=stasis_cell(),f=hero(78),h=hero(28),c=human_carrier();
    int enemy=0,enemy_b=0,cloaked=0,launched=0,battery_energy=0;
    /* Shared Protoss service evidence: the human Carrier's hangar and
     * launched Interceptors, cloaked enemies, and Battery energy. */
    for(int i=0;i<MAX_UNITS;i++){
        if(u[i].alive&&u[i].owner==ENEMY&&sc64_p7_observer_target_is_cloaked(i))++cloaked;
        if(u[i].alive&&c>=0&&u[i].raw_type==73&&u[i].build==c&&
           sc64_p7_carrier_child_state(&u[i])==SC64_P7_INTERCEPTOR_OUTSIDE)++launched;
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].type==B_P7_SHIELD_BATTERY)
        battery_energy+=b[i].queue_progress/256;
    for(int i=0;i<MAX_UNITS;i++)enemy+=u[i].alive&&u[i].owner==ENEMY;
    for(int i=0;i<MAX_BUILDINGS;i++)enemy_b+=b[i].alive&&b[i].owner==ENEMY;
    fprintf(trace,"{\"kind\":\"checkpoint\",\"label\":\"%s\",\"frame\":%u,\"call_index\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,"
        "\"side_units\":%d,\"army\":%d,\"probes\":%d,\"enemy_units\":%d,\"enemy_buildings\":%d,"
        "\"stasis_hp\":%d,\"fenix\":[%d,%d,%d],\"hyperion\":[%d,%d,%d],\"bank\":[%d,%d],\"supply\":[%d,%d],"
        "\"gateways\":%d,\"cursor\":[%d,%d],\"triggers_done\":[%u,%u,%u],\"outcome\":%u,\"fault\":[%u,%u,%u],"
        "\"towns\":[%u,%u,%u],\"waves\":[%u,%u],\"ai_wallets\":[%u,%u,%u,%u],"
        "\"carrier\":[%d,%u,%d,%d],\"cloaked_enemies\":%d,\"battery_energy\":%d}\n",
        label,(unsigned)frame,host_tick,screen,won,lost,
        live_units_owner(SIDE),army_count(),probe_count(),enemy,enemy_b,
        cell>=0?b[cell].hp:-1,f>=0?u[f].owner:-1,f>=0?u[f].hp:-1,f>=0?u[f].shield:-1,
        h>=0?u[h].owner:-1,h>=0?u[h].hp:-1,h>=0?u[h].shield:-1,minerals,gas,
        race_owner_supply(SIDE),race_owner_supply_cap(SIDE),count_type(B_GATEWAY,0),cx,cy,
        race_p8.runtime[0].done,race_p8.runtime[1].done,race_p8.runtime[2].done,
        race_p8.runtime[0].outcome,race_p8.runtime[0].fault,race_p8.runtime[1].fault,race_p8.runtime[2].fault,
        race_p8.towns[0].script,race_p8.towns[1].script,race_p8.towns[2].script,
        race_p8.towns[0].wave,race_p8.towns[1].wave,
        race_p8.ai_minerals[0],race_p8.ai_gas[0],race_p8.ai_minerals[1],race_p8.ai_gas[1],
        c>=0?u[c].hp:-1,c>=0?race_p7_carrier_training_child_count(c):0u,launched,launch_peak,
        cloaked,battery_energy);
    fflush(trace);
}

int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:".local/traces/p8-controller.jsonl";
    unsigned limit_ticks=argc>2?(unsigned)strtoul(argv[2],0,10):P8_RUN_LIMIT;
    trace=fopen(path,"w");if(!trace){perror(path);return 2;}
    fprintf(trace,"{\"kind\":\"metadata\",\"schema\":1,\"mission\":27,\"profile\":\"%s\",\"input_policy\":\"ordinary game_update held/pressed only\"}\n",P8_CONTROLLER_PROFILE);
    game_init();prior=0;
    assert(screen==SCREEN_TITLE&&race_front.page==SC_FRONT_HOME);
    /* Campaign, page 4 of the two-entry mission list, second row, briefing. */
    KEY_TAP(KEY_A);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);
    KEY_TAP(KEY_DOWN);KEY_TAP(KEY_A);KEY_TAP(KEY_START);
    assert(screen==SCREEN_PLAY&&!won&&!lost&&race_selected_mission==27);
    census("launch");
    for(int i=0;i<MAX_UNITS;i++)pending_since[i]=-1;

    int saved=0;
    int next_checkpoint=2000;
    int next_mine=30;
    int next_probe=60,next_supply=400,next_build=600,next_gas=3000,next_production=900;
    int next_order=120,next_hero=240,next_carrier=3000,next_save_build=0,next_evade=600;
    int attacking=0,prior_distance=-1,stalls=0;
    while(host_tick<limit_ticks&&screen==SCREEN_PLAY&&!won&&!lost){
        static int stuck_reported;
        if(menu_open&&!stuck_reported){
            stuck_reported=1;
            fprintf(stderr,"menu left open frame=%d mode=%d cursor=%d rows=%d sel=%d selb=%d\n",
                    frame,mode,menu_cursor,field_menu_count(),selected_count(),selected_building);
        }
        if(menu_open)KEY_TAP(KEY_B);
        int home=home_nexus();
        int f=hero(78),h=hero(28);
        /* Save while a structure is still warping in, so the record carries
         * active construction: start a Pylon if nothing is, and fall back
         * after a bounded wait. */
        mineral_reserve=!saved&&frame>=P8_SAVE_FRAME-600?150:0;
        if(!saved&&frame>=P8_SAVE_FRAME&&!under_construction()&&frame>=next_save_build){
            (void)build_structure(B_PYLON,MENU_DEPOT,64,448,-1,-1);
            next_save_build=frame+300;
        }
        if(!saved&&frame>=P8_SAVE_FRAME&&(under_construction()||frame>=P8_SAVE_FRAME+6000)){
            int building=under_construction();
            /* The building selection and the orders menu's remembered row
             * are UI state, not saved: a fresh process starts with neither.
             * Return the row to the top and deselect first, so the replayed
             * inputs mean the same thing before and after a load. */
            close_menus();
            /* The menu reopens on the last chosen command: choose MOVE
             * ORDER (row 0), then cancel its targeting with B. Try a free
             * Probe first, then any other unit that will select alone. */
            for(int i=-1;i<MAX_UNITS&&menu_last!=0;i++){
                int pick=i<0?free_builder():i;
                if(pick<0||(i>=0&&pick==free_builder())||!select_single(pick))continue;
                KEY_TAP(KEY_B);
                for(int n=0;n<16&&menu_open&&menu_cursor!=0;n++)KEY_TAP(KEY_UP);
                if(menu_open&&menu_cursor==0)KEY_TAP(KEY_A);
                close_menus();
            }
            clear_selected_building();
            /* B from a targeting mode reopens the orders menu; an open menu
             * holds the game and is not saved, so close everything first. */
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
        /* A hero with an enemy in Scarab range steps to the walkable spot
         * near the Nexus farthest from every enemy; Protoss hit points do
         * not regenerate. */
        if(frame>=next_evade&&home>=0){
            int heroes[2]={f,h};
            for(int k=0;k<2;k++){
                int z=heroes[k];
                if(z<0||u[z].owner!=SIDE||enemy_clearance(u[z].x,u[z].y)>=288)continue;
                int ex=-1,ey=-1,best=-1;
                for(int r=96;r<=224;r+=64)for(int a=0;a<16;a++){
                    static const int ox[16]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
                    static const int oy[16]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
                    int x=limit(b[home].x+ox[a]*r/16,48,WORLD_W-48),y=limit(b[home].y+oy[a]*r/16,48,WORLD_H-48);
                    int cell=limit(x/16,0,MAP_COLS-1)+limit(y/16,0,MAP_ROWS-1)*MAP_COLS,wx,wy,distance=-1;
                    if(!path_grid_valid||!SC64_GRID_GET(path_walkable,cell))continue;
                    /* Score the walk, not just the destination: the
                     * nearest any enemy comes to the straight path once
                     * the hero is under way. */
                    int clear=1024;
                    for(int t=2;t<=8;t++){
                        int c=enemy_clearance(u[z].x+(x-u[z].x)*t/8,u[z].y+(y-u[z].y)*t/8);
                        if(c<clear)clear=c;
                    }
                    if(clear<=best)continue;
                    if(!waypoint_to(u[z].x,u[z].y,x,y,&wx,&wy,&distance)||distance>32)continue;
                    best=clear;ex=x;ey=y;
                }
                if(best<=enemy_clearance(u[z].x,u[z].y))ex=-1;
                if(ex>=0&&dist2(u[z].tx,u[z].ty,ex,ey)>48*48)(void)hero_command(z,ex,ey);
                next_hero=frame+180;
            }
            next_evade=frame+150;
        }
        if(frame>=next_mine){(void)order_idle_probes_to_visible_minerals();next_mine=frame+300;}
        if(frame>=next_probe){(void)queue_probe();next_probe=frame+300;}
        if(frame>=next_supply){
            int pending=0;
            for(int i=0;i<MAX_BUILDINGS;i++)pending+=b[i].alive&&b[i].owner==SIDE&&
                b[i].type==B_PYLON&&b[i].under_construction;
            for(int i=0;i<MAX_UNITS;i++)pending+=u[i].alive&&u[i].owner==SIDE&&
                SC64_PENDING_BUILD_ORDER(u[i].order)&&u[i].target==B_PYLON;
            if(pending<2&&race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<=12){
                int ok=build_structure(B_PYLON,MENU_DEPOT,64,448,-1,-1);
                if(getenv("P8_BLOG")&&!ok)fprintf(stderr,"pylon fail f=%d pending=%d min=%d builder=%d\n",frame,pending,minerals,free_builder());
            } else if(getenv("P8_BLOG")&&race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<=12)
                fprintf(stderr,"pylon wait f=%d pending=%d\n",frame,pending);
            next_supply=frame+300;
        }
        if(frame>=next_build){
            /* Cannons cover the mineral line south of the Nexus, where raids
             * arrive from frame 9,000, then the heroes' station: the Forge
             * and four Cannons come right after the first Gateway, then gas
             * and the Cybernetics Core, and a large bank buys more. */
            static const int ax[6]={-40,40,-104,104,-120,-40},ay[6]={160,160,112,112,-80,-80};
            int cannons=count_type(B_PHOTON_CANNON,0);
            if(count_type(B_GATEWAY,0)<1)
                (void)build_structure(B_GATEWAY,MENU_BARRACKS,80,224,-1,-1);
            else if(!count_type(B_FORGE,0))
                (void)build_structure(B_FORGE,MENU_ENG_BAY,80,224,-1,-1);
            else if(count_type(B_FORGE,1)&&home>=0&&cannons<4)
                (void)build_cannon(b[home].x+ax[cannons%6],b[home].y+ay[cannons%6]);
            else if(!count_type(B_ASSIMILATOR,0))(void)build_assimilator();
            else if(!count_type(B_CYBERNETICS_CORE,0)&&count_type(B_GATEWAY,1))
                (void)build_structure(B_CYBERNETICS_CORE,MENU_ACADEMY,80,224,-1,-1);
            else if(probe_count()>=10&&count_type(B_GATEWAY,0)<6)
                (void)build_structure(B_GATEWAY,MENU_BARRACKS,80,224,-1,-1);
            else if(count_type(B_FORGE,1)&&home>=0&&minerals>=500&&cannons<12)
                (void)build_cannon(b[home].x+ax[cannons%6],b[home].y+ay[cannons%6]);
            next_build=frame+(minerals>=400?240:600);
        }
        if(frame>=next_gas){
            (void)assign_gas_worker();
            for(int k=0;k<4&&remine_stuck_builder();k++){}
            next_gas=frame+700;
        }
        if(frame>=next_production){
            int queued=0;
            while(queued<6&&queue_combat_unit())++queued;
            next_production=frame+60;
        }
        /* Fill the Carrier's hangar through its BUILD INTERCEPTOR row. */
        if(frame>=next_carrier){
            int c=human_carrier();
            if(c>=0&&home>=0&&minerals>=SC64_P7_INTERCEPTOR_COST){
                int have=(int)race_p7_carrier_training_child_count(c)+u[c].carry;
                if(have<SC64_P7_CARRIER_CAPACITY)
                    (void)train_interceptors(c,SC64_P7_CARRIER_CAPACITY-have);
            }
            next_carrier=frame+1200;
        }
        /* Collect the rescuable heroes, then keep them at home. */
        if(frame>=next_hero){
            if(f>=0&&u[f].owner==RESCUABLE)
                (void)army_command(MENU_MOVE,u[f].x,u[f].y);
            else if(home>=0){
                int fx=b[home].x,fy=b[home].y+48;
                if(f>=0&&u[f].owner==SIDE&&dist2(u[f].x,u[f].y,fx,fy)>48*48)
                    (void)hero_command(f,fx,fy);
                if(h>=0&&u[h].owner==SIDE&&dist2(u[h].x,u[h].y,b[home].x,b[home].y)>96*96)
                    (void)hero_command(h,b[home].x,b[home].y+24);
            }
            next_hero=frame+180;
        }
        if(frame>=next_order&&home>=0){
            int threat=-1,bd=360*360;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY){
                int d=dist2(u[i].x,u[i].y,b[home].x,b[home].y);
                if(d<bd){bd=d;threat=i;}
            }
            /* A raid on the base recalls an attacking army. */
            int raiders=0;
            for(int i=0;i<MAX_UNITS;i++)raiders+=u[i].alive&&u[i].owner==ENEMY&&
                dist2(u[i].x,u[i].y,b[home].x,b[home].y)<320*320;
            if(raiders>=2)attacking=0;
            if(threat>=0&&!attacking){
                (void)army_command(MENU_ATTACK,u[threat].x,u[threat].y);
                fprintf(trace,"{\"kind\":\"defend\",\"frame\":%u,\"target\":[%d,%d]}\n",
                    (unsigned)frame,u[threat].x,u[threat].y);
                next_hero=frame+30;
                next_order=frame+180;
            }
        }
        if(frame>=next_order){
            int cell=stasis_cell();
            /* Attack at 40 units, or with what there is once the bank can no
             * longer grow the army (the start base mines out). */
            if(!attacking&&(army_count()>=40||
               (army_count()>=20&&frame>=P8_SAVE_FRAME&&minerals<100)))attacking=1;
            if(attacking&&cell>=0){
                /* Route from the army unit nearest the Cell; new units at
                 * home would skew an average. A unit in a building pocket has
                 * no grid route, so try the next nearest ones. */
                static u8 tried[MAX_UNITS],stuck[MAX_UNITS];
                static int last_front=-1;
                /* A front unit whose route stopped shrinking is stuck (for
                 * example behind a cliff); route from another one. */
                if(stalls>=3&&last_front>=0){stuck[last_front]=1;stalls=0;prior_distance=-1;}
                int sx=0,sy=0,n=0,front=-1,fd=0x7fffffff;
                for(int i=0;i<MAX_UNITS;i++){
                    tried[i]=stuck[i]&&u[i].alive;
                    if(!u[i].alive)stuck[i]=0;
                    if(!u[i].alive||u[i].owner!=SIDE||tried[i]||(u[i].type!=U_ZEALOT&&u[i].type!=U_DRAGOON))continue;
                    int d=dist2(u[i].x,u[i].y,b[cell].x,b[cell].y);
                    if(d<fd){fd=d;front=i;}
                }
                int wx=b[cell].x,wy=b[cell].y,distance=-1;
                for(int k=0;k<8&&front>=0;k++){
                    if(waypoint_to(u[front].x,u[front].y,b[cell].x,b[cell].y,&wx,&wy,&distance))break;
                    tried[front]=1;distance=-1;
                    int next=-1,nd=0x7fffffff;
                    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!tried[i]&&
                        (u[i].type==U_ZEALOT||u[i].type==U_DRAGOON)){
                        int d=dist2(u[i].x,u[i].y,b[cell].x,b[cell].y);
                        if(d<nd){nd=d;next=i;}
                    }
                    front=next;
                }
                if(distance<0){wx=b[cell].x;wy=b[cell].y;}
                last_front=front;
                if(front>=0)for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
                    (u[i].type==U_ZEALOT||u[i].type==U_DRAGOON)&&
                    dist2(u[i].x,u[i].y,u[front].x,u[front].y)<200*200){sx+=u[i].x;sy+=u[i].y;++n;}
                /* Near the Stasis Cell the army has no detector for the
                 * defenders its Arbiters cloak: Dragoons strike the Arbiters,
                 * Zealots the Cell itself (a structure is never cloaked). */
                int arb=-1;
                if(distance>=0&&distance<30&&n){
                    int ad=320*320;
                    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==71){
                        int d=dist2(u[i].x,u[i].y,sx/n,sy/n);
                        if(d<ad){ad=d;arb=i;}
                    }
                }
                /* Progress is measured against the best route seen, so a
                 * front that oscillates between two routes still stalls. */
                if(distance>=0&&prior_distance>=0&&distance+2>=prior_distance)++stalls;
                else{stalls=0;if(distance>=0)prior_distance=distance;}
                if(prior_distance<0)prior_distance=distance;
                /* At the Cell, give every group a fresh order on the Cell or
                 * its Arbiters; stale attack orders would otherwise stand. */
                army_force=distance<0||distance<12;
                if(distance>=0&&distance<12){
                    army_only=U_ZEALOT;
                    (void)army_command(MENU_ATTACK,b[cell].x,b[cell].y);
                    army_only=U_DRAGOON;
                    (void)army_command(MENU_ATTACK,arb>=0?u[arb].x:b[cell].x,arb>=0?u[arb].y:b[cell].y);
                    army_only=0;
                } else {
                    if(arb>=0){wx=u[arb].x;wy=u[arb].y;}
                    (void)army_command(MENU_ATTACK,wx,wy);
                }
                army_force=0;
                {
                    int c=human_carrier();
                    if(c>=0&&race_p7_carrier_training_child_count(c)>=2&&
                       !u[c].selected&&dist2(u[c].tx,u[c].ty,wx,wy)>96*96)
                        (void)unit_order(c,MENU_ATTACK,wx,wy);
                }
                fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"route\":%d,\"army\":%d,\"stalls\":%d}\n",
                    (unsigned)frame,wx,wy,distance,army_count(),stalls);
                /* The whole-army order also moved the heroes; send them home. */
                next_hero=frame+30;
                if(army_count()<8)attacking=0;
            }
            next_order=frame+(attacking?360:600);
        }
        {
            int c=human_carrier(),out=0;
            for(int i=0;c>=0&&i<MAX_UNITS;i++)out+=u[i].alive&&u[i].raw_type==73&&u[i].build==c&&
                sc64_p7_carrier_child_state(&u[i])==SC64_P7_INTERCEPTOR_OUTSIDE;
            if(out>launch_peak)launch_peak=out;
        }
        if(frame>=next_checkpoint){census("progress");next_checkpoint=frame+2000;}
        input(0);
    }
    census("final");
    fclose(trace);
    printf("state: screen=%d menu_open=%d mode=%d src_pause=%u sel=%d selb=%d help=%d toast=%d\n",screen,menu_open,mode,race_p8.source_pause_active,selected_count(),selected_building,help,toast_timer);
    printf("p8 controller: frame=%u calls=%u won=%d lost=%d screen=%d fault=%u/%u/%u\n",
           (unsigned)frame,host_tick,won,lost,screen,race_p8.runtime[0].fault,
           race_p8.runtime[1].fault,race_p8.runtime[2].fault);
    return won?0:1;
}
