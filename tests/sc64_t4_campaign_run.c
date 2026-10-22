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

static int enabled_doodads(void)
{
    int count = 0;
    for (unsigned i = 0; i < sc64_t4_scenario.doodad_count; ++i)
        count += race_t4.doodad_enabled[i] != 0;
    return count;
}

static int doodad_enabled_for_building(unsigned slot)
{
    if (slot >= MAX_BUILDINGS) return -1;
    for (unsigned row = 0; row < sc64_t4_scenario.doodad_count && row < 32u; ++row)
        if (race_t4.doodad_slot[row] == slot) return race_t4.doodad_enabled[row] != 0;
    return -1;
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

static int select_human_group_except(int excluded_slot)
{
    int left = WORLD_W, top = WORLD_H, right = -1, bottom = -1;
    int drag_steps = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (i != excluded_slot &&
        u[i].alive && u[i].owner == SIDE &&
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
    /* Snap the drag endpoint once. Recomputing snap4 from the moving cursor
       can oscillate forever when a live formation edge is two pixels away. */
    int target_x = snap4(cx, right, WORLD_W);
    int target_y = snap4(cy, bottom, WORLD_H);
    while ((cx != target_x || cy != target_y) && drag_steps++ < 600 &&
           host_ticks < (u32)run_limit + 4096u && screen == SCREEN_PLAY &&
           !won && !lost) {
        u16 held = KEY_L | KEY_R;
        if (cx < target_x) held |= KEY_RIGHT;
        if (cx > target_x) held |= KEY_LEFT;
        if (cy < target_y) held |= KEY_DOWN;
        if (cy > target_y) held |= KEY_UP;
        step(held);
    }
    if (cx != target_x || cy != target_y)
        fprintf(stderr, "T4 controller box endpoint stalled at %d,%d target=%d,%d after %d steps\n",
                cx, cy, target_x, target_y, drag_steps);
    step(0);
    int count = selected_count();
    if (selected_building >= 0) {
        fprintf(stderr, "T4 controller box rejected selected building=%d count=%d\n",
                selected_building, count);
        return 0;
    }
    if (excluded_slot >= 0 && excluded_slot < MAX_UNITS &&
        u[excluded_slot].alive && u[excluded_slot].selected) {
        fprintf(stderr, "T4 controller box rejected excluded slot=%d count=%d\n",
                excluded_slot, count);
        return 0;
    }
    int verified = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
        if (u[i].owner != SIDE || !sc64_t4_unit_selectable(u[i].type) ||
            i == excluded_slot) {
            fprintf(stderr, "T4 controller box rejected slot=%d alive=%d owner=%u selectable=%d excluded=%d\n",
                    i, u[i].alive, u[i].owner, sc64_t4_unit_selectable(u[i].type),
                    i == excluded_slot);
            return 0;
        }
        ++verified;
    }
    if (verified == 0 || verified != count) {
        fprintf(stderr, "T4 controller box rejected count mismatch selected=%d verified=%d\n",
                count, verified);
        return 0;
    }
    fprintf(stderr, "T4 controller box selected %d verified live SIDE units\n", count);
    return 1;
}

static int select_human_group(void)
{
    return select_human_group_except(-1);
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

static int unit_in_location(unsigned slot, unsigned location)
{
    if (slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE) return 0;
    Sc64T4QueryRect area;
    if (!race_t4_effect_location(&race_t4, location, &area)) return 0;
    return u[slot].x * 2 >= area.left && u[slot].x * 2 < area.right &&
           u[slot].y * 2 >= area.top && u[slot].y * 2 < area.bottom;
}

static int human_men_in_location(unsigned location)
{
    Sc64T4QueryRect area, actor;
    if (!race_t4_effect_location(&race_t4, location, &area)) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE &&
            race_t4_effect_men_raw((unsigned)u[i].raw_type) &&
            race_t4_effect_unit_rect(&u[i], &actor) &&
            sc64_t4_query_rect_overlap(&area, &actor)) return 1;
    return 0;
}

static int nearest_human_bring_point(int sx, int sy, const u8 *visited,
                                     int doors_only, int *location,
                                     int *out_x, int *out_y)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS];
    static u8 locations[65];
    static u8 ordered_locations[16];
    Sc64T4QueryRect rects[65];
    int start, head = 0, tail = 0, best = -1, best_location = 0;
    unsigned ordered_count = 0;
    if (!location || !out_x || !out_y || sx < 0 || sy < 0) return 0;
    memset(locations, 0, sizeof(locations));
    for (unsigned ti = 0; ti < sc64_t4_scenario.trigger_count; ++ti) {
        if (doors_only && ordered_count >= 6) break; /* opening chain through trigger 15 */
        const Sc64T4Trigger *trigger = &sc64_t4_scenario.triggers[ti];
        int changes_door = 0, moves_units = 0;
        /* Visit only Bring regions tied to an actual door doodad transition
           or a source MoveUnit action. Ignore trap actions and unrelated
           mission-wide/ambush regions (including broad regions 9/10). */
        for (unsigned ai = 0; ai < 64; ++ai) {
            const Sc64T4Action *a = &trigger->actions[ai];
            if (!a->opcode) break;
            if (a->opcode == 42 && a->unit >= 205 && a->unit <= 208)
                changes_door = 1;
            if (a->opcode == 39) moves_units = 1;
        }
        if (!changes_door && (doors_only || !moves_units)) continue;
        for (unsigned ci = 0; ci < 16; ++ci) {
            const Sc64T4Condition *c = &trigger->conditions[ci];
            if (!c->opcode) break;
            if (c->opcode != 3 || c->player != 0 ||
                (changes_door ? c->unit != 229 : c->unit != 230) ||
                (c->flags & 2) || !c->location || c->location >= 64) continue;
            if ((!visited || !visited[c->location]) && !locations[c->location]) {
                locations[c->location] = 1;
                if (ordered_count < sizeof(ordered_locations))
                    ordered_locations[ordered_count++] = c->location;
            }
        }
    }
    for (unsigned id = 1; id < 64; ++id)
        if (locations[id] && !race_t4_effect_location(&race_t4, id, &rects[id]))
            locations[id] = 0;
    build_path_grid();
    memset(seen, 0, sizeof(seen));
    start = sx / 16 + (sy / 16) * MAP_COLS;
    if (start < 0 || start >= MAP_CELLS) return 0;
    seen[start] = 1; queue[tail++] = (u16)start;
    while (head < tail) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
        int px = col * 16 + 8, py = row * 16 + 8;
#define T4_REACHABLE_NEXT(next_cell) do { \
        int n_ = (next_cell); \
        if (!seen[n_] && SC64_GRID_GET(path_walkable,n_)) { \
            seen[n_]=1; queue[tail++]=(u16)n_; \
        } \
    } while (0)
        if (col > 0) T4_REACHABLE_NEXT(cell - 1);
        if (col + 1 < MAP_COLS) T4_REACHABLE_NEXT(cell + 1);
        if (row > 0) T4_REACHABLE_NEXT(cell - MAP_COLS);
        if (row + 1 < MAP_ROWS) T4_REACHABLE_NEXT(cell + MAP_COLS);
#undef T4_REACHABLE_NEXT
    }
    /* Respect source trigger order among door-changing Bring gates. For the
       first reachable region, choose a reachable interior point near center. */
    for (unsigned li = 0; li < ordered_count && best < 0; ++li) {
        unsigned id = ordered_locations[li];
        if (!locations[id]) continue;
        int best_center_distance = 0x7fffffff;
        for (int qi = 0; qi < tail; ++qi) {
            int cell = queue[qi], px = (cell % MAP_COLS) * 16 + 8;
            int py = (cell / MAP_COLS) * 16 + 8;
            const Sc64T4QueryRect *r = &rects[id];
            if (px * 2 < r->left || px * 2 >= r->right ||
                py * 2 < r->top || py * 2 >= r->bottom) continue;
            int center_x = (r->left + r->right) / 4;
            int center_y = (r->top + r->bottom) / 4;
            int d = dist2(px, py, center_x, center_y);
            if (d < best_center_distance) {
                best = cell;
                best_center_distance = d;
                best_location = (int)id;
            }
        }
    }
    if (best < 0 || !best_location) return 0;
    *location = best_location;
    *out_x = (best % MAP_COLS) * 16 + 8;
    *out_y = (best / MAP_COLS) * 16 + 8;
    return 1;
}

static int nearest_reachable_location_point(int sx, int sy, unsigned location,
                                            int *out_x, int *out_y)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS];
    Sc64T4QueryRect rect;
    int start, head = 0, tail = 0, best = -1;
    int best_center_distance = 0x7fffffff;
    if (!out_x || !out_y || sx < 0 || sy < 0 ||
        !race_t4_effect_location(&race_t4, location, &rect)) return 0;
    build_path_grid();
    memset(seen, 0, sizeof(seen));
    start = sx / 16 + (sy / 16) * MAP_COLS;
    if (start < 0 || start >= MAP_CELLS) return 0;
    seen[start] = 1; queue[tail++] = (u16)start;
    while (head < tail) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
        int px = col * 16 + 8, py = row * 16 + 8;
        if (px * 2 >= rect.left && px * 2 < rect.right &&
            py * 2 >= rect.top && py * 2 < rect.bottom) {
            int center_x = (rect.left + rect.right) / 4;
            int center_y = (rect.top + rect.bottom) / 4;
            int d = dist2(px, py, center_x, center_y);
            if (d < best_center_distance) {
                best = cell;
                best_center_distance = d;
            }
        }
#define T4_LOC_NEXT(next_cell) do { \
        int n_ = (next_cell); \
        if (!seen[n_] && SC64_GRID_GET(path_walkable,n_)) { \
            seen[n_]=1; queue[tail++]=(u16)n_; \
        } \
    } while (0)
        if (col > 0) T4_LOC_NEXT(cell - 1);
        if (col + 1 < MAP_COLS) T4_LOC_NEXT(cell + 1);
        if (row > 0) T4_LOC_NEXT(cell - MAP_COLS);
        if (row + 1 < MAP_ROWS) T4_LOC_NEXT(cell + MAP_COLS);
#undef T4_LOC_NEXT
    }
    if (best < 0) return 0;
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

static int choose_human_actor_in_location(int excluded_slot, unsigned location)
{
    int best = -1, best_dist = 0x7fffffff, gx, gy;
    if (!t4_location_center(location, &gx, &gy)) return -1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (i == excluded_slot || !u[i].alive || u[i].owner != SIDE ||
            !race_t4_effect_men_raw(u[i].raw_type) ||
            !sc64_t4_unit_selectable(u[i].type) || !unit_in_location((unsigned)i, location)) continue;
        int d = dist2(gx, gy, u[i].x, u[i].y);
        if (d < best_dist) { best = i; best_dist = d; }
    }
    return best;
}

static int controller_move_actor_to(int x, int y, int attack_move)
{
    if (actor_slot < 0 || actor_slot >= MAX_UNITS || !u[actor_slot].alive) return 0;
    /* A normal click can select a different actor when units overlap. Use the
       ordinary no-drag L gesture to cycle to the exact slot instead. */
    int selected = 0;
    for (int retry = 0; retry <= MAX_UNITS && screen == SCREEN_PLAY &&
         !won && !lost; ++retry) {
        if (u[actor_slot].selected && selected_count() == 1 && selected_building < 0) {
            selected = 1;
            break;
        }
        tap(KEY_L); /* no-drag L release cycles to one selectable SIDE unit */
    }
    if (!selected) {
        fprintf(stderr, "T4 selection failed slot=%d selected=%d count=%d building=%d\n",
                actor_slot, u[actor_slot].selected, selected_count(), selected_building);
        return 0;
    }
    tap(KEY_B);
    if (attack_move) tap(KEY_DOWN);
    tap(KEY_A); /* Ordinary unit command: MOVE or ATTACK MOVE. */
    move_cursor(x, y);
    tap(KEY_A);
    return 1;
}

static int controller_move_unit_to(int slot, int x, int y, int attack_move)
{
    int saved = actor_slot;
    actor_slot = slot;
    int issued = controller_move_actor_to(x, y, attack_move);
    actor_slot = saved;
    return issued;
}

static int controller_move_group_to(int x, int y, int excluded_slot)
{
    if (!select_human_group_except(excluded_slot)) return 0;
    tap(KEY_B);
    tap(KEY_A);
    move_cursor(x, y);
    tap(KEY_A);
    fprintf(stderr, "T4 controller group move target=%d,%d count=%d\n",
            x, y, selected_count());
    return 1;
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
        "actor=%d@%d,%d sides=%d/%d doors=%d faults=%u outcomes=%u\n",
        kind, frame, (unsigned)race_campaign.elapsed_frames, screen,
        won ? "WON" : lost ? "LOST" : "RUNNING",
        !!(race_t4.switches.bits[0] & 0x10),
        !!(race_t4.switches.bits[0] & 0x80), actor_slot,
        actor_slot >= 0 ? u[actor_slot].x : -1,
        actor_slot >= 0 ? u[actor_slot].y : -1,
        owner_units(SIDE), owner_units(ENEMY), enabled_doodads(), faults, outcomes);
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
    fprintf(trace_file, "{\"schema\":1,\"mission\":7,\"policy\":\"armed-raw20-attack-move-source-gates\",\"events\":[\n");
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
        for (int gate = 0; gate < 6 && frame < run_limit && screen == SCREEN_PLAY &&
             !won && !lost && !race_t4.player[0].fault; ++gate) {
            if (actor_slot < 0 || actor_slot >= MAX_UNITS || !u[actor_slot].alive)
                actor_slot = choose_human_actor();
            if (actor_slot < 0) break;
            int bring_location = 0;
            if (!nearest_human_bring_point(u[actor_slot].x, u[actor_slot].y,
                                           visited_bring, 1, &bring_location,
                                           &route_x, &route_y)) break;
            Sc64T4QueryRect gate_rect;
            race_t4_effect_location(&race_t4, (unsigned)bring_location, &gate_rect);
            fprintf(stderr, "T4 controller route uses source Bring location %d at %d,%d rect=%d,%d..%d,%d actor=%d,%d\n",
                    bring_location, route_x, route_y, gate_rect.left/2, gate_rect.top/2,
                    gate_rect.right/2, gate_rect.bottom/2, u[actor_slot].x, u[actor_slot].y);
            if (!controller_move_group_to(route_x, route_y, -1)) {
                fprintf(stderr, "T4 controller aborted: first-gate group selection did not match policy\n");
                break;
            }
            int deadline = frame + 4000;
            while (frame < deadline && frame < run_limit && screen == SCREEN_PLAY &&
                   !won && !lost && !human_men_in_location((unsigned)bring_location))
                step(0);
            if (human_men_in_location((unsigned)bring_location)) {
                visited_bring[bring_location] = 1;
                int settle = frame + 90;
                while (frame < settle && frame < run_limit && screen == SCREEN_PLAY &&
                       !won && !lost) step(0);
                fprintf(stderr, "T4 entered source Bring %d; enabled doodads=%d\n",
                        bring_location, enabled_doodads());
            }
        }
        if (screen == SCREEN_PLAY && !won && !lost && actor_slot >= 0 &&
            actor_slot < MAX_UNITS && u[actor_slot].alive &&
            nearest_reachable_location_point(u[actor_slot].x, u[actor_slot].y,
                                             14, &route_x, &route_y)) {
            int raynor = choose_human_actor();
            if (raynor >= 0 && raynor < MAX_UNITS && u[raynor].alive &&
                u[raynor].raw_type == 20) actor_slot = raynor;
            else raynor = actor_slot;
            fprintf(stderr, "T4 route to source location 14 interior %d,%d (center %d,%d)\n",
                    route_x, route_y, x14, y14);
            /* The source teleport is narrow and dangerous. Keep the group
               back and send the armed Raynor alone with ATTACK MOVE. */
            int raynor_route = raynor >= 0 && raynor < MAX_UNITS && u[raynor].alive &&
                               nearest_reachable_location_point(u[raynor].x, u[raynor].y,
                                                               14, &route_x, &route_y);
            fprintf(stderr, "T4 armed lead=%d raw=%d hp=%d route14=%d target=%d,%d\n",
                    raynor, raynor >= 0 && raynor < MAX_UNITS ? u[raynor].raw_type : -1,
                    raynor >= 0 && raynor < MAX_UNITS ? u[raynor].hp : -1,
                    raynor_route, route_x, route_y);
            if (raynor_route && controller_move_unit_to(raynor, route_x, route_y, 1)) {
                int deadline = frame + 8000;
                while (frame < deadline && frame < run_limit && screen == SCREEN_PLAY &&
                       !won && !lost && u[raynor].alive &&
                       !unit_in_location((unsigned)raynor, 15)) {
                    step(0);
                    if (host_ticks % T4_CHECKPOINT_PERIOD == 0) {
                        Unit *q = &u[raynor];
                        fprintf(stderr, "T4 armed lead route14 slot=%d pos=%d,%d hp=%d order=%d path=%d,%d retry=%d sides=%d\n",
                                raynor, q->x, q->y, q->hp, q->order, q->path_x,
                                q->path_y, q->path_retry, owner_units(SIDE));
                    }
                }
                if (unit_in_location((unsigned)raynor, 15))
                    fprintf(stderr, "T4 armed lead entered source location15 slot=%d hp=%d\n",
                            raynor, u[raynor].hp);
                else
                    fprintf(stderr, "T4 armed lead did not reach location15 alive=%d\n",
                            u[raynor].alive);
            }
            while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost &&
                   !human_men_in_location(15)) {
                if (host_ticks % T4_CHECKPOINT_PERIOD == 0) {
                    if (actor_slot < 0 || actor_slot >= MAX_UNITS || !u[actor_slot].alive) {
                        step(0);
                        continue;
                    }
                    Unit *q = &u[actor_slot];
                    int cell = q->x / 16 + (q->y / 16) * MAP_COLS;
                    fprintf(stderr, "T4 route14 lead=%d raw=%d pos=%d,%d hp=%d target=%d,%d path=%d,%d timer=%d retry=%d walk=%d sides=%d doors=%d\n",
                            actor_slot, q->raw_type, q->x, q->y, q->hp,
                            q->tx, q->ty, q->path_x, q->path_y, q->path_timer,
                            q->path_retry, cell >= 0 && cell < MAP_CELLS &&
                            SC64_GRID_GET(path_walkable, cell), owner_units(SIDE),
                            enabled_doodads());
                }
                step(0);
            }
        }
        fprintf(stderr, "T4 post-route actor=%d alive=%d loc14/15/20=%d/%d/%d center20=%d,%d\n",
                actor_slot, actor_slot >= 0 && actor_slot < MAX_UNITS && u[actor_slot].alive,
                actor_in_location(14), actor_in_location(15), actor_in_location(20), x20, y20);
        if (human_men_in_location(15) && screen == SCREEN_PLAY && !won && !lost &&
            !race_t4.player[0].fault) {
            /* The first door chain reaches the teleport, but the current
               walk grid may still put the objective behind later source
               doors. Continue through their authored Bring gates, then
               recompute a reachable point in location 20. */
            for (int gate = 0; gate < 6 && frame < run_limit &&
                 screen == SCREEN_PLAY && !won && !lost; ++gate) {
                static int courier = -1;
                if (courier < 0 || courier >= MAX_UNITS || !u[courier].alive)
                    courier = choose_human_actor_in_location(actor_slot, 15);
                if (courier < 0 && actor_slot >= 0 && actor_slot < MAX_UNITS &&
                    u[actor_slot].alive && unit_in_location((unsigned)actor_slot, 15))
                    courier = actor_slot;
                int group_lead = courier;
                if (group_lead < 0) break;
                int bring_location = 0;
                if (!nearest_human_bring_point(u[group_lead].x, u[group_lead].y,
                                               visited_bring, 0, &bring_location,
                                               &route_x, &route_y)) break;
                fprintf(stderr, "T4 post-teleport source Bring %d at %d,%d from actor %d@%d,%d\n",
                        bring_location, route_x, route_y, group_lead,
                        u[group_lead].x, u[group_lead].y);
                if (!controller_move_unit_to(group_lead, route_x, route_y,
                                             u[group_lead].raw_type == 20)) break;
                int deadline = frame + 2500;
                while (frame < deadline && frame < run_limit && screen == SCREEN_PLAY &&
                       !won && !lost && u[group_lead].alive &&
                       !unit_in_location((unsigned)group_lead, (unsigned)bring_location)) {
                    step(0);
                }
                if (!unit_in_location((unsigned)group_lead, (unsigned)bring_location)) break;
                courier = group_lead;
                visited_bring[bring_location] = 1;
                int settle_frames = bring_location == 26 ? 1 : 90;
                for (int settle = 0; settle < settle_frames && frame < run_limit &&
                     screen == SCREEN_PLAY && !won && !lost; ++settle) step(0);
                fprintf(stderr, "T4 post-teleport entered source Bring %d; enabled doodads=%d\n",
                        bring_location, enabled_doodads());
                group_lead = courier >= 0 && courier < MAX_UNITS && u[courier].alive ?
                             courier : choose_human_actor_in_location(actor_slot, 15);
                if (group_lead < 0 || !nearest_reachable_location_point(
                        u[group_lead].x, u[group_lead].y, 20, &route_x, &route_y)) continue;
                fprintf(stderr, "T4 post-gate route20 interior %d,%d\n", route_x, route_y);
                if (!controller_move_unit_to(group_lead, route_x, route_y, 1)) break;
                int last_goal_report = frame - 500;
                while (frame < run_limit && screen == SCREEN_PLAY && !won && !lost &&
                       u[group_lead].alive &&
                       !unit_in_location((unsigned)group_lead, 20)) {
                    step(0);
                    if (frame - last_goal_report >= 500) {
                        int nearest_enemy = -1, enemy_dist = 0x7fffffff;
                        int nearest_trap = -1, trap_dist = 0x7fffffff;
                        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
                            sc64_t4_owners_hostile(SIDE, u[i].owner)) {
                            int d = dist2(u[group_lead].x, u[group_lead].y, u[i].x, u[i].y);
                            if (d < enemy_dist) { enemy_dist = d; nearest_enemy = i; }
                        }
                        for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
                            (b[i].raw_type == 203 || b[i].raw_type == 209 ||
                             b[i].raw_type == 211 || b[i].raw_type == 212)) {
                            int d = dist2(u[group_lead].x, u[group_lead].y, b[i].x, b[i].y);
                            if (d < trap_dist) { trap_dist = d; nearest_trap = i; }
                        }
                        fprintf(stderr, "T4 goal approach frame=%d courier=%d@%d,%d hp=%d order=%d path=%d,%d retry=%d enemy=%d owner=%d raw=%d d2=%d trap=%d raw=%d hp=%d enabled=%d d2=%d\n",
                                frame, group_lead, u[group_lead].x, u[group_lead].y,
                                u[group_lead].hp, u[group_lead].order, u[group_lead].path_x,
                                u[group_lead].path_y, u[group_lead].path_retry, nearest_enemy,
                                nearest_enemy >= 0 ? u[nearest_enemy].owner : -1,
                                nearest_enemy >= 0 ? u[nearest_enemy].raw_type : -1,
                                enemy_dist, nearest_trap,
                                nearest_trap >= 0 ? b[nearest_trap].raw_type : -1,
                                nearest_trap >= 0 ? b[nearest_trap].hp : -1,
                                nearest_trap >= 0 ? doodad_enabled_for_building((unsigned)nearest_trap) : -1,
                                trap_dist);
                        last_goal_report = frame;
                    }
                }
                break;
            }
        }
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
