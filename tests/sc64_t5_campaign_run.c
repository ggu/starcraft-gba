/* Bounded normal-controller attempt for the private T5 mission probe.
 * After game_init performs the compile-gated probe entry, all gameplay changes
 * are made through game_update(held, pressed); this fixture is not a runtime
 * state injection test or a hardware performance proof. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define T5_RUN_LIMIT_DEFAULT 30000
#define T5_CHECKPOINT_PERIOD 1000

static FILE *trace_file;
static const char *trace_path;
static u16 prior_keys;
static int last_logged_keys = -1;
static int trace_first = 1;
static int run_limit = T5_RUN_LIMIT_DEFAULT;
static u32 host_ticks;
static void checkpoint(const char *kind);
static void tap(u16 key);
static void move_cursor(int x, int y);
static int dump_final;
static int reachable_location_distance(int sx, int sy, unsigned location,
                                       int *out_x, int *out_y)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS];
    static u16 distance[MAP_CELLS];
    int start = sx / 16 + (sy / 16) * MAP_COLS;
    int head = 0, tail = 0;
    if (!out_x || !out_y || start < 0 || start >= MAP_CELLS || !location ||
        location > sc64_t5_scenario.location_count) return -1;
    const Sc64T4Location *r = &sc64_t5_scenario.locations[location - 1];
    build_path_grid();
    memset(seen, 0, sizeof(seen)); memset(distance, 0, sizeof(distance));
    seen[start] = 1; queue[tail++] = (u16)start;
    while (head < tail) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
        int px = col * 16 + 8, py = row * 16 + 8;
        if (px * 2 >= (int)r->left && px * 2 < (int)r->right &&
            py * 2 >= (int)r->top && py * 2 < (int)r->bottom) {
            *out_x = px; *out_y = py; return distance[cell];
        }
#define T5_ROUTE_NEXT(n_) do { int z_=(n_); if (!seen[z_] && SC64_GRID_GET(path_walkable,z_)) { seen[z_]=1; distance[z_]=(u16)(distance[cell]+1); queue[tail++]=(u16)z_; } } while (0)
        if (col > 0) T5_ROUTE_NEXT(cell - 1);
        if (col + 1 < MAP_COLS) T5_ROUTE_NEXT(cell + 1);
        if (row > 0) T5_ROUTE_NEXT(cell - MAP_COLS);
        if (row + 1 < MAP_ROWS) T5_ROUTE_NEXT(cell + MAP_COLS);
#undef T5_ROUTE_NEXT
    }
    return -1;
}

static int reachable_actor_location_point(int slot, unsigned location,
                                         int *out_x, int *out_y)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS], distance[MAP_CELLS];
    int head = 0, tail = 0, best = -1, best_d = 0x7fffffff;
    if (!out_x || !out_y || slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        !location || location > sc64_t5_scenario.location_count) return -1;
    int start = u[slot].x / 16 + (u[slot].y / 16) * MAP_COLS;
    const Sc64T4Location *r = &sc64_t5_scenario.locations[location - 1];
    build_path_grid();
    memset(seen, 0, sizeof(seen)); memset(distance, 0, sizeof(distance));
    seen[start] = 1; queue[tail++] = (u16)start;
    while (head < tail) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
        int px = col * 16 + 8, py = row * 16 + 8;
        if (px * 2 >= (int)r->left && px * 2 < (int)r->right &&
            py * 2 >= (int)r->top && py * 2 < (int)r->bottom &&
            unit_step_clear(&u[slot], px, py)) {
            int d = dist2(px, py, (int)((r->left + r->right) / 4),
                           (int)((r->top + r->bottom) / 4));
            if (d < best_d) { best = cell; best_d = d; }
        }
#define T5_ACTOR_NEXT(n_) do { int z_=(n_); if (!seen[z_] && SC64_GRID_GET(path_walkable,z_)) { seen[z_]=1; distance[z_]=(u16)(distance[cell]+1); queue[tail++]=(u16)z_; } } while (0)
        if (col > 0) T5_ACTOR_NEXT(cell - 1);
        if (col + 1 < MAP_COLS) T5_ACTOR_NEXT(cell + 1);
        if (row > 0) T5_ACTOR_NEXT(cell - MAP_COLS);
        if (row + 1 < MAP_ROWS) T5_ACTOR_NEXT(cell + MAP_COLS);
#undef T5_ACTOR_NEXT
    }
    if (best < 0) return -1;
    *out_x = (best % MAP_COLS) * 16 + 8;
    *out_y = (best / MAP_COLS) * 16 + 8;
    return distance[best];
}

static int owner_units(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++count;
    return count;
}

static int first_side_unit(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && sc64_t4_unit_selectable(u[i].type))
            return i;
    return -1;
}

static int owner_buildings(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) ++count;
    return count;
}

static int nearest_enemy_approach(int *out_x, int *out_y)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS], distance[MAP_CELLS];
    int lead = first_side_unit(), head = 0, tail = 0;
    if (!out_x || !out_y || lead < 0) return -1;
    int start = u[lead].x / 16 + (u[lead].y / 16) * MAP_COLS;
    if (start < 0 || start >= MAP_CELLS) return -1;
    build_path_grid(); memset(seen, 0, sizeof(seen));
    memset(distance, 0, sizeof(distance)); seen[start] = 1; queue[tail++] = (u16)start;
    while (head < tail) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
        int px = col * 16 + 8, py = row * 16 + 8;
        for (int bi = 0; bi < MAX_BUILDINGS; ++bi) if (b[bi].alive && b[bi].owner == ENEMY) {
            int d = dist2(px, py, b[bi].x, b[bi].y);
            /* Path grid already excludes the building footprint. Stay within
             * the normal weapon approach band instead of routing onto it. */
            if (d >= 18 * 18 && d <= 40 * 40) {
                *out_x = px; *out_y = py; return distance[cell];
            }
        }
        for (int ui = 0; ui < MAX_UNITS; ++ui) if (u[ui].alive && u[ui].owner == ENEMY) {
            int d = dist2(px, py, u[ui].x, u[ui].y);
            if (d >= 12 * 12 && d <= 40 * 40) {
                *out_x = px; *out_y = py; return distance[cell];
            }
        }
#define T5_APPROACH_NEXT(n_) do { int z_=(n_); if (!seen[z_] && SC64_GRID_GET(path_walkable,z_)) { seen[z_]=1; distance[z_]=(u16)(distance[cell]+1); queue[tail++]=(u16)z_; } } while (0)
        if (col > 0) T5_APPROACH_NEXT(cell - 1);
        if (col + 1 < MAP_COLS) T5_APPROACH_NEXT(cell + 1);
        if (row > 0) T5_APPROACH_NEXT(cell - MAP_COLS);
        if (row + 1 < MAP_ROWS) T5_APPROACH_NEXT(cell + MAP_COLS);
#undef T5_APPROACH_NEXT
    }
    return -1;
}

static int kerrigan_alive(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_KERRIGAN) return 1;
    return 0;
}

static void controller_queue_paid_units(void)
{
    for (int i = 0; i < MAX_BUILDINGS && host_ticks < (u32)run_limit; ++i) {
        int type = -1;
        if (!b[i].alive || b[i].owner != SIDE || b[i].under_construction ||
            b[i].queue_count >= 2) continue;
        if (b[i].type == B_COMMAND) type = U_SCV;
        else if (b[i].type == B_BARRACKS) type = U_MARINE;
        if (type < 0 || minerals < unit_cost(type) ||
            reserved_supply() + unit_supply(type) > supply_cap) continue;
        fprintf(stderr, "T5 controller queues paid unit=%d at SIDE producer slot=%d wallet=%d supply=%d/%d\n",
                type, i, minerals, reserved_supply(), supply_cap);
        move_cursor(b[i].x, b[i].y);
        tap(KEY_A); /* select completed producer */
        tap(KEY_A); /* queue through normal interaction */
    }
    checkpoint("paid-production-orders");
}

static void trace_prefix(void)
{
    if (!trace_first) fputs(",\n", trace_file);
    trace_first = 0;
}

static void step(u16 held)
{
    if (screen != SCREEN_PLAY || won || lost) return;
    u16 pressed = held & (u16)~prior_keys;
    game_update(held, pressed);
    prior_keys = held;
    ++host_ticks;
    if (trace_file && held != (u16)last_logged_keys) {
        trace_prefix();
        fprintf(trace_file,
                "{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}",
                (unsigned)host_ticks, held, pressed);
        last_logged_keys = held;
    }
}

static void tap(u16 key)
{
    step(key);
    step(0);
}

static int clamp_world(int value, int limit_value)
{ return value < 0 ? 0 : value >= limit_value ? limit_value - 1 : value; }

static int snap4_local(int current, int target, int bound)
{
    int delta = target - current;
    int steps = delta >= 0 ? (delta + 2) / 4 : (delta - 2) / 4;
    return clamp_world(current + steps * 4, bound);
}

static void move_cursor(int x, int y)
{
    x = snap4_local(cx, x, WORLD_W);
    y = snap4_local(cy, y, WORLD_H);
    int guard = 0;
    while ((cx != x || cy != y) && guard++ < 2000 && host_ticks < (u32)run_limit) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT;
        if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        if (cy > y) held |= KEY_UP;
        step(held);
    }
    step(0);
}

static int select_human_group(void)
{
    int left = WORLD_W, top = WORLD_H, right = -1, bottom = -1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        sc64_t4_unit_selectable(u[i].type)) {
        if (u[i].x < left) left = u[i].x;
        if (u[i].x > right) right = u[i].x;
        if (u[i].y < top) top = u[i].y;
        if (u[i].y > bottom) bottom = u[i].y;
    }
    if (right < left || bottom < top) return 0;
    left = clamp_world(left - 8, WORLD_W); top = clamp_world(top - 8, WORLD_H);
    right = clamp_world(right + 8, WORLD_W); bottom = clamp_world(bottom + 8, WORLD_H);
    move_cursor(left, top);
    step(KEY_L);
    int end_x = snap4_local(cx, right, WORLD_W);
    int end_y = snap4_local(cy, bottom, WORLD_H);
    for (int guard = 0; (cx != end_x || cy != end_y) && guard < 1000; ++guard) {
        u16 held = KEY_L | KEY_R;
        if (cx < end_x) held |= KEY_RIGHT;
        if (cx > end_x) held |= KEY_LEFT;
        if (cy < end_y) held |= KEY_DOWN;
        if (cy > end_y) held |= KEY_UP;
        step(held);
    }
    step(0);
    int count = 0;
    if (selected_building >= 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
        if (u[i].owner != SIDE || !sc64_t4_unit_selectable(u[i].type)) return 0;
        ++count;
    }
    return count > 0 && count == selected_count();
}

static int command_group_to(int x, int y, int attack_move)
{
    if (!select_human_group()) return 0;
    tap(KEY_B);
    if (attack_move) tap(KEY_DOWN);
    tap(KEY_A);
    move_cursor(x, y);
    tap(KEY_A);
    return 1;
}

static int command_actor_to(int slot, int x, int y)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE)
        return 0;
    int selected = 0;
    for (int retry = 0; retry <= MAX_UNITS && screen == SCREEN_PLAY &&
         !won && !lost; ++retry) {
        if (u[slot].selected && selected_count() == 1 && selected_building < 0) {
            selected = 1; break;
        }
        tap(KEY_L);
    }
    if (!selected) {
        fprintf(stderr, "T5 single selection failed slot=%d selected=%d count=%d building=%d\n",
                slot, u[slot].selected, selected_count(), selected_building);
        return 0;
    }
    tap(KEY_B); tap(KEY_A);
    move_cursor(x, y);
    tap(KEY_A);
    return 1;
}

static int source_location_center(unsigned id, int *x, int *y)
{
    if (!x || !y || !id || id > sc64_t5_scenario.location_count) return 0;
    const Sc64T4Location *loc = &sc64_t5_scenario.locations[id - 1];
    if (loc->left >= loc->right || loc->top >= loc->bottom) return 0;
    *x = (int)((loc->left + loc->right) / 4);
    *y = (int)((loc->top + loc->bottom) / 4);
    return 1;
}

static void wait_frames(int amount)
{
    for (int i = 0; i < amount && screen == SCREEN_PLAY && !won && !lost &&
         host_ticks < (u32)run_limit; ++i) {
        step(0);
        if (trace_file && host_ticks && host_ticks % T5_CHECKPOINT_PERIOD == 0)
            checkpoint("periodic");
    }
}

static void checkpoint(const char *kind)
{
    unsigned faults = 0;
    for (unsigned p = 0; p < 8; ++p) faults += race_t5.player[p].fault != 0;
    if (trace_file) {
        trace_prefix();
        fprintf(trace_file,
            "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%d,\"elapsed\":%u,"
            "\"screen\":%d,\"race_mode\":%d,\"race_selected_mission\":%d,"
            "\"won\":%d,\"lost\":%d,\"faults\":%u,\"switch4\":%u,\"switch6\":%u,"
            "\"objective\":%u,\"side_units\":%d,\"enemy_units\":%d,"
            "\"side_buildings\":%d,\"enemy_buildings\":%d,\"kerrigan_alive\":%d,"
            "\"minerals\":%d,\"gas\":%d,\"enemy_minerals\":%d,\"enemy_gas\":%d}",
            kind, (unsigned)host_ticks, frame, (unsigned)race_campaign.elapsed_frames,
            screen, race_mode, race_selected_mission, won, lost, faults,
            !!(race_t5.switches.bits[0] & 0x10),
            !!(race_t5.switches.bits[0] & 0x40), race_campaign.objective_stage,
            owner_units(SIDE), owner_units(ENEMY), owner_buildings(SIDE),
            owner_buildings(ENEMY), kerrigan_alive(), minerals, gas,
            race_minerals[ENEMY], race_gas[ENEMY]);
        fflush(trace_file);
    }
    fprintf(stderr,
        "T5 %s tick=%u frame=%d result=%s switch4=%u switch6=%u "
        "units=%d/%d buildings=%d/%d kerrigan=%d wallet=%d/%d enemy=%d/%d faults=%u\n",
        kind, (unsigned)host_ticks, frame, won ? "WON" : lost ? "LOST" : "RUNNING",
        !!(race_t5.switches.bits[0] & 0x10),
        !!(race_t5.switches.bits[0] & 0x40), owner_units(SIDE), owner_units(ENEMY),
        owner_buildings(SIDE), owner_buildings(ENEMY), kerrigan_alive(),
        minerals, gas, race_minerals[ENEMY], race_gas[ENEMY], faults);
    for (unsigned p = 0; p < 8; ++p) if (race_t5.player[p].fault)
        fprintf(stderr, "  trigger fault player=%u code=%u trigger=%u slot=%u\n",
                p, race_t5.player[p].fault, race_t5.player[p].fault_trigger,
                race_t5.player[p].fault_slot);
}

int main(int argc, char **argv)
{
    int inspect = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--limit") && i + 1 < argc) run_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc) trace_path = argv[++i];
        else if (!strcmp(argv[i], "--inspect")) inspect = 1;
        else if (!strcmp(argv[i], "--dump-final")) dump_final = 1;
        else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_t5_campaign_run [--limit TICKS] [--trace PATH]");
            return 0;
        } else return 2;
    }
    if (run_limit < 1 || run_limit > 200000) return 2;
    game_init();
    assert(screen == SCREEN_PLAY && !race_mode &&
           race_selected_mission == SC64_T5_GLOBAL_ID && race_t5_active());
    assert(race_t5.active_mask == SC64_T5_ACTIVE_SOURCE_MASK &&
           !mission_unsupported_count && !won && !lost);
    assert(race_cols == 96 && race_rows == 96);

    if (inspect) {
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive)
            fprintf(stderr, "unit slot=%d owner=%u rawowner=%d raw=%d type=%d pos=%d,%d hp=%d flags=%u\n",
                    i, u[i].owner, u[i].raw_owner, u[i].raw_type, u[i].type,
                    u[i].x, u[i].y, u[i].hp, u[i].flags);
        for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive)
            fprintf(stderr, "building slot=%d owner=%u rawowner=%d raw=%d type=%d pos=%d,%d hp=%d\n",
                    i, b[i].owner, b[i].raw_owner, b[i].raw_type, b[i].type,
                    b[i].x, b[i].y, b[i].hp);
        for (unsigned i = 0; i < sc64_t5_scenario.location_count; ++i) {
            const Sc64T4Location *loc = &sc64_t5_scenario.locations[i];
            fprintf(stderr, "location id=%u bounds=%u,%u..%u,%u flags=%u\n", i + 1,
                    loc->left, loc->top, loc->right, loc->bottom, loc->elevation);
        }
        for (unsigned ti = 0; ti < sc64_t5_scenario.trigger_count; ++ti) {
            const Sc64T4Trigger *tr = &sc64_t5_scenario.triggers[ti];
            fprintf(stderr, "trigger=%u enabled0=%u enabled3=%u enabled6=%u\n", ti,
                    tr->enabled_players[0], tr->enabled_players[3], tr->enabled_players[6]);
            for (unsigned j = 0; j < 16; ++j) {
                const Sc64T4Condition *c = &tr->conditions[j];
                if (c->opcode) fprintf(stderr,
                    "  cond%u op=%u player=%u unit=%u cmp=%u res=%u amount=%u loc=%u flags=%u mask=%u\n",
                    j,c->opcode,c->player,c->unit,c->comparison,c->resource,c->amount,c->location,c->flags,c->mask);
            }
            for (unsigned j = 0; j < 64; ++j) {
                const Sc64T4Action *a = &tr->actions[j];
                if (a->opcode) fprintf(stderr,
                    "  act%u op=%u player=%u unit=%u mod=%u arg=%u loc=%u flags=%u time=%u string=%u\n",
                    j,a->opcode,a->player,a->unit,a->modifier,a->argument,a->location,a->flags,a->time,a->string_id);
            }
        }
        return 0;
    }

    if (trace_path) {
        trace_file = fopen(trace_path, "wb");
        if (!trace_file) return 2;
        fprintf(trace_file,
                "{\"trace_schema\":1,\"entry\":\"t5-probe-entry\","
                "\"policy\":\"bounded-normal-controller-quest-economy-ground-attack\","
                "\"mission\":8,\"tick_limit\":%d,\"events\":[\n",
                run_limit);
    }
    checkpoint("start");
    wait_frames(500); /* let the source-ordered opening triggers finish */
    checkpoint("opening");
    int hero = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_KERRIGAN) { hero = i; break; }
    if (hero >= 0 && u[hero].owner != SIDE && screen == SCREEN_PLAY && !won && !lost) {
        fprintf(stderr, "T5 controller moves initial force toward Kerrigan slot=%d at %d,%d\n",
                hero, u[hero].x, u[hero].y);
        int issued = command_group_to(u[hero].x, u[hero].y, 0);
        fprintf(stderr, "T5 rescue move issued=%d selected=%d\n", issued, selected_count());
        wait_frames(4000);
        checkpoint("rescue-attempt");
    }
    /* The map-authored objective places the Antigan Command Center at loc1.
     * Bring the rescued hero there before attempting the gated Force2 grant. */
    for (unsigned retry = 0; retry < 8 && !(race_t5.switches.bits[0] & 0x10) &&
         screen == SCREEN_PLAY && !won && !lost && host_ticks < (u32)run_limit; ++retry) {
        int lead = first_side_unit(), x = -1, y = -1;
        int distance = lead >= 0 ? reachable_location_distance(u[lead].x, u[lead].y,
                                                               1, &x, &y) : -1;
        if (distance < 0) {
            fprintf(stderr, "T5 no reachable ground point in Command Center location1\n");
            break;
        }
        fprintf(stderr, "T5 controller escorts to Command Center retry=%u distance=%d point=%d,%d\n",
                retry, distance, x, y);
        if (!command_group_to(x, y, 1)) break;
        wait_frames(1400);
    }
    if (trace_file || dump_final) checkpoint("command-center-attempt");
    int kerrigan = -1, ccx = -1, ccy = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_KERRIGAN) { kerrigan = i; break; }
    /* A group escort can crowd the narrow walkable approach to the source
     * Command Center region. If the first attempt did not activate the
     * trigger, move the force clear with an ordinary MOVE order before the
     * single-actor approach below. */
    if (kerrigan >= 0 && !(race_t5.switches.bits[0] & 0x10) &&
        screen == SCREEN_PLAY && !won && !lost && host_ticks < (u32)run_limit) {
        fprintf(stderr, "T5 clears the Command Center approach with a normal group MOVE to the starting base\n");
        if (command_group_to(128, 1360, 0)) wait_frames(4000);
        checkpoint("command-center-approach-cleared");
    }
    if (kerrigan >= 0 && !(race_t5.switches.bits[0] & 0x10) &&
        source_location_center(1, &ccx, &ccy) && screen == SCREEN_PLAY && !won && !lost) {
        fprintf(stderr, "T5 normal single-actor correction for Kerrigan at %d,%d toward loc1=%d,%d\n",
                u[kerrigan].x, u[kerrigan].y, ccx, ccy);
        (void)command_actor_to(kerrigan, ccx, ccy);
        wait_frames(1600);
        checkpoint("kerrigan-center-correction");
    }
    for (unsigned retry = 0; kerrigan >= 0 && retry < 6 &&
         !(race_t5.switches.bits[0] & 0x10) && screen == SCREEN_PLAY && !won && !lost &&
         host_ticks < (u32)run_limit; ++retry) {
        int x = -1, y = -1;
        if (!u[kerrigan].alive || u[kerrigan].owner != SIDE) break;
        int distance = reachable_actor_location_point(kerrigan, 1, &x, &y);
        if (distance < 0) {
            fprintf(stderr, "T5 Kerrigan has no reachable cell inside loc1\n");
            break;
        }
        fprintf(stderr, "T5 Kerrigan loc1 retry=%u distance=%d point=%d,%d current=%d,%d\n",
                retry, distance, x, y, u[kerrigan].x, u[kerrigan].y);
        if (!command_actor_to(kerrigan, x, y)) break;
        wait_frames(1000);
    }
    if ((race_t5.switches.bits[0] & 0x10) && screen == SCREEN_PLAY && !won && !lost &&
        host_ticks < (u32)run_limit) {
        for (unsigned retry = 0; retry < 8 && !(race_t5.switches.bits[0] & 0x40) &&
             screen == SCREEN_PLAY && !won && !lost && host_ticks < (u32)run_limit; ++retry) {
            int lead = first_side_unit(), x = -1, y = -1;
            int distance = lead >= 0 ? reachable_location_distance(u[lead].x, u[lead].y,
                                                                  18, &x, &y) : -1;
            if (distance < 0) {
                fprintf(stderr, "T5 no reachable ground point in location18 from SIDE units\n");
                break;
            }
            fprintf(stderr, "T5 controller region18 retry=%u reachable=%d target=%d,%d\n",
                    retry, distance, x, y);
            if (!command_group_to(x, y, 0)) break;
            wait_frames(1200);
        }
        if (trace_file || dump_final) checkpoint("enemy-grant-attempt");
        if (race_t5.switches.bits[0] & 0x40) {
            fprintf(stderr, "T5 switch6 set; Force2 grant=%d/%d\n",
                    race_minerals[ENEMY], race_gas[ENEMY]);
            wait_frames(3000);
            checkpoint("enemy-ai-started");
            int lead = first_side_unit(), x = -1, y = -1;
            int distance = lead >= 0 ? reachable_location_distance(u[lead].x,
                            u[lead].y, 4, &x, &y) : -1;
            if (distance >= 0) {
                fprintf(stderr, "T5 controller routes to source base location4 distance=%d point=%d,%d\n",
                        distance, x, y);
                (void)command_group_to(x, y, 0);
                wait_frames(8000);
                checkpoint("base-contact-attempt");
            }
        }
    }
    if (race_t5.player[0].flags[10] & SC64_T4_TR_DONE) {
        controller_queue_paid_units();
        wait_frames(3600);
        for (unsigned retry = 0; retry < 12 && owner_buildings(ENEMY) &&
             screen == SCREEN_PLAY && !won && !lost && host_ticks < (u32)run_limit; ++retry) {
            int x = -1, y = -1;
            int distance = nearest_enemy_approach(&x, &y);
            if (distance < 0) {
                fprintf(stderr, "T5 no reachable enemy-building approach cell from current force\n");
                break;
            }
            fprintf(stderr, "T5 controller attack-moves to reachable enemy-building approach at %d,%d route=%d remaining=%d\n",
                    x, y, distance, owner_buildings(ENEMY));
            if (!command_group_to(x, y, 1)) break;
            wait_frames(3500);
            checkpoint("enemy-base-attack");
        }
        for (unsigned retry = 0; retry < 3 && owner_buildings(ENEMY) &&
             screen == SCREEN_PLAY && !won && !lost && host_ticks < (u32)run_limit; ++retry) {
            int lead = first_side_unit(), x = -1, y = -1;
            int distance = lead >= 0 ? reachable_location_distance(u[lead].x,
                            u[lead].y, 18, &x, &y) : -1;
            if (distance < 0) break;
            fprintf(stderr, "T5 fallback attack-moves through reachable Force2 area route=%d target=%d,%d\n",
                    distance, x, y);
            if (!command_group_to(x, y, 1)) break;
            wait_frames(7000);
            checkpoint("force2-area-attack");
        }
    }
    while (host_ticks < (u32)run_limit && screen == SCREEN_PLAY && !won && !lost) {
        step(0);
        if (trace_file && host_ticks % T5_CHECKPOINT_PERIOD == 0)
            checkpoint("periodic");
    }
    checkpoint(won ? "won" : lost ? "lost" : "limit");
    for (unsigned p = 0; p < 8; ++p) if (race_t5.active_mask & (1u << p))
        fprintf(stderr, "T5 trigger-state player=%u flags1=%u cursor1=%u flags10=%u cursor10=%u switches=%02x,%02x cooldown=%u wait=%u\n",
                p, race_t5.player[p].flags[1], race_t5.player[p].cursor[1],
                race_t5.player[p].flags[10], race_t5.player[p].cursor[10],
                race_t5.switches.bits[0], race_t5.switches.bits[1],
                race_t5.player[p].cooldown, race_t5.player[p].wait_ticks);
    if (trace_file) {
        fputs("\n]}\n", trace_file);
        fclose(trace_file);
    }
    if (dump_final) for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && (u[i].owner == SIDE || u[i].owner == RESCUABLE))
            fprintf(stderr, "final unit slot=%d owner=%u rawowner=%d raw=%d pos=%d,%d hp=%d order=%u target=%d tx=%u,ty=%u selected=%u path=%d,%d timer=%d retry=%d\n",
                    i, u[i].owner, u[i].raw_owner, u[i].raw_type, u[i].x, u[i].y,
                    u[i].hp, u[i].order, u[i].target, u[i].tx, u[i].ty, u[i].selected,
                    u[i].path_x, u[i].path_y, u[i].path_timer, u[i].path_retry);
    if (dump_final) for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == ENEMY)
            fprintf(stderr, "enemy building slot=%d raw=%d pos=%d,%d hp=%d under_construction=%u\n",
                    i, b[i].raw_type, b[i].x, b[i].y, b[i].hp, b[i].under_construction);
    if (dump_final) for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE)
            fprintf(stderr, "SIDE building slot=%d raw=%d type=%d pos=%d,%d hp=%d under_construction=%u queue=%u\n",
                    i, b[i].raw_type, b[i].type, b[i].x, b[i].y, b[i].hp,
                    b[i].under_construction, b[i].queue_count);
    if (dump_final) for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == ENEMY)
            fprintf(stderr, "enemy unit slot=%d raw=%d pos=%d,%d hp=%d order=%u target=%d\n",
                    i, u[i].raw_type, u[i].x, u[i].y, u[i].hp, u[i].order, u[i].target);
    if (dump_final) {
        int tx, ty, slot = -1;
        if (source_location_center(18, &tx, &ty)) {
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner == SIDE) { slot = i; break; }
            if (slot >= 0) {
                int rx = -1, ry = -1;
                int distance = reachable_location_distance(u[slot].x, u[slot].y,
                                                           18, &rx, &ry);
                fprintf(stderr, "T5 reachability from slot%d=%d,%d to loc18 center=%d,%d nearest=%d at %d,%d\n",
                        slot, u[slot].x, u[slot].y, tx, ty, distance, rx, ry);
            }
        }
        if (slot >= 0) for (unsigned id = 1; id <= sc64_t5_scenario.location_count; ++id) {
            int rx = -1, ry = -1;
            int distance = reachable_location_distance(u[slot].x, u[slot].y,
                                                        id, &rx, &ry);
            if (distance >= 0)
                fprintf(stderr, "T5 reachable source location id=%u route=%d point=%d,%d\n",
                        id, distance, rx, ry);
        }
    }
    return 0;
}
