/*
 * Bounded, normal-input controller probe for the private T6 mission.
 *
 * The probe entry is compile-gated and only selects/initializes mission 9.
 * After that point, all world changes are made by game_update() with ordinary
 * button input.  Runtime arrays and generated locations are read only for
 * planning and verification.  This is not a hardware performance test.
 */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SC64_T6_PROBE_ENTRY
#error "compile with the private T6 probe-entry flag"
#endif

enum { T6_RUN_DEFAULT = 30000, T6_CHECKPOINT_PERIOD = 1000 };
static FILE *trace_file;
static const char *trace_path;
static int run_limit = T6_RUN_DEFAULT;
static int trace_first = 1;
static u16 prior_keys;
static int last_keys = -1;
static u32 host_ticks;
static int watch_norad;
static int hero_slot = -1, norad_slot = -1, starport_slot = -1, repair_worker_slot = -1;
static u8 dropship_goal_attempted[MAX_UNITS];
static int tower_slot = -1, ship_slots[2] = {-1,-1};

static void checkpoint(const char *kind);

static int clamp_world(int p, int limit)
{ return p < 0 ? 0 : p >= limit ? limit - 1 : p; }

static int snap4(int from, int to, int limit)
{
    int d = to - from;
    int n = d >= 0 ? (d + 2) / 4 : (d - 2) / 4;
    return clamp_world(from + n * 4, limit);
}

static void step(u16 held)
{
    if (host_ticks >= (u32)run_limit || screen != SCREEN_PLAY || won || lost)
        return;
    u16 pressed = held & (u16)~prior_keys;
    game_update(held, pressed);
    prior_keys = held;
    ++host_ticks;
    if (watch_norad && norad_slot >= 0 && (host_ticks % 250u) == 0) {
        int nearest = 0x7fffffff, nearest_slot = -1, targeting = 0;
        int repair_slot=-1,repair_distance=-1;
        int nx = b[norad_slot].x, ny = b[norad_slot].y;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && owners_hostile(u[i].owner, RESCUABLE)) {
                int dx = u[i].x - nx, dy = u[i].y - ny, d = dx * dx + dy * dy;
                if (d < nearest) { nearest = d; nearest_slot = i; }
                if (u[i].order == ORDER_ATTACK_TARGET &&
                    u[i].target == MAX_UNITS + norad_slot) ++targeting;
            }
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].order==ORDER_REPAIR&&
            u[i].target==MAX_UNITS+norad_slot){repair_slot=i;repair_distance=dist2(u[i].x,u[i].y,nx,ny);break;}
        if(repair_slot<0)for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].type==U_SCV&&u[i].owner==SIDE&&
            u[i].order==ORDER_REPAIR){repair_slot=i;repair_distance=dist2(u[i].x,u[i].y,nx,ny);break;}
        fprintf(stderr, "NORADWATCH tick=%u hp=%d alive=%d owner=%d ai1=%u ai4=%u nearest=%d raw=%d owner=%d d2=%d order=%d target=%d direct=%d repair=%d xy=%d,%d d2=%d target=%d timer=%d carry=%d tracked=%d alive=%d order=%d xy=%d,%d\n",
                (unsigned)host_ticks, b[norad_slot].hp, b[norad_slot].alive,b[norad_slot].owner,
                race_t6.ai_requests[1], race_t6.ai_requests[4], nearest_slot,
                nearest_slot >= 0 ? u[nearest_slot].raw_type : -1,
                nearest_slot >= 0 ? u[nearest_slot].owner : -1, nearest,
                nearest_slot >= 0 ? u[nearest_slot].order : -1,
                nearest_slot >= 0 ? u[nearest_slot].target : -1, targeting,
                repair_slot,repair_slot>=0?u[repair_slot].x:-1,
                repair_slot>=0?u[repair_slot].y:-1,repair_distance,
                repair_slot>=0?u[repair_slot].target:-1,
                repair_slot>=0?u[repair_slot].work_timer:-1,
                repair_slot>=0?u[repair_slot].carry:-1,
                repair_worker_slot,repair_worker_slot>=0?u[repair_worker_slot].alive:0,
                repair_worker_slot>=0?u[repair_worker_slot].order:-1,
                repair_worker_slot>=0?u[repair_worker_slot].x:-1,
                repair_worker_slot>=0?u[repair_worker_slot].y:-1);
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].type==U_SCV)
            fprintf(stderr,"  SCV slot=%d owner=%d xy=%d,%d order=%d target=%d carry=%d\n",
                    i,u[i].owner,u[i].x,u[i].y,u[i].order,u[i].target,u[i].carry);
    }
    if (trace_file && held != (u16)last_keys) {
        if (!trace_first) fputs(",\n", trace_file);
        trace_first = 0;
        fprintf(trace_file,
                "{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}",
                (unsigned)host_ticks, held, pressed);
        last_keys = held;
    }
    if (trace_file && host_ticks % T6_CHECKPOINT_PERIOD == 0)
        checkpoint("periodic");
}

static void wait_frames(int n)
{ for (int i = 0; i < n && host_ticks < (u32)run_limit && screen == SCREEN_PLAY && !won && !lost; ++i) step(0); }

static void tap(u16 key)
{ step(key); step(0); }

static int normal_field(void)
{
    for (int i = 0; i < 2 && (menu_open || mode != MODE_NORMAL); ++i) tap(KEY_B);
    return screen == SCREEN_PLAY && !menu_open && mode == MODE_NORMAL;
}

static void move_cursor(int x, int y)
{
    x = snap4(cx, x, WORLD_W); y = snap4(cy, y, WORLD_H);
    for (int n = 0; n < 3000 && (cx != x || cy != y) && host_ticks < (u32)run_limit; ++n) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT; if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN; if (cy > y) held |= KEY_UP;
        step(held);
    }
    step(0);
}

static int source_unit(int raw, int live_owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == raw &&
            (live_owner < 0 || u[i].owner == live_owner)) return i;
    return -1;
}

static int source_building(int raw, int live_owner)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw &&
            (live_owner < 0 || b[i].owner == live_owner)) return i;
    return -1;
}

static int owner_units(int owner)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == owner) ++n;
    return n;
}

static int owner_buildings(int owner)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == owner) ++n;
    return n;
}

static int location_center(unsigned id, int *x, int *y)
{
    if (!id || id > sc64_t6_scenario.location_count || !x || !y) return 0;
    const Sc64T4Location *p = &sc64_t6_scenario.locations[id - 1];
    *x = (int)((p->left + p->right) / 4);
    *y = (int)((p->top + p->bottom) / 4);
    return 1;
}

/* Bounded four-neighbor planner through the current terrain path grid.
 * Destinations are generated source locations or observed actors/buildings. */
static int reachable_point(int sx, int sy, unsigned loc, int *ox, int *oy)
{
    static u8 seen[MAP_CELLS];
    static u16 q[MAP_CELLS], d[MAP_CELLS];
    if (!loc || loc > sc64_t6_scenario.location_count || !ox || !oy) return -1;
    int start = sx / 16 + (sy / 16) * MAP_COLS;
    if (start < 0 || start >= MAP_CELLS) return -1;
    const Sc64T4Location *r = &sc64_t6_scenario.locations[loc - 1];
    build_path_grid(); memset(seen, 0, sizeof(seen)); memset(d, 0, sizeof(d));
    int head = 0, tail = 0; q[tail++] = (u16)start; seen[start] = 1;
    while (head < tail) {
        int c = q[head++], col = c % MAP_COLS, row = c / MAP_COLS;
        int px = col * 16 + 8, py = row * 16 + 8;
        if ((u32)(px * 2) >= r->left && (u32)(px * 2) < r->right &&
            (u32)(py * 2) >= r->top && (u32)(py * 2) < r->bottom) { *ox = px; *oy = py; return d[c]; }
#define NEXT_CELL(n_) do { int z_=(n_); if (!seen[z_] && SC64_GRID_GET(path_walkable,z_)) { seen[z_]=1; d[z_]=(u16)(d[c]+1); q[tail++]=(u16)z_; } } while (0)
        if (col) NEXT_CELL(c - 1);
        if (col + 1 < MAP_COLS) NEXT_CELL(c + 1);
        if (row) NEXT_CELL(c - MAP_COLS);
        if (row + 1 < MAP_ROWS) NEXT_CELL(c + MAP_COLS);
#undef NEXT_CELL
    }
    return -1;
}

static int reachable_building_contact(int actor, int sx, int sy, int slot, int *ox, int *oy)
{
    static u8 seen[MAP_CELLS]; static u16 q[MAP_CELLS], d[MAP_CELLS];
    if(actor<0||actor>=MAX_UNITS||!u[actor].alive||slot<0||slot>=MAX_BUILDINGS||!b[slot].alive||!ox||!oy)return -1;
    int start=sx/16+(sy/16)*MAP_COLS;if(start<0||start>=MAP_CELLS)return -1;
    build_path_grid();memset(seen,0,sizeof(seen));memset(d,0,sizeof(d));
    int head=0,tail=0;q[tail++]=(u16)start;seen[start]=1;
    while(head<tail){int c=q[head++],col=c%MAP_COLS,row=c/MAP_COLS;
        int px=col*16+8,py=row*16+8;
        if(dist2(px,py,b[slot].x,b[slot].y)<=32*32&&unit_step_clear(&u[actor],px,py)){
            *ox=px;*oy=py;return d[c];}
#define CONTACT_NEXT(n_) do {int z_=(n_);if(!seen[z_]&&SC64_GRID_GET(path_walkable,z_)){seen[z_]=1;d[z_]=(u16)(d[c]+1);q[tail++]=(u16)z_;}}while(0)
        if(col)CONTACT_NEXT(c-1);
        if(col+1<MAP_COLS)CONTACT_NEXT(c+1);
        if(row)CONTACT_NEXT(c-MAP_COLS);
        if(row+1<MAP_ROWS)CONTACT_NEXT(c+MAP_COLS);
#undef CONTACT_NEXT
    }return -1;
}

static int select_unit(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE || !normal_field()) return 0;
    if (selected_building >= 0) tap(KEY_L);
    for (int n = 0; n <= MAX_UNITS && host_ticks < (u32)run_limit; ++n) {
        if (u[slot].selected && selected_count() == 1 && selected_building < 0) return 1;
        tap(KEY_L);
    }
    return 0;
}

static int click_select_unit(int slot)
{
    if(slot<0||slot>=MAX_UNITS||!u[slot].alive||u[slot].owner!=SIDE||!normal_field())return 0;
    static const int dx[8]={-16,16,0,0,-24,24,0,0};
    static const int dy[8]={0,0,-16,16,0,0,-24,24};
    int selected_point=0;
    for(int attempt=0;attempt<9&&u[slot].alive;attempt++){
        int x=u[slot].x,y=u[slot].y;
        if(attempt){x=clamp_world(x+dx[attempt-1],WORLD_W);y=clamp_world(y+dy[attempt-1],WORLD_H);}
        move_cursor(x,y);
        if(u[slot].alive&&selection_target()==slot){selected_point=1;break;}
    }
    if(!selected_point)return 0;
    tap(KEY_A);
    return u[slot].alive&&u[slot].selected&&selected_count()==1&&selected_building<0;
}

static int select_building(int slot)
{
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive || b[slot].owner != SIDE || !normal_field()) return 0;
    if (selected_building >= 0) tap(KEY_L);
    move_cursor(b[slot].x, b[slot].y); tap(KEY_A);
    return selected_building == slot && selected_count() == 0;
}

static int order_actor(int slot, int x, int y, int attack_move)
{
    if (!select_unit(slot)) return 0;
    tap(KEY_B); if (attack_move) tap(KEY_DOWN); tap(KEY_A);
    move_cursor(x, y); tap(KEY_A); return 1;
}

static int group_to(int x, int y, int attack_move)
{
    if (!normal_field()) return 0;
    int l = WORLD_W, t = WORLD_H, r = -1, bot = -1;
    for (int i=0;i<MAX_UNITS;i++) if (u[i].alive && u[i].owner==SIDE && sc64_game_unit_selectable(u[i].type)) {
        if (u[i].x<l)l=u[i].x; if(u[i].x>r)r=u[i].x;
        if (u[i].y<t)t=u[i].y; if(u[i].y>bot)bot=u[i].y;
    }
    if (r<l || bot<t) return 0;
    move_cursor(clamp_world(l-8,WORLD_W),clamp_world(t-8,WORLD_H)); step(KEY_L);
    int ex=snap4(cx,clamp_world(r+8,WORLD_W),WORLD_W), ey=snap4(cy,clamp_world(bot+8,WORLD_H),WORLD_H);
    for(int n=0;n<1200&&(cx!=ex||cy!=ey);n++) { u16 h=KEY_L|KEY_R; if(cx<ex)h|=KEY_RIGHT;if(cx>ex)h|=KEY_LEFT;if(cy<ey)h|=KEY_DOWN;if(cy>ey)h|=KEY_UP;step(h); }
    step(0); if(selected_building>=0||!selected_count())return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].owner!=SIDE)return 0;
    tap(KEY_B); if(attack_move)tap(KEY_DOWN);tap(KEY_A);move_cursor(x,y);tap(KEY_A);return 1;
}

static int group_visible_type_to(int type, int x, int y, int attack_move)
{
    if(!normal_field())return 0;
    int seed=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type&&!SC64_TRANSPORT_LOADED(&u[i])){
        move_cursor(u[i].x,u[i].y);
        if(selection_target()==i){seed=i;break;}
    }
    if(seed<0)return 0;
    tap(KEY_A);tap(KEY_A); /* normal double-tap selects visible same-type actors */
    int count=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected){
        if(u[i].owner!=SIDE||u[i].type!=type)return 0;
        ++count;
    }
    if(!count||count!=selected_count()||selected_building>=0)return 0;
    tap(KEY_B);if(attack_move)tap(KEY_DOWN);tap(KEY_A);move_cursor(x,y);tap(KEY_A);return 1;
}

static int point_in_location(unsigned id,int x,int y)
{
    if(!id||id>sc64_t6_scenario.location_count)return 0;
    const Sc64T4Location *r=&sc64_t6_scenario.locations[id-1];
    return (u32)(x*2)>=r->left&&(u32)(x*2)<r->right&&
           (u32)(y*2)>=r->top&&(u32)(y*2)<r->bottom;
}

static int controller_air_cell_safe(int x,int y)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&!SC64_TRANSPORT_LOADED(&u[i])&&
        owners_hostile(SIDE,u[i].owner)){
        int range=sc64_race_unit_range_vs(u[i].type,1);
        if(range>0&&sc64_race_unit_damage_vs(u[i].type,1)>0){
            int radius=(range+1)/2+24;
            if(dist2(x,y,u[i].x,u[i].y)<radius*radius)return 0;
        }
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner)&&
       (b[i].type==B_MISSILE_TURRET||b[i].type==B_SPORE_COLONY||b[i].type==B_PHOTON_CANNON))
        if(dist2(x,y,b[i].x,b[i].y)<136*136)return 0;
    return 1;
}

static int controller_air_goal(unsigned loc,int sx,int sy,int *gx,int *gy)
{
    int best=0x7fffffff,found=0;
    if(!gx||!gy||!loc||loc>sc64_t6_scenario.location_count)return 0;
    for(int y=8;y<WORLD_H;y+=16)for(int x=8;x<WORLD_W;x+=16)
        if(point_in_location(loc,x,y)&&controller_air_cell_safe(x,y)){
            int d=dist2(x,y,sx,sy);
            if(d<best){best=d;*gx=x;*gy=y;found=1;}
        }
    return found;
}

static u32 controller_air_cell_cost(int x,int y)
{
    u32 cost=10;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&!SC64_TRANSPORT_LOADED(&u[i])&&
        owners_hostile(SIDE,u[i].owner)){
        int range=sc64_race_unit_range_vs(u[i].type,1);
        if(range>0&&sc64_race_unit_damage_vs(u[i].type,1)>0){
            int radius=(range+1)/2+24;
            int d2=dist2(x,y,u[i].x,u[i].y);
            if(d2<radius*radius){
                int d=0;while((d+1)*(d+1)<=d2)++d;
                cost+=(u32)(24+(radius-d)/2);
            }
        }
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner)&&
       (b[i].type==B_MISSILE_TURRET||b[i].type==B_SPORE_COLONY||b[i].type==B_PHOTON_CANNON)){
        int radius=136,d2=dist2(x,y,b[i].x,b[i].y);
        if(d2<radius*radius){int d=0;while((d+1)*(d+1)<=d2)++d;cost+=(u32)(28+(radius-d)/3);}
    }
    return cost;
}

typedef struct { u32 cost; u16 cell; } T6AirHeapNode;
static T6AirHeapNode controller_air_heap[MAP_CELLS*8];
static u16 controller_air_previous[MAP_CELLS];
static u32 controller_air_distance[MAP_CELLS];
static u8 controller_air_closed[MAP_CELLS];

static void controller_air_heap_push(unsigned *count,T6AirHeapNode node)
{
    if(*count>=MAP_CELLS*8)return;
    unsigned i=(*count)++;
    while(i){unsigned p=(i-1)/2;if(controller_air_heap[p].cost<=node.cost)break;
        controller_air_heap[i]=controller_air_heap[p];i=p;}
    controller_air_heap[i]=node;
}

static T6AirHeapNode controller_air_heap_pop(unsigned *count)
{
    T6AirHeapNode result=controller_air_heap[0],last=controller_air_heap[--*count];
    unsigned i=0;
    while(i*2+1<*count){unsigned child=i*2+1;
        if(child+1<*count&&controller_air_heap[child+1].cost<controller_air_heap[child].cost)++child;
        if(controller_air_heap[child].cost>=last.cost)break;
        controller_air_heap[i]=controller_air_heap[child];i=child;}
    if(*count)controller_air_heap[i]=last;
    return result;
}

/* Air units ignore terrain. Weighted Dijkstra minimizes flight distance plus
 * estimated exposure; threatened cells remain usable when every route incurs
 * damage, and the caller checks live carrier health after every leg. */
static int controller_air_next_waypoint(int sx,int sy,int gx,int gy,int *wx,int *wy)
{
    static u16 reverse[MAP_CELLS];
    int start=sx/16+(sy/16)*MAP_COLS,goal=gx/16+(gy/16)*MAP_COLS;
    if(!wx||!wy||start<0||start>=MAP_CELLS||goal<0||goal>=MAP_CELLS)return 0;
    for(int i=0;i<MAP_CELLS;i++){controller_air_distance[i]=0xffffffffu;controller_air_previous[i]=0xffffu;controller_air_closed[i]=0;}
    unsigned heap_count=0;controller_air_distance[start]=0;controller_air_heap_push(&heap_count,(T6AirHeapNode){0,(u16)start});
    while(heap_count){
        T6AirHeapNode node=controller_air_heap_pop(&heap_count);int c=node.cell;
        if(controller_air_closed[c]||node.cost!=controller_air_distance[c])continue;
        controller_air_closed[c]=1;if(c==goal)break;
        int col=c%MAP_COLS,row=c/MAP_COLS;
        for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)if(dx||dy){
            int nx=col+dx,ny=row+dy;if(nx<0||ny<0||nx>=MAP_COLS||ny>=MAP_ROWS)continue;
            int z=nx+ny*MAP_COLS;if(controller_air_closed[z])continue;
            int zx=nx*16+8,zy=ny*16+8;
            u32 step_cost=controller_air_cell_cost(zx,zy)+(dx&&dy?4u:0u);
            if(node.cost<=0xffffffffu-step_cost&&node.cost+step_cost<controller_air_distance[z]){
                controller_air_distance[z]=node.cost+step_cost;controller_air_previous[z]=(u16)c;
                controller_air_heap_push(&heap_count,(T6AirHeapNode){node.cost+step_cost,(u16)z});
            }
        }
    }
    if(!controller_air_closed[goal])return 0;
    int len=0,at=goal;
    while(at!=start&&len<MAP_CELLS){reverse[len++]=(u16)at;at=controller_air_previous[at];if(at>=MAP_CELLS)return 0;}
    if(!len){*wx=gx;*wy=gy;return 1;}
    int prior=start,last_dx=0,last_dy=0;
    for(int k=len-1;k>=0;k--){int n=reverse[k];
        int dx=n%MAP_COLS-prior%MAP_COLS,dy=n/MAP_COLS-prior/MAP_COLS;
        if(k!=len-1&&(dx!=last_dx||dy!=last_dy)){
            *wx=(prior%MAP_COLS)*16+8;*wy=(prior/MAP_COLS)*16+8;return 1;
        }
        last_dx=dx;last_dy=dy;prior=n;
    }
    *wx=gx;*wy=gy;return 1;
}

static int controller_move_raynor_to_goal(void)
{
    int gx,gy;
    if(hero_slot<0||!u[hero_slot].alive||point_in_location(2,u[hero_slot].x,u[hero_slot].y))return 1;
    int distance=reachable_point(u[hero_slot].x,u[hero_slot].y,2,&gx,&gy);
    if(distance<0||!order_actor(hero_slot,gx,gy,0))return 0;
    fprintf(stderr,"T6 controller ordered Raynor to source location2 target=%d,%d route=%d\n",gx,gy,distance);
    for(int i=0;i<12000&&host_ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost;i++){
        step(0);
        if(!u[hero_slot].alive)return 0;
        if(point_in_location(2,u[hero_slot].x,u[hero_slot].y)){
            fprintf(stderr,"T6 Raynor reached source location2 tick=%u pos=%d,%d\n",(unsigned)host_ticks,u[hero_slot].x,u[hero_slot].y);
            checkpoint("raynor-at-goal");return 1;
        }
    }
    return point_in_location(2,u[hero_slot].x,u[hero_slot].y);
}

static int controller_move_dropship_to_goal(int ship)
{
    int gx,gy;
    if(ship<0||ship>=MAX_UNITS||!u[ship].alive||u[ship].owner!=SIDE||u[ship].type!=U_DROPSHIP)return 0;
    if(point_in_location(2,u[ship].x,u[ship].y))return 1;
    if(!controller_air_goal(2,u[ship].x,u[ship].y,&gx,&gy)){
        fprintf(stderr,"T6 no air-safe cell in source location2 for Dropship=%d\n",ship);return 0;
    }
    fprintf(stderr,"T6 Dropship route to location2 slot=%d from=%d,%d target=%d,%d\n",ship,u[ship].x,u[ship].y,gx,gy);
    for(int leg=0;leg<64&&host_ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost;leg++){
        int wx,wy;
        if(!controller_air_next_waypoint(u[ship].x,u[ship].y,gx,gy,&wx,&wy)){
            fprintf(stderr,"T6 Dropship no safe waypoint slot=%d leg=%d at=%d,%d goal=%d,%d\n",ship,leg,u[ship].x,u[ship].y,gx,gy);
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&!SC64_TRANSPORT_LOADED(&u[i])&&owners_hostile(SIDE,u[i].owner)){
                int range=sc64_race_unit_range_vs(u[i].type,1),damage=sc64_race_unit_damage_vs(u[i].type,1);
                if(range>0&&damage>0&&dist2(u[ship].x,u[ship].y,u[i].x,u[i].y)<320*320)
                    fprintf(stderr,"  air-threat unit=%d type=%d owner=%d at=%d,%d range=%d damage=%d d2=%d\n",i,u[i].type,u[i].owner,u[i].x,u[i].y,range,damage,dist2(u[ship].x,u[ship].y,u[i].x,u[i].y));
            }
            for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner)&&
              (b[i].type==B_MISSILE_TURRET||b[i].type==B_SPORE_COLONY||b[i].type==B_PHOTON_CANNON)&&
              dist2(u[ship].x,u[ship].y,b[i].x,b[i].y)<320*320)
                fprintf(stderr,"  air-threat building=%d type=%d owner=%d at=%d,%d d2=%d\n",i,b[i].type,b[i].owner,b[i].x,b[i].y,dist2(u[ship].x,u[ship].y,b[i].x,b[i].y));
            return 0;}
        fprintf(stderr,"T6 Dropship waypoint slot=%d leg=%d target=%d,%d\n",ship,leg,wx,wy);
        if(!order_actor(ship,wx,wy,0)){fprintf(stderr,"T6 Dropship waypoint command failed slot=%d leg=%d\n",ship,leg);return 0;}
        int arrived=0;
        for(int t=0;t<5000&&host_ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
            step(0);if(!u[ship].alive)return 0;
            if(dist2(u[ship].x,u[ship].y,wx,wy)<=8*8){arrived=1;break;}
        }
        if(!arrived){fprintf(stderr,"T6 Dropship waypoint timeout slot=%d leg=%d at=%d,%d target=%d,%d\n",ship,leg,u[ship].x,u[ship].y,wx,wy);return 0;}
        if(point_in_location(2,u[ship].x,u[ship].y)){
            fprintf(stderr,"T6 Dropship reached source location2 slot=%d tick=%u pos=%d,%d\n",ship,(unsigned)host_ticks,u[ship].x,u[ship].y);
            checkpoint("dropship-at-goal");return 1;
        }
    }
    fprintf(stderr,"T6 Dropship location2 move incomplete slot=%d alive=%d pos=%d,%d\n",ship,u[ship].alive,u[ship].x,u[ship].y);
    return point_in_location(2,u[ship].x,u[ship].y);
}

static int controller_fly_to_point(int ship,int gx,int gy)
{
    if(ship<0||ship>=MAX_UNITS||!u[ship].alive||u[ship].type!=U_DROPSHIP||
       gx<0||gy<0||gx>=WORLD_W||gy>=WORLD_H)return 0;
    for(int leg=0;leg<64&&host_ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost;leg++){
        if(dist2(u[ship].x,u[ship].y,gx,gy)<=8*8)return 1;
        int wx,wy;
        if(!controller_air_next_waypoint(u[ship].x,u[ship].y,gx,gy,&wx,&wy)||
           !order_actor(ship,wx,wy,0))return 0;
        int arrived=0;
        for(int t=0;t<5000&&host_ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
            step(0);if(!u[ship].alive)return 0;
            if(dist2(u[ship].x,u[ship].y,wx,wy)<=8*8){arrived=1;break;}
        }
        if(!arrived)return 0;
    }
    return u[ship].alive&&dist2(u[ship].x,u[ship].y,gx,gy)<=8*8;
}

static int controller_board_raynor(int ship)
{
    fprintf(stderr,"T6 boarding attempt ship=%d alive=%d owner=%d xy=%d,%d Raynor=%d alive=%d owner=%d xy=%d,%d loaded=%d dist2=%d\n",
            ship,ship>=0&&ship<MAX_UNITS?u[ship].alive:0,
            ship>=0&&ship<MAX_UNITS?u[ship].owner:-1,
            ship>=0&&ship<MAX_UNITS?u[ship].x:-1,ship>=0&&ship<MAX_UNITS?u[ship].y:-1,
            hero_slot,hero_slot>=0&&hero_slot<MAX_UNITS?u[hero_slot].alive:0,
            hero_slot>=0&&hero_slot<MAX_UNITS?u[hero_slot].owner:-1,
            hero_slot>=0&&hero_slot<MAX_UNITS?u[hero_slot].x:-1,
            hero_slot>=0&&hero_slot<MAX_UNITS?u[hero_slot].y:-1,
            hero_slot>=0&&hero_slot<MAX_UNITS?SC64_TRANSPORT_LOADED(&u[hero_slot]):0,
            ship>=0&&ship<MAX_UNITS&&hero_slot>=0&&hero_slot<MAX_UNITS?
              dist2(u[ship].x,u[ship].y,u[hero_slot].x,u[hero_slot].y):-1);
    if(ship<0||ship>=MAX_UNITS||hero_slot<0||hero_slot>=MAX_UNITS||
       !u[ship].alive||!u[hero_slot].alive||SC64_TRANSPORT_LOADED(&u[hero_slot]))return 0;
    int gx=-1,gy=-1,best=0x7fffffff;
    for(int y=8;y<WORLD_H;y+=16)for(int x=8;x<WORLD_W;x+=16)
        if(controller_air_cell_safe(x,y)&&dist2(x,y,u[hero_slot].x,u[hero_slot].y)<18*18){
            int d=dist2(x,y,u[ship].x,u[ship].y);if(d<best){best=d;gx=x;gy=y;}
        }
    if(gx<0){fprintf(stderr,"T6 no air-safe boarding cell near Raynor\n");return 0;}
    if(!controller_fly_to_point(ship,gx,gy)){fprintf(stderr,"T6 Dropship could not reach Raynor boarding cell=%d,%d\n",gx,gy);return 0;}
    if(dist2(u[ship].x,u[ship].y,u[hero_slot].x,u[hero_slot].y)>=18*18){
        if(!order_actor(ship,u[hero_slot].x,u[hero_slot].y,0))return 0;
        for(int t=0;t<200&&host_ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost;t++){
            step(0);if(!u[ship].alive||!u[hero_slot].alive)return 0;
            if(dist2(u[ship].x,u[ship].y,u[hero_slot].x,u[hero_slot].y)<18*18)break;
        }
        if(dist2(u[ship].x,u[ship].y,u[hero_slot].x,u[hero_slot].y)>=18*18){
            fprintf(stderr,"T6 Dropship still too far from Raynor d2=%d\n",dist2(u[ship].x,u[ship].y,u[hero_slot].x,u[hero_slot].y));return 0;
        }
    }
    if(!select_unit(hero_slot)){fprintf(stderr,"T6 could not select Raynor for boarding\n");return 0;}
    move_cursor(u[ship].x,u[ship].y);tap(KEY_A);
    for(int t=0;t<2000&&host_ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost&&
        u[hero_slot].alive&&!SC64_TRANSPORT_LOADED(&u[hero_slot]);t++)step(0);
    int loaded=SC64_TRANSPORT_LOADED(&u[hero_slot])&&u[hero_slot].target==ship;
    fprintf(stderr,"T6 controller boarded Raynor carrier=%d loaded=%d cargo=%d\n",ship,loaded,
            u[ship].alive?race_transport_cargo_space(ship):-1);
    if(loaded)checkpoint("raynor-boarded");
    return loaded;
}

static int controller_unload_raynor_at_goal(int ship,int gx,int gy)
{
    if(!u[ship].alive||!SC64_TRANSPORT_LOADED(&u[hero_slot])||
       u[hero_slot].target!=ship||!point_in_location(2,gx,gy))return 0;
    if(!select_unit(ship)||!normal_field())return 0;
    tap(KEY_B);for(int i=0;i<13;i++)tap(KEY_DOWN);tap(KEY_A);
    move_cursor(gx,gy);tap(KEY_A);
    for(int t=0;t<2000&&host_ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost&&
        u[ship].alive&&SC64_TRANSPORT_LOADED(&u[hero_slot]);t++)step(0);
    int landed=u[hero_slot].alive&&!SC64_TRANSPORT_LOADED(&u[hero_slot])&&
               point_in_location(2,u[hero_slot].x,u[hero_slot].y);
    fprintf(stderr,"T6 controller unloaded Raynor=%d at=%d,%d goal=%d cargo=%d\n",landed,
            u[hero_slot].x,u[hero_slot].y,gx, u[ship].alive?race_transport_cargo_space(ship):-1);
    if(landed)checkpoint("raynor-at-goal");
    return landed;
}

static int choose_build(int menu_row)
{
    if (!normal_field()) return 0; tap(KEY_B);
    for (int i=0;i<menu_row;i++) tap(KEY_DOWN);
    tap(KEY_A); return mode == MODE_BUILD;
}

static int find_site(int type, int nearx, int neary, int radius, int *ox, int *oy)
{
    int best=0x7fffffff, bx=-1,by=-1;
    for(int y=16;y<WORLD_H-48;y+=16)for(int x=16;x<WORLD_W-48;x+=16){
        int dd=dist2(x,y,nearx,neary);
        if(dd>radius*radius||dd>=best||!race_can_site(SIDE,type,x,y)||!can_place(type,x,y))continue;
        bx=x;by=y;best=dd;
    }
    if(bx<0)return 0;*ox=bx;*oy=by;return 1;
}

static int build_paid(int type, int menu_row, int nearx, int neary, int radius, int *out)
{
    int worker=-1,x,y,before=owner_buildings(SIDE);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&!SC64_TRANSPORT_LOADED(&u[i])){worker=i;break;}
    if(worker<0||!select_unit(worker)||!choose_build(menu_row)||!find_site(type,nearx,neary,radius,&x,&y))return 0;
    move_cursor(x,y);x=cx;y=cy;if(!race_can_site(SIDE,type,x,y)||!can_place(type,x,y))return 0;
    tap(KEY_A);int slot=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==type&&b[i].under_construction&&dist2(b[i].x,b[i].y,x,y)<64*64){slot=i;break;}
    if(slot<0||owner_buildings(SIDE)<=before)return 0;
    if(out)*out=slot;return 1;
}

static int wait_building(int slot, int max_ticks)
{
    for(int i=0;i<max_ticks&&host_ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost&&b[slot].alive&&!building_complete(slot);i++)step(0);
    return slot>=0&&slot<MAX_BUILDINGS&&b[slot].alive&&building_complete(slot);
}

static int nearest_owned_cc_geyser(void)
{
    int cc=source_building(106,SIDE),best=-1,bd=0x7fffffff;
    if(cc<0)return -1;
    for(int i=0;i<geyser_count;i++){int d=dist2(geysers[i].x,geysers[i].y,b[cc].x,b[cc].y);if(d<bd){bd=d;best=i;}}
    return best;
}

static int controller_build_refinery(int *out)
{
    if(out)*out=-1;
    int gi=nearest_owned_cc_geyser(),worker=-1,before=owner_buildings(SIDE);
    if(gi<0||minerals<building_cost(B_REFINERY))return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&!SC64_TRANSPORT_LOADED(&u[i])){worker=i;break;}
    if(worker<0||!select_unit(worker)||!choose_build(4))return 0;
    int x=geysers[gi].x,y=geysers[gi].y;
    move_cursor(x,y);x=cx;y=cy;
    if(!race_can_site(SIDE,B_REFINERY,x,y)||!can_place(B_REFINERY,x,y)){fprintf(stderr,"T6 refinery site invalid geyser=%d at=%d,%d mode=%d\n",gi,x,y,mode);normal_field();return 0;}
    tap(KEY_A);int slot=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_REFINERY&&b[i].under_construction&&dist2(x,y,b[i].x,b[i].y)<64*64){slot=i;break;}
    if(slot<0||owner_buildings(SIDE)<=before){fprintf(stderr,"T6 refinery command did not start at %d,%d mineral=%d\n",x,y,minerals);normal_field();return 0;}
    fprintf(stderr,"T6 controller started paid Refinery slot=%d site=%d,%d worker=%d wallet=%d/%d\n",slot,x,y,worker,minerals,gas);
    if(out)*out=slot;return 1;
}

static int controller_assign_gas(int refinery,int max_workers)
{
    int assigned=0;
    for(int i=0;i<MAX_UNITS&&assigned<max_workers&&host_ticks<(u32)run_limit;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&!SC64_TRANSPORT_LOADED(&u[i])&&
           !(u[i].order==ORDER_GAS&&u[i].target==refinery)){
            if(!select_unit(i))continue;
            move_cursor(b[refinery].x,b[refinery].y);tap(KEY_A);
            if(u[i].alive&&u[i].order==ORDER_GAS&&u[i].target==refinery)++assigned;
        }
    return assigned;
}

static int controller_live_scv_count(void)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&
           !SC64_TRANSPORT_LOADED(&u[i])) ++n;
    return n;
}

static int controller_queued_scv_count(void)
{
    int n=0;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE)
        for(int j=0;j<b[i].queue_count&&j<5;j++)
            if(b[i].queue[(b[i].queue_head+j)%5]==U_SCV) ++n;
    return n;
}

static int controller_queue_scv(void)
{
    int cc=source_building(106,SIDE);
    if(cc<0||!building_complete(cc)||b[cc].queue_count>=5||
       minerals<unit_cost(U_SCV)||
       supply_used+unit_supply(U_SCV)>supply_cap||!select_building(cc)) return 0;
    int before=b[cc].queue_count;
    tap(KEY_A);
    if(b[cc].queue_count<=before) return 0;
    fprintf(stderr,"T6 controller queued paid SCV slot=%d live=%d queued=%d wallet=%d/%d\n",
            cc,controller_live_scv_count(),controller_queued_scv_count(),minerals,gas);
    checkpoint("scv-queued");
    return 1;
}

static int controller_build_tower(int starport)
{
    if(starport<0||starport>=MAX_BUILDINGS||!b[starport].alive||
       !building_complete(starport)||race_t6_starport_has_control_tower(starport))return 0;
    if(!select_building(starport))return 0;
    tap(KEY_B);
    for(int i=0;i<12;i++)tap(KEY_DOWN);
    tap(KEY_A);
    for(int i=0;i<1000&&host_ticks<(u32)run_limit;i++){
        if(race_t6_starport_has_control_tower(starport)){
            fprintf(stderr,"T6 controller completed attached Control Tower starport=%d wallet=%d/%d\n",
                    starport,minerals,gas);
            checkpoint("control-tower-active");
            return 1;
        }
        step(0);
    }
    fprintf(stderr,"T6 Control Tower did not complete starport=%d alive=%d raw=%d owner=%d wallet=%d/%d attached=%d mode=%d menu=%d\n",
            starport,b[starport].alive,b[starport].raw_type,b[starport].owner,
            minerals,gas,race_t6_has_attached_control_tower(starport),mode,menu_open);
    return race_t6_starport_has_control_tower(starport);
}

static int controller_build_starport(int factory, int *out)
{
    int worker=-1, x=-1, y=-1, best=0x7fffffff;
    if(out)*out=-1;
    if(factory<0||factory>=MAX_BUILDINGS||!b[factory].alive||
       b[factory].owner!=SIDE||!building_complete(factory))return 0;
    for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&
           !SC64_TRANSPORT_LOADED(&u[i])&&u[i].order!=ORDER_GAS){worker=i;break;}
    if(worker<0)for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&
           !SC64_TRANSPORT_LOADED(&u[i])){worker=i;break;}
    if(worker<0||minerals<building_cost(B_STARPORT)||gas<building_gas_cost(B_STARPORT))return 0;
    for(int cy=16;cy<WORLD_H-48;cy+=16)for(int cx0=32;cx0<WORLD_W-64;cx0+=16){
        int tx=cx0+SC64_T6_ADDON_X_OFFSET,ty=cy+SC64_T6_ADDON_Y_OFFSET;
        int d=dist2(cx0,cy,b[factory].x,b[factory].y)+dist2(cx0,cy,u[worker].x,u[worker].y);
        if(d>=best||d>2*288*288||
           !race_can_site(SIDE,B_STARPORT,cx0,cy)||!can_place(B_STARPORT,cx0,cy)||
           !race_can_site(SIDE,B_CONTROL_TOWER,tx,ty)||!can_place(B_CONTROL_TOWER,tx,ty))continue;
        int clear=1;
        for(int bi=0;bi<MAX_BUILDINGS;bi++)
            if(b[bi].alive&&dist2(tx,ty,b[bi].x,b[bi].y)<24*24){clear=0;break;}
        if(clear){best=d;x=cx0;y=cy;}
    }
    if(x<0||!select_unit(worker)||!choose_build(11))return 0;
    int planned_x=x,planned_y=y;
    move_cursor(x,y);x=cx;y=cy;
    int tx=x+SC64_T6_ADDON_X_OFFSET,ty=y+SC64_T6_ADDON_Y_OFFSET;
    int clear=1;
    for(int bi=0;bi<MAX_BUILDINGS;bi++)
        if(b[bi].alive&&dist2(tx,ty,b[bi].x,b[bi].y)<24*24){clear=0;break;}
    if(!clear||!race_can_site(SIDE,B_STARPORT,x,y)||!can_place(B_STARPORT,x,y)||
       !race_can_site(SIDE,B_CONTROL_TOWER,tx,ty)||!can_place(B_CONTROL_TOWER,tx,ty)){
        fprintf(stderr,"T6 Starport site invalid planned=%d,%d actual=%d,%d worker=%d\n",
                planned_x,planned_y,x,y,worker);
        normal_field();return 0;
    }
    int before=owner_buildings(SIDE);
    tap(KEY_A);
    int slot=-1;
    if(owner_buildings(SIDE)>before)for(int i=0;i<MAX_BUILDINGS;i++)
        if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_STARPORT&&
           b[i].under_construction&&dist2(x,y,b[i].x,b[i].y)<64*64){slot=i;break;}
    if(slot<0){fprintf(stderr,"T6 Starport placement produced no building at=%d,%d mode=%d\n",x,y,mode);normal_field();return 0;}
    fprintf(stderr,"T6 controller started addon-clear paid Starport slot=%d at=%d,%d addon=%d,%d worker=%d\n",slot,x,y,tx,ty,worker);
    if(out)*out=slot;
    return 1;
}

static int controller_queue_dropship(int starport)
{
    int queued=0;
    if(!race_t6_starport_has_control_tower(starport)||!select_building(starport))return 0;
    for(int i=0;i<5&&train_type!=U_DROPSHIP;i++)tap(KEY_R);
    if(train_type!=U_DROPSHIP||minerals<unit_cost(U_DROPSHIP)||gas<unit_gas_cost(U_DROPSHIP))return 0;
    int before=b[starport].queue_count;
    tap(KEY_A);
    if(b[starport].queue_count<=before)return 0;
    for(int i=0;i<b[starport].queue_count&&i<5;i++)
        if(b[starport].queue[(b[starport].queue_head+i)%5]==U_DROPSHIP)++queued;
    fprintf(stderr,"T6 controller queued paid Dropship slot=%d count=%d wallet=%d/%d\n",
            starport,queued,minerals,gas);
    checkpoint("dropship-queued");
    return 1;
}

/* Select a nearby rescued worker group and issue ordinary context-A repair. */
static int controller_start_norad_repair(void)
{
    int target = source_building(126, SIDE);
    if (target < 0 || !b[target].alive) {
        fprintf(stderr,"T6 repair start skipped: Norad not owned (slot=%d owner=%d alive=%d)\n",
                target,target>=0?b[target].owner:-1,target>=0&&b[target].alive);
        return 0;
    }
    if (!normal_field()) return 0;
    int seed=-1, best=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].type==U_SCV&&!SC64_TRANSPORT_LOADED(&u[i])){
        int d=dist2(u[i].x,u[i].y,b[target].x,b[target].y);
        if(d<best){best=d;seed=i;}
    }
    if(seed<0)return 0;
    repair_worker_slot=seed;
    /* The cursor is deliberately kept over Norad during base contact. Cycle
     * the selected human actor to the nearby SCV instead of spending frames
     * moving the cursor across the map while the exposed worker is under fire. */
    if(!select_unit(seed)){
        fprintf(stderr,"T6 near-Norad SCV cycle missed seed=%d alive=%d at=%d,%d owner=%d selected=%d tick=%u\n",
                seed,u[seed].alive,u[seed].x,u[seed].y,u[seed].owner,selected_count(),(unsigned)host_ticks);
        return 0;
    }
    if(u[seed].type!=U_SCV||selected_count()!=1||selected_building>=0)return 0;
    /* cycle_selection recenters the cursor on the chosen actor. Restore the
     * target point before issuing context-A, with the selection retained. */
    move_cursor(b[target].x,b[target].y);
    tap(KEY_A);
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV &&
            u[i].selected && u[i].order == ORDER_REPAIR &&
            u[i].target == MAX_UNITS + target) ++count;
    fprintf(stderr, "T6 Norad repair issued workers=%d hp=%d/%d\n",
            count, b[target].hp, b[target].maxhp);
    checkpoint(count ? "norad-repair-started" : "norad-repair-unavailable");
    return count;
}

static int all_source_count(int raw, int building, int owner)
{
    int n=0;if(building){for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==raw&&(owner<0||b[i].owner==owner))n++;}
    else {for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==raw&&(owner<0||u[i].owner==owner))n++;}return n;
}

static int source_units_at_location(int raw,int owner,unsigned location)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==raw&&u[i].owner==owner&&
       !SC64_TRANSPORT_LOADED(&u[i])&&point_in_location(location,u[i].x,u[i].y))++n;
    return n;
}

static void trace_prefix(void)
{ if(!trace_first)fputs(",\n",trace_file);trace_first=0; }

static void checkpoint(const char *kind)
{
    if(!trace_file)return;
    trace_prefix();
    int cc=source_building(106,SIDE), refinery=source_building(110,SIDE), factory=source_building(113,SIDE);
    int ccx=cc>=0?b[cc].x:-1, ccy=cc>=0?b[cc].y:-1;
    int dropships_at_goal=source_units_at_location(11,SIDE,2),cargo_loaded=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive){
        if(SC64_TRANSPORT_LOADED(&u[i]))++cargo_loaded;
    }
    fprintf(trace_file,"{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,\"mission\":%d,\"faults\":%d,\"objective_stage\":%d,\"t6_outcome\":%u,\"campaign_phase\":%u,\"objective_flags\":%u,\"race_completed\":%d,\"switch2\":%d,\"rescue_mask\":%u,\"raynor\":%d,\"raynor_alive\":%d,\"raynors_at_goal\":%d,\"norad\":%d,\"norad_alive\":%d,\"side_units\":%d,\"enemy_units\":%d,\"side_buildings\":%d,\"enemy_buildings\":%d,\"minerals\":%d,\"gas\":%d,\"cc_slot\":%d,\"cc_x\":%d,\"cc_y\":%d,\"refinery_slot\":%d,\"factory_slot\":%d,\"starport_slot\":%d,\"tower_slot\":%d,\"dropships\":%d,\"dropships_at_goal\":%d,\"cargo_loaded\":%d,\"raw19_alive\":%d,\"raw126_alive\":%d}",
      kind,(unsigned)host_ticks,(unsigned)frame,screen,won,lost,race_selected_mission,
      mission_unsupported_count,mission_objective_stage,
      (unsigned)race_t6.triggers.player[0].outcome,(unsigned)race_campaign.phase,
      (unsigned)race_campaign.objective_flags,race_completed,
      (race_t6.triggers.switches.bits[0]>>2)&1,race_t6.rescue_mask,
      hero_slot,hero_slot>=0&&u[hero_slot].alive,source_units_at_location(19,SIDE,2),norad_slot,norad_slot>=0&&b[norad_slot].alive,
      owner_units(SIDE),owner_units(ENEMY),owner_buildings(SIDE),owner_buildings(ENEMY),minerals,gas,
      cc,ccx,ccy,refinery,factory,starport_slot,tower_slot,all_source_count(11,0,SIDE),dropships_at_goal,
      cargo_loaded,all_source_count(19,0,-1),all_source_count(126,1,-1));
    (void)refinery;(void)factory;fflush(trace_file);
}

static void inspect_world(void)
{
    fprintf(stderr,"T6 initial world dims=%dx%d slots units=%d buildings=%d minerals=%d gas=%d unsupported=%d\n",race_cols,race_rows,owner_units(SIDE),owner_buildings(SIDE),minerals,gas,mission_unsupported_count);
    fprintf(stderr,"T6 AI requests source1=%u source4=%u rescue_mask=%u Norad=%d hp=%d\n",
            race_t6.ai_requests[1],race_t6.ai_requests[4],race_t6.rescue_mask,
            norad_slot,norad_slot>=0&&b[norad_slot].alive?b[norad_slot].hp:-1);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive)fprintf(stderr,"U slot=%d raw=%d owner=%d rawowner=%d type=%d pos=%d,%d hp=%d order=%d target=%d targetxy=%d,%d flags=%u burrow=%d\n",i,u[i].raw_type,u[i].owner,u[i].raw_owner,u[i].type,u[i].x,u[i].y,u[i].hp,u[i].order,u[i].target,u[i].tx,u[i].ty,u[i].flags,race_t6_unit_burrowed(i));
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive)fprintf(stderr,"B slot=%d raw=%d owner=%d rawowner=%d type=%d pos=%d,%d hp=%d done=%d\n",i,b[i].raw_type,b[i].owner,b[i].raw_owner,b[i].type,b[i].x,b[i].y,b[i].hp,building_complete(i));
    for(unsigned i=0;i<sc64_t6_scenario.location_count;i++){const Sc64T4Location *p=&sc64_t6_scenario.locations[i];fprintf(stderr,"L id=%u bounds=%u,%u..%u,%u center=%u,%u\n",i+1,p->left,p->top,p->right,p->bottom,(p->left+p->right)/2,(p->top+p->bottom)/2);}
    if(hero_slot>=0&&u[hero_slot].alive){int x=-1,y=-1;int d=reachable_point(u[hero_slot].x,u[hero_slot].y,2,&x,&y);fprintf(stderr,"hero-to-goal-ground-route=%d target=%d,%d\n",d,x,y);}
    fprintf(stderr,"geysers=%d",geyser_count);for(int i=0;i<geyser_count;i++)fprintf(stderr," %d,%d",geysers[i].x,geysers[i].y);fputc('\n',stderr);
}

int main(int argc,char **argv)
{
    int inspect=0;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--limit")&&i+1<argc)run_limit=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--trace")&&i+1<argc)trace_path=argv[++i];
        else if(!strcmp(argv[i],"--inspect"))inspect=1;
        else if(!strcmp(argv[i],"--watch-norad"))watch_norad=1;
        else if(!strcmp(argv[i],"--help")){puts("usage: sc64_t6_campaign_run [--limit TICKS] [--trace PATH] [--inspect]");return 0;}
        else return 2;
    }
    if(run_limit<1||run_limit>200000)return 2;
    game_init();
    assert(screen==SCREEN_PLAY&&!race_mode&&race_selected_mission==SC64_T6_GLOBAL_ID&&race_t6_active());
    assert(!won&&!lost&&race_cols==96&&race_rows==96&&!mission_unsupported_count);
    hero_slot=source_unit(19,-1);norad_slot=source_building(126,-1);
    if(inspect){wait_frames(300);inspect_world();return 0;}
    if(trace_path){trace_file=fopen(trace_path,"wb");if(!trace_file)return 2;
        fprintf(trace_file,"{\"trace_schema\":1,\"entry\":\"t6-probe-entry\",\"policy\":\"bounded-normal-controller-attempt\",\"profile\":\"terran-t6\",\"mission\":9,\"tick_limit\":%d,\"events\":[\n",run_limit);}
    checkpoint("start");
    wait_frames(300);checkpoint("opening");
    /* First source-faithful acquisition: bring a human actor within contact
     * range of the completed source-slot-6 Command Center. The contact itself
     * is handled by the T6 world policy; this harness only issues a group move. */
    int rescue_cc=source_building(106,RESCUABLE), rx=-1,ry=-1;
    int lead=hero_slot;
    int repair_workers = 0;
    if(lead<0||!u[lead].alive||u[lead].owner!=SIDE)
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE){lead=i;break;}
    if(rescue_cc>=0&&lead>=0){
        int distance=reachable_building_contact(lead,u[lead].x,u[lead].y,rescue_cc,&rx,&ry);
        fprintf(stderr,"T6 controller source6 base contact cc=%d at=%d,%d lead=%d at=%d,%d route=%d target=%d,%d\n",rescue_cc,b[rescue_cc].x,b[rescue_cc].y,lead,u[lead].x,u[lead].y,distance,rx,ry);
        if(distance>=0&&click_select_unit(lead)){
            tap(KEY_B);tap(KEY_A);move_cursor(rx,ry);tap(KEY_A);
            /* Keep the cursor near Norad while the selected actor travels.
             * On the exact tick the rescued SCV group converts, issue repair
             * before its exposed workers have to survive another long cursor
             * trip across the map. */
            move_cursor(b[norad_slot].x,b[norad_slot].y);
            for(int n=0;n<distance*24+400&&host_ticks<(u32)run_limit&&
                screen==SCREEN_PLAY&&!won&&!lost;n++){
                if(source_building(126,SIDE)>=0)break;
                step(0);
            }
            checkpoint("base-contact-attempt");
            /* Repair is deferred until there is an actual paid gas economy;
             * an underfunded reference repair order stops immediately. */
        } else fprintf(stderr,"T6 base lead click failed lead=%d alive=%d pos=%d,%d target=%d\n",
                       lead,u[lead].alive,u[lead].x,u[lead].y,selection_target());
    }
    /* The source Norad starts damaged and is under pressure from the nearby
     * hostile force. Move combat groups there while the captured base begins
     * the paid economy chain; all orders are ordinary field input. */
    lead=hero_slot;
    if(lead>=0&&u[lead].alive&&u[lead].owner==SIDE){
        int gx=-1,gy=-1;int distance=reachable_point(u[lead].x,u[lead].y,2,&gx,&gy);
        fprintf(stderr,"T6 controller routes full rescued force toward goal-region route=%d target=%d,%d Norad=%d alive=%d HP=%d\n",distance,gx,gy,norad_slot,norad_slot>=0&&b[norad_slot].alive,norad_slot>=0?b[norad_slot].hp:0);
        if(distance>=0){
            int mg=group_visible_type_to(U_MARINE,gx,gy,1);
            int fg=group_visible_type_to(U_FIREBAT,gx,gy,1);
            fprintf(stderr,"T6 combat orders Marine=%d Firebat=%d; Raynor remains unselected\n",mg,fg);
        }
    }
    fprintf(stderr,"T6 Norad repair command count=%d\n",repair_workers);
    int refinery=source_building(110,SIDE),gas_workers=0;
    int refinery_check=0,worker_check=0,tech_check=0;
    if(refinery>=0&&building_complete(refinery)){
        gas_workers=controller_assign_gas(refinery,3);
        fprintf(stderr,"T6 assigned existing Refinery workers=%d wallet=%d/%d\n",gas_workers,minerals,gas);
        checkpoint("refinery-active");
    }
    /* A human cannot afford the paid Refinery at the opening 75-mineral
     * trigger. Keep checking as ordinary workers gather; once the building
     * completes, assign workers through normal context-A controls. Retried
     * SCV production keeps the mineral/gas economy alive after imported
     * workers are lost. Repairs are optional here and the map guard can stay
     * alive without spending the opening worker on a long cross-map route. */
    while(host_ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost){
        step(0);
        if(host_ticks>=(u32)worker_check){
            worker_check=(int)host_ticks+300;
            if(controller_live_scv_count()+controller_queued_scv_count()<4&&
               minerals>=unit_cost(U_SCV)) controller_queue_scv();
        }
        if(host_ticks>=(u32)tech_check){
            tech_check=(int)host_ticks+300;
            int cc=source_building(106,SIDE);
            int nearx=cc>=0?b[cc].x:WORLD_W/2, neary=cc>=0?b[cc].y:WORLD_H/2;
            int factory=source_building(113,SIDE),starport=source_building(114,SIDE);
            if(factory<0&&minerals>=building_cost(B_FACTORY)&&gas>=building_gas_cost(B_FACTORY)){
                int slot=-1;
                if(build_paid(B_FACTORY,10,nearx,neary,240,&slot)){
                    fprintf(stderr,"T6 controller started paid Factory slot=%d type=%d raw=%d owner=%d wallet=%d/%d\n",slot,b[slot].type,b[slot].raw_type,b[slot].owner,minerals,gas);
                    checkpoint("factory-started");
                    wait_building(slot,2400);
                    if(slot>=0&&b[slot].alive&&building_complete(slot)){
                        fprintf(stderr,"T6 controller completed paid Factory slot=%d type=%d raw=%d owner=%d alive=%d\n",slot,b[slot].type,b[slot].raw_type,b[slot].owner,b[slot].alive);
                        checkpoint("factory-complete");
                    }
                }
            }
            factory=source_building(113,SIDE);starport=source_building(114,SIDE);
            if(factory>=0&&building_complete(factory)&&starport<0&&
               minerals>=building_cost(B_STARPORT)&&gas>=building_gas_cost(B_STARPORT)){
                int slot=-1;
                if(controller_build_starport(factory,&slot)){
                    fprintf(stderr,"T6 controller started paid Starport slot=%d type=%d raw=%d owner=%d wallet=%d/%d\n",slot,b[slot].type,b[slot].raw_type,b[slot].owner,minerals,gas);
                    checkpoint("starport-started");
                    wait_building(slot,2400);
                    if(slot>=0&&b[slot].alive&&building_complete(slot)){
                        fprintf(stderr,"T6 controller completed paid Starport slot=%d type=%d raw=%d owner=%d alive=%d lookup=%d\n",slot,b[slot].type,b[slot].raw_type,b[slot].owner,b[slot].alive,source_building(114,SIDE));
                        checkpoint("starport-complete");
                    }
                }
            }
            starport=source_building(114,SIDE);
            if(starport>=0&&building_complete(starport)&&
               !race_t6_starport_has_control_tower(starport)&&
               minerals>=building_cost(B_CONTROL_TOWER)&&gas>=building_gas_cost(B_CONTROL_TOWER))
                controller_build_tower(starport);
            if(starport>=0&&race_t6_starport_has_control_tower(starport)){
                int live=0,queued=0;
                for(int i=0;i<MAX_UNITS;i++)
                    if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DROPSHIP)++live;
                for(int i=0;i<b[starport].queue_count&&i<5;i++)
                    if(b[starport].queue[(b[starport].queue_head+i)%5]==U_DROPSHIP)++queued;
                if(live+queued<2)controller_queue_dropship(starport);
            }
            for(int i=0;i<MAX_UNITS&&host_ticks<(u32)run_limit;i++)
                if(!dropship_goal_attempted[i]&&u[i].alive&&u[i].owner==SIDE&&
                   u[i].type==U_DROPSHIP){
                    dropship_goal_attempted[i]=1;
                    int hero_needs_ferry=hero_slot>=0&&u[hero_slot].alive&&
                        !SC64_TRANSPORT_LOADED(&u[hero_slot])&&
                        !point_in_location(2,u[hero_slot].x,u[hero_slot].y);
                    if(hero_needs_ferry&&controller_board_raynor(i)&&
                       controller_move_dropship_to_goal(i))
                        (void)controller_unload_raynor_at_goal(i,u[i].x,u[i].y);
                    else if(!hero_needs_ferry)
                        (void)controller_move_dropship_to_goal(i);
                    if(won||lost)break;
                }
        }
        if(rescue_cc>=0&&source_building(106,SIDE)>=0&&refinery<0&&
           host_ticks>=(u32)refinery_check){
            refinery_check=(int)host_ticks+60;
            if(minerals>=building_cost(B_REFINERY)&&
               controller_build_refinery(&refinery)){
                if(wait_building(refinery,1000)){
                    fprintf(stderr,"T6 controller completed Refinery slot=%d\n",refinery);
                    gas_workers=controller_assign_gas(refinery,3);
                    fprintf(stderr,"T6 assigned refinery workers=%d wallet=%d/%d\n",gas_workers,minerals,gas);
                    checkpoint("refinery-active");
                } else {
                    fprintf(stderr,"T6 Refinery not complete by bounded wait slot=%d tick=%u\n",
                            refinery,(unsigned)host_ticks);
                    refinery=-1;
                }
            }
        }
    }
    checkpoint(won?"won":lost?"lost":"limit");
    if(trace_file){fputs("\n]}\n",trace_file);fclose(trace_file);}
    int goal_raynors=source_units_at_location(19,SIDE,2),goal_ships=source_units_at_location(11,SIDE,2);
    fprintf(stderr,"T6 probe result ticks=%u frame=%u won=%d lost=%d raw19=%d@goal%d raw11=%d@goal%d raw126=%d SIDE=%d/%d ENEMY=%d/%d wallet=%d/%d faults=%d outcome=%u completed=%d\n",(unsigned)host_ticks,(unsigned)frame,won,lost,all_source_count(19,0,-1),goal_raynors,all_source_count(11,0,SIDE),goal_ships,all_source_count(126,1,-1),owner_units(SIDE),owner_buildings(SIDE),owner_units(ENEMY),owner_buildings(ENEMY),minerals,gas,mission_unsupported_count,(unsigned)race_t6.triggers.player[0].outcome,race_completed);
    int loaded=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&SC64_TRANSPORT_LOADED(&u[i]))++loaded;
    return inspect || (won&&!lost&&mission_unsupported_count==0&&
                       race_t6.triggers.player[0].outcome==SC64_T4_TR_WON&&
                       race_campaign.phase==SC64_RACE_PHASE_COMPLETE&&
                       (race_campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)&&
                       (race_completed&(1<<SC64_T6_GLOBAL_ID))&&goal_raynors>=1&&goal_ships>=2&&
                       hero_slot>=0&&u[hero_slot].alive&&norad_slot>=0&&b[norad_slot].alive&&!loaded) ? 0 : 1;
}
