/*
 * Normal-controller T9 (New Gettysburg) campaign run.
 *
 * The compile-time probe selects mission 12 for a host fixture. After
 * game_init(), all simulation changes are made through game_update() with
 * ordinary buttons. The runner never writes actors, triggers, resources or
 * mission state.
 *
 * Information discipline: the policy plans from the player's own actors and
 * wallet, the static terrain and mineral layout, the immutable scenario
 * placement table (the briefing's map knowledge), and hostile actors only
 * while their cell is in the player's current `visible` grid. Hostile
 * buildings seen once are remembered at their last seen position and are
 * forgotten only when that cell is visible again without them. Diagnostic
 * printouts may read more, but no input decision does.
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

enum {
    T9_DEFAULT_LIMIT = 500000, T9_CHECKPOINT_PERIOD = 1000,
    T9_RAW_SCV = 7, T9_RAW_KERRIGAN = 16, T9_RAW_COMMAND = 106,
    T9_RAW_REFINERY = 110, T9_RAW_STARPORT = 114,
    T9_PROTOSS_SLOT = 1, T9_ZERG_SLOT = 3,
    T9_SCV_TARGET = 20, T9_OPENING_GAS = 3, T9_PRODUCTION_GAS = 10,
    T9_MINERAL_FLOOR = 9, T9_STARPORTS = 3,
    T9_FIRST_WAVE = 24, T9_REINFORCE_WAVE = 12, T9_RETREAT_AT = 6,
    T9_HINTS = 64
};
static FILE *trace;
static int trace_first_event = 1;
static u16 previous_keys;
static int limit_ticks = T9_DEFAULT_LIMIT;
static u32 host_tick;
static int failed_action;
static int verbose;
static int watch_slot = -1, watch_order = -1;

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

static int living_force_buildings(int raw_owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_owner == raw_owner) ++count;
    return count;
}

static int source_unit(int raw_type, int owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == raw_type &&
            (owner < 0 || u[i].owner == owner)) return i;
    return -1;
}

static int source_building(int raw_type, int owner)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw_type &&
            (owner < 0 || b[i].owner == owner)) return i;
    return -1;
}

/* Trace observations are diagnostic: the replay tool compares them with the
 * ARM run, and none of them feeds an input decision. */
static void checkpoint(const char *kind)
{
    if (!trace) return;
    int raw16 = 0, side_units = 0, zerg_units = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].raw_type == T9_RAW_KERRIGAN) ++raw16;
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
        race_completed, living_force_buildings(T9_PROTOSS_SLOT),
        living_force_buildings(T9_ZERG_SLOT), raw16, side_units,
        zerg_units, minerals, gas, selected_count(), selected_building,
        failed_action);
    fflush(trace);
}

static void observe(void);

static void step(u16 held)
{
    if (host_tick >= (u32)limit_ticks || screen != SCREEN_PLAY || won || lost) return;
    u16 pressed = held & (u16)~previous_keys;
    int input_changed = held != previous_keys;
    static int zerg_hp[MAX_BUILDINGS];
    if (verbose)
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            zerg_hp[i] = b[i].alive && b[i].raw_owner == T9_ZERG_SLOT ? b[i].hp : -1;
    game_update(held, pressed);
    if (verbose)
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (zerg_hp[i] >= 0 && (!b[i].alive || b[i].hp < zerg_hp[i])) {
                fprintf(stderr, "T9 Zerg building hit tick=%u slot=%d raw=%d hp=%d->%d at=%d,%d\n",
                        (unsigned)host_tick + 1, i, b[i].raw_type, zerg_hp[i],
                        b[i].alive ? b[i].hp : 0, b[i].x, b[i].y);
                for (int j = 0; j < MAX_UNITS; ++j)
                    if (u[j].alive && u[j].target == MAX_UNITS + i)
                        fprintf(stderr, "  attacker slot=%d owner=%d raw=%d order=%d at=%d,%d\n",
                                j, u[j].owner, u[j].raw_type, u[j].order, u[j].x, u[j].y);
            }
    previous_keys = held;
    ++host_tick;
    if (trace && input_changed) {
        trace_event_prefix();
        fprintf(trace,"{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}",
                (unsigned)host_tick, held, pressed);
    }
    observe();
    if (watch_slot >= 0 && u[watch_slot].order != watch_order) {
        fprintf(stderr, "T9 watch slot=%d order %d->%d target=%d tick=%u held=%u mode=%d menu=%d sel=%d cursor=%d,%d\n",
                watch_slot, watch_order, u[watch_slot].order, u[watch_slot].target,
                (unsigned)host_tick, held, mode, menu_open, selected_count(), cx, cy);
        watch_order = u[watch_slot].order;
    }
    if (trace && host_tick % T9_CHECKPOINT_PERIOD == 0) checkpoint("periodic");
}

static int live(void)
{ return host_tick < (u32)limit_ticks && screen == SCREEN_PLAY && !won && !lost; }

static void tap(u16 key) { step(key); step(0); }

/* Open the command list and put its cursor on Move using field inputs. */
static void tap_b(void)
{
    int fresh = !menu_open && mode == MODE_NORMAL;
    tap(KEY_B);
    if (fresh && menu_open) {
        int count = field_menu_count();
        if (count <= 0 || MENU_MOVE >= count) {
            failed_action = 1;
            return;
        }
        int up = (menu_cursor - MENU_MOVE + count) % count;
        int down = (MENU_MOVE - menu_cursor + count) % count;
        u16 key = up <= down ? KEY_UP : KEY_DOWN;
        int taps = up <= down ? up : down;
        for (int i = 0; i < taps; ++i) tap(key);
        if (!menu_open || menu_cursor != MENU_MOVE) failed_action = 1;
    }
}

static int normal_field(void)
{
    for (int i = 0; i < 3 && (menu_open || mode != MODE_NORMAL); ++i) tap_b();
    return screen == SCREEN_PLAY && !menu_open && mode == MODE_NORMAL;
}

/* Hold R for the fast cursor and steer it onto a 4-pixel grid point. */
static void move_cursor(int x, int y)
{
    x = snap4(cx, x, WORLD_W); y = snap4(cy, y, WORLD_H);
    for (int i = 0; i < 3000 && (cx != x || cy != y) && live(); ++i) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT;
        if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        if (cy > y) held |= KEY_UP;
        step(held);
    }
    step(0);
}

/* SELECT drops the current selection. With nothing selected it would
 * pause instead, so it is pressed only while something is selected. */
static void drop_selection(void)
{
    if (selected_count() || selected_building >= 0) tap(KEY_SELECT);
    if (screen == SCREEN_PAUSE) { tap(KEY_START); failed_action = 1; }
}

static int click_select_unit(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        u[slot].owner != SIDE || !normal_field()) return 0;
    drop_selection();
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

static int select_building(int slot)
{
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive ||
        b[slot].owner != SIDE || !normal_field()) return 0;
    if (selected_building == slot) return 1;
    drop_selection();
    move_cursor(b[slot].x, b[slot].y);
    tap(KEY_A);
    return selected_building == slot && selected_count() == 0;
}

/* Move (row 0) or Attack (row 1) for the current unit selection. */
static void order_selection(int attack, int x, int y)
{
    tap_b();
    if (attack) tap(KEY_DOWN);
    tap(KEY_A);
    move_cursor(x, y);
    tap(KEY_A);
}

/* ---- Player-visible information --------------------------------------- */

static int cell_visible_xy(int x, int y)
{
    int col = clamp_coord(x / 16, MAP_COLS), row = clamp_coord(y / 16, MAP_ROWS);
    return SC64_GRID_GET(visible, col + row * MAP_COLS);
}

static int unit_seen(int i)
{ return u[i].alive && owners_hostile(SIDE, u[i].owner) && cell_visible_xy(u[i].x, u[i].y); }

/* Last seen hostile buildings, updated only from visible cells. */
static struct { u8 known, raw_owner; u16 raw_type; s16 x, y; } memory[MAX_BUILDINGS];

static void observe(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (memory[i].known && cell_visible_xy(memory[i].x, memory[i].y) &&
            !(b[i].alive && b[i].x == memory[i].x && b[i].y == memory[i].y))
            memory[i].known = 0;
        if (b[i].alive && owners_hostile(SIDE, b[i].owner) &&
            cell_visible_xy(b[i].x, b[i].y)) {
            memory[i].known = 1;
            memory[i].raw_owner = (u8)b[i].raw_owner;
            memory[i].raw_type = (u16)b[i].raw_type;
            memory[i].x = (s16)b[i].x;
            memory[i].y = (s16)b[i].y;
        }
    }
}

/* Briefing knowledge: the immutable placement table's Protoss structures.
 * A hint is cleared once its point has been seen with no remembered
 * Protoss building nearby. */
static struct { s16 x, y; u8 cleared; } hint[T9_HINTS];
static int hint_count;

static void load_hints(void)
{
    const Sc64RaceMap *map = &sc64_race_maps[race_selected_mission];
    for (int i = 0; i < map->unit_count && hint_count < T9_HINTS; ++i) {
        const Sc64ImportedUnit *p = &map->units[i];
        if (p->owner != T9_PROTOSS_SLOT || p->source_type < 154 || p->source_type > 172)
            continue;
        /* Placement coordinates are already in GBA world pixels. */
        hint[hint_count].x = (s16)p->x_px;
        hint[hint_count].y = (s16)p->y_px;
        hint[hint_count].cleared = 0;
        ++hint_count;
    }
}

static int remembered_protoss_near(int x, int y, int radius)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (memory[i].known && memory[i].raw_owner == T9_PROTOSS_SLOT &&
            dist2(memory[i].x, memory[i].y, x, y) < radius * radius) return 1;
    return 0;
}

static void update_hints(void)
{
    for (int i = 0; i < hint_count; ++i)
        if (!hint[i].cleared && cell_visible_xy(hint[i].x, hint[i].y) &&
            !remembered_protoss_near(hint[i].x, hint[i].y, 48))
            hint[i].cleared = 1;
}

/* Air threat from what the player can see now plus remembered static
 * defenses. Dragoons and Photon Cannons are the Protoss anti-air. */
static int air_threat_at(int x, int y, int margin)
{
    int risk = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (unit_seen(i)) {
        if (!sc64_race_unit_can_attack_air(u[i].type)) continue;
        int r = (sc64_race_unit_range_vs(u[i].type, 1) + 1) / 2 + margin;
        if (dist2(x, y, u[i].x, u[i].y) < r * r) ++risk;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (memory[i].known && (memory[i].raw_type == 162 || memory[i].raw_type == 144 ||
                                memory[i].raw_type == 124)) {
            int r = 112 + margin;
            if (dist2(x, y, memory[i].x, memory[i].y) < r * r) risk += 2;
        }
    return risk;
}

/* ---- Own forces and economy ------------------------------------------- */

static int side_count_raw(int raw_type)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == raw_type) ++count;
    return count;
}

static int side_building_count(int raw_type, int complete_only)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].raw_type == raw_type &&
            (!complete_only || building_complete(i))) ++count;
    return count;
}

static int is_wraith(int i)
{
    return u[i].alive && u[i].owner == SIDE && u[i].type == U_WRAITH &&
        !SC64_TRANSPORT_LOADED(&u[i]);
}

static int wraith_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (is_wraith(i)) ++count;
    return count;
}

static int fleet_center(int *x, int *y)
{
    int sx = 0, sy = 0, n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (is_wraith(i)) { sx += u[i].x; sy += u[i].y; ++n; }
    if (!n) return 0;
    *x = sx / n; *y = sy / n;
    return n;
}

static int queued_units(int type)
{
    int queued = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE)
        for (int q = 0; q < b[i].queue_count && q < 5; ++q)
            if (b[i].queue[(b[i].queue_head + q) % 5] == type) ++queued;
    return queued;
}

static int gas_workers(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == T9_RAW_SCV &&
            u[i].order == ORDER_GAS) ++count;
    return count;
}

static int home_cc(void) { return source_building(T9_RAW_COMMAND, SIDE); }

/* Home mineral line: the patches within reach of the starting Command
 * Center. Other fields lie beside hostile bases. */
static u8 home_patch[MAX_MINERALS];

static void mark_home_patches(void)
{
    int cc = home_cc();
    for (int i = 0; i < MAX_MINERALS; ++i)
        home_patch[i] = cc >= 0 && m[i].amount > 0 &&
            dist2(m[i].x, m[i].y, b[cc].x, b[cc].y) < 224 * 224;
}

static int home_minerals_left(void)
{
    int total = 0;
    for (int i = 0; i < MAX_MINERALS; ++i) if (home_patch[i]) total += m[i].amount;
    return total;
}

static int home_patch_center(int *x, int *y)
{
    int sx = 0, sy = 0, n = 0;
    for (int i = 0; i < MAX_MINERALS; ++i)
        if (home_patch[i] && m[i].amount > 0) { sx += m[i].x; sy += m[i].y; ++n; }
    if (!n) return 0;
    *x = sx / n; *y = sy / n;
    return n;
}

static int home_geyser(void)
{
    int cc = home_cc(), best = -1, best_d = 0x7fffffff;
    if (cc < 0) return -1;
    for (int i = 0; i < geyser_count; ++i) {
        int d = dist2(b[cc].x, b[cc].y, geysers[i].x, geysers[i].y);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

static int choose_build_site(int type, int ox, int oy, int *out_x, int *out_y)
{
    int best = 0x7fffffff, found = 0;
    for (int ring = 1; ring <= 12; ++ring)
        for (int dy = -ring; dy <= ring; ++dy)
            for (int dx = -ring; dx <= ring; ++dx) {
                if (FAST_ABS(dx) != ring && FAST_ABS(dy) != ring) continue;
                int x = clamp_coord(ox + dx * 16, WORLD_W);
                int y = clamp_coord(oy + dy * 16, WORLD_H);
                int score = dx * dx + dy * dy;
                if (score < best && race_can_site(SIDE, type, x, y) && can_place(type, x, y)) {
                    *out_x = x; *out_y = y; best = score; found = 1;
                }
            }
    return found;
}

static int build_from_worker(int slot, int type, int menu_choice, int raw_type, int x, int y)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive ||
        u[slot].owner != SIDE || !IS_WORKER(u[slot].type) ||
        !click_select_unit(slot)) return 0;
    tap_b();
    for (int row = 0; row < menu_choice; ++row) tap(KEY_DOWN);
    tap(KEY_A);
    if (mode != MODE_BUILD || menu != type) { normal_field(); return 0; }
    move_cursor(x, y);
    tap(KEY_A);
    normal_field();
    /* The SCV walks to the site before the structure is placed. */
    for (int t = 0; t < 1500 && live() && u[slot].alive &&
         u[slot].order == ORDER_BUILD_PENDING; ++t) step(0);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw_type && b[i].owner == SIDE &&
            !building_complete(i) && dist2(b[i].x, b[i].y, x, y) < 16 * 16) return 1;
    return 0;
}

/* A worker near the given point. Gas workers and builders keep their jobs;
 * idle workers and those that drifted to a distant field go first. */
static int pick_worker(int x, int y)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == T9_RAW_SCV &&
            u[i].order != ORDER_GAS && !SC64_BUILD_WORKER_ORDER(u[i].order) &&
            !SC64_TRANSPORT_LOADED(&u[i])) {
            int d = dist2(u[i].x, u[i].y, x, y) / 4;
            if (u[i].order == ORDER_MINE && u[i].target >= 0 &&
                u[i].target < MAX_MINERALS && home_patch[u[i].target]) d += 1 << 24;
            if (d < best_d) { best_d = d; best = i; }
        }
    return best;
}

static int building_in_progress(int raw_type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].raw_type == raw_type &&
            !building_complete(i)) return 1;
    return 0;
}

static int build_structure(int type, int menu_choice, int raw_type, int ox, int oy)
{
    int worker = pick_worker(ox, oy), x, y;
    if (worker < 0 || minerals < sc64_race_building_rules[type].minerals ||
        gas < sc64_race_building_rules[type].gas ||
        !race_late_building_product_available(SIDE, u[worker].raw_owner, type) ||
        !choose_build_site(type, ox, oy, &x, &y)) return 0;
    int ok = build_from_worker(worker, type, menu_choice, raw_type, x, y);
    fprintf(stderr, "T9 build type=%d raw=%d at=%d,%d worker=%d ok=%d tick=%u wallet=%d/%d\n",
            type, raw_type, x, y, worker, ok, (unsigned)host_tick, minerals, gas);
    return ok;
}

static int build_refinery(void)
{
    int g = home_geyser(), cc = home_cc();
    if (g < 0 || cc < 0 || minerals < 100 || source_building(T9_RAW_REFINERY, SIDE) >= 0)
        return 0;
    int worker = pick_worker(geysers[g].x, geysers[g].y);
    if (worker < 0 ||
        !race_late_building_product_available(SIDE, u[worker].raw_owner, B_REFINERY)) return 0;
    int ok = build_from_worker(worker, B_REFINERY, MENU_REFINERY, T9_RAW_REFINERY,
                               geysers[g].x, geysers[g].y);
    fprintf(stderr, "T9 build Refinery at=%d,%d worker=%d ok=%d tick=%u wallet=%d/%d\n",
            geysers[g].x, geysers[g].y, worker, ok, (unsigned)host_tick, minerals, gas);
    return ok;
}

static int assign_gas_worker(void)
{
    int refinery = source_building(T9_RAW_REFINERY, SIDE);
    if (refinery < 0 || !building_complete(refinery)) return 0;
    int worker = pick_worker(b[refinery].x, b[refinery].y);
    if (worker < 0 || !click_select_unit(worker)) return 0;
    move_cursor(b[refinery].x, b[refinery].y);
    tap(KEY_A);
    int ok = u[worker].alive && u[worker].order == ORDER_GAS;
    fprintf(stderr, "T9 gas worker slot=%d ok=%d tick=%u\n", worker, ok, (unsigned)host_tick);
    return ok;
}

/* Use the visible contextual mineral command; movement does not assign work. */
static int send_worker_home(int slot)
{
    static const int dx[] = {0,8,-8,0,0,8,-8,8,-8};
    static const int dy[] = {0,0,0,8,-8,8,8,-8,-8};
    if (home_cc() < 0 || !u[slot].alive || u[slot].carry || !click_select_unit(slot)) return 0;
    for (int j = 0; j < MAX_MINERALS && host_tick < (u32)limit_ticks; ++j) {
        if (!home_patch[j] || m[j].amount <= 0 || !currently_visible(m[j].x,m[j].y)) continue;
        for (unsigned k = 0; k < sizeof(dx)/sizeof(dx[0]); ++k) {
            move_cursor(m[j].x+dx[k],m[j].y+dy[k]);
            if (!u[slot].alive || !u[slot].selected || u[slot].carry) return 0;
            if (selected_worker_mineral_target() != j || field_ui_a_action(selected_count()) != UI_A_MINE) continue;
            tap(KEY_A);
            if (u[slot].alive && u[slot].order == ORDER_MINE && u[slot].target == j) return 1;
        }
    }
    return 0;
}

static int queue_scv(void)
{
    int cc = home_cc();
    if (cc < 0 || b[cc].queue_count >= 2 || minerals < unit_cost(U_SCV) ||
        supply_used + unit_supply(U_SCV) > supply_cap || !select_building(cc)) return 0;
    int before = b[cc].queue_count;
    move_cursor(b[cc].x, b[cc].y);
    tap(KEY_A);
    return b[cc].queue_count > before;
}

static int queue_wraith(void)
{
    for (int port = 0; port < MAX_BUILDINGS; ++port)
        if (b[port].alive && b[port].owner == SIDE && b[port].raw_type == T9_RAW_STARPORT &&
            building_complete(port) && b[port].queue_count < 2 &&
            minerals >= unit_cost(U_WRAITH) && gas >= unit_gas_cost(U_WRAITH) &&
            supply_used + unit_supply(U_WRAITH) <= supply_cap && select_building(port)) {
            for (int k = 0; k < 5 && train_type != U_WRAITH; ++k) tap(KEY_R);
            if (train_type != U_WRAITH) {
                if (verbose)
                    fprintf(stderr, "T9 Wraith type unavailable port=%d train=%d selected=%d tick=%u\n",
                            port, train_type, selected_building, (unsigned)host_tick);
                continue;
            }
            int before = b[port].queue_count;
            move_cursor(b[port].x, b[port].y);
            tap(KEY_A);
            if (b[port].queue_count > before) {
                fprintf(stderr,"T9 Wraith queued tick=%u port=%d queue=%d wallet=%d/%d wraiths=%d\n",
                        (unsigned)host_tick, port, b[port].queue_count, minerals, gas,
                        wraith_count());
                return 1;
            }
        }
    return 0;
}

static int reserved_supply_total(void)
{
    return supply_used + queued_units(U_SCV) * unit_supply(U_SCV) +
        queued_units(U_WRAITH) * unit_supply(U_WRAITH);
}

/* One bounded economy decision per call; each issues ordinary inputs. */
static int manage_economy(void)
{
    int cc = home_cc();
    if (cc < 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == T9_RAW_SCV &&
            u[i].order == ORDER_IDLE && !u[i].carry && send_worker_home(i)) return 1;
    int scvs = side_count_raw(T9_RAW_SCV) + queued_units(U_SCV);
    int refinery = source_building(T9_RAW_REFINERY, SIDE);
    int ports = side_building_count(T9_RAW_STARPORT, 0);
    int left = home_minerals_left(), alive = side_count_raw(T9_RAW_SCV);
    /* Gas never takes the last mineral workers while the home line lasts;
     * afterwards every SCV moves to the inexhaustible geyser. */
    int want_gas = left <= 0 ? alive :
        side_building_count(T9_RAW_STARPORT, 1) ? T9_PRODUCTION_GAS : T9_OPENING_GAS;
    if (left > 0 && want_gas > alive - T9_MINERAL_FLOOR) want_gas = alive - T9_MINERAL_FLOOR;
    if (left > 0 && want_gas < 2 && refinery >= 0) want_gas = alive > 3 ? 2 : 0;
    if (supply_cap < 400 && supply_cap - reserved_supply_total() < 12 &&
        !building_in_progress(109) && minerals >= 100)
        return build_structure(B_DEPOT, MENU_DEPOT, 109, b[cc].x, b[cc].y + 48);
    if (ports == 0 && minerals >= 150 && gas >= 100)
        return build_structure(B_STARPORT, MENU_STARPORT, T9_RAW_STARPORT, b[cc].x - 64, b[cc].y);
    if (refinery < 0 && minerals >= 100) return build_refinery();
    if (scvs < T9_SCV_TARGET && left > 1500 && minerals >= 50 && queue_scv()) return 1;
    if (refinery >= 0 && building_complete(refinery) && gas_workers() < want_gas &&
        assign_gas_worker())
        return 1;
    if (queue_wraith()) return 1;
    if (ports < T9_STARPORTS && !building_in_progress(T9_RAW_STARPORT) &&
        minerals >= 250 && gas >= 100)
        return build_structure(B_STARPORT, MENU_STARPORT, T9_RAW_STARPORT, b[cc].x - 64, b[cc].y);
    return 0;
}

/* ---- Fleet control ---------------------------------------------------- */

/* Wave membership is the controller's own bookkeeping: production and other
 * selections change u[].selected between fleet orders. */
static u8 in_wave[MAX_UNITS];

static void prune_wave(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (in_wave[i] && !is_wraith(i)) in_wave[i] = 0;
}

static int wave_count(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (in_wave[i]) ++n;
    return n;
}

static int wave_near(int x, int y, int radius)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (in_wave[i] && dist2(u[i].x, u[i].y, x, y) < radius * radius) ++n;
    return n;
}

/* The main clump: the Wraith with the most Wraiths within 96 pixels,
 * counting wave members only while a wave is out. */
static int fleet_clump(int *x, int *y)
{
    int best = -1, best_n = 0, members = wave_count();
    for (int i = 0; i < MAX_UNITS; ++i) if (is_wraith(i) && (!members || in_wave[i])) {
        int n = 0;
        for (int j = 0; j < MAX_UNITS; ++j)
            if (is_wraith(j) && (!members || in_wave[j]) &&
                dist2(u[i].x, u[i].y, u[j].x, u[j].y) < 96 * 96) ++n;
        if (n > best_n) { best_n = n; best = i; }
    }
    if (best < 0) return 0;
    *x = u[best].x; *y = u[best].y;
    return best_n;
}

static int selection_is_fleet(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
        if (!is_wraith(i)) return 0;
        ++count;
    }
    return count > 0 && count == selected_count() && selected_building < 0 ? count : 0;
}

/* Select the main Wraith clump with an L box when no other own unit shares
 * it, else with the ordinary visible same-type double tap. */
static int select_fleet(void)
{
    int kx, ky;
    if (!normal_field() || !fleet_clump(&kx, &ky)) return 0;
    drop_selection();
    int left = WORLD_W, top = WORLD_H, right = -1, bottom = -1, seed = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (is_wraith(i) && dist2(u[i].x, u[i].y, kx, ky) < 160 * 160) {
            if (u[i].x < left) left = u[i].x;
            if (u[i].x > right) right = u[i].x;
            if (u[i].y < top) top = u[i].y;
            if (u[i].y > bottom) bottom = u[i].y;
            if (seed < 0) seed = i;
        }
    left = clamp_coord(left - 8, WORLD_W); top = clamp_coord(top - 8, WORLD_H);
    right = clamp_coord(right + 8, WORLD_W); bottom = clamp_coord(bottom + 8, WORLD_H);
    int foreign = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !is_wraith(i) &&
            !SC64_TRANSPORT_LOADED(&u[i]) &&
            u[i].x >= left - 8 && u[i].x <= right + 8 &&
            u[i].y >= top - 8 && u[i].y <= bottom + 8) foreign = 1;
    if (!foreign) {
        move_cursor(left, top);
        step(KEY_L);
        int end_x = snap4(cx, right, WORLD_W), end_y = snap4(cy, bottom, WORLD_H);
        for (int i = 0; i < 3000 && (cx != end_x || cy != end_y) && live(); ++i) {
            u16 held = KEY_L | KEY_R;
            if (cx < end_x) held |= KEY_RIGHT;
            if (cx > end_x) held |= KEY_LEFT;
            if (cy < end_y) held |= KEY_DOWN;
            if (cy > end_y) held |= KEY_UP;
            step(held);
        }
        step(0);
    } else {
        if (!click_select_unit(seed)) return 0;
        tap(KEY_A);
    }
    int count = selection_is_fleet();
    if (!count) { normal_field(); return 0; }
    for (int i = 0; i < MAX_UNITS; ++i) if (is_wraith(i) && u[i].selected) in_wave[i] = 1;
    return count;
}

static int fleet_order(int attack, int x, int y)
{
    int selected = selection_is_fleet();
    for (int i = 0; selected && i < MAX_UNITS; ++i)
        if (in_wave[i] && !u[i].selected) selected = 0;
    if (!selected) selected = select_fleet();
    if (!selected) { failed_action = 1; return 0; }
    order_selection(attack, x, y);
    int ordered = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (is_wraith(i) && u[i].selected &&
            u[i].order == (attack ? ORDER_ATTACK_MOVE : ORDER_MOVE)) ++ordered;
    if (verbose)
        fprintf(stderr, "T9 fleet %s to=%d,%d ordered=%d/%d tick=%u\n",
                attack ? "attack" : "move", x, y, ordered, selected, (unsigned)host_tick);
    return ordered;
}

static int fleet_busy(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (in_wave[i] && u[i].order != ORDER_IDLE) return 1;
    return 0;
}

/* Zerg structures from the briefing placement table. Any Zerg structure
 * death is a native defeat, so routes and rally points keep clear. */
static struct { s16 x, y; } zerg_site[64];
static int zerg_site_count;

static void load_zerg_sites(void)
{
    const Sc64RaceMap *map = &sc64_race_maps[race_selected_mission];
    for (int i = 0; i < map->unit_count && zerg_site_count < 64; ++i) {
        const Sc64ImportedUnit *p = &map->units[i];
        if (p->owner != T9_ZERG_SLOT || p->source_type < 106) continue;
        zerg_site[zerg_site_count].x = (s16)p->x_px;
        zerg_site[zerg_site_count].y = (s16)p->y_px;
        ++zerg_site_count;
    }
}

enum { T9_ROUTE_STEP = 32, T9_ROUTE_MAX_CELLS = 4096, T9_ZERG_CLEARANCE = 192 };

static int zerg_blocked(int x, int y)
{
    for (int i = 0; i < zerg_site_count; ++i)
        if (dist2(x, y, zerg_site[i].x, zerg_site[i].y) < T9_ZERG_CLEARANCE * T9_ZERG_CLEARANCE)
            return 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (memory[i].known && memory[i].raw_owner == T9_ZERG_SLOT &&
            dist2(x, y, memory[i].x, memory[i].y) < T9_ZERG_CLEARANCE * T9_ZERG_CLEARANCE)
            return 1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (unit_seen(i) && u[i].raw_owner == T9_ZERG_SLOT &&
            dist2(x, y, u[i].x, u[i].y) < 160 * 160) return 1;
    return 0;
}

/* Threat-aware air route on 32-pixel cells. Wraiths fly in eight
 * directions, so a waypoint is emitted at every heading change; each leg is
 * then flown exactly as planned. The goal is the final point. */
static int plan_route(int sx, int sy, int gx, int gy, int *wx, int *wy, int max)
{
    int cols = (WORLD_W + T9_ROUTE_STEP - 1) / T9_ROUTE_STEP;
    int rows = (WORLD_H + T9_ROUTE_STEP - 1) / T9_ROUTE_STEP;
    int cells = cols * rows;
    static u16 risk[T9_ROUTE_MAX_CELLS];
    static u8 blocked[T9_ROUTE_MAX_CELLS], done[T9_ROUTE_MAX_CELLS];
    static u32 cost[T9_ROUTE_MAX_CELLS];
    static s16 previous[T9_ROUTE_MAX_CELLS];
    static int path[T9_ROUTE_MAX_CELLS];
    if (cells > T9_ROUTE_MAX_CELLS || max < 1) return 0;
    for (int at = 0; at < cells; ++at) {
        int x = (at % cols) * T9_ROUTE_STEP + T9_ROUTE_STEP / 2;
        int y = (at / cols) * T9_ROUTE_STEP + T9_ROUTE_STEP / 2;
        blocked[at] = (u8)zerg_blocked(x, y);
        risk[at] = (u16)air_threat_at(x, y, 32);
        cost[at] = 0xffffffffu; previous[at] = -1; done[at] = 0;
    }
    int start = clamp_coord(sx / T9_ROUTE_STEP, cols) + clamp_coord(sy / T9_ROUTE_STEP, rows) * cols;
    int goal = clamp_coord(gx / T9_ROUTE_STEP, cols) + clamp_coord(gy / T9_ROUTE_STEP, rows) * cols;
    if (blocked[goal]) return 0;
    blocked[start] = 0;
    cost[start] = 0;
    for (;;) {
        int at = -1;
        u32 best = 0xffffffffu;
        for (int i = 0; i < cells; ++i) if (!done[i] && cost[i] < best) { best = cost[i]; at = i; }
        if (at < 0 || at == goal) break;
        done[at] = 1;
        int col = at % cols, row = at / cols;
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            if (!dx && !dy) continue;
            int nc = col + dx, nr = row + dy;
            if (nc < 0 || nr < 0 || nc >= cols || nr >= rows) continue;
            int next = nc + nr * cols;
            if (done[next] || blocked[next]) continue;
            u32 edge = (dx && dy ? 14u : 10u) + risk[next] * 60u;
            if (best + edge < cost[next]) { cost[next] = best + edge; previous[next] = (s16)at; }
        }
    }
    if (cost[goal] == 0xffffffffu) return 0;
    int n = 0;
    for (int at = goal; at >= 0 && n < cells; at = previous[at]) path[n++] = at;
    int emitted = 0;
    for (int k = n - 2; k >= 1 && emitted < max - 1; --k) {
        int a = path[k + 1], here = path[k], c = path[k - 1];
        int dx1 = here % cols - a % cols, dy1 = here / cols - a / cols;
        int dx2 = c % cols - here % cols, dy2 = c / cols - here / cols;
        if (dx1 == dx2 && dy1 == dy2) continue;
        wx[emitted] = (here % cols) * T9_ROUTE_STEP + T9_ROUTE_STEP / 2;
        wy[emitted] = (here / cols) * T9_ROUTE_STEP + T9_ROUTE_STEP / 2;
        ++emitted;
    }
    wx[emitted] = gx; wy[emitted] = gy;
    return emitted + 1;
}

/* Nearest remembered Protoss building, else the nearest uncleared briefing
 * point. Zerg structures are never chosen. */
static int choose_target(int fx, int fy, int *tx, int *ty)
{
    int best_d = 0x7fffffff, found = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (memory[i].known && memory[i].raw_owner == T9_PROTOSS_SLOT &&
            !zerg_blocked(memory[i].x, memory[i].y)) {
            int d = dist2(fx, fy, memory[i].x, memory[i].y);
            if (d < best_d) { best_d = d; *tx = memory[i].x; *ty = memory[i].y; found = 1; }
        }
    if (found) return 1;
    for (int i = 0; i < hint_count; ++i)
        if (!hint[i].cleared && !zerg_blocked(hint[i].x, hint[i].y)) {
            int d = dist2(fx, fy, hint[i].x, hint[i].y);
            if (d < best_d) { best_d = d; *tx = hint[i].x; *ty = hint[i].y; found = 1; }
        }
    return found;
}

/* East of the mineral line and clear of the Zerg base. */
static void rally_point(int *x, int *y)
{
    int cc = home_cc();
    *x = cc >= 0 ? b[cc].x + 288 : 912;
    *y = cc >= 0 ? b[cc].y : 376;
}

enum { FLEET_GATHER, FLEET_ROUTE, FLEET_ENGAGE };
static int fleet_state = FLEET_GATHER;
static int wave_size = T9_FIRST_WAVE;
static int route_x[64], route_y[64], route_len, route_at, route_attack;
static u32 leg_deadline;

static int start_route(int gx, int gy, int attack)
{
    int fx, fy;
    if (!fleet_clump(&fx, &fy)) return 0;
    route_len = plan_route(fx, fy, gx, gy, route_x, route_y, 64);
    route_at = 0;
    route_attack = attack;
    if (verbose)
        fprintf(stderr, "T9 route from=%d,%d to=%d,%d legs=%d attack=%d tick=%u\n",
                fx, fy, gx, gy, route_len, attack, (unsigned)host_tick);
    if (!route_len) return 0;
    fleet_state = FLEET_ROUTE;
    leg_deadline = 0;
    return 1;
}

/* Send each idle Wraith that is away from the rally point there. */
static void gather_one(void)
{
    int rx, ry;
    rally_point(&rx, &ry);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (is_wraith(i) && !in_wave[i] && u[i].order == ORDER_IDLE &&
            dist2(u[i].x, u[i].y, rx, ry) > 64 * 64) {
            if (click_select_unit(i)) order_selection(0, rx, ry);
            return;
        }
}

static void manage_fleet(void)
{
    int n = wraith_count(), rx, ry;
    update_hints();
    rally_point(&rx, &ry);
    if (fleet_state == FLEET_GATHER) {
        memset(in_wave, 0, sizeof(in_wave));
        gather_one();
        int kx, ky, clump = fleet_clump(&kx, &ky);
        if (clump >= wave_size && dist2(kx, ky, rx, ry) < 96 * 96) {
            int tx, ty;
            if (select_fleet() && choose_target(kx, ky, &tx, &ty) && start_route(tx, ty, 1)) {
                fprintf(stderr, "T9 wave launch wraiths=%d clump=%d tick=%u wallet=%d/%d\n",
                        n, clump, (unsigned)host_tick, minerals, gas);
                checkpoint("wave-launch");
            }
        }
        return;
    }
    prune_wave();
    int total = wave_count();
    if (total < T9_RETREAT_AT && route_attack) {
        fprintf(stderr, "T9 wave retreat members=%d wraiths=%d tick=%u\n", total, n,
                (unsigned)host_tick);
        checkpoint("wave-retreat");
        wave_size = T9_REINFORCE_WAVE;
        if (!total || !start_route(rx, ry, 0)) {
            memset(in_wave, 0, sizeof(in_wave));
            fleet_state = FLEET_GATHER;
        }
        return;
    }
    if (fleet_state == FLEET_ROUTE) {
        if (route_at > 0) {
            int near = wave_near(route_x[route_at - 1], route_y[route_at - 1], 56);
            if (near * 3 < total * 2 && host_tick < leg_deadline) return;
        }
        if (route_at >= route_len) {
            if (!route_attack) memset(in_wave, 0, sizeof(in_wave));
            fleet_state = route_attack ? FLEET_ENGAGE : FLEET_GATHER;
            leg_deadline = host_tick + 2400;
            return;
        }
        int last = route_at == route_len - 1;
        int fx, fy;
        fleet_center(&fx, &fy);
        if (!fleet_order(last && route_attack, route_x[route_at], route_y[route_at])) {
            fleet_state = FLEET_GATHER;
            return;
        }
        int d = FAST_ABS(route_x[route_at] - fx) + FAST_ABS(route_y[route_at] - fy);
        leg_deadline = host_tick + 300 + d;
        ++route_at;
        if (last && route_attack) {
            fleet_state = FLEET_ENGAGE;
            leg_deadline = host_tick + 2400;
        }
        return;
    }
    /* FLEET_ENGAGE: wait for the attack-move to finish, then retarget. */
    if (fleet_busy() && host_tick < leg_deadline) return;
    int fx, fy, tx, ty;
    if (!fleet_clump(&fx, &fy)) { fleet_state = FLEET_GATHER; return; }
    if (!choose_target(fx, fy, &tx, &ty)) {
        if (!start_route(rx, ry, 0)) { memset(in_wave, 0, sizeof(in_wave)); fleet_state = FLEET_GATHER; }
        return;
    }
    if (verbose)
        fprintf(stderr, "T9 target %d,%d threat=%d selected=%d wraiths=%d protoss=%d tick=%u\n",
                tx, ty, air_threat_at(tx, ty, 16), total, n,
                living_force_buildings(T9_PROTOSS_SLOT), (unsigned)host_tick);
    if (!start_route(tx, ty, 1)) fleet_state = FLEET_GATHER;
}

/* ---- Diagnostics ------------------------------------------------------ */

static void inspect_state(void)
{
    fprintf(stderr, "T9 snapshot tick=%u frame=%u protoss=%d zerg=%d wallet=%d/%d supply=%d/%d\n",
            (unsigned)host_tick, (unsigned)frame, living_force_buildings(T9_PROTOSS_SLOT),
            living_force_buildings(T9_ZERG_SLOT), minerals, gas, supply_used, supply_cap);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive)
        fprintf(stderr, " unit slot=%d raw=%d owner=%d source=%d type=%d xy=%d,%d hp=%d order=%d target=%d to=%d,%d\n",
                i, u[i].raw_type, u[i].owner, u[i].raw_owner, u[i].type,
                u[i].x, u[i].y, u[i].hp, u[i].order, u[i].target, u[i].tx, u[i].ty);
    for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0)
        fprintf(stderr, " mineral slot=%d xy=%d,%d remaining=%d\n", i, m[i].x, m[i].y, m[i].amount);
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive)
        fprintf(stderr, " building slot=%d raw=%d owner=%d source=%d xy=%d,%d hp=%d complete=%d queue=%d\n",
                i, b[i].raw_type, b[i].owner, b[i].raw_owner, b[i].x, b[i].y,
                b[i].hp, building_complete(i), b[i].queue_count);
    for (int i = 0; i < hint_count; ++i)
        fprintf(stderr, " hint %d xy=%d,%d cleared=%d\n", i, hint[i].x, hint[i].y, hint[i].cleared);
}

/* Static map: terrain, 4-connected ground components and own features. */
static void dump_map(void)
{
    static char rowbuf[1024];
    static u16 component[MAP_CELLS];
    static u16 queue[MAP_CELLS];
    int labels = 0;
    memset(component, 0xff, sizeof(component));
    for (int seed = 0; seed < MAP_CELLS; ++seed) {
        if (component[seed] != 0xffff || terrain_blocked(seed % MAP_COLS, seed / MAP_COLS)) continue;
        int head = 0, tail = 0;
        component[seed] = (u16)labels; queue[tail++] = (u16)seed;
        while (head < tail) {
            static const int dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
            int cell = queue[head++], x = cell % MAP_COLS, y = cell / MAP_COLS;
            for (int k = 0; k < 4; ++k) {
                int nx = x + dx[k], ny = y + dy[k];
                if (nx < 0 || ny < 0 || nx >= MAP_COLS || ny >= MAP_ROWS) continue;
                int next = nx + ny * MAP_COLS;
                if (component[next] != 0xffff || terrain_blocked(nx, ny)) continue;
                component[next] = (u16)labels; queue[tail++] = (u16)next;
            }
        }
        fprintf(stderr, "component %d cells=%d\n", labels, tail);
        ++labels;
    }
    for (int row = 0; row < MAP_ROWS; ++row) {
        for (int col = 0; col < MAP_COLS; ++col) {
            int cell = col + row * MAP_COLS;
            rowbuf[col] = terrain_blocked(col, row) ? '#' :
                "abcdefghijklmnopqrstuvwxyz0123456789"[component[cell] % 36];
        }
        for (int i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0 && m[i].y / 16 == row)
            rowbuf[clamp_coord(m[i].x / 16, MAP_COLS)] = 'm';
        for (int i = 0; i < geyser_count; ++i) if (geysers[i].y / 16 == row)
            rowbuf[clamp_coord(geysers[i].x / 16, MAP_COLS)] = 'g';
        for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].y / 16 == row)
            rowbuf[clamp_coord(b[i].x / 16, MAP_COLS)] =
                b[i].owner == SIDE ? 'B' : b[i].raw_owner == T9_PROTOSS_SLOT ? 'P' :
                b[i].raw_owner == T9_ZERG_SLOT ? 'Z' : 'X';
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
            u[i].y / 16 == row)
            rowbuf[clamp_coord(u[i].x / 16, MAP_COLS)] = u[i].raw_type == T9_RAW_KERRIGAN ? 'K' : 'u';
        rowbuf[MAP_COLS] = 0;
        fprintf(stderr, "%3d %s\n", row, rowbuf);
    }
}

/* ---- Run ---------------------------------------------------------------- */

static int run_policy(void)
{
    u32 next_economy = host_tick, next_fleet = host_tick + 200;
    while (live()) {
        if (verbose && host_tick % 5000 == 0)
            fprintf(stderr, "T9 economy tick=%u scvs=%d queuedScv=%d gasWorkers=%d wallet=%d/%d home=%d "
                    "ports=%d/%d wraiths=%d queuedWraith=%d supply=%d/%d protoss=%d\n",
                    (unsigned)host_tick, side_count_raw(T9_RAW_SCV), queued_units(U_SCV), gas_workers(),
                    minerals, gas, home_minerals_left(), side_building_count(T9_RAW_STARPORT, 1),
                    side_building_count(T9_RAW_STARPORT, 0), wraith_count(), queued_units(U_WRAITH),
                    supply_used, supply_cap, living_force_buildings(T9_PROTOSS_SLOT));
        if (host_tick >= next_economy) {
            next_economy = host_tick + (manage_economy() ? 60 : 240);
        }
        if (host_tick >= next_fleet) {
            manage_fleet();
            next_fleet = host_tick + 60;
        }
        step(0);
    }
    return won && !lost && mission_unsupported_count == 0 &&
        race_campaign.phase == SC64_RACE_PHASE_COMPLETE &&
        (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) &&
        (race_completed & (1 << SC64_TERRAN_LATE_GLOBAL_T9)) &&
        living_force_buildings(T9_PROTOSS_SLOT) == 0 &&
        living_force_buildings(T9_ZERG_SLOT) > 0 &&
        source_unit(T9_RAW_KERRIGAN, -1) >= 0;
}

int main(int argc, char **argv)
{
    const char *trace_path = 0;
    int mode_flag = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--limit") && i + 1 < argc) limit_ticks = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc) trace_path = argv[++i];
        else if (!strcmp(argv[i], "--map")) mode_flag = 1;
        else if (!strcmp(argv[i], "--final-inspect")) mode_flag = 2;
        else if (!strcmp(argv[i], "--verbose")) verbose = 1;
        else if (!strcmp(argv[i], "--watch") && i + 1 < argc) watch_slot = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_t9_campaign_run [--limit TICKS] [--trace PATH] [--verbose] "
                 "[--map] [--final-inspect]");
            return 0;
        } else return 2;
    }
    if (limit_ticks < 1 || limit_ticks > 500000) return 2;
    game_init();
    assert(screen == SCREEN_PLAY && !race_mode &&
           race_selected_mission == SC64_TERRAN_LATE_GLOBAL_T9 && race_late_active());
    assert(!won && !lost && race_cols == 128 && race_rows == 96 && !mission_unsupported_count);
    load_hints();
    load_zerg_sites();
    mark_home_patches();
    if (mode_flag == 1) {
        const Sc64RaceMap *map = &sc64_race_maps[race_selected_mission];
        for (int i = 0; i < map->unit_count; ++i)
            if (map->units[i].source_type >= 106)
                fprintf(stderr, "placement %d owner=%d raw=%d xy=%d,%d\n", i, map->units[i].owner,
                        map->units[i].source_type, map->units[i].x_px, map->units[i].y_px);
        dump_map(); return 0;
    }
    if (trace_path) {
        trace = fopen(trace_path, "wb");
        if (!trace) return 2;
        fprintf(trace,
                "{\"trace_schema\":1,\"entry\":\"t9-probe-entry\","
                "\"profile\":\"terran-t9\",\"policy\":\"visible-information-wraith-fleet\","
                "\"mission\":12,\"tick_limit\":%d,\"events\":[\n", limit_ticks);
    }
    checkpoint("start");
    /* On the normal menu the briefing's START press runs mission tick 1, so
     * the controller leaves that tick neutral. */
    step(0);
    /* Both starting SCVs walk to the home line; the idle-worker rule then
     * picks home patches. Production follows from run_policy(). */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == T9_RAW_SCV)
            fprintf(stderr, "T9 opening worker slot=%d home=%d tick=%u\n",
                    i, send_worker_home(i), (unsigned)host_tick);
    checkpoint("opening");
    int result = run_policy();
    checkpoint(won ? "won" : lost ? "lost" : "limit");
    if (trace) { fputs("\n]}\n", trace); fclose(trace); }
    if (mode_flag == 2) inspect_state();
    fprintf(stderr,
        "T9 attempt tick=%u frame=%u outcome=%s ProtossBuildings=%d ZergBuildings=%d "
        "kerrigan=%d wraiths=%d scvs=%d faults=%d wallet=%d/%d supply=%d/%d result=%d\n",
        (unsigned)host_tick, (unsigned)frame, won ? "won" : lost ? "lost" : "limit",
        living_force_buildings(T9_PROTOSS_SLOT), living_force_buildings(T9_ZERG_SLOT),
        source_unit(T9_RAW_KERRIGAN, -1) >= 0, wraith_count(), side_count_raw(T9_RAW_SCV),
        mission_unsupported_count, minerals, gas, supply_used, supply_cap, result);
    return result ? 0 : 1;
}
