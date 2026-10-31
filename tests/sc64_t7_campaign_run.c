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

static int scv_bbox_overlaps_location(unsigned id, int x, int y)
{
    Sc64TerranLateRect area,actor;
    return sc64_terran_late_location_rect(&race_late,id,&area) &&
           sc64_terran_late_actor_rect(7,0,x,y,&actor) &&
           sc64_terran_late_rect_overlap(&area,&actor);
}

static int emitter_overlaps_location(unsigned id, int emitter)
{
    Sc64TerranLateRect area,actor;
    return emitter>=0&&emitter<MAX_UNITS&&u[emitter].alive&&
           u[emitter].raw_type==217&&
           sc64_terran_late_location_rect(&race_late,id,&area)&&
           sc64_terran_late_actor_rect(217,0,u[emitter].x,u[emitter].y,&actor)&&
           sc64_terran_late_rect_overlap(&area,&actor);
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
      emitter_overlaps_location(1,emitter),
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

/* Field controls reopen the order list on the last command. Return to
 * Move using bounded normal inputs before commands that expect that row. */
static void tap_b(void)
{
    int fresh = !menu_open && mode == MODE_NORMAL;
    tap(KEY_B);
    for (int guard=0;fresh&&menu_open&&menu_cursor!=MENU_MOVE&&
         guard<field_menu_count()&&host_tick<(u32)tick_limit;guard++) tap(KEY_UP);
}

static void move_cursor(int x,int y)
{
    x=clamp_world(x,WORLD_W);y=clamp_world(y,WORLD_H);
    for(int n=0;n<4000&&(cx<x-2||cx>x+2||cy<y-2||cy>y+2)&&host_tick<(u32)tick_limit;n++){
        u16 held=KEY_R;if(cx<x-2)held|=KEY_RIGHT;if(cx>x+2)held|=KEY_LEFT;
        if(cy<y-2)held|=KEY_DOWN;if(cy>y+2)held|=KEY_UP;step(held);
    }
    /* R moves the cursor at four pixels per frame.  Exact D-pad correction
       cannot reach the opposite pixel parity at its two-pixel speed; when
       that happens the old helper burned the entire 2,000-frame budget. */
}

static int normal_field(void)
{
    for(int i=0;i<2&&(menu_open||mode!=MODE_NORMAL);i++)tap_b();
    return screen==SCREEN_PLAY&&!menu_open&&mode==MODE_NORMAL;
}

static int leave_building_selection(void)
{
    if(selected_building<0)return 1;
    if(menu_open||mode!=MODE_NORMAL)return 0;
    /* The engine's L gesture cancels a Barracks queue instead of cycling
       away. Select and commit an ordinary no-unit MOVE to clear it. */
    tap_b();tap(KEY_A);tap(KEY_A);
    return selected_building<0&&mode==MODE_NORMAL&&!menu_open;
}

static int select_unit(int slot)
{
    if(slot<0||slot>=MAX_UNITS||!u[slot].alive||u[slot].owner!=SIDE||!normal_field())return 0;
    if(!leave_building_selection())return 0;
    if(u[slot].selected&&selected_count()==1&&selected_building<0)return 1;
    static const int dx[8]={-16,16,0,0,-24,24,0,0};
    static const int dy[8]={0,0,-16,16,0,0,-24,24};
    for(int attempt=0;attempt<=8&&u[slot].alive;attempt++){
        int x=u[slot].x,y=u[slot].y;
        if(attempt){x=clamp_world(x+dx[attempt-1],WORLD_W);
                    y=clamp_world(y+dy[attempt-1],WORLD_H);}
        move_cursor(x,y);
        if(selection_target()==slot){tap(KEY_A);
            if(u[slot].alive&&u[slot].selected&&selected_count()==1&&selected_building<0)return 1;}
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
    tap_b();tap(KEY_A);move_cursor(x,y);
    int cursor_dx=cx-clamp_world(x,WORLD_W),cursor_dy=cy-clamp_world(y,WORLD_H);
    if(cursor_dx < -2 || cursor_dx > 2 || cursor_dy < -2 || cursor_dy > 2){
        fprintf(stderr,"T7 move cursor failed slot=%d cursor=%d,%d target=%d,%d\n",
          slot,cx,cy,x,y);return 0;
    }
    tap(KEY_A);
    if(!u[slot].alive||u[slot].order!=ORDER_MOVE){
        fprintf(stderr,"T7 move command not registered slot=%d alive=%d order=%d at=%d,%d intended=%d,%d cursor=%d,%d\n",
          slot,u[slot].alive,u[slot].order,u[slot].x,u[slot].y,x,y,cx,cy);
        return 0;
    }
    return 1;
}

static int order_attack_unit(int slot,int x,int y)
{
    if(!select_unit(slot))return 0;
    tap_b();tap(KEY_DOWN);tap(KEY_A);move_cursor(x,y);tap(KEY_A);return 1;
}

static int order_every_type(int type,int x,int y,int attack)
{
    int ordered=0,eligible=0;
    for(int i=0;i<MAX_UNITS&&host_tick<(u32)tick_limit&&
        screen==SCREEN_PLAY&&!won&&!lost;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type&&
           !SC64_TRANSPORT_LOADED(&u[i])){
            ++eligible;
            int intended_order=attack?ORDER_ATTACK_MOVE:ORDER_MOVE;
            if(u[i].order==intended_order&&u[i].tx==x&&u[i].ty==y){++ordered;continue;}
            int ok=attack?order_attack_unit(i,x,y):order_unit(i,x,y);
            if(ok)++ordered;
            else fprintf(stderr,"T7 per-unit order failed type=%d slot=%d pos=%d,%d\n",
              type,i,u[i].x,u[i].y);
        }
    fprintf(stderr,"T7 per-unit normal %s type=%d ordered=%d/%d target=%d,%d\n",
      attack?"attack-move":"move",type,ordered,eligible,x,y);
    return ordered;
}

static int order_visible_type_mode(int type,int x,int y,int attack)
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
    tap_b();if(attack)tap(KEY_DOWN);tap(KEY_A);move_cursor(x,y);tap(KEY_A);
    fprintf(stderr,"T7 escorted group type=%d seed=%d count=%d order=%s target=%d,%d\n",
      type,seed,count,attack?"attack-move":"move",x,y);
    return 1;
}

static int order_visible_type(int type,int x,int y)
{ return order_visible_type_mode(type,x,y,1); }

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
    if(!path_grid_valid)return -1;
    memset(closed,0,sizeof(closed));memset(parent,0xff,sizeof(parent));
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
        if(scv_bbox_overlaps_location(id,px,py)){goal=c;break;}
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

static int controller_assign_minerals(void)
{
    static const int dx[]={0,8,-8,0,0,8,-8,8,-8},dy[]={0,0,0,8,-8,8,8,-8,-8};
    int cc=raw_building(106,SIDE),assigned=0;
    if(cc<0||!building_complete(cc))return 0;
    for(int i=0;i<MAX_UNITS&&host_tick<(u32)tick_limit;i++) {
        if(!u[i].alive||u[i].owner!=SIDE||u[i].raw_type!=7||u[i].order!=ORDER_IDLE||u[i].carry||
           i==(int)race_late.emitter_carrier||SC64_TRANSPORT_LOADED(&u[i])||!select_unit(i))continue;
        for(int j=0;j<MAX_MINERALS&&u[i].order==ORDER_IDLE;j++) {
            if(m[j].amount<=0||!currently_visible(m[j].x,m[j].y)||dist2(m[j].x,m[j].y,b[cc].x,b[cc].y)>256*256)continue;
            for(unsigned k=0;k<sizeof(dx)/sizeof(dx[0]);k++) {
                move_cursor(m[j].x+dx[k],m[j].y+dy[k]);
                if(!u[i].alive||!u[i].selected||u[i].carry)break;
                if(selected_worker_mineral_target()!=j||field_ui_a_action(selected_count())!=UI_A_MINE)continue;
                tap(KEY_A);
                if(u[i].order==ORDER_MINE&&u[i].target==j){assigned++;fprintf(stderr,"T7 mineral SCV=%d patch=%d\n",i,j);break;}
            }
        }
    }
    return assigned;
}

static void controller_assign_gas_workers(void)
{
    int refinery=raw_building(110,SIDE);
    if(refinery<0||!building_complete(refinery))return;
    int assigned=0;
    for(int n=0;n<3&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;n++){
        int worker=-1,best=0x7fffffff;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==7&&
           i!=(int)race_late.emitter_carrier&&u[i].order!=ORDER_GAS&&!u[i].carry&&
           !SC64_TRANSPORT_LOADED(&u[i])){
            int d=dist2(u[i].x,u[i].y,b[refinery].x,b[refinery].y);
            if(d<best){best=d;worker=i;}
        }
        if(worker<0)break;
        if(!u[worker].alive||!select_unit(worker))continue;
        move_cursor(b[refinery].x,b[refinery].y);if(!u[worker].alive||!u[worker].selected||u[worker].carry)continue;tap(KEY_A);
        if(u[worker].order==ORDER_GAS&&u[worker].target==refinery){
            ++assigned;
            fprintf(stderr,"T7 assigned SCV=%d to refinery=%d\n",worker,refinery);
        }
    }
    fprintf(stderr,"T7 gas assignment count=%d wallet=%d/%d\n",assigned,minerals,gas);
    checkpoint("gas-workers-assigned");
}

static int current_count_type(int type)
{
    int count=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type)
        ++count;
    return count;
}

static int queue_starport_type(int type)
{
    int starport=raw_building(114,SIDE);
    if(starport<0||!building_complete(starport)||!normal_field())return 0;
    if(selected_building!=starport){
        if(!leave_building_selection())return 0;
        move_cursor(b[starport].x,b[starport].y);tap(KEY_A);
    }
    if(selected_building!=starport||selected_count())return 0;
    for(int n=0;n<5&&train_type!=type;n++)tap(KEY_R);
    if(train_type!=type){
        fprintf(stderr,"T7 Starport product unavailable by normal menu request=%d selected_train=%d raw_owner=%d\n",
          type,train_type,b[starport].raw_owner);
        (void)leave_building_selection();return 0;
    }
    int before=b[starport].queue_count;
    tap(KEY_A);
    int tail=(b[starport].queue_head+before)%5;
    if(b[starport].queue_count<=before||b[starport].queue[tail]!=type){
        fprintf(stderr,"T7 Starport rejected normal product request=%d train=%d count=%d->%d wallet=%d/%d\n",
          type,train_type,before,b[starport].queue_count,minerals,gas);
        (void)leave_building_selection();return 0;
    }
    fprintf(stderr,"T7 paid Starport product type=%d queue=%d wallet=%d/%d\n",
      type,b[starport].queue_count,minerals,gas);
    checkpoint(type==U_WRAITH?"wraith-queued":"dropship-queued");
    return leave_building_selection();
}

static int build_wraith_escorts(int requested)
{
    int initial=current_count_type(U_WRAITH),built=0;
    int starport=raw_building(114,SIDE);
    if(starport<0||!building_complete(starport)||!normal_field())return 0;
    if(selected_building!=starport){
        if(!leave_building_selection())return 0;
        move_cursor(b[starport].x,b[starport].y);tap(KEY_A);
    }
    if(selected_building!=starport||selected_count())return 0;
    for(int n=0;n<5&&train_type!=U_WRAITH;n++)tap(KEY_R);
    if(train_type!=U_WRAITH){
        fprintf(stderr,"T7 Starport Wraith product unavailable by normal menu raw_owner=%d train=%d\n",
          b[starport].raw_owner,train_type);
        (void)leave_building_selection();return 0;
    }
    for(int n=0;n<requested&&
        host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;n++){
        for(int t=0;t<12000&&b[starport].queue_count>=5&&
            host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++)step(0);
        for(int t=0;t<12000&&(minerals<150||gas<100)&&host_tick<(u32)tick_limit&&
            screen==SCREEN_PLAY&&!won&&!lost;t++)step(0);
        if(b[starport].queue_count>=5||minerals<150||gas<100)break;
        int before=b[starport].queue_count;
        int tail=(b[starport].queue_head+before)%5;
        tap(KEY_A);
        if(b[starport].queue_count<=before||b[starport].queue[tail]!=U_WRAITH){
            fprintf(stderr,"T7 Starport rejected normal Wraith queue wallet=%d/%d supply=%d/%d\n",
              minerals,gas,supply_used,supply_cap);break;
        }
        ++built;
        fprintf(stderr,"T7 paid Starport Wraith queue=%d wallet=%d/%d supply=%d/%d\n",
          b[starport].queue_count,minerals,gas,supply_used,supply_cap);
    }
    (void)leave_building_selection();
    int expected=initial+built;
    for(int t=0;t<10000&&current_count_type(U_WRAITH)<expected&&
        host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++)step(0);
    fprintf(stderr,"T7 normal-input Wraith escort count=%d tick=%u wallet=%d/%d\n",
      built,(unsigned)host_tick,minerals,gas);
    return built;
}

static int air_next_waypoint(int sx,int sy,int gx,int gy,int *wx,int *wy);
static int clear_point_in_location(unsigned id,int *out_x,int *out_y);

static int dist2_to_segment(int x,int y,int ax,int ay,int bx,int by)
{
    long long dx=bx-ax,dy=by-ay,len=dx*dx+dy*dy;
    if(!len)return dist2(x,y,ax,ay);
    long long dot=(long long)(x-ax)*dx+(long long)(y-ay)*dy;
    if(dot<=0)return dist2(x,y,ax,ay);
    if(dot>=len)return dist2(x,y,bx,by);
    long long cross=(long long)(x-ax)*dy-(long long)(y-ay)*dx;
    return (int)(cross*cross/len);
}

/* Late missions can collapse several source players into one live ENEMY
   owner. Use the authored source-force table when deciding whether a unit is
   a real threat to the human force. */
static int t7_hostile_source_unit(int slot)
{
    if(slot<0||slot>=MAX_UNITS||!u[slot].alive||u[slot].owner==SIDE||
       u[slot].raw_owner<0||u[slot].raw_owner>=8)return 0;
    const Sc64T4SourceSetup *setup=
        sc64_terran_late_setup_for_mission(race_selected_mission);
    int source=sc64_terran_late_current_source_owner(u[slot].owner,u[slot].raw_owner);
    int human=sc64_terran_late_source_slot_for_controller(
        setup,SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
    if(source<0||source>=8||human<0)return 0;
    int force=sc64_terran_late_source_force((unsigned)source);
    int human_force=sc64_terran_late_source_force((unsigned)human);
    return force>0&&human_force>0&&force!=human_force&&
           sc64_terran_late_source_enumerated((unsigned)source);
}

static int t7_hostile_source_building(int slot)
{
    if(slot<0||slot>=MAX_BUILDINGS||!b[slot].alive||b[slot].owner==SIDE||
       b[slot].raw_owner<0||b[slot].raw_owner>=8)return 0;
    const Sc64T4SourceSetup *setup=
        sc64_terran_late_setup_for_mission(race_selected_mission);
    int source=sc64_terran_late_current_source_owner(b[slot].owner,b[slot].raw_owner);
    int human=sc64_terran_late_source_slot_for_controller(
        setup,SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
    if(source<0||source>=8||human<0)return 0;
    int force=sc64_terran_late_source_force((unsigned)source);
    int human_force=sc64_terran_late_source_force((unsigned)human);
    return force>0&&human_force>0&&force!=human_force&&
           sc64_terran_late_source_enumerated((unsigned)source);
}

static int t7_source_wraith_can_attack(int slot)
{
    return t7_hostile_source_unit(slot)&&
           SC64_CAN_ATTACK_FROM_SOURCE(U_WRAITH,slot,SIDE,u[slot].raw_owner);
}

static int clear_route_goliath_with_wraiths(void)
{
    int cx=640,cy=520,goalx=1880,goaly=840,replenished=0;
    (void)clear_point_in_location(1,&goalx,&goaly);
    for(int outer=0;outer<8&&host_tick<(u32)tick_limit;outer++){
        int n=0;cx=cy=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_WRAITH){
            cx+=u[i].x;cy+=u[i].y;++n;
        }
        if(n){cx/=n;cy/=n;}
        int threat=-1,best=0x7fffffff,final_threat=-1,final_best=0x7fffffff;
        Sc64StockAttack goliath_air;
        int threat_radius=80;
        if(sc64_stock_attack_select(3,1,&goliath_air))
            threat_radius=goliath_air.weapon->max_range/2+32;
        for(int i=0;i<MAX_UNITS;i++)if(t7_source_wraith_can_attack(i)&&
           u[i].raw_type==3){
            int d=dist2(cx,cy,u[i].x,u[i].y);
            int to_goal=dist2(u[i].x,u[i].y,goalx,goaly);
            int route_d=dist2_to_segment(u[i].x,u[i].y,640,520,goalx,goaly);
            if(route_d<=threat_radius*threat_radius&&d<best){best=d;threat=i;}
            if(to_goal<=threat_radius*threat_radius&&to_goal<final_best){final_best=to_goal;final_threat=i;}
        }
        if(threat<0)threat=final_threat;
        if(threat<0)return 1;
        if(!n){
            if(replenished<2){
                fprintf(stderr,"T7 Wraith wave spent; hold cargo and queue a fresh normal-input escort wave\n");
                int fresh=build_wraith_escorts(4);
                if(fresh>0){++replenished;continue;}
            }
            fprintf(stderr,"T7 no surviving Wraiths for further Goliath preclear\n");return 0;
        }
        int near_goal=dist2(u[threat].x,u[threat].y,goalx,goaly)<=threat_radius*threat_radius;
        fprintf(stderr,"T7 hold cargo; attack-move route Goliath raw3 slot=%d at=%d,%d hp=%d Wraiths=%d\n",
          threat,u[threat].x,u[threat].y,u[threat].hp,n);
        if(!u[threat].alive)continue;
        if(current_count_type(U_WRAITH)==0)return 0;
        fprintf(stderr,"T7 Wraiths attack route Goliath raw3 slot=%d at=%d,%d hp=%d count=%d\n",
          threat,u[threat].x,u[threat].y,u[threat].hp,current_count_type(U_WRAITH));
        int group_attack=order_visible_type_mode(U_WRAITH,u[threat].x,u[threat].y,1);
        int individual_attack=order_every_type(U_WRAITH,u[threat].x,u[threat].y,1);
        if(!group_attack&&!individual_attack)return 0;
        checkpoint("goliath-attack-ordered");
        for(int t=0;t<(near_goal?12000:6000)&&u[threat].alive&&
            current_count_type(U_WRAITH)>0&&host_tick<(u32)tick_limit&&
            screen==SCREEN_PLAY&&!won&&!lost;t++){
            step(0);
            if((host_tick%1000)==0){
                checkpoint("goliath-attack-progress");
                fprintf(stderr,"T7 Wraith preclear progress target_hp=%d alive=%d",
                  u[threat].hp,u[threat].alive);
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_WRAITH)
                    fprintf(stderr," wraith%d@%d,%d/hp%d/order%d",i,u[i].x,u[i].y,u[i].hp,u[i].order);
                fputc('\n',stderr);
            }
        }
        fprintf(stderr,"T7 Goliath preclear result alive=%d hp=%d Wraiths=%d tick=%u\n",
          u[threat].alive,u[threat].hp,current_count_type(U_WRAITH),(unsigned)host_tick);
        checkpoint(u[threat].alive?"goliath-still-alive":"goliath-cleared");
        if(u[threat].alive&&current_count_type(U_WRAITH)>0)return 0;
    }
    return 0;
}

static int build_marine_force(int target_count)
{
    for(int t=0;t<20000&&current_count_type(U_MARINE)<target_count&&
        host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
        if(t%1000==0)(void)controller_assign_minerals();
        int barracks=raw_building(111,SIDE);
        if(barracks<0||!building_complete(barracks))return 0;
        if(b[barracks].queue_count<5&&minerals>=50&&
           supply_used+unit_supply(U_MARINE)<=supply_cap){
            int before=current_count_type(U_MARINE);
            if(!queue_marine())break;
            /* Avoid reselecting the Barracks for every item while it is full. */
            if(b[barracks].queue_count>=5&&current_count_type(U_MARINE)==before)
                while(b[barracks].queue_count>=5&&host_tick<(u32)tick_limit&&
                      screen==SCREEN_PLAY&&!won&&!lost)step(0);
        }else step(0);
    }
    for(int t=0;t<8000&&current_count_type(U_MARINE)<target_count&&
        host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++)step(0);
    fprintf(stderr,"T7 ordinary Marine force count=%d target=%d tick=%u wallet=%d/%d\n",
      current_count_type(U_MARINE),target_count,(unsigned)host_tick,minerals,gas);
    return current_count_type(U_MARINE)>=target_count;
}

static int clear_route_goliath_with_ground_force(void)
{
    int threat=-1,best=0x7fffffff;
    int goalx=1880,goaly=840;
    (void)clear_point_in_location(1,&goalx,&goaly);
    Sc64StockAttack goliath_air;
    int threat_radius=112;
    if(sc64_stock_attack_select(3,1,&goliath_air))
        threat_radius=goliath_air.weapon->max_range/2+32;
    for(int i=0;i<MAX_UNITS;i++)if(t7_hostile_source_unit(i)&&
       u[i].raw_type==3){
        int d=dist2(u[i].x,u[i].y,goalx,goaly);
        if(d<best){best=d;threat=i;}
    }
    if(threat<0||best>threat_radius*threat_radius)return 1;
    if(!build_marine_force(8)){
        fprintf(stderr,"T7 source army cannot reach eight Marines for safe Goliath preclear\n");
        return 0;
    }
    int tx=u[threat].x,ty=u[threat].y;
    int carrier=race_late.emitter_carrier<MAX_UNITS?(int)race_late.emitter_carrier:-1;
    int stage_x=carrier>=0&&u[carrier].alive?u[carrier].x:372;
    int stage_y=carrier>=0&&u[carrier].alive?u[carrier].y:699;
    fprintf(stderr,"T7 gather escorts at safe SCV staging point=%d,%d before attacking raw3=%d at=%d,%d hp=%d\n",
      stage_x,stage_y,threat,tx,ty,u[threat].hp);
    int marines=order_visible_type_mode(U_MARINE,stage_x,stage_y,0)+
        order_every_type(U_MARINE,stage_x,stage_y,0);
    int vultures=order_visible_type_mode(U_VULTURE,stage_x,stage_y,0)+
        order_every_type(U_VULTURE,stage_x,stage_y,0);
    int firebats=order_visible_type_mode(U_FIREBAT,stage_x,stage_y,0)+
        order_every_type(U_FIREBAT,stage_x,stage_y,0);
    fprintf(stderr,"T7 ground escort gather orders Marines=%d Vultures=%d Firebats=%d Kerrigan stays with SCV\n",
      marines,vultures,firebats);
    int total=current_count_type(U_MARINE)+current_count_type(U_VULTURE)+current_count_type(U_FIREBAT);
    int near=0;
    for(int t=0;t<12000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
        step(0);near=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
           (u[i].type==U_MARINE||u[i].type==U_VULTURE||u[i].type==U_FIREBAT)&&
           dist2(u[i].x,u[i].y,stage_x,stage_y)<128*128)++near;
        if(near>=(total*2+2)/3)break;
    }
    fprintf(stderr,"T7 ground escort staged near=%d/%d at=%d,%d tick=%u\n",
      near,total,stage_x,stage_y,(unsigned)host_tick);
    int marine_order=order_visible_type(U_MARINE,tx,ty);
    int vulture_order=order_visible_type(U_VULTURE,tx,ty);
    int firebat_order=order_visible_type(U_FIREBAT,tx,ty);
    /* Units outside the visible group receive individual ordinary
       attack-move orders after the gathering rally. */
    marine_order+=order_every_type(U_MARINE,tx,ty,1);
    vulture_order+=order_every_type(U_VULTURE,tx,ty,1);
    firebat_order+=order_every_type(U_FIREBAT,tx,ty,1);
    fprintf(stderr,"T7 Goliath preclear orders Marines=%d Vultures=%d Firebats=%d Kerrigan stays with staged SCV\n",
      marine_order,vulture_order,firebat_order);
    checkpoint("ground-goliath-attack-ordered");
    int last_hp=u[threat].hp,no_damage=0;
    for(int t=0;t<20000&&u[threat].alive&&host_tick<(u32)tick_limit&&
        screen==SCREEN_PLAY&&!won&&!lost;t++){
        step(0);
        if(u[threat].hp!=last_hp){last_hp=u[threat].hp;no_damage=0;}else if(++no_damage>=3000){
            fprintf(stderr,"T7 ground escort made no damage for 3000 frames; retain living force and stop unsafe push\n");
            break;
        }
        if((host_tick%1000)==0){
            checkpoint("ground-goliath-attack-progress");
            fprintf(stderr,"T7 ground preclear target_hp=%d alive=%d Marines=%d Vultures=%d Kerrigan_alive=%d\n",
              u[threat].hp,u[threat].alive,current_count_type(U_MARINE),
              current_count_type(U_VULTURE),kerrigan_slot>=0&&u[kerrigan_slot].alive);
        }
    }
    fprintf(stderr,"T7 ground Goliath preclear complete alive=%d hp=%d Marines=%d Vultures=%d Kerrigan_alive=%d tick=%u\n",
      u[threat].alive,u[threat].hp,current_count_type(U_MARINE),current_count_type(U_VULTURE),
      kerrigan_slot>=0&&u[kerrigan_slot].alive,(unsigned)host_tick);
    checkpoint(u[threat].alive?"ground-goliath-still-alive":"ground-goliath-cleared");
    return !u[threat].alive;
}

static int stage_emitter_carrier(void)
{
    int carrier=race_late.emitter_carrier;
    if(carrier<0||carrier>=MAX_UNITS||!u[carrier].alive||!select_unit(carrier))return 0;
    tap_b();for(int row=0;row<MENU_STOP;row++)tap(KEY_DOWN);tap(KEY_A);
    fprintf(stderr,"T7 staged emitter carrier with normal Stop input slot=%d alive=%d at=%d,%d order=%d tick=%u\n",
      carrier,u[carrier].alive,u[carrier].x,u[carrier].y,u[carrier].order,(unsigned)host_tick);
    return u[carrier].alive&&u[carrier].order==ORDER_IDLE;
}

static int controller_build_dropship(int *out_ship)
{
    if(out_ship)*out_ship=-1;
    int starport=raw_building(114,SIDE);
    if(starport<0||!building_complete(starport)||!normal_field())return 0;
    /* Keep the initial carrier beside the Starport while its source gas line
       reaches the ordinary Dropship cost. A bounded wait still leaves time
       for the ground route if the paid ship is unavailable. */
    for(int i=0;i<6000&&gas<100&&host_tick<(u32)tick_limit&&
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
    for(int i=0;i<MAX_UNITS;i++)if(t7_hostile_source_unit(i)&&
       !SC64_TRANSPORT_LOADED(&u[i])){
        Sc64StockAttack attack;
        int range=0,damage=0;
        if(sc64_stock_attack_select(u[i].raw_type,1,&attack)){
            range=attack.weapon->max_range/2;
            damage=sc64_stock_attack_damage_per_impact(&attack,0)*
                   (int)sc64_stock_attack_impact_count(&attack);
        }else{
            range=sc64_race_unit_range_vs(u[i].type,1);
            damage=sc64_race_unit_damage_vs(u[i].type,1);
        }
        if(range>0&&damage>0){
            int weapon=range,margin=weapon+32,d=dist2(x,y,u[i].x,u[i].y);
            if(d<margin*margin){
                risk+=(u32)damage*((u32)(margin*margin-d)/(u32)(margin*4)+1u);
                if(d<(weapon+32)*(weapon+32))risk+=4000u;
            }
        }
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(t7_hostile_source_building(i)&&building_complete(i)){
        if(b[i].type==B_MISSILE_TURRET||b[i].type==B_PHOTON_CANNON||
           b[i].type==B_SPORE_COLONY||b[i].type==B_BUNKER){
            int weapon=56,margin=weapon+32,d=dist2(x,y,b[i].x,b[i].y);
            if(d<margin*margin){
                risk+=(u32)(margin*margin-d)/(u32)(margin*4)+1u;
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
    /* Return a waypoint far enough ahead to move the Dropship beyond its
       current 16px path-grid cell. A one-cell corner waypoint can be inside
       the arrival radius while the ship remains in that same cell forever. */
    for(int k=length-1;k>=0;k--){
        int next=reverse[k],px=(next%MAP_COLS)*16+8,py=(next/MAP_COLS)*16+8;
        if(dist2(sx,sy,px,py)>=32*32){*wx=px;*wy=py;return 1;}
    }
#undef T7_AIR_POP
#undef T7_AIR_PUSH
    *wx=gx;*wy=gy;return 1;
}

static int clear_point_in_location(unsigned id,int *out_x,int *out_y)
{
    if(!id||id>sc64_t7_scenario.location_count||!out_x||!out_y)return 0;
    u32 best_risk=0xffffffffu;int best_clearance=-1,bx=-1,by=-1;
    for(int y=8;y<WORLD_H;y+=16)for(int x=8;x<WORLD_W;x+=16){
        if(!scv_bbox_overlaps_location(id,x,y))continue;
        int nearest=0x7fffffff;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&i!=race_late.emitter_unit&&
           !SC64_TRANSPORT_LOADED(&u[i])){
            int d=dist2(x,y,u[i].x,u[i].y);if(d<nearest)nearest=d;
        }
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive){
            int d=dist2(x,y,b[i].x,b[i].y);if(d<nearest)nearest=d;
        }
        u32 risk=air_cell_risk(x,y);int clearance=nearest>>8;
        if(risk<best_risk||(risk==best_risk&&clearance>best_clearance)){
            best_risk=risk;best_clearance=clearance;bx=x;by=y;
        }
    }
    if(bx<0)return 0;
    *out_x=bx;*out_y=by;
    fprintf(stderr,"T7 source location %u safest SCV-overlap point=%d,%d airrisk=%u clearance=%d\n",
      id,bx,by,(unsigned)best_risk,best_clearance);
    return 1;
}

static int controller_air_deliver_emitter(int emitter)
{
    int ship=-1,carrier=race_late.emitter_carrier<MAX_UNITS?(int)race_late.emitter_carrier:-1;
    if(carrier<0||!controller_build_dropship(&ship)||ship<0)return 0;
    int starport=raw_building(114,SIDE);
    if(starport<0||!u[carrier].alive||!u[ship].alive)return 0;
    if(!order_unit(ship,u[carrier].x,u[carrier].y))return 0;
    int close=0;
    for(int t=0;t<5000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
        step(0);
        if(!u[carrier].alive||!u[ship].alive)return 0;
        if(dist2(u[ship].x,u[ship].y,u[carrier].x,u[carrier].y)<24*24){close=1;break;}
    }
    if(!close){fprintf(stderr,"T7 Dropship failed to rendezvous ship=%d at=%d,%d SCV=%d at=%d,%d\n",
      ship,u[ship].x,u[ship].y,carrier,u[carrier].x,u[carrier].y);return 0;}
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
        if(dist2(u[ship].x,u[ship].y,goal_x,goal_y)<12*12)break;
        int wx,wy;
        if(!air_next_waypoint(u[ship].x,u[ship].y,goal_x,goal_y,&wx,&wy)){
            fprintf(stderr,"T7 no safe air route ship=%d at=%d,%d goal=%d,%d\n",ship,u[ship].x,u[ship].y,goal_x,goal_y);
            return 0;
        }
        fprintf(stderr,"T7 Dropship leg=%d at=%d,%d target=%d,%d goal=%d,%d\n",leg,u[ship].x,u[ship].y,wx,wy,goal_x,goal_y);
        /* The coarse air waypoint search can choose a waypoint already
           inside its arrival radius. Reissuing MOVE at the same point is a
           harmless no-op in the game, not evidence that input failed. */
        if(dist2(u[ship].x,u[ship].y,wx,wy)<12*12)continue;
        if(!order_unit(ship,wx,wy))return 0;
        int arrived=0;
        for(int t=0;t<4000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
            step(0);
            if(!u[ship].alive||!u[carrier].alive)return 0;
            if(dist2(u[ship].x,u[ship].y,wx,wy)<12*12){arrived=1;break;}
        }
        if(!arrived){
            fprintf(stderr,"T7 Dropship waypoint timeout ship=%d at=%d,%d target=%d,%d order=%d tx=%d ty=%d leg=%d\n",
              ship,u[ship].x,u[ship].y,wx,wy,u[ship].order,u[ship].tx,u[ship].ty,leg);
            checkpoint("dropship-waypoint-timeout");return 0;
        }
    }
    if(dist2(u[ship].x,u[ship].y,goal_x,goal_y)>=12*12){
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
    tap_b();for(int i=0;i<13;i++)tap(KEY_DOWN);tap(KEY_A);
    int gx=goal_x,gy=goal_y;
    move_cursor(gx,gy);tap(KEY_A);
    for(int t=0;t<5000&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
        step(0);
        if(!u[carrier].alive)return 0;
        if(!SC64_TRANSPORT_LOADED(&u[carrier])&&emitter_overlaps_location(1,emitter)){
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
    if(race_late.emitter_unit>=MAX_UNITS||!u[race_late.emitter_unit].alive){
        fprintf(stderr,"T7 ground delivery emitter unavailable slot=%u\n",(unsigned)race_late.emitter_unit);
        return 0;
    }
    int emitter=race_late.emitter_unit;
    for(int relay=0;relay<24&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;relay++){
        if(emitter_overlaps_location(1,emitter))return 1;
        int carrier=race_late.emitter_carrier;
        if(carrier>=MAX_UNITS||!u[carrier].alive||u[carrier].owner!=SIDE){
            carrier=nearest_pickup_actor(emitter);
            if(carrier<0){fprintf(stderr,"T7 no live human SCV can collect emitter=%d at=%d,%d tick=%u\n",
                emitter,u[emitter].x,u[emitter].y,(unsigned)host_tick);return 0;}
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
        if(!stage_emitter_carrier()){
            if(!u[carrier].alive||race_late.emitter_carrier!=carrier)continue;
            return 0;
        }
        escort_clear_local_threat(carrier);
        if(!u[carrier].alive||race_late.emitter_carrier!=carrier)continue;
        if(!order_unit(carrier,wx,wy)){
            if(!u[carrier].alive||race_late.emitter_carrier!=carrier)continue;
            return 0;
        }
        (void)order_visible_type(U_MARINE,wx,wy);
        (void)order_visible_type(U_VULTURE,wx,wy);
        (void)order_visible_type(U_FIREBAT,wx,wy);
        checkpoint("emitter-delivery-ordered");
        for(int t=0;t<1500&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
            step(0);
            if(!u[emitter].alive)return 0;
            if(emitter_overlaps_location(1,emitter)){
                fprintf(stderr,"T7 emitter entered source location1 tick=%u at=%d,%d\n",(unsigned)host_tick,u[emitter].x,u[emitter].y);
                checkpoint("emitter-at-goal");return 1;
            }
            if(!u[carrier].alive||race_late.emitter_carrier!=carrier)break;
            if(dist2(u[carrier].x,u[carrier].y,wx,wy)<100)break;
        }
    }
    fprintf(stderr,"T7 emitter relay budget exhausted at tick=%u slot=%d alive=%d pos=%d,%d carrier=%u\n",
      (unsigned)host_tick,emitter,u[emitter].alive,u[emitter].x,u[emitter].y,(unsigned)race_late.emitter_carrier);
    return emitter_overlaps_location(1,emitter);
}

static void progress_until_terminal(void)
{
    static int progress_attempt;
    int attempt=progress_attempt++;
    int emitter=-1;
    fprintf(stderr,"T7 objective search tick=%u emitter_slot=%u carrier=%u\n",
      (unsigned)host_tick,(unsigned)race_late.emitter_unit,(unsigned)race_late.emitter_carrier);
    for(int i=0;i<tick_limit&&host_tick<(u32)tick_limit&&screen==SCREEN_PLAY&&!won&&!lost;i++){
        step(0);
        emitter=raw_unit(217,SIDE);
        if(emitter>=0)break;
    }
    if(emitter>=0){
        fprintf(stderr,"T7 objective actor acquired slot=%d owner=%d rawowner=%d carrier=%u tick=%u\n",
          emitter,u[emitter].owner,u[emitter].raw_owner,(unsigned)race_late.emitter_carrier,(unsigned)host_tick);
        checkpoint("emitter-created");
        if(controller_collect_emitter(emitter)){
            if(!stage_emitter_carrier()){
                fprintf(stderr,"T7 cannot stage emitter SCV using normal Stop input; refusing exposed transfer\n");
                return;
            }
            controller_assign_gas_workers();
            (void)controller_assign_minerals();
            if(controller_air_deliver_emitter(emitter)) goto delivery_complete;
            /* Gather the Marines already supplied by the scenario and clear
               only a source-hostile Goliath that can reach the drop point.
               Repeated Wraith waves spent far beyond source weapon range. */
            int escorts=build_wraith_escorts(4);
            int route_clear=escorts>0&&clear_route_goliath_with_wraiths();
            if(!route_clear)route_clear=clear_route_goliath_with_ground_force();
            if(!route_clear){
                fprintf(stderr,"T7 hold emitter carrier: Goliath preclear not verified\n");
                return;
            }
            /* Try the paid Dropship route after preclear; fall back to the
               ground relay if that ordinary-input transfer fails. */
            if(attempt==0){
                if(!controller_air_deliver_emitter(emitter))
                    controller_deliver_emitter();
            }else if(!controller_air_deliver_emitter(emitter))
                controller_deliver_emitter();
        }
    } else fprintf(stderr,"T7 objective actor not human-owned tick=%u emitter_slot=%u owner=%d alive=%d\n",
      (unsigned)host_tick,(unsigned)race_late.emitter_unit,
      race_late.emitter_unit<MAX_UNITS?u[race_late.emitter_unit].owner:-1,
      race_late.emitter_unit<MAX_UNITS?u[race_late.emitter_unit].alive:0);
delivery_complete:
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
    /* The source emitter begins beside an SCV and the actor can be lost while
       an escort is being queued. Attempt the authored Bring objective before
       spending time on optional setup; if it remains incomplete, build the
       escort and retry with normal input. */
    progress_until_terminal();
    if(!won&&!lost&&screen==SCREEN_PLAY){
        prepare_marine_escort();
        preclear_route_threat();
        prepare_marine_escort();
        progress_until_terminal();
    }
    if(trace_file){checkpoint(won?"won":lost?"lost":"limit");fputs("\n]}\n",trace_file);fclose(trace_file);}
    int emitter=race_late.emitter_unit<MAX_UNITS?(int)race_late.emitter_unit:-1;
    int emitter_at_goal=emitter_overlaps_location(1,emitter);
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
