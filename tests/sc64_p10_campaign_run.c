/* Ordinary-input P10 "Eye of the Storm" campaign controller, built from the P8
 * one. Three of the four Zerg attack towns are nearer the Protoss base and Raynor's
 * base sits on far more minerals, so it grows Raynor's base and sends Marines at
 * the Overmind. The Gantrithor flies to Raynor's base and Zeratul, cloaked, waits
 * in the far corner: losing either loses the mission.
 * The controller may inspect live state to choose targets; every mutation is
 * an ordinary HOME/menu/gameplay input delivered through game_update(). */
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
#ifdef P10_POOL_AUDIT
/* Optional pool-pressure audit (stderr only; the trace is unchanged). */
static int audit_pool_peak,audit_kids_peak,audit_outside_peak,audit_shots;
static unsigned audit_outside_updates;
#endif

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
#ifdef P10_POOL_AUDIT
    {
        int n=0,kids=0,out=0;
        for(int i=0;i<MAX_UNITS;i++){
            n+=u[i].alive!=0;
            if(u[i].alive&&u[i].type==U_P7_INTERCEPTOR){
                ++kids;out+=sc64_p7_carrier_child_state(&u[i])==SC64_P7_INTERCEPTOR_OUTSIDE;
            }
        }
        if(n>audit_pool_peak)audit_pool_peak=n;
        if(kids>audit_kids_peak)audit_kids_peak=kids;
        if(out>audit_outside_peak)audit_outside_peak=out;
        audit_outside_updates+=out!=0;
        audit_shots+=sc64_pool_active_head(&race_p7_projectiles)!=SC64_END;
    }
#endif
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
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_COMMAND)return i;
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
        if(!u[w].alive || u[w].owner!=SIDE || u[w].type!=U_SCV ||
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
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV;return n;
}

static int army_count(void)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&!IS_WORKER(u[i].type)&&
        u[i].raw_type!=28&&u[i].raw_type!=78&&u[i].raw_type!=72&&u[i].raw_type!=73;
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
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&
        u[i].order==ORDER_MINE&&!u[i].carry)return i;
    /* An idle SCV holding cargo cannot be sent back to minerals afterward. */
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&
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
    if(home<0||count_type(B_REFINERY,0))return 0;
    for(int g=0;g<geyser_count;g++){
        int d=dist2(geysers[g].x,geysers[g].y,b[home].x,b[home].y);
        if(d<bd){bd=d;best=g;}
    }
    if(best<0||bd>320*320)return 0;
    return build_structure(B_REFINERY,MENU_REFINERY,0,0,geysers[best].x,geysers[best].y);
}

/* A builder that cannot reach its site stays pending; after 900 frames send
 * it back to the nearest mineral patch and never try that site again. */
static int pending_since[MAX_UNITS];
static int remine_stuck_builder(void)
{
    int worker=-1,home=home_center(),patch=-1,best=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++){
        if(!(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&SC64_PENDING_BUILD_ORDER(u[i].order))){
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
        b[i].type==B_REFINERY&&building_complete(i)){refinery=i;break;}
    for(int i=0;i<MAX_UNITS&&worker<0;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&
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
        b[i].type==B_REFINERY&&building_complete(i)){refinery=i;break;}
    if(refinery<0)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV){
        if(u[i].order==ORDER_GAS)++gas_workers;
        else if(worker<0&&u[i].order==ORDER_MINE&&!u[i].carry)worker=i;
    }
    /* Gas beyond what Firebats can spend costs mineral income: STOP an SCV and
     * let the mining pass send it to a patch. */
    if(gas>=P10_GAS_BANK){
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&
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

int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:".local/traces/p10-controller.jsonl";
    unsigned limit_ticks=argc>2?(unsigned)strtoul(argv[2],0,10):P10_RUN_LIMIT;
    trace=fopen(path,"w");if(!trace){perror(path);return 2;}
    fprintf(trace,"{\"kind\":\"metadata\",\"schema\":1,\"mission\":29,\"profile\":\"%s\",\"input_policy\":\"ordinary game_update held/pressed only\"}\n",P10_CONTROLLER_PROFILE);
    game_init();prior=0;
    assert(screen==SCREEN_TITLE&&race_front.page==SC_FRONT_HOME);
    /* Campaign, page 5 of the two-entry mission list, second row, briefing. */
    KEY_TAP(KEY_A);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);
    KEY_TAP(KEY_DOWN);KEY_TAP(KEY_A);KEY_TAP(KEY_START);
    assert(screen==SCREEN_PLAY&&!won&&!lost&&race_selected_mission==29);
    census("launch");
    for(int i=0;i<MAX_UNITS;i++)pending_since[i]=-1;

    /* Opening at the Protoss base, before the cursor leaves for Raynor's: mine,
     * hide Zeratul, and start the Gantrithor up the east edge. */
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&
        u[i].order==ORDER_IDLE&&select_workers_at(i)){
        int patch=-1,best=0x7fffffff;
        for(int k=0;k<MAX_MINERALS;k++)if(m[k].amount>=1000&&currently_visible(m[k].x,m[k].y)){
            int d=dist2(u[i].x,u[i].y,m[k].x,m[k].y);
            if(d<best){best=d;patch=k;}
        }
        if(patch<0)break;
        move_cursor(m[patch].x,m[patch].y);KEY_TAP(KEY_A);
    }
    {
        int z=hero(75),g=hero(82);
        if(z>=0)(void)hero_command(z,2000,2000);
        if(g>=0)(void)hero_command(g,2030,1700);
    }
    int saved=0;
    int next_checkpoint=2000;
    int next_mine=30;
    int next_probe=60,next_supply=400,next_build=600,next_gas=3000,next_production=900;
    int next_order=120,next_hero=240,next_save_build=0,next_evade=600;
    int attacking=0,prior_distance=-1,stalls=0;
    while(host_tick<limit_ticks&&screen==SCREEN_PLAY&&!won&&!lost){
        static int stuck_reported;
        if(menu_open&&!stuck_reported){
            stuck_reported=1;
            fprintf(stderr,"menu left open frame=%d mode=%d cursor=%d rows=%d sel=%d selb=%d\n",
                    frame,mode,menu_cursor,field_menu_count(),selected_count(),selected_building);
        }
        if(menu_open)KEY_TAP(KEY_B);
        int home=home_center();
        int f=hero(28),h=hero(82);
        /* Save while a structure is still warping in, so the record carries
         * active construction: start a Depot if nothing is, and fall back
         * after a bounded wait. */
        mineral_reserve=!saved&&frame>=P10_SAVE_FRAME-600?150:0;
        if(!saved&&frame>=P10_SAVE_FRAME&&!under_construction()&&frame>=next_save_build){
            (void)build_structure(B_DEPOT,MENU_DEPOT,64,448,-1,-1);
            next_save_build=frame+300;
        }
        if(!saved&&frame>=P10_SAVE_FRAME&&(under_construction()||frame>=P10_SAVE_FRAME+6000)){
            int building=under_construction();
            /* The building selection and the orders menu's remembered row
             * are UI state, not saved: a fresh process starts with neither.
             * Return the row to the top and deselect first, so the replayed
             * inputs mean the same thing before and after a load. */
            close_menus();
            /* The menu reopens on the last chosen command: choose MOVE
             * ORDER (row 0), then cancel its targeting with B. Try a free
             * SCV first, then any other unit that will select alone. */
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
            /* A pause-menu attempt must have committed an SRAM record. */
            int any_saved=0;
            for(unsigned i=0;i<sizeof(host_save);i++) any_saved |= host_save[i];
            assert(any_saved);
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
         * near the Command Center farthest from every enemy; shields and hit points do
         * not regenerate. */
        if(frame>=next_evade&&home>=0){
            int heroes[1]={f};
            for(int k=0;k<1;k++){
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
        if(frame>=next_mine){
            for(int k=0;k<3&&recover_loaded_probe();k++){}
            (void)order_idle_scvs_to_visible_minerals();
            next_mine=frame+300;
        }
        if(frame>=next_probe){(void)queue_probe();next_probe=frame+300;}
        if(frame>=next_supply){
            int pending=0;
            for(int i=0;i<MAX_BUILDINGS;i++)pending+=b[i].alive&&b[i].owner==SIDE&&
                b[i].type==B_DEPOT&&b[i].under_construction;
            for(int i=0;i<MAX_UNITS;i++)pending+=u[i].alive&&u[i].owner==SIDE&&
                SC64_PENDING_BUILD_ORDER(u[i].order)&&u[i].target==B_DEPOT;
            if(pending<2&&race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<=12){
                int ok=build_structure(B_DEPOT,MENU_DEPOT,64,448,-1,-1);
                if(getenv("P10_BLOG")&&!ok)fprintf(stderr,"pylon fail f=%d pending=%d min=%d builder=%d\n",frame,pending,minerals,free_builder());
            } else if(getenv("P10_BLOG")&&race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<=12)
                fprintf(stderr,"pylon wait f=%d pending=%d\n",frame,pending);
            next_supply=frame+300;
        }
        if(frame>=next_build){
            /* Raynor's base starts with a Barracks and an Engineering Bay. Gas
             * and an Academy unlock Firebats; a large bank buys more Barracks. */
            if(count_type(B_BARRACKS,0)<1)
                (void)build_structure(B_BARRACKS,MENU_BARRACKS,80,224,-1,-1);
            else if(probe_count()>=8&&!count_type(B_REFINERY,0))(void)build_assimilator();
            else if(probe_count()>=8&&!count_type(B_ACADEMY,0))
                (void)build_structure(B_ACADEMY,MENU_ACADEMY,80,224,-1,-1);
            else if(minerals>=200&&count_type(B_BARRACKS,0)<5)
                (void)build_structure(B_BARRACKS,MENU_BARRACKS,80,256,-1,-1);
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
        /* Walk the Gantrithor the long way round the Spore Colonies, one order
         * per leg: the cursor crosses the map each time. */
        if(frame>=next_hero){
            static const int fly_x[3]={2030,1000,420},fly_y[3]={260,260,240};
            static int fly_stage;
            if(h>=0&&u[h].owner==SIDE&&fly_stage<3&&u[h].order!=ORDER_MOVE){
                (void)hero_command(h,fly_x[fly_stage],fly_y[fly_stage]);
                ++fly_stage;
            } else if(fly_stage>=3&&h>=0&&u[h].owner==SIDE&&home>=0){
                /* The Gantrithor fights through its Interceptors, which hit
                 * ground and air alike (Guardians cannot touch the ship):
                 * meet a raid near the base with an attack order, then go
                 * home. It stops to launch once the target is in range. */
                int rt=-1,rd=420*420;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY){
                    int d=dist2(u[i].x,u[i].y,b[home].x,b[home].y);
                    if(u[i].order==ORDER_IDLE&&d>=288*288)continue;
                    if(d<rd){rd=d;rt=i;}
                }
                if(rt>=0){
                    if(!(u[h].order==ORDER_ATTACK_MOVE&&dist2(u[h].tx,u[h].ty,u[rt].x,u[rt].y)<96*96))
                        (void)unit_order(h,MENU_ATTACK,u[rt].x,u[rt].y);
                } else if(u[h].order!=ORDER_MOVE&&dist2(u[h].x,u[h].y,b[home].x,b[home].y)>96*96)
                    (void)hero_command(h,b[home].x,b[home].y-24);
            }
            if(f>=0&&u[f].owner==SIDE&&home>=0&&u[f].order!=ORDER_MOVE&&
               dist2(u[f].x,u[f].y,b[home].x,b[home].y)>96*96)
                (void)hero_command(f,b[home].x,b[home].y+48);
            next_hero=frame+180;
        }
        if(frame>=next_order&&home>=0){
            /* Guardians outrange idle Marines and shell them unanswered, so meet
             * anything that comes this close. */
            int threat=-1,bd=520*520;
            /* Idle guards in the nearest town are not a threat until close. */
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY){
                int d=dist2(u[i].x,u[i].y,b[home].x,b[home].y);
                if(u[i].order==ORDER_IDLE&&d>=288*288)continue;
                if(d<bd){bd=d;threat=i;}
            }
            /* A raid on the base recalls an attacking army. */
            int raiders=0;
            for(int i=0;i<MAX_UNITS;i++)raiders+=u[i].alive&&u[i].owner==ENEMY&&
                dist2(u[i].x,u[i].y,b[home].x,b[home].y)<320*320;
            if(raiders>=2)attacking=0;
            /* Attack orders chase raiders into their towns, where the army dies
             * a few at a time. Walk strays home before anything else. */
            int strays=0;
            for(int i=0;i<MAX_UNITS;i++)strays+=u[i].alive&&u[i].owner==SIDE&&
                (u[i].type==U_MARINE||u[i].type==U_FIREBAT)&&u[i].order!=ORDER_MOVE&&
                dist2(u[i].x,u[i].y,b[home].x,b[home].y)>440*440;
            if(strays&&!attacking){
                (void)army_command(MENU_MOVE,b[home].x+64,b[home].y+64);
                next_order=frame+120;
            } else if(threat>=0&&!attacking){
                (void)army_command(MENU_ATTACK,u[threat].x,u[threat].y);
                fprintf(trace,"{\"kind\":\"defend\",\"frame\":%u,\"target\":[%d,%d]}\n",
                    (unsigned)frame,u[threat].x,u[threat].y);
                next_order=frame+120;
            }
        }
        if(frame>=next_order){
            int cell=overmind();
            /* Attack at 40 units, or with what there is once the bank can no
             * longer grow the army (the start base mines out). */
            if(!attacking&&(army_count()>=P10_ATTACK_ARMY||
               (army_count()>=20&&frame>=P10_SAVE_FRAME&&minerals<100)))attacking=1;
            if(attacking&&cell>=0){
                /* Route from the army unit nearest the Overmind; new units at
                 * home would skew an average. A unit in a building pocket has
                 * no grid route, so try the next nearest ones. */
                static u8 tried[MAX_UNITS],stuck[MAX_UNITS];
                static int last_front=-1;
                /* A front unit whose route stopped shrinking is stuck (for
                 * example behind a cliff); route from another one. */
                if(stalls>=3&&last_front>=0){stuck[last_front]=1;stalls=0;prior_distance=-1;}
                int sx=0,sy=0,n=0,front=-1;
                for(int i=0;i<MAX_UNITS;i++){
                    tried[i]=stuck[i]&&u[i].alive;
                    if(!u[i].alive)stuck[i]=0;
                }
                front=nearest_by_route(b[cell].x,b[cell].y,tried);
                int wx=b[cell].x,wy=b[cell].y,distance=-1;
                for(int k=0;k<8&&front>=0;k++){
                    if(waypoint_to(u[front].x,u[front].y,b[cell].x,b[cell].y,&wx,&wy,&distance))break;
                    tried[front]=1;distance=-1;
                    int next=-1,nd=0x7fffffff;
                    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!tried[i]&&
                        (u[i].type==U_MARINE||u[i].type==U_FIREBAT)){
                        int d=dist2(u[i].x,u[i].y,b[cell].x,b[cell].y);
                        if(d<nd){nd=d;next=i;}
                    }
                    front=next;
                }
                if(distance<0){wx=b[cell].x;wy=b[cell].y;}
                last_front=front;
                if(front>=0)for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
                    (u[i].type==U_MARINE||u[i].type==U_FIREBAT)&&
                    dist2(u[i].x,u[i].y,u[front].x,u[front].y)<200*200){sx+=u[i].x;sy+=u[i].y;++n;}
                /* Near the target, strike any Arbiter first. */
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
                /* At the Overmind, give every group a fresh order on it; stale
                 * attack orders would otherwise stand. */
                army_force=distance<0||distance<12;
                if(distance>=0&&distance<12){
                    army_only=U_MARINE;
                    (void)army_command(MENU_ATTACK,b[cell].x,b[cell].y);
                    army_only=U_FIREBAT;
                    (void)army_command(MENU_ATTACK,arb>=0?u[arb].x:b[cell].x,arb>=0?u[arb].y:b[cell].y);
                    army_only=0;
                } else {
                    if(arb>=0){wx=u[arb].x;wy=u[arb].y;}
                    (void)army_command(MENU_ATTACK,wx,wy);
                }
                army_force=0;
                fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"route\":%d,\"army\":%d,\"stalls\":%d}\n",
                    (unsigned)frame,wx,wy,distance,army_count(),stalls);
                /* The whole-army order also moved the heroes; send them home. */
                next_hero=frame+30;
                if(army_count()<8)attacking=0;
            }
            next_order=frame+(attacking?360:600);
        }
        if(frame>=next_checkpoint){census("progress");next_checkpoint=frame+2000;}
        input(0);
    }
    census("final");
#ifdef P10_POOL_AUDIT
    fprintf(stderr,"p10 pool audit: peak live units %d of %d, Interceptors peak %d, outside peak %d, "
            "updates with Interceptors outside %u, updates with a shot in flight %d\n",
            audit_pool_peak,MAX_UNITS,audit_kids_peak,audit_outside_peak,audit_outside_updates,audit_shots);
#endif
    fclose(trace);
    printf("p10 controller: frame=%u calls=%u won=%d lost=%d screen=%d\n",
           (unsigned)frame,host_tick,won,lost,screen);
    return won?0:1;
}
