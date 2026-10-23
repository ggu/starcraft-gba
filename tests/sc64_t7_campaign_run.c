/* Bounded, normal-input controller probe for the private T7 mission. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SC64_TERRAN_LATE_PROBE_MISSION
#error "compile with the private late-mission probe entry"
#endif
#if SC64_TERRAN_LATE_PROBE_MISSION != 10
#error "this runner targets T7/global mission 10"
#endif

enum { T7_DEFAULT_LIMIT = 60000, T7_CHECKPOINT_PERIOD = 1000 };
static FILE *trace_file;
static const char *trace_path;
static int tick_limit = T7_DEFAULT_LIMIT, inspect_only;
static u32 host_tick;
static u16 prior_keys;
static int last_held = -1, trace_first = 1;
static int kerrigan_slot = -1;
static int last_carrier_slot = -1;

static int clamp_world(int p, int limit)
{ return p < 0 ? 0 : p >= limit ? limit - 1 : p; }

static int snap4(int from, int to, int limit)
{
    int d = to - from;
    int n = d >= 0 ? (d + 2) / 4 : (d - 2) / 4;
    return clamp_world(from + n * 4, limit);
}

static int point_in_location(unsigned id, int x, int y)
{
    if (!id || id > sc64_t7_scenario.location_count) return 0;
    const Sc64T4Location *r = &sc64_t7_scenario.locations[id - 1];
    return (u32)(x * 2) >= r->left && (u32)(x * 2) < r->right &&
           (u32)(y * 2) >= r->top && (u32)(y * 2) < r->bottom;
}

static int raw_unit(int raw, int owner)
{
    for (int i=0;i<MAX_UNITS;i++)
        if (u[i].alive && u[i].raw_type==raw && (owner<0 || u[i].owner==owner)) return i;
    return -1;
}

static int raw_building(int raw, int owner)
{
    for (int i=0;i<MAX_BUILDINGS;i++)
        if (b[i].alive && b[i].raw_type==raw && (owner<0 || b[i].owner==owner)) return i;
    return -1;
}

static int owner_units(int owner)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==owner)++n;return n;
}

static int owner_buildings(int owner)
{
    int n=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==owner)++n;return n;
}

static int unit_raw_count(int raw, int owner, unsigned loc)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==raw&&
       (owner<0||u[i].owner==owner)&&!SC64_TRANSPORT_LOADED(&u[i])&&
       (!loc||point_in_location(loc,u[i].x,u[i].y)))++n;
    return n;
}

static void checkpoint(const char *kind)
{
    if(!trace_file)return;
    if(!trace_first)fputs(",\n",trace_file);trace_first=0;
    int emitter=race_late.emitter_unit<MAX_UNITS?(int)race_late.emitter_unit:-1;
    int carrier=race_late.emitter_carrier<MAX_UNITS?(int)race_late.emitter_carrier:-1;
    int observed_carrier=carrier>=0?carrier:last_carrier_slot;
    int enemy=-1, enemy_distance=0x7fffffff;
    if(observed_carrier>=0)for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&
       owners_hostile(SIDE,u[i].owner)){
        int d=dist2(u[observed_carrier].x,u[observed_carrier].y,u[i].x,u[i].y);
        if(d<enemy_distance){enemy_distance=d;enemy=i;}
    }
    fprintf(trace_file,
      "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,\"mission\":%d,\"faults\":%d,\"phase\":%u,\"objective_flags\":%u,\"race_completed\":%d,\"kerrigan\":%d,\"kerrigan_alive\":%d,\"emitter_slot\":%d,\"emitter_alive\":%d,\"emitter_owner\":%d,\"emitter_raw_owner\":%d,\"emitter_x\":%d,\"emitter_y\":%d,\"emitter_carrier\":%d,\"tracked_carrier\":%d,\"carrier_alive\":%d,\"carrier_x\":%d,\"carrier_y\":%d,\"carrier_hp\":%d,\"carrier_order\":%d,\"carrier_tx\":%d,\"carrier_ty\":%d,\"carrier_path_retry\":%d,\"nearest_enemy\":%d,\"nearest_enemy_raw\":%d,\"nearest_enemy_x\":%d,\"nearest_enemy_y\":%d,\"nearest_enemy_dist2\":%d,\"emitter_at_goal\":%d,\"side_units\":%d,\"enemy_units\":%d,\"side_buildings\":%d,\"enemy_buildings\":%d,\"minerals\":%d,\"gas\":%d}",
      kind,(unsigned)host_tick,(unsigned)frame,screen,won,lost,race_selected_mission,
      mission_unsupported_count,(unsigned)race_campaign.phase,
      (unsigned)race_campaign.objective_flags,race_completed,
      kerrigan_slot,kerrigan_slot>=0&&u[kerrigan_slot].alive,emitter,
      emitter>=0&&u[emitter].alive,emitter>=0?u[emitter].owner:-1,
      emitter>=0?u[emitter].raw_owner:-1,emitter>=0?u[emitter].x:-1,
      emitter>=0?u[emitter].y:-1,carrier,observed_carrier,
      observed_carrier>=0&&u[observed_carrier].alive,
      observed_carrier>=0?u[observed_carrier].x:-1,
      observed_carrier>=0?u[observed_carrier].y:-1,
      observed_carrier>=0?u[observed_carrier].hp:-1,
      observed_carrier>=0?u[observed_carrier].order:-1,
      observed_carrier>=0?u[observed_carrier].tx:-1,
      observed_carrier>=0?u[observed_carrier].ty:-1,
      observed_carrier>=0?u[observed_carrier].path_retry:-1,enemy,
      enemy>=0?u[enemy].raw_type:-1,enemy>=0?u[enemy].x:-1,
      enemy>=0?u[enemy].y:-1,enemy>=0?enemy_distance:0x7fffffff,
      emitter>=0&&point_in_location(1,u[emitter].x,u[emitter].y),
      owner_units(SIDE),owner_units(ENEMY),owner_buildings(SIDE),owner_buildings(ENEMY),
      minerals,gas);
    fflush(trace_file);
}

static void step(u16 held)
{
    if(host_tick>=(u32)tick_limit||screen!=SCREEN_PLAY||won||lost)return;
    u16 pressed=held&(u16)~prior_keys;
    int old_screen=screen;
    game_update(held,pressed);prior_keys=held;++host_tick;
    if(pressed||screen!=old_screen)fprintf(stderr,"T7 input tick=%u held=%u pressed=%u screen=%d->%d\n",
      (unsigned)host_tick,held,pressed,old_screen,screen);
    if(trace_file&&held!=(u16)last_held){
        if(!trace_first)fputs(",\n",trace_file);trace_first=0;
        fprintf(trace_file,"{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}",
                (unsigned)host_tick,held,pressed);last_held=held;
    }
    if(trace_file&&host_tick%T7_CHECKPOINT_PERIOD==0)checkpoint("periodic");
}

static void wait_frames(int n)
{ for(int i=0;i<n&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;i++)step(0); }

static void tap(u16 key){step(key);step(0);}

static void move_cursor(int x,int y)
{
    x=snap4(cx,x,WORLD_W);y=snap4(cy,y,WORLD_H);
    for(int n=0;n<4000&&(cx!=x||cy!=y)&&host_tick<(u32)tick_limit;n++){
        u16 held=KEY_R;if(cx<x)held|=KEY_RIGHT;if(cx>x)held|=KEY_LEFT;
        if(cy<y)held|=KEY_DOWN;if(cy>y)held|=KEY_UP;step(held);
    }
    step(0);
}

static int normal_field(void)
{
    for(int i=0;i<2&&(menu_open||mode!=MODE_NORMAL);i++)tap(KEY_B);
    return screen==SCREEN_PLAY&&!menu_open&&mode==MODE_NORMAL;
}

static int leave_building_selection(void)
{
    if(selected_building<0)return 1;
    if(menu_open||mode!=MODE_NORMAL)return 0;
    /* The engine's L gesture cancels a Barracks queue instead of cycling
       away. Select and commit an ordinary no-unit MOVE to clear it. */
    tap(KEY_B);tap(KEY_A);tap(KEY_A);
    return selected_building<0&&mode==MODE_NORMAL&&!menu_open;
}

static int select_unit(int slot)
{
    if(slot<0||slot>=MAX_UNITS||!u[slot].alive||u[slot].owner!=SIDE||!normal_field())return 0;
    if(!leave_building_selection())return 0;
    for(int n=0;n<=MAX_UNITS&&host_tick<(u32)tick_limit;n++){
        if(u[slot].selected&&selected_count()==1&&selected_building<0)return 1;
        tap(KEY_L);
    }
    return 0;
}

static int select_source_unit(int slot)
{
    if(slot<0||slot>=MAX_UNITS||!u[slot].alive||!normal_field())return 0;
    if(!leave_building_selection())return 0;
    for(int n=0;n<=MAX_UNITS&&host_tick<(u32)tick_limit;n++){
        if(u[slot].selected&&selected_count()==1&&selected_building<0)return 1;
        tap(KEY_L);
    }
    return 0;
}

static int order_unit(int slot,int x,int y)
{
    if(!select_unit(slot)){
        fprintf(stderr,"T7 order-unit select failed slot=%d alive=%d owner=%d selected=%d selected_count=%d building=%d mode=%d menu=%d carrier=%u\n",
          slot,slot>=0&&slot<MAX_UNITS?u[slot].alive:0,
          slot>=0&&slot<MAX_UNITS?u[slot].owner:-1,
          slot>=0&&slot<MAX_UNITS?u[slot].selected:0,selected_count(),selected_building,mode,menu_open,
          (unsigned)race_late.emitter_carrier);
        return 0;
    }
    tap(KEY_B);tap(KEY_A);move_cursor(x,y);tap(KEY_A);return 1;
}

static int order_attack_unit(int slot,int x,int y)
{
    if(!select_unit(slot))return 0;
    tap(KEY_B);tap(KEY_DOWN);tap(KEY_A);move_cursor(x,y);tap(KEY_A);return 1;
}

static int order_visible_type(int type,int x,int y)
{
    if(!normal_field())return 0;
    int seed=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type&&
       !SC64_TRANSPORT_LOADED(&u[i])){
        move_cursor(u[i].x,u[i].y);
        if(selection_target()==i){seed=i;break;}
    }
    if(seed<0)return 0;
    tap(KEY_A);tap(KEY_A);
    int count=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected){
        if(u[i].owner!=SIDE||u[i].type!=type)return 0;
        ++count;
    }
    if(!count||count!=selected_count()||selected_building>=0)return 0;
    tap(KEY_B);tap(KEY_DOWN);tap(KEY_A);move_cursor(x,y);tap(KEY_A);
    fprintf(stderr,"T7 escorted group type=%d seed=%d count=%d order=attack-move target=%d,%d\n",type,seed,count,x,y);
    return 1;
}

static int queue_marine(void)
{
    int barracks=raw_building(111,SIDE);
    if(barracks<0||!building_complete(barracks)||b[barracks].queue_count>=5||
       minerals<unit_cost(U_MARINE)||
       supply_used+unit_supply(U_MARINE)>supply_cap||!normal_field())return 0;
    if(selected_building!=barracks){
        if(!leave_building_selection())return 0;
        move_cursor(b[barracks].x,b[barracks].y);tap(KEY_A);
    }
    if(selected_building!=barracks||selected_count())return 0;
    int before=b[barracks].queue_count;
    tap(KEY_A);
    if(b[barracks].queue_count<=before)return 0;
    fprintf(stderr,"T7 paid Marine queued at barracks=%d queue=%d wallet=%d/%d supply=%d/%d\n",
      barracks,b[barracks].queue_count,minerals,gas,supply_used,supply_cap);
    checkpoint("marine-queued");
    return leave_building_selection();
}

static void prepare_marine_escort(void)
{
    for(int n=0;n<5&&host_tick<(u32)tick_limit;n++){
        if(!queue_marine())break;
    }
}

static void preclear_route_threat(void)
{
    int emitter=race_late.emitter_unit<MAX_UNITS?(int)race_late.emitter_unit:-1;
    int threat=-1,best=0x7fffffff;
    if(emitter>=0&&u[emitter].alive)for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==3){
            int d=dist2(u[emitter].x,u[emitter].y,u[i].x,u[i].y);
            if(d<best){best=d;threat=i;}
        }
    if(threat<0)return;
    int x=u[threat].x,y=u[threat].y;
    fprintf(stderr,"T7 preclear target raw3 slot=%d at=%d,%d hp=%d\n",threat,x,y,u[threat].hp);
    (void)order_visible_type(U_MARINE,x,y);
    (void)order_visible_type(U_VULTURE,x,y);
    (void)order_visible_type(U_FIREBAT,x,y);
    checkpoint("route-threat-preclear-ordered");
    for(int i=0;i<2400&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;i++){
        step(0);
        if(!u[threat].alive)break;
    }
    fprintf(stderr,"T7 preclear end tick=%u slot=%d alive=%d hp=%d side=%d enemy=%d\n",
      (unsigned)host_tick,threat,u[threat].alive,u[threat].hp,owner_units(SIDE),owner_units(ENEMY));
    checkpoint("route-threat-preclear-result");
}

static int location_center(unsigned id,int *x,int *y)
{
    if(!id||id>sc64_t7_scenario.location_count||!x||!y)return 0;
    const Sc64T4Location *p=&sc64_t7_scenario.locations[id-1];
    *x=(int)((p->left+p->right)/4);*y=(int)((p->top+p->bottom)/4);return 1;
}

static int reachable_location_point(int sx,int sy,unsigned id,int *ox,int *oy)
{
    static u8 closed[MAP_CELLS];static u16 steps[MAP_CELLS];
    static u32 cost[MAP_CELLS],risk[MAP_CELLS];static s16 parent[MAP_CELLS];
    static u16 heap_node[MAP_CELLS*8];static u32 heap_key[MAP_CELLS*8];
    if(!id||id>sc64_t7_scenario.location_count||!ox||!oy)return -1;
    int start=sx/16+(sy/16)*MAP_COLS;if(start<0||start>=MAP_CELLS)return -1;
    const Sc64T4Location *r=&sc64_t7_scenario.locations[id-1];
    build_path_grid();memset(closed,0,sizeof(closed));memset(parent,0xff,sizeof(parent));
    for(int i=0;i<MAP_CELLS;i++){cost[i]=0xffffffffu;risk[i]=0;steps[i]=0;}
    /* Prefer ground paths that stay well outside enemy weapon envelopes.
       Movers can advance while we are issuing each waypoint, so use a wider
       planning margin than the firing range and a steep cost inside range. */
    for(int c=0;c<MAP_CELLS;c++){
        int x=(c%MAP_COLS)*16+8,y=(c/MAP_COLS)*16+8;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&owners_hostile(SIDE,u[i].owner)&&
           sc64_race_unit_damage_vs(u[i].type,0)>0){
            int weapon=unit_range(u[i].type),radius=weapon+512,d=dist2(x,y,u[i].x,u[i].y);
            if(d<radius*radius){
                u32 near=(u32)(radius*radius-d)/(u32)(radius*8)+1u;
                risk[c]+=near+(d<(weapon+64)*(weapon+64)?10000u:0u);
            }
        }
        /* Bunkers are not modeled as loaded weapon hosts by the compact
           runtime, but treating source bunker envelopes as costly helps the
           controller avoid staging a worker beside one. Missile turrets are
           air-only and therefore do not threaten this ground carrier. */
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&
           b[i].raw_type==125){
            int radius=320,d=dist2(x,y,b[i].x,b[i].y);
            if(d<radius*radius)risk[c]+=(u32)(radius*radius-d)/(u32)(radius*8)+24u;
        }
    }
    int heap_count=0,goal=-1;
#define T7_HEAP_PUSH(node_,key_) do{ \
      if(heap_count>=MAP_CELLS*8)return -1; \
      int h_=heap_count++;heap_node[h_]=(u16)(node_);heap_key[h_]=(u32)(key_); \
      while(h_>0){int p_=(h_-1)/2;if(heap_key[p_]<=heap_key[h_])break; \
        u32 k_=heap_key[p_];heap_key[p_]=heap_key[h_];heap_key[h_]=k_; \
        u16 n_=heap_node[p_];heap_node[p_]=heap_node[h_];heap_node[h_]=n_;h_=p_;} \
    }while(0)
#define T7_HEAP_POP(node_out_,key_out_) do{ \
      (node_out_)=heap_node[0];(key_out_)=heap_key[0];--heap_count; \
      if(heap_count>0){heap_node[0]=heap_node[heap_count];heap_key[0]=heap_key[heap_count]; \
        int h_=0;for(;;){int l_=h_*2+1,rr_=l_+1;if(l_>=heap_count)break; \
          int m_=(rr_<heap_count&&heap_key[rr_]<heap_key[l_])?rr_:l_; \
          if(heap_key[h_]<=heap_key[m_])break;u32 k_=heap_key[h_];heap_key[h_]=heap_key[m_];heap_key[m_]=k_; \
          u16 n_=heap_node[h_];heap_node[h_]=heap_node[m_];heap_node[m_]=n_;h_=m_;}} \
    }while(0)
    cost[start]=0;T7_HEAP_PUSH(start,0);
    while(heap_count){
        int c;u32 popped;T7_HEAP_POP(c,popped);(void)popped;
        if(closed[c])continue;closed[c]=1;
        int col=c%MAP_COLS,row=c/MAP_COLS,px=col*16+8,py=row*16+8;
        if((u32)(px*2)>=r->left&&(u32)(px*2)<r->right&&
           (u32)(py*2)>=r->top&&(u32)(py*2)<r->bottom){goal=c;break;}
#define T7_RELAX(n_) do{int z_=(n_);if(!closed[z_]&&SC64_GRID_GET(path_walkable,z_)){ \
      u32 nc_=cost[c]+10u+risk[z_]*5u;if(nc_<cost[z_]){cost[z_]=nc_;parent[z_]=(s16)c;steps[z_]=(u16)(steps[c]+1);T7_HEAP_PUSH(z_,nc_);}}}while(0)
        if(col)T7_RELAX(c-1);if(col+1<MAP_COLS)T7_RELAX(c+1);
        if(row)T7_RELAX(c-MAP_COLS);if(row+1<MAP_ROWS)T7_RELAX(c+MAP_COLS);
#undef T7_RELAX
    }
#undef T7_HEAP_POP
#undef T7_HEAP_PUSH
    if(goal<0)return -1;
    int cell=goal,back=(int)steps[goal]-8;
    while(back-->0&&parent[cell]>=0)cell=parent[cell];
    *ox=(cell%MAP_COLS)*16+8;*oy=(cell/MAP_COLS)*16+8;
    return steps[goal];
}

static int nearest_pickup_actor(int emitter)
{
    if(emitter<0||emitter>=MAX_UNITS||!u[emitter].alive)return -1;
    int best=0x7fffffff,slot=-1;
    for(int i=0;i<MAX_UNITS;i++)if(i!=emitter&&u[i].alive&&u[i].owner==SIDE&&
       u[i].raw_type==7&&u[i].type!=U_KERRIGAN&&
       (sc64_race_t4_actor_flags(u[i].type)&SC64_T4_ACTOR_MOVABLE)){
        int d=dist2(u[i].x,u[i].y,u[emitter].x,u[emitter].y);
        if(d<best){best=d;slot=i;}
    }
    return slot;
}

static int controller_collect_emitter(int emitter)
{
    if(emitter<0||!u[emitter].alive)return 0;
    if(race_late.emitter_carrier<MAX_UNITS){last_carrier_slot=race_late.emitter_carrier;return 1;}
    int actor=nearest_pickup_actor(emitter);
    if(actor<0)return 0;
    int dx=u[actor].x-u[emitter].x,dy=u[actor].y-u[emitter].y;
    fprintf(stderr,"T7 pickup actor=%d raw=%d at=%d,%d emitter=%d at=%d,%d dist2=%d\n",
      actor,u[actor].raw_type,u[actor].x,u[actor].y,emitter,u[emitter].x,u[emitter].y,dx*dx+dy*dy);
    if(dx < -8 || dx > 8 || dy < -8 || dy > 8){
        if(!order_unit(actor,u[emitter].x,u[emitter].y))return 0;
    }
    for(int i=0;i<2000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;i++){
        step(0);
        if(race_late.emitter_carrier<MAX_UNITS&&race_late.emitter_carrier==actor){
            last_carrier_slot=actor;
            fprintf(stderr,"T7 emitter picked up by actor=%d raw=%d tick=%u\n",actor,u[actor].raw_type,(unsigned)host_tick);
            checkpoint("emitter-picked-up");return 1;
        }
        if(!u[actor].alive||!u[emitter].alive)return 0;
    }
    fprintf(stderr,"T7 pickup timeout actor=%d alive=%d emitter=%d carrier=%u pos=%d,%d emitterpos=%d,%d\n",
      actor,u[actor].alive,emitter,(unsigned)race_late.emitter_carrier,u[actor].x,u[actor].y,u[emitter].x,u[emitter].y);
    return 0;
}

static void controller_assign_gas_workers(void)
{
    int refinery=raw_building(110,SIDE);
    if(refinery<0||!building_complete(refinery))return;
    int assigned=0;
    for(int n=0;n<3&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;n++){
        int worker=-1,best=0x7fffffff;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==7&&
           i!=(int)race_late.emitter_carrier&&u[i].order!=ORDER_GAS&&
           !SC64_TRANSPORT_LOADED(&u[i])){
            int d=dist2(u[i].x,u[i].y,b[refinery].x,b[refinery].y);
            if(d<best){best=d;worker=i;}
        }
        if(worker<0)break;
        if(!u[worker].alive||!select_unit(worker))continue;
        move_cursor(b[refinery].x,b[refinery].y);tap(KEY_A);
        if(u[worker].order==ORDER_GAS&&u[worker].target==refinery){
            ++assigned;
            fprintf(stderr,"T7 assigned SCV=%d to refinery=%d\n",worker,refinery);
        }
    }
    fprintf(stderr,"T7 gas assignment count=%d wallet=%d/%d\n",assigned,minerals,gas);
    checkpoint("gas-workers-assigned");
}

static int controller_build_dropship(int *out_ship)
{
    if(out_ship)*out_ship=-1;
    int starport=raw_building(114,SIDE);
    if(starport<0||!building_complete(starport)||!normal_field())return 0;
    for(int i=0;i<30000&&gas<100&&host_tick<(u32)tick_limit&&
        screen==SCREEN_PLAY&&!won&&!lost;i++)step(0);
    if(gas<100||minerals<100){fprintf(stderr,"T7 Dropship resources unavailable wallet=%d/%d\n",minerals,gas);return 0;}
    if(selected_building!=starport){
        if(!leave_building_selection())return 0;
        move_cursor(b[starport].x,b[starport].y);tap(KEY_A);
    }
    if(selected_building!=starport||selected_count())return 0;
    for(int i=0;i<5&&train_type!=U_DROPSHIP;i++)tap(KEY_R);
    if(train_type!=U_DROPSHIP||minerals<100||gas<100)return 0;
    int before=b[starport].queue_count;
    tap(KEY_A);
    if(b[starport].queue_count<=before||b[starport].queue[b[starport].queue_head]!=U_DROPSHIP)return 0;
    fprintf(stderr,"T7 paid Dropship queued starport=%d wallet=%d/%d\n",starport,minerals,gas);
    checkpoint("dropship-queued");
    for(int t=0;t<6000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
        step(0);
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DROPSHIP){
            if(out_ship)*out_ship=i;
            checkpoint("dropship-complete");
            return 1;
        }
    }
    return 0;
}

static u32 air_cell_risk(int x,int y)
{
    u32 risk=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&!SC64_TRANSPORT_LOADED(&u[i])&&
       owners_hostile(SIDE,u[i].owner)){
        int range=sc64_race_unit_range_vs(u[i].type,1);
        if(range>0&&sc64_race_unit_damage_vs(u[i].type,1)>0){
            int weapon=(range+1)/2,margin=weapon+256,d=dist2(x,y,u[i].x,u[i].y);
            if(d<margin*margin){
                risk+=(u32)(margin*margin-d)/(u32)(margin*8)+1u;
                if(d<(weapon+32)*(weapon+32))risk+=4000u;
            }
        }
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&building_complete(i)){
        if(b[i].type==B_MISSILE_TURRET||b[i].type==B_PHOTON_CANNON||
           b[i].type==B_SPORE_COLONY||b[i].type==B_BUNKER){
            int weapon=112,margin=weapon+256,d=dist2(x,y,b[i].x,b[i].y);
            if(d<margin*margin){
                risk+=(u32)(margin*margin-d)/(u32)(margin*8)+1u;
                if(d<(weapon+32)*(weapon+32))risk+=4000u;
            }
        }
    }
    return risk;
}

static int air_next_waypoint(int sx,int sy,int gx,int gy,int *wx,int *wy)
{
    static u8 closed[MAP_CELLS];static u16 previous[MAP_CELLS],heap_node[MAP_CELLS*8],reverse[MAP_CELLS];
    static u32 cost[MAP_CELLS],heap_key[MAP_CELLS*8];
    int start=sx/16+(sy/16)*MAP_COLS,goal=gx/16+(gy/16)*MAP_COLS;
    if(!wx||!wy||start<0||start>=MAP_CELLS||goal<0||goal>=MAP_CELLS)return 0;
    memset(closed,0,sizeof(closed));for(int i=0;i<MAP_CELLS;i++){previous[i]=0xffffu;cost[i]=0xffffffffu;}
    int heap_count=0;
#define T7_AIR_PUSH(node_,key_) do{if(heap_count>=MAP_CELLS*8)return 0; \
    int h_=heap_count++;heap_node[h_]=(u16)(node_);heap_key[h_]=(u32)(key_); \
    while(h_>0){int p_=(h_-1)/2;if(heap_key[p_]<=heap_key[h_])break; \
      u32 k_=heap_key[p_];heap_key[p_]=heap_key[h_];heap_key[h_]=k_; \
      u16 n_=heap_node[p_];heap_node[p_]=heap_node[h_];heap_node[h_]=n_;h_=p_;}}while(0)
#define T7_AIR_POP(node_out_,key_out_) do{(node_out_)=heap_node[0];(key_out_)=heap_key[0];--heap_count; \
    if(heap_count){heap_node[0]=heap_node[heap_count];heap_key[0]=heap_key[heap_count];int h_=0;for(;;){ \
      int l_=h_*2+1,r_=l_+1;if(l_>=heap_count)break;int m_=(r_<heap_count&&heap_key[r_]<heap_key[l_])?r_:l_; \
      if(heap_key[h_]<=heap_key[m_])break;u32 k_=heap_key[h_];heap_key[h_]=heap_key[m_];heap_key[m_]=k_; \
      u16 n_=heap_node[h_];heap_node[h_]=heap_node[m_];heap_node[m_]=n_;h_=m_;}}}while(0)
    cost[start]=0;T7_AIR_PUSH(start,0);
    while(heap_count){
        int cell;u32 popped;T7_AIR_POP(cell,popped);(void)popped;
        if(closed[cell])continue;closed[cell]=1;if(cell==goal)break;
        int col=cell%MAP_COLS,row=cell/MAP_COLS;
#define T7_AIR_RELAX(n_) do{int z_=(n_);if(!closed[z_]){int nx_=(z_%MAP_COLS)*16+8,ny_=(z_/MAP_COLS)*16+8; \
      u32 nc_=cost[cell]+10u+air_cell_risk(nx_,ny_)*4u;if(nc_<cost[z_]){cost[z_]=nc_;previous[z_]=(u16)cell;T7_AIR_PUSH(z_,nc_);}}}while(0)
        if(col)T7_AIR_RELAX(cell-1);if(col+1<MAP_COLS)T7_AIR_RELAX(cell+1);
        if(row)T7_AIR_RELAX(cell-MAP_COLS);if(row+1<MAP_ROWS)T7_AIR_RELAX(cell+MAP_COLS);
        if(col&&row)T7_AIR_RELAX(cell-MAP_COLS-1);
        if(col+1<MAP_COLS&&row)T7_AIR_RELAX(cell-MAP_COLS+1);
        if(col&&row+1<MAP_ROWS)T7_AIR_RELAX(cell+MAP_COLS-1);
        if(col+1<MAP_COLS&&row+1<MAP_ROWS)T7_AIR_RELAX(cell+MAP_COLS+1);
#undef T7_AIR_RELAX
    }
    if(!closed[goal])return 0;
    int length=0,at=goal;
    while(at!=start&&length<MAP_CELLS){reverse[length++]=(u16)at;at=previous[at];if(at>=MAP_CELLS)return 0;}
    if(!length){*wx=gx;*wy=gy;return 1;}
    int prior=start,last_dx=0,last_dy=0;
    for(int k=length-1;k>=0;k--){
        int next=reverse[k],dx=next%MAP_COLS-prior%MAP_COLS,dy=next/MAP_COLS-prior/MAP_COLS;
        if(k!=length-1&&(dx!=last_dx||dy!=last_dy)){
            *wx=(prior%MAP_COLS)*16+8;*wy=(prior/MAP_COLS)*16+8;return 1;
        }
        prior=next;last_dx=dx;last_dy=dy;
    }
#undef T7_AIR_POP
#undef T7_AIR_PUSH
    *wx=gx;*wy=gy;return 1;
}

static int clear_point_in_location(unsigned id,int *out_x,int *out_y)
{
    if(!id||id>sc64_t7_scenario.location_count||!out_x||!out_y)return 0;
    const Sc64T4Location *r=&sc64_t7_scenario.locations[id-1];
    int best=-0x7fffffff,bx=-1,by=-1;
    for(int y=8;y<WORLD_H;y+=16)for(int x=8;x<WORLD_W;x+=16){
        if(!point_in_location(id,x,y))continue;
        int nearest=0x7fffffff;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&i!=race_late.emitter_unit&&
           !SC64_TRANSPORT_LOADED(&u[i])){
            int d=dist2(x,y,u[i].x,u[i].y);if(d<nearest)nearest=d;
        }
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive){
            int d=dist2(x,y,b[i].x,b[i].y);if(d<nearest)nearest=d;
        }
        int risk=(int)air_cell_risk(x,y);
        int score=nearest-risk*8;
        if(score>best){best=score;bx=x;by=y;}
    }
    (void)r;
    if(bx<0)return 0;
    *out_x=bx;*out_y=by;
    fprintf(stderr,"T7 source location %u clear air point=%d,%d clearance2=%d\n",id,bx,by,best);
    return 1;
}

static int controller_air_deliver_emitter(int emitter)
{
    int ship=-1,carrier=race_late.emitter_carrier<MAX_UNITS?(int)race_late.emitter_carrier:-1;
    if(carrier<0||!controller_build_dropship(&ship)||ship<0)return 0;
    int starport=raw_building(114,SIDE);
    if(starport<0||!u[carrier].alive||!u[ship].alive)return 0;
    int sx=b[starport].x,sy=b[starport].y;
    if(dist2(u[carrier].x,u[carrier].y,sx,sy)>10*10){
        if(!order_unit(carrier,sx,sy))return 0;
        for(int t=0;t<5000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
            step(0);
            if(!u[carrier].alive||!u[ship].alive)return 0;
            if(dist2(u[carrier].x,u[carrier].y,sx,sy)<10*10)break;
        }
    }
    if(!select_unit(carrier))return 0;
    move_cursor(u[ship].x,u[ship].y);tap(KEY_A);
    for(int t=0;t<3000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
        step(0);
        if(!u[ship].alive||!u[carrier].alive)return 0;
        if(SC64_TRANSPORT_LOADED(&u[carrier]))break;
    }
    if(!SC64_TRANSPORT_LOADED(&u[carrier]))return 0;
    fprintf(stderr,"T7 emitter SCV loaded ship=%d carrier=%d\n",ship,carrier);
    last_carrier_slot=carrier;checkpoint("emitter-loaded-dropship");
    int goal_x=-1,goal_y=-1;
    if(!clear_point_in_location(1,&goal_x,&goal_y))return 0;
    for(int leg=0;leg<128&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;leg++){
        if(point_in_location(1,u[ship].x,u[ship].y))break;
        int wx,wy;
        if(!air_next_waypoint(u[ship].x,u[ship].y,goal_x,goal_y,&wx,&wy)){
            fprintf(stderr,"T7 no safe air route ship=%d at=%d,%d goal=%d,%d\n",ship,u[ship].x,u[ship].y,goal_x,goal_y);
            return 0;
        }
        fprintf(stderr,"T7 Dropship leg=%d at=%d,%d target=%d,%d goal=%d,%d\n",leg,u[ship].x,u[ship].y,wx,wy,goal_x,goal_y);
        if(!order_unit(ship,wx,wy))return 0;
        int arrived=0;
        for(int t=0;t<4000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
            step(0);
            if(!u[ship].alive||!u[carrier].alive)return 0;
            if(dist2(u[ship].x,u[ship].y,wx,wy)<12*12){arrived=1;break;}
        }
        if(!arrived)return 0;
    }
    if(!point_in_location(1,u[ship].x,u[ship].y))return 0;
    if(!point_in_location(1,u[ship].x,u[ship].y)){
        fprintf(stderr,"T7 Dropship did not enter location ship=%d alive=%d pos=%d,%d order=%d target=%d,%d\n",
          ship,u[ship].alive,u[ship].x,u[ship].y,u[ship].order,u[ship].tx,u[ship].ty);
        return 0;
    }
    fprintf(stderr,"T7 Dropship reached goal ship=%d pos=%d,%d cargo=%d selected=%d count=%d mode=%d menu=%d\n",
      ship,u[ship].x,u[ship].y,race_transport_cargo_space(ship),u[ship].selected,selected_count(),mode,menu_open);
    if(!select_unit(ship)){
        fprintf(stderr,"T7 Dropship select for unload failed selected=%d count=%d building=%d mode=%d menu=%d\n",
          u[ship].selected,selected_count(),selected_building,mode,menu_open);
        return 0;
    }
    tap(KEY_B);for(int i=0;i<13;i++)tap(KEY_DOWN);tap(KEY_A);
    int gx=goal_x,gy=goal_y;
    move_cursor(gx,gy);tap(KEY_A);
    for(int t=0;t<5000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
        step(0);
        if(!u[carrier].alive)return 0;
        if(!SC64_TRANSPORT_LOADED(&u[carrier])&&point_in_location(1,u[emitter].x,u[emitter].y)){
            checkpoint("emitter-at-goal");return 1;
        }
    }
    return 0;
}

static int nearest_hostile_ground_threat(int x,int y,int max_distance2)
{
    int best=max_distance2,slot=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&owners_hostile(SIDE,u[i].owner)&&
       sc64_race_unit_damage_vs(u[i].type,0)>0){
        int d=dist2(x,y,u[i].x,u[i].y);
        if(d<best){best=d;slot=i;}
    }
    return slot;
}

static void escort_clear_local_threat(int carrier)
{
    if(carrier<0||carrier>=MAX_UNITS||!u[carrier].alive)return;
    int threat=nearest_hostile_ground_threat(u[carrier].x,u[carrier].y,512*512);
    if(threat<0)return;
    int x=u[threat].x,y=u[threat].y;
    fprintf(stderr,"T7 escort clearing threat slot=%d raw=%d type=%d hp=%d at=%d,%d carrier=%d at=%d,%d\n",
      threat,u[threat].raw_type,u[threat].type,u[threat].hp,x,y,carrier,u[carrier].x,u[carrier].y);
    (void)order_visible_type(U_MARINE,x,y);
    (void)order_visible_type(U_VULTURE,x,y);
    (void)order_visible_type(U_FIREBAT,x,y);
    checkpoint("escort-threat-ordered");
    for(int t=0;t<1800&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
        step(0);
        if(!u[carrier].alive||!u[threat].alive)break;
    }
    fprintf(stderr,"T7 escort threat result slot=%d alive=%d hp=%d carrier_alive=%d side=%d enemy=%d\n",
      threat,u[threat].alive,u[threat].hp,u[carrier].alive,owner_units(SIDE),owner_units(ENEMY));
    checkpoint("escort-threat-result");
}

static int controller_deliver_emitter(void)
{
    if(race_late.emitter_unit>=MAX_UNITS||!u[race_late.emitter_unit].alive)return 0;
    int emitter=race_late.emitter_unit;
    for(int relay=0;relay<24&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;relay++){
        if(point_in_location(1,u[emitter].x,u[emitter].y))return 1;
        int carrier=race_late.emitter_carrier;
        if(carrier>=MAX_UNITS||!u[carrier].alive||u[carrier].owner!=SIDE){
            carrier=nearest_pickup_actor(emitter);
            if(carrier<0)return 0;
            fprintf(stderr,"T7 relay=%d send actor=%d raw=%d at=%d,%d to dropped emitter=%d at=%d,%d\n",
              relay,carrier,u[carrier].raw_type,u[carrier].x,u[carrier].y,emitter,u[emitter].x,u[emitter].y);
            int dx=u[carrier].x-u[emitter].x,dy=u[carrier].y-u[emitter].y;
            if(dx < -8 || dx > 8 || dy < -8 || dy > 8){
                if(!order_unit(carrier,u[emitter].x,u[emitter].y))return 0;
            }
            for(int t=0;t<4000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
                step(0);
                if(!u[carrier].alive)break;
                if(race_late.emitter_carrier==carrier)break;
            }
            if(!u[carrier].alive||race_late.emitter_carrier!=carrier)continue;
            last_carrier_slot=carrier;
            fprintf(stderr,"T7 relay pickup actor=%d raw=%d tick=%u\n",carrier,u[carrier].raw_type,(unsigned)host_tick);
            checkpoint("emitter-relay-picked-up");
        }
        int gx=-1,gy=-1,distance=reachable_location_point(u[carrier].x,u[carrier].y,1,&gx,&gy);
        fprintf(stderr,"T7 delivery carrier=%d raw=%d at=%d,%d location1 route=%d target=%d,%d\n",
          carrier,u[carrier].raw_type,u[carrier].x,u[carrier].y,distance,gx,gy);
        if(distance<0)return 0;
        int wx=gx,wy=gy;
        /* Move armed groups to a short point ahead before the slow SCV. This
           uses only ordinary attack-move commands and keeps the escort near. */
        escort_clear_local_threat(carrier);
        if(!u[carrier].alive||race_late.emitter_carrier!=carrier)continue;
        if(!order_unit(carrier,wx,wy))return 0;
        (void)order_visible_type(U_MARINE,wx,wy);
        (void)order_visible_type(U_VULTURE,wx,wy);
        (void)order_visible_type(U_FIREBAT,wx,wy);
        checkpoint("emitter-delivery-ordered");
        for(int t=0;t<1500&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
            step(0);
            if(!u[emitter].alive)return 0;
            if(point_in_location(1,u[emitter].x,u[emitter].y)){
                fprintf(stderr,"T7 emitter entered source location1 tick=%u at=%d,%d\n",(unsigned)host_tick,u[emitter].x,u[emitter].y);
                checkpoint("emitter-at-goal");return 1;
            }
            if(!u[carrier].alive||race_late.emitter_carrier!=carrier)break;
            if(dist2(u[carrier].x,u[carrier].y,wx,wy)<100)break;
        }
    }
    fprintf(stderr,"T7 emitter relay budget exhausted at tick=%u slot=%d alive=%d pos=%d,%d carrier=%u\n",
      (unsigned)host_tick,emitter,u[emitter].alive,u[emitter].x,u[emitter].y,(unsigned)race_late.emitter_carrier);
    return point_in_location(1,u[emitter].x,u[emitter].y);
}

static void progress_until_terminal(void)
{
    int emitter=-1;
    for(int i=0;i<tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;i++){
        step(0);
        emitter=raw_unit(217,SIDE);
        if(emitter>=0)break;
    }
    if(emitter>=0){
        checkpoint("emitter-created");
        if(controller_collect_emitter(emitter)){
            controller_assign_gas_workers();
            if(!controller_air_deliver_emitter(emitter))
                controller_deliver_emitter();
        }
    }
    for(int i=0;i<5000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;i++)step(0);
}

static void inspect_world(void)
{
    fprintf(stderr,"T7 dims=%dx%d units SIDE=%d ENEMY=%d buildings SIDE=%d ENEMY=%d faults=%d emitter=%u carrier=%u Kerrigan=%d\n",
      race_cols,race_rows,owner_units(SIDE),owner_units(ENEMY),owner_buildings(SIDE),owner_buildings(ENEMY),mission_unsupported_count,
      (unsigned)race_late.emitter_unit,(unsigned)race_late.emitter_carrier,kerrigan_slot);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive)
        fprintf(stderr,"unit slot=%d raw=%d owner=%d rawowner=%d type=%d pos=%d,%d hp=%d order=%d target=%d\n",i,u[i].raw_type,u[i].owner,u[i].raw_owner,u[i].type,u[i].x,u[i].y,u[i].hp,u[i].order,u[i].target);
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive)
        fprintf(stderr,"building slot=%d raw=%d owner=%d rawowner=%d type=%d pos=%d,%d hp=%d\n",i,b[i].raw_type,b[i].owner,b[i].raw_owner,b[i].type,b[i].x,b[i].y,b[i].hp);
    for(unsigned i=0;i<sc64_t7_scenario.location_count;i++){
        const Sc64T4Location *p=&sc64_t7_scenario.locations[i];
        fprintf(stderr,"loc id=%u bounds=%u,%u..%u,%u center=%u,%u\n",i+1,p->left,p->top,p->right,p->bottom,(p->left+p->right)/2,(p->top+p->bottom)/2);
    }
}

int main(int argc,char **argv)
{
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--limit")&&i+1<argc)tick_limit=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--trace")&&i+1<argc)trace_path=argv[++i];
        else if(!strcmp(argv[i],"--inspect"))inspect_only=1;
        else if(!strcmp(argv[i],"--help")){puts("usage: sc64_t7_campaign_run [--limit TICKS] [--trace PATH] [--inspect]");return 0;}
        else return 2;
    }
    if(tick_limit<1||tick_limit>200000)return 2;
    game_init();
    assert(screen==SCREEN_PLAY&&!race_mode&&race_selected_mission==SC64_TERRAN_LATE_GLOBAL_T7&&race_late_active());
    assert(!won&&!lost&&!mission_unsupported_count);
    kerrigan_slot=raw_unit(16,-1);
    if(inspect_only){wait_frames(1);inspect_world();return 0;}
    if(trace_path){trace_file=fopen(trace_path,"wb");if(!trace_file)return 2;
        fprintf(trace_file,"{\"trace_schema\":1,\"entry\":\"t7-probe-entry\",\"policy\":\"bounded-normal-controller-attempt\",\"profile\":\"terran-t7\",\"mission\":10,\"tick_limit\":%d,\"events\":[\n",tick_limit);}
    checkpoint("start");wait_frames(1);checkpoint("opening");
    kerrigan_slot=raw_unit(16,-1);
    if(kerrigan_slot>=0)checkpoint("kerrigan-identified");
    prepare_marine_escort();
    preclear_route_threat();
    prepare_marine_escort();
    /* Mission logic moves source location 4 to a human SCV, then creates the
       source Psi Emitter there. The controller collects it and walks it to
       location 1 with ordinary input. */
    progress_until_terminal();
    if(trace_file){checkpoint(won?"won":lost?"lost":"limit");fputs("\n]}\n",trace_file);fclose(trace_file);}
    int emitter=race_late.emitter_unit<MAX_UNITS?(int)race_late.emitter_unit:-1;
    int emitter_at_goal=emitter>=0&&u[emitter].alive&&point_in_location(1,u[emitter].x,u[emitter].y);
    fprintf(stderr,"T7 result tick=%u frame=%u won=%d lost=%d faults=%d Kerrigan=%d alive=%d emitter=%d alive=%d at_goal=%d carrier=%u side=%d enemy=%d\n",
      (unsigned)host_tick,(unsigned)frame,won,lost,mission_unsupported_count,kerrigan_slot,
      kerrigan_slot>=0&&u[kerrigan_slot].alive,emitter,emitter>=0&&u[emitter].alive,emitter_at_goal,
      (unsigned)race_late.emitter_carrier,owner_units(SIDE),owner_units(ENEMY));
    for(unsigned p=0;p<8;p++)if(race_late.triggers.player[p].fault||race_late.triggers.player[p].outcome){
        unsigned ft=race_late.triggers.player[p].fault_trigger;
        unsigned cursor=ft<SC64_TERRAN_LATE_TRIGGER_LIMIT?
            race_late.triggers.player[p].cursor[ft]:255u;
        fprintf(stderr,"T7 source-player=%u fault=%u trigger=%u slot=%u outcome=%u cursor=%u cooldown=%u wait=%u\n",
          p,race_late.triggers.player[p].fault,ft,race_late.triggers.player[p].fault_slot,
          race_late.triggers.player[p].outcome,cursor,
          race_late.triggers.player[p].cooldown,race_late.triggers.player[p].wait_ticks);
    }
    return won&&!lost&&mission_unsupported_count==0&&kerrigan_slot>=0&&u[kerrigan_slot].alive&&emitter_at_goal?0:1;
}
