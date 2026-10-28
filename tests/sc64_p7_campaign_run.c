/* Ordinary-input P7 Homeland campaign controller experiment.
 * The controller may inspect live state to choose targets; all mutations are
 * ordinary HOME/menu/gameplay input delivered through game_update(). */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef P7_CONTROLLER_PROFILE
#define P7_CONTROLLER_PROFILE "P7_TYPED_MOVE_FLAGS"
#endif

#define RUN_LIMIT 60000u
#define KEY_TAP(k) do { input((k)); input(0); } while (0)

static FILE *trace;
static u16 prior;
static unsigned host_tick;
static int army_center(int *x,int *y);

/* This is deliberately a read-only controller-side route query.  The game
 * builds its walkability plane as part of ordinary simulation; if it is not
 * current/available, wait for the engine rather than rebuilding its cache
 * from this observer. */
static int waypoint_to(int tx,int ty,int *wx,int *wy,int *steps_out,
                       int *path_distance_out)
{
    enum { WAYPOINT_STEPS = 8 };
    int cells=MAP_COLS*MAP_ROWS;
    static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    static int prev[MAP_CELLS],queue[MAP_CELLS];
    int sx=0,sy=0,n=0,best_origin=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type!=U_PROBE){
        int d=dist2(u[i].x,u[i].y,tx,ty);
        if(d<best_origin){best_origin=d;sx=u[i].x;sy=u[i].y;}
        ++n;
    }
    if(!n||!path_grid_valid)return 0;
    sx=limit(sx/16,0,MAP_COLS-1);sy=limit(sy/16,0,MAP_ROWS-1);
    int start=sx+sy*MAP_COLS,gx=limit(tx/16,0,MAP_COLS-1),gy=limit(ty/16,0,MAP_ROWS-1);
    int head=0,tail=0,best=-1,bestd=0x7fffffff;
    for(int i=0;i<cells;i++)prev[i]=-2;
    prev[start]=-1;queue[tail++]=start;
    while(head<tail){
        int cell=queue[head++],col=cell%MAP_COLS,row=cell/MAP_COLS;
        /* A Nexus occupies more than one route tile. The engine path API
         * searches only the immediate 3x3 around a clicked point, which can
         * be fully covered by its inflated collision footprint. Treat the
         * visible Nexus neighborhood as the valid attack-move arrival zone. */
        if(FAST_ABS(col-gx)<=4&&FAST_ABS(row-gy)<=4){
            int d=(col-gx)*(col-gx)+(row-gy)*(row-gy);
            if(d<bestd){best=cell;bestd=d;}
            break;
        }
        for(int k=0;k<4;k++){
            int nc=col+dx[k],nr=row+dy[k];
            if(nc<0||nr<0||nc>=MAP_COLS||nr>=MAP_ROWS)continue;
            int next=nc+nr*MAP_COLS;
            if(prev[next]!=-2||(!SC64_GRID_GET(path_walkable,next)&&next!=start))continue;
            prev[next]=cell;queue[tail++]=next;
        }
    }
    if(best<0)return 0;
    int path[MAP_CELLS],len=0;
    for(int c=best;c>=0&&len<cells;c=prev[c])path[len++]=c;
    if(!len)return 0;
    int distance=len-1;
    if(path_distance_out)*path_distance_out=distance;
    int pick=distance>WAYPOINT_STEPS?distance-WAYPOINT_STEPS:0;
    int cell=path[pick];
    *wx=(cell%MAP_COLS)*16+8;*wy=(cell/MAP_COLS)*16+8;
    if(steps_out)*steps_out=distance-pick;
    return 1;
}

static void input(u16 held)
{
    u16 pressed=(u16)(held & (u16)~prior);
    int changed=held!=prior;
    unsigned call_index=host_tick;
    unsigned frame_before=(unsigned)frame;
    if (screen!=SCREEN_RESULT && !won && !lost) game_update(held,pressed);
    prior=held;
    ++host_tick;
    if(trace && changed) {
        fprintf(trace,"{\"kind\":\"input\",\"call_index\":%u,\"frame_before\":%u,\"frame_after\":%u,\"held\":%u,\"pressed\":%u}\n",
            call_index,frame_before,(unsigned)frame,(unsigned)held,(unsigned)pressed);
    }
}

static int nexus(int owner)
{
    int ax=0,ay=0; (void)army_center(&ax,&ay);
    int best=-1,best_d=0x7fffffff;
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(b[i].alive && b[i].raw_type==154 &&
           (owner>=0?b[i].owner==owner:b[i].owner!=SIDE)){
            int d=dist2(ax,ay,b[i].x,b[i].y);
            if(d<best_d){best=i;best_d=d;}
        }
    return best;
}

static int nexus_source(int raw_owner)
{
    int best=-1,best_d=0x7fffffff,ax=0,ay=0;
    (void)army_center(&ax,&ay);
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==154&&
        b[i].raw_owner==raw_owner){
        int d=dist2(ax,ay,b[i].x,b[i].y);
        if(d<best_d){best=i;best_d=d;}
    }
    return best;
}

static int routeable_nexus(int raw_owner,int *wx,int *wy,int *steps,
                           int *path_distance,int excluded_slot)
{
    int best=-1,best_d=0x7fffffff,best_path=0x7fffffff;
    int ax=0,ay=0,bx=0,by=0,bs=0;
    (void)army_center(&ax,&ay);
    for(int i=0;i<MAX_BUILDINGS;i++)if(i!=excluded_slot&&b[i].alive&&b[i].raw_type==154&&b[i].raw_owner==raw_owner){
        int x,y,n,path,d=dist2(ax,ay,b[i].x,b[i].y);
        if(!waypoint_to(b[i].x,b[i].y,&x,&y,&n,&path))continue;
        if(path>best_path||(path==best_path&&d>=best_d))continue;
        best=i;best_d=d;best_path=path;bx=x;by=y;bs=n;
    }
    if(best>=0){*wx=bx;*wy=by;*steps=bs;if(path_distance)*path_distance=best_path;}
    return best;
}

/* Pick the nearest source Nexus by measured walkable route length. A force
 * count heuristic can send the army to a single remote base while a closer
 * victory-gate force is available. */
static int routeable_nexus_any(int *raw_owner,int *wx,int *wy,int *steps,
                               int *path_distance,int excluded_slot)
{
    int best=-1,best_owner=-1,best_path=0x7fffffff;
    int best_x=0,best_y=0,best_steps=0;
    const int owners[2]={0,4};
    for(unsigned k=0;k<2;k++){
        int x,y,n,distance;
        int slot=routeable_nexus(owners[k],&x,&y,&n,&distance,excluded_slot);
        if(slot<0||distance>=best_path)continue;
        best=slot;best_owner=owners[k];best_path=distance;
        best_x=x;best_y=y;best_steps=n;
    }
    if(best>=0){
        *raw_owner=best_owner;*wx=best_x;*wy=best_y;*steps=best_steps;
        if(path_distance)*path_distance=best_path;
    }
    return best;
}

static int heroes_alive(void)
{
    static const int raw[]={75,78,79};
    int count=0;
    for(unsigned h=0;h<sizeof(raw)/sizeof(raw[0]);h++)
        for(int i=0;i<MAX_UNITS;i++)
            if(u[i].alive&&u[i].raw_type==raw[h]&&u[i].raw_owner==2&&
               (u[i].owner==RESCUABLE||u[i].owner==SIDE)){++count;break;}
    return count;
}

static int source_nexus_condition_true(unsigned trigger)
{
    const Sc64P7SourceCondition *condition=NULL;
    if ((trigger!=SC64_P7_TRIGGER_SET_GATE_FORCE2 &&
         trigger!=SC64_P7_TRIGGER_SET_GATE_FORCE3) ||
        !sc64_p7_condition_row(trigger,0,&condition) || !race_p7_ops.condition)
        return 0;
    return race_p7_ops.condition(race_p7_ops.context,&race_p7.runtime,condition)==1;
}

static int strict_campaign_win(void)
{
    const Sc64P7State *model=&race_p7.runtime.model;
    int nexus_gate0=source_nexus_condition_true(SC64_P7_TRIGGER_SET_GATE_FORCE2);
    int nexus_gate1=source_nexus_condition_true(SC64_P7_TRIGGER_SET_GATE_FORCE3);
    int defeat_started=(model->started & 0x000fu)!=0;
    int timers_clear=1;
    for(unsigned i=0;i<SC64_P7_TRIGGER_COUNT;++i)
        if(race_p7.runtime.action_ticks[i])timers_clear=0;
    int ok=screen==SCREEN_RESULT && won && !lost &&
        model->outcome==SC64_P7_WON && race_p7.runtime.fault==SC64_P7_FAULT_NONE &&
        (race_p7.runtime.switch_bits & (1u<<SC64_P7_SWITCH_VICTORY_GATE))!=0 &&
        !defeat_started && sc64_p7_trigger_started(model,SC64_P7_TRIGGER_VICTORY) &&
        model->action_cursor[SC64_P7_TRIGGER_VICTORY]==0 && timers_clear &&
        (nexus_gate0 || nexus_gate1) && heroes_alive()==3;
    fprintf(trace,"{\"kind\":\"strict-verdict\",\"ok\":%d,\"screen_result\":%d,\"won\":%d,\"lost\":%d,\"outcome\":%u,\"fault\":%u,\"switch_bits\":%u,\"defeat_started\":%d,\"victory_started\":%d,\"victory_cursor\":%u,\"action_timers_clear\":%d,\"force2_nexus_condition\":%d,\"force3_nexus_condition\":%d,\"heroes_alive\":%d}\n",
        ok,screen==SCREEN_RESULT,won,lost,(unsigned)model->outcome,
        (unsigned)race_p7.runtime.fault,(unsigned)race_p7.runtime.switch_bits,
        defeat_started,sc64_p7_trigger_started(model,SC64_P7_TRIGGER_VICTORY),
        (unsigned)model->action_cursor[SC64_P7_TRIGGER_VICTORY],timers_clear,
        nexus_gate0,nexus_gate1,heroes_alive());
    return ok;
}

static int army_center(int *x,int *y)
{
    int n=0,sx=0,sy=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE){
        sx+=u[i].x;sy+=u[i].y;++n;
    }
    if(!n)return 0;
    *x=sx/n;*y=sy/n;return n;
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
        fprintf(stderr,"cursor guard=%u wanted=%d,%d got=%d,%d frame=%u\n",guard,x,y,cx,cy,(unsigned)frame);
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

static int nearest_hero_threat(int *out_x,int *out_y)
{
    static const int raw[]={75,78,79};
    int best=0x7fffffff,found=-1;
    for(int h=0;h<3;h++)for(int i=0;i<MAX_UNITS;i++){
        if(!u[i].alive||u[i].raw_type!=raw[h])continue;
        for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&
            owners_hostile(SIDE,u[j].owner)){
            int d=dist2(u[i].x,u[i].y,u[j].x,u[j].y);
            if(d<best){best=d;found=j;*out_x=u[j].x;*out_y=u[j].y;}
        }
    }
    return best<=300*300?found:-1;
}

static int issue_attack(int x,int y)
{
    if(menu_open)KEY_TAP(KEY_B);
    if(mode!=MODE_NORMAL)KEY_TAP(KEY_B);
    clear_selected_building();
    KEY_TAP(KEY_R); /* standard whole-army selection shortcut */
    if(selected_count()<=0)return 0;
    KEY_TAP(KEY_B);
    /* The field menu remembers the last command. Reach Attack using input
     * from the current row instead of assuming that B opens on Move. */
    for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=MENU_ATTACK;n++)
        KEY_TAP(KEY_DOWN);
    if(!menu_open||menu_cursor!=MENU_ATTACK)return 0;
    KEY_TAP(KEY_A);
    if(mode!=MODE_ATTACK)return 0;
    move_cursor(x,y);KEY_TAP(KEY_A);
    return 1;
}

static int live_units(int owner)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&(owner<0||u[i].owner==owner);return n;
}

static int live_buildings(int owner)
{
    int n=0;for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&(owner<0||b[i].owner==owner);return n;
}

static int source_force_military(int raw_owner)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_owner==raw_owner&&
        !IS_WORKER(u[i].type)&&u[i].type!=U_OVERLORD)++n;
    return n;
}

static int queue_combat_unit(void)
{
    int wanted=U_ZEALOT;
    int core=0;
    for(int j=0;j<MAX_BUILDINGS;j++)core|=b[j].alive&&b[j].owner==SIDE&&
        b[j].type==B_CYBERNETICS_CORE&&building_complete(j);
    if(core&&gas>=50&&minerals>=125)wanted=U_DRAGOON;
    const Sc64RaceUnitRule *rule=sc64_race_unit_rule(wanted);
    if(!rule||minerals<(int)rule->minerals||gas<(int)rule->gas||
       race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<(int)rule->supply)return 0;
    int producer=-1,best_queue=6;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_GATEWAY&&building_complete(i)&&b[i].queue_count<best_queue){
        producer=i;best_queue=b[i].queue_count;
    }
    if(producer>=0){int i=producer;
        if(menu_open)KEY_TAP(KEY_B);
        if(mode!=MODE_NORMAL)KEY_TAP(KEY_B);
        move_cursor(b[i].x,b[i].y);KEY_TAP(KEY_A);
        if(selected_building!=i)return 0;
        for(int n=0;n<4&&train_type!=wanted;n++)KEY_TAP(KEY_R);
        if(train_type!=wanted)return 0;
        int before=b[i].queue_count,bank=minerals;
        KEY_TAP(KEY_A);
        if(b[i].queue_count>before){
            fprintf(trace,"{\"kind\":\"production\",\"frame\":%u,\"building\":%d,\"unit\":\"%s\",\"bank_before\":%d,\"bank_after\":%d,\"gas_after\":%d,\"queue\":%u}\n",
                (unsigned)frame,i,wanted==U_DRAGOON?"dragoon":"zealot",bank,minerals,gas,(unsigned)b[i].queue_count);
            return 1;
        }
        return 0;
    }
    return 0;
}

static int probe_count(void)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE;return n;
}

static int queue_probe(void)
{
    int home=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_NEXUS&&
        building_complete(i)){home=i;break;}
    int pending=0;
    if(home>=0)for(int q=0;q<b[home].queue_count;q++)
        pending+=b[home].queue[(b[home].queue_head+q)%5]==U_PROBE;
    if(home<0||probe_count()+pending>=10||minerals<50||b[home].queue_count>=5||
       race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<2)return 0;
    if(menu_open)KEY_TAP(KEY_B);
    if(mode!=MODE_NORMAL)KEY_TAP(KEY_B);
    if(selected_building!=home){clear_selected_building();move_cursor(b[home].x,b[home].y);KEY_TAP(KEY_A);}
    if(selected_building!=home)return 0;
    int before=b[home].queue_count,bank=minerals;
    KEY_TAP(KEY_A);
    if(b[home].queue_count<=before)return 0;
    fprintf(trace,"{\"kind\":\"production\",\"frame\":%u,\"building\":%d,\"unit\":\"probe\",\"bank_before\":%d,\"bank_after\":%d,\"gas_after\":%d,\"queue\":%u}\n",
        (unsigned)frame,home,bank,minerals,gas,(unsigned)b[home].queue_count);
    return 1;
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
    if(gas_workers>=2||worker<0)return 0;
    if(menu_open)KEY_TAP(KEY_B);
    if(mode!=MODE_NORMAL)KEY_TAP(KEY_B);
    clear_selected_building();move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    if(!u[worker].selected)return 0;
    move_cursor(b[refinery].x,b[refinery].y);KEY_TAP(KEY_A);
    if(u[worker].order!=ORDER_GAS||u[worker].target!=refinery)return 0;
    fprintf(trace,"{\"kind\":\"economy\",\"frame\":%u,\"action\":\"assign-gas\",\"worker\":%d,\"refinery\":%d,\"gas_workers\":%d}\n",
        (unsigned)frame,worker,refinery,gas_workers+1);
    return 1;
}

static int count_gateway(void)
{
    int n=0;for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&
        b[i].owner==SIDE&&b[i].type==B_GATEWAY;
    return n;
}

static int build_gateway(void)
{
    int home=-1,worker=-1,best=0x7fffffff,site_x=-1,site_y=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_NEXUS){home=i;break;}
    if(home<0||minerals<150)return 0;
    for(int dy=-160;dy<=160;dy+=16)for(int dx=-160;dx<=160;dx+=16){
        int x=b[home].x+dx,y=b[home].y+dy,d=dx*dx+dy*dy;
        if(d<80*80||x<32||x>=WORLD_W-32||y<32||y>=WORLD_H-32||
           d>=best||!race_can_site(SIDE,B_GATEWAY,x,y)||!can_place(B_GATEWAY,x,y))continue;
        best=d;site_x=x;site_y=y;
    }
    if(site_x<0)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].type==U_PROBE&&u[i].order!=ORDER_BUILD){worker=i;break;}
    if(worker<0)return 0;
    if(menu_open)KEY_TAP(KEY_B);
    if(mode!=MODE_NORMAL)KEY_TAP(KEY_B);
    clear_selected_building();
    move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    if(!u[worker].selected)return 0;
    KEY_TAP(KEY_B);
    for(int n=0;n<MENU_BARRACKS;n++)KEY_TAP(KEY_DOWN);
    KEY_TAP(KEY_A);
    if(mode!=MODE_BUILD||menu!=B_GATEWAY)return 0;
    move_cursor(site_x,site_y);KEY_TAP(KEY_A);
    fprintf(stderr,"gateway construction attempt frame=%u worker=%d site=%d,%d order=%d\n",
        (unsigned)frame,worker,site_x,site_y,u[worker].order);
    return u[worker].order==ORDER_BUILD;
}

static int build_pylon(void)
{
    int home=-1,worker=-1,best=0x7fffffff,site_x=-1,site_y=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_NEXUS){home=i;break;}
    if(home<0||minerals<100)return 0;
    for(int dy=-192;dy<=192;dy+=16)for(int dx=-192;dx<=192;dx+=16){
        int x=b[home].x+dx,y=b[home].y+dy,d=dx*dx+dy*dy;
        if(d<80*80||x<32||x>=WORLD_W-32||y<32||y>=WORLD_H-32||d>=best||
           !race_can_site(SIDE,B_PYLON,x,y)||!can_place(B_PYLON,x,y))continue;
        best=d;site_x=x;site_y=y;
    }
    if(site_x<0)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&u[i].order!=ORDER_BUILD){worker=i;break;}
    if(worker<0)return 0;
    if(menu_open)KEY_TAP(KEY_B);
    if(mode!=MODE_NORMAL)KEY_TAP(KEY_B);
    clear_selected_building();move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    if(!u[worker].selected)return 0;
    KEY_TAP(KEY_B);for(int n=0;n<MENU_DEPOT;n++)KEY_TAP(KEY_DOWN);KEY_TAP(KEY_A);
    if(mode!=MODE_BUILD||menu!=B_PYLON)return 0;
    move_cursor(site_x,site_y);KEY_TAP(KEY_A);
    if(u[worker].order!=ORDER_BUILD)return 0;
    fprintf(trace,"{\"kind\":\"build\",\"frame\":%u,\"building\":\"pylon\",\"worker\":%d,\"site\":[%d,%d],\"count\":%d}\n",
        (unsigned)frame,worker,site_x,site_y,live_buildings(SIDE));
    return 1;
}

static void census(const char *label)
{
    int x=0,y=0,army=army_center(&x,&y),target=nexus(-1);
    int nexuses=0,nexus_force0=0,nexus_force1=0,probes=probe_count(),idle_probes=0,
        mine_probes=0,gas_probes=0,probe_carry=0,gateways=0,complete_gateways=0,
        gateway_queue=0,pylons=0,nexus_queue=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE){
        idle_probes+=u[i].order==ORDER_IDLE;mine_probes+=u[i].order==ORDER_MINE;
        gas_probes+=u[i].order==ORDER_GAS;probe_carry+=u[i].carry;
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE){
        if(b[i].type==B_GATEWAY){++gateways;complete_gateways+=building_complete(i);gateway_queue+=b[i].queue_count;}
        if(b[i].type==B_PYLON)++pylons;
        if(b[i].type==B_NEXUS)nexus_queue+=b[i].queue_count;
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==154){
        ++nexuses;if(b[i].raw_owner==0)++nexus_force1;if(b[i].raw_owner==4)++nexus_force0;
    }
    fprintf(trace,"{\"kind\":\"checkpoint\",\"label\":\"%s\",\"frame\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,\"units\":%d,\"side_units\":%d,\"buildings\":%d,\"heroes_alive\":%d,\"nexuses\":%d,\"force1_nexuses_raw4\":%d,\"force2_nexuses_raw0\":%d,\"force1_military\":%d,\"force2_military\":%d,\"nexus_hp\":%d,\"army_center\":[%d,%d],\"cursor\":[%d,%d],\"mode\":%d,\"selected\":%d,\"target\":%d,\"target_xy\":[%d,%d],\"bank\":[%d,%d],\"supply\":[%d,%d],\"probes\":%d,\"probe_orders\":[%d,%d,%d],\"probe_carry\":%d,\"gateways\":%d,\"complete_gateways\":%d,\"gateway_queue\":%d,\"nexus_queue\":%d,\"pylons\":%d,\"outcome\":%d,\"runtime_fault\":%u,\"switch_bits\":%u,\"cursor_force1\":%u,\"cursor_force2\":%u,\"cursor_victory\":%u}\n",
        label,(unsigned)frame,screen,won,lost,live_units(-1),live_units(SIDE),live_buildings(-1),heroes_alive(),nexuses,
        nexus_force0,nexus_force1,source_force_military(4),source_force_military(0),target>=0?b[target].hp:-1,x,y,cx,cy,mode,selected_count(),target,
        target>=0?b[target].x:-1,target>=0?b[target].y:-1,minerals,gas,race_owner_supply(SIDE),race_owner_supply_cap(SIDE),
        probes,idle_probes,mine_probes,gas_probes,probe_carry,gateways,complete_gateways,gateway_queue,nexus_queue,pylons,
        race_p7.runtime.model.outcome,race_p7.runtime.fault,
        (unsigned)race_p7.runtime.switch_bits,(unsigned)race_p7.runtime.model.action_cursor[8],
        (unsigned)race_p7.runtime.model.action_cursor[9],(unsigned)race_p7.runtime.model.action_cursor[5]);
    fflush(trace);
}

int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:".local/build/p7-controller/trace.json";
    char mkdir_cmd[512];
    snprintf(mkdir_cmd,sizeof(mkdir_cmd),"mkdir -p '%s'", ".local/build/p7-controller");
    (void)system(mkdir_cmd);
    trace=fopen(path,"w");if(!trace){perror(path);return 2;}
    fprintf(trace,"{\"kind\":\"metadata\",\"schema\":1,\"mission\":26,\"profile\":\"%s\",\"input_policy\":\"ordinary game_update held/pressed only\"}\n",P7_CONTROLLER_PROFILE);
    game_init();prior=0;
    assert(screen==SCREEN_TITLE&&race_front.page==SC_FRONT_HOME);
    KEY_TAP(KEY_A);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);
    KEY_TAP(KEY_A);KEY_TAP(KEY_A);
    assert(screen==SCREEN_PLAY&&!won&&!lost&&race_selected_mission==26);
    census("launch");
    int force0_start=source_force_military(0),force4_start=source_force_military(4);
    fprintf(trace,"{\"kind\":\"force-plan\",\"frame\":%u,\"rawowner0_military\":%d,\"rawowner4_military\":%d,\"route_policy\":\"shortest-walkable-source-force\"}\n",
        (unsigned)frame,force0_start,force4_start);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&(u[i].raw_type==75||u[i].raw_type==78||u[i].raw_type==79))
        fprintf(stderr,"hero slot=%d raw=%d owner=%d rawowner=%d xy=%d,%d hp=%d\n",i,u[i].raw_type,u[i].owner,u[i].raw_owner,u[i].x,u[i].y,u[i].hp);
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==154)
        fprintf(stderr,"nexus slot=%d owner=%d rawowner=%d xy=%d,%d\n",i,b[i].owner,b[i].raw_owner,b[i].x,b[i].y);

    unsigned next_checkpoint=1000;
    int locked_target_owner=-1;
    int last_order=-10000,next_probe=300,next_gas=5000,next_production=5500,next_build=7000,next_pylon=5000;
    while(host_tick<RUN_LIMIT&&screen==SCREEN_PLAY&&!won&&!lost){
        if(frame>=next_probe&&probe_count()<10){
            (void)queue_probe();
            next_probe=frame+600;
        }
        if(frame>=next_gas){
            (void)assign_gas_worker();
            next_gas=frame+800;
        }
        if(frame>=next_pylon){
            int pending=0;
            for(int i=0;i<MAX_BUILDINGS;i++)pending|=b[i].alive&&b[i].owner==SIDE&&
                b[i].type==B_PYLON&&b[i].under_construction;
            if(!pending&&race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<=6&&minerals>=200)
                (void)build_pylon();
            next_pylon=frame+1500;
        }
        if(frame>=next_build){
            if(frame>=7000&&probe_count()>=8&&count_gateway()<4&&build_gateway())
                fprintf(trace,"{\"kind\":\"build\",\"frame\":%u,\"building\":\"gateway\",\"count\":%d}\n",
                    (unsigned)frame,count_gateway());
            next_build=frame+1200;
        }
        if(frame>=next_production&&probe_count()>=8){
            (void)queue_combat_unit();
            next_production=frame+180;
        }
        if(frame-last_order>=480){
            int ax=0,ay=0;army_center(&ax,&ay);
            int threat_x=-1,threat_y=-1,threat=nearest_hero_threat(&threat_x,&threat_y);
            int target_owner=-1,target_path=-1;
            static int prior_goal=-1,prior_path=0x7fffffff,stalled_orders=0;
            int t=-1;
            int use_threat=threat>=0&&frame<10000;
            int target_x=use_threat?threat_x:(t>=0?b[t].x:-1),
                target_y=use_threat?threat_y:(t>=0?b[t].y:-1);
            int command_x=target_x,command_y=target_y,route_steps=-1,has_route=0;
            if(!use_threat&&path_grid_valid){
                if(locked_target_owner<0){
                    (void)routeable_nexus_any(&locked_target_owner,
                        &command_x,&command_y,&route_steps,&target_path,-1);
                    fprintf(trace,"{\"kind\":\"goal-lock\",\"frame\":%u,\"source_owner\":%d,\"route_distance\":%d}\n",
                        (unsigned)frame,locked_target_owner,target_path);
                }
                if(locked_target_owner>=0){
                    target_owner=locked_target_owner;
                    t=routeable_nexus(target_owner,&command_x,&command_y,
                                      &route_steps,&target_path,-1);
                }
                if(t>=0&&t==prior_goal){
                    if(target_path+2<prior_path)stalled_orders=0;
                    else ++stalled_orders;
                }else stalled_orders=0;
                if(t>=0&&stalled_orders>=2){
                    int alternate_path=-1;
                    int alternate=routeable_nexus(target_owner,
                        &command_x,&command_y,&route_steps,&alternate_path,t);
                    if(alternate>=0){
                        t=alternate;target_path=alternate_path;stalled_orders=0;
                    }
                }
                if(t>=0){prior_goal=t;prior_path=target_path;}
                if(t>=0){target_x=b[t].x;target_y=b[t].y;has_route=1;}
                else {
                    int raw=nexus_source(0)>=0?0:4;
                    fprintf(trace,"{\"kind\":\"route\",\"frame\":%u,\"source_force\":%d,\"army_center\":[%d,%d],\"grid_valid\":1,\"reachable\":0,\"targets\":[",
                        (unsigned)frame,raw,ax,ay);
                    int comma=0;
                    for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].raw_type==154&&b[j].raw_owner==raw){
                        fprintf(trace,"%s[%d,%d,%d]",comma?",":"",j,b[j].x,b[j].y);comma=1;
                    }
                    fputs("]}\n",trace);
                    target_x=target_y=-1;
                }
            }
            if(target_x>=0){
                if(use_threat)has_route=waypoint_to(target_x,target_y,&command_x,&command_y,&route_steps,NULL);
                if(!has_route&&t>=0&&!use_threat){
                    fprintf(trace,"{\"kind\":\"route\",\"frame\":%u,\"target_slot\":%d,\"target_xy\":[%d,%d],\"army_center\":[%d,%d],\"grid_valid\":%d,\"reachable\":0}\n",
                        (unsigned)frame,t,target_x,target_y,ax,ay,path_grid_valid);
                    fprintf(stderr,"no host route frame=%u target=%d,%d army=%d,%d grid_valid=%d\n",
                        (unsigned)frame,target_x,target_y,ax,ay,path_grid_valid);
                }
                if((has_route||use_threat)&&issue_attack(command_x,command_y)){
                    last_order=frame;
                    fprintf(trace,"{\"kind\":\"command\",\"frame\":%u,\"target\":\"%s\",\"target_slot\":%d,\"target_owner\":%d,\"path_distance\":%d,\"stalled_orders\":%d,\"target_xy\":[%d,%d],\"command_xy\":[%d,%d],\"route_steps\":%d,\"army_center\":[%d,%d],\"heroes_alive\":%d}\n",
                        (unsigned)frame,use_threat?"hero-threat":"nexus",use_threat?threat:t,target_owner,target_path,stalled_orders,target_x,target_y,command_x,command_y,route_steps,ax,ay,heroes_alive());
                    fprintf(stderr,"order frame=%u target=%s slot=%d at=%d,%d waypoint=%d,%d steps=%d heroes=%d army=%d\n",
                        (unsigned)frame,use_threat?"threat":"nexus",use_threat?threat:t,target_x,target_y,command_x,command_y,route_steps,heroes_alive(),live_units(SIDE));
                }
            }
            if(!last_order||frame-last_order>=480)last_order=frame;
        }
        input(0);
        if((unsigned)frame>=next_checkpoint){
            char label[32];snprintf(label,sizeof(label),"frame-%u",(unsigned)frame);census(label);next_checkpoint+=1000;
        }
    }
    census("final");
    for(int i=0;i<MAX_UNITS;i++)
        fprintf(trace,"{\"kind\":\"source-unit\",\"slot\":%d,\"alive\":%d,\"raw\":%d,\"source_owner\":%d,\"owner\":%d,\"x\":%d,\"y\":%d,\"hp\":%d,\"shield\":%d,\"order\":%d,\"target\":%d,\"tx\":%d,\"ty\":%d,\"path_timer\":%d,\"selected\":%d}\n",
            i,u[i].alive,u[i].raw_type,u[i].raw_owner,u[i].owner,u[i].x,u[i].y,u[i].hp,u[i].shield,u[i].order,u[i].target,u[i].tx,u[i].ty,u[i].path_timer,u[i].selected);
    for(int i=0;i<MAX_BUILDINGS;i++)
        fprintf(trace,"{\"kind\":\"source-building\",\"slot\":%d,\"alive\":%d,\"raw\":%d,\"source_owner\":%d,\"owner\":%d,\"x\":%d,\"y\":%d,\"hp\":%d}\n",
            i,b[i].alive,b[i].raw_type,b[i].raw_owner,b[i].owner,b[i].x,b[i].y,b[i].hp);
    fprintf(trace,"{\"kind\":\"final\",\"total_calls\":%u,\"frame\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,\"heroes_alive\":%d,\"units\":%d,\"buildings\":%d}\n",
        (unsigned)host_tick,(unsigned)frame,screen,won,lost,heroes_alive(),live_units(-1),live_buildings(-1));
    int strict_win=strict_campaign_win();
    fclose(trace);
    fprintf(stderr,"P7 run frame=%u won=%d lost=%d heroes=%d units=%d buildings=%d\n",
        (unsigned)frame,won,lost,heroes_alive(),live_units(-1),live_buildings(-1));
    return strict_win?0:1;
}
