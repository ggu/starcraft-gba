/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Input-driven P6 campaign attempt. The controller observes the live world
 * for steering; every gameplay change is issued through game_update input. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define P6_RUN_LIMIT 180000
#define P6_CHECKPOINT_PERIOD 900

static FILE *trace;
static u16 prior_keys;
static u32 host_ticks;
static int deployed;
static int start_checkpoint_written;
static int run_limit = P6_RUN_LIMIT;
static int trace_first = 1;
static int last_keys = -1;
static int next_order_frame;
static int next_storm_frame;
static int tassadar_support_positioned;
static unsigned tassadar_storm_casts;
static unsigned frontline_storm_casts;
static unsigned waiting_self_storm_frame;
static int waiting_self_retreat_x, waiting_self_retreat_y;
static int waiting_self_retreat_valid;
static int path_diagnostics;
static int team_escort_diagnostics;
static int team_escort_commit_frame;
static int storm_self_splash_rejected;
static int frontline_pursuit;
static unsigned frontline_self_storm_frame;
static int frontline_lane_x = 680;
static const int frontline_lane_y = 1943;
static int frontline_last_x[MAX_UNITS];
static int frontline_last_y[MAX_UNITS];
static int frontline_last_frame[MAX_UNITS];
static int frontline_vx_q8[MAX_UNITS];
static int frontline_vy_q8[MAX_UNITS];
static u8 frontline_history_valid[MAX_UNITS];
static int frontline_aim_x, frontline_aim_y;
static const char *prefix_inputs_path;
static char attempt_path[512] = ".local/sc64/p6-controller-attempt-running.json";
static char final_path[512] = ".local/sc64/p6-controller-attempt.json";
static char verified_path[512] = ".local/sc64/p6-controller-win.json";
static int storm_trap_raw(int raw);

static void step(u16 held);
static int stop_actor(int slot);
static int select_actor_only(int slot);
#define tap(key) do { \
    u16 tap_key_ = (key); \
    if (deployed && path_diagnostics && mode == MODE_P4_STORM && \
        (tap_key_ & KEY_A)) { \
        int tap_hero_ = raw_unit(SC64_P6_RAW_HERO, SIDE); \
        fprintf(stderr, "P6 Storm-mode A callsite line=%d frame=%u cursor=%d,%d hero=%d at=%d,%d energy=%d\n", \
            __LINE__, (unsigned)frame, cx, cy, tap_hero_, \
            tap_hero_ >= 0 ? u[tap_hero_].x : -1, \
            tap_hero_ >= 0 ? u[tap_hero_].y : -1, \
            tap_hero_ >= 0 ? u[tap_hero_].carry : -1); \
    } \
    step(tap_key_); step(0); \
} while (0)

static int isqrt_u32(unsigned value)
{
    unsigned root=0,bit=1u<<30;
    while(bit>value)bit>>=2;
    while(bit){
        if(value>=root+bit){value-=root+bit;root=(root>>1)+bit;}
        else root>>=1;
        bit>>=2;
    }
    return (int)root;
}

static int raw_unit(unsigned raw, int owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == (int)raw &&
            (owner < 0 || u[i].owner == owner)) return i;
    return -1;
}

static int raw_count(unsigned raw, int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].raw_type == (int)raw &&
                 (owner < 0 || u[i].owner == owner);
    return count;
}

static int nearby_hostile_units(int slot, int radius)
{
    if(slot<0||slot>=MAX_UNITS||!u[slot].alive)return 1;
    for(int i=0;i<MAX_UNITS;++i)
        if(i!=slot&&u[i].alive&&race_p6_owners_hostile(SIDE,u[i].owner)&&
           dist2(u[slot].x,u[slot].y,u[i].x,u[i].y)<=radius*radius)return 1;
    return 0;
}

static int nearby_hostile_sources(int slot, int radius)
{
    if (nearby_hostile_units(slot, radius)) return 1;
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive) return 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && storm_trap_raw(b[i].raw_type) &&
            race_p6_owners_hostile(SIDE, b[i].owner) &&
            dist2(u[slot].x, u[slot].y, b[i].x, b[i].y) <= radius * radius)
            return 1;
    return 0;
}

static int nearby_unburrowed_hostile_units(int slot, int radius)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (i != slot && u[i].alive &&
            race_p6_owners_hostile(SIDE, u[i].owner) &&
            !race_p6_combat_unit_burrowed(i) &&
            dist2(u[slot].x, u[slot].y, u[i].x, u[i].y) <= radius * radius)
            return 1;
    return 0;
}

/* Sparse traces hide which nearby attackers are still closing on Tassadar.
 * During the southern frontier trial, record the local live threat set often
 * enough to distinguish a missed Storm from an advancing ranged attacker. */
static void log_south_combat_snapshot(unsigned frame, int slot)
{
    if (!path_diagnostics || slot < 0 || slot >= MAX_UNITS ||
        !u[slot].alive || u[slot].y < 1700 || frame % 10 != 0) return;

    int n = 0;
    fprintf(stderr,
        "P6 south snapshot frame=%u host=%u mode=%d menu=%d hero=%d raw=%d at=%d,%d hp=%d shield=%d energy=%d order=%d target=%d cd=%d\n",
        frame, host_ticks, mode, menu_open, slot, u[slot].raw_type,
        u[slot].x, u[slot].y, u[slot].hp, u[slot].shield, u[slot].carry,
        u[slot].order, u[slot].target, u[slot].cd);
    for(unsigned i=0;i<SC64_STORM_MAX_ACTIVE;++i){
        const Sc64PsionicStorm *e=&race_p6.storm.storm[i];
        if(e->active)fprintf(stderr,
            "P6 south Storm frame=%u index=%u center=%u,%u age=%u pulses=%u coversHero=%d\n",
            frame,i,e->x,e->y,e->age_ticks,e->pulses_left,
            sc64_psionic_storm_contains(e,(u16)u[slot].x,(u16)u[slot].y));
    }
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (i == slot || !u[i].alive ||
            !race_p6_owners_hostile(SIDE, u[i].owner) ||
            dist2(u[slot].x, u[slot].y, u[i].x, u[i].y) > 320 * 320)
            continue;
        if (n++ >= 12) continue;
        fprintf(stderr,
            "P6 south threat frame=%u slot=%d raw=%d owner=%d at=%d,%d d2=%d hp=%d shield=%d burrow=%d order=%d target=%d cd=%d\n",
            frame, i, u[i].raw_type, u[i].owner, u[i].x, u[i].y,
            dist2(u[slot].x, u[slot].y, u[i].x, u[i].y), u[i].hp,
            u[i].shield, race_p6_combat_unit_burrowed(i), u[i].order,
            u[i].target, u[i].cd);
    }
    for (int i = 0; i < MAX_BUILDINGS && n < 16; ++i) {
        if (!b[i].alive || !storm_trap_raw(b[i].raw_type) ||
            !race_p6_owners_hostile(SIDE, b[i].owner) ||
            dist2(u[slot].x, u[slot].y, b[i].x, b[i].y) > 320 * 320)
            continue;
        ++n;
        fprintf(stderr,
            "P6 south prop frame=%u slot=%d raw=%d owner=%d at=%d,%d d2=%d hp=%d\n",
            frame, i, b[i].raw_type, b[i].owner, b[i].x, b[i].y,
            dist2(u[slot].x, u[slot].y, b[i].x, b[i].y), b[i].hp);
    }
}

static int objective_location(int *left, int *top, int *right, int *bottom)
{
    return sc64_p6_location_runtime(15, MAP_COLS, MAP_ROWS,
                                    left, top, right, bottom);
}

static int location_waypoint(unsigned id, int sx, int sy, int *out_x, int *out_y)
{
    int left, top, right, bottom, best = 0x7fffffff, found = 0;
    if (!sc64_p6_location_runtime(id, MAP_COLS, MAP_ROWS,
                                  &left, &top, &right, &bottom)) return 0;
    for (int row = top / 16; row <= (bottom - 1) / 16; ++row)
        for (int col = left / 16; col <= (right - 1) / 16; ++col) {
            int x = col * 16 + 8, y = row * 16 + 8, nx, ny;
            int cell = col + row * MAP_COLS;
            if (cell < 0 || cell >= MAP_CELLS ||
                !SC64_GRID_GET(path_walkable, cell) ||
                terrain_blocked(col, row) ||
                !sc64_route_next(sx, sy, x, y, &nx, &ny)) continue;
            int d = dist2(x, y, (left + right) / 2, (top + bottom) / 2);
            if (d < best) { best = d; *out_x = x; *out_y = y; found = 1; }
        }
    return found;
}

static int route_point_ahead(int sx,int sy,int goal_x,int goal_y,
                             unsigned steps,int *out_x,int *out_y)
{
    int x=sx,y=sy,nx,ny;
    if(!steps)steps=1;
    for(unsigned i=0;i<steps;++i){
        if(!sc64_route_next(x,y,goal_x,goal_y,&nx,&ny))return 0;
        x=nx;y=ny;
        if(x==goal_x&&y==goal_y)break;
    }
    *out_x=x;*out_y=y;
    return 1;
}

static void diagnose_approach_neighborhood(int slot, int target_x, int target_y,
                                          int next_x, int next_y)
{
    int x=u[slot].x, y=u[slot].y, col=x/16, row=y/16;
    fprintf(stderr,"P6 route actor=%d raw=%d owner=%d xy=%d,%d order=%d target=%d,%d requested=%d,%d next=%d,%d\n",
            slot,u[slot].raw_type,u[slot].owner,x,y,u[slot].order,
            u[slot].tx,u[slot].ty,target_x,target_y,next_x,next_y);
    for(int dy=-1;dy<=1;++dy){
        fprintf(stderr,"P6 route grid y=%d ",row+dy);
        for(int dx=-1;dx<=1;++dx){
            int c=col+dx,r=row+dy,cell=c+r*MAP_COLS;
            if(c<0||c>=MAP_COLS||r<0||r>=MAP_ROWS)fputc('X',stderr);
            else fputc(!SC64_GRID_GET(path_walkable,cell)?'P':
                       terrain_blocked(c,r)?'T':'.',stderr);
        }
        fputc('\n',stderr);
    }
    for(int i=0;i<MAX_UNITS;++i)if(i!=slot&&u[i].alive&&
        dist2(x,y,u[i].x,u[i].y)<=112*112)
        fprintf(stderr,"P6 route unit=%d raw=%d owner=%d xy=%d,%d hp=%d order=%d\n",
                i,u[i].raw_type,u[i].owner,u[i].x,u[i].y,u[i].hp,u[i].order);
    for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&
        dist2(x,y,b[i].x,b[i].y)<=160*160){
        int l=0,t=0,r=0,d=0;
        int valid=sc64_game_building_bounds_source(b[i].type,&l,&t,&r,&d);
        fprintf(stderr,"P6 route building=%d raw=%d owner=%d xy=%d,%d hp=%d collide=%d bounds=%d:%d,%d,%d,%d\n",
                i,b[i].raw_type,b[i].owner,b[i].x,b[i].y,b[i].hp,
                race_p6_building_collidable(i),valid,l,t,r,d);
    }
    if(x>=1000&&x<=1300&&y>=1100&&y<=1400){
        static const int probes[][2]={{1360,1500},{1600,1500},{1800,1500},
            {1400,1650},{1200,1600},{1000,1500},{800,1500},
            {1000,1800},{1400,1800},{1704,1704},{1400,1200},
            {1600,1200},{1500,1300},{1400,1400},{1300,1400},
            {1280,1480},{1240,1400},{1100,1440},{1150,1520},
            {1300,1600},{1500,1600},{1700,1600}};
        for(unsigned p=0;p<sizeof(probes)/sizeof(probes[0]);++p){
            int px=-1,py=-1;
            int ok=sc64_route_next(x,y,probes[p][0],probes[p][1],&px,&py);
            fprintf(stderr,"P6 route probe goal=%d,%d ok=%d next=%d,%d\n",
                probes[p][0],probes[p][1],ok,px,py);
        }
    }
    if(x>=1300&&x<=1600&&y>=1100&&y<=1300){
        static const int probes[][2]={{1500,1500},{1700,1500},{1800,1500},
            {1400,1500},{1600,1600},{1300,1500},{1500,1650}};
        for(unsigned p=0;p<sizeof(probes)/sizeof(probes[0]);++p){
            int px=-1,py=-1;
            int ok=sc64_route_next(x,y,probes[p][0],probes[p][1],&px,&py);
            fprintf(stderr,"P6 shelf probe goal=%d,%d ok=%d next=%d,%d\n",
                probes[p][0],probes[p][1],ok,px,py);
        }
    }
}

static void trace_prefix(void)
{
    if (!trace_first) fputs(",\n", trace);
    trace_first = 0;
}

static void checkpoint(const char *kind)
{
    int zeratul = raw_unit(SC64_P6_RAW_TEMPLAR, SIDE);
    int zeratul_any = raw_unit(SC64_P6_RAW_TEMPLAR, -1);
    int tassadar = raw_unit(SC64_P6_RAW_HERO, SIDE);
    int l = 0, t = 0, r = 0, btm = 0;
    (void)objective_location(&l, &t, &r, &btm);
    trace_prefix();
    fprintf(trace,
        "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,"
        "\"screen\":%d,\"won\":%d,\"lost\":%d,\"completed\":%u,\"outcome\":%u,"
        "\"started\":%u,\"fired\":%u,\"action_cursors\":[",
        kind, (unsigned)host_ticks, (unsigned)frame, screen, won, lost,
        (unsigned)((race_completed & (1 << SC64_P6_GLOBAL_ID)) != 0),
        (unsigned)race_p6.runtime.model.outcome,
        (unsigned)race_p6.runtime.model.started,
        (unsigned)race_p6.runtime.model.started);
    for (int i = 0; i < SC64_P6_TRIGGER_COUNT; ++i)
        fprintf(trace, "%s%u", i ? "," : "",
                (unsigned)race_p6.runtime.model.action_cursor[i]);
    fputs("],\"action_ticks\":[", trace);
    for (int i = 0; i < SC64_P6_TRIGGER_COUNT; ++i)
        fprintf(trace, "%s%u", i ? "," : "",
                (unsigned)race_p6.runtime.action_ticks[i]);
    fprintf(trace,
        "],\"switch_bits\":%u,\"fault\":%u,"
        "\"objective\":[%d,%d,%d,%d],"
        "\"zeratul\":[%d,%d,%d,%d,%d,%d,%d,%d],"
        "\"tassadar\":[%d,%d,%d,%d,%d,%d,%d,%d],"
        "\"side\":%d,\"enemy3\":%d,\"enemy6\":%d,"
        "\"cursor\":[%d,%d],\"selected\":%d,\"mode\":%d}",
        (unsigned)race_p6.runtime.switch_bits,
        (unsigned)race_p6.runtime.fault,
        l, t, r, btm,
        zeratul >= 0, zeratul >= 0 ? u[zeratul].raw_type : -1,
        zeratul >= 0 ? u[zeratul].raw_owner : -1,
        zeratul >= 0 ? u[zeratul].x : -1,
        zeratul >= 0 ? u[zeratul].y : -1,
        zeratul >= 0 ? u[zeratul].owner : -1,
        zeratul >= 0 ? u[zeratul].hp : 0,
        zeratul >= 0 ? u[zeratul].order : -1,
        tassadar >= 0, tassadar >= 0 ? u[tassadar].raw_type : -1,
        tassadar >= 0 ? u[tassadar].raw_owner : -1,
        tassadar >= 0 ? u[tassadar].x : -1,
        tassadar >= 0 ? u[tassadar].y : -1,
        tassadar >= 0 ? u[tassadar].owner : -1,
        tassadar >= 0 ? u[tassadar].hp : 0,
        tassadar >= 0 ? u[tassadar].order : -1,
        raw_count(65, SIDE), raw_count(65, ENEMY), raw_count(65, 4),
        cx, cy, selected_count(), mode);
    fflush(trace);
    if (path_diagnostics)
        fprintf(stderr, "P6 checkpoint %s frame=%u Z=%d anyZ=%d owner=%d rawowner=%d T=%d SIDE zealots=%d enemy3 zealots=%d enemy6 zealots=%d started=%08x fault=%u\n",
                kind, (unsigned)frame, zeratul, zeratul_any,
                zeratul_any >= 0 ? u[zeratul_any].owner : -1,
                zeratul_any >= 0 ? u[zeratul_any].raw_owner : -1,
                tassadar, raw_count(65, SIDE),
                raw_count(65, ENEMY), raw_count(65, 4),
                (unsigned)race_p6.runtime.model.started, (unsigned)race_p6.runtime.fault);
}

static void step(u16 held)
{
    int hero_before = deployed ? raw_unit(SC64_P6_RAW_HERO, SIDE) : -1;
    int carry_before = hero_before >= 0 ? u[hero_before].carry : -1;
    int mode_before = mode, menu_before = menu_open;
    int x_before = cx, y_before = cy;
    unsigned frame_before = frame;
    struct StormProbe { int active, x, y, age, left; } storms_before[SC64_STORM_MAX_ACTIVE];
    int probe_units = 0;
    int old_x[MAX_UNITS], old_y[MAX_UNITS], old_hp[MAX_UNITS], old_shield[MAX_UNITS];
    int old_owner[MAX_UNITS], old_raw[MAX_UNITS];
    u8 old_alive[MAX_UNITS];
    if (deployed && path_diagnostics && screen == SCREEN_PLAY) {
        for (unsigned i=0;i<SC64_STORM_MAX_ACTIVE;++i) {
            const Sc64PsionicStorm *s=&race_p6.storm.storm[i];
            storms_before[i]=(struct StormProbe){s->active,s->x,s->y,s->age_ticks,s->pulses_left};
            probe_units |= s->active;
        }
        if (probe_units) for (int i=0;i<MAX_UNITS;++i) {
            old_x[i]=u[i].x; old_y[i]=u[i].y; old_hp[i]=u[i].hp;
            old_shield[i]=u[i].shield; old_owner[i]=u[i].owner;
            old_raw[i]=u[i].raw_type; old_alive[i]=u[i].alive;
        }
    } else probe_units=0;
    if (deployed && (screen != SCREEN_PLAY || won || lost)) return;
    u16 pressed = held & (u16)~prior_keys;
    game_update(held, pressed);
    if (probe_units) for (unsigned f=0;f<SC64_STORM_MAX_ACTIVE;++f) {
        const Sc64PsionicStorm *after=&race_p6.storm.storm[f];
        const struct StormProbe *before=&storms_before[f];
        if (!before->active || after->pulses_left>=before->left) continue;
        fprintf(stderr,"P6 storm pulse frame=%u field=%u center=%d,%d age=%d->%u pulse=%u left=%u->%u\n",
            (unsigned)frame,f,before->x,before->y,before->age,after->age_ticks,
            (unsigned)(SC64_STORM_PULSE_COUNT-before->left+1),before->left,after->pulses_left);
        for (int i=0;i<MAX_UNITS;++i) {
            int d0=dist2(before->x,before->y,old_x[i],old_y[i]);
            int d1=dist2(before->x,before->y,u[i].x,u[i].y);
            if (d0>24*24 && d1>24*24) continue;
            fprintf(stderr,"P6 storm actor frame=%u field=%u slot=%d raw=%d owner=%d at=%d,%d->%d,%d d2=%d->%d hp=%d->%d shield=%d->%d alive=%u->%u\n",
                (unsigned)frame,f,i,old_raw[i],old_owner[i],old_x[i],old_y[i],
                u[i].x,u[i].y,d0,d1,old_hp[i],u[i].hp,old_shield[i],
                u[i].shield,old_alive[i],u[i].alive);
        }
    }
    if (deployed && path_diagnostics && hero_before >= 0 &&
        u[hero_before].alive && carry_before - u[hero_before].carry >= 19000) {
        fprintf(stderr,
            "P6 hero energy debit host=%u frame_before=%u frame_after=%u mode=%d->%d menu=%d->%d held=%u pressed=%u cursor=%d,%d->%d,%d carry=%d->%d storms=",
            (unsigned)(host_ticks + 1), frame_before,
            (unsigned)frame, mode_before, mode, menu_before, menu_open,
            held, pressed, x_before, y_before, cx, cy, carry_before,
            u[hero_before].carry);
        for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
            const Sc64PsionicStorm *s = &race_p6.storm.storm[i];
            if (s->active) fprintf(stderr, "[%u:%u,%u age%u left%u]",
                i, s->x, s->y, s->age_ticks, s->pulses_left);
        }
        fputc('\n', stderr);
    }
    prior_keys = held;
    ++host_ticks;
    if (held != (u16)last_keys) {
        trace_prefix();
        fprintf(trace, "{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}\n",
                (unsigned)host_ticks, held, pressed);
        last_keys = held;
        fflush(trace);
    }
    if (deployed && screen == SCREEN_PLAY && frame > 0 &&
        frame % P6_CHECKPOINT_PERIOD == 0) checkpoint("periodic");
    if (team_escort_diagnostics && team_escort_commit_frame &&
        frame >= team_escort_commit_frame && frame % 600 == 0) {
        int ts=raw_unit(SC64_P6_RAW_HERO,SIDE);
        fprintf(stderr,"P6 team census frame=%u live_marines=%d Tslot=%d T=%d,%d hp=%d shield=%d energy=%d order=%d target=%d,%d path=%d,%d retry=%u\n",
                (unsigned)frame,raw_count(0,SIDE),
                ts,ts>=0?u[ts].x:-1,ts>=0?u[ts].y:-1,
                ts>=0?u[ts].hp:0,ts>=0?u[ts].shield:0,
                ts>=0?u[ts].carry:-1,ts>=0?u[ts].order:-1,
                ts>=0?u[ts].tx:-1,ts>=0?u[ts].ty:-1,
                ts>=0?u[ts].path_x:-1,ts>=0?u[ts].path_y:-1,
                ts>=0?u[ts].path_retry:0);
        for(int i=0;i<MAX_UNITS;++i)
            if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==0)
                fprintf(stderr,"  marine slot=%d at=%d,%d hp=%d order=%d target=%d,%d path=%d,%d\n",
                        i,u[i].x,u[i].y,u[i].hp,u[i].order,u[i].tx,u[i].ty,
                        u[i].path_x,u[i].path_y);
    }
}

/* Optional reproducibility bridge: replay an earlier controller's complete
 * held-key stream through game_update, then let the current policy continue.
 * The private text file contains one unsigned held-key mask per host tick. */
static void replay_prefix_inputs(const char *path)
{
    FILE *input = fopen(path, "r");
    unsigned held;
    assert(input);
    game_init();
    prior_keys = 0;
    while (fscanf(input, "%u", &held) == 1) {
        assert(held <= 0xffffu);
        step((u16)held);
        if (host_ticks == 12 && !start_checkpoint_written) {
            assert(screen == SCREEN_PLAY && !won && !lost && race_p6_active());
            deployed = 1;
            checkpoint("start");
            start_checkpoint_written = 1;
        }
        if (screen == SCREEN_RESULT || won || lost) break;
    }
    assert(!ferror(input));
    fclose(input);
    if (!(screen == SCREEN_PLAY && !won && !lost && race_p6_active()))
        fprintf(stderr, "P6 prefix ended outside gameplay host=%u frame=%u screen=%d won=%d lost=%d active=%d\n",
                (unsigned)host_ticks, (unsigned)frame, screen, won, lost,
                race_p6_active());
    assert(screen == SCREEN_PLAY && !won && !lost && race_p6_active());
    assert(start_checkpoint_written);
    if (path_diagnostics) {
        int hero = raw_unit(SC64_P6_RAW_HERO, SIDE);
        int near = 0x7fffffff, near_slot = -1, close = 0;
        if (hero >= 0) for (int i = 0; i < MAX_UNITS; ++i)
            if (i != hero && u[i].alive &&
                race_p6_owners_hostile(SIDE, u[i].owner)) {
                int d = dist2(u[hero].x, u[hero].y, u[i].x, u[i].y);
                if (d < near) { near = d; near_slot = i; }
                if (d <= 240 * 240) ++close;
            }
        fprintf(stderr,
            "P6 prefix snapshot host=%u frame=%u hero=%d at=%d,%d hp=%d shield=%d energy_q8=%d nearest_slot=%d raw=%d owner=%d at=%d,%d d2=%d hostile_within240=%d cursor=%d,%d mode=%d menu=%d\n",
            host_ticks, (unsigned)frame, hero,
            hero >= 0 ? u[hero].x : -1, hero >= 0 ? u[hero].y : -1,
            hero >= 0 ? u[hero].hp : 0, hero >= 0 ? u[hero].shield : 0,
            hero >= 0 ? u[hero].carry : -1, near_slot,
            near_slot >= 0 ? u[near_slot].raw_type : -1,
            near_slot >= 0 ? u[near_slot].owner : -1,
            near_slot >= 0 ? u[near_slot].x : -1,
            near_slot >= 0 ? u[near_slot].y : -1,
            near, close, cx, cy, mode, menu_open);
    }
    deployed = 1;
}

/* Enter the six-entry probe through its ordinary campaign menu. Route
 * commands below follow the live actors rather than a frozen input opening. */
static void start_live_route(void)
{
    game_init();
    prior_keys = 0;
    tap(KEY_A);
    tap(KEY_RIGHT);
    tap(KEY_RIGHT);
    tap(KEY_DOWN);
    tap(KEY_A);
    tap(KEY_A);
    assert(host_ticks == 12 && screen == SCREEN_PLAY && !won && !lost &&
           race_p6_active());
    deployed = 1;
    checkpoint("start");
    start_checkpoint_written = 1;
}

static int snap4(int current, int target, int bound)
{
    int delta = target - current;
    int steps = delta >= 0 ? (delta + 2) / 4 : (delta - 2) / 4;
    int value = current + steps * 4;
    return value < 0 ? 0 : value >= bound ? bound - 1 : value;
}

static void move_cursor(int x, int y)
{
    x = snap4(cx, x, WORLD_W); y = snap4(cy, y, WORLD_H);
    while ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT;
        if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        if (cy > y) held |= KEY_UP;
        step(held);
    }
    step(0);
}

static int issue_group_order(int x, int y, int attack_move)
{
    if (selected_building >= 0) { tap(KEY_L); tap(KEY_L); }
    tap(KEY_R);
    if (!selected_count()) return 0;
    tap(KEY_B);
    if (attack_move)
        for (int i = 0; i < MENU_ATTACK; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    if (mode != (attack_move ? MODE_ATTACK : MODE_MOVE)) {
        if (path_diagnostics)
            fprintf(stderr, "P6 group menu rejected frame=%u mode=%d menu=%d selected=%d\n",
                    (unsigned)frame, mode, menu_open, selected_count());
        return 0;
    }
    move_cursor(x, y); tap(KEY_A);
    if (path_diagnostics)
        fprintf(stderr, "P6 group order input frame=%u target=%d,%d attack=%d mode=%d selected=%d\n",
                (unsigned)frame, x, y, attack_move, mode, selected_count());
    if (path_diagnostics) for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].selected)
        {
            int nx = -1, ny = -1;
            int route = sc64_route_next(u[i].x, u[i].y, x, y, &nx, &ny);
            fprintf(stderr, "  selected slot=%d raw=%d xy=%d,%d order=%d tx=%d,%d path=%d,%d retry=%u route=%d next=%d,%d terrain=%d walk=%d target-terrain=%d target-walk=%d\n",
                    i, u[i].raw_type, u[i].x, u[i].y, u[i].order,
                    u[i].tx, u[i].ty, u[i].path_x, u[i].path_y,
                    u[i].path_retry, route, nx, ny,
                    terrain_blocked(u[i].x / 16, u[i].y / 16),
                    SC64_GRID_GET(path_walkable, u[i].x/16 + (u[i].y/16)*MAP_COLS),
                    terrain_blocked(x / 16, y / 16),
                    SC64_GRID_GET(path_walkable, x/16 + (y/16)*MAP_COLS));
        }
    return 1;
}

static int issue_actor_move(int slot, int x, int y)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        u[slot].owner != SIDE) return 0;
    if (!select_actor_only(slot)) return 0;
    if (!u[slot].selected || selected_count() != 1) {
        for (int i = 0; i <= MAX_UNITS; ++i) {
            tap(KEY_L);
            if (u[slot].selected && selected_count() == 1) break;
        }
    }
    if (!u[slot].selected || selected_count() != 1) return 0;
    tap(KEY_B); tap(KEY_A);
    if (mode != MODE_MOVE) return 0;
    move_cursor(x, y); tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_MOVE;
}

static int issue_selected_actor_move(int slot, int x, int y)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        u[slot].owner != SIDE) return 0;
    if (!(u[slot].selected && selected_count() == 1))
        return issue_actor_move(slot, x, y);
    tap(KEY_B); tap(KEY_A);
    if (mode != MODE_MOVE) return 0;
    move_cursor(x, y); tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_MOVE;
}

static int issue_actor_attack_move(int slot, int x, int y)
{
    if (!issue_actor_move(slot, x, y)) return 0;
    /* Re-selecting the actor through normal input preserves the engine's
     * selection semantics; issue_actor_move leaves its sole actor selected. */
    tap(KEY_B);
    for (int i = 0; i < MENU_ATTACK; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    if (mode != MODE_ATTACK) return 0;
    move_cursor(x, y); tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_ATTACK_MOVE;
}

/* Keep the hero out of the bulk route order. A normal click selects one
 * actor; the ordinary double-tap gesture groups visible units of that type. */
static int select_actor_only(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE)
        return 0;
    /* A moving hero can leave the old cursor destination while the controller
     * crosses the map. Chase its observed position using ordinary cursor keys. */
    for (int attempt = 0; attempt < 600 && u[slot].alive; ++attempt) {
        int dx = u[slot].x - cx, dy = u[slot].y - cy;
        if (abs(dx) <= 3 && abs(dy) <= 3) break;
        u16 held = KEY_R;
        if (dx > 0) held |= KEY_RIGHT;
        else if (dx < 0) held |= KEY_LEFT;
        if (dy > 0) held |= KEY_DOWN;
        else if (dy < 0) held |= KEY_UP;
        step(held);
    }
    step(0);
    tap(KEY_A);
    return u[slot].selected && selected_count() == 1;
}

static int select_zealot_group(void)
{
    int first = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 65 &&
            (first < 0 || u[i].hp > u[first].hp)) first = i;
    if (first < 0) return 0;
    if (!select_actor_only(first)) return 0;
    tap(KEY_A);
    tap(KEY_A); /* Same-target double tap selects visible raw65 actors. */
    return raw_count(65, SIDE) > 0 && selected_count() > 0;
}

static int select_marine_group_near(int x,int y,int radius)
{
    int first=-1;
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==0&&
           dist2(u[i].x,u[i].y,x,y)<=radius*radius&&
           (first<0||u[i].hp>u[first].hp))first=i;
    if(first<0||!select_actor_only(first))return 0;
    tap(KEY_A);tap(KEY_A);
    int n=0;
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].selected){
            if(u[i].raw_type!=0)return 0;
            ++n;
        }
    return n;
}

static int issue_selected_order(int x, int y, int attack_move)
{
    if (!selected_count()) return 0;
    tap(KEY_B);
    if (attack_move)
        for (int i = 0; i < MENU_ATTACK; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    if (mode != (attack_move ? MODE_ATTACK : MODE_MOVE)) return 0;
    move_cursor(x, y);
    tap(KEY_A);
    return mode == MODE_NORMAL;
}

/* Select a rectangular SIDE group with the actual L-drag gesture. This keeps
 * the controller from relying on selection internals when grouping the
 * source-provided eastern Marine force. */
static int select_box_group(int left, int top, int right, int bottom)
{
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right >= WORLD_W) right = WORLD_W - 1;
    if (bottom >= WORLD_H) bottom = WORLD_H - 1;
    if (left > right || top > bottom) return 0;
    move_cursor(left, top);
    int drag_x = snap4(cx, right, WORLD_W);
    int drag_y = snap4(cy, bottom, WORLD_H);
    step(KEY_L); /* press and anchor */
    while ((cx != drag_x || cy != drag_y) &&
           screen == SCREEN_PLAY && !won && !lost) {
        u16 held = KEY_L | KEY_R;
        if (cx < drag_x) held |= KEY_RIGHT;
        else if (cx > drag_x) held |= KEY_LEFT;
        if (cy < drag_y) held |= KEY_DOWN;
        else if (cy > drag_y) held |= KEY_UP;
        step(held);
    }
    step(0); /* release selects every eligible unit in the rectangle */
    return selected_count();
}

static int eastern_marine_bounds(int *left, int *top, int *right, int *bottom)
{
    int n = 0;
    *left = WORLD_W; *top = WORLD_H; *right = *bottom = -1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || u[i].raw_type != 0 ||
            u[i].x < 1200 || u[i].y < 1000) continue;
        if (u[i].x < *left) *left = u[i].x;
        if (u[i].y < *top) *top = u[i].y;
        if (u[i].x > *right) *right = u[i].x;
        if (u[i].y > *bottom) *bottom = u[i].y;
        ++n;
    }
    return n;
}

static int eastern_marine_count_near(int x, int y, int radius)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 0 &&
            dist2(u[i].x, u[i].y, x, y) <= radius * radius) ++n;
    return n;
}

static int marine_bounds_near(int x, int y, int radius,
                              int *left, int *top, int *right, int *bottom)
{
    int n = 0;
    *left = WORLD_W; *top = WORLD_H; *right = *bottom = -1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || u[i].raw_type != 0 ||
            dist2(u[i].x,u[i].y,x,y) > radius * radius) continue;
        if (u[i].x < *left) *left = u[i].x;
        if (u[i].y < *top) *top = u[i].y;
        if (u[i].x > *right) *right = u[i].x;
        if (u[i].y > *bottom) *bottom = u[i].y;
        ++n;
    }
    return n;
}

static int western_marine_bounds(int *left,int *top,int *right,int *bottom)
{
    int n=0;
    *left=WORLD_W;*top=WORLD_H;*right=*bottom=-1;
    for(int i=0;i<MAX_UNITS;++i){
        if(!u[i].alive||u[i].owner!=SIDE||u[i].raw_type!=0||
           u[i].x>=1200||u[i].y>=1000)continue;
        if(u[i].x<*left)*left=u[i].x;
        if(u[i].y<*top)*top=u[i].y;
        if(u[i].x>*right)*right=u[i].x;
        if(u[i].y>*bottom)*bottom=u[i].y;
        ++n;
    }
    return n;
}

static int marked_marines_near(const u8 *slots,int x,int y,int radius)
{
    int n=0;
    if(!slots)return 0;
    for(int i=0;i<MAX_UNITS;++i)
        if(slots[i]&&u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==0&&
           dist2(u[i].x,u[i].y,x,y)<=radius*radius)++n;
    return n;
}

static int zealot_center(int *x, int *y)
{
    int xs[MAX_UNITS], ys[MAX_UNITS], n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 65) {
            xs[n] = u[i].x; ys[n] = u[i].y; ++n;
        }
    if (!n) return 0;
    for (int i = 1; i < n; ++i) {
        int vx = xs[i], vy = ys[i], j = i;
        while (j && xs[j-1] > vx) { xs[j] = xs[j-1]; --j; }
        xs[j] = vx; j = i;
        while (j && ys[j-1] > vy) { ys[j] = ys[j-1]; --j; }
        ys[j] = vy;
    }
    *x = xs[n/2]; *y = ys[n/2];
    return n;
}

static int storm_trap_raw(int raw)
{
    return raw == 203 || raw == 209 || raw == 210 ||
           raw == 211 || raw == 213;
}

/* Score a Storm center against all actors the P6 spell adapter supports.
 * THG2 traps live in the engine's Building pool even though the source marks
 * them as non-building units, so consider those observed positions as well. */
static int storm_candidate_score(int x, int y, int caster, int escort_x,
                                 int escort_y, int escort_present)
{
    int enemy_count = 0, friendly_count = 0;
    if (dist2(x, y, u[caster].x, u[caster].y) >
        SC64_STORM_CAST_RANGE_GBA_PX * SC64_STORM_CAST_RANGE_GBA_PX ||
        dist2(x, y, u[caster].x, u[caster].y) <= 40 * 40) return 0;
    if (escort_present && dist2(x, y, escort_x, escort_y) > 240 * 240)
        return 0;
    for (int other = 0; other < MAX_UNITS; ++other) {
        if (!u[other].alive || dist2(x, y, u[other].x, u[other].y) >
            SC64_STORM_RADIUS_GBA_PX * SC64_STORM_RADIUS_GBA_PX) continue;
        if (race_p6_owners_hostile(SIDE, u[other].owner)) ++enemy_count;
        else if (u[other].owner == SIDE) ++friendly_count;
    }
    for (int other = 0; other < MAX_BUILDINGS; ++other) {
        if (!b[other].alive || !storm_trap_raw(b[other].raw_type) ||
            dist2(x, y, b[other].x, b[other].y) >
                SC64_STORM_RADIUS_GBA_PX * SC64_STORM_RADIUS_GBA_PX) continue;
        if (race_p6_owners_hostile(SIDE, b[other].owner)) ++enemy_count;
        else if (b[other].owner == SIDE) ++friendly_count;
    }
    return enemy_count * 1000 - friendly_count * 1800;
}

static void storm_consider_candidate(int x, int y, int caster, int escort_x,
                                     int escort_y, int escort_present,
                                     int *best_score, int *best_x, int *best_y)
{
    int score = storm_candidate_score(x,y,caster,escort_x,escort_y,escort_present);
    if (score > *best_score) {
        *best_score=score; *best_x=x; *best_y=y;
    }
    /* Actor centers can put the small source-faithful Storm radius on the
     * edge of a packed line. Also test the center of each nearby hostile
     * group; this still uses only observed actor positions. */
    {
        int sx=0,sy=0,n=0;
        for (int i=0;i<MAX_UNITS;++i)
            if (u[i].alive && race_p6_owners_hostile(SIDE,u[i].owner) &&
                dist2(x,y,u[i].x,u[i].y)<=48*48) {
                sx+=u[i].x; sy+=u[i].y; ++n;
            }
        for (int i=0;i<MAX_BUILDINGS;++i)
            if (b[i].alive && storm_trap_raw(b[i].raw_type) &&
                race_p6_owners_hostile(SIDE,b[i].owner) &&
                dist2(x,y,b[i].x,b[i].y)<=48*48) {
                sx+=b[i].x; sy+=b[i].y; ++n;
            }
        if (n > 1) {
            int cx=sx/n,cy=sy/n;
            score=storm_candidate_score(cx,cy,caster,escort_x,escort_y,
                                        escort_present);
            if (score > *best_score) {
                *best_score=score; *best_x=cx; *best_y=cy;
            }
        }
    }
}

/* Choose a legal Storm center from observed actors only. Prefer enemy clusters
 * near the Zealot escort while avoiding friendly fire; no world state is
 * changed except by the resulting normal menu/cursor inputs. */
static int storm_cluster_target(int caster, int escort_x, int escort_y,
                                int escort_present, int allow_single,
                                int *out_x, int *out_y)
{
    int best_score = 0, best_x = -1, best_y = -1;
    if (caster < 0 || !u[caster].alive || u[caster].carry <
        SC64_STORM_ENERGY_COST * SC64_STORM_ENERGY_ONE) return 0;
    /* In the late corridor, clear the nearest eligible frontline cluster
     * before pursuing a denser group behind it. The generic score-first
     * policy can walk past the nearest attackers to reach a farther pack. */
    if (u[caster].y > 1650) {
        int nearest = 0x7fffffff;
        int required = allow_single ? 1000 : 1800;
        for (int candidate = 0; candidate < MAX_UNITS; ++candidate) {
            int score, distance;
            if (!u[candidate].alive ||
                !race_p6_owners_hostile(SIDE, u[candidate].owner)) continue;
            score = storm_candidate_score(u[candidate].x,u[candidate].y,
                caster,escort_x,escort_y,escort_present);
            distance = dist2(u[caster].x,u[caster].y,
                             u[candidate].x,u[candidate].y);
            if (score < required) continue;
            if (distance < nearest ||
                (distance == nearest && score > best_score)) {
                nearest=distance;best_score=score;
                best_x=u[candidate].x;best_y=u[candidate].y;
            }
        }
        for (int candidate = 0; candidate < MAX_BUILDINGS; ++candidate) {
            int score, distance;
            if (!b[candidate].alive || !storm_trap_raw(b[candidate].raw_type) ||
                !race_p6_owners_hostile(SIDE,b[candidate].owner)) continue;
            score = storm_candidate_score(b[candidate].x,b[candidate].y,
                caster,escort_x,escort_y,escort_present);
            distance = dist2(u[caster].x,u[caster].y,
                             b[candidate].x,b[candidate].y);
            if (score < required) continue;
            if (distance < nearest ||
                (distance == nearest && score > best_score)) {
                nearest=distance;best_score=score;
                best_x=b[candidate].x;best_y=b[candidate].y;
            }
        }
        if (best_x >= 0) {
            *out_x=best_x;*out_y=best_y;
            return 1;
        }
        return 0;
    }
    for (int candidate = 0; candidate < MAX_UNITS; ++candidate) {
        int x, y;
        if (!u[candidate].alive ||
            !race_p6_owners_hostile(SIDE, u[candidate].owner)) continue;
        x = u[candidate].x; y = u[candidate].y;
        storm_consider_candidate(x,y,caster,escort_x,escort_y,escort_present,
                                 &best_score,&best_x,&best_y);
    }
    /* Trap source types 203/209/210/211/213 are represented by Building
     * slots, but the P6 Storm integration treats them as non-building actors. */
    for (int candidate = 0; candidate < MAX_BUILDINGS; ++candidate) {
        if (!b[candidate].alive || !storm_trap_raw(b[candidate].raw_type) ||
            !race_p6_owners_hostile(SIDE, b[candidate].owner)) continue;
        storm_consider_candidate(b[candidate].x,b[candidate].y,caster,
            escort_x,escort_y,escort_present,&best_score,&best_x,&best_y);
    }
    if (best_x < 0 || best_score < (allow_single ? 1000 : 1800)) return 0;
    *out_x=best_x; *out_y=best_y;
    return 1;
}

/* When moving units have closed during cursor travel, a center shifted a few
 * pixels toward the caster can put the first pulse on the leading edge. Keep
 * the shift only when it preserves or improves the observed target count and
 * remains outside the caster's self-splash safety radius. */
static void storm_lead_target(int caster, int *x, int *y)
{
    int dx, dy, abs_dx, abs_dy, axis_x, base, best, best_shift = 0;
    if (caster < 0 || caster >= MAX_UNITS || !u[caster].alive) return;
    dx = u[caster].x - *x;
    dy = u[caster].y - *y;
    abs_dx = dx < 0 ? -dx : dx;
    abs_dy = dy < 0 ? -dy : dy;
    axis_x = abs_dx;
    if (abs_dy > axis_x) axis_x = 0;
    else axis_x = 1;
    base = storm_candidate_score(*x, *y, caster, 0, 0, 0);
    best = base;
    int axis_distance = axis_x ? abs_dx : abs_dy;
    int max_shift = axis_distance < 48 ? 0 : axis_distance - 48;
    if (max_shift > 32) max_shift = 32;
    for (int shift = 4; shift <= max_shift; shift += 4) {
        int nx = *x, ny = *y;
        if (axis_x) nx += dx < 0 ? -shift : shift;
        else ny += dy < 0 ? -shift : shift;
        int score = storm_candidate_score(nx, ny, caster, 0, 0, 0);
        if (score > best || (score == best && score > 0 && shift > best_shift)) {
            best = score; best_shift = shift; *x = nx; *y = ny;
        }
    }
}

static int storm_south_retreat_waypoint(int slot, int *out_x, int *out_y)
{
    /* In the southern campaign lane, retreat only toward the already-cleared
     * west side. An unconstrained maximum-clearance search can send Tassadar
     * east through the next hostile group. */
    static const int dir[3][2] = {
        {-1,0},{-1,-1},{-1,1}
    };
    int best_clearance = -1, found = 0;
    for (int distance = 64; distance <= 192; distance += 64)
        for (int k = 0; k < 3; ++k) {
            int x = u[slot].x + dir[k][0] * distance;
            int y = u[slot].y + dir[k][1] * distance;
            int nx = -1, ny = -1, clearance = 0x7fffffff;
            if (x < 16 || y < 16 || x >= MAP_COLS * 16 || y >= MAP_ROWS * 16 ||
                x >= u[slot].x - 16)
                continue;
            if (!SC64_GRID_GET(path_walkable,
                               x / 16 + (y / 16) * MAP_COLS) ||
                !sc64_route_next(u[slot].x, u[slot].y, x, y, &nx, &ny))
                continue;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && race_p6_owners_hostile(SIDE, u[i].owner)) {
                    int d = dist2(x, y, u[i].x, u[i].y);
                    if (d < clearance) clearance = d;
                }
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (b[i].alive && storm_trap_raw(b[i].raw_type) &&
                    race_p6_owners_hostile(SIDE, b[i].owner)) {
                    int d = dist2(x, y, b[i].x, b[i].y);
                    if (d < clearance) clearance = d;
                }
            if (path_diagnostics && u[slot].y > 1650)
                fprintf(stderr,"P6 retreat candidate at=%d,%d next=%d,%d clear2=%d\n",
                        x,y,nx,ny,clearance);
            if (!found || clearance > best_clearance) {
                found = 1; best_clearance = clearance;
                *out_x = x; *out_y = y;
            }
        }
    return found;
}

static int storm_quick_retreat(int slot,int *out_x,int *out_y)
{
    static const int dir[8][2]={{-1,0},{1,0},{0,-1},{0,1},
        {-1,-1},{-1,1},{1,-1},{1,1}};
    int best=-1,found=0;
    for(int k=0;k<8;++k){
        int x=u[slot].x+dir[k][0]*64,y=u[slot].y+dir[k][1]*64;
        int nx=-1,ny=-1,clear=0x7fffffff;
        if(x<16||y<16||x>=WORLD_W||y>=WORLD_H||
           !SC64_GRID_GET(path_walkable,x/16+(y/16)*MAP_COLS)||
           !sc64_route_next(u[slot].x,u[slot].y,x,y,&nx,&ny))continue;
        for(int i=0;i<MAX_UNITS;++i)
            if(u[i].alive&&race_p6_owners_hostile(SIDE,u[i].owner)){
                int d=dist2(x,y,u[i].x,u[i].y);if(d<clear)clear=d;
            }
        for(int i=0;i<MAX_BUILDINGS;++i)
            if(b[i].alive&&storm_trap_raw(b[i].raw_type)&&
               race_p6_owners_hostile(SIDE,b[i].owner)){
                int d=dist2(x,y,b[i].x,b[i].y);if(d<clear)clear=d;
            }
        if(clear>best){best=clear;found=1;*out_x=x;*out_y=y;}
    }
    return found;
}

/* Stop and stage the map cursor near the observed frontline before entering
 * Storm targeting. This prevents the long cursor travel from consuming the
 * last safe distance after a burrowed pack releases. */
static void prepare_storm_frontline(int caster)
{
    int best=0x7fffffff,tx=-1,ty=-1;
    if(caster<0||caster>=MAX_UNITS||!u[caster].alive)return;
    for(int i=0;i<MAX_UNITS;++i)
        if(i!=caster&&u[i].alive&&race_p6_owners_hostile(SIDE,u[i].owner)){
            int d=dist2(u[caster].x,u[caster].y,u[i].x,u[i].y);
            if(d<best){best=d;tx=u[i].x;ty=u[i].y;}
        }
    for(int i=0;i<MAX_BUILDINGS;++i)
        if(b[i].alive&&storm_trap_raw(b[i].raw_type)&&
           race_p6_owners_hostile(SIDE,b[i].owner)){
            int d=dist2(u[caster].x,u[caster].y,b[i].x,b[i].y);
            if(d<best){best=d;tx=b[i].x;ty=b[i].y;}
        }
    if(tx<0||best>240*240||best<=144*144)return;
    if(u[caster].order==ORDER_MOVE)return;
    if(u[caster].order!=ORDER_IDLE&&!stop_actor(caster))return;
    if(dist2(cx,cy,tx,ty)>20*20)move_cursor(tx,ty);
    best=dist2(u[caster].x,u[caster].y,tx,ty);
    if(best<=144*144||best>240*240)return;
    {
        int dx=u[caster].x-tx,dy=u[caster].y-ty;
        int distance=dx<0?-dx:dx;
        if((dy<0?-dy:dy)>distance)distance=dy<0?-dy:dy;
        if(!distance)distance=1;
        int aim_x=tx+dx*112/distance;
        int aim_y=ty+dy*112/distance;
        if(path_diagnostics)
            fprintf(stderr,"P6 frontline stage frame=%u caster=%d,%d front=%d,%d d2=%d aim=%d,%d cursor=%d,%d\n",
                (unsigned)frame,u[caster].x,u[caster].y,tx,ty,best,
                aim_x,aim_y,cx,cy);
        if(u[caster].order!=ORDER_MOVE||
           dist2(u[caster].tx,u[caster].ty,aim_x,aim_y)>32*32)
            (void)issue_actor_move(caster,aim_x,aim_y);
    }
}

/* Stage an ordinary MOVE to the nearest frontier while the actor is still
 * outside acquisition range. The MOVE input leaves the cursor on that target;
 * when the hero reaches Storm range, B+R/A can cast without a slow cursor or
 * menu travel through the already-awake defenders. */
static int frontline_target(int caster,int *x,int *y,int *raw,int *owner,
                            int *slot,int *is_building)
{
    int best=0x7fffffff;
    if(caster<0||caster>=MAX_UNITS||!u[caster].alive||
       u[caster].carry<75*256)return 0;
    for(int i=0;i<MAX_UNITS;++i)
        if(i!=caster&&u[i].alive&&race_p6_owners_hostile(SIDE,u[i].owner)){
            /* Clear only the local southern extraction lane. A remote unit
             * can be numerically closer while lying north of the actual
             * Zeratul route; never chase it from this controller phase. */
            if(u[i].y<frontline_lane_y-240||u[i].y>frontline_lane_y+240||
               u[i].x<frontline_lane_x-200)continue;
            int d=dist2(u[caster].x,u[caster].y,u[i].x,u[i].y);
            if(d>200*200)continue;
            if(d<best){best=d;*x=u[i].x;*y=u[i].y;*raw=u[i].raw_type;
                *owner=u[i].owner;*slot=i;*is_building=0;}
        }
    for(int i=0;i<MAX_BUILDINGS;++i)
        if(b[i].alive&&storm_trap_raw(b[i].raw_type)&&
           race_p6_owners_hostile(SIDE,b[i].owner)){
            if(b[i].y<frontline_lane_y-240||b[i].y>frontline_lane_y+240||
               b[i].x<frontline_lane_x-200)continue;
            int d=dist2(u[caster].x,u[caster].y,b[i].x,b[i].y);
            if(d>200*200)continue;
            if(d<best){best=d;*x=b[i].x;*y=b[i].y;*raw=b[i].raw_type;
                *owner=b[i].owner;*slot=i;*is_building=1;}
        }
    return best<0x7fffffff;
}

static int storm_covers_point(int x,int y,unsigned minimum_pulses)
{
    for(unsigned i=0;i<SC64_STORM_MAX_ACTIVE;++i){
        const Sc64PsionicStorm *e=&race_p6.storm.storm[i];
        if(e->active&&e->pulses_left>=minimum_pulses&&
           sc64_psionic_storm_contains(e,(u16)x,(u16)y))return 1;
    }
    return 0;
}

static int storm_maturely_covers_point(int x,int y)
{
    for(unsigned i=0;i<SC64_STORM_MAX_ACTIVE;++i){
        const Sc64PsionicStorm *e=&race_p6.storm.storm[i];
        if(e->active&&e->pulses_left&&
           e->age_ticks>=SC64_STORM_FIRST_PULSE_TICKS+
                         SC64_STORM_PULSE_INTERVAL_TICKS&&
           sc64_psionic_storm_contains(e,(u16)x,(u16)y))return 1;
    }
    return 0;
}

/* Controller-only spacing rule: avoid spending a second cast only a few
 * pixels from an active field when that field is still pulsing. */
static int storm_field_near_point(int x,int y,int radius)
{
    for(unsigned i=0;i<SC64_STORM_MAX_ACTIVE;++i){
        const Sc64PsionicStorm *e=&race_p6.storm.storm[i];
        if(e->active&&e->pulses_left&&
           dist2(x,y,e->x,e->y)<=radius*radius)return 1;
    }
    return 0;
}

static int storm_recently_started_at(int x,int y)
{
    for(unsigned i=0;i<SC64_STORM_MAX_ACTIVE;++i){
        const Sc64PsionicStorm *e=&race_p6.storm.storm[i];
        if(e->active&&e->x==(u16)x&&e->y==(u16)y&&
           e->age_ticks<=2&&e->pulses_left==SC64_STORM_PULSE_COUNT)return 1;
    }
    return 0;
}

static void frontline_predict(int caster,int slot,int is_building,
                              int *aim_x,int *aim_y);

/* Prepare the ordinary map cursor while Tassadar is still at a safe lane
 * waypoint. This removes the long cursor chase from the cast boundary; the
 * live actor position is re-read every frame and the point remains bounded
 * by the source Storm range. */
static int preposition_frontline_cursor(int caster)
{
    int best=0x7fffffff,slot=-1,bx=0,by=0,building=0;
    if(caster<0||caster>=MAX_UNITS||!u[caster].alive||
       u[caster].carry<75*256||u[caster].y<1650)return 0;
    for(int i=0;i<MAX_UNITS;++i)
        if(i!=caster&&u[i].alive&&race_p6_owners_hostile(SIDE,u[i].owner)&&
           u[i].y>=frontline_lane_y-160&&u[i].y<=frontline_lane_y+160&&
           u[i].x>=frontline_lane_x-200){
            int d=dist2(u[caster].x,u[caster].y,u[i].x,u[i].y);
            if(d<best&&d<=360*360){best=d;slot=i;bx=u[i].x;by=u[i].y;building=0;}
        }
    for(int i=0;i<MAX_BUILDINGS;++i)
        if(b[i].alive&&storm_trap_raw(b[i].raw_type)&&
           race_p6_owners_hostile(SIDE,b[i].owner)&&
           b[i].y>=frontline_lane_y-160&&b[i].y<=frontline_lane_y+160&&
           b[i].x>=frontline_lane_x-200){
            int d=dist2(u[caster].x,u[caster].y,b[i].x,b[i].y);
            if(d<best&&d<=360*360){best=d;slot=i;bx=b[i].x;by=b[i].y;building=1;}
        }
    if(slot<0)return 0;
    if(!building)frontline_predict(caster,slot,0,&bx,&by);
    if(dist2(cx,cy,bx,by)<=12*12)return 0;
    u16 held=KEY_R;
    if(cx<snap4(cx,bx,WORLD_W))held|=KEY_RIGHT;
    if(cx>snap4(cx,bx,WORLD_W))held|=KEY_LEFT;
    if(cy<snap4(cy,by,WORLD_H))held|=KEY_DOWN;
    if(cy>snap4(cy,by,WORLD_H))held|=KEY_UP;
    step(held);
    return 1;
}

/* Storm does not pulse on the cast tick. Estimate the live front unit's
 * observed velocity and aim up to 20 ticks ahead, the first-pulse delay. This
 * is controller prediction only; all observed actor state remains read-only. */
static void frontline_predict(int caster,int slot,int is_building,
                              int *aim_x,int *aim_y)
{
    int x,y,dt,dx,dy;
    if(is_building||slot<0||slot>=MAX_UNITS||!u[slot].alive)return;
    x=u[slot].x;y=u[slot].y;
    if(frontline_history_valid[slot]){
        dt=frame-frontline_last_frame[slot];
        if(dt>0){
            dx=x-frontline_last_x[slot];dy=y-frontline_last_y[slot];
            /* Filter implausible jumps and retain source-pixel motion as
             * observed by the simulation, rather than a unit-speed guess. */
            if(dx>=-4*dt&&dx<=4*dt&&dy>=-4*dt&&dy<=4*dt){
                frontline_vx_q8[slot]=(dx*256)/dt;
                frontline_vy_q8[slot]=(dy*256)/dt;
            }
        }
    }
    frontline_last_x[slot]=x;frontline_last_y[slot]=y;
    frontline_last_frame[slot]=frame;frontline_history_valid[slot]=1;
    /* Aim midway between the first two pulses. A fast Zergling otherwise
     * takes the first hit, moves out of the radius, and survives the volley. */
    int horizon = SC64_STORM_FIRST_PULSE_TICKS +
                  SC64_STORM_PULSE_INTERVAL_TICKS / 2;
    dx=(frontline_vx_q8[slot]*horizon)/256;
    dy=(frontline_vy_q8[slot]*horizon)/256;
    int lead=isqrt_u32((unsigned)(dx*dx+dy*dy));
    int caster_distance=isqrt_u32((unsigned)dist2(u[caster].x,u[caster].y,x,y));
    int max_lead=caster_distance>52?caster_distance-52:0;
    if(max_lead>60)max_lead=60;
    if(lead>max_lead&&lead){dx=(dx*max_lead)/lead;dy=(dy*max_lead)/lead;}
    *aim_x=x+dx;*aim_y=y+dy;
    if(*aim_x<0)*aim_x=0;if(*aim_x>=WORLD_W)*aim_x=WORLD_W-1;
    if(*aim_y<0)*aim_y=0;if(*aim_y>=WORLD_H)*aim_y=WORLD_H-1;
}

static int track_frontline_cursor(int caster)
{
    int moved=0;
    for(int n=0;n<1&&screen==SCREEN_PLAY&&!won&&!lost;++n){
        int tx=0,ty=0,raw=-1,owner=-1,slot=-1,is_building=0;
        if(!frontline_target(caster,&tx,&ty,&raw,&owner,&slot,&is_building))
            return moved;
        int aim_x=tx,aim_y=ty;
        frontline_predict(caster,slot,is_building,&aim_x,&aim_y);
        frontline_aim_x=aim_x;frontline_aim_y=aim_y;
        int sx=snap4(cx,aim_x,WORLD_W),sy=snap4(cy,aim_y,WORLD_H);
        if(sx==cx&&sy==cy)break;
        u16 held=KEY_R;
        if(cx<sx)held|=KEY_RIGHT;
        if(cx>sx)held|=KEY_LEFT;
        if(cy<sy)held|=KEY_DOWN;
        if(cy>sy)held|=KEY_UP;
        step(held);moved=1;
        if(path_diagnostics&&frame%60==0)
            fprintf(stderr,"P6 frontline cursor-track frame=%u raw=%d owner=%d slot=%d building=%d target=%d,%d aim=%d,%d vel=%d,%d cursor=%d,%d caster=%d,%d burrow=%d order=%d hp=%d\n",
                (unsigned)frame,raw,owner,slot,is_building,tx,ty,aim_x,aim_y,
                slot>=0&&!is_building?frontline_vx_q8[slot]:0,
                slot>=0&&!is_building?frontline_vy_q8[slot]:0,
                cx,cy,u[caster].x,u[caster].y,
                slot>=0&&!is_building?race_p6_combat_unit_burrowed(slot):0,
                slot>=0&&!is_building?u[slot].order:-1,
                slot>=0&&!is_building?u[slot].hp:0);
    }
    return moved;
}

static int frontline_waypoint(int caster,int tx,int ty,int *out_x,int *out_y)
{
    static const int ox[8]={128,-128,0,0,90,90,-90,-90};
    static const int oy[8]={0,0,128,-128,90,-90,90,-90};
    int best=0x7fffffff,bx=0,by=0;
    for(int i=0;i<8;++i){
        int x=tx+ox[i],y=ty+oy[i],nx,ny;
        if(x<0||x>=WORLD_W||y<0||y>=WORLD_H||
           dist2(x,y,tx,ty)>144*144||terrain_blocked(x/16,y/16)||
           !SC64_GRID_GET(path_walkable,x/16+(y/16)*MAP_COLS))continue;
        if(sc64_route_next(u[caster].x,u[caster].y,x,y,&nx,&ny)<=0)continue;
        int d=dist2(u[caster].x,u[caster].y,x,y);
        if(d<best){best=d;bx=x;by=y;}
    }
    if(best==0x7fffffff)return 0;
    *out_x=bx;*out_y=by;return 1;
}

static int stage_frontline_move(int caster)
{
    int tx=0,ty=0,raw=-1,owner=-1,slot=-1,is_building=0;
    int best;
    if(!frontline_target(caster,&tx,&ty,&raw,&owner,&slot,&is_building))return 0;
    best=dist2(u[caster].x,u[caster].y,tx,ty);
    if(best>640*640)return 0;
    /* Stop about 112 px short of the stationary frontier. This remains
     * outside the observed 96 px aggro radius while accounting for route
     * arrival tolerance, and avoids ordering the hero through its old tile. */
    if(u[caster].order!=ORDER_IDLE&&!stop_actor(caster))return 0;
    if(path_diagnostics){
        int burrowed=!is_building&&race_p6_combat_unit_burrowed(slot);
        int order=is_building?b[slot].queue_count:u[slot].order;
        int hp=is_building?b[slot].hp:u[slot].hp;
        fprintf(stderr,"P6 frontline stage-input frame=%u caster=%d,%d dist2=%d target=%d raw=%d owner=%d at=%d,%d burrow=%d order=%d hp=%d cursor=%d,%d cursor-prep=1\n",
            (unsigned)frame,u[caster].x,u[caster].y,best,is_building,raw,owner,
            tx,ty,burrowed,order,hp,cx,cy);
    }
    int wx=-1,wy=-1;
    if(!frontline_waypoint(caster,tx,ty,&wx,&wy)||
       !issue_actor_move(caster,wx,wy))return 0;
    if(path_diagnostics)fprintf(stderr,
        "P6 frontline stage-waypoint frame=%u target=%d,%d waypoint=%d,%d\n",
        (unsigned)frame,tx,ty,wx,wy);
    frontline_pursuit=1;
    next_order_frame=frame+120;
    return 1;
}

static int issue_frontline_safe_waypoint(int caster,int tx,int ty)
{
    int wx,wy;
    if(!frontline_waypoint(caster,tx,ty,&wx,&wy))return 0;
    return issue_actor_move(caster,wx,wy);
}

static int issue_move_at_frontline_cursor(int caster)
{
    if(caster<0||caster>=MAX_UNITS||!u[caster].alive||
       !(u[caster].selected&&selected_count()==1))return 0;
    tap(KEY_B);tap(KEY_A);
    if(mode!=MODE_MOVE)return 0;
    tap(KEY_A);
    return u[caster].alive&&u[caster].order==ORDER_MOVE;
}

static int cast_storm_on_staged_cursor(int caster)
{
    int carry_before,score;
    if(caster<0||caster>=MAX_UNITS||!u[caster].alive||
       !(u[caster].selected&&selected_count()==1)||mode!=MODE_NORMAL||
       u[caster].carry<75*256)return 0;
    if(dist2(cx,cy,u[caster].x,u[caster].y)>144*144||
       dist2(cx,cy,u[caster].x,u[caster].y)<=40*40)return 0;
    int tx=0,ty=0,raw=-1,owner=-1,slot=-1,is_building=0;
    if(!frontline_target(caster,&tx,&ty,&raw,&owner,&slot,&is_building))return 0;
    /* Do not spend another cast while an existing Storm still has pulses
     * over the current frontline actor. */
    if(slot>=0&&!is_building&&storm_covers_point(u[slot].x,u[slot].y,2)){
        if(path_diagnostics)fprintf(stderr,
            "P6 staged Storm skipped covered unit frame=%u slot=%d at=%d,%d\n",
            (unsigned)frame,slot,u[slot].x,u[slot].y);
        return 0;
    }
    /* Do not spend a third cast while the existing field still has multiple
     * pulses left over the same frontline actor. */
    if(slot>=0&&!is_building&&storm_covers_point(u[slot].x,u[slot].y,2)){
        if(path_diagnostics)fprintf(stderr,
            "P6 staged Storm skipped covered unit frame=%u slot=%d at=%d,%d\n",
            (unsigned)frame,slot,u[slot].x,u[slot].y);
        return 0;
    }
    /* Do not spend energy on a live unit's old position while the cursor is
     * still catching the estimated first-pulse intercept point. */
    if(dist2(cx,cy,frontline_aim_x,frontline_aim_y)>4*4)return 0;
    score=storm_candidate_score(tx,ty,caster,0,0,0);
    if(score<1000)return 0;
    if(path_diagnostics){
        int near=0x7fffffff,ni=-1;
        for(int i=0;i<MAX_UNITS;++i)if(i!=caster&&u[i].alive&&
            race_p6_owners_hostile(SIDE,u[i].owner)){
            int d=dist2(u[caster].x,u[caster].y,u[i].x,u[i].y);
            if(d<near){near=d;ni=i;}
        }
        if(ni>=0)fprintf(stderr,"P6 frontline cast-input frame=%u caster=%d,%d cursor=%d,%d score=%d nearest=%d raw=%d owner=%d at=%d,%d burrow=%d order=%d hp=%d energy=%d velocity_q8=%d,%d predicted=%d,%d\n",
            (unsigned)frame,u[caster].x,u[caster].y,cx,cy,score,ni,
            u[ni].raw_type,u[ni].owner,u[ni].x,u[ni].y,
            race_p6_combat_unit_burrowed(ni),u[ni].order,u[ni].hp,u[caster].carry,
            slot>=0&&!is_building?frontline_vx_q8[slot]:0,
            slot>=0&&!is_building?frontline_vy_q8[slot]:0,
            frontline_aim_x,frontline_aim_y);
    }
    carry_before=u[caster].carry;
    tap(KEY_B|KEY_R);
    if(mode!=MODE_P4_STORM){
        if(path_diagnostics)fprintf(stderr,"P6 staged Storm shortcut rejected frame=%u mode=%d selected=%d count=%d order=%d energy=%d\n",
            (unsigned)frame,mode,u[caster].selected,selected_count(),
            u[caster].order,u[caster].carry);
        return 0;
    }
    tap(KEY_A);
    int success=u[caster].alive&&carry_before-u[caster].carry>=
        SC64_STORM_ENERGY_COST*SC64_STORM_ENERGY_ONE-32;
    success|=u[caster].alive&&storm_recently_started_at(cx,cy);
    if(path_diagnostics)fprintf(stderr,"P6 staged Storm debit frame=%u success=%d mode=%d before=%d after=%d\n",
        (unsigned)frame,success,mode,carry_before,u[caster].alive?u[caster].carry:-1);
    return success;
}

/* Emergency close-pack tactic for the southern lane only: when an already
 * engaged pack is within one Storm radius, a self-centered cast can trade
 * shields/HP to remove it. Keep this bounded to a healthy caster and refuse
 * to stack the caster inside an existing Storm. */
static int cast_close_pack_storm(int caster)
{
    int nearest=0x7fffffff,nearest_slot=-1,carry_before;
    int center_x,center_y;
    if(caster<0||caster>=MAX_UNITS||!u[caster].alive||
       !(u[caster].selected&&selected_count()==1)||mode!=MODE_NORMAL||
       u[caster].carry<75*256||u[caster].hp+u[caster].shield<=60)return 0;
    for(unsigned i=0;i<SC64_STORM_MAX_ACTIVE;++i)
        if(sc64_psionic_storm_contains(&race_p6.storm.storm[i],
                                       (u16)u[caster].x,(u16)u[caster].y))
            return 0;
    for(int i=0;i<MAX_UNITS;++i)
        if(i!=caster&&u[i].alive&&race_p6_owners_hostile(SIDE,u[i].owner)&&
           !storm_covers_point(u[i].x,u[i].y,2)){
            int d=dist2(u[caster].x,u[caster].y,u[i].x,u[i].y);
            if(d<nearest){nearest=d;nearest_slot=i;}
        }
    if(nearest_slot<0||nearest>SC64_STORM_RADIUS_GBA_PX*
                                SC64_STORM_RADIUS_GBA_PX)
        return 0;
    /* Aim just beyond the caster toward the incoming pack. This keeps the
     * hero outside the 24px field while attackers that stop at melee range
     * remain roughly 20px inside it for the first two pulses. */
    int distance=isqrt_u32((unsigned)nearest);
    if(distance<1)return 0;
    center_x=u[caster].x+(u[nearest_slot].x-u[caster].x)*28/distance;
    center_y=u[caster].y+(u[nearest_slot].y-u[caster].y)*28/distance;
    if(center_x<0)center_x=0;
    if(center_y<0)center_y=0;
    if(center_x>=WORLD_W)center_x=WORLD_W-1;
    if(center_y>=WORLD_H)center_y=WORLD_H-1;
    /* R+direction moves the cursor on a 4px lattice. move_cursor() snaps
     * relative to the current coordinate; a raw odd/even target can be
     * unreachable and make it oscillate forever while the enemy closes. */
    center_x=snap4(cx,center_x,WORLD_W);
    center_y=snap4(cy,center_y,WORLD_H);
    if(cx!=center_x||cy!=center_y)
        move_cursor(center_x,center_y);
    if(!u[caster].alive||!u[caster].selected||selected_count()!=1||
       mode!=MODE_NORMAL||u[caster].carry<75*256||
       u[caster].hp+u[caster].shield<=60||
       cx!=center_x||cy!=center_y||
       dist2(cx,cy,u[caster].x,u[caster].y)<=
           SC64_STORM_RADIUS_GBA_PX*SC64_STORM_RADIUS_GBA_PX||
       dist2(cx,cy,u[caster].x,u[caster].y)>
           SC64_STORM_CAST_RANGE_GBA_PX*SC64_STORM_CAST_RANGE_GBA_PX)
        return 0;
    carry_before=u[caster].carry;
    tap(KEY_B|KEY_R);
    if(mode!=MODE_P4_STORM)return 0;
    tap(KEY_A);
    if(!u[caster].alive||
       (carry_before-u[caster].carry<
           SC64_STORM_ENERGY_COST*SC64_STORM_ENERGY_ONE-32&&
        !storm_recently_started_at(cx,cy)))return 0;
    if(path_diagnostics)fprintf(stderr,
        "P6 close-pack offset-Storm frame=%u caster=%d,%d nearest=%d raw=%d owner=%d at=%d,%d d2=%d hp=%d shield=%d energy=%d center=%d,%d caster_d2=%d\n",
        (unsigned)frame,u[caster].x,u[caster].y,nearest_slot,
        u[nearest_slot].raw_type,u[nearest_slot].owner,
        u[nearest_slot].x,u[nearest_slot].y,nearest,u[caster].hp,
        u[caster].shield,u[caster].carry,cx,cy,
        dist2(cx,cy,u[caster].x,u[caster].y));
    frontline_self_storm_frame=frame;
    return 1;
}

static int late_wait_for_close_pack(int caster)
{
    int nearest=0x7fffffff,slot=-1;
    if(caster<0||caster>=MAX_UNITS||!u[caster].alive||u[caster].y<=1650)
        return 0;
    for(int i=0;i<MAX_UNITS;++i)
        if(i!=caster&&u[i].alive&&race_p6_owners_hostile(SIDE,u[i].owner)){
            int d=dist2(u[caster].x,u[caster].y,u[i].x,u[i].y);
            if(d<nearest){nearest=d;slot=i;}
        }
    return slot>=0&&nearest>56*56&&nearest<=96*96&&
        !storm_covers_point(u[slot].x,u[slot].y,2);
}

static int late_close_pack_ready(int caster)
{
    if(caster<0||caster>=MAX_UNITS||!u[caster].alive||u[caster].y<=1650)
        return 0;
    for(int i=0;i<MAX_UNITS;++i)
        if(i!=caster&&u[i].alive&&race_p6_owners_hostile(SIDE,u[i].owner)&&
           dist2(u[caster].x,u[caster].y,u[i].x,u[i].y)<=56*56&&
           !storm_covers_point(u[i].x,u[i].y,2))return 1;
    return 0;
}

static int covered_frontline_retreat_waypoint(int caster,int *out_x,int *out_y,
                                             int *hostile_slot,int *distance2)
{
    int nearest=0x7fffffff,slot=-1;
    if(caster<0||caster>=MAX_UNITS||!u[caster].alive||u[caster].y<=1650||
       !out_x||!out_y)
        return 0;
    for(int i=0;i<MAX_UNITS;++i)
        if(i!=caster&&u[i].alive&&race_p6_owners_hostile(SIDE,u[i].owner)){
            int d=dist2(u[caster].x,u[caster].y,u[i].x,u[i].y);
            if(d<nearest){nearest=d;slot=i;}
        }
    if(slot<0||nearest>24*24||
       !storm_maturely_covers_point(u[slot].x,u[slot].y)||
       !storm_south_retreat_waypoint(caster,out_x,out_y))return 0;
    if(hostile_slot)*hostile_slot=slot;
    if(distance2)*distance2=nearest;
    return 1;
}

static int zealot_center(int *x, int *y);

static int cast_storm_at_cluster(int caster, int x, int y, int escort_present,
                                 int allow_single)
{
    unsigned active_before = 0, active_after = 0;
    int carry_before;
    storm_self_splash_rejected = 0;
    if (caster < 0 || caster >= MAX_UNITS || !u[caster].alive) return 0;
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i)
        active_before += race_p6.storm.storm[i].active != 0;
    /* The caster often remains selected after its last order. In that case,
     * reuse the live selection so cursor chase time does not cost several
     * hundred frames in the defender pack. */
    if (!(u[caster].selected && selected_count() == 1) &&
        !select_actor_only(caster)) {
        if (path_diagnostics)
            fprintf(stderr, "P6 Storm select rejected frame=%u slot=%d alive=%d selected=%d count=%d energy=%d mode=%d\n",
                    (unsigned)frame, caster, caster >= 0 && u[caster].alive,
                    caster >= 0 && u[caster].selected, selected_count(),
                    caster >= 0 && u[caster].alive ? u[caster].carry : -1, mode);
        return 0;
    }
    /* In the late approach, targeting does not interrupt an existing move
     * order. Stop first so cursor travel cannot carry the caster into the
     * hostile pack before the actual cast input. Keep the established early
     * route unchanged. */
    if (u[caster].y > 1650 && u[caster].order != ORDER_IDLE) {
        if (path_diagnostics)
            fprintf(stderr,"P6 Storm late Stop begin frame=%u at=%d,%d order=%d energy=%d shield=%d\n",
                    (unsigned)frame,u[caster].x,u[caster].y,u[caster].order,
                    u[caster].carry,u[caster].shield);
        if (!stop_actor(caster)) {
            if (path_diagnostics)
                fprintf(stderr,"P6 Storm late Stop rejected frame=%u order=%d\n",
                        (unsigned)frame,u[caster].order);
            return 0;
        }
        if (path_diagnostics)
            fprintf(stderr,"P6 Storm late Stop done frame=%u at=%d,%d order=%d energy=%d shield=%d\n",
                    (unsigned)frame,u[caster].x,u[caster].y,u[caster].order,
                    u[caster].carry,u[caster].shield);
    }
    /* Actor selection can consume many simulation ticks while the cursor
     * catches up. Re-select a currently legal target after stopping the
     * caster, preserving escort-friendly-fire avoidance. */
    if (escort_present) {
        int escort_x=0,escort_y=0;
        escort_present=zealot_center(&escort_x,&escort_y)>0;
        if (escort_present) {
            if (!storm_cluster_target(caster,escort_x,escort_y,1,allow_single,&x,&y)) {
                if(path_diagnostics)fprintf(stderr,"P6 Storm retarget rejected frame=%u escort\n",(unsigned)frame);
                return 0;
            }
        }
    } else if (!storm_cluster_target(caster,0,0,0,allow_single,&x,&y)) {
        if(path_diagnostics)fprintf(stderr,"P6 Storm retarget rejected frame=%u solo\n",(unsigned)frame);
        return 0;
    }
    if(allow_single&&u[caster].y>1750&&storm_covers_point(x,y,2)){
        if(path_diagnostics)fprintf(stderr,
            "P6 Storm skipped covered target frame=%u target=%d,%d\n",
            (unsigned)frame,x,y);
        return 0;
    }
    if (!race_p6_storm_menu_available()) {
        if(path_diagnostics)fprintf(stderr,"P6 Storm menu unavailable frame=%u\n",(unsigned)frame);
        return 0;
    }
    /* Preserve the original menu route through the opening gates. Use the
     * faster P6 shortcut only for the late southern extraction approach. */
    if (allow_single && u[caster].y > 1750) {
        tap(KEY_B | KEY_R);
    } else {
        tap(KEY_B);
        for (int i = 0; i < MENU_STOP + 1; ++i) tap(KEY_DOWN);
        tap(KEY_A);
    }
    if (mode != MODE_P4_STORM) {
        if (path_diagnostics)
            fprintf(stderr, "P6 Storm menu rejected frame=%u mode=%d cursor=%d count=%d\n",
                    (unsigned)frame, mode, menu_cursor, field_menu_count());
        return 0;
    }
    /* Preserve the established early-gate route. Only refresh the target
     * during the southern extraction approach, where the pack can move
     * substantially while the cursor crosses the map. */
    if (allow_single && u[caster].y > 1750) {
        storm_lead_target(caster, &x, &y);
        move_cursor(x, y);
        int escort_x = 0, escort_y = 0;
        int escort_now = escort_present && zealot_center(&escort_x, &escort_y) > 0;
        /* The cursor may now be near a farther, denser group while the
         * frontline has moved closer. Re-select the closest live threat after
         * travel so a cursor hit cannot make us walk past the front row. */
        {
            int next_x = -1, next_y = -1;
            if (!storm_cluster_target(caster, escort_x, escort_y, escort_now,
                                      allow_single, &next_x, &next_y))
                return 0;
            x = next_x; y = next_y;
            storm_lead_target(caster, &x, &y);
            if (cx != x || cy != y) move_cursor(x, y);
        }
    } else {
        move_cursor(x, y);
    }
    /* Cursor travel advances the live simulation. Recheck caster splash
     * safety against its position at the actual input boundary. */
    if (dist2(cx,cy,u[caster].x,u[caster].y) <= 40 * 40) {
        storm_self_splash_rejected = 1;
        if(path_diagnostics)fprintf(stderr,"P6 Storm self-splash rejected frame=%u center=%d,%d caster=%d,%d\n",
            (unsigned)frame,cx,cy,u[caster].x,u[caster].y);
        return 0;
    }
    if (dist2(cx,cy,u[caster].x,u[caster].y) >
        SC64_STORM_CAST_RANGE_GBA_PX * SC64_STORM_CAST_RANGE_GBA_PX) {
        if(path_diagnostics)fprintf(stderr,"P6 Storm range rejected frame=%u caster=%d,%d cursor=%d,%d\n",
            (unsigned)frame,u[caster].x,u[caster].y,cx,cy);
        return 0;
    }
    if (path_diagnostics)
        fprintf(stderr,"P6 Storm cast center frame=%u center=%d,%d caster=%d,%d dist2=%d shield=%d\n",
            (unsigned)frame,cx,cy,u[caster].x,u[caster].y,
            dist2(cx,cy,u[caster].x,u[caster].y),u[caster].shield);
    if (path_diagnostics) {
        int nearest = 0x7fffffff, near_slot = -1;
        for (int i=0;i<MAX_UNITS;++i)
            if (i!=caster&&u[i].alive&&race_p6_owners_hostile(SIDE,u[i].owner)) {
                int d=dist2(u[caster].x,u[caster].y,u[i].x,u[i].y);
                if(d<nearest){nearest=d;near_slot=i;}
            }
        if(near_slot>=0)
            fprintf(stderr,"P6 Storm nearest hostile slot=%d raw=%d owner=%d at=%d,%d d2=%d hp=%d\n",
                near_slot,u[near_slot].raw_type,u[near_slot].owner,
                u[near_slot].x,u[near_slot].y,nearest,u[near_slot].hp);
    }
    carry_before = u[caster].carry;
    tap(KEY_A);
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i)
        active_after += race_p6.storm.storm[i].active != 0;
    /* A previous Storm can expire while the controller is selecting the
     * caster and moving the cursor. The replacement can therefore keep the
     * active-count constant; verify the actual Q8 energy debit as well. */
    return active_after > active_before || storm_recently_started_at(cx,cy) ||
        (u[caster].alive && carry_before - u[caster].carry >=
         SC64_STORM_ENERGY_COST * SC64_STORM_ENERGY_ONE - 16);
}

static int stop_actor(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        !select_actor_only(slot)) return 0;
    if (u[slot].order == ORDER_IDLE) return 1;
    tap(KEY_B);
    for (int i = 0; i < MENU_STOP; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_IDLE;
}


static int army_center(int *x, int *y)
{
    int xs[MAX_UNITS], ys[MAX_UNITS], n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE &&
            !sc64_race_is_worker(u[i].type) &&
            sc64_game_unit_selectable(u[i].type)) {
            xs[n] = u[i].x; ys[n] = u[i].y; ++n;
        }
    if (!n) return 0;
    for (int i = 1; i < n; ++i) {
        int vx = xs[i], vy = ys[i], j = i;
        while (j && xs[j-1] > vx) { xs[j] = xs[j-1]; --j; }
        xs[j] = vx; j = i;
        while (j && ys[j-1] > vy) { ys[j] = ys[j-1]; --j; }
        ys[j] = vy;
    }
    *x = xs[n/2]; *y = ys[n/2];
    return n;
}

static int terrain_only_route(int sx, int sy, int tx, int ty,
                              int *nearest_x, int *nearest_y, int with_buildings)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS];
    int start = sx / 16 + (sy / 16) * MAP_COLS;
    int goal = tx / 16 + (ty / 16) * MAP_COLS;
    int head = 0, tail = 0, best = 0x7fffffff, best_cell = -1;
    memset(seen, 0, sizeof(seen));
    if (start < 0 || start >= MAP_CELLS || goal < 0 || goal >= MAP_CELLS ||
        (with_buildings ? !SC64_GRID_GET(path_walkable,start) :
                          terrain_blocked(sx/16, sy/16))) return 0;
    queue[tail++] = (u16)start; seen[start] = 1;
    while (head < tail) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
        int d = abs(col - goal % MAP_COLS) + abs(row - goal / MAP_COLS);
        if (d < best) { best = d; best_cell = cell; }
        if (cell == goal) {
            if (nearest_x) *nearest_x = col * 16 + 8;
            if (nearest_y) *nearest_y = row * 16 + 8;
            return 1;
        }
        const int dc[4] = {1,-1,0,0}, dr[4] = {0,0,1,-1};
        for (int k = 0; k < 4; ++k) {
            int c = col + dc[k], r = row + dr[k], next = c + r * MAP_COLS;
            if (c < 0 || c >= MAP_COLS || r < 0 || r >= MAP_ROWS || seen[next] ||
                (with_buildings ? !SC64_GRID_GET(path_walkable,next) :
                                  terrain_blocked(c,r))) continue;
            seen[next] = 1; queue[tail++] = (u16)next;
        }
    }
    if (nearest_x && best_cell >= 0) *nearest_x = (best_cell % MAP_COLS) * 16 + 8;
    if (nearest_y && best_cell >= 0) *nearest_y = (best_cell / MAP_COLS) * 16 + 8;
    return 0;
}

static void diagnose_terrain_route(int sx, int sy, int tx, int ty)
{
    static u16 queue[MAP_CELLS];
    static s32 parent[MAP_CELLS];
    int start=sx/16+(sy/16)*MAP_COLS, goal=tx/16+(ty/16)*MAP_COLS;
    int head=0,tail=0;
    for(int i=0;i<MAP_CELLS;++i)parent[i]=-1;
    if(start<0||start>=MAP_CELLS||goal<0||goal>=MAP_CELLS)return;
    queue[tail++]=(u16)start;parent[start]=start;
    while(head<tail&&parent[goal]<0){
        int cell=queue[head++],col=cell%MAP_COLS,row=cell/MAP_COLS;
        const int dc[4]={1,-1,0,0},dr[4]={0,0,1,-1};
        for(int k=0;k<4;++k){
            int c=col+dc[k],r=row+dr[k],next=c+r*MAP_COLS;
            if(c<0||c>=MAP_COLS||r<0||r>=MAP_ROWS||parent[next]>=0||
               terrain_blocked(c,r))continue;
            parent[next]=cell;queue[tail++]=(u16)next;
        }
    }
    if(parent[goal]<0){fprintf(stderr,"P6 terrain route unavailable\n");return;}
    int cell=goal,n=0;
    static u16 reverse_path[MAP_CELLS];
    while(cell!=start&&n<MAP_CELLS){reverse_path[n++]=(u16)cell;cell=parent[cell];}
    fprintf(stderr,"P6 terrain route len=%d building-blocked cells:",n);
    int shown=0;
    while(n){
        cell=reverse_path[--n];
        if(SC64_GRID_GET(path_walkable,cell))continue;
        int x=(cell%MAP_COLS)*16+8,y=(cell/MAP_COLS)*16+8;
        fprintf(stderr," (%d,%d",x,y);
        int found=0;
        for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&
            race_p6_building_collidable(i)&&
            abs((int)b[i].x-x)<=80&&abs((int)b[i].y-y)<=80){
            fprintf(stderr," b%d/raw%d@%d,%d",i,b[i].raw_type,b[i].x,b[i].y);found=1;
        }
        if(!found)fputs(" no-near-building",stderr);
        fputc(')',stderr);if(++shown==24)break;
    }
    fputc('\n',stderr);
}

/* Read-only variants of build_path_grid used to isolate P6 connectivity. */
static void diagnose_grid_variant(int mode, int sx, int sy, int gx, int gy)
{
    static u8 walk[MAP_CELLS], seen[MAP_CELLS];
    static u16 queue[MAP_CELLS];
    for(int row=0;row<MAP_ROWS;++row)for(int col=0;col<MAP_COLS;++col)
        walk[col+row*MAP_COLS]=(u8)!terrain_blocked(col,row);
    for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive){
        int raw=b[i].raw_type>=0?b[i].raw_type:sc64_race_raw_building(b[i].type);
        int door=raw>=205&&raw<=208;
        int beacon=raw==195||raw==196;
        int left,up,right,down;
        if((mode&1)&&beacon)continue;
        if(door&&((mode&4)||((mode&8)&&raw==205)||
                  ((mode&16)&&raw==206)||((mode&32)&&raw>=207)||
                  !race_p6_building_collidable(i)))continue;
        if(!door){
            unsigned flags=sc64_game_building_actor_flags(b[i].type);
            if(flags&&(flags&SC64_GAME_ACTOR_COLLIDABLE)==0)continue;
        }
        if(!sc64_game_building_bounds_source(b[i].type,&left,&up,&right,&down))continue;
        int x0=(int)b[i].x*2-left,x1=(int)b[i].x*2+right;
        int y0=(int)b[i].y*2-up,y1=(int)b[i].y*2+down;
        for(int row=0;row<MAP_ROWS;++row)for(int col=0;col<MAP_COLS;++col){
            int x0cell=col*32,y0cell=row*32;
            int overlap=x0cell<=x1&&x0cell+32>=x0&&
                        y0cell<=y1&&y0cell+32>=y0;
            int center=(x0cell+16>=x0&&x0cell+16<=x1&&
                        y0cell+16>=y0&&y0cell+16<=y1);
            if((mode&2)&&door?center:overlap)walk[col+row*MAP_COLS]=0;
        }
    }
    int start=sx/16+(sy/16)*MAP_COLS,goal=gx/16+(gy/16)*MAP_COLS;
    memset(seen,0,sizeof(seen));int head=0,tail=0,nearest=-1,best=0x7fffffff;
    if(start>=0&&start<MAP_CELLS&&walk[start]){queue[tail++]=(u16)start;seen[start]=1;}
    while(head<tail){
        int cell=queue[head++],col=cell%MAP_COLS,row=cell/MAP_COLS;
        int d=(col-gx/16)*(col-gx/16)+(row-gy/16)*(row-gy/16);
        if(d<best){best=d;nearest=cell;}
        const int dc[4]={1,-1,0,0},dr[4]={0,0,1,-1};
        for(int k=0;k<4;++k){int c=col+dc[k],r=row+dr[k],n=c+r*MAP_COLS;
            if(c<0||c>=MAP_COLS||r<0||r>=MAP_ROWS||seen[n]||!walk[n])continue;
            seen[n]=1;queue[tail++]=(u16)n;}
    }
    int goal_walk=goal>=0&&goal<MAP_CELLS?walk[goal]:0;
    int dist_px=-1;
    if(best<0x7fffffff){dist_px=0;while((dist_px+1)*(dist_px+1)<=best*256)++dist_px;}
    fprintf(stderr,"P6 grid variant mode=%d start_component=%d goal_walk=%d same=%d nearest_to_goal=(%d,%d) dist=%dpx\n",
        mode,tail,goal_walk,goal>=0&&goal<MAP_CELLS&&seen[goal],
        nearest>=0?(nearest%MAP_COLS)*16+8:-1,
        nearest>=0?(nearest/MAP_COLS)*16+8:-1,
        dist_px);
}

static void launch_from_menu(void)
{
    assert(sc64_catalog_playable_count(SC64_FACTION_PROTOSS) == 6);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_PROTOSS,
                                         SC64_P6_GLOBAL_ID) == 5);
    game_init(); prior_keys = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION &&
           race_front.faction == SC_FRONT_PROTOSS);
    tap(KEY_RIGHT); tap(KEY_RIGHT); tap(KEY_DOWN);
    assert(race_front.mission_page == 2 && race_front.row == 1);
    assert(sc64_catalog_playable_at(SC64_FACTION_PROTOSS, 5) == SC64_P6_GLOBAL_ID);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING &&
           race_front.mission == SC64_P6_GLOBAL_ID);
    tap(KEY_A);
    deployed = 1;
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == SC64_P6_GLOBAL_ID && race_p6_active());
    assert(race_campaign.mission == SC64_P6_GLOBAL_ID);
    if (path_diagnostics) {
        fprintf(stderr, "P6 economy frame=%u minerals=%d gas=%d supply=%d/%d\n",
                (unsigned)frame, minerals, gas, supply_used, supply_cap);
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive)
            fprintf(stderr, "  unit slot=%d raw=%d owner=%d src=%d xy=%d,%d hp=%d order=%d target=%d\n",
                    i, u[i].raw_type, u[i].owner, u[i].raw_owner,
                    u[i].x, u[i].y, u[i].hp, u[i].order, u[i].target);
        for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive)
            fprintf(stderr, "  building slot=%d raw=%d owner=%d src=%d xy=%d,%d hp=%d type=%d\n",
                    i, b[i].raw_type, b[i].owner, b[i].raw_owner,
                    b[i].x, b[i].y, b[i].hp, b[i].type);
        fprintf(stderr, "P6 slot0 active=%u owner=%d slot7 active=%u owner=%d\n",
                race_p6_world.source_slot_active[0],
                race_p6_world.live_owner_for_source[0],
                race_p6_world.source_slot_active[7],
                race_p6_world.live_owner_for_source[7]);
        for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i)
            if (sc64_p6_doodads[i].raw_type == 207 ||
                sc64_p6_doodads[i].raw_type == 208)
                fprintf(stderr, "P6 doodad row=%u raw=%u xy=%u,%u owner=%u enabled=%u boundslot=%u\n",
                        i, sc64_p6_doodads[i].raw_type,
                        sc64_p6_doodads[i].x, sc64_p6_doodads[i].y,
                        sc64_p6_doodads[i].raw_owner,
                        race_p6_doodad_bindings[i].enabled,
                        race_p6_doodad_bindings[i].building_slot);
    }
}

static void promote_verified_trace(void)
{
    int z = raw_unit(SC64_P6_RAW_TEMPLAR, SIDE);
    int t = raw_unit(SC64_P6_RAW_HERO, SIDE);
    int l, top, r, bottom;
    assert(objective_location(&l, &top, &r, &bottom));
    assert(screen == SCREEN_RESULT && won && !lost && z >= 0 && t >= 0 &&
           u[z].raw_owner == 2 && u[z].owner == SIDE &&
           u[t].raw_owner == 2 && u[t].owner == SIDE &&
           u[z].x >= l && u[z].x < r && u[z].y >= top && u[z].y < bottom &&
           race_p6.runtime.model.outcome == SC64_P6_WON &&
           (race_completed & (1 << SC64_P6_GLOBAL_ID)) != 0);
    if (rename(attempt_path, verified_path) != 0) {
        FILE *src = fopen(attempt_path, "rb"), *dst = fopen(verified_path, "wb");
        assert(src && dst);
        char buffer[4096]; size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), src)) != 0)
            assert(fwrite(buffer, 1, n, dst) == n);
        assert(!ferror(src)); fclose(src); fclose(dst); remove(attempt_path);
    }
}

int main(int argc, char **argv)
{
    const char *tag = NULL;
    const char *trace_output_path = NULL;
    int team_escort = 1;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--limit") && i + 1 < argc) run_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--attempt-tag") && i + 1 < argc) tag = argv[++i];
        else if (!strcmp(argv[i], "--path-diagnostics")) path_diagnostics = 1;
        else if (!strcmp(argv[i], "--team-escort")) team_escort = 1;
        else if (!strcmp(argv[i], "--prefix-inputs") && i + 1 < argc)
            prefix_inputs_path = argv[++i];
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc)
            trace_output_path = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_p6_campaign_run [--limit FRAMES] [--attempt-tag TAG] [--trace PATH] [--path-diagnostics] [--prefix-inputs FILE] [--team-escort]");
            return 0;
        } else return 2;
    }
    if (run_limit < 1 || run_limit > 1000000) return 2;
    team_escort_diagnostics = team_escort;
    if (path_diagnostics)
        fprintf(stderr, "P6 controller options limit=%d team=%d prefix=%s\n",
                run_limit, team_escort, prefix_inputs_path ? prefix_inputs_path : "none");
    if (tag) {
        for (const char *p = tag; *p; ++p)
            if (!( (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                   (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' )) return 2;
        if (snprintf(attempt_path, sizeof(attempt_path),
                     ".local/sc64/p6-controller-%s-running.json", tag) >=
                (int)sizeof(attempt_path) ||
            snprintf(final_path, sizeof(final_path),
                     ".local/sc64/p6-controller-%s.json", tag) >=
                (int)sizeof(final_path)) return 2;
    }
    if (trace_output_path) {
        if (snprintf(attempt_path, sizeof(attempt_path), "%s.running",
                     trace_output_path) >= (int)sizeof(attempt_path) ||
            snprintf(final_path, sizeof(final_path), "%s",
                     trace_output_path) >= (int)sizeof(final_path) ||
            snprintf(verified_path, sizeof(verified_path), "%s",
                     trace_output_path) >= (int)sizeof(verified_path)) return 2;
    }
    mkdir(".local", 0777); mkdir(".local/sc64", 0777);
    trace = fopen(attempt_path, "w"); assert(trace);
    fprintf(trace, "{\"schema\":1,\"mission\":25,\"profile\":\"protoss-p6\","
                   "\"policy\":\"normal-input-rescue-zeratul-and-reach-extraction\",\"events\":[\n");
    gba_init();
    if (prefix_inputs_path) replay_prefix_inputs(prefix_inputs_path);
    else start_live_route();
    if (!start_checkpoint_written) {
        checkpoint("start");
        start_checkpoint_written = 1;
    }

    int l, top, r, bottom;
    assert(objective_location(&l, &top, &r, &bottom));
    int goal_x = (l + r) / 2, goal_y = (top + bottom) / 2;
    static const u8 gate_trigger[6] = {2,3,4,5,6,7};
    static const u8 gate_location[6] = {1,2,3,5,6,4};
    int waiting_for_storm = 0;
    int waiting_at_east_shelf = 0;
    int waiting_at_south_safe = 0;
    int waiting_for_full_south_charge = 0;
    int south_safe_x = 0, south_safe_y = 0;
    int south_stage = 0;
    int marine_team_phase = team_escort ? 0 : 3;
    int rescued_marine_rally_phase=0;
    int team_front_action_issued = 0;
    unsigned team_support_order_frame=0,team_storm_wait_until=0;
    int team_support_x=-1,team_support_y=-1,team_storm_casts=0;
    int team_follow_ordered=0;
    int team_flank_ordered=0;
    int team_flank_pending=0;
    int team_west_ordered=0;
    int team_repositioned=0;
    int route_direct_rescue=0;
    u8 team_east_slots[MAX_UNITS]={0};
    const int marine_stage_x = 720, marine_stage_y = 1640;
    const int marine_front_x = 860, marine_front_y = 1800;
    while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost) {
        int zeratul_rescue = raw_unit(SC64_P6_RAW_TEMPLAR, RESCUABLE);
        int zeratul = raw_unit(SC64_P6_RAW_TEMPLAR, SIDE);
        int tassadar = raw_unit(SC64_P6_RAW_HERO, SIDE);
        log_south_combat_snapshot((unsigned)frame, tassadar);
        int army_x = 0, army_y = 0;
        int army_n = zealot_center(&army_x, &army_y);
        int east_arrived = marked_marines_near(team_east_slots,
                                                marine_stage_x,
                                                marine_stage_y, 96);
        int gate_pending = 0;
        for (int j=0;j<6;++j)
            gate_pending |= !sc64_p6_trigger_started(&race_p6.runtime.model,
                                                       gate_trigger[j]);
        /* Recruit living source Marines using Tassadar's normal movement.
         * The campaign tick converts a RESCUABLE unit to SIDE only when a
         * living SIDE actor comes within 32 pixels, so approach the live
         * cluster with a reachable point inside that radius. */
        int rescue_marine=-1, rescue_dist=0x7fffffff;
        if (tassadar>=0 && raw_count(0,SIDE)<9) for (int i=0;i<MAX_UNITS;++i)
            if (u[i].alive && u[i].owner==RESCUABLE && u[i].raw_type==0 &&
                u[i].x<1200 && u[i].y<900) {
                int d=dist2(u[tassadar].x,u[tassadar].y,u[i].x,u[i].y);
                if(d<rescue_dist){rescue_dist=d;rescue_marine=i;}
            }
        if (rescue_marine>=0 && tassadar>=0 &&
            (frame>=next_order_frame || rescue_dist<=32*32)) {
            int rx=u[rescue_marine].x, ry=u[rescue_marine].y;
            static const int rescue_offsets[][2]={{24,0},{-24,0},{0,24},{0,-24},
                {16,16},{-16,16},{16,-16},{-16,-16},{32,0},{-32,0},{0,32},{0,-32}};
            int approach_x=-1,approach_y=-1,route_nx=-1,route_ny=-1;
            if(rescue_dist>32*32) for(unsigned k=0;k<sizeof(rescue_offsets)/sizeof(rescue_offsets[0]);++k){
                int x=rx+rescue_offsets[k][0], y=ry+rescue_offsets[k][1];
                if(x<0||x>=WORLD_W||y<0||y>=WORLD_H||terrain_blocked(x/16,y/16)||
                   !SC64_GRID_GET(path_walkable,x/16+(y/16)*MAP_COLS))continue;
                if(terrain_only_route(u[tassadar].x,u[tassadar].y,x,y,&route_nx,&route_ny,1)){
                    approach_x=x;approach_y=y;break;
                }
            }
            if(rescue_dist>32*32 && approach_x>=0){
                int cx=approach_x,cy=approach_y;
                if(dist2(u[tassadar].x,u[tassadar].y,cx,cy)>64*64)
                    (void)route_point_ahead(u[tassadar].x,u[tassadar].y,cx,cy,6,&cx,&cy);
                int issued=issue_actor_move(tassadar,cx,cy);
                next_order_frame=frame+180;
                if(path_diagnostics)fprintf(stderr,
                    "P6 Marine rescue approach frame=%u slot=%d at=%d,%d distance2=%d Tassadar=%d,%d goal=%d,%d command=%d,%d route=%d next=%d,%d issued=%d host=%u\n",
                    (unsigned)frame,rescue_marine,rx,ry,rescue_dist,
                    u[tassadar].x,u[tassadar].y,approach_x,approach_y,cx,cy,
                    1,route_nx,route_ny,issued,host_ticks);
                if(path_diagnostics)fprintf(stderr,"P6 Marine rescue census owned=%d unresolved=%d\n",
                    raw_count(0,SIDE),raw_count(0,RESCUABLE));
                if(path_diagnostics)fprintf(stderr,
                    "P6 Marine rescue actor slot=%d xy=%d,%d order=%d target=%d,%d path=%d,%d retry=%u hp=%d shield=%d energy=%d\n",
                    tassadar,u[tassadar].x,u[tassadar].y,u[tassadar].order,
                    u[tassadar].tx,u[tassadar].ty,u[tassadar].path_x,
                    u[tassadar].path_y,u[tassadar].path_retry,u[tassadar].hp,
                    u[tassadar].shield,u[tassadar].carry);
                step(0);
                continue;
            }
            if(rescue_dist<=32*32){
                if(u[tassadar].order!=ORDER_IDLE)(void)stop_actor(tassadar);
                next_order_frame=frame+30;
                if(path_diagnostics&&frame%30==0)fprintf(stderr,
                    "P6 Marine rescue contact frame=%u slot=%d dist2=%d converted=%d living-marines=%d at=%d,%d host=%u\n",
                    (unsigned)frame,rescue_marine,rescue_dist,
                    u[rescue_marine].owner==SIDE,raw_count(0,SIDE),rx,ry,host_ticks);
                step(0);
                continue;
            }
        }
        /* Regroup the two safe rescue clusters before sending them ahead of
         * Tassadar. Keep the four western Marines together on a short move to
         * the five eastern Marines, then issue one group attack-move when the
         * hero reaches the southern lane. */
        if (!gate_pending && raw_count(0,SIDE)>=9 && tassadar>=0) {
            if(rescued_marine_rally_phase==0){
                int n=select_marine_group_near(276,713,112);
                int ordered=n>=3&&issue_selected_order(996,500,0);
                if(ordered)rescued_marine_rally_phase=1;
                if(path_diagnostics)fprintf(stderr,
                    "P6 Marine regroup order frame=%u selected=%d ordered=%d target=996,500 host=%u\n",
                    (unsigned)frame,n,ordered,host_ticks);
                step(0);continue;
            }
            if(rescued_marine_rally_phase==1 &&
               eastern_marine_count_near(996,500,128)>=8){
                rescued_marine_rally_phase=2;
                if(path_diagnostics)fprintf(stderr,
                    "P6 Marine regroup ready frame=%u count=%d T=%d,%d\n",
                    (unsigned)frame,eastern_marine_count_near(996,500,128),
                    u[tassadar].x,u[tassadar].y);
            }
            if(rescued_marine_rally_phase==2 && u[tassadar].y>=1500 &&
               u[tassadar].carry>=250*256 && u[tassadar].shield>=230){
                int n=select_marine_group_near(996,500,180);
                int ordered=n>=8&&issue_selected_order(marine_front_x,
                                                       marine_front_y,1);
                if(ordered)rescued_marine_rally_phase=3;
                if(path_diagnostics)fprintf(stderr,
                    "P6 Marine escort commit frame=%u selected=%d ordered=%d target=%d,%d T=%d,%d host=%u\n",
                    (unsigned)frame,n,ordered,marine_front_x,marine_front_y,
                    u[tassadar].x,u[tassadar].y,host_ticks);
                step(0);continue;
            }
        }
        if(path_diagnostics&&frame>=30000&&frame%600==0){
            int near=eastern_marine_count_near(marine_front_x,marine_front_y,112);
            int printed=0;
            fprintf(stderr,"P6 marine escort census frame=%u owned=%d near-front=%d T=%d,%d\n",
                (unsigned)frame,raw_count(0,SIDE),near,tassadar>=0?u[tassadar].x:-1,
                tassadar>=0?u[tassadar].y:-1);
            for(int i=0;i<MAX_UNITS&&printed<8;++i)
                if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==0){
                    fprintf(stderr,"  escort slot=%d at=%d,%d order=%d target=%d,%d hp=%d\n",
                        i,u[i].x,u[i].y,u[i].order,u[i].tx,u[i].ty,u[i].hp);++printed;
                }
        }
        if(path_diagnostics&&team_escort&&frame>=108000&&frame%600==0)
            fprintf(stderr,"P6 team policy frame=%u gates=%d wait=%d pursuit=%d next_order=%u next_storm=%u lane=%d,%d follow=%d flank=%d reposition=%d host=%u\n",
                (unsigned)frame,gate_pending,waiting_for_storm,
                frontline_pursuit,(unsigned)next_order_frame,
                (unsigned)next_storm_frame,frontline_lane_x,
                frontline_lane_y,team_follow_ordered,team_flank_ordered,
                team_repositioned,(unsigned)host_ticks);
        if (marine_team_phase < 3) {
            if (marine_team_phase == 0) {
                int left, top, right, bottom;
                int candidates = eastern_marine_bounds(&left, &top, &right, &bottom);
                int selected = candidates ? select_box_group(left - 4, top - 4,
                                                              right + 4, bottom + 4) : 0;
                int selected_marines = 0;
                fprintf(stderr, "P6 team box frame=%u candidates=%d bounds=%d,%d..%d,%d selected=%d",
                        (unsigned)frame, candidates, left, top, right, bottom,
                        selected);
                for (int i = 0; i < MAX_UNITS; ++i)
                    if (u[i].alive && u[i].owner == SIDE && u[i].selected) {
                        fprintf(stderr, " [%d raw=%d at=%d,%d hp=%d]",
                                i,u[i].raw_type,u[i].x,u[i].y,u[i].hp);
                        if (u[i].raw_type == 0) {
                            ++selected_marines;
                            team_east_slots[i]=1;
                        }
                    }
                int ordered = selected_marines == candidates && candidates >= 6 &&
                    issue_selected_order(marine_stage_x, marine_stage_y, 0);
                fprintf(stderr, " marines=%d ordered=%d target=%d,%d host=%u\n",
                        selected_marines, ordered, marine_stage_x, marine_stage_y,
                        host_ticks);
                if (path_diagnostics) for (int i = 0; i < MAX_UNITS; ++i)
                    if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 0 &&
                        u[i].selected)
                        fprintf(stderr,"P6 team order slot=%d at=%d,%d order=%d target=%d,%d path=%d,%d hp=%d\n",
                                i,u[i].x,u[i].y,u[i].order,u[i].tx,u[i].ty,
                                u[i].path_x,u[i].path_y,u[i].hp);
                marine_team_phase = ordered ? 1 : 3;
            } else if (marine_team_phase == 1) {
                if(!team_west_ordered){
                    int left,top,right,bottom;
                    int candidates=western_marine_bounds(&left,&top,
                                                          &right,&bottom);
                    int selected=candidates?select_box_group(left-4,top-4,
                        right+4,bottom+4):0;
                    int marines=0,other=0;
                    for(int i=0;i<MAX_UNITS;++i)
                        if(u[i].alive&&u[i].owner==SIDE&&u[i].selected){
                            if(u[i].raw_type==0)++marines;else ++other;
                        }
                    int ordered=candidates>=4&&selected>=4&&marines>=4&&
                        !other&&issue_selected_order(marine_stage_x,
                                                     marine_stage_y,0);
                    team_west_ordered=1;
                    fprintf(stderr,"P6 west Marine transit frame=%u candidates=%d selected=%d marines=%d other=%d ordered=%d target=%d,%d host=%u\n",
                            (unsigned)frame,candidates,selected,marines,
                            other,ordered,marine_stage_x,marine_stage_y,
                            host_ticks);
                }
                int arrived = eastern_marine_count_near(marine_stage_x,
                                                        marine_stage_y, 96);
                if (east_arrived >= 6 && tassadar >= 0 &&
                    issue_actor_move(tassadar, marine_stage_x - 48,
                                     marine_stage_y)) {
                    marine_team_phase = 2;
                    fprintf(stderr, "P6 team staging frame=%u marines=%d/%d Tassadar=%d,%d move=%d,%d host=%u\n",
                            (unsigned)frame, arrived, raw_count(0, SIDE),
                            u[tassadar].x,u[tassadar].y,
                            marine_stage_x - 48,marine_stage_y,host_ticks);
                }
            } else if (marine_team_phase == 2) {
                int arrived = eastern_marine_count_near(marine_stage_x,
                                                        marine_stage_y, 112);
                if (tassadar >= 0 &&
                    dist2(u[tassadar].x,u[tassadar].y,
                          marine_stage_x-48,marine_stage_y) <= 96*96 &&
                    arrived >= 6) {
                    int left,top,right,bottom;
                    int candidates=marine_bounds_near(marine_stage_x,
                        marine_stage_y,112,&left,&top,&right,&bottom);
                    int selected=select_box_group(left-8,top-8,right+8,bottom+8);
                    int marines=0;
                    for(int i=0;i<MAX_UNITS;++i)
                        if(u[i].alive&&u[i].owner==SIDE&&u[i].selected&&
                           u[i].raw_type==0)++marines;
                    int ordered=candidates>=6&&marines==candidates&&
                        issue_selected_order(marine_front_x,marine_front_y,0);
                    fprintf(stderr,"P6 team escort commit frame=%u selected=%d/%d ordered=%d corridor=%d,%d Tassadar=%d,%d marines-near=%d host=%u\n",
                            (unsigned)frame,marines,candidates,ordered,
                            marine_front_x,marine_front_y,u[tassadar].x,
                            u[tassadar].y,arrived,host_ticks);
                    marine_team_phase=3;
                    team_escort_commit_frame=(int)frame;
                }
            }
            if (marine_team_phase < 3) {
                if (path_diagnostics && frame % 120 == 0) {
                    int arrived=eastern_marine_count_near(marine_stage_x,
                                                          marine_stage_y,96);
                    fprintf(stderr,"P6 team approach frame=%u phase=%d east=%d all=%d/%d T=%d,%d order=%d goal=%d,%d hp=%d shield=%d energy=%d host=%u\n",
                            (unsigned)frame,marine_team_phase,east_arrived,arrived,
                            raw_count(0,SIDE),tassadar>=0?u[tassadar].x:-1,
                            tassadar>=0?u[tassadar].y:-1,
                            tassadar>=0?u[tassadar].order:-1,
                            tassadar>=0?u[tassadar].tx:-1,
                            tassadar>=0?u[tassadar].ty:-1,
                            tassadar>=0?u[tassadar].hp:0,
                            tassadar>=0?u[tassadar].shield:0,
                            tassadar>=0?u[tassadar].carry:-1,host_ticks);
                }
                step(0);
                continue;
            }
        }
        /* A close-front offset cast must stay planted through pulse three:
         * the modeled spell deals 14 damage per pulse, so 35-HP Zerglings
         * need at least three hits. Moving early can pull survivors out. */
        if (waiting_self_storm_frame && tassadar >= 0 &&
            u[tassadar].alive) {
            unsigned hold_until = waiting_self_storm_frame +
                SC64_STORM_FIRST_PULSE_TICKS +
                2u * SC64_STORM_PULSE_INTERVAL_TICKS;
            if ((unsigned)frame < hold_until) {
                if (path_diagnostics && (unsigned)frame % 10 == 0)
                    fprintf(stderr,
                        "P6 hold self-Storm frame=%u age=%u at=%d,%d hp=%d shield=%d energy=%d\n",
                        (unsigned)frame,(unsigned)frame-waiting_self_storm_frame,
                        u[tassadar].x,u[tassadar].y,u[tassadar].hp,
                        u[tassadar].shield,u[tassadar].carry);
                step(0);
                continue;
            }
            int rx=waiting_self_retreat_x,ry=waiting_self_retreat_y;
            int have=waiting_self_retreat_valid ||
                storm_south_retreat_waypoint(tassadar,&rx,&ry);
            int issued=have&&
                issue_selected_actor_move(tassadar,rx,ry);
            if (issued) {
                waiting_for_storm=1;
                waiting_at_south_safe=1;
                south_safe_x=rx;south_safe_y=ry;
                next_storm_frame=frame+1;
                next_order_frame=frame+80;
            }
            if (path_diagnostics)
                fprintf(stderr,
                    "P6 post-defensive-self-Storm retreat frame=%u age=%u issued=%d at=%d,%d to=%d,%d hp=%d shield=%d\n",
                    (unsigned)frame,(unsigned)frame-waiting_self_storm_frame,
                    issued,u[tassadar].x,u[tassadar].y,rx,ry,
                    u[tassadar].hp,u[tassadar].shield);
            waiting_self_storm_frame=0;
            waiting_self_retreat_valid=0;
            if (issued) { step(0); continue; }
        }
        /* Bank the full reserve at a quiet cleared position before the
         * southern extraction lane. This runs independently of whether
         * Zealots are currently alive/selected, and cancels if pursuit starts. */
        if (tassadar >= 0 && waiting_for_full_south_charge &&
            nearby_hostile_sources(tassadar, 240)) {
            waiting_for_full_south_charge = 0;
            waiting_for_storm = 0;
            waiting_at_south_safe = 0;
            if (path_diagnostics)
                fprintf(stderr,
                    "P6 full reserve cancelled frame=%u at=%d,%d nearest hostile within240\n",
                    (unsigned)frame, u[tassadar].x, u[tassadar].y);
        }
        if (tassadar >= 0 && !waiting_for_full_south_charge &&
            !waiting_for_storm && !waiting_at_south_safe &&
            !waiting_at_east_shelf && u[tassadar].y > 1400 &&
            u[tassadar].y < 1750 &&
            (u[tassadar].carry < 250 * 256 || u[tassadar].shield < 300) &&
            !nearby_hostile_sources(tassadar, 240)) {
            waiting_for_full_south_charge = 1;
            waiting_for_storm = 1;
            waiting_at_south_safe = 1;
            south_safe_x = u[tassadar].x;
            south_safe_y = u[tassadar].y;
            if (path_diagnostics)
                fprintf(stderr,
                    "P6 full reserve hold frame=%u at=%d,%d hp=%d shield=%d energy_q8=%d gates_pending=%d\n",
                    (unsigned)frame, u[tassadar].x, u[tassadar].y,
                    u[tassadar].hp, u[tassadar].shield, u[tassadar].carry,
                    gate_pending);
        }
        if (tassadar >= 0 && waiting_for_full_south_charge) {
            if (u[tassadar].carry >= 250 * 256 && u[tassadar].shield >= 300) {
                waiting_for_full_south_charge = 0;
                waiting_for_storm = 0;
                waiting_at_south_safe = 0;
                if (path_diagnostics)
                    fprintf(stderr,
                        "P6 full reserve ready frame=%u at=%d,%d energy_q8=%d shield=%d\n",
                        (unsigned)frame, u[tassadar].x, u[tassadar].y,
                        u[tassadar].carry, u[tassadar].shield);
            } else {
                if (u[tassadar].order != ORDER_IDLE) (void)stop_actor(tassadar);
                if (path_diagnostics && frame % 300 == 0)
                    fprintf(stderr,
                        "P6 full reserve charging frame=%u at=%d,%d hp=%d shield=%d energy_q8=%d\n",
                        (unsigned)frame, u[tassadar].x, u[tassadar].y,
                        u[tassadar].hp, u[tassadar].shield, u[tassadar].carry);
                step(0);
                continue;
            }
        }
        /* Keep the nine-Marine force on the southern corridor, then bring
         * fully charged Tassadar to firing range of the actual frontline
         * Hydralisk before allowing the solo waypoint policy to take over. */
        if (team_escort && marine_team_phase == 3 &&
            !team_front_action_issued && tassadar >= 0 &&
            u[tassadar].carry >= 250 * 256 && u[tassadar].shield >= 300 &&
            eastern_marine_count_near(marine_front_x, marine_front_y, 112) >= 5) {
            int nearby=eastern_marine_count_near(marine_front_x,
                                                 marine_front_y,112);
            /* The group holds this reachable corridor shelf. Select Tassadar
             * first, then read the actual target again after cursor travel;
             * a moving Hydralisk must not make the support point stale. */
            int hero_selected=select_actor_only(tassadar);
            int target=-1,nearest=0x7fffffff;
            if(hero_selected)for(int i=0;i<MAX_UNITS;++i)
                if(u[i].alive&&u[i].raw_type==38&&
                   race_p6_owners_hostile(SIDE,u[i].owner)){
                    int d=dist2(marine_front_x,marine_front_y,u[i].x,u[i].y);
                    if(d<nearest){nearest=d;target=i;}
                }
            if(nearest>200*200)target=-1;
            static const int support_offsets[][2]={{96,-96},{128,-64},
                {64,-128},{128,0},{96,96},{64,128},{0,128},{-128,-64},
                {-96,-96},{-128,0},{-64,128},{-96,96},{0,-128},
                {64,-64},{-64,-64}};
            int sx=-1,sy=-1;
            int support_route=0,route_next_x=-1,route_next_y=-1;
            if(target>=0)for(unsigned k=0;
                k<sizeof(support_offsets)/sizeof(support_offsets[0]);++k){
                int x=u[target].x+support_offsets[k][0];
                int y=u[target].y+support_offsets[k][1];
                int col=x/16,row=y/16;
                if(x>=0&&x<WORLD_W&&y>=0&&y<WORLD_H&&
                   !terrain_blocked(col,row)&&
                   SC64_GRID_GET(path_walkable,col+row*MAP_COLS)&&
                   dist2(x,y,u[target].x,u[target].y)<=
                       SC64_STORM_CAST_RANGE_GBA_PX*
                       SC64_STORM_CAST_RANGE_GBA_PX&&
                   terrain_only_route(u[tassadar].x,u[tassadar].y,x,y,
                       &route_next_x,&route_next_y,1)){
                    sx=x;sy=y;support_route=1;break;
                }
            }
            int tassadar_ordered=support_route&&
                issue_selected_actor_move(tassadar,sx,sy);
            team_front_action_issued=1;
            team_support_order_frame=(unsigned)frame;
            team_support_x=sx;team_support_y=sy;
            next_order_frame=frame+10000;
            if(path_diagnostics)
                fprintf(stderr,"P6 team frontline engage frame=%u marines-holding=%d hero-selected=%d Torder=%d hydra=%d at=%d,%d d2=%d T=%d,%d support=%d,%d route=%d host=%u\n",
                        (unsigned)frame,nearby,hero_selected,tassadar_ordered,
                        target,
                        target>=0?u[target].x:-1,target>=0?u[target].y:-1,
                        nearest,tassadar>=0?u[tassadar].x:-1,
                        tassadar>=0?u[tassadar].y:-1,sx,sy,support_route,
                        host_ticks);
            if(path_diagnostics)fprintf(stderr,"P6 team support route=%d next=%d,%d target-walk=%d terrain=%d\n",
                support_route,route_next_x,route_next_y,
                sx>=0?SC64_GRID_GET(path_walkable,sx/16+(sy/16)*MAP_COLS):0,
                sx>=0?terrain_blocked(sx/16,sy/16):1);
            step(0);
            continue;
        }
        if(team_escort&&team_front_action_issued==1&&tassadar>=0){
            if((unsigned)frame>team_support_order_frame+1800u){
                team_front_action_issued=3;
                next_order_frame=frame+1;
                if(path_diagnostics)fprintf(stderr,
                    "P6 team support timeout frame=%u at=%d,%d order=%d target=%d,%d retry=%u\n",
                    (unsigned)frame,u[tassadar].x,u[tassadar].y,
                    u[tassadar].order,u[tassadar].tx,u[tassadar].ty,
                    u[tassadar].path_retry);
            }else if(dist2(u[tassadar].x,u[tassadar].y,
                           team_support_x,team_support_y)>24*24||
                     u[tassadar].order!=ORDER_IDLE){
                step(0);
                continue;
            }else{
                int target=-1,nearest=0x7fffffff,tx=-1,ty=-1;
                for(int i=0;i<MAX_UNITS;++i)
                    if(i!=tassadar&&u[i].alive&&
                       race_p6_owners_hostile(SIDE,u[i].owner)){
                        int d=dist2(u[tassadar].x,u[tassadar].y,
                                    u[i].x,u[i].y);
                        if(d<nearest){nearest=d;target=i;}
                    }
                if(target>=0&&nearest<=
                   SC64_STORM_CAST_RANGE_GBA_PX*
                   SC64_STORM_CAST_RANGE_GBA_PX){
                    tx=u[target].x;ty=u[target].y;
                    int covered=storm_covers_point(tx,ty,2);
                    int cast=!covered&&cast_storm_at_cluster(tassadar,tx,ty,
                                                              0,1);
                    if(path_diagnostics)fprintf(stderr,
                        "P6 team Storm frame=%u target=%d raw=%d at=%d,%d d2=%d covered=%d cast=%d caster=%d,%d hp=%d shield=%d energy=%d\n",
                        (unsigned)frame,target,u[target].raw_type,tx,ty,
                        nearest,covered,cast,u[tassadar].x,u[tassadar].y,
                        u[tassadar].hp,u[tassadar].shield,u[tassadar].carry);
                    if(cast){
                        ++team_storm_casts;
                        team_front_action_issued=2;
                        team_storm_wait_until=(unsigned)frame+
                            SC64_STORM_FIRST_PULSE_TICKS+
                            (SC64_STORM_PULSE_COUNT-1)*
                                SC64_STORM_PULSE_INTERVAL_TICKS;
                        step(0);
                        continue;
                    }
                    if(covered){
                        team_front_action_issued=2;
                        team_storm_wait_until=(unsigned)frame+10;
                        step(0);
                        continue;
                    }
                }
                team_front_action_issued=3;
                next_order_frame=frame+1;
                if(path_diagnostics)fprintf(stderr,
                    "P6 team support no-target frame=%u target=%d d2=%d at=%d,%d energy=%d\n",
                    (unsigned)frame,target,nearest,u[tassadar].x,
                    u[tassadar].y,u[tassadar].carry);
            }
        }else if(team_escort&&team_front_action_issued==2&&
                 (unsigned)frame>=team_storm_wait_until){
            team_front_action_issued=3;
            next_order_frame=frame+1;
            if(path_diagnostics)fprintf(stderr,
                "P6 team Storm finished frame=%u casts=%d at=%d,%d hp=%d shield=%d energy=%d\n",
                (unsigned)frame,team_storm_casts,tassadar>=0?u[tassadar].x:-1,
                tassadar>=0?u[tassadar].y:-1,tassadar>=0?u[tassadar].hp:0,
                tassadar>=0?u[tassadar].shield:0,
                tassadar>=0?u[tassadar].carry:-1);
            if(!team_follow_ordered){
                int left,top,right,bottom;
                int candidates=marine_bounds_near(marine_front_x,
                    marine_front_y,200,&left,&top,&right,&bottom);
                int selected=candidates>=5?select_box_group(left-8,top-8,
                    right+8,bottom+8):0;
                int marines=0,other=0;
                for(int i=0;i<MAX_UNITS;++i)
                    if(u[i].alive&&u[i].owner==SIDE&&u[i].selected){
                        if(u[i].raw_type==0)++marines;else ++other;
                    }
                int follow_x=frontline_lane_x+(team_escort?60:0);
                int ordered=candidates>=5&&marines==candidates&&!other&&
                    issue_selected_order(follow_x,
                                         frontline_lane_y,team_escort?1:0);
                team_follow_ordered=ordered;
                if(path_diagnostics)fprintf(stderr,
                    "P6 team corridor follow frame=%u selected=%d marines=%d/%d other=%d ordered=%d target=%d,%d host=%u\n",
                    (unsigned)frame,selected,marines,candidates,other,
                    ordered,follow_x,frontline_lane_y,host_ticks);
            }
        }
        if(team_escort&&team_follow_ordered&&!team_repositioned&&
           (unsigned)frame>=109500&&tassadar>=0&&u[tassadar].alive&&
           u[tassadar].x<720&&u[tassadar].y>1800){
            int selected=select_marine_group_near(frontline_lane_x,
                                                   frontline_lane_y,128);
            int ordered=selected>=5&&issue_selected_order(720,1860,0);
            team_repositioned=ordered;
            if(path_diagnostics)fprintf(stderr,
                "P6 team clear T route frame=%u selected=%d ordered=%d target=720,1860 T=%d,%d host=%u\n",
                (unsigned)frame,selected,ordered,u[tassadar].x,
                u[tassadar].y,host_ticks);
        }
        int late_retreat_active = tassadar >= 0 &&
            u[tassadar].y > 1650 && waiting_for_storm;
        /* When the next wave reaches the southern lane, put an offset field
         * between Tassadar and the approaching front. Keep him planted through
         * the modeled third pulse before retreating. */
        if(late_retreat_active&&tassadar>=0&&u[tassadar].alive&&
           u[tassadar].carry>=75*256&&
           u[tassadar].hp+u[tassadar].shield>60){
            int near=0x7fffffff,near_slot=-1;
            for(int i=0;i<MAX_UNITS;++i)
                if(i!=tassadar&&u[i].alive&&
                   race_p6_owners_hostile(SIDE,u[i].owner)&&
                   !storm_covers_point(u[i].x,u[i].y,2)){
                    int d=dist2(u[tassadar].x,u[tassadar].y,u[i].x,u[i].y);
                    if(d<near){near=d;near_slot=i;}
                }
            if(near_slot>=0&&near<=24*24&&
               !storm_covers_point(u[near_slot].x,u[near_slot].y,2)&&
               frame>=next_storm_frame){
                int cast=cast_close_pack_storm(tassadar);
                if(cast){
                    ++tassadar_storm_casts;
                    ++frontline_storm_casts;
                    waiting_self_storm_frame=frame;
                    next_storm_frame=frame+
                        SC64_STORM_FIRST_PULSE_TICKS+
                        SC64_STORM_PULSE_INTERVAL_TICKS;
                    waiting_self_retreat_valid=
                        storm_south_retreat_waypoint(tassadar,
                            &waiting_self_retreat_x,&waiting_self_retreat_y);
                    /* The cursor is free after the cast. Move it toward the
                     * retreat point during the pulse window so the later
                     * ordinary move command does not spend another ~30 live
                     * ticks walking the cursor while melee continues. */
                    if(waiting_self_retreat_valid)
                        move_cursor(waiting_self_retreat_x,
                                    waiting_self_retreat_y);
                    if(path_diagnostics)fprintf(stderr,
                        "P6 waiting defensive self-Storm frame=%u nearest=%d raw=%d at=%d,%d d2=%d center=%d,%d retreat=%d,%d valid=%d hp=%d shield=%d\n",
                        (unsigned)frame,near_slot,u[near_slot].raw_type,
                        u[near_slot].x,u[near_slot].y,
                        near,cx,cy,waiting_self_retreat_x,
                        waiting_self_retreat_y,waiting_self_retreat_valid,
                        u[tassadar].hp,u[tassadar].shield);
                    step(0);
                    continue;
                }
            }
        }
        if(tassadar>=0&&!gate_pending&&!frontline_pursuit&&
           !waiting_for_storm&&u[tassadar].y>1650&&
           u[tassadar].carry>=75*256)
            (void)preposition_frontline_cursor(tassadar);
        if(tassadar>=0&&u[tassadar].y>1650&&!frontline_pursuit&&
           !waiting_for_storm&&u[tassadar].carry>=75*256&&
           frame>=next_storm_frame&&
           dist2(u[tassadar].x,u[tassadar].y,
                 frontline_lane_x,frontline_lane_y)<=24*24){
            (void)stage_frontline_move(tassadar);
        }
        if(frontline_pursuit){
            if(tassadar<0||!u[tassadar].alive){
                frontline_pursuit=0;
            }else{
                /* Once a self-centered field is down, leave the cursor on
                 * its prepositioned retreat point. Re-tracking the enemy
                 * would undo that preparation and add another live cursor
                 * walk until pulse three has killed 35-HP frontline units. */
                int cursor_moved=frontline_self_storm_frame?0:
                    track_frontline_cursor(tassadar);
                if(cursor_moved)step(0);
                int tx=0,ty=0,raw=-1,owner=-1,slot=-1,is_building=0;
                int have_front=frontline_target(tassadar,&tx,&ty,&raw,&owner,
                                                &slot,&is_building);
                if(!have_front||dist2(u[tassadar].x,u[tassadar].y,tx,ty)>640*640){
                    frontline_pursuit=0;
                    if(team_escort)next_order_frame=frame+1;
                }
                if(frontline_pursuit&&frontline_self_storm_frame&&
                   (unsigned)frame>=frontline_self_storm_frame+
                         SC64_STORM_FIRST_PULSE_TICKS+
                         2u*SC64_STORM_PULSE_INTERVAL_TICKS){
                    int rx=-1,ry=-1;
                    if(storm_south_retreat_waypoint(tassadar,&rx,&ry)&&
                       issue_selected_actor_move(tassadar,rx,ry)){
                        frontline_pursuit=0;
                        waiting_for_storm=1;
                        waiting_at_south_safe=1;
                        south_safe_x=rx;south_safe_y=ry;
                        next_storm_frame=frame+1;
                        next_order_frame=frame+80;
                        if(path_diagnostics)fprintf(stderr,
                            "P6 post-self-Storm retreat frame=%u storm_age=%u at=%d,%d hp=%d shield=%d energy=%d target=%d,%d\n",
                            (unsigned)frame,frame-frontline_self_storm_frame,
                            u[tassadar].x,u[tassadar].y,u[tassadar].hp,
                            u[tassadar].shield,u[tassadar].carry,rx,ry);
                    }
                    frontline_self_storm_frame=0;
                }
                if(frontline_pursuit&&have_front&&
                   u[tassadar].order==ORDER_IDLE&&
                   dist2(u[tassadar].x,u[tassadar].y,tx,ty)>144*144&&
                   dist2(u[tassadar].x,u[tassadar].y,tx,ty)<=640*640&&
                   dist2(cx,cy,tx,ty)<=24*24&&
                   !is_building){
                    int issued=issue_frontline_safe_waypoint(tassadar,tx,ty);
                    if(path_diagnostics)fprintf(stderr,"P6 frontline safe-waypoint frame=%u issued=%d target=%d,%d raw=%d owner=%d at=%d,%d caster=%d,%d menu=%d mode=%d host=%u\n",
                        (unsigned)frame,issued,tx,ty,raw,owner,
                        is_building?-1:u[slot].x,is_building?-1:u[slot].y,
                        u[tassadar].x,u[tassadar].y,menu_open,mode,host_ticks);
                }
                int wait_for_close_pack=frontline_pursuit&&
                    frame>=next_storm_frame&&
                    late_wait_for_close_pack(tassadar);
                if(wait_for_close_pack){
                    /* Keep the staged cursor near the front. Walking it back
                     * to Tassadar here wastes live ticks while Zerglings close. */
                    next_storm_frame=frame+1;
                    if(path_diagnostics&&frame%10==0){
                        int near=0x7fffffff,ni=-1;
                        for(int i=0;i<MAX_UNITS;++i)
                            if(i!=tassadar&&u[i].alive&&
                               race_p6_owners_hostile(SIDE,u[i].owner)){
                                int d=dist2(u[tassadar].x,u[tassadar].y,
                                            u[i].x,u[i].y);
                                if(d<near){near=d;ni=i;}
                            }
                        fprintf(stderr,
                            "P6 deferred first cast for close pack frame=%u hero=%d,%d nearest=%d raw=%d at=%d,%d d2=%d cursor=%d,%d\n",
                            (unsigned)frame,u[tassadar].x,u[tassadar].y,ni,
                            ni>=0?u[ni].raw_type:-1,ni>=0?u[ni].x:-1,
                            ni>=0?u[ni].y:-1,near,cx,cy);
                    }
                }
                int force_close_pack=frontline_pursuit&&
                    late_close_pack_ready(tassadar);
                int keep_team_reserve=team_escort&&
                    frontline_storm_casts>=1&&tassadar>=0&&
                    u[tassadar].carry<150*256;
                int cast_succeeded=0;
                if(path_diagnostics&&frame>=109240&&frame<=109305){
                    int nd=0x7fffffff,ns=-1;
                    for(int z=0;z<MAX_UNITS;++z)
                        if(z!=tassadar&&u[z].alive&&
                           race_p6_owners_hostile(SIDE,u[z].owner)){
                            int d=dist2(u[tassadar].x,u[tassadar].y,u[z].x,u[z].y);
                            if(d<nd){nd=d;ns=z;}
                        }
                    fprintf(stderr,"P6 close gate frame=%u next=%u pursuit=%d wait=%d force=%d near=%d slot=%d at=%d,%d cursor=%d,%d mode=%d selected=%d fieldnear=%d storms=",
                        (unsigned)frame,(unsigned)next_storm_frame,frontline_pursuit,
                        wait_for_close_pack,force_close_pack,nd,ns,
                        ns>=0?u[ns].x:-1,ns>=0?u[ns].y:-1,cx,cy,mode,
                        u[tassadar].selected&&selected_count()==1,
                        storm_field_near_point(cx,cy,48));
                    for(unsigned f=0;f<SC64_STORM_MAX_ACTIVE;++f){
                        const Sc64PsionicStorm *e=&race_p6.storm.storm[f];
                        if(e->active)fprintf(stderr,"[%u:%u,%u,a%u,p%u]",f,e->x,e->y,e->age_ticks,e->pulses_left);
                    }
                    fputc('\n',stderr);
                }
                if(frontline_pursuit&&
                   (frame>=next_storm_frame||force_close_pack)&&
                   !keep_team_reserve&&
                   !wait_for_close_pack&&
                   !(u[tassadar].y>1650&&
                     storm_field_near_point(cx,cy,48))){
                    if(force_close_pack)
                        cast_succeeded=cast_close_pack_storm(tassadar);
                    else
                        cast_succeeded=cast_storm_on_staged_cursor(tassadar)||
                            cast_close_pack_storm(tassadar);
                }else if(keep_team_reserve&&path_diagnostics&&frame%20==0){
                    fprintf(stderr,"P6 team reserve wait frame=%u casts=%u energy=%d fieldnear=%d\n",
                        (unsigned)frame,frontline_storm_casts,
                        tassadar>=0?u[tassadar].carry:-1,
                        storm_field_near_point(cx,cy,48));
                }
                if(cast_succeeded){
                    ++tassadar_storm_casts;
                    ++frontline_storm_casts;
                    next_storm_frame=frame+
                        (frontline_storm_casts == 1 ? 10 : 30);
                    next_order_frame=frame+1;
                    if(path_diagnostics)
                        fprintf(stderr,"P6 staged Storm success frame=%u caster=%d,%d energy=%d shield=%d\n",
                            (unsigned)frame,u[tassadar].x,u[tassadar].y,
                            u[tassadar].carry,u[tassadar].shield);
                    if(team_escort&&team_follow_ordered&&
                       !team_flank_ordered)team_flank_pending=1;
                    if(frontline_self_storm_frame&&u[tassadar].y>1650&&
                       (unsigned)frame-frontline_self_storm_frame<=2){
                        int rx=-1,ry=-1;
                        if(storm_south_retreat_waypoint(tassadar,&rx,&ry)){
                            move_cursor(rx,ry);
                            if(path_diagnostics)
                                fprintf(stderr,
                                    "P6 prepositioned self-Storm retreat frame=%u age=%u cursor=%d,%d to=%d,%d hp=%d shield=%d\n",
                                    (unsigned)frame,
                                    (unsigned)frame-frontline_self_storm_frame,
                                    cx,cy,rx,ry,u[tassadar].hp,u[tassadar].shield);
                        }
                    }
                    int nearest_d2=0x7fffffff,near_slot=-1;
                    for(int i=0;i<MAX_UNITS;++i)
                        if(i!=tassadar&&u[i].alive&&
                           race_p6_owners_hostile(SIDE,u[i].owner)){
                            int d=dist2(u[tassadar].x,u[tassadar].y,
                                        u[i].x,u[i].y);
                            if(d<nearest_d2){nearest_d2=d;near_slot=i;}
                        }
                    int covered_front=near_slot>=0&&
                        storm_maturely_covers_point(u[near_slot].x,
                                                    u[near_slot].y);
                    if(u[tassadar].carry<75*256||
                       (u[tassadar].y>1650&&nearest_d2<=24*24&&
                        covered_front)){
                        int safe_x=u[tassadar].x,safe_y=u[tassadar].y;
                        if(nearby_unburrowed_hostile_units(tassadar,160)){
                            int rx=-1,ry=-1,found=0;
                            if(u[tassadar].y>1650)
                                found=storm_south_retreat_waypoint(tassadar,&rx,&ry);
                            if(!found)found=storm_quick_retreat(tassadar,&rx,&ry);
                            if(found){
                                int issued=issue_selected_actor_move(tassadar,rx,ry);
                                if(issued){safe_x=rx;safe_y=ry;}
                                if(path_diagnostics)fprintf(stderr,"P6 staged Storm quick retreat frame=%u issued=%d from=%d,%d to=%d,%d shield=%d\n",
                                    (unsigned)frame,issued,u[tassadar].x,u[tassadar].y,
                                    rx,ry,u[tassadar].shield);
                            }
                        }
                        waiting_for_storm=1;
                        waiting_at_south_safe=1;
                        south_safe_x=safe_x;south_safe_y=safe_y;
                        if(path_diagnostics)fprintf(stderr,"P6 staged Storm local recharge frame=%u at=%d,%d energy=%d\n",
                            (unsigned)frame,safe_x,safe_y,u[tassadar].carry);
                    }
                    if(team_flank_pending&&tassadar>=0&&u[tassadar].alive&&
                       (u[tassadar].order==ORDER_MOVE||waiting_for_storm)){
                        int left,top,right,bottom;
                        int candidates=marine_bounds_near(frontline_lane_x,
                            frontline_lane_y,128,&left,&top,&right,&bottom);
                        int selected=candidates>=5?select_box_group(left-32,
                            top-32,right+32,bottom+32):0;
                        int marines=0,other=0;
                        for(int i=0;i<MAX_UNITS;++i)
                            if(u[i].alive&&u[i].owner==SIDE&&u[i].selected){
                                if(u[i].raw_type==0)++marines;else ++other;
                            }
                        int tx=frontline_lane_x+40,ty=frontline_lane_y-8;
                        int ordered=candidates>=5&&selected>=5&&marines>=5&&!other&&
                            issue_selected_order(tx,ty,0);
                        team_flank_ordered=ordered;
                        team_flank_pending=0;
                        if(path_diagnostics)fprintf(stderr,
                            "P6 team flank order frame=%u selected=%d marines=%d/%d other=%d ordered=%d target=%d,%d host=%u\n",
                            (unsigned)frame,selected,marines,candidates,other,
                            ordered,tx,ty,host_ticks);
                    }
                } else if(frontline_pursuit) {
                    int rx=-1,ry=-1,threat=-1,threat_d2=0;
                    if(frontline_storm_casts>=2&&
                       covered_frontline_retreat_waypoint(tassadar,&rx,&ry,
                                                         &threat,&threat_d2)&&
                       issue_selected_actor_move(tassadar,rx,ry)){
                        frontline_pursuit=0;
                        waiting_for_storm=1;
                        waiting_at_south_safe=1;
                        south_safe_x=rx;south_safe_y=ry;
                        next_storm_frame=frame+90;
                        next_order_frame=frame+80;
                        if(path_diagnostics)fprintf(stderr,
                            "P6 retreat from covered frontline frame=%u hostile=%d raw=%d at=%d,%d d2=%d shield=%d energy=%d move=%d,%d\n",
                            (unsigned)frame,threat,u[threat].raw_type,
                            u[threat].x,u[threat].y,threat_d2,
                            u[tassadar].shield,u[tassadar].carry,rx,ry);
                    }
                    /* Preserve the live cursor-on-front order until the cast
                     * boundary; waypoint logic must not redirect it. */
                    else next_order_frame=frame+10000;
                }
                if(path_diagnostics&&frame%60==0){
                    int near=0x7fffffff,ni=-1;
                    int nx=-1,ny=-1,route=-1;
                    for(int i=0;i<MAX_UNITS;++i)if(i!=tassadar&&u[i].alive&&
                        race_p6_owners_hostile(SIDE,u[i].owner)){
                        int d=dist2(u[tassadar].x,u[tassadar].y,u[i].x,u[i].y);
                        if(d<near){near=d;ni=i;}
                    }
                    if(slot>=0&&!is_building)
                        route=sc64_route_next(u[tassadar].x,u[tassadar].y,
                                              tx,ty,&nx,&ny);
                    fprintf(stderr,"P6 frontline wait frame=%u host=%u menu=%d mode=%d at=%d,%d cursor=%d,%d cursor_d2=%d order=%d tx=%d,%d path=%d,%d retry=%u route=%d next=%d,%d targetterrain=%d targetwalk=%d energy=%d nearest=%d raw=%d at=%d,%d d2=%d burrow=%d order=%d hp=%d shield=%d\n",
                        (unsigned)frame,host_ticks,menu_open,mode,
                        u[tassadar].x,u[tassadar].y,cx,cy,
                        dist2(cx,cy,u[tassadar].x,u[tassadar].y),u[tassadar].order,
                        u[tassadar].tx,u[tassadar].ty,u[tassadar].path_x,
                        u[tassadar].path_y,u[tassadar].path_retry,route,nx,ny,
                        terrain_blocked(tx/16,ty/16),
                        SC64_GRID_GET(path_walkable,tx/16+(ty/16)*MAP_COLS),
                        u[tassadar].carry,ni,ni>=0?u[ni].raw_type:-1,
                        ni>=0?u[ni].x:-1,ni>=0?u[ni].y:-1,near,
                        ni>=0?race_p6_combat_unit_burrowed(ni):0,
                        ni>=0?u[ni].order:-1,
                        u[tassadar].hp,u[tassadar].shield);
                }
            }
        }
        if (tassadar >= 0 && !late_retreat_active &&
            !frontline_pursuit && frame >= next_storm_frame &&
            u[tassadar].carry >= 75 * 256 &&
            !(u[tassadar].y > 1650 && !gate_pending)) {
            int storm_x = -1, storm_y = -1;
            int late_approach = u[tassadar].y > 1650;
            int allow_single = !gate_pending || late_approach;
            if (storm_cluster_target(tassadar, army_x, army_y, army_n > 0,
                                     allow_single,
                                     &storm_x, &storm_y)) {
                if(late_approach&&storm_field_near_point(storm_x,storm_y,48)){
                    next_storm_frame=frame+20;
                    if(path_diagnostics&&frame%20==0)
                        fprintf(stderr,
                            "P6 skipped near-active Storm frame=%u target=%d,%d energy=%d\n",
                            (unsigned)frame,storm_x,storm_y,u[tassadar].carry);
                    continue;
                }
                u32 cast_frame = frame;
                int cast = cast_storm_at_cluster(tassadar, storm_x, storm_y,
                                                 army_n > 0,allow_single);
                if (path_diagnostics)
                    fprintf(stderr, "P6 Storm input frame=%u target=%d,%d cast=%d caster=%d,%d hp=%d energy=%d\n",
                            cast_frame, storm_x, storm_y, cast,
                            u[tassadar].alive ? u[tassadar].x : -1,
                            u[tassadar].alive ? u[tassadar].y : -1,
                            u[tassadar].alive ? u[tassadar].hp : 0,
                            u[tassadar].alive ? u[tassadar].carry : -1);
                if (cast && tassadar >= 0 && u[tassadar].alive) {
                    ++tassadar_storm_casts;
                    /* A cast can leave Tassadar in range of the same pack
                     * while cursor travel and menu input consumed live
                     * frames. Kite immediately from any nearby pack, not
                     * only after reaching the southern corridor. */
                    int south_kite = u[tassadar].y > 1650 &&
                        nearby_unburrowed_hostile_units(tassadar, 160) &&
                        dist2(u[tassadar].x, u[tassadar].y,
                              storm_x, storm_y) <= 240 * 240;
                    if (south_kite || (u[tassadar].shield < 80 &&
                        u[tassadar].x > 900 && u[tassadar].y > 1100)) {
                        /* Let the Storm resolve while retreating from the
                         * current pack instead of waiting in its firing lane. */
                        if (south_kite) {
                            int found = storm_south_retreat_waypoint(tassadar,
                                &south_safe_x, &south_safe_y);
                            waiting_at_south_safe = found;
                            if (!found) south_kite = 0;
                        }
                        if (!south_kite && u[tassadar].x > 900 &&
                            u[tassadar].y > 1100)
                            waiting_at_east_shelf=1;
                        int retreat_x = south_kite ? south_safe_x :
                                        waiting_at_east_shelf ? 1704 : 315;
                        int retreat_y = south_kite ? south_safe_y :
                                        waiting_at_east_shelf ? 900 : 235;
                        int retreat_cmd_x = retreat_x, retreat_cmd_y = retreat_y;
                        (void)route_point_ahead(u[tassadar].x,u[tassadar].y,
                                                retreat_x,retreat_y,6,
                                                &retreat_cmd_x,&retreat_cmd_y);
                        int retreat_issued = issue_selected_actor_move(tassadar,
                            retreat_cmd_x,retreat_cmd_y);
                        waiting_for_storm=1;
                        if(path_diagnostics)
                            fprintf(stderr,"P6 post-Storm retreat frame=%u at=%d,%d target=%d,%d sent=%d order=%d hp=%d shield=%d energy=%d\n",
                                (unsigned)frame,u[tassadar].x,u[tassadar].y,
                                retreat_x,retreat_y,retreat_issued,u[tassadar].order,
                                u[tassadar].hp,u[tassadar].shield,u[tassadar].carry);
                    }
                    next_order_frame = frame + SC64_STORM_FIRST_PULSE_TICKS +
                        (SC64_STORM_PULSE_COUNT - 1) *
                        SC64_STORM_PULSE_INTERVAL_TICKS;
                } else if (storm_self_splash_rejected && tassadar >= 0 &&
                           u[tassadar].alive) {
                    /* The live caster moved into its own area while the
                     * ordinary target cursor was being positioned. Back out
                     * instead of continuing to close on the same pack. */
                    if(u[tassadar].x>900&&u[tassadar].y>1100)
                        waiting_at_east_shelf=1;
                    (void)issue_actor_move(tassadar,
                        waiting_at_east_shelf?1704:315,
                        waiting_at_east_shelf?900:235);
                    waiting_for_storm=1;
                    next_order_frame=frame+SC64_STORM_FIRST_PULSE_TICKS+
                        (SC64_STORM_PULSE_COUNT-1)*
                        SC64_STORM_PULSE_INTERVAL_TICKS;
                }
                next_storm_frame = u[tassadar].y > 1650 ?
                    frame + (cast ? 100 : 1) : frame + (cast ? 300 : 90);
            } else {
                next_storm_frame = late_approach ? frame + 1 : frame + 90;
                if (late_approach) prepare_storm_frontline(tassadar);
            }
        }
        if (frame >= next_order_frame) {
            if (zeratul_rescue >= 0 && army_n &&
                dist2(army_x, army_y, u[zeratul_rescue].x, u[zeratul_rescue].y) > 64*64) {
                int gate = -1, tx = -1, ty = -1;
                for (int j = 0; j < 6; ++j)
                    if (!sc64_p6_trigger_started(&race_p6.runtime.model,
                                                  gate_trigger[j])) { gate = j; break; }
                if (path_diagnostics) {
                    int nx = -1, ny = -1;
                    int reachable = terrain_only_route(army_x, army_y,
                        u[zeratul_rescue].x, u[zeratul_rescue].y, &nx, &ny, 0);
                    int path_nx = -1, path_ny = -1;
                    int path_reachable = terrain_only_route(army_x, army_y,
                        u[zeratul_rescue].x, u[zeratul_rescue].y,
                        &path_nx, &path_ny, 1);
                    fprintf(stderr, "P6 routes start=%d,%d Z=%d,%d terrain=%d nearest=%d,%d path=%d nearest=%d,%d\n",
                            army_x, army_y, u[zeratul_rescue].x,
                            u[zeratul_rescue].y, reachable, nx, ny,
                            path_reachable, path_nx, path_ny);
                }
                int target_x = u[zeratul_rescue].x, target_y = u[zeratul_rescue].y;
                int target_ready = gate < 0;
                if (gate >= 0) {
                    target_ready = location_waypoint(gate_location[gate],
                                      army_x, army_y, &tx, &ty);
                    if (target_ready) { target_x = tx; target_y = ty; }
                    if (path_diagnostics)
                        fprintf(stderr, "P6 source gate stage=%d trigger=%u location=%u point=%d,%d reachable=%d frame=%u\n",
                                gate, gate_trigger[gate], gate_location[gate],
                                tx, ty, target_ready, (unsigned)frame);
                }
                if (target_ready && path_diagnostics)
                    fprintf(stderr, "P6 group toward %s frame=%u center=%d,%d target=%d,%d\n",
                            gate < 0 ? "Zeratul" : "source gate",
                            (unsigned)frame, army_x, army_y, target_x, target_y);
                if (target_ready) {
                    int selected = select_zealot_group();
                    int ordered = selected ?
                        issue_selected_order(target_x, target_y, gate >= 0) : 0;
                    if (path_diagnostics) {
                        fprintf(stderr, "P6 zealot command frame=%u selected=%d count=%d ordered=%d target=%d,%d\n",
                                (unsigned)frame, selected, selected_count(), ordered,
                                target_x, target_y);
                        for (int k = 0; k < MAX_UNITS; ++k)
                            if (u[k].alive && u[k].owner == SIDE && u[k].raw_type == 65)
                                fprintf(stderr, "  zealot slot=%d xy=%d,%d selected=%d order=%d target=%d,%d hp=%d\n",
                                        k,u[k].x,u[k].y,u[k].selected,u[k].order,
                                        u[k].tx,u[k].ty,u[k].hp);
                    }
                    /* Keep Tassadar independently controllable, but close
                     * enough to support the opening engagement with Storm. */
                    if (ordered && gate == 0 && !tassadar_support_positioned &&
                        tassadar >= 0 && u[tassadar].x < 180 &&
                        !terrain_blocked(315 / 16, 235 / 16) &&
                        SC64_GRID_GET(path_walkable, 315 / 16 + (235 / 16) * MAP_COLS)) {
                        (void)issue_actor_move(tassadar, 315, 235);
                        tassadar_support_positioned = 1;
                    }
                }
                next_order_frame = frame + 900;
            } else if (zeratul_rescue >= 0 && !army_n && tassadar >= 0) {
                int gate = -1, tx = -1, ty = -1;
                for (int j = 0; j < 6; ++j)
                    if (!sc64_p6_trigger_started(&race_p6.runtime.model,
                                                  gate_trigger[j])) { gate = j; break; }
                if(!waiting_for_storm&&
                   (u[tassadar].shield<240||u[tassadar].carry<75*256)&&
                   u[tassadar].x>900&&u[tassadar].y>1100)
                    waiting_at_east_shelf=1;
                if(!waiting_for_storm&&!waiting_at_south_safe&&south_stage==2&&
                   (u[tassadar].shield<180||u[tassadar].carry<75*256)&&
                   u[tassadar].x<900&&u[tassadar].y>1500&&
                   u[tassadar].shield>=180&&!nearby_hostile_units(tassadar,192)){
                    waiting_at_south_safe=1;
                    south_safe_x=u[tassadar].x;south_safe_y=u[tassadar].y;
                }
                int safe_x=waiting_at_south_safe?south_safe_x:
                           waiting_at_east_shelf?1704:gate<0?u[tassadar].x:315;
                int safe_y=waiting_at_south_safe?south_safe_y:
                           waiting_at_east_shelf?900:gate<0?u[tassadar].y:235;
                int at_safe=dist2(u[tassadar].x,u[tassadar].y,safe_x,safe_y)<=64*64;
                if ((gate < 0 || waiting_for_storm) &&
                    (waiting_for_full_south_charge ?
                        (u[tassadar].carry < 250 * 256 || u[tassadar].shield < 300) :
                        (u[tassadar].carry < 75 * 256 ||
                                 (!waiting_for_storm &&
                                  u[tassadar].shield < 240) ||
                                 (waiting_for_storm &&
                                  (!at_safe || u[tassadar].shield < 230))))) {
                    waiting_for_storm = 1;
                    /* Once the source gates are open, keep a low-energy
                     * Tassadar at his current cleared western position. The
                     * old opening-base waypoint sends him across the map. */
                    if(gate<0&&!waiting_at_south_safe&&!waiting_at_east_shelf){
                        waiting_at_south_safe=1;
                        south_safe_x=u[tassadar].x;
                        south_safe_y=u[tassadar].y;
                    }
                    if (!at_safe && (u[tassadar].order != ORDER_MOVE ||
                        dist2(u[tassadar].tx,u[tassadar].ty,safe_x,safe_y)>32*32)) {
                        int safe_cmd_x=safe_x,safe_cmd_y=safe_y;
                        (void)route_point_ahead(u[tassadar].x,u[tassadar].y,
                            safe_x,safe_y,6,&safe_cmd_x,&safe_cmd_y);
                        (void)issue_actor_move(tassadar,safe_cmd_x,safe_cmd_y);
                    }
                    else if (at_safe && u[tassadar].order != ORDER_IDLE)
                        (void)stop_actor(tassadar);
                    if (path_diagnostics)
                        fprintf(stderr, "P6 retreat/recharge frame=%u at=%d,%d safe=%d,%d hp=%d shield=%d energy=%d\n",
                                (unsigned)frame, u[tassadar].x, u[tassadar].y,
                                safe_x,safe_y,u[tassadar].hp,u[tassadar].shield,
                                u[tassadar].carry);
                    next_order_frame = frame + 300;
                    tx = -2; /* Recharge away from the current defender pack. */
                } else if (waiting_for_storm && at_safe &&
                           u[tassadar].carry >=
                               (waiting_for_full_south_charge ? 250 * 256 : 75 * 256) &&
                           u[tassadar].shield >=
                               (waiting_for_full_south_charge ? 300 : 230)) {
                    waiting_for_storm = 0;
                    waiting_at_east_shelf = 0;
                    waiting_at_south_safe = 0;
                    waiting_for_full_south_charge = 0;
                    if (path_diagnostics)
                        fprintf(stderr, "P6 Storm recharged frame=%u at=%d,%d hp=%d energy=%d\n",
                                (unsigned)frame, u[tassadar].x, u[tassadar].y,
                                u[tassadar].hp, u[tassadar].carry);
                }
                if (tx != -2 && gate >= 0 && location_waypoint(gate_location[gate],
                        u[tassadar].x, u[tassadar].y, &tx, &ty)) {
                    if (path_diagnostics)
                        fprintf(stderr, "P6 solo Tassadar waypoint frame=%u target=%d,%d\n",
                                (unsigned)frame, tx, ty);
                    (void)issue_actor_move(tassadar, tx, ty);
                } else if (tx != -2 && gate < 0 &&
                           dist2(u[tassadar].x,u[tassadar].y,
                                 u[zeratul_rescue].x,u[zeratul_rescue].y)>64*64) {
                    /* After the source gates open, advance along short
                     * southern-lane waypoints. Clear the local frontier at
                     * 45 px intervals instead of walking toward a farther
                     * hostile through the nearest attacker. */
                    int target_x,target_y,command_x,command_y;
                    if(frontline_lane_x<1400){
                        target_x=frontline_lane_x;target_y=frontline_lane_y;
                    }else{
                        target_x=u[zeratul_rescue].x;
                        target_y=u[zeratul_rescue].y;
                    }
                    if(dist2(u[tassadar].x,u[tassadar].y,
                             frontline_lane_x,frontline_lane_y)<=24*24){
                        int fx,fy,fr,fo,fs,fb;
                        int have_front=frontline_target(tassadar,&fx,&fy,&fr,&fo,&fs,&fb);
                        if(!have_front&&frontline_lane_x<1400){
                            int nearest=0x7fffffff,near_slot=-1;
                            for(int k=0;k<MAX_UNITS;++k)
                                if(k!=tassadar&&u[k].alive&&
                                   race_p6_owners_hostile(SIDE,u[k].owner)){
                                    int d=dist2(u[tassadar].x,u[tassadar].y,
                                                u[k].x,u[k].y);
                                    if(d<nearest){nearest=d;near_slot=k;}
                                }
                            if(path_diagnostics)
                                fprintf(stderr,"P6 lane clear frame=%u at=%d,%d lane=%d nearest=%d raw=%d owner=%d at=%d,%d ydelta=%d\n",
                                    (unsigned)frame,u[tassadar].x,u[tassadar].y,
                                    frontline_lane_x,nearest,near_slot>=0?
                                    u[near_slot].raw_type:-1,near_slot>=0?
                                    u[near_slot].owner:-1,near_slot>=0?
                                    u[near_slot].x:-1,near_slot>=0?
                                    u[near_slot].y:-1,near_slot>=0?
                                    u[near_slot].y-frontline_lane_y:0);
                            if(u[tassadar].order!=ORDER_IDLE)(void)stop_actor(tassadar);
                            frontline_lane_x+=45;
                            if(frontline_lane_x>1400)frontline_lane_x=1400;
                            target_x=frontline_lane_x;
                        }else if(frontline_lane_x<1400){
                            /* Wait at the current safe waypoint until the
                             * nearby enemy group is removed by Storm. */
                            next_order_frame=frame+30;
                            target_x=-1;
                        }
                    }
                    if(u[tassadar].path_retry>=1){
                        int lane_d2=dist2(u[tassadar].x,u[tassadar].y,
                                          frontline_lane_x,frontline_lane_y);
                        if(lane_d2<=80*80&&
                           u[tassadar].carry>=75*256&&
                           stage_frontline_move(tassadar)){
                            target_x=-1;
                            next_order_frame=frame+1;
                            if(path_diagnostics)fprintf(stderr,
                                "P6 route retry stage-front frame=%u at=%d,%d retry=%u lane=%d,%d\n",
                                (unsigned)frame,u[tassadar].x,u[tassadar].y,
                                u[tassadar].path_retry,frontline_lane_x,
                                frontline_lane_y);
                        }else{
                            route_direct_rescue=1;
                            target_x=u[zeratul_rescue].x;
                            target_y=u[zeratul_rescue].y;
                            if(path_diagnostics&&frame%120==0)
                                fprintf(stderr,"P6 route retry fallback frame=%u at=%d,%d retry=%u direct-rescue=%d,%d\n",
                                    (unsigned)frame,u[tassadar].x,u[tassadar].y,
                                    u[tassadar].path_retry,target_x,target_y);
                        }
                    }
                    if(route_direct_rescue){
                        target_x=u[zeratul_rescue].x;
                        target_y=u[zeratul_rescue].y;
                    }
                    if(target_x>=0){
                        command_x=target_x;command_y=target_y;
                        if(dist2(u[tassadar].x,u[tassadar].y,
                                 target_x,target_y)>64*64)
                            (void)route_point_ahead(u[tassadar].x,u[tassadar].y,
                                target_x,target_y,6,&command_x,&command_y);
                        if(path_diagnostics)
                            fprintf(stderr,"P6 lane waypoint frame=%u at=%d,%d goal=%d,%d command=%d,%d next-lane=%d energy=%d\n",
                                (unsigned)frame,u[tassadar].x,u[tassadar].y,
                                target_x,target_y,command_x,command_y,
                                frontline_lane_x,u[tassadar].carry);
                        (void)issue_actor_move(tassadar,command_x,command_y);
                        next_order_frame=frame+120;
                    }
                }
                if(gate>=0)next_order_frame=frame+300;
            } else if (zeratul >= 0) {
                if (dist2(u[zeratul].x, u[zeratul].y, goal_x, goal_y) > 12*12 &&
                    (u[zeratul].order != ORDER_MOVE ||
                     dist2(u[zeratul].tx, u[zeratul].ty, goal_x, goal_y) > 12*12)) {
                    if (path_diagnostics)
                        fprintf(stderr, "P6 move Zeratul frame=%u at=%d,%d goal=%d,%d\n",
                                (unsigned)frame, u[zeratul].x, u[zeratul].y, goal_x, goal_y);
                    (void)issue_actor_move(zeratul, goal_x, goal_y);
                    next_order_frame = frame + 300;
                } else {
                    int tx = goal_x, ty = goal_y;
                    if (army_n && (dist2(army_x, army_y, tx, ty) > 96*96)) {
                        if (path_diagnostics)
                            fprintf(stderr, "P6 group objective frame=%u center=%d,%d target=%d,%d\n",
                                    (unsigned)frame, army_x, army_y, tx, ty);
                        if (select_zealot_group())
                            (void)issue_selected_order(tx, ty, 1);
                    }
                    next_order_frame = frame + 900;
                }
            } else {
                int target_x = (l + r) / 2, target_y = (top + bottom) / 2;
                if (army_n && (dist2(army_x, army_y, target_x, target_y) > 96*96)) {
                    if (path_diagnostics)
                        fprintf(stderr, "P6 rescue formation frame=%u center=%d,%d target=%d,%d\n",
                                (unsigned)frame, army_x, army_y, target_x, target_y);
                    if (select_zealot_group())
                        (void)issue_selected_order(target_x, target_y, 1);
                }
                next_order_frame = frame + 900;
            }
        }
        step(0);
    }
    if (path_diagnostics) {
        for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i)
            if (!sc64_p6_doodads[i].sprite_only &&
                sc64_p6_doodads[i].raw_type >= 205 &&
                sc64_p6_doodads[i].raw_type <= 208)
                fprintf(stderr, "P6 final prop row=%u raw=%u xy=%u,%u enabled=%u bound=%u slot=%u alive=%u hp=%d\n",
                        i, sc64_p6_doodads[i].raw_type,
                        sc64_p6_doodads[i].x, sc64_p6_doodads[i].y,
                        race_p6_doodad_row_enabled(i),
                        race_p6_doodad_slot_bound(race_p6_doodad_bindings[i].building_slot),
                        race_p6_doodad_bindings[i].building_slot,
                        race_p6_doodad_bindings[i].building_slot < MAX_BUILDINGS ?
                            b[race_p6_doodad_bindings[i].building_slot].alive : 0,
                        race_p6_doodad_bindings[i].building_slot < MAX_BUILDINGS ?
                            b[race_p6_doodad_bindings[i].building_slot].hp : 0);
        fprintf(stderr, "P6 final route Z source=%d,%d walk=%d terrain=%d\n",
                raw_unit(SC64_P6_RAW_TEMPLAR, RESCUABLE) >= 0 ?
                    u[raw_unit(SC64_P6_RAW_TEMPLAR, RESCUABLE)].x : -1,
                raw_unit(SC64_P6_RAW_TEMPLAR, RESCUABLE) >= 0 ?
                    u[raw_unit(SC64_P6_RAW_TEMPLAR, RESCUABLE)].y : -1,
                SC64_GRID_GET(path_walkable, 1536 / 16 + (1910 / 16) * MAP_COLS),
                terrain_blocked(1536 / 16, 1910 / 16));
        for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
            abs((int)b[i].x - 1544) <= 160 && abs((int)b[i].y - 1576) <= 160)
            fprintf(stderr, "P6 frontier building slot=%d raw=%d type=%d xy=%d,%d collidable=%d owner=%d\n",
                    i, b[i].raw_type, b[i].type, b[i].x, b[i].y,
                    race_p6_building_collidable(i), b[i].owner);
        fprintf(stderr, "P6 frontier local grid (P=path blocked,T=terrain blocked):\n");
        for (int row = 92; row <= 104; ++row) {
            fprintf(stderr, "%3d ", row * 16 + 8);
            for (int col = 90; col <= 102; ++col) {
                int cell = col + row * MAP_COLS;
                fputc(!SC64_GRID_GET(path_walkable, cell) ?
                      (terrain_blocked(col, row) ? '#' : 'B') : '.', stderr);
            }
            fputc('\n', stderr);
        }
        int tslot=raw_unit(SC64_P6_RAW_HERO,SIDE);
        if(tslot>=0)diagnose_terrain_route(u[tslot].x,u[tslot].y,1536,1910);
        if(tslot>=0){
            const int variants[]={0,1,2,3,4,8,16,32,24,40,56};
            for(unsigned i=0;i<sizeof(variants)/sizeof(variants[0]);++i)
                diagnose_grid_variant(variants[i],u[tslot].x,u[tslot].y,1536,1910);
        }
        FILE *grid=fopen(".local/sc64/p6-pathgrid.txt","w");
        if(grid){
            for(int row=0;row<MAP_ROWS;++row){
                for(int col=0;col<MAP_COLS;++col){
                    int cell=col+row*MAP_COLS;
                    fputc(terrain_blocked(col,row)?'#':
                          SC64_GRID_GET(path_walkable,cell)?'.':'B',grid);
                }
                fputc('\n',grid);
            }
            fclose(grid);
        }
    }
    checkpoint(won ? "won" : lost ? "lost" : "limit");
    fprintf(trace, "]}\n"); fclose(trace); trace = NULL;
    if (won && screen == SCREEN_RESULT && race_p6.runtime.model.outcome == SC64_P6_WON &&
        race_selected_mission == SC64_P6_GLOBAL_ID &&
        (race_completed & (1 << SC64_P6_GLOBAL_ID))) promote_verified_trace();
    else assert(rename(attempt_path, final_path) == 0);
    printf("P6 controller attempt result=%s frame=%u outcome=%u started=%08x trace=%s\n",
           won ? "WON" : lost ? "LOST" : "LIMIT", (unsigned)frame,
           race_p6.runtime.model.outcome, (unsigned)race_p6.runtime.model.started,
           won ? verified_path : final_path);
    return won && screen == SCREEN_RESULT && race_p6.runtime.model.outcome == SC64_P6_WON &&
           race_selected_mission == SC64_P6_GLOBAL_ID &&
           (race_completed & (1 << SC64_P6_GLOBAL_ID)) ? 0 : 1;
}
