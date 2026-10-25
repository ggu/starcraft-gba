/* Genuine input-driven P5 campaign attempt. Controller decisions inspect
 * live world state, but all gameplay changes go through game_update inputs. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define P5_RUN_LIMIT 10000
#define P5_CHECKPOINT_PERIOD 900

static FILE *trace;
static u16 prior_keys;
static u32 host_ticks;
static int deployed;
static int run_limit = P5_RUN_LIMIT;
static int trace_first = 1;
static int last_keys = -1;
static int next_order_frame;
static int path_diagnostics;
static int diagnostic_carrier = -1;
static int diagnostic_carrier_hp = -1;
static int diagnostic_carrier_shield = -1;
typedef struct {
    u8 alive, owner, raw;
    s16 x, y;
    s16 order, target, cd;
    u8 flying, air_range, air_damage;
} P5ThreatSnapshot;
static P5ThreatSnapshot prior_threats[MAX_UNITS];
static P5ThreatSnapshot prior_buildings[MAX_BUILDINGS];
static char attempt_path[160] = ".local/sc64/p5-controller-attempt-running.json";
static char final_path[160] = ".local/sc64/p5-controller-attempt.json";
static char verified_path[160] = ".local/sc64/p5-controller-win.json";

static void tap(u16 key);
static void move_cursor(int x, int y);
static int objective_location(int *left, int *top, int *right, int *bottom);

static void trace_prefix(void)
{
    if (!trace_first) fputs(",\n", trace);
    trace_first = 0;
}

static int live_raw_unit(unsigned raw, int live_owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == (int)raw &&
            (live_owner < 0 || u[i].owner == live_owner)) return i;
    return -1;
}

static int live_units(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) count += u[i].alive != 0;
    return count;
}

static int live_buildings(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        count += b[i].alive && (owner < 0 || b[i].owner == owner);
    return count;
}

static int raw_unit_count(unsigned raw, int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].raw_type == (int)raw &&
                 (owner < 0 || u[i].owner == owner);
    return count;
}

static int live_shuttle(void)
{
    return live_raw_unit(69u, SIDE);
}

static int hostile_building_air_range(int type, int *damage)
{
    *damage = 0;
    if (type == B_SPORE_COLONY) { *damage = 15; return 112; }
    if (type == B_MISSILE_TURRET || type == B_PHOTON_CANNON) {
        *damage = type == B_PHOTON_CANNON ? 20 : 20;
        return 112;
    }
    return 0;
}

static int select_actor(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        u[slot].owner != SIDE || SC64_TRANSPORT_LOADED(&u[slot]) ||
        selected_building >= 0) return 0;
    for (int i = 0; i <= MAX_UNITS; ++i) {
        if (u[slot].selected && selected_count() == 1) return 1;
        tap(KEY_L);
    }
    return u[slot].selected && selected_count() == 1;
}

static int issue_move_actor(int slot, int x, int y)
{
    if (!select_actor(slot)) return 0;
    tap(KEY_B); tap(KEY_A);
    if (mode != MODE_MOVE) return 0;
    move_cursor(x, y); tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_MOVE;
}

static int issue_attack_move_actor(int slot, int x, int y)
{
    if (!select_actor(slot)) return 0;
    tap(KEY_B);
    if (!menu_open || field_menu_count() <= MENU_ATTACK) return 0;
    for (int i = 0; i < MENU_ATTACK; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    if (mode != MODE_ATTACK) return 0;
    move_cursor(x, y); tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_ATTACK_MOVE;
}

static int issue_board_actor(int passenger, int carrier)
{
    if (passenger < 0 || carrier < 0 || !select_actor(passenger)) return 0;
    move_cursor(u[carrier].x, u[carrier].y); tap(KEY_A);
    return u[passenger].alive && u[passenger].target == carrier &&
           (u[passenger].order == ORDER_BOARD || SC64_TRANSPORT_LOADED(&u[passenger]));
}

static int choose_landing_point(int *out_x, int *out_y)
{
    int left, top, right, bottom;
    if (!objective_location(&left, &top, &right, &bottom)) return 0;
    /* Use the southern edge of the extraction region: the northern edge is
     * covered by Hydralisks while the far south lies inside the Spore line. */
    for (int y = bottom - 8; y >= top + 8; y -= 8)
        for (int x = left + 8; x < right; x += 8) {
            if (terrain_blocked(x / 16, y / 16)) continue;
            int clear = 1;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && SC64_TRANSPORT_LOADED(&u[i]) &&
                    u[i].target == live_shuttle() && !unit_step_clear(&u[i], x, y)) {
                    clear = 0; break;
                }
            if (!clear) continue;
            *out_x = x; *out_y = y; return 1;
        }
    return 0;
}

static int issue_shuttle_unload(int carrier, int x, int y)
{
    if (!select_actor(carrier) || !race_p5_shuttle_menu_available()) return 0;
    tap(KEY_B);
    if (!menu_open || field_menu_count() != MENU_STOP + 2) return 0;
    for (int i = 0; i < MENU_STOP + 1; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    if (mode != MODE_UNLOAD) return 0;
    move_cursor(x, y); tap(KEY_A);
    return u[carrier].order == ORDER_UNLOAD;
}

static int find_boardable_zealot(int carrier)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 65 &&
            !SC64_TRANSPORT_LOADED(&u[i]) &&
            (u[i].order == ORDER_IDLE || u[i].order == ORDER_MOVE ||
             u[i].order == ORDER_ATTACK_MOVE) &&
            u[i].target != carrier) return i;
    return -1;
}

static int find_boardable_raw(int raw, int carrier)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == raw &&
            !SC64_TRANSPORT_LOADED(&u[i]) &&
            (u[i].order == ORDER_IDLE || u[i].order == ORDER_MOVE ||
             u[i].order == ORDER_ATTACK_MOVE) && u[i].target != carrier) return i;
    return -1;
}

static int loaded_raw_count(int raw, int carrier)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].raw_type == raw &&
                 SC64_TRANSPORT_LOADED(&u[i]) && u[i].target == carrier;
    return count;
}

static int boarding_raw_count(int raw, int carrier)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].raw_type == raw &&
                 u[i].order == ORDER_BOARD && u[i].target == carrier;
    return count;
}

static int objective_location(int *left, int *top, int *right, int *bottom)
{
    return sc64_p5_world_location(SC64_P5_LOCATION_VICTORY,
                                  left, top, right, bottom);
}

/* The authored objective rectangle includes blocked structure/terrain cells.
 * Pick a routable tile inside that same source region using the live path grid,
 * then command actors through ordinary Move/Attack-Move inputs. */
static int objective_approach(int start_x, int start_y, int *out_x, int *out_y)
{
    int left, top, right, bottom, best = 0x7fffffff;
    if (!out_x || !out_y || !objective_location(&left, &top, &right, &bottom))
        return 0;
    int c0 = left / 16, c1 = (right - 1) / 16;
    int r0 = top / 16, r1 = (bottom - 1) / 16;
    int found = 0;
    for (int row = r0; row <= r1; ++row) for (int col = c0; col <= c1; ++col) {
        int x = col * 16 + 8, y = row * 16 + 8, nx = -1, ny = -1;
        int cell = col + row * MAP_COLS;
        if (cell < 0 || cell >= MAP_CELLS || !SC64_GRID_GET(path_walkable, cell) ||
            !sc64_route_next(start_x, start_y, x, y, &nx, &ny)) continue;
        int d = dist2(x, y, (left + right) / 2, (top + bottom) / 2);
        if (d < best) { best = d; *out_x = x; *out_y = y; found = 1; }
    }
    return found;
}

/* Long exact goals may sit behind temporary path-grid blockers. Plan a short
 * segment over the source terrain grid, then let the engine execute it. */
static int terrain_waypoint_to_objective(int start_x, int start_y,
                                         int *out_x, int *out_y)
{
    static u16 queue[MAP_CELLS], parent[MAP_CELLS], reverse[MAP_CELLS];
    int left, top, right, bottom;
    if (!out_x || !out_y || !objective_location(&left, &top, &right, &bottom))
        return 0;
    int sc = start_x / 16, sr = start_y / 16;
    if (sc < 0 || sr < 0 || sc >= MAP_COLS || sr >= MAP_ROWS) return 0;
    int start = sc + sr * MAP_COLS;
    for (int i = 0; i < MAP_CELLS; ++i) parent[i] = 0xffffu;
    unsigned head = 0, tail = 0, count = 0;
    queue[tail] = (u16)start; tail = (tail + 1u) % MAP_CELLS; ++count;
    parent[start] = (u16)start;
    int goal = -1;
    if (path_diagnostics) {
        int c0 = left / 16, c1 = (right - 1) / 16;
        int r0 = top / 16, r1 = (bottom - 1) / 16;
        int open = 0, walk = 0;
        for (int row = r0; row <= r1; ++row) for (int col = c0; col <= c1; ++col) {
            open += !terrain_blocked(col, row);
            walk += SC64_GRID_GET(path_walkable, col + row * MAP_COLS) != 0;
        }
        fprintf(stderr, "P5 objective cells region=%d,%d..%d,%d terrain_open=%d path_open=%d\n",
                c0, r0, c1, r1, open, walk);
    }
    static const int dc[4] = {0, 1, 0, -1};
    static const int dr[4] = {-1, 0, 1, 0};
    while (count) {
        int cell = queue[head]; head = (head + 1u) % MAP_CELLS; --count;
        int col = cell % MAP_COLS, row = cell / MAP_COLS;
        int x = col * 16 + 8, y = row * 16 + 8;
        if (!terrain_blocked(col, row) && x >= left && x < right &&
            y >= top && y < bottom) { goal = cell; break; }
        for (int d = 0; d < 4; ++d) {
            int nc = col + dc[d], nr = row + dr[d];
            if (nc < 0 || nr < 0 || nc >= MAP_COLS || nr >= MAP_ROWS) continue;
            int next = nc + nr * MAP_COLS;
            if (parent[next] != 0xffffu || terrain_blocked(nc, nr)) continue;
            parent[next] = (u16)cell;
            queue[tail] = (u16)next; tail = (tail + 1u) % MAP_CELLS; ++count;
        }
    }
    if (goal < 0) {
        if (path_diagnostics) {
            int best = 0x7fffffff, best_cell = -1;
            for (int cell = 0; cell < MAP_CELLS; ++cell) if (parent[cell] != 0xffffu) {
                int x = cell % MAP_COLS * 16 + 8;
                int y = cell / MAP_COLS * 16 + 8;
                int dx = x < left ? left - x : x >= right ? x - right + 1 : 0;
                int dy = y < top ? top - y : y >= bottom ? y - bottom + 1 : 0;
                int d = dx * dx + dy * dy;
                if (d < best) { best = d; best_cell = cell; }
            }
            fprintf(stderr, "P5 terrain disconnected: nearest=%d,%d goal=%d,%d d2=%d\n",
                    best_cell < 0 ? -1 : best_cell % MAP_COLS * 16 + 8,
                    best_cell < 0 ? -1 : best_cell / MAP_COLS * 16 + 8,
                    (left + right) / 2, (top + bottom) / 2, best);
        }
        return 0;
    }
    unsigned n = 0;
    for (int cell = goal; cell != start && n < MAP_CELLS; cell = parent[cell])
        reverse[n++] = (u16)cell;
    if (!n) { *out_x = goal % MAP_COLS * 16 + 8;
              *out_y = goal / MAP_COLS * 16 + 8; return 1; }
    unsigned pick = n > 8u ? n - 8u : 0u;
    int cell = reverse[pick];
    *out_x = cell % MAP_COLS * 16 + 8;
    *out_y = cell / MAP_COLS * 16 + 8;
    return 1;
}

static int objective_unit_count(unsigned raw)
{
    int left, top, right, bottom, count = 0;
    if (!objective_location(&left, &top, &right, &bottom)) return -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == (int)raw &&
            u[i].x >= left && u[i].x < right &&
            u[i].y >= top && u[i].y < bottom) ++count;
    return count;
}

static int army_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE &&
            !sc64_race_is_worker(u[i].type) &&
            sc64_game_unit_selectable(u[i].type)) ++count;
    return count;
}

static void checkpoint(const char *kind)
{
    int hero = live_raw_unit(SC64_P5_RAW_HERO, SIDE);
    int l = 0, t = 0, r = 0, btm = 0;
    (void)objective_location(&l, &t, &r, &btm);
    int zealots = objective_unit_count(SC64_P5_MODEL_RAW_ZEALOT);
    trace_prefix();
    fprintf(trace,
        "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,"
        "\"screen\":%d,\"won\":%d,\"lost\":%d,\"outcome\":%u,"
        "\"started\":%u,\"fired\":%u,\"cursor3\":%u,\"live_source_raw\":[%u,%u,%u],"
        "\"hero\":[%d,%d,%d,%d],\"zealots_at_goal\":%d,"
        "\"goal\":[%d,%d,%d,%d],\"army\":%d,\"alive\":%d,"
        "\"switch0\":%d,\"reserve\":%u,\"fault\":%u,"
        "\"cursor\":[%d,%d],\"selected\":%d,\"mode\":%d,"
        "\"minerals\":%d,\"side_buildings\":%d,\"action_cursors\":[",
        kind, (unsigned)host_ticks, (unsigned)frame, screen, won, lost,
        race_p5.runtime.model.outcome, race_p5.runtime.model.started,
        race_p5.runtime.model.started,
        race_p5.runtime.model.action_cursor[3],
        raw_unit_count(8, -1), raw_unit_count(12, -1), raw_unit_count(101, -1),
        hero >= 0, hero >= 0 ? u[hero].x : -1,
        hero >= 0 ? u[hero].y : -1, hero >= 0 ? u[hero].owner : -1,
        zealots, l, t, r, btm, army_count(), live_units(),
        sc64_p5_switch_is_set(&race_p5.runtime, 0),
        race_p5_reserved_units(), race_p5.runtime.fault, cx, cy,
        selected_count(), mode, minerals, live_buildings(SIDE));
    for (int i = 0; i < SC64_P5_TRIGGER_COUNT; ++i) {
        if (i) fputc(',', trace);
        fprintf(trace, "%u", race_p5.runtime.model.action_cursor[i]);
    }
    fprintf(trace, "],\"action_ticks\":[");
    for (int i = 0; i < SC64_P5_TRIGGER_COUNT; ++i) {
        if (i) fputc(',', trace);
        fprintf(trace, "%u", race_p5.runtime.action_ticks[i]);
    }
    fprintf(trace, "],\"pressure_dispatched\":%u,\"source_create_actions\":[%u,%u],"
                   "\"source_deaths\":[",
            race_p5.pressure_dispatched_mask,
            SC64_P5_SOURCE_CREATE_COUNT,
            (sc64_p5_trigger_mask_test(race_p5.runtime.model.started, 3) &&
             !race_p5.runtime.model.action_cursor[3]) ?
                SC64_P5_SOURCE_CREATE_COUNT :
                SC64_P5_SOURCE_CREATE_COUNT - race_p5_remaining_creations(&race_p5));
    for (int i = 0; i < SC64_P5_SOURCE_SLOT_COUNT; ++i) {
        if (i) fputc(',', trace);
        fprintf(trace, "%u", (unsigned)race_p5.deaths_by_source[i]);
    }
    {
        int carrier = live_shuttle();
        fprintf(trace, "],\"shuttle\":[%d,%d,%d,%d,%d,%d,%d,%d]}",
                carrier >= 0,
                carrier >= 0 ? u[carrier].x : -1,
                carrier >= 0 ? u[carrier].y : -1,
                carrier >= 0 ? u[carrier].hp : 0,
                carrier >= 0 ? u[carrier].order : -1,
                carrier >= 0 ? u[carrier].tx : -1,
                carrier >= 0 ? u[carrier].ty : -1,
                carrier >= 0 ? race_transport_cargo_space(carrier) : -1);
    }
    fputc('\n', trace);
    fflush(trace);
    if (path_diagnostics) {
        int carrier = live_shuttle();
        if (carrier >= 0) {
            fprintf(stderr, "P5 checkpoint Shuttle frame=%u xy=%d,%d hp=%d order=%d tx=%d,%d cargo=%d\n",
                    (unsigned)frame, u[carrier].x, u[carrier].y, u[carrier].hp,
                    u[carrier].order, u[carrier].tx, u[carrier].ty,
                    race_transport_cargo_space(carrier));
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner != SIDE &&
                    ((u32)frame == 0 ||
                     dist2(u[i].x, u[i].y, u[carrier].x, u[carrier].y) < 280 * 280))
                    fprintf(stderr, "P5 checkpoint threat slot=%d raw=%d owner=%d xy=%d,%d hp=%d flying=%d\n",
                            i, u[i].raw_type, u[i].owner, u[i].x, u[i].y,
                            u[i].hp, sc64_race_unit_flying(u[i].type));
        }
    }
}

static void step(u16 held)
{
    if (deployed && (screen != SCREEN_PLAY || won || lost)) return;
    u16 pressed = held & (u16)~prior_keys;
    int before_carrier = live_shuttle();
    int before_hp = before_carrier >= 0 ? u[before_carrier].hp : -1;
    int before_shield = before_carrier >= 0 ? u[before_carrier].shield : -1;
    int before_x = before_carrier >= 0 ? u[before_carrier].x : -1;
    int before_y = before_carrier >= 0 ? u[before_carrier].y : -1;
    if (path_diagnostics && before_carrier >= 0) {
        for (int i = 0; i < MAX_UNITS; ++i) {
            P5ThreatSnapshot *s = &prior_threats[i];
            s->alive = u[i].alive && u[i].owner != SIDE;
            s->owner = (u8)u[i].owner; s->raw = (u8)u[i].raw_type;
            s->x = (s16)u[i].x; s->y = (s16)u[i].y;
            s->order = (s16)u[i].order; s->target = (s16)u[i].target;
            s->cd = (s16)u[i].cd;
            s->flying = (u8)sc64_race_unit_flying(u[i].type);
            s->air_range = (u8)sc64_race_unit_range_vs(u[i].type, 1);
            s->air_damage = (u8)sc64_race_unit_damage_vs(u[i].type, 1);
        }
        for (int i = 0; i < MAX_BUILDINGS; ++i) {
            P5ThreatSnapshot *s = &prior_buildings[i];
            s->alive = b[i].alive && b[i].owner != SIDE;
            s->owner = (u8)b[i].owner; s->raw = (u8)b[i].raw_type;
            s->x = (s16)b[i].x; s->y = (s16)b[i].y;
            s->order = -1; s->target = -1; s->cd = -1;
            s->flying = 0;
            int damage = 0;
            s->air_range = (u8)hostile_building_air_range(b[i].type, &damage);
            s->air_damage = (u8)damage;
        }
    }
    game_update(held, pressed);
    prior_keys = held;
    ++host_ticks;
    if (path_diagnostics && before_carrier >= 0 &&
        (u[before_carrier].hp < before_hp || u[before_carrier].shield < before_shield ||
         !u[before_carrier].alive)) {
        fprintf(stderr, "P5 Shuttle damage frame=%u slot=%d hp=%d->%d shield=%d->%d alive=%d xy=%d,%d\n",
                (unsigned)frame, before_carrier, before_hp, u[before_carrier].hp,
                before_shield, u[before_carrier].shield, u[before_carrier].alive,
                u[before_carrier].x, u[before_carrier].y);
        for (int i = 0; i < MAX_UNITS; ++i) {
            const P5ThreatSnapshot *s = &prior_threats[i];
            int reach = s->air_range + 64;
            if (s->alive && (s->target == before_carrier ||
                (s->air_range > 0 && dist2(s->x, s->y, before_x, before_y) < reach * reach)))
                fprintf(stderr, "P5 prior hostile unit=%d raw=%u owner=%u xy=%d,%d order=%d target=%d cd=%d flying=%u air_range=%u air_damage=%u\n",
                        i, s->raw, s->owner, s->x, s->y, s->order, s->target,
                        s->cd, s->flying, s->air_range, s->air_damage);
        }
        for (int i = 0; i < MAX_BUILDINGS; ++i) {
            const P5ThreatSnapshot *s = &prior_buildings[i];
            int reach = s->air_range + 64;
            if (s->alive && s->air_range > 0 &&
                dist2(s->x, s->y, before_x, before_y) < reach * reach)
                fprintf(stderr, "P5 prior hostile building=%d raw=%u owner=%u xy=%d,%d order=%d target=%d cd=%d air_range=%u air_damage=%u\n",
                        i, s->raw, s->owner, s->x, s->y, s->order, s->target,
                        s->cd, s->air_range, s->air_damage);
        }
    }
    diagnostic_carrier = live_shuttle();
    diagnostic_carrier_hp = diagnostic_carrier >= 0 ? u[diagnostic_carrier].hp : -1;
    diagnostic_carrier_shield = diagnostic_carrier >= 0 ? u[diagnostic_carrier].shield : -1;
    if (held != (u16)last_keys) {
        trace_prefix();
        fprintf(trace, "{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}\n",
                (unsigned)host_ticks, held, pressed);
        last_keys = held;
        fflush(trace);
    }
    int checkpoint_period = path_diagnostics ? 100 : P5_CHECKPOINT_PERIOD;
    if (deployed && screen == SCREEN_PLAY && frame > 0 &&
        frame % checkpoint_period == 0) checkpoint("periodic");
}

static void tap(u16 key) { step(key); step(0); }

static int snap4(int current, int target, int bound)
{
    int delta = target - current;
    int steps = delta >= 0 ? (delta + 2) / 4 : (delta - 2) / 4;
    int result = current + steps * 4;
    return result < 0 ? 0 : result >= bound ? bound - 1 : result;
}

static void move_cursor(int x, int y)
{
    x = snap4(cx, x, WORLD_W);
    y = snap4(cy, y, WORLD_H);
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
    if (selected_building >= 0) {
        move_cursor(0, 0); tap(KEY_L); tap(KEY_L);
    }
    tap(KEY_R);
    if (!selected_count()) {
        if (path_diagnostics) fprintf(stderr, "P5 no group selection frame=%u\n", (unsigned)frame);
        return 0;
    }
    tap(KEY_B);
    if (attack_move) tap(KEY_DOWN);
    tap(KEY_A);
    if (mode != (attack_move ? MODE_ATTACK : MODE_MOVE)) {
        if (path_diagnostics) fprintf(stderr, "P5 menu failed frame=%u mode=%d menu=%d sel=%d\n",
                                      (unsigned)frame, mode, menu_open, selected_count());
        return 0;
    }
    move_cursor(x, y);
    tap(KEY_A);
    if (path_diagnostics) fprintf(stderr, "P5 group order frame=%u target=%d,%d attack=%d mode=%d sel=%d\n",
                                  (unsigned)frame, x, y, attack_move, mode, selected_count());
    return 1;
}

static int issue_actor_move(int slot, int x, int y)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE)
        return 0;
    move_cursor(u[slot].x, u[slot].y);
    tap(KEY_A);
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

static void launch_from_menu(void)
{
    /* Later staged profiles retain P5 at ordinal four while adding entries. */
    assert(sc64_catalog_playable_count(SC64_FACTION_PROTOSS) >= 5);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_PROTOSS,
                                         SC64_P5_GLOBAL_ID) == 4);
    game_init(); prior_keys = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION &&
           race_front.faction == SC_FRONT_PROTOSS);
    tap(KEY_RIGHT); tap(KEY_RIGHT); /* Fifth mission: page 2, first row. */
    assert(race_front.mission_page == 2 && race_front.row == 0);
    assert(sc64_catalog_playable_at(SC64_FACTION_PROTOSS, 4) == SC64_P5_GLOBAL_ID);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING &&
           race_front.mission == SC64_P5_GLOBAL_ID);
    step(KEY_A);
    deployed = 1;
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == SC64_P5_GLOBAL_ID && race_p5_active());
    assert(race_campaign.mission == SC64_P5_GLOBAL_ID);
    assert(live_units() == SC64_P5_SOURCE_UNIT_COUNT);
}

static void promote_verified_trace(void)
{
    assert(screen == SCREEN_RESULT && won && !lost &&
           race_selected_mission == SC64_P5_GLOBAL_ID &&
           race_p5.runtime.model.outcome == SC64_P5_WON &&
           (race_completed & (1 << SC64_P5_GLOBAL_ID)) != 0);
    if (rename(attempt_path, verified_path) != 0) {
        FILE *src = fopen(attempt_path, "rb"), *dst = fopen(verified_path, "wb");
        assert(src && dst);
        char buffer[4096]; size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), src)) != 0)
            assert(fwrite(buffer, 1, n, dst) == n);
        assert(!ferror(src)); fclose(src); fclose(dst);
        remove(attempt_path);
    }
}

int main(int argc, char **argv)
{
    const char *tag = NULL;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--limit") && i + 1 < argc) run_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--attempt-tag") && i + 1 < argc) tag = argv[++i];
        else if (!strcmp(argv[i], "--path-diagnostics")) path_diagnostics = 1;
        else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_p5_campaign_run [--limit TICKS] [--attempt-tag TAG] [--path-diagnostics]");
            return 0;
        } else return 2;
    }
    if (run_limit < 1 || run_limit > 1000000) return 2;
    if (tag) {
        for (const char *p = tag; *p; ++p)
            if (!( (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                   (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' )) return 2;
        if (snprintf(attempt_path, sizeof(attempt_path),
                     ".local/sc64/p5-controller-%s-running.json", tag) >=
                (int)sizeof(attempt_path) ||
            snprintf(final_path, sizeof(final_path),
                     ".local/sc64/p5-controller-%s.json", tag) >=
                (int)sizeof(final_path)) return 2;
    }
    (void)path_diagnostics;
    mkdir(".local", 0777); mkdir(".local/sc64", 0777);
    trace = fopen(attempt_path, "w"); assert(trace);
    fprintf(trace, "{\"schema\":1,\"mission\":24,\"profile\":\"protoss-p5\","
                   "\"policy\":\"normal-input-rescue-and-extract\",\"events\":[\n");
    gba_init(); launch_from_menu(); checkpoint("start");

    if (path_diagnostics) {
        int tx = -1, ty = -1;
        int reachable = terrain_waypoint_to_objective(u[0].x, u[0].y, &tx, &ty);
        fprintf(stderr, "P5 ground terrain route start=%d,%d reachable=%d waypoint=%d,%d\n",
                u[0].x, u[0].y, reachable, tx, ty);
        static const int probes[][2] = {{2038,1446},{2038,1980},{1760,1980},{1800,1500}};
        for (unsigned i = 0; i < sizeof(probes)/sizeof(probes[0]); ++i) {
            int x = probes[i][0], y = probes[i][1], wx = -1, wy = -1;
            int r = terrain_waypoint_to_objective(x, y, &wx, &wy);
            fprintf(stderr, "P5 ground route probe=%d,%d reach=%d waypoint=%d,%d blocked=%d walk=%d\n",
                    x, y, r, wx, wy, terrain_blocked(x/16,y/16),
                    SC64_GRID_GET(path_walkable,x/16+(y/16)*MAP_COLS));
        }
    }

    int landing_x = -1, landing_y = -1;
    assert(choose_landing_point(&landing_x, &landing_y));
    int air_waypoint_stage = 0;
    while ((u32)frame < (u32)run_limit && screen == SCREEN_PLAY && !won && !lost) {
        int hero_side = live_raw_unit(SC64_P5_RAW_HERO, SIDE);
        int hero_rescue = live_raw_unit(SC64_P5_RAW_HERO, RESCUABLE);
        int carrier = live_shuttle();
        if (frame >= next_order_frame) {
            if (carrier < 0) {
                if (path_diagnostics) fprintf(stderr, "P5 Shuttle lost frame=%u\n", (unsigned)frame);
                next_order_frame = frame + 300;
            } else if (hero_side < 0 && hero_rescue >= 0) {
                /* Use the source Shuttle to make the ordinary 32 px rescue
                 * contact, then load the actual hero and two source Zealots. */
                if (u[carrier].order != ORDER_MOVE ||
                    dist2(u[carrier].tx, u[carrier].ty,
                          u[hero_rescue].x, u[hero_rescue].y) > 24 * 24) {
                    fprintf(stderr, "P5 Shuttle contacts Tassadar frame=%u at=%d,%d\n",
                            (unsigned)frame, u[hero_rescue].x, u[hero_rescue].y);
                    (void)issue_move_actor(carrier, u[hero_rescue].x, u[hero_rescue].y);
                }
                next_order_frame = frame + 60;
            } else if (hero_side >= 0 && loaded_raw_count(SC64_P5_RAW_HERO, carrier) == 0) {
                if (u[hero_side].order != ORDER_BOARD || u[hero_side].target != carrier) {
                    fprintf(stderr, "P5 board Tassadar frame=%u cargo=%d\n",
                            (unsigned)frame, race_transport_cargo_space(carrier));
                    (void)issue_board_actor(hero_side, carrier);
                }
                next_order_frame = frame + 30;
            } else if (loaded_raw_count(SC64_P5_RAW_HERO, carrier) > 0 &&
                       loaded_raw_count(SC64_P5_MODEL_RAW_ZEALOT, carrier) < 2) {
                int boarded = loaded_raw_count(SC64_P5_MODEL_RAW_ZEALOT, carrier) +
                              boarding_raw_count(SC64_P5_MODEL_RAW_ZEALOT, carrier);
                if (boarded < 2) {
                    int zealot = find_boardable_zealot(carrier);
                    if (zealot >= 0) {
                        fprintf(stderr, "P5 board Zealot slot=%d frame=%u cargo=%d\n",
                                zealot, (unsigned)frame, race_transport_cargo_space(carrier));
                        (void)issue_board_actor(zealot, carrier);
                    }
                }
                next_order_frame = frame + 30;
            } else if (loaded_raw_count(SC64_P5_RAW_HERO, carrier) > 0 &&
                       loaded_raw_count(SC64_P5_MODEL_RAW_ZEALOT, carrier) >= 2) {
                /* Go north of the eastern Spore line, descend west of the
                 * x1488 colony, then thread the y1792 gap into extraction. */
                static const int route_x[9] = {40,40,2040,2040,1360,1360,1400,1400,1696};
                static const int route_y[9] = {755,40,40,1200,1200,1600,1600,1792,1792};
                int tx = air_waypoint_stage < 9 ? route_x[air_waypoint_stage] : landing_x;
                int ty = air_waypoint_stage < 9 ? route_y[air_waypoint_stage] : landing_y;
                if (air_waypoint_stage < 9 &&
                    dist2(u[carrier].x, u[carrier].y, tx, ty) <= 8 * 8) {
                    ++air_waypoint_stage;
                    tx = air_waypoint_stage < 9 ? route_x[air_waypoint_stage] : landing_x;
                    ty = air_waypoint_stage < 9 ? route_y[air_waypoint_stage] : landing_y;
                }
                if (dist2(u[carrier].x, u[carrier].y, landing_x, landing_y) > 8 * 8 &&
                    (u[carrier].order != ORDER_MOVE ||
                     dist2(u[carrier].tx, u[carrier].ty, tx, ty) > 8 * 8)) {
                    fprintf(stderr, "P5 fly extraction frame=%u at=%d,%d target=%d,%d\n",
                            (unsigned)frame, u[carrier].x, u[carrier].y, tx, ty);
                    (void)issue_move_actor(carrier, tx, ty);
                } else if (dist2(u[carrier].x, u[carrier].y, landing_x, landing_y) <= 8 * 8 &&
                           u[carrier].order != ORDER_UNLOAD) {
                    fprintf(stderr, "P5 unload extraction frame=%u cargo=%d at=%d,%d\n",
                            (unsigned)frame, race_transport_cargo_space(carrier),
                            u[carrier].x, u[carrier].y);
                    (void)issue_shuttle_unload(carrier, landing_x, landing_y);
                }
                next_order_frame = frame + 60;
            }
        }
        step(0);
    }
    checkpoint(won ? "won" : lost ? "lost" : "limit");
    fprintf(trace, "]}\n"); fclose(trace); trace = NULL;
    if (won && screen == SCREEN_RESULT && race_p5.runtime.model.outcome == SC64_P5_WON &&
        race_selected_mission == SC64_P5_GLOBAL_ID &&
        (race_completed & (1 << SC64_P5_GLOBAL_ID))) promote_verified_trace();
    else assert(rename(attempt_path, final_path) == 0);
    printf("P5 controller attempt result=%s frame=%u outcome=%u army=%d trace=%s\n",
           won ? "WON" : lost ? "LOST" : "LIMIT", (unsigned)frame,
           race_p5.runtime.model.outcome, army_count(),
           won ? verified_path : final_path);
    if (path_diagnostics && !won) {
        int carrier = live_shuttle();
        if (carrier >= 0) {
            fprintf(stderr, "P5 Shuttle final slot=%d xy=%d,%d hp=%d order=%d tx=%d,%d cargo=%d\n",
                    carrier, u[carrier].x, u[carrier].y, u[carrier].hp,
                    u[carrier].order, u[carrier].tx, u[carrier].ty,
                    race_transport_cargo_space(carrier));
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner != SIDE &&
                    dist2(u[i].x, u[i].y, u[carrier].x, u[carrier].y) < 320 * 320)
                    fprintf(stderr, "P5 nearby hostile slot=%d raw=%d owner=%d xy=%d,%d hp=%d type=%d\n",
                            i, u[i].raw_type, u[i].owner, u[i].x, u[i].y,
                            u[i].hp, u[i].type);
        } else fprintf(stderr, "P5 Shuttle dead\n");
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && (u[i].owner == SIDE || u[i].owner == RESCUABLE))
                fprintf(stderr, "P5 actor slot=%d raw=%d owner=%d at=%d,%d order=%d target=%d,%d path=%d,%d retry=%d\n",
                        i, u[i].raw_type, u[i].owner, u[i].x, u[i].y,
                        u[i].order, u[i].tx, u[i].ty, u[i].path_x,
                        u[i].path_y, u[i].path_retry);
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 66) {
            int r = unit_range(u[i].type) > 64 ? unit_range(u[i].type) + 18 : 64;
            int target = nearest_enemy_typed(u[i].x,u[i].y,u[i].owner,r,u[i].type
                                            SC64_P7_SOURCE_OWNER_ARG(u[i].raw_owner));
            fprintf(stderr,"P5 Dragoon target-query slot=%d xy=%d,%d radius=%d target=%d",
                    i,u[i].x,u[i].y,r,target);
            if (target >= 0) fprintf(stderr," raw=%d xy=%d,%d",target<MAX_UNITS?u[target].raw_type:b[target-MAX_UNITS].raw_type,
                                    target<MAX_UNITS?u[target].x:b[target-MAX_UNITS].x,
                                    target<MAX_UNITS?u[target].y:b[target-MAX_UNITS].y);
            fputc('\n',stderr);
        }
        for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].raw_type == 144 && b[i].owner != SIDE)
            fprintf(stderr,"P5 Spore slot=%d xy=%d,%d hp=%d flags=%u owner=%d\n",
                    i,b[i].x,b[i].y,b[i].hp,b[i].flags,b[i].owner);
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
                                                    !sc64_race_is_worker(u[i].type)) {
            int nx = -1, ny = -1;
            int ok = sc64_route_next(u[i].x, u[i].y, u[i].tx, u[i].ty, &nx, &ny);
            fprintf(stderr, "P5 route unit=%d pos=%d,%d goal=%d,%d route=%d next=%d,%d cellwalk=%d goalwalk=%d terrain=%d/%d\n",
                    i, u[i].x, u[i].y, u[i].tx, u[i].ty, ok, nx, ny,
                    SC64_GRID_GET(path_walkable, u[i].x / 16 + u[i].y / 16 * MAP_COLS),
                    SC64_GRID_GET(path_walkable, u[i].tx / 16 + u[i].ty / 16 * MAP_COLS),
                    terrain_blocked(u[i].x / 16, u[i].y / 16),
                    terrain_blocked(u[i].tx / 16, u[i].ty / 16));
            break;
        }
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].raw_owner == 6)
                fprintf(stderr, "P5 source6 building slot=%d raw=%d xy=%d,%d hp=%d owner=%d\n",
                        i, b[i].raw_type, b[i].x, b[i].y, b[i].hp, b[i].owner);
    }
    return won && screen == SCREEN_RESULT &&
           race_p5.runtime.model.outcome == SC64_P5_WON &&
           race_selected_mission == SC64_P5_GLOBAL_ID &&
           (race_completed & (1 << SC64_P5_GLOBAL_ID)) ? 0 : 1;
}
