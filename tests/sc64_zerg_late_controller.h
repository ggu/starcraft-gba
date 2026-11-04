/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary-input helpers shared by the Z8, Z9 and Z10 campaign controllers.
 * Include after starcraft/game.c. Live state only chooses inputs; every change
 * reaches the game as a held/pressed pair through game_update(). */
#ifndef SC64_ZERG_LATE_CONTROLLER_H
#define SC64_ZERG_LATE_CONTROLLER_H

#include <stdio.h>
#include <stdlib.h>

static FILE *trace;
static u16 prior;
static unsigned host_tick;

static void input(u16 held)
{
    /* Once the mission ends, later calls are not inputs: the trace stops at
     * the deciding call, so a replay ends there too. */
    if(screen==SCREEN_RESULT||won||lost)return;
    u16 pressed=(u16)(held&(u16)~prior);
    int changed=held!=prior;
    unsigned call_index=host_tick;
    unsigned frame_before=(unsigned)frame;
    game_update(held,pressed);
    prior=held;
    ++host_tick;
    if(trace&&changed)
        fprintf(trace,"{\"kind\":\"input\",\"call_index\":%u,\"frame_before\":%u,\"frame_after\":%u,\"held\":%u,\"pressed\":%u}\n",
            call_index,frame_before,(unsigned)frame,(unsigned)held,(unsigned)pressed);
}
#define KEY_TAP(k) do{input((k));input(0);}while(0)

static void launch_catalog_mission(int global_id)
{
    int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,global_id);
    KEY_TAP(KEY_A);
    for(int page=0;page<ordinal/2;page++)KEY_TAP(KEY_RIGHT);
    if(ordinal&1)KEY_TAP(KEY_DOWN);
    KEY_TAP(KEY_A);
    KEY_TAP(KEY_START);
}

static void move_cursor(int x,int y)
{
    unsigned guard=0;
    x=limit(x,0,WORLD_W-1);y=limit(y,0,WORLD_H-1);
    /* R holds the 4-pixel speed. Compare live: CenterView can move the cursor. */
    while((FAST_ABS(cx-x)>2||FAST_ABS(cy-y)>2)&&screen==SCREEN_PLAY&&!won&&!lost&&guard++<12000u){
        u16 held=KEY_R;
        if(cx<x-2)held|=KEY_RIGHT;else if(cx>x+2)held|=KEY_LEFT;
        if(cy<y-2)held|=KEY_DOWN;else if(cy>y+2)held|=KEY_UP;
        input(held);
    }
    input(0);
}

static void close_menus(void)
{
    for(int k=0;k<4&&(menu_open||mode!=MODE_NORMAL);k++)KEY_TAP(KEY_B);
}

/* A selected building stays selected until an L box closes elsewhere. */
static void clear_selected_building(void)
{
    if(selected_building<0)return;
    int x=b[selected_building].x+96,y=b[selected_building].y+96;
    x=limit(x,32,WORLD_W-64);y=limit(y,32,WORLD_H-64);
    move_cursor(x,y);input(KEY_L);
    unsigned guard=0;
    int ex=cx+16,ey=cy+16;
    while((cx<ex||cy<ey)&&guard++<64){
        u16 held=KEY_L;
        if(cx<ex)held|=KEY_RIGHT;
        if(cy<ey)held|=KEY_DOWN;
        input(held);
    }
    input(0);
}

static int choose_row(int row)
{
    if(!menu_open)KEY_TAP(KEY_B);
    int count=field_menu_count();
    if(!menu_open||row>=count){close_menus();return 0;}
    for(int n=0;n<count&&menu_open&&menu_cursor!=row;n++)KEY_TAP(KEY_DOWN);
    if(!menu_open||menu_cursor!=row){close_menus();return 0;}
    KEY_TAP(KEY_A);
    return 1;
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

static int select_building(int slot)
{
    if(slot<0||!b[slot].alive||b[slot].owner!=SIDE)return 0;
    close_menus();
    if(selected_building!=slot){
        clear_selected_building();
        move_cursor(b[slot].x,b[slot].y);KEY_TAP(KEY_A);
    }
    return selected_building==slot;
}

static int command_selected(int which,int x,int y)
{
    if(!choose_row(which))return 0;
    if(mode!=(which==MENU_ATTACK?MODE_ATTACK:MODE_MOVE)){close_menus();return 0;}
    move_cursor(x,y);KEY_TAP(KEY_A);
    return 1;
}

/* Attack one moving enemy: chase it with the cursor so A lands on it and the
 * selection takes a direct target instead of an attack-move to a stale point. */
static int attack_moving_unit(int enemy)
{
    if(!choose_row(MENU_ATTACK))return 0;
    if(mode!=MODE_ATTACK){close_menus();return 0;}
    for(unsigned guard=0;guard<80u&&u[enemy].alive;guard++){
        int dx=u[enemy].x-cx,dy=u[enemy].y-cy;
        if(FAST_ABS(dx)<=6&&FAST_ABS(dy)<=6)break;
        u16 held=KEY_R;
        if(dx>2)held|=KEY_RIGHT;else if(dx<-2)held|=KEY_LEFT;
        if(dy>2)held|=KEY_DOWN;else if(dy<-2)held|=KEY_UP;
        input(held);
    }
    KEY_TAP(KEY_A);
    if(mode!=MODE_NORMAL)close_menus();
    return 1;
}

static int unit_order(int slot,int which,int x,int y)
{ return select_single(slot)&&command_selected(which,x,y); }

static int count_units(int owner,int type)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==owner&&(type<0||u[i].type==type);return n;
}

static int count_buildings(int type,int complete_only)
{
    int n=0;for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&b[i].type==type&&
        (!complete_only||building_complete(i));
    return n;
}

static int under_construction(void)
{
    int n=0;for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&b[i].under_construction;return n;
}

static int queued_units(int type)
{
    int n=0;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE)
        for(int q=0;q<b[i].queue_count;q++)n+=b[i].queue[(b[i].queue_head+q)%5]==type;
    return n;
}

static int drone_count(void){return count_units(SIDE,U_DRONE);}

/* The late cargo-enabled profiles accept a loaded Drone's mineral order.
 * Keep the previously qualified Z8 controller unchanged. */
static int late_drone_mining_available(const Unit *q)
{
#if defined(SC64_WORKER_CARGO_KIND) && (defined(SC64_Z9_RUNTIME) || defined(SC64_Z10_RUNTIME))
    (void)q; return 1;
#else
    return !q->carry;
#endif
}

/* The selected idle Drone's slot plus one, or zero. */
static int idle_drone_selected(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].owner==SIDE&&
        u[i].type==U_DRONE&&u[i].order==ORDER_IDLE&&late_drone_mining_available(&u[i]))return i+1;
    return 0;
}

/* Drones with nothing to do, pointed at the closest visible field near a
 * finished Hatchery through the ordinary contextual A press. */
static int idle_reach; /* When set, leave idle Drones this far from the cursor for a later trip. */
static int order_idle_drones(int most)
{
    static const int dx[]={0,8,-8,0,0,8,-8,8,-8};
    static const int dy[]={0,0,0,8,-8,8,8,-8,-8};
    int assigned=0,tries=0;
    /* Limit retries when selection or the contextual field click fails. */
    for(int w=0;w<MAX_UNITS&&screen==SCREEN_PLAY&&assigned<most&&tries<16;w++){
        if(!u[w].alive||u[w].owner!=SIDE||u[w].type!=U_DRONE||u[w].order!=ORDER_IDLE||!late_drone_mining_available(&u[w]))continue;
        ++tries;
        if(idle_reach&&dist2(u[w].x,u[w].y,cx,cy)>idle_reach*idle_reach)continue;
        /* Spread the Drones: a field worked by fewer of them wins over a closer
           one, and a Drone whose field runs out just stands there. */
        int target=-1,best=0x7fffffff,best_d=0;
        for(int p=0;p<MAX_MINERALS;p++)if(m[p].amount>0&&currently_visible(m[p].x,m[p].y)){
            int d=dist2(u[w].x,u[w].y,m[p].x,m[p].y),load=0;
            if(d>360*360)continue;
            for(int k=0;k<MAX_UNITS;k++)load+=u[k].alive&&u[k].owner==SIDE&&u[k].type==U_DRONE&&
                u[k].order==ORDER_MINE&&u[k].target==p;
            int score=load*64*64+d;
            if(score<best){best=score;best_d=d;target=p;}
        }
        if(target<0||best_d>640*640)continue;
        /* Idle Drones stack at a spent field. Any of them will do, so a tap
           that lands on another idle one needs no L cycling through the side. */
        close_menus();clear_selected_building();
        move_cursor(u[w].x,u[w].y);KEY_TAP(KEY_A);
        for(int n=0;n<4&&!(selected_count()==1&&idle_drone_selected());n++)KEY_TAP(KEY_L);
        if(!(selected_count()==1&&idle_drone_selected()))continue;
        w=idle_drone_selected()-1;
        for(unsigned k=0;k<sizeof(dx)/sizeof(dx[0]);k++){
            int x=m[target].x+dx[k],y=m[target].y+dy[k];
            move_cursor(x,y);
            if(!u[w].alive||!u[w].selected)break;
            if(field_ui_a_action(selected_count())!=UI_A_MINE)continue;
            KEY_TAP(KEY_A);
            if(u[w].alive&&u[w].order==ORDER_MINE)assigned++;
            break;
        }
    }
    return assigned;
}

static int free_drone(int near_x,int near_y)
{
    int best=-1,bd=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE&&
        !SC64_BUILD_WORKER_ORDER(u[i].order)&&u[i].order!=ORDER_GAS&&!u[i].carry){
        int d=near_x>=0?dist2(u[i].x,u[i].y,near_x,near_y):0;
        if(d<bd){bd=d;best=i;}
    }
    return best;
}

/* Sites the game refused once are not tried again. */
static int refused_x[64],refused_y[64],refused_count;

/* Keep a 16-pixel lane around every structure and mineral field. */
static int site_clear(int type,int x,int y)
{
    for(int k=0;k<refused_count;k++)
        if(FAST_ABS(refused_x[k]-x)<16&&FAST_ABS(refused_y[k]-y)<16)return 0;
    int raw=sc64_race_raw_building(type);
    if(raw<0||raw>=228)return 0;
    int hw=siege_bounds[raw][0]*8,hh=siege_bounds[raw][1]*8;
    for(int k=0;k<MAX_BUILDINGS;k++)if(b[k].alive&&b[k].raw_type>=0&&b[k].raw_type<228){
        int kw=siege_bounds[b[k].raw_type][0]*8,kh=siege_bounds[b[k].raw_type][1]*8;
        if(FAST_ABS(b[k].x-x)<hw+kw+16&&FAST_ABS(b[k].y-y)<hh+kh+16)return 0;
    }
    for(int k=0;k<MAX_MINERALS;k++)if(m[k].amount>0&&
        FAST_ABS(m[k].x-x)<hw+32&&FAST_ABS(m[k].y-y)<hh+24)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&!u[i].selected&&dist2(u[i].x,u[i].y,x,y)<24*24)return 0;
    return 1;
}

/* Morph a Drone into `type` through its build rows, at the legal site nearest
 * the given anchor. */
static int build_near(int type,int menu_row,int anchor_x,int anchor_y,int radius)
{
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
    int best=0x7fffffff,site_x=-1,site_y=-1;
    if(!rule||minerals<(int)rule->minerals||gas<(int)rule->gas)return 0;
    for(int dy=-radius;dy<=radius;dy+=16)for(int dx=-radius;dx<=radius;dx+=16){
        int x=anchor_x+dx,y=anchor_y+dy,d=dx*dx+dy*dy;
        if(d>=best||x<32||x>=WORLD_W-32||y<32||y>=WORLD_H-32||
           !race_can_site(SIDE,type,x,y)||!can_place(type,x,y)||!site_clear(type,x,y))continue;
        best=d;site_x=x;site_y=y;
    }
    if(site_x<0)return 0;
    int worker=free_drone(site_x,site_y);
    if(worker<0||!select_single(worker))return 0;
    if(!choose_row(menu_row))return 0;
    if(mode!=MODE_BUILD||menu!=type){close_menus();return 0;}
    move_cursor(site_x,site_y);KEY_TAP(KEY_A);
    if(mode!=MODE_NORMAL)close_menus();
    if(!SC64_BUILD_WORKER_ORDER(u[worker].order)){
        if(refused_count<64){refused_x[refused_count]=site_x;refused_y[refused_count++]=site_y;}
        return 0;
    }
    fprintf(trace,"{\"kind\":\"build\",\"frame\":%u,\"type\":%d,\"site\":[%d,%d]}\n",(unsigned)frame,type,site_x,site_y);
    return 1;
}

static int build_extractor_near(int x,int y)
{
    int best=-1,bd=0x7fffffff;
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(B_EXTRACTOR);
    if(!rule||minerals<(int)rule->minerals)return 0;
    for(int g=0;g<geyser_count;g++){
        int d=dist2(geysers[g].x,geysers[g].y,x,y);
        if(d<bd){bd=d;best=g;}
    }
    if(best<0||bd>320*320)return 0;
    int worker=free_drone(geysers[best].x,geysers[best].y);
    if(worker<0||!select_single(worker)||!choose_row(MENU_REFINERY))return 0;
    if(mode!=MODE_BUILD||menu!=B_EXTRACTOR){close_menus();return 0;}
    move_cursor(geysers[best].x,geysers[best].y);KEY_TAP(KEY_A);
    if(mode!=MODE_NORMAL)close_menus();
    return SC64_BUILD_WORKER_ORDER(u[worker].order);
}

static int assign_gas_drone(int wanted)
{
    int refinery=-1,have=0,worker=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_EXTRACTOR&&building_complete(i)){refinery=i;break;}
    if(refinery<0)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE){
        if(u[i].order==ORDER_GAS)++have;
        else if(worker<0&&u[i].order==ORDER_MINE&&!u[i].carry)worker=i;
    }
    if(have>=wanted||worker<0||!select_single(worker))return 0;
    move_cursor(b[refinery].x,b[refinery].y);KEY_TAP(KEY_A);
    return u[worker].order==ORDER_GAS;
}

/* Train from one Hatchery's larva: select it, cycle R to the type, press A. */
static int train_at(int hatch,int type)
{
    const Sc64RaceUnitRule *rule=sc64_race_unit_rule(type);
    if(hatch<0||!b[hatch].alive||!rule||b[hatch].larva<=0||
       minerals<(int)rule->minerals||gas<(int)rule->gas||
       race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<(int)rule->supply)return 0;
    if(!select_building(hatch))return 0;
    for(int n=0;n<8&&train_type!=type;n++)KEY_TAP(KEY_R);
    if(train_type!=type)return 0;
    int before=minerals,larva=b[hatch].larva;
    KEY_TAP(KEY_A);
    return minerals<before||b[hatch].larva<larva;
}

/* Order every unit of one type on screen around each still-unordered anchor:
 * a double tap selects that type on screen, which never includes a hero. */
static int group_area_x=-1,group_area_y=-1,group_area_r=0,group_rounds=6;
static int group_command(int type,int which,int x,int y,int force)
{
    static u8 ordered[MAX_UNITS];
    int groups=0;
    for(int i=0;i<MAX_UNITS;i++)
        ordered[i]=(!force&&((u[i].order==(which==MENU_ATTACK?ORDER_ATTACK_MOVE:ORDER_MOVE)&&
            dist2(u[i].tx,u[i].ty,x,y)<96*96)||(which==MENU_ATTACK&&u[i].order==ORDER_ATTACK_TARGET)))||
            (group_area_x>=0&&dist2(u[i].x,u[i].y,group_area_x,group_area_y)>group_area_r*group_area_r);
    for(int round=0;round<group_rounds;round++){
        int anchor=-1,bd=0x7fffffff;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!ordered[i]&&u[i].type==type){
            int d=dist2(u[i].x,u[i].y,x,y);
            if(d<bd){bd=d;anchor=i;}
        }
        if(anchor<0)break;
        close_menus();clear_selected_building();
        move_cursor(u[anchor].x,u[anchor].y);
        KEY_TAP(KEY_A);KEY_TAP(KEY_A);
        int bad=0,count=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected){++count;if(u[i].type!=type)bad=1;}
        ordered[anchor]=1;
        if(!count||bad)continue;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected)ordered[i]=1;
        if(command_selected(which,x,y))++groups;
    }
    return groups;
}

/* An L drag selects everything inside the box, one order moves them all. */
static int box_select(int x0,int y0,int x1,int y1)
{
    close_menus();
    clear_selected_building();
    move_cursor(x0,y0);
    input(KEY_L);
    unsigned guard=0;
    while((cx<x1-2||cy<y1-2)&&guard++<200u){
        u16 held=KEY_L|KEY_R;
        if(cx<x1-2)held|=KEY_RIGHT;
        if(cy<y1-2)held|=KEY_DOWN;
        input(held);
    }
    input(0);
    return selected_count();
}

/* The orders menu reopens on the last chosen command. Choose MOVE ORDER (row
 * 0) with whatever a tap on `unit` selects, and cancel its targeting. */
static int clear_menu_last(int unit)
{
    if(menu_last==0)return 1;
    if(unit<0||!u[unit].alive)return 0;
    close_menus();clear_selected_building();
    move_cursor(u[unit].x,u[unit].y);KEY_TAP(KEY_A);
    if(!selected_count()){close_menus();return 0;}
    KEY_TAP(KEY_B);
    for(int n=0;n<16&&menu_open&&menu_cursor!=0;n++)KEY_TAP(KEY_UP);
    if(menu_open&&menu_cursor==0)KEY_TAP(KEY_A);
    close_menus();
    return menu_last==0;
}

/* One pause-menu save: return the unsaved UI state to its load-time form with
 * ordinary inputs first, so the same keys mean the same after a load. */
static int pause_menu_save(const char *path,int selectable_unit)
{
    close_menus();
    /* The orders menu reopens on the last chosen command: choose MOVE ORDER
       (row 0) and cancel its targeting. Any unit that selects alone will do. */
    for(int i=-1;i<MAX_UNITS&&menu_last!=0;i++){
        int pick=i<0?selectable_unit:i;
        if(pick<0||!select_single(pick))continue;
        KEY_TAP(KEY_B);
        for(int n=0;n<16&&menu_open&&menu_cursor!=0;n++)KEY_TAP(KEY_UP);
        if(menu_open&&menu_cursor==0)KEY_TAP(KEY_A);
        close_menus();
    }
    clear_selected_building();
    for(int k=0;k<4&&(menu_open||mode!=MODE_NORMAL);k++)KEY_TAP(KEY_B);
    unsigned first=host_tick;
    int building=under_construction();
    KEY_TAP(KEY_START);
    int paused=screen==SCREEN_PAUSE;
    KEY_TAP(KEY_B);
    KEY_TAP(KEY_START);
    int any_saved=0;
    for(unsigned i=0;i<sizeof(host_save);i++)any_saved|=host_save[i];
    if(!any_saved)return 0;
    char save_path[512];
    snprintf(save_path,sizeof(save_path),"%s.sav",path);
    FILE *sf=fopen(save_path,"wb");
    if(!sf||fwrite(host_save,1,sizeof(host_save),sf)!=sizeof(host_save)){perror(save_path);return 0;}
    fclose(sf);
    fprintf(trace,"{\"kind\":\"save\",\"frame\":%u,\"first_call\":%u,\"resume_call\":%u,\"paused\":%d,\"screen\":%d,\"under_construction\":%d,\"menu_last\":%d}\n",
        (unsigned)frame,first,host_tick,paused,screen,building,menu_last);
    return 1;
}

#endif
