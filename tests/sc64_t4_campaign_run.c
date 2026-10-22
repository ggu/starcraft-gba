/* Bounded ordinary-controller attempt at Jacobs Installation. Enter through
 * the private HOME chord and deploy through briefing; all gameplay changes
 * after launch are ordinary game_update controller inputs. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define T4_RUN_LIMIT_DEFAULT 120000
#define T4_CHECKPOINT_PERIOD 1000

static FILE *trace_file;
static const char *trace_path;
static u16 prior_keys;
static u32 host_ticks;
static int last_logged_keys = -1;
static int trace_first = 1;
static int run_limit = T4_RUN_LIMIT_DEFAULT;
static int deployed;
static int actor_slot = -1;
static void checkpoint(const char *kind);
static u8 visited_bring[65];

static int raynor_alive(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == 20) return 1;
    return 0;
}

static int owner_units(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++count;
    return count;
}

static void trace_prefix(void)
{
    if (!trace_first) fputs(",\n", trace_file);
    trace_first = 0;
}

static void step(u16 held)
{
    if (deployed && (screen != SCREEN_PLAY || won || lost)) return;
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
    if (deployed && host_ticks % T4_CHECKPOINT_PERIOD == 0)
        checkpoint("periodic");
}

static void tap(u16 key)
{
    step(key);
    step(0);
}

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
    while ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost &&
           host_ticks < (u32)run_limit + 4096u) {
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
    left = limit(left - 8, 0, WORLD_W - 1);
    top = limit(top - 8, 0, WORLD_H - 1);
    right = limit(right + 8, 0, WORLD_W - 1);
    bottom = limit(bottom + 8, 0, WORLD_H - 1);
    move_cursor(left, top);
    step(KEY_L);
    while ((cx != snap4(cx, right, WORLD_W) || cy != snap4(cy, bottom, WORLD_H)) &&
           host_ticks < (u32)run_limit + 4096u) {
        u16 held = KEY_L | KEY_R;
        int tx = snap4(cx, right, WORLD_W), ty = snap4(cy, bottom, WORLD_H);
        if (cx < tx) held |= KEY_RIGHT;
        if (cx > tx) held |= KEY_LEFT;
        if (cy < ty) held |= KEY_DOWN;
        if (cy > ty) held |= KEY_UP;
        step(held);
    }
    step(0);
    fprintf(stderr, "T4 controller box selected %d live units\n", selected_count());
    return selected_count() > 0;
}

static int t4_location_center(unsigned location, int *x, int *y)
{
    return x && y && race_t4_effect_center_location(&race_t4, location, x, y);
}

static int actor_in_location(unsigned location)
{
    Sc64T4QueryRect area;
    if (actor_slot < 0 || actor_slot >= MAX_UNITS || !u[actor_slot].alive ||
        !race_t4_effect_location(&race_t4, location, &area)) return 0;
    return u[actor_slot].x * 2 >= area.left && u[actor_slot].x * 2 < area.right &&
           u[actor_slot].y * 2 >= area.top && u[actor_slot].y * 2 < area.bottom;
}

static int nearest_human_bring_point(int sx, int sy, const u8 *visited,
                                     int *location, int *out_x, int *out_y)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS];
    static u16 distance[MAP_CELLS];
    static u8 locations[65];
    Sc64T4QueryRect rects[65];
    int start, head = 0, tail = 0, best = -1, best_location = 0;
    u16 best_depth = 0xffff;
    int best_center_distance = 0x7fffffff;
    if (!location || !out_x || !out_y || sx < 0 || sy < 0) return 0;
    memset(locations, 0, sizeof(locations));
    for (unsigned ti = 0; ti < sc64_t4_scenario.trigger_count; ++ti) {
        const Sc64T4Trigger *trigger = &sc64_t4_scenario.triggers[ti];
        for (unsigned ci = 0; ci < 16; ++ci) {
            const Sc64T4Condition *c = &trigger->conditions[ci];
            if (!c->opcode) break;
            if (c->opcode != 3 || c->player != 0 || c->unit != 229 ||
                (c->flags & 2) || !c->location || c->location >= 64) continue;
            if (!visited || !visited[c->location]) locations[c->location] = 1;
        }
    }
    for (unsigned id = 1; id < 64; ++id)
        if (locations[id] && !race_t4_effect_location(&race_t4, id, &rects[id]))
            locations[id] = 0;
    build_path_grid();
    memset(seen, 0, sizeof(seen));
    start = sx / 16 + (sy / 16) * MAP_COLS;
    if (start < 0 || start >= MAP_CELLS) return 0;
    seen[start] = 1; distance[start] = 0; queue[tail++] = (u16)start;
    while (head < tail) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
        int px = col * 16 + 8, py = row * 16 + 8;
        for (unsigned id = 1; id < 64; ++id) if (locations[id] &&
            px * 2 >= rects[id].left && px * 2 < rects[id].right &&
            py * 2 >= rects[id].top && py * 2 < rects[id].bottom) {
            if (distance[cell] < best_depth ||
                (distance[cell] == best_depth && (!best_location || id < (unsigned)best_location))) {
                best = cell; best_depth = distance[cell]; best_location = (int)id;
            }
        }
#define T4_REACHABLE_NEXT(next_cell) do { \
        int n_ = (next_cell); \
        if (!seen[n_] && SC64_GRID_GET(path_walkable,n_)) { \
            seen[n_]=1; distance[n_]=(u16)(distance[cell]+1); queue[tail++]=(u16)n_; \
        } \
    } while (0)
        if (col > 0) T4_REACHABLE_NEXT(cell - 1);
        if (col + 1 < MAP_COLS) T4_REACHABLE_NEXT(cell + 1);
        if (row > 0) T4_REACHABLE_NEXT(cell - MAP_COLS);
        if (row + 1 < MAP_ROWS) T4_REACHABLE_NEXT(cell + MAP_COLS);
#undef T4_REACHABLE_NEXT
    }
    if (best < 0 || !best_location) return 0;
    /* Once the nearest reachable Bring rectangle is selected, target its
       reachable point closest to the rectangle center so unit clearance at
       the boundary cannot leave the front just outside the condition. */
    best_center_distance = 0x7fffffff;
    for (int qi = 0; qi < tail; ++qi) {
        int cell = queue[qi], px = (cell % MAP_COLS) * 16 + 8;
        int py = (cell / MAP_COLS) * 16 + 8;
        const Sc64T4QueryRect *r = &rects[best_location];
        if (px * 2 < r->left || px * 2 >= r->right ||
            py * 2 < r->top || py * 2 >= r->bottom) continue;
        int center_x = (r->left + r->right) / 4;
        int center_y = (r->top + r->bottom) / 4;
        int d = dist2(px, py, center_x, center_y);
        if (d < best_center_distance) { best = cell; best_center_distance = d; }
    }
    *location = best_location;
    *out_x = (best % MAP_COLS) * 16 + 8;
    *out_y = (best / MAP_COLS) * 16 + 8;
    return 1;
}

static int choose_human_actor(void)
{
    int best = -1, best_dist = 0x7fffffff;
    int gx, gy;
    if (!t4_location_center(14, &gx, &gy)) return -1;
    /* Preserve the source Raynor actor for the mission's explicit hero-loss
       gate when one is present; otherwise choose the nearest controllable man. */
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < MAX_UNITS; ++i) {
            if (!u[i].alive || u[i].owner != SIDE ||
                !race_t4_effect_men_raw(u[i].raw_type) ||
                !sc64_t4_unit_selectable(u[i].type)) continue;
            if ((pass == 0) != (u[i].raw_type == 20)) continue;
            int d = dist2(gx, gy, u[i].x, u[i].y);
            if (d < best_dist) { best = i; best_dist = d; }
        }
        if (best >= 0) break;
    }
    return best;
}

static void controller_move_actor_to(int x, int y, int attack_move)
{
    if (actor_slot < 0 || actor_slot >= MAX_UNITS || !u[actor_slot].alive) return;
    if (!u[actor_slot].selected) {
        move_cursor(u[actor_slot].x, u[actor_slot].y);
        tap(KEY_A);
    }
    fprintf(stderr, "T4 controller select cursor=%d,%d selected=%d actor_selected=%d building=%d\n",
            cx, cy, selected_count(), actor_slot >= 0 ? u[actor_slot].selected : 0,
            selected_building);
    if (selected_count() <= 0 || selected_building >= 0) return;
    tap(KEY_B);
    if (attack_move) tap(KEY_DOWN);
    tap(KEY_A); /* Ordinary unit command: MOVE or ATTACK MOVE. */
    move_cursor(x, y);
    tap(KEY_A);
    fprintf(stderr, "T4 controller order target=%d,%d actor_order=%d target=%d tx=%d,%d\n",
            x, y, u[actor_slot].order, u[actor_slot].target,
            u[actor_slot].tx, u[actor_slot].ty);
}

static void checkpoint(const char *kind)
{
    unsigned faults = 0, outcomes = 0;
    for (unsigned p = 0; p < 8; ++p) {
        if (race_t4.player[p].fault) ++faults;
        if (race_t4.player[p].outcome) ++outcomes;
    }
    if (trace_file) {
        trace_prefix();
        fprintf(trace_file,
            "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%d,\"elapsed\":%u,"
            "\"screen\":%d,\"race_mode\":%d,\"race_selected_mission\":%d,"
            "\"won\":%d,\"lost\":%d,\"outcomes\":%u,\"faults\":%u,"
            "\"switch4\":%u,\"switch7\":%u,\"objective\":%u,\"actor_slot\":%d,"
            "\"actor_alive\":%d,\"actor_x\":%d,\"actor_y\":%d,\"raynor_alive\":%d,"
            "\"human_outcome\":%u,\"side_units\":%d,\"enemy_units\":%d}",
            kind, (unsigned)host_ticks, frame, (unsigned)race_campaign.elapsed_frames,
            screen, race_mode, race_selected_mission, won, lost, outcomes, faults,
            !!(race_t4.switches.bits[0] & 0x10),
            !!(race_t4.switches.bits[0] & 0x80), race_campaign.objective_stage,
            actor_slot, actor_slot >= 0 && u[actor_slot].alive,
            actor_slot >= 0 ? u[actor_slot].x : -1,
            actor_slot >= 0 ? u[actor_slot].y : -1,
            raynor_alive(), race_t4.player[0].outcome,
            owner_units(SIDE), owner_units(ENEMY));
        fflush(trace_file);
    }
    fprintf(stderr,
        "T4 %s frame=%d elapsed=%u screen=%d result=%s switch4=%u switch7=%u "
        "actor=%d@%d,%d sides=%d/%d faults=%u outcomes=%u\n",
        kind, frame, (unsigned)race_campaign.elapsed_frames, screen,
        won ? "WON" : lost ? "LOST" : "RUNNING",
        !!(race_t4.switches.bits[0] & 0x10),
        !!(race_t4.switches.bits[0] & 0x80), actor_slot,
        actor_slot >= 0 ? u[actor_slot].x : -1,
        actor_slot >= 0 ? u[actor_slot].y : -1,
        owner_units(SIDE), owner_units(ENEMY), faults, outcomes);
    for (unsigned p = 0; p < 8; ++p)
        if (race_t4.player[p].fault)
            fprintf(stderr, "  trigger fault player=%u code=%u trigger=%u slot=%u\n",
                    p, race_t4.player[p].fault, race_t4.player[p].fault_trigger,
                    race_t4.player[p].fault_slot);
}

static int launch_gated_t4(void)
{
    game_init();
    step(KEY_START | KEY_SELECT);
    step(0);
    step(KEY_A);
    deployed = 1;
    return screen == SCREEN_PLAY && !race_mode &&
           race_selected_mission == SC64_T4_GLOBAL_ID &&
           race_campaign.mission == SC64_T4_GLOBAL_ID &&
           race_t4.active_mask == 0xdf && mission_unsupported_count == 0;
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--limit") && i + 1 < argc)
            run_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc)
            trace_path = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_t4_campaign_run [--limit FRAMES] [--trace PATH]");
            return 0;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            return 2;
        }
    }
    if (run_limit < 1 || run_limit > 500000) return 2;
    mkdir(".local", 0777);
    mkdir(".local/sc64", 0777);
    if (!trace_path) trace_path = ".local/sc64/t4-controller-attempt.json";
    trace_file = fopen(trace_path, "w");
    assert(trace_file);
    fprintf(trace_file, "{\"schema\":1,\"mission\":7,\"policy\":\"move-men-to-location14-then20\",\"events\":[\n");
    gba_init();
    prior_keys = 0;
    if (!launch_gated_t4()) {
        fprintf(stderr, "T4 gated launch failed screen=%d unsupported=%d active=%u\n",
                screen, mission_unsupported_count, race_t4.active_mask);
        fclose(trace_file);
        return 1;
    }
    actor_slot = choose_human_actor();
    fprintf(stderr, "T4 launch roster: side=%d enemy=%d buildings=%d actor=%d\n",
            owner_units(SIDE), owner_units(ENEMY), race_campaign_count_buildings(SIDE), actor_slot);
    checkpoint("start");
    select_human_group();
    if (actor_slot < 0) {
        checkpoint("no-human-men");
    } else {
        int x14, y14, x20, y20, route_x, route_y;
        assert(t4_location_center(14, &x14, &y14));
        assert(t4_location_center(20, &x20, &y20));
        for (int gate = 0; gate < 12 && frame < run_limit && screen == SCREEN_PLAY &&
             !won && !lost && !race_t4.player[0].fault; ++gate) {
            if (actor_slot < 0 || actor_slot >= MAX_UNITS || !u[actor_slot].alive)
                actor_slot = choose_human_actor();
            if (actor_slot < 0) break;
            int bring_location = 0;
            if (!nearest_human_bring_point(u[actor_slot].x, u[actor_slot].y,
                                           visited_bring, &bring_location,
                                           &route_x, &route_y)) break;
            Sc64T4QueryRect gate_rect;
            race_t4_effect_location(&race_t4, (unsigned)bring_location, &gate_rect);
            fprintf(stderr, "T4 controller route uses source Bring location %d at %d,%d rect=%d,%d..%d,%d actor=%d,%d\n",
                    bring_location, route_x, route_y, gate_rect.left/2, gate_rect.top/2,
                    gate_rect.right/2, gate_rect.bottom/2, u[actor_slot].x, u[actor_slot].y);
            controller_move_actor_to(route_x, route_y, 1);
            int deadline = frame + 4000;
            while (frame < deadline && frame < run_limit && screen == SCREEN_PLAY &&
                   !won && !lost && !actor_in_location((unsigned)bring_location))
                step(0);
            if (actor_in_location((unsigned)bring_location)) {
                visited_bring[bring_location] = 1;
                int settle = frame + 90;
                while (frame < settle && frame < run_limit && screen == SCREEN_PLAY &&
                       !won && !lost) step(0);
            }
        }
        if (screen == SCREEN_PLAY && !won && !lost &&
            path_next_tile(u[actor_slot].x, u[actor_slot].y, x14, y14,
                           &route_x, &route_y)) {
            controller_move_actor_to(x14, y14, 0);
            while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost &&
                   !actor_in_location(15)) step(0);
        }
        if (actor_in_location(15) && screen == SCREEN_PLAY && !won && !lost &&
            !race_t4.player[0].fault)
            controller_move_actor_to(x20, y20, 1);
        while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost) {
            step(0);
        }
    }
    checkpoint(won ? "won" : lost ? "lost" : "limit");
    fputs("\n]}\n", trace_file);
    fclose(trace_file);
    fprintf(stderr, "T4 attempt result=%s frame=%d elapsed=%u trace=%s\n",
            won ? "WON" : lost ? "LOST" : "LIMIT", frame,
            (unsigned)race_campaign.elapsed_frames, trace_path);
    return won ? 0 : 1;
}
