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
static int controller_wraith_queue_count(void);
static int controller_wraith_count(void);
static int controller_isolate_one_wraith(void);
static int controller_select_visible_type_group(int type, int after_slot,
                                                int *out_count, int *out_seed);
static int controller_marine_count(void);
static int controller_wait_building(int slot, int ticks);
static int controller_defend_command_center(void);
static int controller_assign_idle_scvs_to_gas(int maximum);
static int controller_assign_idle_scvs_to_minerals(int maximum);
static void tap(u16 key);
static int controller_normalize_field(void);
static void move_cursor(int x, int y);
static void wait_frames(int amount);
static int controller_order_actor(int slot, int x, int y, int attack_move);
static int dump_final;
static int trace_ship_slot = -1;
static int trace_isolated_wraith_slot = -1;
static int trace_starport_slot = -1;
static int trace_tower_slot = -1;
static int trace_landing_x = -1;
static int trace_landing_y = -1;
static int trace_landed_count = -1;
static int trace_ship_hp = -1;
static int trace_ship_x = -1;
static int trace_ship_y = -1;
static int trace_landed_manifest_count;
static int trace_landed_slots[SC64_TRANSPORT_DROPSHIP_SPACE];
static int trace_landed_raws[SC64_TRANSPORT_DROPSHIP_SPACE];
static int trace_passenger_count;
static int trace_passenger_slots[SC64_TRANSPORT_DROPSHIP_SPACE];
static int trace_passenger_raws[SC64_TRANSPORT_DROPSHIP_SPACE];
static int trace_passenger_snapshot_valid;
static u32 trace_passenger_snapshot_tick;
static int trace_passenger_alive[SC64_TRANSPORT_DROPSHIP_SPACE];
static int trace_passenger_loaded[SC64_TRANSPORT_DROPSHIP_SPACE];
static int trace_passenger_x[SC64_TRANSPORT_DROPSHIP_SPACE];
static int trace_passenger_y[SC64_TRANSPORT_DROPSHIP_SPACE];
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
        if (u[i].alive && u[i].owner == SIDE && sc64_game_unit_selectable(u[i].type))
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

static int nearest_enemy_approach_from_mode(int sx, int sy, int *out_x, int *out_y,
                                            int buildings_only)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS], distance[MAP_CELLS];
    int start = sx / 16 + (sy / 16) * MAP_COLS, head = 0, tail = 0;
    if (!out_x || !out_y || start < 0 || start >= MAP_CELLS) return -1;
    build_path_grid(); memset(seen, 0, sizeof(seen));
    memset(distance, 0, sizeof(distance)); seen[start] = 1; queue[tail++] = (u16)start;
    while (head < tail) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
        int px = col * 16 + 8, py = row * 16 + 8;
        for (int bi = 0; bi < MAX_BUILDINGS; ++bi) if (b[bi].alive && b[bi].owner == ENEMY) {
            int d = dist2(px, py, b[bi].x, b[bi].y);
            if (d >= 18 * 18 && d <= 40 * 40) {
                *out_x = px; *out_y = py; return distance[cell];
            }
        }
        for (int ui = 0; !buildings_only && ui < MAX_UNITS; ++ui)
            if (u[ui].alive && u[ui].owner == ENEMY) {
            int d = dist2(px, py, u[ui].x, u[ui].y);
            if (d >= 12 * 12 && d <= 40 * 40) {
                *out_x = px; *out_y = py; return distance[cell];
            }
        }
#define T5_FROM_NEXT(n_) do { int z_=(n_); if (!seen[z_] && SC64_GRID_GET(path_walkable,z_)) { seen[z_]=1; distance[z_]=(u16)(distance[cell]+1); queue[tail++]=(u16)z_; } } while (0)
        if (col > 0) T5_FROM_NEXT(cell - 1);
        if (col + 1 < MAP_COLS) T5_FROM_NEXT(cell + 1);
        if (row > 0) T5_FROM_NEXT(cell - MAP_COLS);
        if (row + 1 < MAP_ROWS) T5_FROM_NEXT(cell + MAP_COLS);
#undef T5_FROM_NEXT
    }
    return -1;
}

static int nearest_enemy_approach_from(int sx, int sy, int *out_x, int *out_y)
{
    return nearest_enemy_approach_from_mode(sx, sy, out_x, out_y, 0);
}

static int kerrigan_alive(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_KERRIGAN) return 1;
    return 0;
}

static int raynor_alive(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == 19) return 1;
    return 0;
}

static int controller_order_nonhero_force(int x, int y, int attack_move)
{
    int issued = 0;
    for (int i = 0; i < MAX_UNITS && host_ticks < (u32)run_limit &&
         screen == SCREEN_PLAY && !won && !lost; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || SC64_TRANSPORT_LOADED(&u[i]) ||
            u[i].raw_type == 19 || u[i].raw_type == 20 || u[i].type == U_KERRIGAN ||
            (u[i].type != U_MARINE && u[i].type != U_FIREBAT && u[i].type != U_MEDIC &&
             u[i].type != U_VULTURE && u[i].type != U_WRAITH)) continue;
        if (controller_order_actor(i, x, y, attack_move)) ++issued;
    }
    return issued;
}

static void controller_queue_paid_units(void)
{
    for (int i = 0; i < MAX_BUILDINGS && host_ticks < (u32)run_limit; ++i) {
        int type = -1;
        if (!b[i].alive || b[i].owner != SIDE || b[i].under_construction ||
            b[i].queue_count >= 5) continue;
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
    if (host_ticks >= (u32)run_limit || screen != SCREEN_PLAY || won || lost) return;
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

static int controller_normalize_field(void)
{
    if (screen != SCREEN_PLAY || won || lost) return 0;
    for (int i = 0; i < 2 && (menu_open || mode != MODE_NORMAL); ++i) {
        tap(KEY_B);
        if (host_ticks >= (u32)run_limit) return 0;
    }
    if (menu_open || mode != MODE_NORMAL) {
        fprintf(stderr, "T5 field normalize failed mode=%d menu=%d tick=%u\n",
                mode, menu_open, (unsigned)host_ticks);
        return 0;
    }
    return 1;
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
        sc64_game_unit_selectable(u[i].type)) {
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
        if (u[i].owner != SIDE || !sc64_game_unit_selectable(u[i].type)) return 0;
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

static int controller_order_actor(int slot, int x, int y, int attack_move)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE)
        return 0;
    if (!controller_normalize_field()) return 0;
    /* L cycles units, but on a selected Barracks it cancels the training
       queue instead of clearing the producer. A normal drag selection clears
       that sticky building selection; a click then selects just this actor. */
    if (selected_building >= 0) {
        if (!select_human_group()) return 0;
        move_cursor(u[slot].x, u[slot].y);
        tap(KEY_A);
        return u[slot].alive && u[slot].selected && selected_count() == 1 &&
               selected_building < 0;
    }
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
    tap(KEY_B);
    if (attack_move) tap(KEY_DOWN);
    tap(KEY_A);
    move_cursor(x, y);
    tap(KEY_A);
    return 1;
}

static int command_actor_to(int slot, int x, int y)
{ return controller_order_actor(slot, x, y, 0); }

static int command_actor_attack_to(int slot, int x, int y)
{ return controller_order_actor(slot, x, y, 1); }

static int controller_select_unit(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE)
        return 0;
    if (!controller_normalize_field()) return 0;
    if (selected_building >= 0) tap(KEY_L);
    if (selected_building >= 0) return 0;
    for (int retry = 0; retry <= MAX_UNITS && screen == SCREEN_PLAY &&
         !won && !lost && host_ticks < (u32)run_limit; ++retry) {
        if (u[slot].selected && selected_count() == 1 && selected_building < 0)
            return 1;
        tap(KEY_L);
    }
    return 0;
}

static int controller_select_building(int slot)
{
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive || b[slot].owner != SIDE)
        return 0;
    if (!controller_normalize_field()) return 0;
    /* Field interaction treats A as producer-queue input while a building
       remains selected, so clear that selection with the ordinary L cycle
       before clicking the requested building. */
    if (selected_building >= 0) tap(KEY_L);
    move_cursor(b[slot].x, b[slot].y);
    tap(KEY_A);
    return selected_building == slot && selected_count() == 0;
}

static int controller_choose_menu_row(int row)
{
    if (row < 0 || row >= 14 || host_ticks >= (u32)run_limit) return 0;
    tap(KEY_B);
    for (int i = 0; i < row; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    return screen == SCREEN_PLAY && !won && !lost;
}

static int find_side_building(int type, int completed_only)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == type &&
            (!completed_only || building_complete(i))) return i;
    return -1;
}

static int find_side_worker(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV &&
            !SC64_TRANSPORT_LOADED(&u[i])) return i;
    return -1;
}

static int controller_find_starport_site(int factory, int *out_x, int *out_y)
{
    int worker = find_side_worker(), best_distance = 0x7fffffff;
    if (factory < 0 || worker < 0 || !out_x || !out_y) return 0;
    for (int y = 16; y < WORLD_H - 48; y += 16)
        for (int x = 32; x < WORLD_W - 64; x += 16) {
            int tx = x + SC64_T5_ADDON_X_OFFSET;
            int ty = y + SC64_T5_ADDON_Y_OFFSET;
            int d = dist2(x, y, b[factory].x, b[factory].y) +
                    dist2(x, y, u[worker].x, u[worker].y);
            if (d > 2 * 288 * 288 || d >= best_distance ||
                !race_can_site(SIDE, B_STARPORT, x, y) || !can_place(B_STARPORT, x, y) ||
                !race_can_site(SIDE, B_CONTROL_TOWER, tx, ty) ||
                !can_place(B_CONTROL_TOWER, tx, ty)) continue;
            int clear = 1;
            for (int bi = 0; bi < MAX_BUILDINGS; ++bi)
                if (b[bi].alive && dist2(tx, ty, b[bi].x, b[bi].y) < 24 * 24) {
                    clear = 0; break;
                }
            if (clear) { best_distance = d; *out_x = x; *out_y = y; }
        }
    return best_distance != 0x7fffffff;
}

static int controller_build_supply_depot(int *out_slot)
{
    int worker = find_side_worker(), command = -1, x = -1, y = -1;
    if (out_slot) *out_slot = -1;
    if (worker < 0 || minerals < building_cost(B_DEPOT)) {
        fprintf(stderr, "T5 Supply Depot unavailable worker=%d minerals=%d cost=%d\n",
                worker, minerals, building_cost(B_DEPOT));
        return 0;
    }
    int best_score = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_COMMAND) {
            command = i; break;
        }
    if (command < 0) {
        fprintf(stderr, "T5 Supply Depot has no surviving owned Command Center\n");
        return 0;
    }
    if (!controller_select_unit(worker) || !controller_choose_menu_row(3)) {
        fprintf(stderr, "T5 Supply Depot could not select worker=%d or open depot command\n", worker);
        return 0;
    }
    /* Build placement uses the cursor's current four-pixel lattice. A planned
       site based on source coordinates can round differently after selecting
       the SCV, so evaluate candidates on the actual placement lattice. */
    for (int candidate_y = b[command].y - 192;
         candidate_y <= b[command].y + 192; candidate_y += 16)
        for (int cx0 = b[command].x - 192; cx0 <= b[command].x + 192; cx0 += 16) {
            int px = snap4_local(cx, cx0, WORLD_W);
            int py = snap4_local(cy, candidate_y, WORLD_H);
            if (dist2(px, py, b[command].x, b[command].y) > 192 * 192 ||
                !race_can_site(SIDE, B_DEPOT, px, py) || !can_place(B_DEPOT, px, py)) continue;
            int hostile = 0x7fffffff;
            for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == ENEMY) {
                int d = dist2(px, py, u[j].x, u[j].y);
                if (d < hostile) hostile = d;
            }
            for (int j = 0; j < MAX_BUILDINGS; ++j) if (b[j].alive && b[j].owner == ENEMY) {
                int d = dist2(px, py, b[j].x, b[j].y);
                if (d < hostile) hostile = d;
            }
            int score = hostile > 0x3fffffff ? 0x3fffffff : hostile;
            score -= dist2(px, py, u[worker].x, u[worker].y) / 8;
            if (score > best_score) { best_score = score; x = px; y = py; }
        }
    if (x < 0 || y < 0) {
        fprintf(stderr, "T5 Supply Depot found no legal site near Command Center=%d,%d\n",
                b[command].x, b[command].y);
        (void)controller_normalize_field();
        return 0;
    }
    int planned_x = x, planned_y = y;
    move_cursor(x, y);
    x = cx; y = cy;
    if (!race_can_site(SIDE, B_DEPOT, x, y) || !can_place(B_DEPOT, x, y)) {
        fprintf(stderr, "T5 Supply Depot rounded site invalid planned=%d,%d actual=%d,%d\n",
                planned_x, planned_y, cx, cy);
        (void)controller_normalize_field();
        return 0;
    }
    int before = owner_buildings(SIDE);
    tap(KEY_A);
    int slot = -1;
    if (owner_buildings(SIDE) > before)
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == SIDE && b[i].type == B_DEPOT &&
                b[i].under_construction && dist2(x, y, b[i].x, b[i].y) < 64 * 64) {
                slot = i; break;
            }
    if (slot < 0) {
        fprintf(stderr, "T5 Supply Depot command did not create building planned=%d,%d actual=%d,%d mode=%d menu=%d wallet=%d\n",
                x, y, cx, cy, mode, menu_open, minerals);
        (void)controller_normalize_field();
        return 0;
    }
    fprintf(stderr, "T5 controller started paid Supply Depot slot=%d at %d,%d worker=%d supply=%d/%d\n",
            slot, x, y, worker, reserved_supply(), supply_cap);
    if (!controller_wait_building(slot, 5000)) return 0;
    fprintf(stderr, "T5 controller completed Supply Depot slot=%d supply=%d/%d\n",
            slot, reserved_supply(), supply_cap);
    if (out_slot) *out_slot = slot;
    return 1;
}

static int controller_build_starport(int *out_slot)
{
    int factory = find_side_building(B_FACTORY, 1), worker = find_side_worker();
    int x, y, before = owner_buildings(SIDE);
    int existing = find_side_building(B_STARPORT, 0);
    if (out_slot) *out_slot = existing;
    if (existing >= 0) return 1;
    if (factory < 0 || worker < 0 || minerals < 100 || gas < 100 ||
        !controller_find_starport_site(factory, &x, &y)) return 0;
    if (!controller_select_unit(worker) || !controller_choose_menu_row(11)) return 0;
    move_cursor(x, y);
    tap(KEY_A);
    int slot = find_side_building(B_STARPORT, 0);
    if (owner_buildings(SIDE) <= before || slot < 0 || b[slot].x != x || b[slot].y != y)
        return 0;
    fprintf(stderr, "T5 controller started paid Starport slot=%d site=%d,%d worker=%d wallet=%d/%d\n",
            slot, x, y, worker, minerals, gas);
    if (out_slot) *out_slot = slot;
    return 1;
}

static int controller_wait_building(int slot, int ticks)
{
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive) return 0;
    for (int i = 0; i < ticks && screen == SCREEN_PLAY && !won && !lost &&
         host_ticks < (u32)run_limit && !building_complete(slot); ++i) step(0);
    return b[slot].alive && building_complete(slot);
}

static int controller_build_control_tower(int starport)
{
    if (starport < 0 || starport >= MAX_BUILDINGS || !b[starport].alive ||
        b[starport].owner != SIDE || b[starport].type != B_STARPORT) {
        fprintf(stderr, "T5 tower route invalid Starport slot=%d\n", starport);
        return 0;
    }
    if (race_t5_starport_has_control_tower(starport)) return 1;
    if (!building_complete(starport) || minerals < 50 || gas < 50) {
        fprintf(stderr, "T5 tower route prerequisites incomplete=%d wallet=%d/%d\n",
                building_complete(starport), minerals, gas);
        return 0;
    }
    if (!controller_select_building(starport)) {
        fprintf(stderr, "T5 could not select Starport=%d selected_building=%d selected_units=%d cursor=%d,%d\n",
                starport, selected_building, selected_count(), cx, cy);
        return 0;
    }
    if (!controller_choose_menu_row(12)) {
        fprintf(stderr, "T5 could not issue Control Tower menu command\n");
        return 0;
    }
    fprintf(stderr, "T5 controller started paid attached Control Tower for Starport=%d wallet=%d/%d\n",
            starport, minerals, gas);
    for (int i = 0; i < 900 && screen == SCREEN_PLAY && !won && !lost &&
         host_ticks < (u32)run_limit; ++i) {
        step(0);
        if (race_t5_starport_has_control_tower(starport)) return 1;
    }
    return race_t5_starport_has_control_tower(starport);
}

static int controller_build_dropship(int starport, int *out_slot)
{
    int ship = -1;
    if (out_slot) *out_slot = -1;
    if (starport < 0 || !race_t5_starport_has_control_tower(starport) ||
        !controller_select_building(starport)) return 0;
    for (int i = 0; i < 5 && train_type != U_DROPSHIP; ++i) tap(KEY_R);
    if (train_type != U_DROPSHIP || minerals < 100 || gas < 100) return 0;
    tap(KEY_A);
    if (!b[starport].queue_count || b[starport].queue[b[starport].queue_head] != U_DROPSHIP)
        return 0;
    fprintf(stderr, "T5 controller queued paid Dropship at Starport=%d wallet=%d/%d\n",
            starport, minerals, gas);
    for (int tick = 0; tick < 5000 && screen == SCREEN_PLAY && !won && !lost &&
         host_ticks < (u32)run_limit; ++tick) {
        step(0);
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_DROPSHIP &&
                !SC64_TRANSPORT_LOADED(&u[i])) { ship = i; break; }
        if (ship >= 0) break;
    }
    if (out_slot) *out_slot = ship;
    return ship >= 0;
}

static int controller_air_threat_radius(int attacker)
{
    int range = sc64_race_unit_range_vs(attacker, 1);
    if (range <= 0 || sc64_race_unit_damage_vs(attacker, 1) <= 0) return 0;
    return (range + 1) / 2 + 24; /* engine half-scale plus a conservative margin */
}

static int controller_air_cell_safe(int x, int y)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) &&
        owners_hostile(SIDE, u[i].owner)) {
        int radius = controller_air_threat_radius(u[i].type);
        if (radius > 0 && dist2(x, y, u[i].x, u[i].y) < radius * radius) return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) &&
        owners_hostile(SIDE, b[i].owner)) {
        /* These are the complete set of supported static air defenses in this
           runtime. Their firing radii and damage are defined in race_combat.h. */
        if (b[i].type == B_MISSILE_TURRET || b[i].type == B_SPORE_COLONY ||
            b[i].type == B_PHOTON_CANNON) {
            int radius = 112 + 24;
            if (dist2(x, y, b[i].x, b[i].y) < radius * radius) return 0;
        }
    }
    return 1;
}

typedef struct { u16 cell; u16 density; u32 enemy_distance; } T5LandingCandidate;
enum { T5_LANDING_CANDIDATES = 24, T5_AIR_ROUTE_MAX_LEGS = 16 };

static int controller_landing_point(int *out_x, int *out_y)
{
    T5LandingCandidate best[T5_LANDING_CANDIDATES];
    int count = 0, terrain_candidates = 0, threat_safe = 0, ground_reachable = 0;
    if (!out_x || !out_y) return 0;
    build_path_grid();
    for (int cell = 0; cell < MAP_CELLS; ++cell) {
        if (!SC64_GRID_GET(path_walkable, cell)) continue;
        ++terrain_candidates;
        int x = (cell % MAP_COLS) * 16 + 8, y = (cell / MAP_COLS) * 16 + 8;
        if (!controller_air_cell_safe(x, y)) continue;
        ++threat_safe;
        int enemy_d = 0x7fffffff, cluster = 0;
        for (int bi = 0; bi < MAX_BUILDINGS; ++bi) if (b[bi].alive && b[bi].owner == ENEMY) {
            int d = dist2(x, y, b[bi].x, b[bi].y);
            if (d < enemy_d) enemy_d = d;
            if (d <= 256 * 256) ++cluster;
        }
        if (enemy_d < 64 * 64 || enemy_d > 320 * 320) continue;
        T5LandingCandidate item = {(u16)cell, (u16)cluster, (u32)enemy_d};
        int at = 0;
        while (at < count && (best[at].density > item.density ||
               (best[at].density == item.density && best[at].enemy_distance <= item.enemy_distance))) ++at;
        if (at >= T5_LANDING_CANDIDATES) continue;
        if (count < T5_LANDING_CANDIDATES) ++count;
        for (int j = count - 1; j > at; --j) best[j] = best[j - 1];
        best[at] = item;
    }
    /* An airborne-safe cell is useful only if the existing ground path grid
       can reach a real enemy approach cell from it. */
    for (int i = 0; i < count; ++i) {
        int x = (best[i].cell % MAP_COLS) * 16 + 8;
        int y = (best[i].cell / MAP_COLS) * 16 + 8, ax, ay;
        if (nearest_enemy_approach_from_mode(x, y, &ax, &ay, 1) < 0) continue;
        ++ground_reachable;
        *out_x = x; *out_y = y;
        fprintf(stderr, "T5 safe landing candidate=%d,%d cluster=%u enemy-distance=%u ground-approach=%d,%d\n",
                x, y, best[i].density, best[i].enemy_distance, ax, ay);
        return 1;
    }
    fprintf(stderr, "T5 no connected AA-safe landing terrain=%d threat-safe=%d candidates=%d ground-reachable=%d\n",
            terrain_candidates, threat_safe, count, ground_reachable);
    return 0;
}

/* Air movement ignores terrain in move_toward, but still follows a straight
   line between orders. This bounded BFS finds a safe next turning point on a
   16-pixel lattice using the live hostile air-weapon envelopes. */
static int controller_air_next_waypoint(int sx, int sy, int gx, int gy, int *wx, int *wy)
{
    static u8 seen[MAP_CELLS];
    static u16 previous[MAP_CELLS], queue[MAP_CELLS], reverse[MAP_CELLS];
    int start = sx / 16 + (sy / 16) * MAP_COLS;
    int goal = gx / 16 + (gy / 16) * MAP_COLS;
    int head = 0, tail = 0;
    if (!wx || !wy || start < 0 || start >= MAP_CELLS || goal < 0 || goal >= MAP_CELLS)
        return 0;
    memset(seen, 0, sizeof(seen));
    for (int i = 0; i < MAP_CELLS; ++i) previous[i] = 0xffffu;
    seen[start] = 1; queue[tail++] = (u16)start;
    while (head < tail && !seen[goal]) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
#define T5_AIR_NEXT(n_) do { int z_=(n_); if (!seen[z_]) { \
    int zx_=(z_%MAP_COLS)*16+8, zy_=(z_/MAP_COLS)*16+8; \
    if (z_ == start || controller_air_cell_safe(zx_,zy_)) { \
        seen[z_]=1; previous[z_]=(u16)cell; queue[tail++]=(u16)z_; } } } while (0)
        if (col > 0) T5_AIR_NEXT(cell - 1);
        if (col + 1 < MAP_COLS) T5_AIR_NEXT(cell + 1);
        if (row > 0) T5_AIR_NEXT(cell - MAP_COLS);
        if (row + 1 < MAP_ROWS) T5_AIR_NEXT(cell + MAP_COLS);
        if (col > 0 && row > 0) T5_AIR_NEXT(cell - MAP_COLS - 1);
        if (col + 1 < MAP_COLS && row > 0) T5_AIR_NEXT(cell - MAP_COLS + 1);
        if (col > 0 && row + 1 < MAP_ROWS) T5_AIR_NEXT(cell + MAP_COLS - 1);
        if (col + 1 < MAP_COLS && row + 1 < MAP_ROWS) T5_AIR_NEXT(cell + MAP_COLS + 1);
#undef T5_AIR_NEXT
    }
    if (!seen[goal]) return 0;
    int len = 0, at = goal;
    while (at != start && len < MAP_CELLS) {
        reverse[len++] = (u16)at;
        at = previous[at];
        if (at >= MAP_CELLS) return 0;
    }
    if (len <= 0) { *wx = gx; *wy = gy; return 1; }
    int prior = start, last_dx = 0, last_dy = 0;
    for (int k = len - 1; k >= 0; --k) {
        int next = reverse[k];
        int dx = (next % MAP_COLS) - (prior % MAP_COLS);
        int dy = (next / MAP_COLS) - (prior / MAP_COLS);
        if (k != len - 1 && (dx != last_dx || dy != last_dy)) {
            *wx = (prior % MAP_COLS) * 16 + 8;
            *wy = (prior / MAP_COLS) * 16 + 8;
            return 1;
        }
        last_dx = dx; last_dy = dy; prior = next;
    }
    *wx = gx; *wy = gy;
    return 1;
}

/* Keep the shuttle off the Starport footprint while units board. Returning to
   the building center leaves passengers crowded against the structure. */
static int controller_home_staging_point(int ship, int *out_x, int *out_y)
{
    if (ship < 0 || ship >= MAX_UNITS || !u[ship].alive ||
        trace_starport_slot < 0 || trace_starport_slot >= MAX_BUILDINGS ||
        !b[trace_starport_slot].alive || !out_x || !out_y) return 0;
    const int sx = b[trace_starport_slot].x, sy = b[trace_starport_slot].y;
    int best = 0x7fffffff, found = 0;
    build_path_grid();
    for (int cell = 0; cell < MAP_CELLS; ++cell) {
        if (!SC64_GRID_GET(path_walkable, cell)) continue;
        int x = (cell % MAP_COLS) * 16 + 8, y = (cell / MAP_COLS) * 16 + 8;
        int d = dist2(x, y, sx, sy);
        if (d < 40 * 40 || d > 144 * 144 || d >= best ||
            !controller_air_cell_safe(x, y)) continue;
        int clear = 1;
        for (int bi = 0; bi < MAX_BUILDINGS; ++bi)
            if (b[bi].alive && dist2(x, y, b[bi].x, b[bi].y) < 40 * 40) {
                clear = 0; break;
            }
        if (!clear) continue;
        for (int ui = 0; ui < MAX_UNITS; ++ui)
            if (ui != ship && u[ui].alive && !SC64_TRANSPORT_LOADED(&u[ui]) &&
                dist2(x, y, u[ui].x, u[ui].y) < 24 * 24) {
                clear = 0; break;
            }
        if (clear) { best = d; *out_x = x; *out_y = y; found = 1; }
    }
    return found;
}

static int controller_move_ship_home_for_boarding(int ship)
{
    int x, y;
    if (!controller_home_staging_point(ship, &x, &y)) return 0;
    if (dist2(u[ship].x, u[ship].y, x, y) <= 6 * 6) return 1;
    if (!command_actor_to(ship, x, y)) return 0;
    for (int tick = 0; tick < 5000 && screen == SCREEN_PLAY && !won && !lost &&
         host_ticks < (u32)run_limit; ++tick) {
        step(0);
        if (!u[ship].alive) return 0;
        if (dist2(u[ship].x, u[ship].y, x, y) <= 6 * 6) {
            fprintf(stderr, "T5 shuttle staged for boarding at %d,%d\n", x, y);
            return 1;
        }
    }
    return 0;
}

static int controller_ferry_assault(int ship, int *out_landing_x, int *out_landing_y)
{
    int landing_x, landing_y, cargo = 0;
    if (ship < 0 || ship >= MAX_UNITS || !race_transport_carrier_valid(ship)) {
        fprintf(stderr, "T5 ferry has invalid Dropship slot=%d\n", ship);
        return 0;
    }
    if (!controller_landing_point(&landing_x, &landing_y)) return 0;
    if (!controller_move_ship_home_for_boarding(ship)) {
        fprintf(stderr, "T5 ferry could not stage ship for boarding at home\n");
        return 0;
    }
    trace_passenger_count = 0;
    trace_passenger_snapshot_valid = 0;
    trace_landed_manifest_count = 0;
    /* Ferry armed support units individually and leave Kerrigan/Raynor on the
       home side: the mission has explicit hero-survival loss conditions. */
    for (int i = 0; i < MAX_UNITS && screen == SCREEN_PLAY && !won && !lost &&
         host_ticks < (u32)run_limit; ++i) {
        Unit *q = &u[i];
        if (!q->alive || q->owner != SIDE || SC64_TRANSPORT_LOADED(q) ||
            q->raw_type == 19 || q->raw_type == 20 ||
            (q->type != U_MARINE && q->type != U_FIREBAT && q->type != U_VULTURE &&
             q->type != U_GOLIATH && q->type != U_TANK)) continue;
        if (trace_starport_slot < 0 || trace_starport_slot >= MAX_BUILDINGS ||
            !b[trace_starport_slot].alive ||
            dist2(q->x, q->y, b[trace_starport_slot].x,
                  b[trace_starport_slot].y) > 320 * 320) continue;
        fprintf(stderr, "T5 boarding candidate slot=%d raw=%d type=%d pos=%d,%d ship=%d,%d loaded=%d order=%u dist2=%d\n",
                i, q->raw_type, q->type, q->x, q->y, u[ship].x, u[ship].y,
                SC64_TRANSPORT_LOADED(q), q->order,
                dist2(q->x, q->y, b[trace_starport_slot].x, b[trace_starport_slot].y));
        cargo = race_transport_cargo_space(ship);
        if (cargo < 0 || cargo >= SC64_TRANSPORT_DROPSHIP_SPACE) break;
        unsigned space;
        if (!sc64_transport_space_required_raw((unsigned)q->raw_type, &space) ||
            space == SC64_TRANSPORT_SPACE_UNAVAILABLE ||
            cargo + (int)space > SC64_TRANSPORT_DROPSHIP_SPACE) continue;
        if (!controller_select_unit(i)) {
            fprintf(stderr, "T5 boarding selection failed slot=%d selected=%d count=%d building=%d\n",
                    i, q->selected, selected_count(), selected_building);
            continue;
        }
        move_cursor(u[ship].x, u[ship].y);
        tap(KEY_A); /* normal field interaction issues ORDER_BOARD */
        for (int tick = 0; tick < 4000 && q->alive && q->order == ORDER_BOARD &&
             screen == SCREEN_PLAY && !won && !lost && host_ticks < (u32)run_limit; ++tick)
            step(0);
        if (SC64_TRANSPORT_LOADED(q)) {
            cargo = race_transport_cargo_space(ship);
            fprintf(stderr, "T5 boarded assault actor slot=%d raw=%d cargo=%d/8\n",
                    i, q->raw_type, cargo);
        } else {
            fprintf(stderr, "T5 boarding incomplete slot=%d alive=%d order=%u target=%d pos=%d,%d ship=%d,%d space=%d\n",
                    i, q->alive, q->order, q->target, q->x, q->y, u[ship].x, u[ship].y,
                    race_transport_cargo_space(ship));
        }
    }
    cargo = race_transport_cargo_space(ship);
    if (cargo <= 0) return 0;
    trace_passenger_count = 0;
    for (int i = 0; i < MAX_UNITS && trace_passenger_count < SC64_TRANSPORT_DROPSHIP_SPACE; ++i)
        if (u[i].alive && SC64_TRANSPORT_LOADED(&u[i]) && u[i].target == ship) {
            int k = trace_passenger_count++;
            trace_passenger_slots[k] = i;
            trace_passenger_raws[k] = u[i].raw_type;
        }
    fprintf(stderr, "T5 controller loaded Dropship slot=%d cargo-space=%d landing=%d,%d\n",
            ship, cargo, landing_x, landing_y);
    trace_ship_slot = ship; trace_landing_x = landing_x; trace_landing_y = landing_y;
    checkpoint("dropship-loaded");
    for (int leg = 0; leg < T5_AIR_ROUTE_MAX_LEGS &&
         dist2(u[ship].x, u[ship].y, landing_x, landing_y) > 10 * 10 &&
         screen == SCREEN_PLAY && !won && !lost && host_ticks < (u32)run_limit; ++leg) {
        int wx, wy;
        if (!controller_air_next_waypoint(u[ship].x, u[ship].y,
                                          landing_x, landing_y, &wx, &wy)) {
            fprintf(stderr, "T5 no safe air route ship=%d at=%d,%d landing=%d,%d\n",
                    ship, u[ship].x, u[ship].y, landing_x, landing_y);
            return 0;
        }
        fprintf(stderr, "T5 Dropship safe waypoint leg=%d at=%d,%d target=%d,%d\n",
                leg, u[ship].x, u[ship].y, wx, wy);
        if (!command_actor_to(ship, wx, wy)) {
            fprintf(stderr, "T5 Dropship move command failed selected=%u count=%d order=%u\n",
                    u[ship].selected, selected_count(), u[ship].order);
            return 0;
        }
        int arrived = 0;
        for (int tick = 0; tick < 5000 && screen == SCREEN_PLAY && !won && !lost &&
             host_ticks < (u32)run_limit; ++tick) {
            step(0);
            trace_ship_hp = u[ship].alive ? u[ship].hp : 0;
            trace_ship_x = u[ship].x; trace_ship_y = u[ship].y;
            if (!u[ship].alive) {
                fprintf(stderr, "T5 Dropship destroyed in transit tick=%u last-pos=%d,%d\n",
                        (unsigned)host_ticks, u[ship].x, u[ship].y);
                checkpoint("dropship-destroyed-in-transit");
                return 0;
            }
            if (dist2(u[ship].x, u[ship].y, wx, wy) <= 6 * 6) { arrived = 1; break; }
        }
        if (!arrived) {
            fprintf(stderr, "T5 Dropship waypoint timeout tick=%u at=%d,%d target=%d,%d hp=%d\n",
                    (unsigned)host_ticks, u[ship].x, u[ship].y, wx, wy, u[ship].hp);
            return 0;
        }
    }
    if (dist2(u[ship].x, u[ship].y, landing_x, landing_y) > 10 * 10) {
        fprintf(stderr, "T5 Dropship travel incomplete tick=%u pos=%d,%d target=%d,%d order=%u\n",
                (unsigned)host_ticks, u[ship].x, u[ship].y, landing_x, landing_y, u[ship].order);
        return 0;
    }
    if (!controller_select_unit(ship) || !controller_choose_menu_row(13)) {
        fprintf(stderr, "T5 Dropship unload command failed selected=%u count=%d\n",
                u[ship].selected, selected_count());
        return 0;
    }
    move_cursor(landing_x, landing_y);
    tap(KEY_A);
    for (int tick = 0; tick < 800 && screen == SCREEN_PLAY && !won && !lost &&
         host_ticks < (u32)run_limit; ++tick) {
        step(0);
        if (race_transport_cargo_space(ship) == 0) break;
    }
    if (!race_transport_carrier_valid(ship) || race_transport_cargo_space(ship) != 0) {
        fprintf(stderr, "T5 unload incomplete ship=%d alive=%d cargo=%d\n",
                ship, u[ship].alive, race_transport_cargo_space(ship));
        return 0;
    }
    int landed = 0;
    for (int passenger = 0; passenger < trace_passenger_count; ++passenger) {
        int i = trace_passenger_slots[passenger];
        if (u[i].alive && u[i].owner == SIDE &&
            u[i].raw_type == trace_passenger_raws[passenger] &&
            !SC64_TRANSPORT_LOADED(&u[i]) &&
            dist2(u[i].x, u[i].y, landing_x, landing_y) < 72 * 72) {
            ++landed;
            if (trace_landed_manifest_count < SC64_TRANSPORT_DROPSHIP_SPACE) {
                int k = trace_landed_manifest_count++;
                trace_landed_slots[k] = i; trace_landed_raws[k] = u[i].raw_type;
            }
        }
    }
    fprintf(stderr, "T5 controller unload result ship=%d cargo=%d landed-near=%d at %d,%d\n",
            ship, race_transport_cargo_space(ship), landed, u[ship].x, u[ship].y);
    trace_landed_count = landed;
    checkpoint("dropship-landed");
    if (out_landing_x) *out_landing_x = landing_x;
    if (out_landing_y) *out_landing_y = landing_y;
    return landed > 0;
}

static int controller_return_dropship(int ship)
{
    if (ship < 0 || ship >= MAX_UNITS || !u[ship].alive ||
        trace_starport_slot < 0 || trace_starport_slot >= MAX_BUILDINGS ||
        !b[trace_starport_slot].alive) return 0;
    int gx, gy;
    if (!controller_home_staging_point(ship, &gx, &gy)) return 0;
    for (int leg = 0; leg < T5_AIR_ROUTE_MAX_LEGS &&
         dist2(u[ship].x, u[ship].y, gx, gy) > 10 * 10 &&
         screen == SCREEN_PLAY && !won && !lost && host_ticks < (u32)run_limit; ++leg) {
        int wx, wy;
        if (!controller_air_next_waypoint(u[ship].x, u[ship].y, gx, gy, &wx, &wy) ||
            !command_actor_to(ship, wx, wy)) return 0;
        int arrived = 0;
        for (int tick = 0; tick < 5000 && screen == SCREEN_PLAY && !won && !lost &&
             host_ticks < (u32)run_limit; ++tick) {
            step(0);
            if (!u[ship].alive) return 0;
            if (dist2(u[ship].x, u[ship].y, wx, wy) <= 6 * 6) {
                arrived = 1; break;
            }
        }
        if (!arrived) return 0;
    }
    return u[ship].alive && dist2(u[ship].x, u[ship].y, gx, gy) <= 10 * 10;
}

static void controller_assault_from_landing(int ship, int landing_x, int landing_y)
{
    int issued = 0;
    for (int i = 0; i < MAX_UNITS && host_ticks < (u32)run_limit &&
         screen == SCREEN_PLAY && !won && !lost; ++i) {
        int x, y;
        if (i == ship || !u[i].alive || u[i].owner != SIDE ||
            SC64_TRANSPORT_LOADED(&u[i]) ||
            dist2(u[i].x, u[i].y, landing_x, landing_y) >= 96 * 96) continue;
        int route = nearest_enemy_approach_from_mode(u[i].x, u[i].y, &x, &y, 1);
        if (route < 0) continue;
        fprintf(stderr, "T5 landing assault actor=%d raw=%d route=%d target=%d,%d\n",
                i, u[i].raw_type, route, x, y);
        if (command_actor_attack_to(i, x, y)) ++issued;
    }
    fprintf(stderr, "T5 landing assault issued=%d\n", issued);
    checkpoint("dropship-landing-assault");
    /* Let the first attack orders tick once, then get the empty shuttle out
       of the static AA envelope instead of waiting over the landing zone. */
    wait_frames(1);
    checkpoint("dropship-assault-result");
}

static int controller_reissue_island_assault(int starport)
{
    if (starport < 0 || starport >= MAX_BUILDINGS || !b[starport].alive) return 0;
    int issued = 0;
    for (int i = 0; i < MAX_UNITS && host_ticks < (u32)run_limit &&
         screen == SCREEN_PLAY && !won && !lost; ++i) {
        int x, y;
        if (!u[i].alive || u[i].owner != SIDE || SC64_TRANSPORT_LOADED(&u[i]) ||
            u[i].raw_type == 19 || u[i].raw_type == 20 ||
            u[i].type == U_KERRIGAN ||
            dist2(u[i].x, u[i].y, b[starport].x, b[starport].y) < 320 * 320) continue;
        if (nearest_enemy_approach_from_mode(u[i].x, u[i].y, &x, &y, 1) < 0) continue;
        if (command_actor_attack_to(i, x, y)) ++issued;
    }
    return issued;
}

static int controller_order_paid_wraiths(void)
{
    int live = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_WRAITH &&
            !SC64_TRANSPORT_LOADED(&u[i])) ++live;
    /* A lone Wraith is fed into the island's static AA and contributes little.
       Hold the force at the home rally until a useful group has formed. */
    if (live < 12) return 0;

    unsigned char handled[MAX_UNITS] = {0};
    int last_seed = -1, newly_ordered = 0, groups = 0;
    /* A double-tap selects only same-type units in the current viewport. Pan
       to successive slot clusters and give each visible group an order. */
    for (; groups < 4 && host_ticks < (u32)run_limit &&
           screen == SCREEN_PLAY && !won && !lost; ++groups) {
        int selected_wraiths = 0, seed = -1;
        if (!controller_select_visible_type_group(U_WRAITH, last_seed,
                                                  &selected_wraiths, &seed) ||
            selected_wraiths < 1 || seed <= last_seed) {
            if (!groups)
                fprintf(stderr, "T5 Wraith group selection rejected: live=%d selected=%d seed=%d after=%d count=%d building=%d\n",
                        live, selected_wraiths, seed, last_seed,
                        selected_count(), selected_building);
            break;
        }
        int cx = 0, cy = 0, members = 0, new_members = 0;
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
            if (u[i].owner != SIDE || u[i].type != U_WRAITH ||
                SC64_TRANSPORT_LOADED(&u[i])) {
                fprintf(stderr, "T5 Wraith group rejected mixed slot=%d owner=%d type=%d\n",
                        i, u[i].owner, u[i].type);
                return newly_ordered;
            }
            cx += u[i].x; cy += u[i].y; ++members;
            if (!handled[i]) { handled[i] = 1; ++new_members; }
        }
        if (!members || !new_members) break;
        cx /= members; cy /= members;
        int target = -1, best = 0x7fffffff;
        for (int j = 0; j < MAX_BUILDINGS; ++j) {
            if (!b[j].alive || b[j].owner != ENEMY) continue;
            int d = dist2(cx, cy, b[j].x, b[j].y);
            if (d < best) { best = d; target = j; }
        }
        if (target < 0) break;
        fprintf(stderr, "T5 Wraith cluster order: wing=%d new=%d selected=%d seed=%d target=%d at %d,%d tick=%u\n",
                live, new_members, selected_wraiths, seed, target,
                b[target].x, b[target].y, (unsigned)host_ticks);
        if (trace_file) checkpoint("wraith-wing-before-attack");
        tap(KEY_B);
        tap(KEY_DOWN);
        tap(KEY_A);
        move_cursor(b[target].x, b[target].y);
        tap(KEY_A);
        if (trace_file) checkpoint("wraith-wing-attack-issued");
        newly_ordered += new_members;
        last_seed = seed;
        wait_frames(180);
    }
    if (trace_file && newly_ordered) checkpoint("wraith-wing-after-attack");
    return newly_ordered;
}

static int controller_marine_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 0 &&
            !SC64_TRANSPORT_LOADED(&u[i])) ++count;
    return count;
}

static void controller_scv_counts(int *mineral_workers, int *gas_workers,
                                  int *idle_workers, int *other_workers,
                                  int *live_workers)
{
    int mineral = 0, gas_count = 0, idle = 0, other = 0, live = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || u[i].type != U_SCV ||
            SC64_TRANSPORT_LOADED(&u[i])) continue;
        ++live;
        if (u[i].order == ORDER_MINE && u[i].target >= 0 &&
            u[i].target < MAX_MINERALS &&
            (m[u[i].target].amount > 0 || u[i].carry > 0)) ++mineral;
        else if (u[i].order == ORDER_GAS && u[i].target >= 0 &&
                 u[i].target < MAX_BUILDINGS && b[u[i].target].alive &&
                 b[u[i].target].owner == SIDE && b[u[i].target].type == B_REFINERY) ++gas_count;
        else if (u[i].order == ORDER_IDLE) ++idle;
        else ++other;
    }
    if (mineral_workers) *mineral_workers = mineral;
    if (gas_workers) *gas_workers = gas_count;
    if (idle_workers) *idle_workers = idle;
    if (other_workers) *other_workers = other;
    if (live_workers) *live_workers = live;
}

static void controller_log_scv_mineral_targets(const char *label)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV &&
            u[i].order == ORDER_MINE && u[i].target >= 0 &&
            u[i].target < MAX_MINERALS)
            fprintf(stderr, "T5 %s ORDER_MINE SCV=%d patch=%d source=%d,%d remaining=%d carry=%d\n",
                    label, i, u[i].target, m[u[i].target].x, m[u[i].target].y,
                    m[u[i].target].amount, u[i].carry);
}

static int controller_count_queued_type(int type)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE)
            for (int j = 0; j < b[i].queue_count; ++j)
                if (b[i].queue[(b[i].queue_head + j) % 5] == type) ++count;
    return count;
}

static int controller_reassign_excess_gas_to_minerals(int keep_gas_workers)
{
    int mineral_workers, gas_workers, idle_workers, other_workers, live_workers;
    int moved = 0;
    if (keep_gas_workers < 0) keep_gas_workers = 0;
    controller_scv_counts(&mineral_workers, &gas_workers, &idle_workers,
                          &other_workers, &live_workers);
    for (int i = 0; i < MAX_UNITS && gas_workers > keep_gas_workers &&
         host_ticks < (u32)run_limit; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || u[i].type != U_SCV ||
            u[i].order != ORDER_GAS || u[i].target < 0 || u[i].target >= MAX_BUILDINGS ||
            !b[u[i].target].alive || b[u[i].target].type != B_REFINERY) continue;
        int patch = -1, best_score = 0x7fffffff, command = -1;
        for (int j = 0; j < MAX_BUILDINGS; ++j)
            if (b[j].alive && b[j].owner == SIDE && b[j].type == B_COMMAND) {
                command = j; break;
            }
        if (command < 0) break;
        for (int j = 0; j < MAX_MINERALS; ++j) if (m[j].amount > 0) {
            int cc_d = dist2(m[j].x, m[j].y, b[command].x, b[command].y);
            if (cc_d > 256 * 256) continue;
            int score = dist2(u[i].x, u[i].y, m[j].x, m[j].y) + cc_d / 4;
            if (score < best_score) { best_score = score; patch = j; }
        }
        if (patch < 0) break;
        int before = minerals;
        fprintf(stderr, "T5 reassigning excess gas SCV=%d to mineral patch=%d at %d,%d gas-workers=%d keep=%d\n",
                i, patch, m[patch].x, m[patch].y, gas_workers, keep_gas_workers);
        if (!controller_order_actor(i, m[patch].x, m[patch].y, 0)) continue;
        for (int t = 0; t < 1200 && host_ticks < (u32)run_limit &&
             screen == SCREEN_PLAY && !won && !lost; ++t) {
            if (u[i].order == ORDER_MINE && u[i].target >= 0 &&
                u[i].target < MAX_MINERALS && m[u[i].target].amount > 0) break;
            step(0);
        }
        if (u[i].alive && u[i].order == ORDER_MINE && u[i].target >= 0 &&
            u[i].target < MAX_MINERALS && m[u[i].target].amount > 0) {
            ++moved; --gas_workers;
            fprintf(stderr, "T5 mineral assignment verified SCV=%d patch=%d amount=%d wallet=%d->%d\n",
                    i, u[i].target, m[u[i].target].amount, before, minerals);
        } else {
            fprintf(stderr, "T5 mineral assignment incomplete SCV=%d order=%d target=%d xy=%d,%d\n",
                    i, u[i].order, u[i].target, u[i].x, u[i].y);
        }
    }
    controller_scv_counts(&mineral_workers, &gas_workers, &idle_workers,
                          &other_workers, &live_workers);
    fprintf(stderr, "T5 SCV economy verified live=%d mineral=%d gas=%d idle=%d other=%d moved=%d wallet=%d\n",
            live_workers, mineral_workers, gas_workers, idle_workers, other_workers,
            moved, minerals);
    return moved;
}

static int controller_queue_home_reinforcements(void)
{
    int marines = controller_marine_count() + controller_count_queued_type(U_MARINE);
    const int marine_reserve_target = 24;
    int scvs = 0, queued_scvs = controller_count_queued_type(U_SCV);
    int active_near_command = 0, command = find_side_building(B_COMMAND, 1);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV &&
            !SC64_TRANSPORT_LOADED(&u[i])) ++scvs;
    if (command >= 0)
        for (int i = 0; i < MAX_MINERALS; ++i)
            if (m[i].amount > 0 && dist2(m[i].x, m[i].y,
                                        b[command].x, b[command].y) <= 256 * 256)
                ++active_near_command;
    int queued = 0;
        if (marines < marine_reserve_target && minerals >= unit_cost(U_MARINE) &&
        reserved_supply() + unit_supply(U_MARINE) <= supply_cap) {
        int barracks = find_side_building(B_BARRACKS, 1);
        if (barracks >= 0 && b[barracks].queue_count < 5 &&
            controller_select_building(barracks)) {
            int before = b[barracks].queue_count;
            tap(KEY_A);
            if (b[barracks].queue_count > before) {
                ++queued;
                fprintf(stderr, "T5 replenished paid Marine queue=%d live=%d/%d\n",
                        controller_count_queued_type(U_MARINE), controller_marine_count(), marine_reserve_target);
            }
        }
    }
    if (scvs + queued_scvs < 8 && active_near_command > 0 &&
        minerals >= unit_cost(U_SCV) &&
        reserved_supply() + unit_supply(U_SCV) <= supply_cap) {
        if (command >= 0 && b[command].queue_count < 5 &&
            controller_select_building(command)) {
            int before = b[command].queue_count;
            tap(KEY_A);
            if (b[command].queue_count > before) {
                ++queued;
                fprintf(stderr, "T5 replenished paid SCV queue=%d workers=%d/8\n",
                        controller_count_queued_type(U_SCV), scvs + queued_scvs);
            }
        }
    }
    return queued;
}

static void controller_service_home_base(void)
{
    if (find_side_building(B_COMMAND, 1) < 0) return;
    (void)controller_queue_home_reinforcements();
    (void)controller_defend_command_center();
    if (host_ticks % 900 == 0) {
        (void)controller_assign_idle_scvs_to_gas(3);
        (void)controller_assign_idle_scvs_to_minerals(3);
    }
}

static int controller_select_visible_type_group(int type, int after_slot,
                                                int *out_count, int *out_seed)
{
    int seed = -1, expected = 0;
    if (out_count) *out_count = 0;
    if (out_seed) *out_seed = -1;
    if (!controller_normalize_field()) return 0;
    /* L on a selected Barracks cancels its queue without clearing selection.
       A normal drag box clears any producer first, after which the usual
       double-tap on the isolated seed groups this type. */
    if (selected_building >= 0 && !select_human_group()) return 0;
    for (int i = after_slot + 1; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == type &&
            !SC64_TRANSPORT_LOADED(&u[i])) { seed = i; break; }
    if (seed < 0) return 0;
    int cursor_unit = -1, click_x = -1, click_y = -1, click_target = -1;
    for (; seed >= 0 && seed < MAX_UNITS && cursor_unit < 0; ++seed) {
        if (!u[seed].alive || u[seed].owner != SIDE || u[seed].type != type ||
            SC64_TRANSPORT_LOADED(&u[seed])) continue;
        /* A rally next to the Starport can put units inside its 26px click
           radius. Probe nearby legal unit-click points, preferring the point
           furthest from every owned building while staying in unit range. */
        int best_clearance = -1;
        for (int dy = -32; dy <= 32 && cursor_unit < 0; dy += 4)
            for (int dx = -32; dx <= 32 && cursor_unit < 0; dx += 4) {
                int px = u[seed].x + dx, py = u[seed].y + dy;
                if (px < 0 || px >= WORLD_W || py < 0 || py >= WORLD_H ||
                    dist2(px, py, u[seed].x, u[seed].y) > 30 * 30) continue;
                int clearance = 0x7fffffff, blocked = 0;
                for (int bi = 0; bi < MAX_BUILDINGS; ++bi)
                    if (b[bi].alive && b[bi].owner == SIDE) {
                        int d = dist2(px, py, b[bi].x, b[bi].y);
                        if (d <= 26 * 26) blocked = 1;
                        if (d < clearance) clearance = d;
                    }
                /* Field interaction prioritizes boarding when a friendly
                   Dropship is within 18px of the cursor, before unit selection. */
                for (int ui = 0; ui < MAX_UNITS; ++ui)
                    if (u[ui].alive && u[ui].owner == SIDE &&
                        u[ui].type == U_DROPSHIP &&
                        dist2(px, py, u[ui].x, u[ui].y) < 18 * 18)
                        blocked = 1;
                if (blocked || clearance <= best_clearance) continue;
                move_cursor(px, py);
                int picked = selection_target();
                if (picked >= 0 && picked < MAX_UNITS && u[picked].alive &&
                    u[picked].owner == SIDE && u[picked].type == type &&
                    !SC64_TRANSPORT_LOADED(&u[picked])) {
                    cursor_unit = picked; click_x = px; click_y = py;
                    click_target = picked; best_clearance = clearance;
                }
            }
    }
    if (cursor_unit < 0) {
        fprintf(stderr, "T5 group click unavailable type=%d live=%d cursor=%d,%d selected_building=%d\n",
                type, type == U_WRAITH ? controller_wraith_count() : controller_marine_count(),
                cx, cy, selected_building);
        return 0;
    }
    int left = viewport_left(), top = viewport_top();
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == type &&
            !SC64_TRANSPORT_LOADED(&u[i]) && u[i].x >= left && u[i].x < left + 240 &&
            u[i].y >= top && u[i].y < top + 108) ++expected;
    if (!expected) return 0;
    if (selection_target() != click_target) {
        fprintf(stderr, "T5 group click target moved type=%d expected=%d actual=%d at %d,%d tick=%u\n",
                type, click_target, selection_target(), click_x, click_y, (unsigned)host_ticks);
        return 0;
    }
    tap(KEY_A);
    tap(KEY_A);
    int selected = 0, other = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
        if (u[i].owner == SIDE && u[i].type == type && !SC64_TRANSPORT_LOADED(&u[i])) ++selected;
        else ++other;
    }
    if (out_count) *out_count = selected;
    if (out_seed) *out_seed = cursor_unit;
    if (selected != expected || selected_count() != expected || other || selected_building >= 0)
        fprintf(stderr, "T5 type-group selection mismatch type=%d expected=%d selected=%d other=%d count=%d building=%d click=%d,%d tick=%u\n",
                type, expected, selected, other, selected_count(), selected_building,
                click_x, click_y, (unsigned)host_ticks);
    return selected == expected && selected_count() == expected && !other &&
           selected_building < 0;
}

static int controller_hold_visible_marines(void)
{
    int selected = 0;
    if (!controller_select_visible_type_group(U_MARINE, -1, &selected, NULL)) {
        fprintf(stderr, "T5 reserve Marines not grouped: live=%d visible-selected=%d\n",
                controller_marine_count(), selected);
        return 0;
    }
    tap(KEY_B);
    for (int i = 0; i < 9; ++i) tap(KEY_DOWN);
    tap(KEY_A); /* ordinary STOP / HOLD */
    fprintf(stderr, "T5 held visible Marine defenders=%d total-live=%d\n",
            selected, controller_marine_count());
    return 1;
}

static int controller_select_type_box_group(int type, int *out_count)
{
    int left = WORLD_W, top = WORLD_H, right = -1, bottom = -1, members = 0;
    if (out_count) *out_count = 0;
    if (!controller_normalize_field()) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == type &&
            !SC64_TRANSPORT_LOADED(&u[i]) && sc64_game_unit_selectable(u[i].type)) {
            if (u[i].x < left) left = u[i].x;
            if (u[i].x > right) right = u[i].x;
            if (u[i].y < top) top = u[i].y;
            if (u[i].y > bottom) bottom = u[i].y;
            ++members;
        }
    if (!members) return 0;
    left = clamp_world(left - 8, WORLD_W); top = clamp_world(top - 8, WORLD_H);
    right = clamp_world(right + 8, WORLD_W); bottom = clamp_world(bottom + 8, WORLD_H);
    /* Do not let the diagnostic drag include heroes, workers, or the carrier. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !SC64_TRANSPORT_LOADED(&u[i]) &&
            sc64_game_unit_selectable(u[i].type) && u[i].type != type &&
            u[i].x >= left && u[i].x <= right && u[i].y >= top && u[i].y <= bottom) {
            fprintf(stderr, "T5 type-box rejected mixed actor type=%d slot=%d at=%d,%d\n",
                    u[i].type, i, u[i].x, u[i].y);
            return 0;
        }
    move_cursor(left, top);
    step(KEY_L);
    int end_x = snap4_local(cx, right, WORLD_W);
    int end_y = snap4_local(cy, bottom, WORLD_H);
    for (int guard = 0; (cx != end_x || cy != end_y) && guard < 1000 &&
         host_ticks < (u32)run_limit; ++guard) {
        u16 held = KEY_L | KEY_R;
        if (cx < end_x) held |= KEY_RIGHT;
        if (cx > end_x) held |= KEY_LEFT;
        if (cy < end_y) held |= KEY_DOWN;
        if (cy > end_y) held |= KEY_UP;
        step(held);
    }
    step(0);
    int selected = 0, other = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
        if (u[i].owner == SIDE && u[i].type == type && !SC64_TRANSPORT_LOADED(&u[i])) ++selected;
        else ++other;
    }
    if (out_count) *out_count = selected;
    if (selected != members || other || selected_building >= 0 || selected_count() != members) {
        fprintf(stderr, "T5 type-box selection mismatch type=%d members=%d selected=%d other=%d building=%d count=%d rect=%d,%d..%d,%d\n",
                type, members, selected, other, selected_building, selected_count(),
                left, top, right, bottom);
        return 0;
    }
    return 1;
}

/* Defensive orders stay on the real controller path: select visible Marines
 * by the ordinary double-tap, then use the normal Attack command on an
 * actual hostile that has approached the owned Command Center. */
static int controller_defend_command_center(void)
{
    static u32 last_order_tick;
    int command = find_side_building(B_COMMAND, 1);
    if (command < 0) return 0;
    int target = -1, best = 300 * 300;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || !owners_hostile(SIDE, u[i].owner)) continue;
        int d = dist2(u[i].x, u[i].y, b[command].x, b[command].y);
        if (d < best) { best = d; target = i; }
    }
    if (target < 0 || host_ticks - last_order_tick < 1200) return 0;
    fprintf(stderr, "T5 CC pressure tick=%u hp=%d nearest-hostile slot=%d raw=%d owner=%d xy=%d,%d d2=%d\n",
            (unsigned)host_ticks, b[command].hp, target, u[target].raw_type,
            u[target].owner, u[target].x, u[target].y, best);
    int listed = 0;
    for (int i = 0; i < MAX_UNITS && listed < 8; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 0) {
            fprintf(stderr, "T5 CC defense Marine slot=%d xy=%d,%d hp=%d d2=%d selected=%d\n",
                    i, u[i].x, u[i].y, u[i].hp,
                    dist2(u[i].x, u[i].y, b[command].x, b[command].y), u[i].selected);
            ++listed;
        }
    int selected = 0;
    if (!controller_select_visible_type_group(U_MARINE, -1, &selected, NULL) || selected < 1) {
        fprintf(stderr, "T5 CC defense could not select Marine same-type group count=%d building=%d mode=%d menu=%d\n",
                selected, selected_building, mode, menu_open);
        last_order_tick = host_ticks;
        return 0;
    }
    tap(KEY_B); tap(KEY_DOWN); tap(KEY_A); /* normal Attack mode */
    move_cursor(u[target].x, u[target].y);
    tap(KEY_A);
    fprintf(stderr, "T5 CC defense Attack-Move issued selected-Marines=%d target=%d remaining-hp=%d\n",
            selected, target, u[target].hp);
    last_order_tick = host_ticks;
    return selected;
}

static int controller_safe_home_point(int actor, int *out_x, int *out_y)
{
    static u8 seen[MAP_CELLS];
    static u16 queue[MAP_CELLS], distance[MAP_CELLS];
    int command = -1, nearest = 0x7fffffff;
    if (actor < 0 || actor >= MAX_UNITS || !u[actor].alive || !out_x || !out_y)
        return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive || b[i].owner != SIDE || b[i].type != B_COMMAND) continue;
        int d = dist2(u[actor].x, u[actor].y, b[i].x, b[i].y);
        if (d < nearest) { nearest = d; command = i; }
    }
    if (command < 0) return 0;
    build_path_grid();
    memset(seen, 0, sizeof(seen)); memset(distance, 0, sizeof(distance));
    int start = u[actor].x / 16 + (u[actor].y / 16) * MAP_COLS;
    if (start < 0 || start >= MAP_CELLS) return 0;
    int head = 0, tail = 0, best = -1, best_safety = -1, best_route = 0x7fffffff;
    seen[start] = 1; queue[tail++] = (u16)start;
    while (head < tail) {
        int cell = queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
        int x = col * 16 + 8, y = row * 16 + 8;
        if (dist2(x, y, b[command].x, b[command].y) <= 144 * 144 &&
            SC64_GRID_GET(path_walkable, cell) && unit_step_clear(&u[actor], x, y)) {
            int hero_clear = 1;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (i != actor && u[i].alive && u[i].owner == SIDE &&
                    (u[i].type == U_KERRIGAN || u[i].raw_type == 19 ||
                     u[i].raw_type == 20) && dist2(x, y, u[i].x, u[i].y) < 28 * 28) {
                    hero_clear = 0; break;
                }
            if (!hero_clear) goto home_next;
            int safety = 0x7fffffff;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner == ENEMY) {
                    int d = dist2(x, y, u[i].x, u[i].y);
                    if (d < safety) safety = d;
                }
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (b[i].alive && b[i].owner == ENEMY) {
                    int d = dist2(x, y, b[i].x, b[i].y);
                    if (d < safety) safety = d;
                }
            if (safety > best_safety || (safety == best_safety && distance[cell] < best_route)) {
                best = cell; best_safety = safety; best_route = distance[cell];
            }
        }
home_next:
#define T5_HOME_NEXT(n_) do { int z_=(n_); if (!seen[z_] && SC64_GRID_GET(path_walkable,z_)) { seen[z_]=1; distance[z_]=(u16)(distance[cell]+1); queue[tail++]=(u16)z_; } } while (0)
        if (col > 0) T5_HOME_NEXT(cell - 1);
        if (col + 1 < MAP_COLS) T5_HOME_NEXT(cell + 1);
        if (row > 0) T5_HOME_NEXT(cell - MAP_COLS);
        if (row + 1 < MAP_ROWS) T5_HOME_NEXT(cell + MAP_COLS);
#undef T5_HOME_NEXT
    }
    if (best < 0) return 0;
    *out_x = (best % MAP_COLS) * 16 + 8;
    *out_y = (best / MAP_COLS) * 16 + 8;
    fprintf(stderr, "T5 hero safe home point actor=%d ownerCC=%d route=%u nearest-hostile-distance2=%d point=%d,%d\n",
            actor, command, distance[best], best_safety, *out_x, *out_y);
    return 1;
}

static int controller_hold_actor(int actor)
{
    if (!controller_select_unit(actor)) return 0;
    tap(KEY_B);
    for (int i = 0; i < 9; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    return u[actor].alive && u[actor].order == ORDER_IDLE;
}

static int controller_park_actor_safely(int actor, int label)
{
    int x = -1, y = -1;
    if (actor < 0 || !controller_safe_home_point(actor, &x, &y)) return 0;
    if (!controller_order_actor(actor, x, y, 0)) return 0;
    for (int i = 0; i < 5000 && u[actor].alive && screen == SCREEN_PLAY &&
         !won && !lost && host_ticks < (u32)run_limit; ++i) {
        if (dist2(u[actor].x, u[actor].y, x, y) <= 8 * 8) break;
        step(0);
    }
    int arrived = u[actor].alive && dist2(u[actor].x, u[actor].y, x, y) <= 8 * 8;
    int held = arrived && controller_hold_actor(actor);
    fprintf(stderr, "T5 hero parking label=%d raw=%d slot=%d arrived=%d held=%d final=%d,%d order=%u\n",
            label, u[actor].raw_type, actor, arrived, held, u[actor].x, u[actor].y,
            u[actor].order);
    return held;
}

static int controller_park_hero_safely(int raw_type)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == raw_type)
            return controller_park_actor_safely(i, raw_type);
    return 0;
}

static int controller_park_kerrigan_safely(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_KERRIGAN)
            return controller_park_actor_safely(i, U_KERRIGAN);
    return 0;
}

static int controller_assign_idle_scvs_to_gas(int maximum)
{
    int existing = 0, assigned = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV &&
            u[i].order == ORDER_GAS && u[i].target >= 0 &&
            u[i].target < MAX_BUILDINGS && b[u[i].target].alive &&
            b[u[i].target].owner == SIDE && b[u[i].target].type == B_REFINERY)
            ++existing;
    if (maximum <= existing) return 0;
    int needed = maximum - existing;
    for (int ref = 0; ref < MAX_BUILDINGS && assigned < needed; ++ref) {
        if (!b[ref].alive || b[ref].owner != SIDE || b[ref].type != B_REFINERY ||
            !building_complete(ref)) continue;
        for (int i = 0; i < MAX_UNITS && assigned < needed; ++i) {
            if (!u[i].alive || u[i].owner != SIDE || u[i].type != U_SCV ||
                (u[i].order != ORDER_IDLE && u[i].order != ORDER_MINE)) continue;
            if (!controller_select_unit(i)) continue;
            move_cursor(b[ref].x, b[ref].y);
            tap(KEY_A); /* ordinary worker interaction assigns this Refinery */
            if (u[i].order == ORDER_GAS && u[i].target == ref) {
                ++assigned;
                fprintf(stderr, "T5 assigned SCV slot=%d to owned Refinery=%d total-gas=%d/%d\n",
                        i, ref, existing + assigned, maximum);
            }
        }
    }
    if (!assigned && existing < maximum)
        fprintf(stderr, "T5 no idle/mining SCV could be assigned to an owned Refinery total=%d/%d\n",
                existing, maximum);
    return assigned;
}

static int controller_assign_idle_scvs_to_minerals(int maximum)
{
    int command = find_side_building(B_COMMAND, 1), assigned = 0;
    int idle_seen = 0, active_fields = 0, nearby_fields = 0;
    for (int j = 0; j < MAX_MINERALS; ++j) if (m[j].amount > 0) {
        ++active_fields;
        if (command >= 0 && dist2(m[j].x, m[j].y, b[command].x, b[command].y) <= 256 * 256)
            ++nearby_fields;
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV &&
            !SC64_TRANSPORT_LOADED(&u[i]) && u[i].order == ORDER_IDLE) ++idle_seen;
    if (maximum <= 0) return 0;
    if (command < 0) {
        fprintf(stderr, "T5 mineral retarget skipped: no completed owned Command Center; idle=%d active-fields=%d\n",
                idle_seen, active_fields);
        return 0;
    }
    if (idle_seen)
        fprintf(stderr, "T5 mineral retarget check: CC=%d at=%d,%d idle=%d active-fields=%d within-256=%d\n",
                command, b[command].x, b[command].y, idle_seen, active_fields, nearby_fields);
    for (int i = 0; i < MAX_UNITS && assigned < maximum &&
         host_ticks < (u32)run_limit; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || u[i].type != U_SCV ||
            SC64_TRANSPORT_LOADED(&u[i]) || u[i].order != ORDER_IDLE) continue;
        int patch = -1, best_score = 0x7fffffff;
        for (int j = 0; j < MAX_MINERALS; ++j) if (m[j].amount > 0) {
            int cc_d = dist2(m[j].x, m[j].y, b[command].x, b[command].y);
            int worker_d = dist2(m[j].x, m[j].y, u[i].x, u[i].y);
            if (cc_d > 256 * 256 || worker_d > 288 * 288) continue;
            int score = worker_d + cc_d / 4;
            if (score < best_score) { best_score = score; patch = j; }
        }
        if (patch < 0) {
            int active = 0, nearest = -1, nearest_d = 0x7fffffff;
            for (int j = 0; j < MAX_MINERALS; ++j) if (m[j].amount > 0) {
                int d = dist2(m[j].x, m[j].y, b[command].x, b[command].y);
                ++active;
                if (d < nearest_d) { nearest_d = d; nearest = j; }
            }
            fprintf(stderr, "T5 no nearby mineral assignment for idle-SCV=%d xy=%d,%d active-fields=%d nearest-patch=%d d2=%d\n",
                    i, u[i].x, u[i].y, active, nearest, nearest_d);
            continue;
        }
        int before = minerals;
        if (dist2(u[i].x, u[i].y, m[patch].x, m[patch].y) > 7 * 7 &&
            !controller_order_actor(i, m[patch].x, m[patch].y, 0)) {
            fprintf(stderr, "T5 could not issue mineral MOVE idle-SCV=%d order=%d target=%d building=%d mode=%d\n",
                    i, u[i].order, u[i].target, selected_building, mode);
            continue;
        }
        for (int t = 0; t < 1200 && host_ticks < (u32)run_limit &&
             screen == SCREEN_PLAY && !won && !lost; ++t) {
            if (u[i].order == ORDER_MINE && u[i].target >= 0 &&
                u[i].target < MAX_MINERALS && m[u[i].target].amount > 0) break;
            step(0);
        }
        if (u[i].alive && u[i].owner == SIDE && u[i].order == ORDER_MINE &&
            u[i].target >= 0 && u[i].target < MAX_MINERALS && m[u[i].target].amount > 0) {
            ++assigned;
            fprintf(stderr, "T5 mineral assignment verified idle-SCV=%d patch=%d source=%d,%d amount=%d wallet=%d->%d\n",
                    i, u[i].target, m[u[i].target].x, m[u[i].target].y,
                    m[u[i].target].amount, before, minerals);
        } else {
            fprintf(stderr, "T5 idle-SCV mineral assignment failed slot=%d order=%d target=%d xy=%d,%d\n",
                    i, u[i].order, u[i].target, u[i].x, u[i].y);
        }
    }
    return assigned;
}

static int controller_wraith_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_WRAITH &&
            !SC64_TRANSPORT_LOADED(&u[i])) ++count;
    return count;
}

static int controller_wraith_queue_count(void)
{
    if (trace_starport_slot < 0 || trace_starport_slot >= MAX_BUILDINGS ||
        !b[trace_starport_slot].alive) return 0;
    int count = 0;
    for (int i = 0; i < b[trace_starport_slot].queue_count; ++i)
        if (b[trace_starport_slot].queue[(b[trace_starport_slot].queue_head + i) % 5] ==
            U_WRAITH) ++count;
    return count;
}

static int controller_isolate_one_wraith(void)
{
    if (trace_isolated_wraith_slot >= 0 && trace_isolated_wraith_slot < MAX_UNITS &&
        u[trace_isolated_wraith_slot].alive && u[trace_isolated_wraith_slot].owner == SIDE &&
        u[trace_isolated_wraith_slot].type == U_WRAITH) {
        int clear = 1;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == SIDE &&
                dist2(u[trace_isolated_wraith_slot].x, u[trace_isolated_wraith_slot].y,
                      b[i].x, b[i].y) < 48 * 48) clear = 0;
        if (clear) return 1;
    }
    int slot = -1, nearest = -1, nearest_d = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || u[i].type != U_WRAITH ||
            SC64_TRANSPORT_LOADED(&u[i])) continue;
        int bd = 0x7fffffff, bi = -1;
        for (int j = 0; j < MAX_BUILDINGS; ++j)
            if (b[j].alive && b[j].owner == SIDE) {
                int d = dist2(u[i].x, u[i].y, b[j].x, b[j].y);
                if (d < bd) { bd = d; bi = j; }
            }
        if (bi >= 0 && bd < 48 * 48 && bd < nearest_d) {
            slot = i; nearest = bi; nearest_d = bd;
        }
    }
    if (slot < 0) return 1;
    int dx = u[slot].x - b[nearest].x, dy = u[slot].y - b[nearest].y;
    if (!dx && !dy) dx = 1;
    int norm = abs(dx) + abs(dy);
    int tx = clamp_world(u[slot].x + dx * 72 / norm, WORLD_W);
    int ty = clamp_world(u[slot].y + dy * 72 / norm, WORLD_H);
    int safe = 0;
    for (int attempt = 0; attempt < 16 && !safe; ++attempt) {
        safe = 1;
        for (int j = 0; j < MAX_BUILDINGS; ++j)
            if (b[j].alive && b[j].owner == SIDE &&
                dist2(tx, ty, b[j].x, b[j].y) < 48 * 48) safe = 0;
        if (!safe) { tx = clamp_world(tx + 16, WORLD_W); ty = clamp_world(ty + 16, WORLD_H); }
    }
    if (!safe || !controller_order_actor(slot, tx, ty, 0)) {
        fprintf(stderr, "T5 Wraith isolation order failed slot=%d selected=%d count=%d\n",
                slot, u[slot].selected, selected_count());
        return 0;
    }
    trace_isolated_wraith_slot = slot;
    fprintf(stderr, "T5 isolated Wraith slot=%d from-building=%d at %d,%d target=%d,%d\n",
            slot, nearest, u[slot].x, u[slot].y, tx, ty);
    for (int t = 0; t < 1200 && u[slot].alive &&
         dist2(u[slot].x, u[slot].y, tx, ty) >= 8 * 8 &&
         host_ticks < (u32)run_limit && screen == SCREEN_PLAY && !won && !lost; ++t)
        step(0);
    int arrived = u[slot].alive && dist2(u[slot].x, u[slot].y, tx, ty) < 8 * 8;
    fprintf(stderr, "T5 isolated Wraith arrival slot=%d alive=%d xy=%d,%d target=%d,%d arrived=%d\n",
            slot, u[slot].alive, u[slot].x, u[slot].y, tx, ty, arrived);
    if (trace_file) checkpoint("wraith-seed-isolated");
    return arrived;
}

static int controller_queue_paid_wraith(int starport)
{
    if (starport < 0 || starport >= MAX_BUILDINGS || !b[starport].alive ||
        b[starport].owner != SIDE || !building_complete(starport) ||
        !race_t5_starport_has_control_tower(starport)) return 0;
    if (!controller_select_building(starport)) return 0;
    for (int i = 0; i < 5 && train_type != U_WRAITH; ++i) tap(KEY_R);
    if (train_type != U_WRAITH) return 0;
    int queued = 0;
    while (b[starport].alive && b[starport].queue_count < 5 &&
           minerals >= unit_cost(U_WRAITH) && gas >= unit_gas_cost(U_WRAITH) &&
           reserved_supply() + unit_supply(U_WRAITH) <= supply_cap &&
           screen == SCREEN_PLAY && !won && !lost && host_ticks < (u32)run_limit) {
        int before = b[starport].queue_count;
        tap(KEY_A);
        if (b[starport].queue_count == before) break;
        ++queued;
    }
    if (queued)
        fprintf(stderr, "T5 queued paid Wraiths=%d at Starport=%d queue=%d wallet=%d/%d\n",
                queued, starport, b[starport].queue_count, minerals, gas);
    return queued;
}

static int controller_board_hero_on_dropship(int ship, int actor)
{
    if (ship < 0 || ship >= MAX_UNITS || actor < 0 || actor >= MAX_UNITS ||
        !u[ship].alive || !u[actor].alive || u[ship].owner != SIDE ||
        u[actor].owner != SIDE || SC64_TRANSPORT_LOADED(&u[actor])) return 0;
    unsigned space = 0;
    if (!sc64_transport_space_required_raw((unsigned)u[actor].raw_type, &space) ||
        space == SC64_TRANSPORT_SPACE_UNAVAILABLE ||
        race_transport_cargo_space(ship) + (int)space > SC64_TRANSPORT_DROPSHIP_SPACE)
        return 0;
    if (dist2(u[ship].x, u[ship].y, u[actor].x, u[actor].y) >= 18 * 18) {
        if (!controller_order_actor(ship, u[actor].x, u[actor].y, 0)) return 0;
        for (int t = 0; t < 1200 && host_ticks < (u32)run_limit &&
             screen == SCREEN_PLAY && !won && !lost; ++t) {
            step(0);
            if (!u[ship].alive || !u[actor].alive ||
                dist2(u[ship].x, u[ship].y, u[actor].x, u[actor].y) < 18 * 18) break;
        }
    }
    if (!u[ship].alive || !u[actor].alive ||
        dist2(u[ship].x, u[ship].y, u[actor].x, u[actor].y) >= 18 * 18 ||
        !controller_select_unit(actor)) return 0;
    move_cursor(u[ship].x, u[ship].y);
    tap(KEY_A); /* ordinary field interaction issues ORDER_BOARD */
    for (int t = 0; t < 1200 && u[actor].alive && !SC64_TRANSPORT_LOADED(&u[actor]) &&
         host_ticks < (u32)run_limit && screen == SCREEN_PLAY && !won && !lost; ++t)
        step(0);
    int loaded = u[actor].alive && SC64_TRANSPORT_LOADED(&u[actor]) &&
                 u[actor].target == ship;
    fprintf(stderr, "T5 hero passenger slot=%d raw=%d loaded=%d carrier=%d cargo=%d\n",
            actor, u[actor].raw_type, loaded, ship,
            u[ship].alive ? race_transport_cargo_space(ship) : -1);
    return loaded;
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
        if (host_ticks % 300 == 0) controller_service_home_base();
        if (trace_file && host_ticks && host_ticks % T5_CHECKPOINT_PERIOD == 0)
            checkpoint("periodic");
    }
}

static void checkpoint(const char *kind)
{
    unsigned faults = 0;
    int scv_minerals, scv_gas, scv_idle, scv_other, scv_live;
    controller_scv_counts(&scv_minerals, &scv_gas, &scv_idle, &scv_other, &scv_live);
    int owned_command_centers = 0, active_minerals = 0, nearby_active_minerals = 0;
    int command = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_COMMAND && building_complete(i)) {
            if (command < 0) command = i;
            ++owned_command_centers;
        }
    for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0) {
        ++active_minerals;
        if (command >= 0 && dist2(m[i].x, m[i].y, b[command].x, b[command].y) <= 256 * 256)
            ++nearby_active_minerals;
    }
    if (!trace_passenger_snapshot_valid && !strcmp(kind, "dropship-landed")) {
        for (int i = 0; i < trace_passenger_count; ++i) {
            int slot = trace_passenger_slots[i];
            int valid = slot >= 0 && slot < MAX_UNITS;
            int alive = valid && u[slot].alive && u[slot].raw_type == trace_passenger_raws[i];
            trace_passenger_alive[i] = alive;
            trace_passenger_loaded[i] = alive && SC64_TRANSPORT_LOADED(&u[slot]) &&
                                         u[slot].target == trace_ship_slot;
            trace_passenger_x[i] = alive ? u[slot].x : -1;
            trace_passenger_y[i] = alive ? u[slot].y : -1;
        }
        trace_passenger_snapshot_valid = 1;
        trace_passenger_snapshot_tick = host_ticks;
    }
    if (trace_ship_slot >= 0 && trace_ship_slot < MAX_UNITS) {
        trace_ship_hp = u[trace_ship_slot].alive ? u[trace_ship_slot].hp : 0;
        trace_ship_x = u[trace_ship_slot].x; trace_ship_y = u[trace_ship_slot].y;
    }
    for (unsigned p = 0; p < 8; ++p) faults += race_t5.player[p].fault != 0;
    if (trace_file) {
        trace_prefix();
        fprintf(trace_file,
            "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%d,\"elapsed\":%u,"
            "\"screen\":%d,\"race_mode\":%d,\"race_selected_mission\":%d,"
            "\"won\":%d,\"lost\":%d,\"faults\":%u,\"switch4\":%u,\"switch6\":%u,"
            "\"objective\":%u,\"side_units\":%d,\"enemy_units\":%d,"
            "\"side_buildings\":%d,\"enemy_buildings\":%d,\"kerrigan_alive\":%d,"
            "\"raynor_alive\":%d,\"marine_defenders\":%d,\"marine_queued\":%d,"
            "\"scv_live\":%d,\"scv_mineral_workers\":%d,\"scv_gas_workers\":%d,"
            "\"scv_idle_workers\":%d,\"scv_other_workers\":%d,"
            "\"owned_command_centers\":%d,\"active_minerals\":%d,\"nearby_active_minerals\":%d,"
            "\"supply_used\":%d,\"supply_cap\":%d,"
            "\"paid_wraith_alive\":%d,\"paid_wraith_queued\":%d,"
            "\"minerals\":%d,\"gas\":%d,\"enemy_minerals\":%d,\"enemy_gas\":%d,"
            "\"starport_slot\":%d,\"starport_raw\":%d,\"starport_complete\":%d,"
            "\"control_tower_slot\":%d,\"control_tower_raw\":%d,\"control_tower_complete\":%d,"
            "\"transport_ship_slot\":%d,\"transport_ship_raw\":%d,"
            "\"transport_cargo_space\":%d,\"transport_ship_alive\":%d,"
            "\"transport_ship_hp\":%d,\"transport_ship_xy\":[%d,%d],"
            "\"transport_landing\":[%d,%d],"
            "\"transport_landed_count\":%d,\"transport_loaded\":[",
            kind, (unsigned)host_ticks, frame, (unsigned)race_campaign.elapsed_frames,
            screen, race_mode, race_selected_mission, won, lost, faults,
            !!(race_t5.switches.bits[0] & 0x10),
            !!(race_t5.switches.bits[0] & 0x40), race_campaign.objective_stage,
            owner_units(SIDE), owner_units(ENEMY), owner_buildings(SIDE),
            owner_buildings(ENEMY), kerrigan_alive(), raynor_alive(),
            controller_marine_count(), controller_count_queued_type(U_MARINE),
            scv_live, scv_minerals, scv_gas, scv_idle, scv_other,
            owned_command_centers, active_minerals, nearby_active_minerals,
            reserved_supply(), supply_cap,
            controller_wraith_count(), controller_wraith_queue_count(), minerals, gas,
            race_minerals[ENEMY], race_gas[ENEMY], trace_starport_slot,
            trace_starport_slot >= 0 && trace_starport_slot < MAX_BUILDINGS ?
                b[trace_starport_slot].raw_type : -1,
            trace_starport_slot >= 0 && trace_starport_slot < MAX_BUILDINGS &&
                b[trace_starport_slot].alive && !b[trace_starport_slot].under_construction,
            trace_tower_slot,
            trace_tower_slot >= 0 && trace_tower_slot < MAX_BUILDINGS ?
                b[trace_tower_slot].raw_type : -1,
            trace_tower_slot >= 0 && trace_tower_slot < MAX_BUILDINGS &&
                b[trace_tower_slot].alive && !b[trace_tower_slot].under_construction,
            trace_ship_slot,
            trace_ship_slot >= 0 && trace_ship_slot < MAX_UNITS ? u[trace_ship_slot].raw_type : -1,
            trace_ship_slot >= 0 && trace_ship_slot < MAX_UNITS && u[trace_ship_slot].alive ?
                race_transport_cargo_space(trace_ship_slot) : -1,
            trace_ship_slot >= 0 && trace_ship_slot < MAX_UNITS && u[trace_ship_slot].alive,
            trace_ship_hp, trace_ship_x, trace_ship_y,
            trace_landing_x, trace_landing_y, trace_landed_count);
        int first_loaded = 1;
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
                SC64_TRANSPORT_LOADED(&u[i]) && u[i].target == trace_ship_slot) {
            fprintf(trace_file, "%s{\"slot\":%d,\"raw_type\":%d}",
                    first_loaded ? "" : ",", i, u[i].raw_type);
            first_loaded = 0;
        }
        fputs("],\"transport_landed\":[", trace_file);
        for (int i = 0; i < trace_landed_manifest_count; ++i)
            fprintf(trace_file, "%s{\"slot\":%d,\"raw_type\":%d}",
                    i ? "," : "", trace_landed_slots[i], trace_landed_raws[i]);
        fprintf(trace_file, "],\"transport_passenger_snapshot_tick\":%u,\"transport_passengers\":[",
                (unsigned)(trace_passenger_snapshot_valid ? trace_passenger_snapshot_tick : host_ticks));
        for (int i = 0; i < trace_passenger_count; ++i) {
            int slot = trace_passenger_slots[i];
            int valid = slot >= 0 && slot < MAX_UNITS;
            int alive = trace_passenger_snapshot_valid ? trace_passenger_alive[i] :
                valid && u[slot].alive && u[slot].raw_type == trace_passenger_raws[i];
            int loaded = trace_passenger_snapshot_valid ? trace_passenger_loaded[i] :
                alive && SC64_TRANSPORT_LOADED(&u[slot]) && u[slot].target == trace_ship_slot;
            int x = trace_passenger_snapshot_valid ? trace_passenger_x[i] : alive ? u[slot].x : -1;
            int y = trace_passenger_snapshot_valid ? trace_passenger_y[i] : alive ? u[slot].y : -1;
            fprintf(trace_file,
                    "%s{\"slot\":%d,\"original_raw_type\":%d,\"alive\":%d,"
                    "\"loaded\":%d,\"xy\":[%d,%d]}",
                    i ? "," : "", slot, trace_passenger_raws[i], alive, loaded,
                    x, y);
        }
        fputs("]}", trace_file);
        fflush(trace_file);
    }
    fprintf(stderr,
        "T5 %s tick=%u frame=%d result=%s switch4=%u switch6=%u "
        "units=%d/%d buildings=%d/%d heroes=%d/%d wallet=%d/%d enemy=%d/%d faults=%u\n",
        kind, (unsigned)host_ticks, frame, won ? "WON" : lost ? "LOST" : "RUNNING",
        !!(race_t5.switches.bits[0] & 0x10),
        !!(race_t5.switches.bits[0] & 0x40), owner_units(SIDE), owner_units(ENEMY),
        owner_buildings(SIDE), owner_buildings(ENEMY), kerrigan_alive(), raynor_alive(),
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
                "\"policy\":\"bounded-normal-controller-paid-air-wing-attempt\","
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
        int starport = -1, ship = -1, landing_x = -1, landing_y = -1;
        checkpoint("before-starport-tech");
        (void)controller_park_hero_safely(19);
        (void)controller_park_hero_safely(20);
        (void)controller_park_kerrigan_safely();
        (void)controller_hold_visible_marines();
        (void)controller_assign_idle_scvs_to_gas(3);
        checkpoint("heroes-held-safe-home");
        if (controller_build_starport(&starport) &&
            controller_wait_building(starport, 6000)) {
            fprintf(stderr, "T5 controller completed Starport slot=%d at %d,%d\n",
                    starport, b[starport].x, b[starport].y);
            trace_starport_slot = starport;
            checkpoint("starport-complete");
            if (controller_build_control_tower(starport)) {
                for (int i = 0; i < MAX_BUILDINGS; ++i)
                    if (b[i].alive && b[i].raw_type == 115 && b[i].level == starport + 1) {
                        trace_tower_slot = i; break;
                    }
                checkpoint("control-tower-complete");
                if (controller_build_dropship(starport, &ship)) {
                    checkpoint("dropship-complete");
                    /* Add paid supply capacity so the Wraith wave is not capped
                       by the pre-existing garrison, while keeping the Marines
                       and heroes in the home defense. */
                    int depots_built = 0;
                    while (depots_built < 8 && supply_cap - reserved_supply() <
                           20 * unit_supply(U_WRAITH) && screen == SCREEN_PLAY &&
                           !won && !lost && host_ticks < (u32)run_limit) {
                        int depot = -1;
                        if (!controller_build_supply_depot(&depot)) break;
                        ++depots_built;
                        checkpoint("paid-supply-depot-complete");
                    }
                    fprintf(stderr, "T5 Wraith production capacity: used=%d cap=%d live=%d queued=%d depots-added=%d\n",
                            reserved_supply(), supply_cap, controller_wraith_count(),
                            controller_wraith_queue_count(), depots_built);
                    int mineral_wallet_before = minerals;
                    int mineral_assignments = controller_assign_idle_scvs_to_minerals(3);
                    wait_frames(600);
                    int scv_mineral_workers, scv_gas_workers, scv_idle_workers;
                    int scv_other_workers, scv_live_workers;
                    controller_scv_counts(&scv_mineral_workers, &scv_gas_workers,
                                          &scv_idle_workers, &scv_other_workers,
                                          &scv_live_workers);
                    fprintf(stderr, "T5 mineral income preflight assignments=%d SCVs=%d mineral=%d gas=%d idle=%d other=%d wallet=%d->%d\n",
                            mineral_assignments, scv_live_workers, scv_mineral_workers,
                            scv_gas_workers, scv_idle_workers, scv_other_workers,
                            mineral_wallet_before, minerals);
                    controller_log_scv_mineral_targets("preflight");
                    u32 last_wraith_order_tick = 0;
                    for (unsigned tick = 0; tick < 100000 && owner_buildings(ENEMY) &&
                         screen == SCREEN_PLAY && !won && !lost &&
                         host_ticks < (u32)run_limit; ++tick) {
                        step(0);
                        if (tick % 300 == 0) {
                            (void)controller_assign_idle_scvs_to_gas(3);
                            (void)controller_queue_paid_wraith(starport);
                            (void)controller_queue_home_reinforcements();
                            (void)controller_defend_command_center();
                        }
                        if (tick % 1800 == 0) {
                            int retargeted = controller_assign_idle_scvs_to_minerals(2);
                            if (retargeted)
                                fprintf(stderr, "T5 maintenance mineral retargets=%d tick=%u\n",
                                        retargeted, (unsigned)host_ticks);
                            if (gas > 2000)
                                (void)controller_reassign_excess_gas_to_minerals(2);
                        }
                        if (controller_wraith_count() >= 12 &&
                            host_ticks - last_wraith_order_tick >= 1200) {
                            fprintf(stderr, "T5 Wraith wing command check alive=%d queue=%d enemy-buildings=%d tick=%u\n",
                                    controller_wraith_count(), controller_wraith_queue_count(),
                                    owner_buildings(ENEMY), (unsigned)host_ticks);
                            int issued = controller_order_paid_wraiths();
                            if (issued) fprintf(stderr,
                                "T5 gathered Wraith wing=%d issued=%d at tick=%u\n",
                                controller_wraith_count(), issued, (unsigned)host_ticks);
                            if (issued) checkpoint("wraith-wing-assault");
                            last_wraith_order_tick = host_ticks;
                        }
                        if (trace_file && host_ticks && host_ticks % T5_CHECKPOINT_PERIOD == 0)
                            checkpoint(controller_wraith_count() >= 12 ?
                                       "wraith-wing-progress" : "wraith-wing-gathering");
                    }
                }
            }
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
    int command_center = find_side_building(B_COMMAND, 1);
    int qualified = won && !lost && mission_unsupported_count == 0 &&
        race_campaign.phase == SC64_RACE_PHASE_COMPLETE &&
        (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) &&
        (race_completed & (1 << SC64_T5_GLOBAL_ID)) &&
        kerrigan_alive() && raynor_alive() && command_center >= 0 &&
        owner_buildings(ENEMY) == 0;
    fprintf(stderr,
        "T5 qualification won=%d lost=%d faults=%u phase=%u primary=%u completed=%u "
        "Kerrigan=%d Raynor=%d command_center=%d enemy_buildings=%d result=%s\n",
        won, lost, mission_unsupported_count, race_campaign.phase,
        !!(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY),
        !!(race_completed & (1 << SC64_T5_GLOBAL_ID)), kerrigan_alive(),
        raynor_alive(), command_center >= 0, owner_buildings(ENEMY),
        qualified ? "PASS" : "FAIL");
    return qualified ? 0 : 1;
}
