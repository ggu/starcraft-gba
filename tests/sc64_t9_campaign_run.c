/*
 * Normal-controller T9 (New Gettysburg) campaign attempt.
 *
 * The compile-time probe selects mission 12 for a host fixture. After
 * game_init(), all simulation changes are made through game_update() with
 * ordinary buttons. The runner reads the world and immutable scenario tables
 * only to plan orders and report source objectives; it never writes actors,
 * triggers, resources, or mission state.
 */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SC64_TERRAN_LATE_PROBE_MISSION
#error "compile with the private late-mission probe entry"
#endif
#if SC64_TERRAN_LATE_PROBE_MISSION != 12
#error "this controller fixture is for global mission 12 (T9 New Gettysburg)"
#endif

enum { T9_DEFAULT_LIMIT = 500000, T9_CHECKPOINT_PERIOD = 1000 };
static FILE *trace;
static int trace_first_event = 1;
static u16 previous_keys;
static int limit_ticks = T9_DEFAULT_LIMIT;
static u32 host_tick;
static int failed_action;
static int gas_worker_slot = -1;
static int home_miner_slot = -1;
static int select_unit(int slot);
static void move_cursor(int x, int y);
static int queue_scv(void);
static int issue_wraith_group_order(int x, int y);

static void trace_event_prefix(void)
{
    if (!trace) return;
    if (!trace_first_event) fputs(",\n", trace);
    trace_first_event = 0;
}

static int clamp_coord(int v, int max) { return v < 0 ? 0 : v >= max ? max - 1 : v; }
static int snap4(int from, int to, int max)
{
    int delta = to - from;
    int steps = delta >= 0 ? (delta + 2) / 4 : (delta - 2) / 4;
    return clamp_coord(from + steps * 4, max);
}

static void checkpoint(const char *kind)
{
    if (!trace) return;
    int p2_buildings = 0, zerg_buildings = 0;
    int raw16 = 0, side_units = 0, zerg_units = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].raw_owner == 1) ++p2_buildings;
        if (b[i].raw_owner == 3) ++zerg_buildings;
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].raw_type == 16) ++raw16;
        if (u[i].owner == SIDE) ++side_units;
        if (u[i].owner == 4) ++zerg_units;
    }
    trace_event_prefix();
    fprintf(trace,
        "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,\"screen\":%d,"
        "\"won\":%d,\"lost\":%d,\"mission\":%d,\"faults\":%d,"
        "\"phase\":%u,\"objective_flags\":%u,\"race_completed\":%d,"
        "\"protoss_force2_buildings\":%d,\"zerg_force3_buildings\":%d,"
        "\"raw16_alive\":%d,\"side_units\":%d,\"zerg_units\":%d,"
        "\"minerals\":%d,\"gas\":%d,\"selection\":%d,\"selected_building\":%d,"
        "\"failed_action\":%d}",
        kind, (unsigned)host_tick, (unsigned)frame, screen, won, lost,
        race_selected_mission, mission_unsupported_count,
        (unsigned)race_campaign.phase, (unsigned)race_campaign.objective_flags,
        race_completed, p2_buildings, zerg_buildings, raw16, side_units,
        zerg_units, minerals, gas, selected_count(), selected_building,
        failed_action);
    fflush(trace);
}

static void step(u16 held)
{
    if (host_tick >= (u32)limit_ticks || screen != SCREEN_PLAY || won || lost) return;
    int zerg_hp[MAX_BUILDINGS];
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        zerg_hp[i] = b[i].alive && b[i].raw_owner == 3 ? b[i].hp : -1;
    u16 pressed = held & (u16)~previous_keys;
    int input_changed = held != previous_keys;
    game_update(held, pressed);
    previous_keys = held;
    ++host_tick;
    if (trace && input_changed) {
        trace_event_prefix();
        fprintf(trace,"{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}",
                (unsigned)host_tick, held, pressed);
        fflush(trace);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (zerg_hp[i] >= 0 && (!b[i].alive || b[i].hp < zerg_hp[i])) {
            fprintf(stderr,"T9 Zerg building damage tick=%u slot=%d oldhp=%d newhp=%d alive=%d raw=%d at=%d,%d\n",
                    (unsigned)host_tick,i,zerg_hp[i],b[i].hp,b[i].alive,
                    b[i].raw_type,b[i].x,b[i].y);
            for (int j=0;j<MAX_UNITS;++j)
                if (u[j].alive && u[j].order==ORDER_ATTACK_TARGET &&
                    u[j].target==MAX_UNITS+i)
                    fprintf(stderr," T9 current attacker slot=%d owner=%d rawowner=%d raw=%d at=%d,%d\n",
                            j,u[j].owner,u[j].raw_owner,u[j].raw_type,u[j].x,u[j].y);
            break;
        }
    if (trace && host_tick % T9_CHECKPOINT_PERIOD == 0) checkpoint("periodic");
}

static void tap(u16 key) { step(key); step(0); }
static void wait_ticks(int count)
{ for (int i = 0; i < count && host_tick < (u32)limit_ticks && screen == SCREEN_PLAY && !won && !lost; ++i) step(0); }

static int normal_field(void)
{
    for (int i = 0; i < 3 && (menu_open || mode != MODE_NORMAL); ++i) tap(KEY_B);
    return screen == SCREEN_PLAY && !menu_open && mode == MODE_NORMAL;
}

static int select_unit(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        u[slot].owner != SIDE || !normal_field()) return 0;
    if (selected_building >= 0) { tap(KEY_B); tap(KEY_A); tap(KEY_A); }
    for (int n = 0; n <= MAX_UNITS && host_tick < (u32)limit_ticks; ++n) {
        if (u[slot].selected && selected_count() == 1 && selected_building < 0)
            return 1;
        tap(KEY_L);
    }
    return 0;
}

static int click_select_unit(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        u[slot].owner != SIDE || !normal_field()) return 0;
    if (selected_building >= 0) { tap(KEY_B); tap(KEY_A); tap(KEY_A); }
    static const int dx[8] = {-16, 16, 0, 0, -24, 24, 0, 0};
    static const int dy[8] = {0, 0, -16, 16, 0, 0, -24, 24};
    for (int attempt = 0; attempt < 9 && u[slot].alive; ++attempt) {
        int x = u[slot].x, y = u[slot].y;
        if (attempt) { x = clamp_coord(x + dx[attempt - 1], WORLD_W);
                       y = clamp_coord(y + dy[attempt - 1], WORLD_H); }
        move_cursor(x, y);
        if (u[slot].alive && selection_target() == slot) {
            tap(KEY_A);
            return u[slot].alive && u[slot].selected && selected_count() == 1 &&
                selected_building < 0;
        }
    }
    return 0;
}

static void move_cursor(int x, int y)
{
    x = snap4(cx, x, WORLD_W); y = snap4(cy, y, WORLD_H);
    for (int i = 0; i < 3000 && (cx != x || cy != y) && host_tick < (u32)limit_ticks; ++i) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT; if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN; if (cy > y) held |= KEY_UP;
        step(held);
    }
    step(0);
}

static int group_selection(void)
{
    if (!normal_field()) return 0;
    int left = WORLD_W, top = WORLD_H, right = -1, bottom = -1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        sc64_game_unit_selectable(u[i].type) && !SC64_TRANSPORT_LOADED(&u[i])) {
        if (u[i].x < left) left = u[i].x;
        if (u[i].x > right) right = u[i].x;
        if (u[i].y < top) top = u[i].y;
        if (u[i].y > bottom) bottom = u[i].y;
    }
    if (right < left || bottom < top) return 0;
    move_cursor(clamp_coord(left - 8, WORLD_W), clamp_coord(top - 8, WORLD_H));
    step(KEY_L);
    int end_x = snap4(cx, clamp_coord(right + 8, WORLD_W), WORLD_W);
    int end_y = snap4(cy, clamp_coord(bottom + 8, WORLD_H), WORLD_H);
    for (int i = 0; i < 3000 && (cx != end_x || cy != end_y); ++i) {
        u16 held = KEY_L | KEY_R;
        if (cx < end_x) held |= KEY_RIGHT; if (cx > end_x) held |= KEY_LEFT;
        if (cy < end_y) held |= KEY_DOWN; if (cy > end_y) held |= KEY_UP;
        step(held);
    }
    step(0);
    if (selected_building >= 0 || !selected_count()) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected && u[i].owner != SIDE) return 0;
    return 1;
}

static int issue_attack_move(int x, int y)
{
    /* Keep the source Kerrigan and workers at home for this first probe.
     * Selecting all SIDE units sent the objective actor into the enemy
     * formation and caused the source defeat trigger. */
    if (!normal_field()) return 0;
    /* Queueing a Wraith leaves the Starport selected. Clear that building
     * selection through the ordinary menu before drawing a unit selection. */
    if (selected_building >= 0) { tap(KEY_B); tap(KEY_A); tap(KEY_A); }
    int left = WORLD_W, top = WORLD_H, right = -1, bottom = -1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        (u[i].type == U_MARINE || u[i].type == U_WRAITH) &&
        !SC64_TRANSPORT_LOADED(&u[i])) {
        if (u[i].x < left) left = u[i].x;
        if (u[i].x > right) right = u[i].x;
        if (u[i].y < top) top = u[i].y;
        if (u[i].y > bottom) bottom = u[i].y;
    }
    if (right < left || bottom < top) return 0;
    move_cursor(clamp_coord(left - 4, WORLD_W), clamp_coord(top - 4, WORLD_H));
    step(KEY_L);
    int end_x = snap4(cx, clamp_coord(right + 4, WORLD_W), WORLD_W);
    int end_y = snap4(cy, clamp_coord(bottom + 4, WORLD_H), WORLD_H);
    for (int i = 0; i < 1000 && (cx != end_x || cy != end_y); ++i) {
        u16 held = KEY_L | KEY_R;
        if (cx < end_x) held |= KEY_RIGHT; if (cx > end_x) held |= KEY_LEFT;
        if (cy < end_y) held |= KEY_DOWN; if (cy > end_y) held |= KEY_UP;
        step(held);
    }
    step(0);
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
        if (u[i].owner != SIDE ||
            (u[i].type != U_MARINE && u[i].type != U_WRAITH)) return 0;
        ++count;
    }
    if (!count || count != selected_count() || selected_building >= 0) return 0;
    tap(KEY_B); tap(KEY_DOWN); tap(KEY_A);
    move_cursor(x, y); tap(KEY_A);
    fprintf(stderr, "T9 issued attacker attack-move target=%d,%d selected=%d cursor=%d,%d\n",
            x, y, selected_count(), cx, cy);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        (u[i].type == U_MARINE || u[i].type == U_WRAITH))
        fprintf(stderr, "  marine slot=%d at=%d,%d order=%d target=%d tx=%d ty=%d\n",
                i, u[i].x, u[i].y, u[i].order, u[i].target, u[i].tx, u[i].ty);
    return 1;
}

static int issue_wraith_orders(int x, int y)
{
    int ordered = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_WRAITH &&
            !SC64_TRANSPORT_LOADED(&u[i]) && click_select_unit(i)) {
            tap(KEY_B); tap(KEY_DOWN); tap(KEY_A);
            move_cursor(x,y); tap(KEY_A);
            if (u[i].alive && u[i].order == ORDER_ATTACK_MOVE) ++ordered;
        }
    fprintf(stderr,"T9 Wraith attack orders=%d target=%d,%d tick=%u\n",
            ordered,x,y,(unsigned)host_tick);
    return ordered > 0;
}

static int select_visible_wraith_group(void)
{
    if (!normal_field()) return 0;
    int seed = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_WRAITH &&
            !SC64_TRANSPORT_LOADED(&u[i])) { seed = i; break; }
    if (seed < 0 || !click_select_unit(seed)) return 0;
    tap(KEY_A);
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
        if (u[i].owner != SIDE || u[i].type != U_WRAITH ||
            SC64_TRANSPORT_LOADED(&u[i])) return 0;
        ++count;
    }
    return count > 0 && count == selected_count() && selected_building < 0;
}

enum { T9_ROUTE_STEP = 32, T9_ROUTE_CELLS = 4096 };
static int t9_air_risk_at(int x, int y)
{
    int risk = 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && owners_hostile(SIDE,u[i].owner)) {
        int damage=sc64_race_unit_damage_vs(u[i].type,1);
        if (damage<=0) continue;
        int range=(sc64_race_unit_range_vs(u[i].type,1)+1)/2;
        int d2=dist2(x,y,u[i].x,u[i].y), r=range+20;
        if (d2<r*r) risk += 1+(r*r-d2)/1024;
    }
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive && owners_hostile(SIDE,b[i].owner)) {
        /* Pinned static air-defense envelope in GBA pixels: Protoss Photon
         * Cannons, Zerg Spore Colonies, and Terran Missile Turrets. */
        int range=(b[i].raw_type==124 || b[i].raw_type==144 || b[i].raw_type==162)?112:0;
        if (!range) continue;
        int d2=dist2(x,y,b[i].x,b[i].y), r=range+20;
        if (d2<r*r) risk += 1+(r*r-d2)/1024;
    }
    return risk>1000?1000:risk;
}

static int t9_threat_route(int sx,int sy,int gx,int gy,int out_x[256],int out_y[256])
{
    int cols=(WORLD_W+T9_ROUTE_STEP-1)/T9_ROUTE_STEP;
    int rows=(WORLD_H+T9_ROUTE_STEP-1)/T9_ROUTE_STEP;
    int cells=cols*rows;
    if (cells<=0 || cells>T9_ROUTE_CELLS) return 0;
    static u16 risk[T9_ROUTE_CELLS];
    static u32 cost[T9_ROUTE_CELLS];
    static s16 previous[T9_ROUTE_CELLS];
    static u8 done[T9_ROUTE_CELLS];
    for(int row=0;row<rows;++row) for(int col=0;col<cols;++col) {
        int at=col+row*cols;
        int x=col*T9_ROUTE_STEP+T9_ROUTE_STEP/2;
        int y=row*T9_ROUTE_STEP+T9_ROUTE_STEP/2;
        if(x>=WORLD_W)x=WORLD_W-1;if(y>=WORLD_H)y=WORLD_H-1;
        risk[at]=(u16)t9_air_risk_at(x,y);
        cost[at]=0x7fffffffu;previous[at]=-1;done[at]=0;
    }
    int sc=clamp_coord(sx/T9_ROUTE_STEP,cols),sr=clamp_coord(sy/T9_ROUTE_STEP,rows);
    int gc=clamp_coord(gx/T9_ROUTE_STEP,cols),gr=clamp_coord(gy/T9_ROUTE_STEP,rows);
    int start=sc+sr*cols,goal=gc+gr*cols;cost[start]=0;
    for(int n=0;n<cells;++n) {
        int at=-1;u32 best=0x7fffffffu;
        for(int i=0;i<cells;++i)if(!done[i]&&cost[i]<best){best=cost[i];at=i;}
        if(at<0||at==goal)break;done[at]=1;
        int col=at%cols,row=at/cols;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy) {
            int nc=col+dx,nr=row+dy;
            if(nc<0||nr<0||nc>=cols||nr>=rows)continue;
            int next=nc+nr*cols;if(done[next])continue;
            u32 edge=(dx&&dy?14u:10u)+(u32)risk[next]*18u+(u32)risk[at]*5u;
            if(best<=0x7fffffffu-edge&&best+edge<cost[next]){
                cost[next]=best+edge;previous[next]=(s16)at;
            }
        }
    }
    if(cost[goal]==0x7fffffffu)return 0;
    int reverse[T9_ROUTE_CELLS],n=0;
    for(int at=goal;at>=0&&n<cells;at=previous[at]){
        reverse[n++]=at;if(at==start)break;
    }
    if(!n||reverse[n-1]!=start)return 0;
    int emitted=0;
    for(int k=n-1;k>=0&&emitted<256;--k) {
        int idx=reverse[k];
        int keep=(k==n-1||k==0||k%6==0);
        if(k<n-1&&k>0) {
            int prev=reverse[k+1],next=reverse[k-1];
            int dx1=idx%cols-prev%cols,dy1=idx/cols-prev/cols;
            int dx2=next%cols-idx%cols,dy2=next/cols-idx/cols;
            if(dx1!=dx2||dy1!=dy2)keep=1;
        }
        if(keep){out_x[emitted]=(idx%cols)*T9_ROUTE_STEP+T9_ROUTE_STEP/2;
            out_y[emitted]=(idx/cols)*T9_ROUTE_STEP+T9_ROUTE_STEP/2;++emitted;}
    }
    return emitted;
}

static int issue_wraith_group_move(int x,int y)
{
    if(!select_visible_wraith_group())return 0;
    tap(KEY_B);tap(KEY_A);move_cursor(x,y);tap(KEY_A);
    int moved=0;
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].selected&&
        u[i].owner==SIDE&&u[i].type==U_WRAITH&&u[i].order==ORDER_MOVE)++moved;
    return moved>0;
}

static int send_wraith_wave_to(int target)
{
    if(target<0||target>=MAX_BUILDINGS||!b[target].alive)return 0;
    int sx=0,sy=0,n=0;
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].type==U_WRAITH&&!SC64_TRANSPORT_LOADED(&u[i])){sx+=u[i].x;sy+=u[i].y;++n;}
    if(n<6)return 0;sx/=n;sy/=n;
    int wx[256],wy[256];
    int count=t9_threat_route(sx,sy,b[target].x,b[target].y,wx,wy);
    if(!count)return 0;
    fprintf(stderr,"T9 safe Wraith route count=%d wing=%d target=%d at=%d,%d\n",
            count,n,target,b[target].x,b[target].y);
    for(int k=0;k<count&&screen==SCREEN_PLAY&&!won&&!lost;++k) {
        if(!issue_wraith_group_move(wx[k],wy[k]))return 0;
        for(int t=0;t<1800&&screen==SCREEN_PLAY&&!won&&!lost;++t) {
            int near=0;
            for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].owner==SIDE&&
                u[i].type==U_WRAITH&&!SC64_TRANSPORT_LOADED(&u[i])&&
                dist2(u[i].x,u[i].y,wx[k],wy[k])<72*72)++near;
            if(near>=n*2/3)break;
            step(0);
        }
    }
    return issue_wraith_group_order(b[target].x,b[target].y);
}

static int issue_wraith_group_order(int x, int y)
{
    if (!select_visible_wraith_group()) return issue_wraith_orders(x,y);
    tap(KEY_B); tap(KEY_DOWN); tap(KEY_A);
    move_cursor(x,y); tap(KEY_A);
    int ordered = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected && u[i].owner == SIDE &&
            u[i].type == U_WRAITH && u[i].order == ORDER_ATTACK_MOVE) ++ordered;
    fprintf(stderr,"T9 Wraith group attack orders=%d/%d target=%d,%d tick=%u\n",
            ordered,selected_count(),x,y,(unsigned)host_tick);
    return ordered > 0;
}

static int first_force_building(unsigned raw_owner)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_owner == (int)raw_owner) return i;
    return -1;
}

static int nearest_force_building(unsigned raw_owner)
{
    int best_slot = -1, best_distance = 0x7fffffff;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].raw_owner == (int)raw_owner) {
        int distance = 0x7fffffff;
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
            (u[j].type == U_MARINE || u[j].type == U_WRAITH) &&
            !SC64_TRANSPORT_LOADED(&u[j])) {
            int d = dist2(u[j].x, u[j].y, b[i].x, b[i].y);
            if (d < distance) distance = d;
        }
        if (distance < best_distance) { best_distance = distance; best_slot = i; }
    }
    return best_slot;
}

static int living_force_buildings(unsigned raw_owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_owner == (int)raw_owner) ++count;
    return count;
}

static int source_building(int raw_type, int owner)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw_type &&
            (owner < 0 || b[i].owner == owner)) return i;
    return -1;
}

static int source_unit(int raw_type, int owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == raw_type &&
            (owner < 0 || u[i].owner == owner)) return i;
    return -1;
}

static int ground_reachable(int from_x, int from_y, int to_x, int to_y)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS];
    int start = clamp_coord(from_x / 16, MAP_COLS) + clamp_coord(from_y / 16, MAP_ROWS) * MAP_COLS;
    int goal = clamp_coord(to_x / 16, MAP_COLS) + clamp_coord(to_y / 16, MAP_ROWS) * MAP_COLS;
    int head = 0, tail = 0;
    memset(seen, 0, sizeof(seen));
    if (!SC64_GRID_GET(path_walkable, start) || !SC64_GRID_GET(path_walkable, goal)) return 0;
    queue[tail++] = (u16)start; seen[start] = 1;
    while (head < tail) {
        int cell = queue[head++], x = cell % MAP_COLS, y = cell / MAP_COLS;
        if (cell == goal) return 1;
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            int nx = x + dx, ny = y + dy, next;
            if ((!dx && !dy) || nx < 0 || ny < 0 || nx >= MAP_COLS || ny >= MAP_ROWS) continue;
            next = nx + ny * MAP_COLS;
            if (!seen[next] && SC64_GRID_GET(path_walkable, next)) {
                seen[next] = 1; queue[tail++] = (u16)next;
            }
        }
    }
    return 0;
}

static int campaign_attacker(int type)
{ return type == U_MARINE || type == U_VULTURE || type == U_WRAITH; }

static int owner_units(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++count;
    return count;
}

static int side_wraith_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_WRAITH &&
            !SC64_TRANSPORT_LOADED(&u[i])) ++count;
    return count;
}

static int side_scv_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 7) ++count;
    return count;
}

static int assign_worker_to_mineral(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE ||
        u[slot].raw_type != 7 || !normal_field()) return 0;
    int cc = source_building(106, SIDE);
    int patch = -1, best = 0x7fffffff;
    int bx = cc >= 0 ? b[cc].x : u[slot].x;
    int by = cc >= 0 ? b[cc].y : u[slot].y;
    for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0) {
        int d = dist2(bx, by, m[i].x, m[i].y);
        if (d < best) { best = d; patch = i; }
    }
    if (patch < 0 || patch >= MAX_MINERALS) { fprintf(stderr,"T9 worker %d no patch=%d\n",slot,patch); return 0; }
    /* There is no direct mineral-click command in this field UI. Put the SCV
     * at the home mineral line with an ordinary Move order, then Stop it so
     * the normal idle-worker economy behavior assigns the nearest patch. */
    if (!click_select_unit(slot)) return 0;
    tap(KEY_B); tap(KEY_A); /* command menu -> Move */
    move_cursor(m[patch].x, m[patch].y);
    tap(KEY_A);
    if (!u[slot].alive || u[slot].order != ORDER_MOVE) return 0;
    for (int tick = 0; tick < 900 && u[slot].alive &&
         dist2(u[slot].x, u[slot].y, m[patch].x, m[patch].y) > 24 * 24; ++tick)
        step(0);
    if (!u[slot].alive) return 0;
    int distance_to_patch = dist2(u[slot].x, u[slot].y, m[patch].x, m[patch].y);
    tap(KEY_B);
    for (int row = 0; row < MENU_STOP; ++row) tap(KEY_DOWN);
    tap(KEY_A); /* Stop -> ordinary idle economy can select nearby minerals. */
    for (int tick = 0; tick < 40 && u[slot].alive &&
         u[slot].order != ORDER_MINE; ++tick) step(0);
    fprintf(stderr,"T9 worker home-line slot=%d intended_patch=%d distance=%d order=%d target=%d at=%d,%d wallet=%d\n",
            slot,patch,distance_to_patch,u[slot].order,u[slot].target,u[slot].x,u[slot].y,minerals);
    int assigned = u[slot].alive && u[slot].order == ORDER_MINE &&
        u[slot].target >= 0 && u[slot].target < MAX_MINERALS;
    if (assigned) home_miner_slot = slot;
    return assigned;
}

static int select_building(int slot)
{
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive ||
        b[slot].owner != SIDE || !normal_field()) return 0;
    if (selected_building >= 0) { tap(KEY_B); tap(KEY_A); tap(KEY_A); }
    move_cursor(b[slot].x, b[slot].y);
    tap(KEY_A);
    return selected_building == slot && selected_count() == 0;
}

static int queue_marine(void)
{
    int barracks = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_BARRACKS &&
            building_complete(i)) { barracks = i; break; }
    if (barracks < 0 || b[barracks].queue_count >= 5 ||
        minerals < unit_cost(U_MARINE) ||
        supply_used + unit_supply(U_MARINE) > supply_cap ||
        !select_building(barracks)) return 0;
    int before = b[barracks].queue_count;
    tap(KEY_A);
    return b[barracks].queue_count > before;
}

static int choose_build_site(int type, int ox, int oy, int *out_x, int *out_y)
{
    int best = 0x7fffffff, found = 0;
    for (int ring = 1; ring <= 12; ++ring) {
        for (int dx = -ring; dx <= ring; ++dx) {
            for (int side = -1; side <= 1; side += 2) {
                int dy = side * ring;
                int x = clamp_coord(ox + dx * 16, WORLD_W);
                int y = clamp_coord(oy + dy * 16, WORLD_H);
                int score = dx * dx + dy * dy;
                if (score < best && race_can_site(SIDE,type,x,y) && can_place(type,x,y)) {
                    *out_x = x; *out_y = y; best = score; found = 1;
                }
            }
        }
        for (int dy = -ring + 1; dy < ring; ++dy) {
            for (int side = -1; side <= 1; side += 2) {
                int dx = side * ring;
                int x = clamp_coord(ox + dx * 16, WORLD_W);
                int y = clamp_coord(oy + dy * 16, WORLD_H);
                int score = dx * dx + dy * dy;
                if (score < best && race_can_site(SIDE,type,x,y) && can_place(type,x,y)) {
                    *out_x = x; *out_y = y; best = score; found = 1;
                }
            }
        }
    }
    return found;
}

static int build_from_worker(int slot, int type, int menu_choice,
                             int raw_type, int x, int y)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        u[slot].owner != SIDE || !IS_WORKER(u[slot].type) ||
        !click_select_unit(slot)) return 0;
    tap(KEY_B);
    for (int row = 0; row < menu_choice; ++row) tap(KEY_DOWN);
    tap(KEY_A);
    if (mode != MODE_BUILD || menu != type) return 0;
    move_cursor(x,y);
    tap(KEY_A);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw_type && b[i].owner == SIDE &&
            dist2(b[i].x,b[i].y,x,y) < 8*8) return 1;
    return 0;
}

static int build_starport(void)
{
    int cc = source_building(106,SIDE), worker = -1, nearest = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 7 &&
        !SC64_TRANSPORT_LOADED(&u[i]) && u[i].order != ORDER_BUILD) {
        int d = cc >= 0 ? dist2(u[i].x,u[i].y,b[cc].x,b[cc].y) : 0;
        if (d < nearest) { nearest=d; worker=i; }
    }
    int x, y;
    if (cc < 0 || worker < 0 || minerals < 150 || gas < 100 ||
        !race_late_building_product_available(SIDE,u[worker].raw_owner,B_STARPORT) ||
        !choose_build_site(B_STARPORT,b[cc].x,b[cc].y,&x,&y)) return 0;
    fprintf(stderr,"T9 try Starport source PUNI/buildsite=%d,%d wallet=%d/%d\n",x,y,minerals,gas);
    return build_from_worker(worker,B_STARPORT,MENU_STARPORT,114,x,y);
}

static int side_starport_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].raw_type == 114) ++count;
    return count;
}

static int build_supply_depot(void)
{
    int cc = source_building(106, SIDE), worker = -1, nearest = 0x7fffffff;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_DEPOT &&
            b[i].under_construction) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        u[i].raw_type == 7 && !SC64_TRANSPORT_LOADED(&u[i]) &&
        u[i].order != ORDER_GAS && u[i].order != ORDER_BUILD) {
        int d = cc >= 0 ? dist2(u[i].x,u[i].y,b[cc].x,b[cc].y) : 0;
        if (d < nearest) { nearest=d; worker=i; }
    }
    int x, y;
    if (cc < 0 || worker < 0 || minerals < 100 ||
        !race_late_building_product_available(SIDE,u[worker].raw_owner,B_DEPOT) ||
        !choose_build_site(B_DEPOT,b[cc].x,b[cc].y,&x,&y)) return 0;
    fprintf(stderr,"T9 try paid Supply Depot at=%d,%d wallet=%d/%d reserved=%d cap=%d\n",
            x,y,minerals,gas,reserved_supply(),supply_cap);
    return build_from_worker(worker,B_DEPOT,MENU_DEPOT,109,x,y);
}

static int build_refinery(void)
{
    int worker = source_unit(7,SIDE), cc = source_building(106,SIDE);
    int geyser = -1, best = 0x7fffffff;
    if (worker < 0 || cc < 0 || minerals < 100) return 0;
    for (int i = 0; i < geyser_count; ++i) {
        int distance = dist2(b[cc].x,b[cc].y,geysers[i].x,geysers[i].y);
        if (distance < best) { best = distance; geyser = i; }
    }
    if (geyser < 0 || !race_late_building_product_available(SIDE,u[worker].raw_owner,B_REFINERY))
        return 0;
    int x = geysers[geyser].x, y = geysers[geyser].y;
    fprintf(stderr,"T9 try Refinery geyser=%d at=%d,%d wallet=%d/%d\n",geyser,x,y,minerals,gas);
    return build_from_worker(worker,B_REFINERY,MENU_REFINERY,110,x,y);
}

static int assign_worker_to_gas(int slot)
{
    int refinery = source_building(110,SIDE);
    if (refinery < 0 || slot < 0 || slot >= MAX_UNITS ||
        !u[slot].alive || u[slot].raw_type != 7 || !click_select_unit(slot)) return 0;
    move_cursor(b[refinery].x,b[refinery].y);
    tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_GAS && u[slot].target == refinery;
}

static int add_gas_worker(void)
{
    int refinery = source_building(110,SIDE), before = side_scv_count();
    u8 existed[MAX_UNITS];
    for (int i = 0; i < MAX_UNITS; ++i)
        existed[i] = (u8)(u[i].alive && u[i].owner == SIDE && u[i].raw_type == 7);
    if (refinery < 0 || !queue_scv()) return 0;
    for (int i = 0; i < 1800 && side_scv_count() == before; ++i) step(0);
    if (side_scv_count() <= before) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (!existed[i] && u[i].alive && u[i].owner == SIDE && u[i].raw_type == 7 &&
            i != home_miner_slot && i != gas_worker_slot &&
            !SC64_TRANSPORT_LOADED(&u[i]) && u[i].order != ORDER_GAS &&
            u[i].order != ORDER_BUILD) {
            if (assign_worker_to_gas(i)) {
                fprintf(stderr,"T9 extra gas worker assigned slot=%d tick=%u wallet=%d/%d\n",
                        i,(unsigned)host_tick,minerals,gas);
                return 1;
            }
        }
    return 0;
}

static int build_gas_outpost(void)
{
    int refinery = source_building(110, SIDE);
    int worker = gas_worker_slot;
    if (refinery < 0 || worker < 0 || worker >= MAX_UNITS || !u[worker].alive ||
        minerals < 400 || !race_late_building_product_available(
            SIDE, u[worker].raw_owner, B_COMMAND)) return 0;
    int x = 0, y = 0;
    if (!choose_build_site(B_COMMAND, b[refinery].x, b[refinery].y, &x, &y))
        return 0;
    fprintf(stderr,"T9 try paid gas outpost at=%d,%d wallet=%d/%d worker=%d\n",
            x,y,minerals,gas,worker);
    if (!build_from_worker(worker,B_COMMAND,MENU_COMMAND_CENTER,106,x,y)) return 0;
    int outpost = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].raw_type == 106 &&
            dist2(b[i].x,b[i].y,x,y) < 8*8) { outpost = i; break; }
    for (int i = 0; i < 5000 && outpost >= 0 && !building_complete(outpost); ++i) step(0);
    if (outpost < 0 || !building_complete(outpost)) return 0;
    /* Construction leaves the SCV idle. Restore the real refinery order. */
    if (u[worker].alive && !(u[worker].order == ORDER_GAS && u[worker].target == refinery))
        (void)assign_worker_to_gas(worker);
    fprintf(stderr,"T9 paid gas outpost complete slot=%d worker=%d gas-order=%d tick=%u\n",
            outpost,worker,u[worker].order == ORDER_GAS && u[worker].target == refinery,
            (unsigned)host_tick);
    return 1;
}

static int assign_spare_worker_to_minerals(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 7 &&
            i != home_miner_slot && i != gas_worker_slot &&
            !SC64_TRANSPORT_LOADED(&u[i]) && u[i].order != ORDER_GAS &&
            u[i].order != ORDER_BUILD)
            if (assign_worker_to_mineral(i)) {
                fprintf(stderr,"T9 extra mineral worker assigned slot=%d tick=%u\n",
                        i,(unsigned)host_tick);
                return 1;
            }
    return 0;
}

static int lift_command_center_to(int cc, int gx, int gy, int *land_x, int *land_y)
{
    if (cc < 0 || cc >= MAX_BUILDINGS || !b[cc].alive || !select_building(cc)) return 0;
    tap(KEY_B);
    for (int row = 0; row < MENU_LIFT_LAND; ++row) tap(KEY_DOWN);
    tap(KEY_A);
    if (!race_late_airborne(cc)) return 0;
    int found = 0, best = 0x7fffffff;
    for (int radius = 64; radius <= 240; radius += 16)
        for (int dy = -radius; dy <= radius; dy += 16)
            for (int dx = -radius; dx <= radius; dx += 16) {
                if (FAST_ABS(dx) != radius && FAST_ABS(dy) != radius) continue;
                int x = clamp_coord(gx + dx,WORLD_W), y = clamp_coord(gy + dy,WORLD_H);
                int score = dist2(x,y,gx,gy);
                if (score < best && race_late_landing_site(cc,x,y)) {
                    *land_x=x; *land_y=y; best=score; found=1;
                }
            }
    if (!found) return 0;
    tap(KEY_B); tap(KEY_A); /* ordinary Move command for the airborne CC */
    move_cursor(*land_x,*land_y); tap(KEY_A);
    for (int i = 0; i < 12000 && b[cc].alive &&
         dist2(b[cc].x,b[cc].y,*land_x,*land_y) > 4*4; ++i) step(0);
    if (!b[cc].alive || !race_late_airborne(cc) ||
        dist2(b[cc].x,b[cc].y,*land_x,*land_y) > 4*4) return 0;
    tap(KEY_B);
    for (int row = 0; row < MENU_LIFT_LAND; ++row) tap(KEY_DOWN);
    tap(KEY_A); move_cursor(*land_x,*land_y); tap(KEY_A);
    for (int i = 0; i < 2000 && race_late_airborne(cc); ++i) step(0);
    return b[cc].alive && !race_late_airborne(cc) &&
        dist2(b[cc].x,b[cc].y,*land_x,*land_y) <= 4*4;
}

static int queue_wraith(void)
{
    for (int starport = 0; starport < MAX_BUILDINGS; ++starport)
        if (b[starport].alive && b[starport].owner == SIDE &&
            b[starport].raw_type == 114 && building_complete(starport) &&
            b[starport].queue_count < 5 && select_building(starport)) {
            tap(KEY_R);
            if (train_type != U_WRAITH) continue;
            int before = b[starport].queue_count;
            tap(KEY_A);
            if (b[starport].queue_count > before) {
                fprintf(stderr,"T9 Wraith queued tick=%u port=%d queue=%d wallet=%d/%d\n",
                        (unsigned)host_tick,starport,b[starport].queue_count,minerals,gas);
                return 1;
            }
        }
    return 0;
}

static int ferry_scv_to_gas_site(void)
{
    int carrier = source_unit(11, SIDE), worker = -1, geyser = -1, best = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 7 &&
            i != home_miner_slot &&
            u[i].order != ORDER_BUILD && u[i].order != ORDER_GAS &&
            !SC64_TRANSPORT_LOADED(&u[i]) && i > worker) worker = i;
    if (carrier < 0 || worker < 0) {
        fprintf(stderr,"T9 gas ferry missing actor carrier=%d worker=%d units=%d tick=%u\n",
                carrier,worker,owner_units(SIDE),(unsigned)host_tick); return 0;
    }
    gas_worker_slot = worker;
    for (int i = 0; i < geyser_count; ++i) {
        int d = dist2(u[carrier].x,u[carrier].y,geysers[i].x,geysers[i].y);
        if (d < best) { best = d; geyser = i; }
    }
    if (geyser < 0) return 0;
    if (!click_select_unit(worker)) {
        fprintf(stderr,"T9 gas ferry cannot select worker=%d xy=%d,%d order=%d tick=%u\n",
                worker,u[worker].x,u[worker].y,u[worker].order,(unsigned)host_tick); return 0;
    }
    tap(KEY_B); tap(KEY_A); move_cursor(u[carrier].x,u[carrier].y); tap(KEY_A);
    for (int i = 0; i < 1200 && u[worker].alive &&
         dist2(u[worker].x,u[worker].y,u[carrier].x,u[carrier].y) > 12*12; ++i) step(0);
    if (!u[worker].alive || !u[worker].selected) {
        fprintf(stderr,"T9 gas ferry worker did not approach carrier=%d worker=%d xy=%d,%d d2=%d alive=%d selected=%d tick=%u\n",
                carrier,worker,u[worker].x,u[worker].y,
                dist2(u[worker].x,u[worker].y,u[carrier].x,u[carrier].y),u[worker].alive,u[worker].selected,(unsigned)host_tick); return 0;
    }
    move_cursor(u[carrier].x,u[carrier].y); tap(KEY_A);
    for (int i = 0; i < 800 && !SC64_TRANSPORT_LOADED(&u[worker]); ++i) step(0);
    if (!SC64_TRANSPORT_LOADED(&u[worker]) || u[worker].target != carrier ||
        !click_select_unit(carrier)) {
        fprintf(stderr,"T9 gas ferry board failed worker=%d order=%d target=%d carrier=%d tick=%u\n",
                worker,u[worker].order,u[worker].target,carrier,(unsigned)host_tick); return 0;
    }
    int gx = geysers[geyser].x, gy = geysers[geyser].y;
    tap(KEY_B); tap(KEY_A);
    move_cursor(gx,gy); tap(KEY_A);
    for (int i = 0; i < 8000 && u[carrier].alive &&
         dist2(u[carrier].x,u[carrier].y,gx,gy) > 24*24; ++i) step(0);
    if (!u[carrier].alive || dist2(u[carrier].x,u[carrier].y,gx,gy) > 24*24 ||
        !click_select_unit(carrier)) return 0;
    tap(KEY_B);
    for (int row = 0; row < MENU_UNLOAD; ++row) tap(KEY_DOWN);
    tap(KEY_A); move_cursor(clamp_coord(gx + 32,WORLD_W),gy); tap(KEY_A);
    for (int i = 0; i < 1800 && SC64_TRANSPORT_LOADED(&u[worker]); ++i) step(0);
    if (SC64_TRANSPORT_LOADED(&u[worker]) || !u[worker].alive) return 0;
    fprintf(stderr,"T9 gas ferry worker=%d at=%d,%d geyser=%d(%d,%d) tick=%u\n",
            worker,u[worker].x,u[worker].y,geyser,gx,gy,(unsigned)host_tick);
    fprintf(stderr,"T9 gas ferry post-unload state order=%d loaded=%d target=%d carrier=%d\n",
            u[worker].order,SC64_TRANSPORT_LOADED(&u[worker]),u[worker].target,carrier);
    int built = build_from_worker(worker,B_REFINERY,MENU_REFINERY,110,gx,gy);
    fprintf(stderr,"T9 gas Refinery build=%d wallet=%d/%d workerOrder=%d target=%d\n",
            built,minerals,gas,u[worker].order,u[worker].target);
    fprintf(stderr,"T9 gas post-build loaded=%d carrier=%d xy=%d,%d\n",
            SC64_TRANSPORT_LOADED(&u[worker]),carrier,u[carrier].x,u[carrier].y);
    if (!built) return 0;
    int refinery = source_building(110,SIDE);
    for (int i = 0; i < 1400 && refinery >= 0 && !building_complete(refinery); ++i) step(0);
    fprintf(stderr,"T9 gas post-wait order=%d loaded=%d target=%d complete=%d progress=%d tick=%u\n",
            u[worker].order,SC64_TRANSPORT_LOADED(&u[worker]),u[worker].target,
            refinery>=0?building_complete(refinery):0,refinery>=0?b[refinery].build_progress:-1,
            (unsigned)host_tick);
    if (refinery < 0 || !building_complete(refinery)) {
        fprintf(stderr,"T9 gas refinery not complete slot=%d progress=%d workerOrder=%d workerXY=%d,%d tick=%u\n",
                refinery,refinery>=0?b[refinery].build_progress:-1,u[worker].order,u[worker].x,u[worker].y,(unsigned)host_tick);
        return 0;
    }
    if (u[worker].order == ORDER_GAS && u[worker].target == refinery) {
        fprintf(stderr,"T9 refinery builder entered gas order automatically worker=%d tick=%u\n",
                worker,(unsigned)host_tick);
        return 1;
    }
    /* The ship has just unloaded over the worker and still has its selection
     * footprint there. Move it away with normal controls before attempting to
     * select the SCV; otherwise A on the SCV's tile can board it again. */
    if (!u[carrier].selected && !select_unit(carrier)) return 0;
    tap(KEY_B); tap(KEY_A); move_cursor(clamp_coord(gx + 112,WORLD_W),
                                         clamp_coord(gy + 96,WORLD_H)); tap(KEY_A);
    for (int i = 0; i < 1000 && dist2(u[carrier].x,u[carrier].y,u[worker].x,u[worker].y) < 48*48; ++i) step(0);
    if (dist2(u[carrier].x,u[carrier].y,u[worker].x,u[worker].y) < 48*48) return 0;
    if (!select_unit(worker)) return 0;
    tap(KEY_B); tap(KEY_A); move_cursor(clamp_coord(b[refinery].x + 16,WORLD_W),b[refinery].y); tap(KEY_A);
    for (int i = 0; i < 300 && dist2(u[worker].x,u[worker].y,b[refinery].x,b[refinery].y) > 20*20; ++i) step(0);
    if (!select_unit(worker)) {
        fprintf(stderr,"T9 gas worker selection failed slot=%d xy=%d,%d order=%d refinery=%d at=%d,%d tick=%u\n",
                worker,u[worker].x,u[worker].y,u[worker].order,refinery,b[refinery].x,b[refinery].y,(unsigned)host_tick);
        /* The SCV can stand beneath the new Refinery's selection footprint;
         * keep the genuine construction outcome and let the caller relocate
         * its Command Center before retrying assignment. */
        return 1;
    }
    move_cursor(b[refinery].x,b[refinery].y); tap(KEY_A);
    fprintf(stderr,"T9 gas assignment worker=%d order=%d target=%d refinery=%d complete=%d\n",
            worker,u[worker].order,u[worker].target,refinery,building_complete(refinery));
    return u[worker].order == ORDER_GAS && u[worker].target == refinery;
}

static int queue_scv(void)
{
    int cc = source_building(106, SIDE);
    if (cc < 0 || b[cc].queue_count >= 5 || !select_building(cc)) return 0;
    int before = b[cc].queue_count;
    tap(KEY_A);
    return b[cc].queue_count > before;
}

static int select_visible_marine_group(void)
{
    if (!normal_field()) return 0;
    int seed = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_MARINE &&
            !SC64_TRANSPORT_LOADED(&u[i])) {
            seed = i; break;
        }
    if (seed < 0 || !click_select_unit(seed)) return 0;
    tap(KEY_A); /* ordinary same-type visible double-tap selection */
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected) {
            if (u[i].owner != SIDE || u[i].type != U_MARINE ||
                SC64_TRANSPORT_LOADED(&u[i])) return 0;
            ++count;
        }
    return count > 0 && count == selected_count() && selected_building < 0;
}

static int select_landed_marine_group(const int *slots, int count)
{
    for (int n = 0; n < count; ++n) {
        int slot = slots[n];
        if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
            u[slot].owner != SIDE || u[slot].type != U_MARINE ||
            SC64_TRANSPORT_LOADED(&u[slot])) continue;
        int clicked = click_select_unit(slot);
        fprintf(stderr,"T9 landed slot=%d at=%d,%d alive=%d click=%d selected=%d count=%d cursor=%d,%d\n",
                slot,u[slot].x,u[slot].y,u[slot].alive,clicked,u[slot].selected,
                selected_count(),cx,cy);
        if (!clicked) continue;
        tap(KEY_A); /* same-type double-tap selects visible landing group */
        int selected = 0;
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
            if (u[i].owner != SIDE || u[i].type != U_MARINE ||
                SC64_TRANSPORT_LOADED(&u[i])) return 0;
            ++selected;
        }
        if (selected > 0 && selected == selected_count() && selected_building < 0)
            return 1;
    }
    return 0;
}

static int loaded_for(int carrier)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && SC64_TRANSPORT_LOADED(&u[i]) && u[i].target == carrier)
            ++count;
    return count;
}

static int ferry_marine_assault(void)
{
    int carrier = source_unit(11, SIDE);
    if (carrier < 0 || !select_visible_marine_group()) {
        fprintf(stderr, "T9 ferry: no carrier or visible marine group carrier=%d\n", carrier);
        return 0;
    }
    int selected_before = selected_count();
    move_cursor(u[carrier].x, u[carrier].y);
    tap(KEY_A); /* board selected Marines at the friendly Dropship */
    wait_ticks(1200);
    int cargo = loaded_for(carrier), cargo_slots[MAX_UNITS], cargo_count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && SC64_TRANSPORT_LOADED(&u[i]) && u[i].target == carrier &&
            cargo_count < MAX_UNITS) cargo_slots[cargo_count++] = i;
    fprintf(stderr, "T9 ferry boarding selected=%d cargo=%d carrier=%d xy=%d,%d tick=%u\n",
            selected_before, cargo, carrier, u[carrier].x, u[carrier].y,
            (unsigned)host_tick);
    if (cargo < 2 || !u[carrier].alive || !click_select_unit(carrier)) return 0;

    int target = first_force_building(1);
    if (target < 0) return 0;
    int tx = b[target].x, ty = b[target].y;
    /* Carry the ground group across the island gap to an open point near the
     * observed Protoss building. The cursor only supplies an ordinary move
     * destination; the Dropship and its cargo move through game_update. */
    tap(KEY_B); tap(KEY_A); /* Move */
    move_cursor(clamp_coord(tx - 40, WORLD_W), clamp_coord(ty - 32, WORLD_H));
    tap(KEY_A);
    for (int i = 0; i < 12000 && u[carrier].alive &&
         dist2(u[carrier].x, u[carrier].y, tx - 40, ty - 32) > 24 * 24; ++i)
        step(0);
    fprintf(stderr, "T9 ferry arrived carrier=%d xy=%d,%d target=%d,%d cargo=%d tick=%u\n",
            carrier, u[carrier].x, u[carrier].y, tx, ty, loaded_for(carrier),
            (unsigned)host_tick);
    if (!u[carrier].alive || loaded_for(carrier) <= 0 || !click_select_unit(carrier)) return 0;
    tap(KEY_B);
    for (int row = 0; row < MENU_UNLOAD; ++row) tap(KEY_DOWN);
    tap(KEY_A); /* select the controller's Unload command */
    move_cursor(clamp_coord(tx - 40, WORLD_W), clamp_coord(ty - 32, WORLD_H));
    tap(KEY_A);
    for (int i = 0; i < 1800 && loaded_for(carrier) > 0; ++i) step(0);
    fprintf(stderr, "T9 ferry unloaded cargo=%d carrier=%d xy=%d,%d tick=%u\n",
            loaded_for(carrier), carrier, u[carrier].x, u[carrier].y,
            (unsigned)host_tick);
    if (loaded_for(carrier) > 0) return 0;
    int ordered = 0;
    for (int n = 0; n < cargo_count; ++n) {
        int slot = cargo_slots[n];
        if (!u[slot].alive || u[slot].owner != SIDE ||
            SC64_TRANSPORT_LOADED(&u[slot]) || !click_select_unit(slot)) continue;
        tap(KEY_B); tap(KEY_DOWN); tap(KEY_A); /* Attack move */
        move_cursor(tx, ty); tap(KEY_A);
        if (u[slot].order == ORDER_ATTACK_MOVE) ++ordered;
    }
    fprintf(stderr, "T9 ferry attack-move ordered=%d/%d target=%d,%d\n",
            ordered, cargo_count, tx, ty);
    wait_ticks(1500);
    for (int n = 0; n < cargo_count; ++n) {
        int slot = cargo_slots[n];
        fprintf(stderr,"T9 ferry followup slot=%d alive=%d xy=%d,%d hp=%d order=%d target=%d tx=%d ty=%d\n",
                slot,u[slot].alive,u[slot].x,u[slot].y,u[slot].hp,u[slot].order,
                u[slot].target,u[slot].tx,u[slot].ty);
    }
    return ordered > 0;
}

static void inspect_state(void)
{
    int cc = source_building(106,SIDE);
    fprintf(stderr, "T9 read-only snapshot tick=%u frame=%u SIDE=%d Protoss buildings=%d Zerg buildings=%d wallet=%d/%d\n",
            (unsigned)host_tick, (unsigned)frame, owner_units(SIDE),
            living_force_buildings(1), living_force_buildings(3), minerals, gas);
    int worker = source_unit(7,SIDE);
    fprintf(stderr," T9 PUNI starport=%d refinery=%d wraith=%d worker=%d geysers=%d\n",
            worker>=0?race_late_building_product_available(SIDE,u[worker].raw_owner,B_STARPORT):0,
            worker>=0?race_late_building_product_available(SIDE,u[worker].raw_owner,B_REFINERY):0,
            worker>=0?race_late_unit_product_available(SIDE,u[worker].raw_owner,U_WRAITH):0,
            worker,geyser_count);
    for (int i=0;i<geyser_count;++i)
        fprintf(stderr," geyser %d xy=%d,%d ground_reachable=%d\n",i,geysers[i].x,geysers[i].y,
                cc>=0?ground_reachable(b[cc].x,b[cc].y,geysers[i].x,geysers[i].y):0);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive)
        fprintf(stderr, " unit slot=%d raw=%d owner=%d source=%d type=%d xy=%d,%d hp=%d order=%d target=%d\n",
                i, u[i].raw_type, u[i].owner, u[i].raw_owner, u[i].type,
                u[i].x, u[i].y, u[i].hp, u[i].order, u[i].target);
    for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0)
        fprintf(stderr, " mineral slot=%d xy=%d,%d remaining=%d\n", i, m[i].x, m[i].y, m[i].amount);
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE)
        fprintf(stderr, " building slot=%d raw=%d source=%d type=%d xy=%d,%d hp=%d complete=%d queue=%d\n",
                i, b[i].raw_type, b[i].raw_owner, b[i].type, b[i].x, b[i].y,
                b[i].hp, building_complete(i), b[i].queue_count);
}

static int run_policy(void)
{
    /* Build a paid Wraith wave before committing it. Individual early Wraiths
     * were being lost before reaching the enemy base. */
    u32 next_order = host_tick + 15000;
    u32 next_wraith_attempt = host_tick + 700;
    u32 next_starport_attempt = host_tick + 900;
    int queued_total = 0, wave_sent = 0;
    while (host_tick < (u32)limit_ticks && screen == SCREEN_PLAY && !won && !lost) {
        int live_wraiths = side_wraith_count();
        int queued = 0;
        for (int i=0;i<MAX_BUILDINGS;++i)
            if (b[i].alive && b[i].owner==SIDE && b[i].raw_type==114)
                queued += b[i].queue_count;
        if (side_starport_count() < 3 && minerals >= 150 && gas >= 100 &&
            host_tick >= next_starport_attempt) {
            if (build_starport()) checkpoint("additional-starport-order");
            next_starport_attempt = host_tick + 900;
        }
        if (queued_total < 60 && live_wraiths + queued < 10 &&
            host_tick >= next_wraith_attempt) {
            if (reserved_supply() + 8 > supply_cap) (void)build_supply_depot();
            if (queue_wraith()) ++queued_total;
            next_wraith_attempt = host_tick + 700;
        }
        if (!wave_sent && live_wraiths >= 6) {
            int target = nearest_force_building(1);
            if (target >= 0) {
                int ok = send_wraith_wave_to(target);
                failed_action = !ok;
                if (ok) { wave_sent = 1; checkpoint("protoss-wave-order"); }
            }
        }
        if (wave_sent && host_tick >= next_order) {
            next_order = host_tick + 15000;
            int target = nearest_force_building(1);
            if (target >= 0 && live_wraiths > 0) {
                int ok = live_wraiths >= 6 ? send_wraith_wave_to(target) : 0;
                failed_action = !ok;
                checkpoint(ok ? "protoss-retarget" : "retarget-unavailable");
            }
        }
        step(0);
    }
    return won && !lost && mission_unsupported_count == 0 &&
        race_campaign.phase == SC64_RACE_PHASE_COMPLETE &&
        (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) &&
        (race_completed & (1 << SC64_TERRAN_LATE_GLOBAL_T9)) &&
        living_force_buildings(1) == 0 && living_force_buildings(3) > 0 &&
        source_unit(16, -1) >= 0;
}

int main(int argc, char **argv)
{
    const char *trace_path = 0;
    int inspect = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--limit") && i + 1 < argc) limit_ticks = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc) trace_path = argv[++i];
        else if (!strcmp(argv[i], "--inspect")) inspect = 1;
        else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_t9_campaign_run [--limit TICKS] [--trace PATH] [--inspect]");
            return 0;
        } else return 2;
    }
    if (limit_ticks < 1 || limit_ticks > 500000) return 2;
    game_init();
    assert(screen == SCREEN_PLAY && !race_mode &&
           race_selected_mission == SC64_TERRAN_LATE_GLOBAL_T9 && race_late_active());
    assert(!won && !lost && race_cols == 128 && race_rows == 96 && !mission_unsupported_count);
    if (trace_path) {
        trace = fopen(trace_path, "wb");
        if (!trace) return 2;
        fprintf(trace,
                "{\"trace_schema\":1,\"entry\":\"t9-probe-entry\","
                "\"profile\":\"terran-t9\",\"policy\":\"bounded-normal-controller-attempt\","
                "\"mission\":12,\"tick_limit\":%d,\"events\":[\n", limit_ticks);
    }
    checkpoint("start");
    int miners = 0;
    for (int i = 0; i < MAX_UNITS && miners < 1; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 7 &&
            assign_worker_to_mineral(i)) ++miners;
    fprintf(stderr, "T9 immediate mineral orders=%d tick=%u wallet=%d/%d\n",
            miners, (unsigned)host_tick, minerals, gas);
    int scv_queued = queue_scv();
    fprintf(stderr,"T9 opening paid SCV queue=%d at tick=%u wallet=%d/%d\n",
            scv_queued,(unsigned)host_tick,minerals,gas);
    if (inspect) {
        inspect_state();
        wait_ticks(6000);
        inspect_state();
        checkpoint("inspect");
        if (trace) { fputs("\n]}\n", trace); fclose(trace); }
        return 0;
    }
    for (int i = 0; i < 2400 && side_scv_count() < 2; ++i) step(0);
    fprintf(stderr,"T9 second worker ready count=%d tick=%u wallet=%d/%d\n",
            side_scv_count(),(unsigned)host_tick,minerals,gas);
    if (side_scv_count() == 2 && minerals >= 50 && queue_scv()) {
        for (int i = 0; i < 1800 && side_scv_count() < 3; ++i) step(0);
        fprintf(stderr,"T9 third worker ready count=%d tick=%u wallet=%d/%d\n",
                side_scv_count(),(unsigned)host_tick,minerals,gas);
    }
    int gas_setup = ferry_scv_to_gas_site();
    fprintf(stderr,"T9 gas setup=%d tick=%u wallet=%d/%d\n",
            gas_setup,(unsigned)host_tick,minerals,gas);
    if (gas_setup) {
        /* Keep the original Command Center at the mineral base. A paid second
         * center gives the isolated refinery a depot without abandoning the
         * opening mineral line. */
        for (int i = 0; i < 5000 && minerals < 400 && !lost && !won; ++i) step(0);
        int outpost = build_gas_outpost();
        fprintf(stderr,"T9 gas outpost=%d tick=%u wallet=%d/%d\n",
                outpost,(unsigned)host_tick,minerals,gas);
        if (outpost) {
            int extra_gas = add_gas_worker();
            fprintf(stderr,"T9 additional paid gas worker=%d tick=%u wallet=%d/%d\n",
                    extra_gas,(unsigned)host_tick,minerals,gas);
        }
    }
    fprintf(stderr,"T9 gas income checkpoint tick=%u wallet=%d/%d refinery=%d\n",
            (unsigned)host_tick,minerals,gas,source_building(110,SIDE));
    { int w = source_unit(7,SIDE), r = source_building(110,SIDE), c = source_building(106,SIDE);
      if (w >= 0) fprintf(stderr,"T9 gas worker state slot=%d order=%d target=%d carry=%d timer=%d xy=%d,%d home=%d,%d refinery=%d,%d\n",
          w,u[w].order,u[w].target,u[w].carry,u[w].work_timer,u[w].x,u[w].y,
          c>=0?b[c].x:-1,c>=0?b[c].y:-1,r>=0?b[r].x:-1,r>=0?b[r].y:-1); }
    if (gas_worker_slot >= 0) fprintf(stderr,"T9 ferried SCV slot=%d alive=%d owner=%d raw=%d order=%d target=%d carry=%d timer=%d xy=%d,%d hp=%d\n",
        gas_worker_slot,u[gas_worker_slot].alive,u[gas_worker_slot].owner,u[gas_worker_slot].raw_type,
        u[gas_worker_slot].order,u[gas_worker_slot].target,u[gas_worker_slot].carry,
        u[gas_worker_slot].work_timer,u[gas_worker_slot].x,u[gas_worker_slot].y,u[gas_worker_slot].hp);
    int starport_built = 0;
    for (int tries = 0; tries < 3 && !starport_built && host_tick < (u32)limit_ticks; ++tries) {
        starport_built = build_starport();
        if (!starport_built) wait_ticks(800);
    }
    fprintf(stderr,"T9 Starport started=%d tick=%u wallet=%d/%d\n",
            starport_built,(unsigned)host_tick,minerals,gas);
    int starport = source_building(114,SIDE);
    for (int i = 0; starport_built && i < 1800 && starport >= 0 &&
         !building_complete(starport); ++i) step(0);
    int first_wraith = starport >= 0 && building_complete(starport) ? queue_wraith() : 0;
    fprintf(stderr,"T9 initial Wraith queued=%d tick=%u wallet=%d/%d\n",
            first_wraith,(unsigned)host_tick,minerals,gas);
    wait_ticks(600);
    checkpoint("opening");
    int result = run_policy();
    checkpoint(won ? "won" : lost ? "lost" : "limit");
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 7)
        fprintf(stderr,"T9 ending SCV slot=%d xy=%d,%d hp=%d order=%d target=%d\n",
                i,u[i].x,u[i].y,u[i].hp,u[i].order,u[i].target);
    if (trace) { fputs("\n]}\n", trace); fclose(trace); }
    fprintf(stderr,
        "T9 attempt tick=%u frame=%u outcome=%s ProtossBuildings=%d ZergBuildings=%d "
        "raw16=%d sideUnits=%d faults=%d wallet=%d/%d\n",
        (unsigned)host_tick, (unsigned)frame, won ? "won" : lost ? "lost" : "limit",
        living_force_buildings(1), living_force_buildings(3),
        source_unit(16, -1), owner_units(SIDE), mission_unsupported_count, minerals, gas);
    fprintf(stderr,"T9 combat summary live-wraiths=%d supply=%d/%d reserved=%d\n",
            side_wraith_count(),supply_used,supply_cap,reserved_supply());
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].owner==SIDE && u[i].type==U_WRAITH)
        fprintf(stderr," T9 Wraith slot=%d hp=%d xy=%d,%d order=%d target=%d destination=%d,%d\n",
                i,u[i].hp,u[i].x,u[i].y,u[i].order,u[i].target,u[i].tx,u[i].ty);
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive && b[i].raw_owner==1)
        fprintf(stderr," T9 Protoss building slot=%d raw=%d hp=%d xy=%d,%d\n",
                i,b[i].raw_type,b[i].hp,b[i].x,b[i].y);
    if (lost)
        fprintf(stderr,"T9 lost phase=%u flags=%u raw16-alive=%d\n",
                (unsigned)race_campaign.phase,
                (unsigned)race_campaign.objective_flags,source_unit(16,-1)>=0);
    return result ? 0 : 1;
}
