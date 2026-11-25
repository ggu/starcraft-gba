/* SPDX-License-Identifier: GPL-3.0-or-later
 * HOST author for whole Custom Game matches played with ordinary buttons.
 *
 * The player reads game state to decide what to do (it is a test policy,
 * not a visibility-only strategy), but every change to the game goes
 * through game_update() as held GBA keys: lobby rows, cursor travel, the
 * command list, A, R, SELECT and the pause menu. The held-key transitions
 * are written as a trace that tools/sc64_custom_full_match.py replays on
 * the race cartridge built from the same product vector.
 *
 * One HOST-only step is not a key: a Blood Bath Melee start is drawn from the
 * time since power-on, which the HOST has no clock for. With
 * SC64_CFM_START_ORDER the author sets the elapsed time (a test hook) to one
 * that resolves the order the cartridge drew from the same lobby keys; the
 * replay reads the ROM's own draw and refuses a trace whose start differs.
 *
 * During the first busy battle the player opens the pause menu and saves
 * (START, then B), then resumes with A. The trace records that update so
 * the replay can export the save and continue it in a fresh process.
 */
#include "../starcraft/game.c"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SC64_NATIVE_MELEE_STARTS
#error "the Custom Game author needs the cartridge lobby (SC64_NATIVE_MELEE_STARTS)"
#endif

enum { CHECK_INTERVAL = 250, BUSY_UNITS = 16, BUSY_HOSTILE = 5, BUSY_OWN = 6 };

static int option_map, option_human, option_bots, option_melee, option_save = 1;
static int option_bot_races[3];
static unsigned option_start_order, match_limit = 400000u;
static int option_lobby_wait, option_list_orders;
static u16 prior_keys;
static unsigned host_ticks;
static FILE *trace;
static int trace_first = 1, verbose;
static unsigned save_tick, save_frame, resume_tick;
static int actions_build, actions_train, actions_attack, actions_other;

static void trace_record(const char *format, ...)
{
    va_list args;
    if (!trace) return;
    if (!trace_first) fputs(",\n", trace);
    trace_first = 0;
    va_start(args, format);
    vfprintf(trace, format, args);
    va_end(args);
}

static void say(const char *format, ...)
{
    va_list args;
    if (!verbose) return;
    fprintf(stderr, "[%6u f%6d] ", host_ticks, frame);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
}

static int count_units(int owner, int type)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].owner == owner && (type < 0 || u[i].type == type);
    return count;
}

static int count_buildings(int owner, int type)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        count += b[i].alive && b[i].owner == owner && (type < 0 || b[i].type == type);
    return count;
}

/* The replay recomputes this from guest memory at every check. */
static unsigned world_hash(void)
{
    unsigned h = 2166136261u;
#define MIX(v) (h = (h ^ (unsigned)(v)) * 16777619u)
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive) continue;
        MIX(i); MIX(u[i].owner); MIX(u[i].type); MIX(u[i].x); MIX(u[i].y); MIX(u[i].hp);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive) continue;
        MIX(i); MIX(b[i].owner); MIX(b[i].type); MIX(b[i].x); MIX(b[i].y); MIX(b[i].hp);
    }
    MIX(minerals); MIX(gas); MIX(frame);
#undef MIX
    return h;
}

static void trace_check(const char *kind)
{
    static const int owners[4] = { 0, 1, 3, 4 };
    if (!trace) return;
    trace_record("{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%d,\"screen\":%d,\"minerals\":%d,\"gas\":%d,\"hash\":%u,\"owners\":[",
                 kind, host_ticks, frame, screen, minerals, gas, world_hash());
    for (int p = 0; p < 4; ++p)
        fprintf(trace, "%s[%d,%d]", p ? "," : "", count_units(owners[p], -1),
                count_buildings(owners[p], -1));
    fputs("]}", trace);
}

static int match_over(void)
{
    return screen == SCREEN_RESULT || won || lost || host_ticks >= match_limit;
}

static void input(u16 held)
{
    u16 pressed = (u16)(held & (u16)~prior_keys);
    if (match_over()) return;
    game_update(held, pressed);
    /* The cartridge draws between updates; the camera follows the cursor there. */
    game_render();
    ++host_ticks;
    if (trace && held != prior_keys)
        trace_record("{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u}", host_ticks, (unsigned)held);
    prior_keys = held;
    if (host_ticks % CHECK_INTERVAL == 0) trace_check("check");
}

static void tap(u16 key) { input(key); input(0); }
static void idle(int n) { while (n-- > 0 && !match_over()) input(0); }
static int playing(void) { return screen == SCREEN_PLAY && !match_over(); }

/* ---- Lobby ----------------------------------------------------------- */

static int launch_match(void)
{
    game_init();
    prior_keys = 0;
    tap(KEY_DOWN); /* HOME: Custom Game */
    tap(KEY_A);
    if (screen != SCREEN_TITLE || race_front.page != SC_FRONT_SKIRMISH) return 0;
    /* Every walk is bounded: a spent update budget stops input() advancing. */
    for (int n = 0; race_front.row > 0 && n < 16; ++n) tap(KEY_UP);
    for (int n = 0; race_front.map != option_map && n <= SC64_SKIRMISH_IMPORTED_COUNT; ++n)
        tap(KEY_RIGHT);
    if (race_front.row != 0 || race_front.map != option_map) return 0;
    tap(KEY_DOWN);
    for (int n = 0; race_front.race != option_human && n < 3; ++n) tap(KEY_RIGHT);
    if (race_front.race != option_human) return 0;
    tap(KEY_DOWN);
    for (int n = 0; race_front.bots != option_bots && n < 3; ++n) tap(KEY_RIGHT);
    if (race_front.bots != option_bots) return 0;
    for (int i = 0; i < option_bots; ++i) {
        tap(KEY_DOWN);
        for (int n = 0; race_front.bot_races[i] != option_bot_races[i] && n < 5; ++n)
            tap(KEY_RIGHT);
        if (race_front.bot_races[i] != option_bot_races[i]) return 0;
    }
    for (int n = 0; race_front.row < SC_FRONT_SKIRMISH_MODE_ROW && n < 16; ++n) tap(KEY_DOWN);
    if (race_front.row != SC_FRONT_SKIRMISH_MODE_ROW) return 0;
    if (!!race_front.skirmish_melee != option_melee) tap(KEY_RIGHT);
    if (!!race_front.skirmish_melee != option_melee) return 0;
    for (int n = 0; race_front.row < SC_FRONT_SKIRMISH_LAUNCH_ROW && n < 16; ++n) tap(KEY_DOWN);
    if (race_front.row != SC_FRONT_SKIRMISH_LAUNCH_ROW) return 0;
    /* A Melee start is drawn from the time since power-on, so waiting on
     * the Launch row before pressing A picks a different draw. */
    idle(option_lobby_wait);
    if (option_list_orders) {
        unsigned seen[256] = {0};
        for (unsigned ms = 0; ms < 20000; ++ms) {
            int human, races[3];
            unsigned order;
            if (sc64_front_skirmish_native_resolve(&race_front, ms, &human, races, &order) &&
                order < 256 && !seen[order]++) printf("%u\n", order);
        }
        exit(0);
    }
    if (option_start_order) {
        /* Use the time that gives the order the cartridge resolved from this tape. */
        unsigned ms;
        for (ms = 0; ms < 10000; ++ms) {
            int human, races[3];
            unsigned order;
            if (sc64_front_skirmish_native_resolve(&race_front, ms, &human, races, &order) &&
                order == option_start_order) break;
        }
        if (ms == 10000) return 0;
        gba_test_set_elapsed_milliseconds(ms);
    }
    tap(KEY_A);
    if (screen != SCREEN_PLAY || !race_mode || race_human != option_human ||
        race_bots != option_bots || race_skirmish_map != option_map ||
        !!race_skirmish_melee != option_melee) return 0;
    for (int i = 0; i < option_bots; ++i)
        if (race_bot_races[i] != option_bot_races[i]) return 0;
    if (option_start_order && race_skirmish_start_order != option_start_order) return 0;
    trace_record("{\"kind\":\"start\",\"host_tick\":%u,\"frame\":%d,\"map\":%d,\"human_race\":%d,"
                 "\"melee\":%d,\"start_order\":%u,\"bots\":[",
                 host_ticks, frame, race_skirmish_map, race_human, !!race_skirmish_melee,
                 (unsigned)race_skirmish_start_order);
    for (int i = 0; i < race_bots; ++i)
        fprintf(trace, "%s{\"owner\":%d,\"race\":%d}", i ? "," : "",
                race_bot_owner_ids[i], race_bot_races[i]);
    fputs("]}", trace);
    return 1;
}

/* ---- Buttons in the field -------------------------------------------- */

static void close_menus(void)
{
    if (menu_open) tap(KEY_B);
    if (mode != MODE_NORMAL) { tap(KEY_B); if (menu_open) tap(KEY_B); }
}

/* SELECT pauses when nothing is selected, so press it only to drop one. */
static void deselect(void)
{
    close_menus();
    if (selected_count() || selected_building >= 0) tap(KEY_SELECT);
}

static void move_cursor(int x, int y)
{
    unsigned guard = 0;
    x = limit(x, 0, WORLD_W - 1); y = limit(y, 0, WORLD_H - 1);
    while ((abs(cx - x) > 1 || abs(cy - y) > 1) && guard++ < 1200u && playing()) {
        int dx = x - cx, dy = y - cy;
        u16 held = (abs(dx) >= 4 && (abs(dy) >= 4 || abs(dy) <= 1)) ||
                   (abs(dy) >= 4 && abs(dx) <= 1) ? KEY_R : 0;
        if (dx > 1) held |= KEY_RIGHT;
        if (dx < -1) held |= KEY_LEFT;
        if (dy > 1) held |= KEY_DOWN;
        if (dy < -1) held |= KEY_UP;
        input(held);
    }
    input(0);
}

static int choose_menu_command(int command)
{
    close_menus();
    tap(KEY_B);
    for (int guard = 0; menu_open && menu_cursor != command && guard < 64; ++guard) {
#ifdef SC64_COMMAND_ICONS
        if (card_fits()) {
            int slot = card_command_slot(menu_cursor);
            int target_col = card_command_slot(command) % CARD_COLUMNS;
            int row_length = field_menu_count() - (slot / CARD_COLUMNS) * CARD_COLUMNS;
            if (row_length > CARD_COLUMNS) row_length = CARD_COLUMNS;
            if (slot % CARD_COLUMNS != target_col)
                tap(target_col >= row_length ? KEY_UP : KEY_RIGHT);
            else
                tap(KEY_DOWN);
        } else
#endif
            tap(KEY_DOWN);
    }
    if (!menu_open || menu_cursor != command) { close_menus(); return 0; }
    tap(KEY_A);
    return 1;
}

static int menu_row_for_building(int type)
{
    for (unsigned row = 0; row < sc64_skirmish_menu_count((unsigned)race_human); ++row) {
        Sc64SkirmishMenuRow item;
        if (sc64_skirmish_menu_at((unsigned)race_human, row, &item) && item.record &&
            item.kind == SC64_SK_MENU_BUILD &&
            item.record->raw_id == sc64_race_raw_building(type)) return (int)row;
    }
    return -1;
}

static int select_building(int slot)
{
    static const signed char offsets[][2] = {
        {0,0}, {-10,-10}, {10,-10}, {-10,10}, {10,10}, {0,-16}, {0,16}, {-16,0}, {16,0},
        {-18,-14}, {18,-14}, {-18,14}, {18,14}
    };
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive || b[slot].owner != SIDE) return 0;
    close_menus();
    if (selected_building == slot && !selected_count()) {
        if (!field_ui_on_selected_building()) move_cursor(b[slot].x, b[slot].y);
        return selected_building == slot && field_ui_on_selected_building();
    }
    deselect();
    for (unsigned k = 0; k < sizeof offsets / sizeof offsets[0] && playing(); ++k) {
        move_cursor(b[slot].x + offsets[k][0], b[slot].y + offsets[k][1]);
        if (selection_target() != MAX_UNITS + slot) continue;
        tap(KEY_A);
        if (selected_building == slot) return 1;
        deselect();
    }
    return 0;
}

static int select_unit(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE) return 0;
    deselect();
    for (int attempt = 0; attempt < 4 && u[slot].alive && playing(); ++attempt) {
        unsigned guard = 0;
        while (u[slot].alive && selection_target() != slot && guard++ < 900u && playing()) {
            int dx = u[slot].x - cx, dy = u[slot].y - cy;
            if (abs(dx) <= 1 && abs(dy) <= 1) break;
            u16 held = (abs(dx) >= 4 && (abs(dy) >= 4 || abs(dy) <= 1)) ||
                       (abs(dy) >= 4 && abs(dx) <= 1) ? KEY_R : 0;
            if (dx > 1) held |= KEY_RIGHT;
            if (dx < -1) held |= KEY_LEFT;
            if (dy > 1) held |= KEY_DOWN;
            if (dy < -1) held |= KEY_UP;
            input(held);
        }
        input(0);
        if (selection_target() != slot) continue;
        tap(KEY_A);
        if (u[slot].selected && selected_count() == 1) return 1;
        deselect();
    }
    return 0;
}

#include "sc64_visible_mineral_cursor.h"

/* ---- Observations ---------------------------------------------------- */

static RaceAiCensus me;
static int home_x, home_y;

static int raw_unit(int raw) { return sc64_race_unit_from_raw(raw); }
static int raw_building(int raw) { return sc64_race_building_from_raw(raw); }

static int complete_buildings(int type)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        count += b[i].alive && b[i].owner == SIDE && b[i].type == type && building_complete(i);
    return count;
}

/* Complete, under construction, or carried by a walking builder. */
static int planned_buildings(int type)
{
    int count = count_buildings(SIDE, type);
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].owner == SIDE && u[i].order == ORDER_BUILD_PENDING &&
                 u[i].target == type;
    return count;
}

static int building_price(int type, int *gas_price)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
    if (gas_price) *gas_price = r ? r->gas : 0;
    return r ? r->minerals : 0;
}

static int army_unit(const Unit *q)
{
    return q->alive && q->owner == SIDE && !SC64_TRANSPORT_LOADED(q) &&
           !sc64_race_is_worker(q->type) && q->type != U_OVERLORD && race_ai_armed(q->type);
}

static int army_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) count += army_unit(&u[i]);
    return count;
}

static void army_center(int *x, int *y)
{
    long sx = 0, sy = 0; int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (army_unit(&u[i])) { sx += u[i].x; sy += u[i].y; ++n; }
    *x = n ? (int)(sx / n) : home_x;
    *y = n ? (int)(sy / n) : home_y;
}

static int hostile_units_near(int x, int y, int radius, int armed_only)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && owners_hostile(SIDE, u[i].owner) &&
                 !SC64_TRANSPORT_LOADED(&u[i]) &&
                 (!armed_only || (race_ai_armed(u[i].type) && !sc64_race_is_worker(u[i].type))) &&
                 dist2(x, y, u[i].x, u[i].y) < radius * radius;
    return count;
}

static int own_army_near(int x, int y, int radius)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += army_unit(&u[i]) && dist2(x, y, u[i].x, u[i].y) < radius * radius;
    return count;
}

/* ---- Sites ----------------------------------------------------------- */

static int site_ok(int type, int x, int y, int spacing)
{
    if (x < 24 || y < 24 || x >= WORLD_W - 24 || y >= WORLD_H - 24) return 0;
    if (!race_ai_site_lane_clear(&me, x, y) || !race_ai_exit_clear(type, x, y)) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && dist2(x, y, b[i].x, b[i].y) < spacing * spacing) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].order == ORDER_BUILD_PENDING &&
            dist2(x, y, u[i].tx, u[i].ty) < spacing * spacing) return 0;
    return race_can_site(SIDE, type, x, y) && can_place(type, x, y);
}

/* Lattice rings around (ax,ay), nearest ring first. toward picks the side
 * facing the map center (producers, defenses) or away from it (supply). */
static int find_site(int type, int ax, int ay, int toward, int min_radius, int max_radius,
                     int spacing, int *out_x, int *out_y)
{
    int mx = WORLD_W / 2 - ax, my = WORLD_H / 2 - ay, front = 0, best_dot = -0x7fffffff;
    for (int k = 0; k < 16; ++k) {
        int dot = race_ai_dir[k][0] * mx + race_ai_dir[k][1] * my;
        if (dot > best_dot) { best_dot = dot; front = k; }
    }
    if (!toward) front = (front + 8) & 15;
    for (int radius = min_radius; radius <= max_radius; radius += 32)
        for (int step = 0; step < 16; ++step) {
            int k = (front + ((step & 1) ? (step + 1) / 2 : -(step / 2)) + 16) & 15;
            int x = ax + race_ai_dir[k][0] * radius / 64;
            int y = ay + race_ai_dir[k][1] * radius / 64;
            field_ui_site(type, &x, &y);
            if (!site_ok(type, x, y, spacing)) continue;
            *out_x = x; *out_y = y;
            return 1;
        }
    return 0;
}

/* ---- Actions --------------------------------------------------------- */

/* Place type at a snapped site through the command list. The nearest free
 * worker to the cursor takes the job; the price is paid on arrival. */
static int build_at(int type, int x, int y)
{
    int row = menu_row_for_building(type), before = planned_buildings(type);
    if (row < 0) return 0;
    deselect();
    if (!playing() || !choose_menu_command(row) || mode != MODE_BUILD || menu != type) {
        close_menus();
        return 0;
    }
    move_cursor(x, y);
    if (!playing() || mode != MODE_BUILD) { close_menus(); return 0; }
    tap(KEY_A);
    if (mode != MODE_NORMAL) { close_menus(); return 0; }
    if (planned_buildings(type) <= before) return 0;
    ++actions_build;
    trace_check("build");
    say("build type%d at %d,%d bank %d/%d", type, x, y, minerals, gas);
    return 1;
}

static int build_near(int type, int ax, int ay, int toward, int min_radius, int max_radius)
{
    int x, y, found = 0;
    for (int spacing = 52; spacing >= 44 && !found; spacing -= 8)
        found = find_site(type, ax, ay, toward, min_radius, max_radius, spacing, &x, &y) ||
                find_site(type, ax, ay, !toward, min_radius, max_radius, spacing, &x, &y);
    if (!found) {
        /* A base in a map corner runs out of ring sites: scan a grid, nearest first. */
        int best_d = 0x7fffffff;
        for (int gy = ay - 448; gy <= ay + 448; gy += 16)
            for (int gx = ax - 448; gx <= ax + 448; gx += 16) {
                int sx = gx, sy = gy, d;
                field_ui_site(type, &sx, &sy);
                d = dist2(ax, ay, sx, sy);
                if (d >= best_d || d < min_radius * min_radius || !site_ok(type, sx, sy, 44)) continue;
                best_d = d; x = sx; y = sy; found = 1;
            }
    }
    if (!found) {
        say("no site for type%d", type);
        return 0;
    }
    return build_at(type, x, y);
}

static int build_gas(int type)
{
    int best = -1, best_d = 288 * 288;
    for (int g = 0; g < geyser_count; ++g) {
        int d = dist2(geysers[g].x, geysers[g].y, home_x, home_y);
        if (d < best_d && race_can_site(SIDE, type, geysers[g].x, geysers[g].y) &&
            can_place(type, geysers[g].x, geysers[g].y)) { best_d = d; best = g; }
    }
    return best >= 0 && build_at(type, geysers[best].x, geysers[best].y);
}

/* Queue up to count units of type at a producer; R cycles its products. */
static int train(int producer, int type, int count)
{
    int queued = 0;
    if (!select_building(producer)) return 0;
    for (int step = 0; train_type != type && step < 12 && playing(); ++step) tap(KEY_R);
    if (train_type != type || selected_building != producer) return 0;
    while (queued < count && playing() && field_ui_on_selected_building()) {
        int before = b[producer].queue_count, larva = b[producer].larva;
        tap(KEY_A);
        if (b[producer].queue_count <= before && b[producer].larva >= larva) break;
        ++queued;
    }
    if (queued) { ++actions_train; trace_check("train"); }
    return queued;
}

static int research(int armor)
{
    int *progress = armor ? &armor_research : &weapons_research;
    deselect();
    if (!choose_menu_command(armor ? MENU_ARMOR : MENU_WEAPONS)) return 0;
    close_menus();
    if (!*progress) return 0;
    ++actions_other;
    trace_check("research");
    say("research %s", armor ? "armor" : "weapons");
    return 1;
}

/* R selects every combat unit; the command list arms attack-move. With
 * focus, A lands on the hostile actor under the cursor and the whole
 * selection attacks it; otherwise the cursor steps off to keep attack-move. */
static int attack_move(int x, int y, int focus)
{
    int issued = 0;
    deselect();
    if (!playing()) return 0;
    tap(KEY_R);
    if (selected_count() <= 0 || selected_building >= 0) return 0;
    if (!choose_menu_command(MENU_ATTACK) || mode != MODE_ATTACK) { close_menus(); return 0; }
    move_cursor(x, y);
#ifdef SC64_DIRECT_ATTACK
    for (int k = 0; !focus && k < 8 && playing() && sc64_direct_attack_find_target(cx, cy) >= 0; ++k)
        move_cursor(cx + (home_x < cx ? -12 : 12), cy + (home_y < cy ? -12 : 12));
#else
    (void)focus;
#endif
    if (!playing() || mode != MODE_ATTACK) { close_menus(); return 0; }
    tap(KEY_A);
    for (int i = 0; i < MAX_UNITS; ++i)
        issued += u[i].alive && u[i].owner == SIDE && u[i].selected &&
                  (u[i].order == ORDER_ATTACK_MOVE || u[i].order == ORDER_ATTACK_TARGET);
    if (issued) { ++actions_attack; trace_check("attack"); }
    deselect();
    return issued;
}

static int assign_gas(int refinery)
{
    int worker = -1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || !sc64_race_is_worker(u[i].type) ||
            u[i].order != ORDER_MINE || u[i].carry) continue;
        if (worker < 0 || dist2(u[i].x, u[i].y, b[refinery].x, b[refinery].y) <
                          dist2(u[worker].x, u[worker].y, b[refinery].x, b[refinery].y)) worker = i;
    }
    if (worker < 0 || !select_unit(worker)) return 0;
    move_cursor(b[refinery].x, b[refinery].y);
    if (playing()) tap(KEY_A);
    deselect();
    if (u[worker].order != ORDER_GAS) return 0;
    ++actions_other;
    return 1;
}

static int return_idle_worker(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || !sc64_race_is_worker(u[i].type) ||
            u[i].order != ORDER_IDLE || SC64_TRANSPORT_LOADED(&u[i])) continue;
        if (!select_unit(i)) continue;
        (void)order_selected_worker_to_visible_minerals(i, move_cursor, tap);
        deselect();
        ++actions_other;
        return 1;
    }
    return 0;
}

/* Leave the remembered command row and the selection where a fresh load
 * leaves them, so the same keys follow the pause in both continuations. */
static void neutral_ui(void)
{
    deselect();
    if (choose_menu_command(MENU_MOVE)) close_menus();
    deselect();
    idle(2);
}

static void viewport_census(int *own, int *hostile)
{
    int left = viewport_left(), top = viewport_top();
    *own = *hostile = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i]) || u[i].x < left || u[i].x >= left + 240 ||
            u[i].y < top || u[i].y >= top + 108) continue;
        if (u[i].owner == SIDE) ++*own;
        else if (owners_hostile(SIDE, u[i].owner)) ++*hostile;
    }
}

static void pause_save(int visible_own, int visible_hostile)
{
    tap(KEY_START);
    if (screen != SCREEN_PAUSE) return;
    input(KEY_B);
    save_tick = host_ticks; save_frame = (unsigned)frame;
    trace_record("{\"kind\":\"save\",\"host_tick\":%u,\"frame\":%d,\"hash\":%u,"
                 "\"visible_own\":%d,\"visible_hostile\":%d,\"cursor\":[%d,%d]}",
                 host_ticks, frame, world_hash(), visible_own, visible_hostile, cx, cy);
    input(0);
    tap(KEY_A);
    idle(4);
    resume_tick = host_ticks;
    trace_record("{\"kind\":\"resume\",\"host_tick\":%u,\"frame\":%d,\"hash\":%u}",
                 host_ticks, frame, world_hash());
    say("pause save at tick %u frame %u, %d own and %d hostile units on screen",
        save_tick, save_frame, visible_own, visible_hostile);
}

/* Bring the cursor into the fight and save once many units share the screen:
 * at least BUSY_HOSTILE hostile units and BUSY_UNITS units in the viewport. */
static unsigned next_busy_look;
static int try_busy_save(void)
{
    int best = -1, best_score = 0, own, hostile;
    /* The pause sequence must finish inside the update budget. */
    if (!option_save || save_tick || host_ticks < next_busy_look || host_ticks + 4000 > match_limit) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!army_unit(&u[i])) continue;
        int near_hostile = 0, near_own = 0;
        for (int j = 0; j < MAX_UNITS; ++j) {
            if (!u[j].alive || SC64_TRANSPORT_LOADED(&u[j]) || abs(u[j].x - u[i].x) > 100 ||
                abs(u[j].y - u[i].y) > 44) continue;
            if (u[j].owner == SIDE) ++near_own;
            else if (owners_hostile(SIDE, u[j].owner)) ++near_hostile;
        }
        if (near_hostile >= BUSY_HOSTILE && near_own + near_hostile >= BUSY_UNITS &&
            near_own + near_hostile > best_score) {
            best = i; best_score = near_own + near_hostile;
        }
    }
    if (best < 0) return 0;
    next_busy_look = host_ticks + 45;
    neutral_ui();
    if (!playing() || menu_open || mode != MODE_NORMAL || !u[best].alive) return 1;
    move_cursor(u[best].x, u[best].y);
    idle(2);
    viewport_census(&own, &hostile);
    if (hostile < BUSY_HOSTILE || own < BUSY_OWN || own + hostile < BUSY_UNITS) return 1;
    pause_save(own, hostile);
    return 1;
}

/* ---- Policy ---------------------------------------------------------- */

typedef struct {
    int worker, hall, supply, supply_unit, gas, pre, producer, tech, lab, line, basic, support, support_ratio;
    int defense, defenses, defense_frame, tech_frame;
    int workers, producers, attack_min, attack_margin, attack_ratio, retreat_army, first_attack;
    int tech_after, lab_after, tech_deadline, max_producers, recall_threat, all_in_frame, defense_weight, strength_px, early_workers, grow_frame;
} Plan;

static Plan plan;
static unsigned next_defend, next_attack, next_rally, next_idle, next_gas, next_research;
static int attacking, attack_owner = -1, last_target = -1;

static int plan_option(const char *name, int fallback)
{
    char key[64];
    const char *value;
    snprintf(key, sizeof key, "SC64_CFM_%s", name);
    value = getenv(key);
    return value ? atoi(value) : fallback;
}

static void setup_plan(void)
{
    memset(&plan, 0, sizeof plan);
    plan.support = plan.pre = plan.supply = plan.supply_unit = -1;
    if (race_human == R_TER) {
        plan.worker = raw_unit(7); plan.hall = raw_building(106); plan.supply = raw_building(109);
        plan.gas = raw_building(110); plan.producer = raw_building(111);
        plan.tech = raw_building(112); plan.lab = raw_building(122);
        plan.line = plan.basic = raw_unit(0);
        plan.support = raw_unit(34); plan.support_ratio = plan_option("SUPPORT_RATIO", 8);
    } else if (race_human == R_ZERG) {
        /* Larva at a Hatchery train everything, Overlords included; a Drone
         * becomes each structure, and the Spawning Pool comes first. */
        plan.worker = raw_unit(41); plan.hall = raw_building(131); plan.supply_unit = raw_unit(42);
        plan.gas = raw_building(149); plan.pre = raw_building(142); plan.producer = raw_building(131);
        plan.tech = raw_building(135); plan.lab = raw_building(139);
        plan.line = raw_unit(38); plan.basic = raw_unit(37);
    } else {
        /* Gateways need Pylon power; Zealots until the Cybernetics Core, then Dragoons. */
        plan.worker = raw_unit(64); plan.hall = raw_building(154); plan.supply = raw_building(156);
        plan.gas = raw_building(157); plan.producer = raw_building(160);
        plan.tech = raw_building(164); plan.lab = raw_building(166);
        plan.line = raw_unit(66); plan.basic = raw_unit(65);
    }
    int melee = race_skirmish_map == 1;
    plan.defense = -1;
    if (plan_option("LINE", -1) >= 0) plan.line = raw_unit(plan_option("LINE", -1));
    /* The army is trained only at the plan's producer (Hatchery larva for
     * Zerg), from its Custom Game product list, with prerequisites the plan
     * builds. */
    {
        unsigned producer_raw = race_human == R_ZERG ? 35u : (unsigned)sc64_race_raw_building(plan.producer);
        unsigned built[5] = { (unsigned)sc64_race_raw_building(plan.producer),
                              plan.pre >= 0 ? (unsigned)sc64_race_raw_building(plan.pre) : 255u,
                              (unsigned)sc64_race_raw_building(plan.tech),
                              (unsigned)sc64_race_raw_building(plan.lab),
                              (unsigned)sc64_race_raw_building(plan.hall) };
        const Sc64SkirmishRecord *product = 0;
        for (unsigned i = 0; i < sc64_skirmish_product_count(producer_raw); ++i)
            if (sc64_skirmish_product_at(producer_raw, i)->raw_id == (unsigned)sc64_race_raw_unit(plan.line))
                product = sc64_skirmish_product_at(producer_raw, i);
        for (int k = 0; product && k < 2; ++k) {
            unsigned need = k ? product->prereq2_raw : product->prereq1_raw, have = need == 255u;
            for (int j = 0; j < 5; ++j) have |= need == built[j];
            if (!have) product = 0;
        }
        if (plan.line < 0 || !product || sc64_race_is_worker(plan.line)) {
            fprintf(stderr, "SC64_CFM_LINE is not a unit this plan's producer trains\n");
            exit(2);
        }
    }
    if (plan_option("DEFENSE", -1) >= 0) plan.defense = raw_building(plan_option("DEFENSE", -1));
    plan.defenses = plan_option("DEFENSES", 0);
    plan.defense_frame = plan_option("DEFENSE_FRAME", 5000);
    plan.tech_frame = plan_option("TECH_FRAME", 0);
    plan.workers = plan_option("WORKERS", melee ? 20 : 12);
    plan.producers = plan_option("PRODUCERS", race_human == R_ZERG ? 1 : melee ? 6 : 4);
    plan.tech_after = plan_option("TECH_AFTER", race_human == R_ZERG ? 1 : melee ? 4 : 3);
    plan.lab_after = plan_option("LAB_AFTER", race_human == R_ZERG ? 2 : melee ? 5 : 4);
    plan.tech_deadline = plan_option("TECH_DEADLINE", 7000);
    plan.max_producers = plan_option("MAX_PRODUCERS", race_human == R_ZERG ? 5 : melee ? 10 : 4);
    plan.strength_px = plan_option("STRENGTH_PX", melee ? 40 : 1000);
    plan.early_workers = plan_option("EARLY_WORKERS", melee ? 12 : plan.workers);
    plan.grow_frame = plan_option("GROW_FRAME", melee ? 6000 : 0);
    plan.recall_threat = plan_option("RECALL_THREAT", 5);
    plan.all_in_frame = plan_option("ALL_IN_FRAME", 46000);
    plan.defense_weight = plan_option("DEFENSE_WEIGHT", 4);
    plan.attack_min = plan_option("ATTACK_MIN", melee ? 10 : 12);
    plan.attack_margin = plan_option("ATTACK_MARGIN", melee ? 4 : 6);
    plan.attack_ratio = plan_option("ATTACK_RATIO", melee ? 2 : 3);
    plan.retreat_army = plan_option("RETREAT", 6);
    plan.first_attack = plan_option("FIRST_ATTACK", melee ? 4500 : 6000);
}

static int hostile_army(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].owner == owner && !SC64_TRANSPORT_LOADED(&u[i]) &&
                 race_ai_armed(u[i].type) && !sc64_race_is_worker(u[i].type);
    return count;
}

static int hostile_defenses(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int raw;
        if (!b[i].alive || b[i].owner != owner) continue;
        raw = sc64_race_raw_building(b[i].type);
        count += raw == 125 || raw == 146 || raw == 162;
    }
    return count;
}

static int all_hostile_army(void)
{
    int count = 0;
    for (int k = 0; k < race_bots; ++k) count += hostile_army(race_bot_owner_ids[k]);
    return count;
}

/* The opponent to attack next: weak first, and near before far. */
static int weakest_owner(int *strength)
{
    int best = -1, best_score = 0x7fffffff, best_strength = 0;
    for (int k = 0; k < race_bots; ++k) {
        int owner = race_bot_owner_ids[k], value, score, d = 0x7fffffff;
        if (!count_buildings(owner, -1)) continue;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == owner && dist2(home_x, home_y, b[i].x, b[i].y) < d)
                d = dist2(home_x, home_y, b[i].x, b[i].y);
        value = hostile_army(owner) + plan.defense_weight * hostile_defenses(owner);
        score = value * plan.strength_px;
        for (int r = 0; r * r < d; r += 16) score += 16; /* plus the distance in pixels */
        if (score < best_score) { best_score = score; best_strength = value; best = owner; }
    }
    if (strength) *strength = best_strength;
    return best;
}

static int nearest_hostile_building(int x, int y, int owner)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive || !owners_hostile(SIDE, b[i].owner)) continue;
        if (owner >= 0 && b[i].owner != owner) continue;
        int d = dist2(x, y, b[i].x, b[i].y);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

static int first_hall(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && race_ai_is_depot(b[i].type) &&
            building_complete(i)) return i;
    return -1;
}

static int wanted_minerals; /* held back from army production for the next step */
static int free_bank(void) { return minerals - me.reserve_minerals; }
static int army_bank(void) { return free_bank() - wanted_minerals; }

static int queued_units(int type)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive || b[i].owner != SIDE) continue;
        for (int j = 0; j < b[i].queue_count; ++j)
            count += b[i].queue[(b[i].queue_head + j) % 5] == type;
    }
    return count;
}

/* Workers from the town hall, two at a time (one larva each for Zerg). */
static int policy_workers(void)
{
    int hall = first_hall(), price, pending;
    if (hall < 0) return 0;
    price = sc64_race_unit_rule(plan.worker)->minerals;
    pending = queued_units(plan.worker);
    if (me.workers + pending >= (frame < plan.grow_frame ? plan.early_workers : plan.workers) ||
        pending >= 2 || free_bank() < price || me.free_supply < 2) return 0;
    if (race_human == R_ZERG) {
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == SIDE && sc64_race_is_zerg_townhall(b[i].type) &&
                building_complete(i) && b[i].larva > 0) return train(i, plan.worker, 1) > 0;
        return 0;
    }
    if (b[hall].queue_count >= 2) return 0;
    return train(hall, plan.worker, 2) > 0;
}

static int policy_supply(void)
{
    int margin = 6 + 6 * complete_buildings(plan.producer), price;
    if (margin > 44) margin = 44;
    if (me.supply_cap >= 400 || me.pending_supply >= 2) return 0;
    if (me.free_supply + 16 * me.pending_supply >= margin) return 0;
    if (plan.supply_unit >= 0) {
        price = sc64_race_unit_rule(plan.supply_unit)->minerals;
        if (free_bank() < price) { wanted_minerals = price; return 0; }
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == SIDE && sc64_race_is_zerg_townhall(b[i].type) &&
                building_complete(i) && b[i].larva > 0) return train(i, plan.supply_unit, 1) > 0;
        return 0;
    }
    price = building_price(plan.supply, 0);
    if (free_bank() < price) { wanted_minerals = price; return 0; }
    /* The first Pylon goes toward the map center, where the Gateways follow. */
    return build_near(plan.supply, home_x, home_y, race_human == R_PROTOSS && !planned_buildings(plan.supply),
                      64, 448);
}

/* One step of the opening at a time. Supply and the first producer hold
 * their price back from army production; later steps are bought from what
 * the busy producers leave over. */
static int want_structure(int type, int toward, int reserve)
{
    int gas_price, price = building_price(type, &gas_price);
    if (gas < gas_price) return 0;
    if (free_bank() < price) { if (reserve) wanted_minerals = price; return 0; }
    return type == plan.gas ? build_gas(type) : build_near(type, home_x, home_y, toward, 64, 448);
}

static int policy_structures(void)
{
    int producers = planned_buildings(plan.producer), want = plan.producers;
    if (race_human == R_PROTOSS && !complete_buildings(plan.supply)) return 0;
    if (plan.pre >= 0 && !planned_buildings(plan.pre)) return want_structure(plan.pre, 1, 1);
    if (plan.pre >= 0) --producers; /* the starting Hatchery is not an added producer */
    if (producers < 1) return want_structure(plan.producer, 1, plan.pre < 0);
    if (free_bank() > 400) want += (free_bank() - 250) / 150;
    if (want > plan.max_producers) want = plan.max_producers;
    if (producers < 2 && producers < want) return want_structure(plan.producer, 1, 0);
    /* An all-in opening spends on producers only until tech_frame. */
    if (frame < plan.tech_frame) return producers < want ? want_structure(plan.producer, 1, 0) : 0;
    if (!planned_buildings(plan.gas)) return want_structure(plan.gas, 0, 0);
    if (producers < plan.tech_after && producers < want) return want_structure(plan.producer, 1, 0);
    if (complete_buildings(plan.producer) && !planned_buildings(plan.tech))
        return want_structure(plan.tech, 0, frame > plan.tech_deadline);
    if (producers < plan.lab_after && producers < want) return want_structure(plan.producer, 1, 0);
    if (complete_buildings(plan.tech) && !planned_buildings(plan.lab))
        return want_structure(plan.lab, 0, frame > plan.tech_deadline);
    if (plan.defense >= 0 && frame >= plan.defense_frame &&
        planned_buildings(plan.defense) + (race_human == R_ZERG ? count_buildings(SIDE, raw_building(146)) : 0) <
            plan.defenses &&
        complete_buildings(race_human == R_ZERG ? plan.pre : plan.lab)) {
        /* Shooting structures face the map center; a Protoss one needs power there. */
        int gas_price, price = building_price(plan.defense, &gas_price), x, y;
        if (free_bank() < price) { wanted_minerals = price; return 0; }
        if (race_human == R_ZERG) return build_near(plan.defense, home_x, home_y, 1, 64, 320);
        if (find_site(plan.defense, home_x, home_y, 1, 96, 224, 44, &x, &y)) return build_at(plan.defense, x, y);
        if (race_human == R_PROTOSS && me.pend[AK_PYLON] == 0 &&
            find_site(plan.supply, home_x, home_y, 1, 96, 224, 44, &x, &y)) return build_at(plan.supply, x, y);
        return 0;
    }
    if (producers < want) return want_structure(plan.producer, 1, 0);
    return 0;
}

/* A finished Creep Colony becomes a Sunken Colony from its own command list. */
static int policy_sunken(void)
{
    int sunken = raw_building(146), row = -1;
    if (race_human != R_ZERG || plan.defense < 0 || plan.defense != raw_building(143) || free_bank() < 50)
        return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive || b[i].owner != SIDE || b[i].type != plan.defense || !building_complete(i) ||
            race_construction_deferred_morph(&b[i]) >= 0) continue;
        if (!select_building(i)) continue;
        for (unsigned r = 0; r < sc64_skirmish_menu_count((unsigned)race_human); ++r) {
            Sc64SkirmishMenuRow item;
            if (sc64_skirmish_menu_at((unsigned)race_human, r, &item) && item.record &&
                item.kind == SC64_SK_MENU_MORPH && item.record->raw_id == 146) row = (int)r;
        }
        if (row < 0 || !choose_menu_command(row)) { deselect(); return 0; }
        close_menus();
        deselect();
        if (b[i].type != sunken && race_construction_deferred_morph(&b[i]) < 0) return 0;
        ++actions_build;
        trace_check("build");
        say("sunken morph at %d,%d", b[i].x, b[i].y);
        return 1;
    }
    return 0;
}

static int policy_gas(void)
{
    if (host_ticks < next_gas) return 0;
    for (int k = 0; k < me.refineries; ++k) {
        int refinery = me.refinery[k];
        if (!building_complete(refinery) || me.refinery_load[k] >= 3 || me.workers < 9) continue;
        next_gas = host_ticks + 90;
        return assign_gas(refinery);
    }
    return 0;
}

static int policy_research(void)
{
    int need = (weapons_level < 2 && !weapons_research) || (armor_level < 2 && !armor_research);
    if (!complete_buildings(plan.lab) || !need || gas < 100) return 0;
    if (free_bank() < 100) {
        if (frame > plan.tech_deadline && wanted_minerals < 100) wanted_minerals = 100;
        return 0;
    }
    if (host_ticks < next_research) return 0;
    if (weapons_level < 2 && !weapons_research && research(0)) return 1;
    if (armor_level < 2 && !armor_research && research(1)) return 1;
    next_research = host_ticks + 600; /* refused: this product sells no such upgrade */
    return 0;
}

static int producer_building(int i)
{
    if (!b[i].alive || b[i].owner != SIDE || !building_complete(i)) return 0;
    return race_human == R_ZERG ? sc64_race_is_zerg_townhall(b[i].type) && b[i].larva > 0
                                : b[i].type == plan.producer;
}

/* depth 1 keeps every producer busy; later passes deepen the queues. */
static int policy_army(int depth)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int type = plan.line, want, price;
        const Sc64RaceUnitRule *rule;
        if (!producer_building(i)) continue;
        if (race_human != R_ZERG && b[i].queue_count >= depth) continue;
        if (race_human == R_ZERG && depth > 1 && b[i].larva < 2) continue;
        if (plan.support >= 0 && complete_buildings(plan.tech) &&
            (count_units(SIDE, plan.support) + queued_units(plan.support)) * plan.support_ratio <
            count_units(SIDE, plan.line) && gas >= sc64_race_unit_rule(plan.support)->gas)
            type = plan.support;
        if (type == plan.line && plan.line != plan.basic &&
            (!complete_buildings(plan.tech) || gas < sc64_race_unit_rule(plan.line)->gas)) {
            if (plan.pre >= 0 && !complete_buildings(plan.pre)) continue;
            type = plan.basic;
        }
        rule = sc64_race_unit_rule(type);
        price = rule->minerals;
        if (army_bank() < price || me.free_supply < rule->supply * sc64_race_product_count(type)) continue;
        want = army_bank() / price;
        if (race_human == R_ZERG) { if (want > b[i].larva) want = b[i].larva; }
        else if (want > depth + 1 - b[i].queue_count) want = depth + 1 - b[i].queue_count;
        if (type != plan.line && type != plan.basic && want > 1) want = 1;
        if (want > 3) want = 3;
        if (train(i, type, want) > 0) return 1;
    }
    return 0;
}

static int policy_defend(void)
{
    if (me.threat < 0 || host_ticks < next_defend || army_count() < 1) return 0;
    /* A scout does not recall an assault; a raid on the base does. */
    if (attacking && me.threat_count < plan.recall_threat) return 0;
    if (own_army_near(me.threat_x, me.threat_y, 110) * 2 >= army_count()) {
        next_defend = host_ticks + 60;
        return 0;
    }
    next_defend = host_ticks + 150;
    if (attacking) { attacking = 0; say("recall against %d", me.threat_count); }
    say("defend at %d,%d against %d", me.threat_x, me.threat_y, me.threat_count);
    return attack_move(me.threat_x, me.threat_y, 0) > 0;
}

/* Nothing more can be bought: the bank is spent and no producer is working. */
static int spent(void)
{
    if (minerals >= 100 || me.queued_units) return 0;
    for (int k = 0; k < me.lane_patches; ++k)
        if (m[me.lane_patch[k]].amount > 0) return 0;
    return 1;
}

static int assault_stage, stage_x, stage_y;
static unsigned stage_deadline;

static int policy_attack(void)
{
    int army = army_count(), x, y, target, strength = 0, owner = weakest_owner(&strength);
    int all_in = spent() || frame > plan.all_in_frame;
    if (attacking && !count_buildings(attack_owner, -1)) {
        say("owner %d cleared with %d", attack_owner, army);
        attacking = 0;
        next_rally = 0;
    }
    if (attacking && army < plan.retreat_army && !all_in && hostile_army(attack_owner) > army) {
        say("retreat with %d", army);
        attacking = 0;
        next_rally = 0;
    }
    if (!attacking && owner >= 0 && army > 0 && me.threat_count < plan.recall_threat &&
        ((all_in && !all_hostile_army()) ||
         (!strength && frame >= plan.first_attack && army >= 4) ||
         (frame >= plan.first_attack && army >= plan.attack_min &&
          army >= plan.attack_ratio * strength + plan.attack_margin &&
          (race_skirmish_melee || army >= 2 * all_hostile_army())))) {
        attacking = 1; attack_owner = owner; next_attack = 0; last_target = -1;
        say("attack owner %d strength %d with %d%s", owner, strength, army, all_in ? " (all in)" : "");
    }
    if (!attacking) {
        int gx = home_x + (WORLD_W / 2 - home_x) / 5, gy = home_y + (WORLD_H / 2 - home_y) / 5;
        if (host_ticks < next_rally || army < 2) return 0;
        next_rally = host_ticks + 500;
        if (own_army_near(gx, gy, 120) == army) return 0;
        return attack_move(gx, gy, 0) > 0;
    }
    army_center(&x, &y);
    target = nearest_hostile_building(x, y, attack_owner);
    if (target < 0) return 0;
    if (target != last_target && dist2(x, y, b[target].x, b[target].y) > 320 * 320) {
        /* Gather short of a distant target so the army arrives together. */
        int dx = x - b[target].x, dy = y - b[target].y, d = 1;
        while (d * d < dx * dx + dy * dy) ++d;
        stage_x = b[target].x + dx * 250 / d; stage_y = b[target].y + dy * 250 / d;
        assault_stage = 1; stage_deadline = host_ticks + 900; last_target = target; next_attack = 0;
    }
    if (assault_stage) {
        if (own_army_near(stage_x, stage_y, 150) * 10 >= army * 7 || host_ticks >= stage_deadline) {
            assault_stage = 0;
            next_attack = 0;
        } else if (host_ticks >= next_attack) {
            next_attack = host_ticks + 240;
            return attack_move(stage_x, stage_y, 0) > 0;
        } else {
            return 0;
        }
    }
    if (host_ticks < next_attack) return 0;
    next_attack = host_ticks + (target == last_target ? 300 : 120);
    last_target = target;
    {
        /* Structures that shoot come first, and the whole army fires at one. */
        int defense = -1, best_d = 260 * 260;
        for (int i = 0; i < MAX_BUILDINGS; ++i) {
            int raw, d;
            if (!b[i].alive || !owners_hostile(SIDE, b[i].owner)) continue;
            raw = sc64_race_raw_building(b[i].type);
            if (raw != 125 && raw != 146 && raw != 162) continue;
            d = dist2(x, y, b[i].x, b[i].y);
            if (d < best_d) { best_d = d; defense = i; }
        }
        if (defense >= 0) {
            next_attack = host_ticks + 150;
            return attack_move(b[defense].x, b[defense].y, 1) > 0;
        }
    }
    return attack_move(b[target].x, b[target].y, 0) > 0;
}

static int policy_idle_workers(void)
{
    if (host_ticks < next_idle || !me.idle_workers) return 0;
    next_idle = host_ticks + 120;
    return return_idle_worker();
}

static void think(void)
{
    int hall;
    race_ai_census(SIDE, &me);
    hall = first_hall();
    if (hall >= 0) { home_x = b[hall].x; home_y = b[hall].y; }
    if (try_busy_save()) return;
    if (policy_defend()) return;
    wanted_minerals = 0;
    if (policy_workers()) return;
    if (policy_supply()) return;
    if (policy_army(1)) return;
    if (policy_structures()) return;
    if (policy_sunken()) return;
    if (policy_gas()) return;
    if (policy_research()) return;
    if (policy_army(2)) return;
    if (policy_idle_workers()) return;
    (void)policy_attack();
}

/* ---- Entry ----------------------------------------------------------- */

static void write_progress(void)
{
    static const int owners[4] = { SIDE, 1, 3, 4 };
    fprintf(stderr, "tick=%u frame=%d bank=%d/%d supply=%d/%d army=%d up=%d/%d ", host_ticks, frame,
            minerals, gas, supply_used, supply_cap, army_count(), weapons_level, armor_level);
    for (int p = 0; p < 4; ++p)
        fprintf(stderr, "o%d(u%d,a%d,b%d,m%d) ", owners[p], count_units(owners[p], -1),
                p ? hostile_army(owners[p]) : army_count(), count_buildings(owners[p], -1),
                race_owner_minerals(owners[p]));
    if (verbose) {
        fputs("ore", stderr);
        for (int k = 0; k < 4; ++k) {
            int ore = 0;
            for (int i = 0; i < MAX_MINERALS; ++i)
                if (m[i].amount > 0 && dist2(m[i].x, m[i].y, race_base_x[k], race_base_y[k]) < 300 * 300)
                    ore += m[i].amount;
            fprintf(stderr, " %d", ore);
        }
    }
    fputc('\n', stderr);
}

static int parse_race(char c)
{
    return c == 't' ? R_TER : c == 'z' ? R_ZERG : c == 'p' ? R_PROTOSS : -1;
}

int main(int argc, char **argv)
{
    const char *env;
    unsigned next_report = 2000;
    if (argc < 5 || argc > 6 || strlen(argv[2]) != 1 || parse_race(argv[2][0]) < 0 ||
        strlen(argv[3]) < 1 || strlen(argv[3]) > 3) {
        fprintf(stderr, "usage: %s MAP HUMAN CPUS TRACE [MAX_UPDATES]\n"
                        "  MAP 0 Outpost (Free for All) or 1 Blood Bath (Melee)\n"
                        "  HUMAN t, z or p; CPUS one to three of t, z, p in seat order\n"
                        "  SC64_CFM_START_ORDER=<n> pins the Blood Bath start order the cartridge resolved\n"
                        "  SC64_CFM_LOBBY_WAIT=<n> waits n updates on the Launch row (a different Melee draw)\n"
                        "  SC64_CFM_SAVE=0 skips the pause save; SC64_CFM_VERBOSE=1 logs actions\n"
                        "  other SC64_CFM_<NAME> values set the opening; see setup_plan()\n", argv[0]);
        return 2;
    }
    option_map = atoi(argv[1]);
    option_human = parse_race(argv[2][0]);
    option_bots = (int)strlen(argv[3]);
    for (int i = 0; i < option_bots; ++i)
        if ((option_bot_races[i] = parse_race(argv[3][i])) < 0) return 2;
    if (option_map != 0 && option_map != 1) return 2;
    option_melee = option_map == 1;
    if (argc == 6) match_limit = (unsigned)strtoul(argv[5], NULL, 10);
    if ((env = getenv("SC64_CFM_START_ORDER")) != NULL) option_start_order = (unsigned)atoi(env);
    if ((env = getenv("SC64_CFM_SAVE")) != NULL) option_save = atoi(env);
    if ((env = getenv("SC64_CFM_LOBBY_WAIT")) != NULL) option_lobby_wait = atoi(env);
    if ((env = getenv("SC64_CFM_LIST_ORDERS")) != NULL) option_list_orders = atoi(env);
    if ((env = getenv("SC64_CFM_VERBOSE")) != NULL) verbose = atoi(env);
    if (option_start_order && option_map != 1) return 2;
    trace = fopen(argv[4], "w");
    if (!trace) { perror(argv[4]); return 2; }
    fprintf(trace, "{\"profile\":\"custom-full-match\",\"map\":%d,\"human\":\"%s\",\"cpus\":\"%s\","
                   "\"check_interval\":%d,\"events\":[\n", option_map, argv[2], argv[3], CHECK_INTERVAL);
    gba_init();
    if (!launch_match()) {
        fprintf(stderr, "lobby launch failed: screen=%d page=%d row=%d\n", screen,
                race_front.page, race_front.row);
        fclose(trace);
        return 1;
    }
    fprintf(stderr, "launched map%d human%d melee%d order%u bases=(%d,%d),(%d,%d),(%d,%d),(%d,%d)\n",
            race_skirmish_map, race_human, !!race_skirmish_melee, (unsigned)race_skirmish_start_order,
            race_base_x[0], race_base_y[0], race_base_x[1], race_base_y[1],
            race_base_x[2], race_base_y[2], race_base_x[3], race_base_y[3]);
    setup_plan();
    trace_record("{\"kind\":\"plan\",\"host_tick\":%u,\"values\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}",
                 host_ticks, plan.line, plan.support_ratio, plan.defense, plan.defenses, plan.defense_frame,
                 plan.workers, plan.producers, plan.attack_min, plan.attack_margin, plan.attack_ratio,
                 plan.retreat_army, plan.first_attack, plan.tech_after, plan.lab_after, plan.tech_deadline,
                 plan.max_producers, plan.recall_threat, plan.all_in_frame, plan.defense_weight,
                 plan.strength_px, plan.early_workers, plan.grow_frame, option_save);
    if (plan.tech_frame)
        trace_record("{\"kind\":\"plan\",\"host_tick\":%u,\"tech_frame\":%d}", host_ticks, plan.tech_frame);
    home_x = race_base_x[0]; home_y = race_base_y[0];
    for (int i = 0; verbose && i < MAX_BUILDINGS; ++i)
        if (b[i].alive && race_ai_is_depot(b[i].type))
            fprintf(stderr, "hall owner %d at %d,%d\n", b[i].owner, b[i].x, b[i].y);
    idle(30);
    while (playing()) {
        idle(6);
        if (playing()) think();
        if (host_ticks >= next_report) { write_progress(); next_report += 2000; }
    }
    write_progress();
    {
        const char *outcome = won ? "won" : lost ? "lost" : "limit";
        static const int owners[4] = { 0, 1, 3, 4 };
        if (!trace_first) fputs(",\n", trace);
        fprintf(trace, "{\"kind\":\"end\",\"host_tick\":%u,\"frame\":%d,\"screen\":%d,\"minerals\":%d,"
                       "\"gas\":%d,\"hash\":%u,\"owners\":[", host_ticks, frame, screen, minerals, gas,
                world_hash());
        for (int p = 0; p < 4; ++p)
            fprintf(trace, "%s[%d,%d]", p ? "," : "", count_units(owners[p], -1),
                    count_buildings(owners[p], -1));
        fprintf(trace, "]}\n],\"terminal\":{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%d,\"won\":%d,"
                       "\"lost\":%d,\"save_tick\":%u,\"save_frame\":%u,\"resume_tick\":%u,"
                       "\"actions\":{\"build\":%d,\"train\":%d,\"attack\":%d,\"other\":%d}}}\n",
                outcome, host_ticks, frame, won, lost, save_tick, save_frame, resume_tick,
                actions_build, actions_train, actions_attack, actions_other);
        fclose(trace);
        fprintf(stderr, "terminal=%s tick=%u frame=%d save_tick=%u\n", outcome, host_ticks, frame, save_tick);
        return won ? 0 : lost ? 3 : 4;
    }
}
