/* Normal-controller, no-state-injection Z3 campaign attempt.
 * Build with the private Z3 import/profile flags used by test-sc64-z3-run.
 * Every mutation of game state below is produced by game_update controller
 * input. Runtime reads are used only for adaptive choices and the trace. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define RUN_LIMIT 180000
#define CHECKPOINT_PERIOD 300

static FILE *trace;
static u16 prior_keys;
static u32 host_ticks;
static int last_logged_keys = -1;
static int trace_first = 1;
static const char *policy = "hybrid";
static const char *trace_path;
static int exploratory;
static int objective_x, objective_y;
static int deployed;

static int side_extractor(void);
static int side_gas_workers(void);
static int side_has_building(int type);

static void trace_prefix(void)
{
    if (!trace_first) fputs(",\n", trace);
    trace_first = 0;
}

static int owner_units(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++count;
    return count;
}

static int owner_buildings(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) ++count;
    return count;
}

static int live_mineral_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0) ++count;
    return count;
}

static void checkpoint(const char *kind)
{
    int side_units = 0, enemy_units = 0, side_buildings = 0, enemy_buildings = 0;
    int side_workers = 0, side_army = 0, enemy_minerals = 0, enemy_gas = 0;
    int chrysalis_hp = -1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].owner == SIDE) {
            ++side_units;
            if (sc64_race_is_worker(u[i].type)) ++side_workers;
            else ++side_army;
        } else if (u[i].owner == ENEMY) ++enemy_units;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].owner == SIDE) {
            ++side_buildings;
            if (b[i].raw_type == SC64_Z3_UNIT_MATURE_CHRYSALIS)
                chrysalis_hp = b[i].hp;
        }
        else if (b[i].owner == ENEMY) ++enemy_buildings;
    }
    enemy_minerals = race_minerals[ENEMY];
    enemy_gas = race_gas[ENEMY];
    trace_prefix();
    fprintf(trace,
        "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%d,\"elapsed\":%u,"
        "\"screen\":%d,\"phase\":%u,\"switch7\":%u,\"sequence\":%u,\"outcome\":%u,"
        "\"action_ticks\":%u,\"side_units\":%d,\"side_workers\":%d,\"side_army\":%d,"
        "\"enemy_units\":%d,\"side_buildings\":%d,\"enemy_buildings\":%d,"
        "\"foes_any\":%d,\"win_presence_condition\":%d,"
        "\"ore\":%d,\"gas\":%d,\"enemy_ore\":%d,\"enemy_gas\":%d,"
        "\"supply\":%d,\"supply_cap\":%d,\"gas_workers\":%d,\"extractor\":%d,\"hydra_den\":%d,"
        "\"chrysalis_hp\":%d,\"cursor_x\":%d,\"cursor_y\":%d,"
        "\"selected\":%d,\"selected_building\":%d,\"train_type\":%d}\n",
        kind, (unsigned)host_ticks, frame, (unsigned)race_campaign.elapsed_frames,
        screen, race_campaign.phase, race_z3.switch7, race_z3.sequence, race_z3.outcome,
        race_z3_action_ticks, side_units, side_workers, side_army, enemy_units,
        side_buildings, enemy_buildings, enemy_units + enemy_buildings,
        enemy_units + enemy_buildings <= 6 || enemy_buildings == 0,
        minerals, gas, enemy_minerals, enemy_gas,
        supply_used, supply_cap, side_gas_workers(), side_extractor() >= 0,
        side_has_building(B_HYDRALISK_DEN), chrysalis_hp, cx, cy, selected_count(), selected_building, train_type);
}

static void step(u16 held)
{
    if (deployed && (screen != SCREEN_PLAY || won || lost)) return;
    if (host_ticks >= RUN_LIMIT * 8u) {
        fprintf(stderr, "controller input budget exhausted at frame %d\n", frame);
        abort();
    }
    u16 pressed = held & (u16)~prior_keys;
    game_update(held, pressed);
    prior_keys = held;
    ++host_ticks;
    if (trace && held != (u16)last_logged_keys) {
        trace_prefix();
        fprintf(trace, "{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}\n",
                (unsigned)host_ticks, held, pressed);
        last_logged_keys = held;
    }
    if (trace && screen == SCREEN_PLAY && frame > 0 && frame % CHECKPOINT_PERIOD == 0)
        checkpoint("periodic");
}

static void tap(u16 key)
{
    step(key);
    step(0);
}

/* Navigate the remembered command row using ordinary controller inputs. */
static void tap_b(void)
{
    int fresh = !menu_open && mode == MODE_NORMAL;
    tap(KEY_B);
    if (fresh)
        for (int guard = 0; menu_open && menu_cursor != MENU_MOVE && guard < MENU_COUNT; ++guard)
            tap(menu_cursor > MENU_MOVE ? KEY_UP : KEY_DOWN);
}

static int snap_cursor_axis(int current, int target, int limit)
{
    int delta = target - current;
    int steps = delta >= 0 ? (delta + 2) / 4 : (delta - 2) / 4;
    int snapped = current + steps * 4;
    if (snapped < 0) snapped = 0;
    if (snapped >= limit) snapped = limit - 1;
    return snapped;
}

static void move_cursor(int x, int y)
{
    x = snap_cursor_axis(cx, x, WORLD_W);
    y = snap_cursor_axis(cy, y, WORLD_H);
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

static void move_cursor_held(int x, int y, u16 held)
{
    x = snap_cursor_axis(cx, x, WORLD_W);
    y = snap_cursor_axis(cy, y, WORLD_H);
    int steps=0;
    while ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost &&
           steps++ < WORLD_W + WORLD_H) {
        u16 keys = held;
        if (cx < x) keys |= KEY_RIGHT;
        if (cx > x) keys |= KEY_LEFT;
        if (cy < y) keys |= KEY_DOWN;
        if (cy > y) keys |= KEY_UP;
        step(keys);
    }
}

static void select_units_box(int x, int y, int radius)
{
    move_cursor(x - radius, y - radius);
    if (screen != SCREEN_PLAY || won || lost) return;
    step(KEY_L);
    move_cursor_held(x + radius, y + radius, KEY_L | KEY_R);
    step(0);
}

static int side_hatchery(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && !b[i].under_construction && b[i].owner == SIDE &&
            sc64_race_is_zerg_townhall(b[i].type)) return i;
    return -1;
}

static int side_has_building(int type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && !b[i].under_construction && b[i].owner == SIDE && b[i].type == type)
            return 1;
    return 0;
}

static int side_has_building_any(int type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == type) return 1;
    return 0;
}

static int side_worker_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && sc64_race_is_worker(u[i].type)) ++count;
    return count;
}

static int armed_count(void)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].owner==SIDE &&
        (sc64_race_unit_damage_vs(u[i].type,0)>0 || sc64_race_unit_damage_vs(u[i].type,1)>0)) ++n;
    return n;
}
static int side_type_count(int type)
{
    int n=0;for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type)++n;return n;
}

static int side_queued_count(int type)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE)
        for (int j = 0; j < b[i].queue_count; ++j)
            if (b[i].queue[(b[i].queue_head + j) % 5] == type) ++count;
    return count;
}

static int side_extractor(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_EXTRACTOR)
            return i;
    return -1;
}

static int side_gas_workers(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_DRONE &&
            u[i].order == ORDER_GAS) ++count;
    return count;
}

static int select_worker_at(int x, int y)
{
    select_units_box(x, y, 8);
    return find_selected_worker();
}

static void build_extractor_at_geyser(void)
{
    int geyser = -1, best = 0x7fffffff;
    int hatch = side_hatchery();
    if (hatch < 0 || side_extractor() >= 0 || minerals < 50) return;
    for (int i = 0; i < geyser_count; ++i) {
        int occupied = 0;
        for (int j = 0; j < MAX_BUILDINGS; ++j)
            if (b[j].alive && b[j].type == B_EXTRACTOR &&
                dist2(b[j].x, b[j].y, geysers[i].x, geysers[i].y) < 15 * 15)
                occupied = 1;
        int d = dist2(b[hatch].x, b[hatch].y, geysers[i].x, geysers[i].y);
        if (!occupied && d < best) { best = d; geyser = i; }
    }
    if (geyser < 0) return;
    int worker = -1, worker_distance = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        u[i].type == U_DRONE && u[i].order != ORDER_BUILD) {
        int d = dist2(u[i].x, u[i].y, geysers[geyser].x, geysers[geyser].y);
        if (d < worker_distance) { worker_distance = d; worker = i; }
    }
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    tap_b();
    for (int i = 0; i < 4; ++i) tap(KEY_DOWN); /* Zerg menu: Extractor */
    tap(KEY_A);
    move_cursor(geysers[geyser].x, geysers[geyser].y);
    tap(KEY_A);
}

static void assign_gas_workers(void)
{
    int extractor = side_extractor();
    if (extractor < 0 || !building_complete(extractor) || side_gas_workers() >= 3 ||
        side_worker_count() - side_gas_workers() <= 3) return;
    int worker = -1, best = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        u[i].type == U_DRONE && u[i].order != ORDER_GAS && u[i].order != ORDER_BUILD) {
        int d = dist2(u[i].x, u[i].y, b[extractor].x, b[extractor].y);
        if (d < best) { best = d; worker = i; }
    }
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    move_cursor(b[extractor].x, b[extractor].y);
    tap(KEY_A); /* Selected Drone assigns itself to the completed Extractor. */
}

/* Reassign an idle Drone with a visible mineral click after a patch runs out. */
static void assign_idle_miner(void)
{
    int worker = -1, patch = -1, best = 0x7fffffff, have_idle = 0;
    int hatch = side_hatchery();
    if (hatch < 0) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_DRONE &&
            u[i].order == ORDER_IDLE && !u[i].carry) { have_idle = 1; break; }
    if (!have_idle) return;
    /* Stacked Drones may not be independently selectable. Accept the actual
     * selected eligible Drone, never substitute an index or write selection. */
    for (int guard = 0; guard < MAX_UNITS; ++guard) {
        tap(KEY_L);
        int actual = find_selected_worker();
        if (selected_count() == 1 && actual >= 0 && u[actual].alive &&
            u[actual].owner == SIDE && u[actual].type == U_DRONE &&
            u[actual].order == ORDER_IDLE && !u[actual].carry) { worker = actual; break; }
    }
    if (worker < 0) return;
    for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0 &&
        dist2(b[hatch].x,b[hatch].y,m[i].x,m[i].y) <= 220*220) {
        int nx, ny, load=0, unsafe=0;
        for (int j=0;j<MAX_UNITS;++j) if (u[j].alive && u[j].owner==ENEMY &&
            currently_visible(u[j].x,u[j].y) && dist2(u[j].x,u[j].y,m[i].x,m[i].y)<120*120)
            {unsafe=1; break;}
        if (unsafe) continue;
        if (!path_next_tile(u[worker].x,u[worker].y,m[i].x,m[i].y,&nx,&ny)) continue;
        for (int j=0;j<MAX_UNITS;++j)
            if (u[j].alive && u[j].owner==SIDE && u[j].order==ORDER_MINE && u[j].target==i) ++load;
        int d = dist2(u[worker].x,u[worker].y,m[i].x,m[i].y) + load * 40000;
        if (d < best) { best = d; patch = i; }
    }
    if (patch < 0) return;
    if (!currently_visible(m[patch].x,m[patch].y)) {
        /* The controller may consult map contents, but uses a normal Move to
         * reveal a field before issuing its contextual mineral command. */
        tap_b();
        if (menu_open && menu_cursor == MENU_MOVE) tap(KEY_A);
        if (mode == MODE_MOVE) {
            move_cursor(m[patch].x,m[patch].y + 24);
            tap(KEY_A);
            fprintf(stderr,"Z3 miner%d approaches field%d frame%d order%d\n",worker,patch,frame,u[worker].order);
        }
        return;
    }
    static const int offsets[][2] = {{0,0},{-8,0},{8,0},{0,-8},{0,8},
        {-8,-8},{8,-8},{-8,8},{8,8},{-12,0},{12,0},{0,-12},{0,12}};
    for (unsigned i = 0; i < sizeof(offsets)/sizeof(offsets[0]); ++i) {
        if (!u[worker].alive || !u[worker].selected || selected_count() != 1) return;
        move_cursor(m[patch].x + offsets[i][0],m[patch].y + offsets[i][1]);
        if (selected_worker_mineral_target() != patch) continue;
        tap(KEY_A);
        if (u[worker].alive && u[worker].order == ORDER_MINE && u[worker].target == patch)
            fprintf(stderr,"Z3 miner%d assigned field%d frame%d\n",worker,patch,frame);
        return;
    }
}

/* A ground assault has repeatedly stalled against the enemy base. Build the
 * ordinary Lair/Spire tech through the same menu used by a human player. */
static void advance_air_tech(void)
{
    int h=side_hatchery();
    if(h<0 || side_worker_count()<6 || !side_has_building(B_SPAWNING_POOL)) return;
    if (!side_has_building_any(B_LAIR) && armed_count()<40) return;
    if(!side_has_building_any(B_LAIR)) {
        if(b[h].type!=B_HATCHERY || b[h].queue_count || minerals<building_cost(B_LAIR) ||
           gas<building_gas_cost(B_LAIR)) return;
        move_cursor(b[h].x,b[h].y); tap(KEY_A);
        if(selected_building!=h) return;
        tap_b();
        for(int n=0;menu_open && menu_cursor!=MENU_ZP_LAIR && n<MENU_COUNT;++n) tap(KEY_DOWN);
        if(menu_open && menu_cursor==MENU_ZP_LAIR) tap(KEY_A);
        return;
    }
    if(!side_has_building(B_LAIR) || side_has_building_any(B_SPIRE) ||
       minerals<building_cost(B_SPIRE) || gas<building_gas_cost(B_SPIRE)) return;
    int sx=-1,sy=-1,best=0x7fffffff;
    for(int dy=-128;dy<=128;dy+=16)for(int dx=-128;dx<=128;dx+=16) {
        int x=b[h].x+dx,y=b[h].y+dy,d=dx*dx+dy*dy;
        if(d<48*48 || d>=best || !race_can_site(SIDE,B_SPIRE,x,y) || !can_place(B_SPIRE,x,y)) continue;
        best=d;sx=x;sy=y;
    }
    if(sx<0)return;
    int worker=-1;
    for(int n=0;n<MAX_UNITS;++n) {
        tap(KEY_L); int v=find_selected_worker();
        if(selected_count()==1 && v>=0 && u[v].alive && u[v].owner==SIDE &&
           u[v].type==U_DRONE && u[v].order!=ORDER_GAS && u[v].order!=ORDER_BUILD &&
           !SC64_PENDING_BUILD_ORDER(u[v].order)) {worker=v;break;}
    }
    if(worker<0)return;
    tap_b();
    for(int n=0;menu_open && menu_cursor!=MENU_ZP_SPIRE && n<MENU_COUNT;++n) tap(KEY_DOWN);
    if(!menu_open || menu_cursor!=MENU_ZP_SPIRE)return;
    tap(KEY_A);
    if(mode==MODE_BUILD && menu==B_SPIRE) {move_cursor(sx,sy);tap(KEY_A);}
}

static void build_hydralisk_den(void)
{
    int hatch = side_hatchery();
    if (hatch < 0 || side_has_building_any(B_HYDRALISK_DEN) ||
        minerals < 100 || gas < 50 || !side_has_building(B_SPAWNING_POOL)) return;
    int site_x = -1, site_y = -1, best = 0x7fffffff;
    for (int y = b[hatch].y - 128; y <= b[hatch].y + 128; y += 16)
        for (int x = b[hatch].x - 128; x <= b[hatch].x + 128; x += 16) {
            if (!race_can_site(SIDE, B_HYDRALISK_DEN, x, y) || !can_place(B_HYDRALISK_DEN, x, y))
                continue;
            int d = dist2(b[hatch].x, b[hatch].y, x, y);
            if (d < best) { best = d; site_x = x; site_y = y; }
        }
    if (site_x < 0) return;
    int worker = -1, worker_distance = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        u[i].type == U_DRONE && u[i].order != ORDER_BUILD) {
        int d = dist2(u[i].x, u[i].y, site_x, site_y);
        if (d < worker_distance) { worker_distance = d; worker = i; }
    }
    if (worker < 0 || select_worker_at(u[worker].x, u[worker].y) < 0) return;
    tap_b();
    for (int i = 0; i < 5; ++i) tap(KEY_DOWN); /* Zerg menu: Hydralisk Den */
    tap(KEY_A);
    move_cursor(site_x, site_y);
    tap(KEY_A);
}

static int enemy_target_position(int *x, int *y)
{
    int best = 0x7fffffff, found = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == ENEMY) {
        int d = dist2(objective_x, objective_y, b[i].x, b[i].y);
        if (d < best) { best = d; *x = b[i].x; *y = b[i].y; found = 1; }
    }
    if (found) return 1; /* Advance on structures when the objective is safe. */
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
        int d = dist2(objective_x, objective_y, u[i].x, u[i].y);
        if (d < best) { best = d; *x = u[i].x; *y = u[i].y; found = 1; }
    }
    return found;
}

static int nearest_enemy_unit_position(int x, int y, int *target_x, int *target_y,
                                       int *target_slot)
{
    int best = 0x7fffffff, found = 0;
    if (target_slot) *target_slot = -1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
        int d = dist2(x, y, u[i].x, u[i].y);
        if (d < best) {
            best = d; *target_x = u[i].x; *target_y = u[i].y;
            if (target_slot) *target_slot = i;
            found = 1;
        }
    }
    return found ? best : -1;
}

static void issue_attack_order_at(int x, int y)
{
    fprintf(stderr, "controller attack target=%d,%d from=%d,%d distance2=%d selected_building=%d\n",
            x, y, cx, cy, dist2(cx, cy, x, y), selected_building);
    if (selected_building >= 0) {
        tap_b();
        tap(KEY_DOWN);
        tap(KEY_A);
        tap(KEY_A);
    }
    tap(KEY_R);             /* select all eligible player combat units */
    tap_b();             /* open command menu */
    tap(KEY_DOWN);          /* ATTACK MOVE */
    tap(KEY_A);             /* choose command */
    move_cursor(x, y);
    tap(KEY_A);             /* order selected units through normal input */
}

static int local_combat_count(int x, int y, int radius)
{
    int count=0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].owner==SIDE &&
        (sc64_race_unit_damage_vs(u[i].type,0)>0 || sc64_race_unit_damage_vs(u[i].type,1)>0) &&
        abs(u[i].x-x)<=radius && abs(u[i].y-y)<=radius) ++count;
    return count;
}

static void issue_selected_attack_order_at(int x, int y)
{
    /* Do not issue an order to a defense box containing mining Drones. */
    for (int i=0; i<MAX_UNITS; ++i) if (u[i].alive && u[i].selected &&
        u[i].owner==SIDE && sc64_race_is_worker(u[i].type)) {
        return;
    }
    if (selected_building>=0) {
        tap_b();
        tap(KEY_DOWN);
        tap(KEY_A);
        tap(KEY_A);
    }
    tap_b();
    tap(KEY_DOWN);
    tap(KEY_A);
    move_cursor(x,y);
    tap(KEY_A);
}

/* L selects one unit even when many units share one position. */
static int select_single_hydra(int x,int y,int radius,int inside)
{
    for(int n=0;n<MAX_UNITS;++n) {
        tap(KEY_L);
        if(selected_count()!=1)continue;
        for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].selected&&u[i].owner==SIDE&&
            u[i].type==U_HYDRALISK &&
            ((dist2(u[i].x,u[i].y,x,y)<=radius*radius)==inside))return 1;
    }
    return 0;
}

static void issue_attack_order(void)
{
    int x,y;
    if (!enemy_target_position(&x,&y)) return;
    if (strcmp(policy,"hybrid")) { issue_attack_order_at(x,y); return; }
    /* L cycles a single aircraft even when it overlaps the ground reserve.
     * Send idle Mutalisks individually; leave Hydralisks at the objectives. */
    for(int sent=0;sent<16 && screen==SCREEN_PLAY && !won && !lost;++sent) {
        int found=0;
        for(int n=0;n<MAX_UNITS;++n) {
            tap(KEY_L);
            if(selected_count()!=1)continue;
            for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].owner==SIDE&&u[i].selected&&
                u[i].type==U_MUTALISK&&u[i].order==ORDER_IDLE) {found=1;break;}
            if(found)break;
        }
        if(!found)break;
        issue_selected_attack_order_at(x,y);
    }
}

static void issue_order_at(int x, int y)
{
    /* R cycles a selected producer's train type instead of selecting combat.
     * Clear that building selection through its normal attack command first. */
    if (selected_building >= 0) {
        tap_b();
        tap(KEY_DOWN);
        tap(KEY_A);
        tap(KEY_A);
    }
    tap(KEY_R);
    tap_b();
    tap(KEY_DOWN);
    tap(KEY_A);
    move_cursor(x, y);
    tap(KEY_A);
    assert(selected_count() > 0 && selected_building < 0 &&
           "normal attack-move sequence must select combat units");
}

static int mature_chrysalis_position(int *x, int *y)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE &&
            b[i].raw_type == SC64_Z3_UNIT_MATURE_CHRYSALIS) {
            *x = b[i].x;
            *y = b[i].y;
            return 1;
        }
    return 0;
}

static int mature_chrysalis_health(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE &&
            b[i].raw_type == SC64_Z3_UNIT_MATURE_CHRYSALIS)
            return b[i].hp;
    return 0;
}

static int unit_choice(void)
{
    int free_supply = race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE);
    if (free_supply <= 4 && minerals >= sc64_race_unit_rules[U_OVERLORD].minerals)
        return U_OVERLORD;
    if (side_worker_count() + side_queued_count(U_DRONE) < 8 &&
        minerals >= sc64_race_unit_rules[U_DRONE].minerals)
        return U_DRONE;
    if (side_has_building(B_SPIRE) && minerals>=100 && gas>=100)
        return U_MUTALISK;
    if (gas >= sc64_race_unit_rules[U_HYDRALISK].gas &&
        minerals >= sc64_race_unit_rules[U_HYDRALISK].minerals &&
        side_has_building(B_HYDRALISK_DEN)) return U_HYDRALISK;
    if (minerals >= sc64_race_unit_rules[U_ZERGLING].minerals &&
        side_has_building(B_SPAWNING_POOL)) return U_ZERGLING;
    return -1;
}

static void queue_one_unit(void)
{
    int hatch = side_hatchery(), choice = unit_choice();
    if (hatch < 0 || choice < 0 || b[hatch].queue_count >= 3 || !b[hatch].larva) return;
    if (side_worker_count() >= 6 && armed_count()>=40 &&
        side_has_building(B_SPAWNING_POOL) && !side_has_building_any(B_LAIR)) return;
    move_cursor(b[hatch].x, b[hatch].y);
    tap(KEY_A); /* select the authored Hatchery */
    for (int i = 0; i < 4 && train_type != choice; ++i) tap(KEY_R);
    if (train_type == choice) tap(KEY_A); /* queue production through the UI */
}

static void launch_z3(void)
{
    game_init();
    prior_keys = 0;
    tap(KEY_A); /* HOME -> Zerg mission list */
    tap(KEY_RIGHT); /* page two, Z3 ordinal 2 */
    tap(KEY_A); /* briefing */
    step(KEY_START); /* explicit deploy without a release tick in the mission */
    deployed = 1;
    assert(screen == SCREEN_PLAY && !race_mode && race_selected_mission == 14);
    assert(race_campaign.mission == 14 && sc64_z3_state_valid(&race_z3));
    assert(sc64_race_maps[14].unit_count == 122);
    assert(mission_unsupported_count == 0);
    fprintf(stderr, "Z3 launch roster: source rows=%u units side/enemy=%d/%d buildings side/enemy=%d/%d minerals=%d geysers=%d\n",
            sc64_race_maps[14].unit_count, owner_units(SIDE), owner_units(ENEMY),
            owner_buildings(SIDE), owner_buildings(ENEMY), live_mineral_count(), geyser_count);
}

static void find_guard_waypoint(int objective_x, int objective_y, int *guard_x, int *guard_y)
{
    int target_x = objective_x, target_y = objective_y;
    int best = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
        int d = dist2(objective_x, objective_y, u[i].x, u[i].y);
        if (d < best) { best = d; target_x = u[i].x; target_y = u[i].y; }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == ENEMY) {
        int d = dist2(objective_x, objective_y, b[i].x, b[i].y);
        if (d < best) { best = d; target_x = b[i].x; target_y = b[i].y; }
    }
    *guard_x = objective_x;
    *guard_y = objective_y;
    if (abs(target_x - objective_x) >= abs(target_y - objective_y))
        *guard_x += target_x > objective_x ? 40 : target_x < objective_x ? -40 : 0;
    else
        *guard_y += target_y > objective_y ? 40 : target_y < objective_y ? -40 : 0;
    *guard_x = limit(*guard_x, 0, WORLD_W - 1);
    *guard_y = limit(*guard_y, 0, WORLD_H - 1);
}

static int nearest_enemy_from(int x, int y, int *target_x, int *target_y)
{
    int best = 0x7fffffff, found = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == ENEMY) {
        int d = dist2(x, y, b[i].x, b[i].y);
        if (d < best) { best = d; *target_x = b[i].x; *target_y = b[i].y; found = 1; }
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
        int d = dist2(x, y, u[i].x, u[i].y);
        if (d < best) { best = d; *target_x = u[i].x; *target_y = u[i].y; found = 1; }
    }
    return found ? best : -1;
}

static void usage(const char *program)
{
    fprintf(stderr, "usage: %s [--policy guard|attack|hybrid] [--trace PATH] [--exploratory]\n", program);
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--policy") && i + 1 < argc) policy = argv[++i];
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc) trace_path = argv[++i];
        else if (!strcmp(argv[i], "--exploratory")) exploratory = 1;
        else if (!strcmp(argv[i], "--help")) { usage(argv[0]); return 0; }
        else { usage(argv[0]); return 2; }
    }
    if (strcmp(policy, "guard") && strcmp(policy, "attack") && strcmp(policy, "hybrid")) {
        usage(argv[0]); return 2;
    }
    mkdir(".local", 0777);
    mkdir(".local/sc64", 0777);
    char default_trace[128];
    if (!trace_path) {
        snprintf(default_trace, sizeof(default_trace), ".local/sc64/z3-controller-attempt-%s.json", policy);
        trace_path = default_trace;
    }
    trace = fopen(trace_path, "w");
    assert(trace);
    fprintf(trace, "{\"schema\":1,\"mission\":14,\"policy\":\"%s\",\"events\":[\n", policy);
    gba_init();
    launch_z3();
    checkpoint("start");

    /* Resource and attack planning can inspect world state and map contents;
       every game action below uses controller inputs. */
    int chrysalis_x, chrysalis_y;
    assert(mature_chrysalis_position(&chrysalis_x, &chrysalis_y) &&
           "source raw 150 was not imported as human mature Chrysalis building");
    objective_x = chrysalis_x;
    objective_y = chrysalis_y;
    int guard_x, guard_y;
    find_guard_waypoint(chrysalis_x, chrysalis_y, &guard_x, &guard_y);
    fprintf(stderr, "controller policy=%s objective=%d,%d guard=%d,%d geysers=%d\n",
            policy, chrysalis_x, chrysalis_y, guard_x, guard_y, geyser_count);
    if (strcmp(policy, "attack")) issue_order_at(guard_x, guard_y);
    int next_train = frame + 120;
    int next_action = frame + 3600;
    int next_extractor_attempt = frame;
    int next_gas_assignment = frame;
    int next_miner_assignment = frame;
    int next_den_attempt = frame;
    int next_air_attempt = frame;
    int next_defense_scan = frame + 120;
    int next_hatch_guard = frame + 300;
    int defended_target = -1;
    while (frame < RUN_LIMIT && screen == SCREEN_PLAY && !won && !lost) {
        if (!strcmp(policy, "hybrid") && frame >= next_defense_scan) {
            int ex=0,ey=0,target=-1;
            int enemy_distance=nearest_enemy_unit_position(
                chrysalis_x,chrysalis_y,&ex,&ey,&target);
            if (defended_target>=0 &&
                (!u[defended_target].alive || u[defended_target].owner!=ENEMY))
                defended_target=-1;
            /* Keep nearby defenders on emergency response timing. This uses
             * ordinary box selection and attack-move input, leaving the main
             * army's longer strategic order and distant economy untouched. */
            if (enemy_distance>=0 && enemy_distance<220*220 && target>=0 &&
                target!=defended_target && local_combat_count(chrysalis_x,
                    chrysalis_y,160)>0) {
                select_units_box(chrysalis_x,chrysalis_y,160);
                if (screen==SCREEN_PLAY && !won && !lost &&
                    selected_count()>0) {
                    issue_selected_attack_order_at(ex,ey);
                    if (screen==SCREEN_PLAY && !won && !lost)
                        defended_target=target;
                }
            }
            int hatch=side_hatchery();
            if (hatch>=0 && (enemy_distance<0 || enemy_distance>=220*220)) {
                int hx=b[hatch].x,hy=b[hatch].y,tx=0,ty=0,attacker=-1;
                int hd=nearest_enemy_unit_position(hx,hy,&tx,&ty,&attacker);
                if (hd>=0 && hd<180*180 && local_combat_count(hx,hy,160)>0) {
                    if (select_single_hydra(hx,hy,160,1) && screen==SCREEN_PLAY && !won && !lost)
                        issue_selected_attack_order_at(tx,ty);
                }
            }
            next_defense_scan=frame+120;
        }
        /* Reinforce the objective and Hatchery individually: spawned units
         * can stack, so a box around either site may capture the whole army. */
        if (frame >= next_hatch_guard) {
            int h=side_hatchery();
            if (local_combat_count(chrysalis_x,chrysalis_y,120)<10) {
                if(select_single_hydra(chrysalis_x,chrysalis_y,120,0))
                    issue_selected_attack_order_at(guard_x,guard_y);
            } else if (h>=0 && local_combat_count(b[h].x,b[h].y,120)<4) {
                if(select_single_hydra(b[h].x,b[h].y,120,0))
                    issue_selected_attack_order_at(b[h].x,b[h].y);
            }
            next_hatch_guard=frame+300;
        }
        if (frame >= next_extractor_attempt && side_extractor() < 0 && minerals >= 50) {
            build_extractor_at_geyser();
            next_extractor_attempt = frame + 900;
        }
        if (frame >= next_miner_assignment) {
            assign_idle_miner();
            next_miner_assignment = frame + 600;
        }
        if (frame >= next_gas_assignment && side_extractor() >= 0) {
            assign_gas_workers();
            next_gas_assignment = frame + 600;
        }
        if (frame >= next_den_attempt && side_extractor() >= 0) {
            build_hydralisk_den();
            next_den_attempt = frame + 900;
        }
        if (frame >= next_air_attempt) {
            advance_air_tech();
            next_air_attempt = frame + 600;
        }
        if (frame >= next_train) {
            queue_one_unit();
            next_train = frame + 120;
        }
        if (frame >= next_action) {
            if (!strcmp(policy, "attack")) {
                issue_attack_order();
            } else if (!strcmp(policy, "guard")) {
                issue_order_at(guard_x, guard_y);
            } else {
                int ex, ey, enemy_unit_distance;
                int threat = nearest_enemy_from(chrysalis_x, chrysalis_y, &ex, &ey);
                enemy_unit_distance = nearest_enemy_unit_position(chrysalis_x,
                                                                   chrysalis_y,
                                                                   &ex, &ey, 0);
                if ((enemy_unit_distance < 0 || enemy_unit_distance >= 220 * 220) &&
                    threat >= 0 && armed_count() >= 60 && side_type_count(U_MUTALISK)>=12 &&
                    side_has_building(B_SPIRE) && mature_chrysalis_health() >= 150)
                    issue_attack_order();
            }
            next_action = frame + (!strcmp(policy, "guard") ? 900 : 1800);
        }
        step(0);
    }
    if (won) {
        int objective_check_x, objective_check_y;
        assert(screen == SCREEN_RESULT && !lost);
        assert(sc64_z3_state_valid(&race_z3) && race_z3.outcome == SC64_Z3_WON);
        assert(race_campaign.phase == SC64_RACE_PHASE_COMPLETE);
        assert(mature_chrysalis_position(&objective_check_x, &objective_check_y));
        assert(owner_units(ENEMY) + owner_buildings(ENEMY) <= 6 ||
               owner_buildings(ENEMY) == 0);
    }
    checkpoint(won ? "won" : lost ? "lost" : "limit");
    fprintf(trace, "]}\n");
    fclose(trace);
    printf("Z3 normal-controller run: policy=%s result=%s frame=%d elapsed=%u side=%d/%d enemy=%d/%d ore=%d gas=%d trace=%s\n",
           policy,
           won ? "WON" : lost ? "LOST" : "LIMIT", frame,
           (unsigned)race_campaign.elapsed_frames, owner_units(SIDE), owner_buildings(SIDE),
           owner_units(ENEMY), owner_buildings(ENEMY), minerals, gas, trace_path);
    return (won || exploratory) ? 0 : 1;
}
