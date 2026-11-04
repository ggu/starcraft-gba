/* Controller-goal coverage for the private four-map race campaign profile.
 * Compile with SC64_RACES and its ignored .local generated headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

static u16 controller_previous;
static unsigned long controller_ticks;
static int controller_mission_started;
static int controller_start_delay;
typedef struct { unsigned long tick; u16 held; } RaceInputEvent;
typedef struct { unsigned long tick; int screen, frame, won, lost, elapsed, flags, phase;
    int rescue_owner, rescue_alive, chrysalis_owner, chrysalis_alive;
    int chrysalis_x, chrysalis_y, carrier_unit, carrier_x, carrier_y;
    int side_units, enemy_units, side_buildings, enemy_buildings; } RaceCheckpoint;
static RaceInputEvent race_events[32768];
static RaceCheckpoint race_checkpoints[128];
static int race_event_count, race_checkpoint_count, race_event_overflow;
static const char *race_trace_path;
static int side_drone(void);
static int side_hunters_near(int x, int y, int radius);

static void race_trace_checkpoint(void)
{
    RaceCheckpoint *c;
    int i;
    if (!race_trace_path || controller_ticks % 1000 ||
        race_checkpoint_count >= (int)(sizeof(race_checkpoints)/sizeof(race_checkpoints[0]))) return;
    c = &race_checkpoints[race_checkpoint_count++];
    c->tick = controller_ticks; c->screen = screen; c->frame = frame; c->won = won; c->lost = lost;
    c->elapsed = race_campaign.elapsed_frames; c->flags = race_campaign.objective_flags; c->phase = race_campaign.phase;
    c->rescue_owner = -1; c->rescue_alive = 0; c->chrysalis_owner = -1; c->chrysalis_alive = 0;
    c->chrysalis_x = c->chrysalis_y = c->carrier_x = c->carrier_y = -1;
    c->carrier_unit = race_campaign.carrier_unit;
    if (race_campaign.rescue_unit < MAX_UNITS) {
        c->rescue_owner = u[race_campaign.rescue_unit].owner;
        c->rescue_alive = u[race_campaign.rescue_unit].alive;
    }
    if (race_campaign.chrysalis_unit < MAX_UNITS) {
        c->chrysalis_owner = u[race_campaign.chrysalis_unit].owner;
        c->chrysalis_alive = u[race_campaign.chrysalis_unit].alive;
        c->chrysalis_x = u[race_campaign.chrysalis_unit].x;
        c->chrysalis_y = u[race_campaign.chrysalis_unit].y;
    }
    if (race_campaign.carrier_unit < MAX_UNITS) {
        c->carrier_x = u[race_campaign.carrier_unit].x;
        c->carrier_y = u[race_campaign.carrier_unit].y;
    }
    c->side_units = c->enemy_units = c->side_buildings = c->enemy_buildings = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].owner == SIDE) ++c->side_units;
        if (u[i].owner == ENEMY) ++c->enemy_units;
    }
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].owner == SIDE) ++c->side_buildings;
        if (b[i].owner == ENEMY) ++c->enemy_buildings;
    }
}

static void race_trace_save(void)
{
    FILE *f;
    int i;
    if (!race_trace_path || !(f = fopen(race_trace_path, "w"))) return;
    fprintf(f, "{\"total_ticks\":%lu,\"event_overflow\":%d,\"target_race\":%d,\"mode\":%d,\"mission\":%d,\"race_human\":%d,\"race_mode\":%d,\"race_selected_mission\":%d,\"race_completed\":%d,\"events\":[",
            controller_ticks, race_event_overflow,
            race_human, race_mode, race_campaign.mission,
            race_human, race_mode, race_selected_mission, race_completed);
    for (i = 0; i < race_event_count; ++i)
        fprintf(f, "%s{\"tick\":%lu,\"held\":%u}", i ? "," : "", race_events[i].tick, race_events[i].held);
    fprintf(f, "],\"checkpoints\":[");
    for (i = 0; i < race_checkpoint_count; ++i) {
        RaceCheckpoint *c = &race_checkpoints[i];
        fprintf(f, "%s{\"tick\":%lu,\"screen\":%d,\"frame\":%d,\"won\":%d,\"lost\":%d,\"elapsed_frames\":%d,\"objective_flags\":%d,\"phase\":%d,\"rescue_owner\":%d,\"rescue_alive\":%d,\"chrysalis_owner\":%d,\"chrysalis_alive\":%d,\"chrysalis_x\":%d,\"chrysalis_y\":%d,\"carrier_unit\":%d,\"carrier_x\":%d,\"carrier_y\":%d,\"side_units\":%d,\"enemy_units\":%d,\"side_buildings\":%d,\"enemy_buildings\":%d}",
                i ? "," : "", c->tick, c->screen, c->frame, c->won, c->lost, c->elapsed, c->flags, c->phase,
                c->rescue_owner, c->rescue_alive, c->chrysalis_owner, c->chrysalis_alive,
                c->chrysalis_x, c->chrysalis_y, c->carrier_unit, c->carrier_x, c->carrier_y,
                c->side_units, c->enemy_units, c->side_buildings, c->enemy_buildings);
    }
    fprintf(f, "],\"expected\":{\"mission\":%d,\"race_selected_mission\":%d,\"race_human\":%d,\"race_mode\":%d,\"race_completed\":%d,\"screen\":%d,\"frame\":%d,\"won\":%d,\"lost\":%d,\"elapsed_frames\":%u,\"objective_flags\":%u,\"phase\":%u,\"rescue_owner\":%d,\"rescue_alive\":%d,\"chrysalis_owner\":%d,\"chrysalis_alive\":%d,\"carrier_unit\":%u}}\n",
            race_campaign.mission, race_selected_mission, race_human, race_mode, race_completed,
            screen, frame, won, lost, race_campaign.elapsed_frames,
            race_campaign.objective_flags, race_campaign.phase,
            race_campaign.rescue_unit < MAX_UNITS ? u[race_campaign.rescue_unit].owner : -1,
            race_campaign.rescue_unit < MAX_UNITS ? u[race_campaign.rescue_unit].alive : 0,
            race_campaign.chrysalis_unit < MAX_UNITS ? u[race_campaign.chrysalis_unit].owner : -1,
            race_campaign.chrysalis_unit < MAX_UNITS ? u[race_campaign.chrysalis_unit].alive : 0,
            race_campaign.carrier_unit);
    fclose(f);
}

static void controller_frame(u16 held)
{
    /* A press on the result screen starts the next mission and resets won. */
    if (controller_mission_started && (won || lost)) held = 0;
    if (held != controller_previous) {
        if (race_event_count < (int)(sizeof(race_events)/sizeof(race_events[0]))) {
            race_events[race_event_count].tick = controller_ticks;
            race_events[race_event_count++].held = held;
        } else {
            race_event_overflow = 1;
        }
    }
    game_update(held, held & (u16)~controller_previous);
    controller_previous = held;
    ++controller_ticks;
    race_trace_checkpoint();
}

static void controller_press(u16 key)
{
    controller_frame(key);
    controller_frame(0);
}

static void controller_cursor(int x, int y)
{
    int guard = 0;
    /* Cursor speed is an even number of pixels, so exact comparison can
     * alternate forever when the requested coordinate has opposite parity. */
    while ((abs(cx - x) > 3 || abs(cy - y) > 3) && !won && !lost && guard++ < 3000) {
        u16 h = 0; /* R selects a group, so do not hold it during orders. */
        if (abs(cx - x) > 3) { if (cx < x) h |= KEY_RIGHT; else h |= KEY_LEFT; }
        if (abs(cy - y) > 3) { if (cy < y) h |= KEY_DOWN; else h |= KEY_UP; }
        controller_frame(h);
    }
    controller_frame(0);
    if (guard >= 3000) fprintf(stderr, "cursor guard target=%d,%d actual=%d,%d\n",x,y,cx,cy);
}

/* R moves the cursor four pixels a frame. Leave an axis alone once it is within
 * two pixels: stepping it anyway overshoots, and two axes overshooting together
 * can cycle around a stopped unit between offsets like (3,1) and (-1,-3), whose
 * squared distance of 10 never reaches the 8 this band guarantees. */
static u16 controller_chase_keys(int x, int y)
{
    u16 held = KEY_R;
    if (cx < x - 2) held |= KEY_RIGHT;
    if (cx > x + 2) held |= KEY_LEFT;
    if (cy < y - 2) held |= KEY_DOWN;
    if (cy > y + 2) held |= KEY_UP;
    return held;
}

/* Follow a moving worker with ordinary cursor input, then verify the UI
 * selected the requested live slot. A one-time cursor target can go stale
 * while the worker walks its previous order. */
static int controller_select_worker(int worker)
{
    int steps = 0;
    if (worker < 0 || worker >= MAX_UNITS || !u[worker].alive ||
        u[worker].owner != SIDE || !sc64_race_is_worker(u[worker].type)) return 0;
    /* Click as close as the four-pixel cursor step allows so a nearby unit
     * cannot consume A before the moving worker. */
    while (u[worker].alive && dist2(cx, cy, u[worker].x, u[worker].y) > 2 * 2 + 2 * 2 &&
           screen == SCREEN_PLAY && !won && !lost && steps++ < 120)
        controller_frame(controller_chase_keys(u[worker].x, u[worker].y));
    if (!u[worker].alive || dist2(cx, cy, u[worker].x, u[worker].y) > 2 * 2 + 2 * 2)
        return 0;
    controller_press(KEY_A);
    return selected_building < 0 && u[worker].selected && selected_count() == 1;
}

static int controller_select_actor(int actor)
{
    static const int offsets[][2] = {{0,0},{-8,0},{8,0},{0,-8},{0,8},
                                    {-8,-8},{8,-8},{-8,8},{8,8}};
    int i;
    if (actor < 0 || actor >= MAX_UNITS || !u[actor].alive ||
        u[actor].owner != SIDE) return 0;
    for (i = 0; i < (int)(sizeof(offsets)/sizeof(offsets[0])); ++i) {
        controller_cursor(u[actor].x + offsets[i][0], u[actor].y + offsets[i][1]);
        controller_press(KEY_A);
        if (selected_building < 0 && u[actor].selected && selected_count() == 1) return 1;
    }
    return 0;
}

/* A worker can stand inside the Nexus click target; an ordinary click then
 * selects the building. Use a moving L-button unit box for the construction
 * order so the Probe stays inside it while mining. */
static int controller_select_worker_box(int worker)
{
    int steps = 0;
    if (worker < 0 || worker >= MAX_UNITS || !u[worker].alive ||
        u[worker].owner != SIDE || !sc64_race_is_worker(u[worker].type)) return 0;
    /* Follow the mining Probe to its current position, then drag while
     * tracking its short mining walk. A stationary click can hit a nearby
     * Nexus instead. */
    while (u[worker].alive && dist2(cx, cy, u[worker].x, u[worker].y) > 8 &&
           screen == SCREEN_PLAY && !won && !lost && steps++ < 200)
        controller_frame(controller_chase_keys(u[worker].x, u[worker].y));
    if (screen != SCREEN_PLAY || won || lost || !u[worker].alive ||
        dist2(cx, cy, u[worker].x, u[worker].y) > 8) return 0;
    controller_frame(KEY_L);
    for (steps = 0; steps < 16 && screen == SCREEN_PLAY && !won && !lost; ++steps) {
        u16 held = KEY_L;
        if (cx < u[worker].x) held |= KEY_RIGHT;
        if (cx > u[worker].x) held |= KEY_LEFT;
        if (cy < u[worker].y) held |= KEY_DOWN;
        if (cy > u[worker].y) held |= KEY_UP;
        controller_frame(held);
    }
    if (screen != SCREEN_PLAY || won || lost) return 0;
    controller_frame(0);
    return screen == SCREEN_PLAY && !won && !lost &&
           selected_count() == 1 && u[worker].selected;
}

/* Use an ordinary L-button drag to select the combat force while excluding
 * the protected objective actor. Geometry is read from live units solely to
 * place the cursor; selection and orders still go through game_update(). */
static int controller_select_combat_except(int excluded)
{
    int i, steps = 0, left = WORLD_W, top = WORLD_H, right = -1, bottom = -1;
    int max_steps = WORLD_W + WORLD_H + 8;
    for (i = 0; i < MAX_UNITS; ++i) if (i != excluded && u[i].alive &&
        u[i].owner == SIDE && !sc64_race_is_worker(u[i].type)) {
        if (u[i].x < left) left = u[i].x;
        if (u[i].x > right) right = u[i].x;
        if (u[i].y < top) top = u[i].y;
        if (u[i].y > bottom) bottom = u[i].y;
    }
    if (right < left || bottom < top) return 0;
    left = left > 8 ? left - 8 : 0;
    top = top > 8 ? top - 8 : 0;
    right = right + 8 < WORLD_W ? right + 8 : WORLD_W - 1;
    bottom = bottom + 8 < WORLD_H ? bottom + 8 : WORLD_H - 1;
    /* When the protected actor stands inside the box, give up the smallest part
     * of the force: keep the side of him that holds the most units. */
    if (excluded >= 0 && excluded < MAX_UNITS && u[excluded].alive &&
        u[excluded].x >= left && u[excluded].x <= right &&
        u[excluded].y >= top && u[excluded].y <= bottom) {
        int west = 0, east = 0, north = 0, south = 0, ex = u[excluded].x, ey = u[excluded].y;
        for (i = 0; i < MAX_UNITS; ++i) if (i != excluded && u[i].alive &&
            u[i].owner == SIDE && !sc64_race_is_worker(u[i].type)) {
            if (u[i].x < ex - 32) ++west;
            if (u[i].x > ex + 32) ++east;
            if (u[i].y < ey - 32) ++north;
            if (u[i].y > ey + 32) ++south;
        }
        if (south >= north && south >= east && south >= west) top = ey + 24;
        else if (north >= east && north >= west) bottom = ey - 24;
        else if (east >= west) left = ex + 24;
        else right = ex - 24;
        if (left >= right || top >= bottom) return 0;
    }
    controller_cursor(left, top);
    if (screen != SCREEN_PLAY || won || lost || abs(cx - left) > 3 || abs(cy - top) > 3)
        return 0;
    controller_frame(KEY_L);
    while (cx < right || cy < bottom) {
        u16 held = KEY_L | KEY_R; /* R doubles cursor speed during the drag. */
        int old_x = cx, old_y = cy;
        if (screen != SCREEN_PLAY || won || lost || steps++ >= max_steps) {
            controller_frame(0);
            return 0;
        }
        if (cx < right) held |= KEY_RIGHT;
        if (cy < bottom) held |= KEY_DOWN;
        controller_frame(held);
        if (cx == old_x && cy == old_y && (cx < right || cy < bottom)) {
            controller_frame(0);
            return 0;
        }
    }
    controller_frame(0);
    if (screen != SCREEN_PLAY || won || lost || abs(cx - right) > 4 || abs(cy - bottom) > 4)
        return 0;
    return selected_count() > 0 &&
           (excluded < 0 || excluded >= MAX_UNITS || !u[excluded].selected);
}

static int controller_attack(int x, int y, int max_ticks)
{
    int guard = 0, i;
    controller_press(KEY_R);
    controller_press(KEY_B);
    while (menu_cursor != MENU_ATTACK && guard++ < MENU_COUNT)
        controller_press(menu_cursor < MENU_ATTACK ? KEY_DOWN : KEY_UP);
    if (menu_cursor != MENU_ATTACK) return 0;
    controller_press(KEY_A);
    controller_cursor(x, y);
    controller_press(KEY_A);
    for (i = 0; i < max_ticks && screen == SCREEN_PLAY; ++i)
        controller_frame(0);
    return screen == SCREEN_PLAY || (won && !lost);
}

static int controller_attack_order(int x, int y)
{
    int guard = 0;
    controller_press(KEY_B);
    while (menu_cursor != MENU_ATTACK && guard++ < MENU_COUNT)
        controller_press(menu_cursor < MENU_ATTACK ? KEY_DOWN : KEY_UP);
    if (menu_cursor != MENU_ATTACK) return 0;
    controller_press(KEY_A);
    controller_cursor(x, y);
    controller_press(KEY_A);
    return mode == MODE_NORMAL;
}

static int controller_move_selected(int x, int y)
{
    int guard = 0;
    controller_press(KEY_B);
    while (menu_cursor != MENU_MOVE && guard++ < MENU_COUNT)
        controller_press(menu_cursor < MENU_MOVE ? KEY_DOWN : KEY_UP);
    if (menu_cursor != MENU_MOVE) return 0;
    controller_press(KEY_A); controller_cursor(x, y); controller_press(KEY_A);
    return mode == MODE_NORMAL;
}

/* Photon Cannon range is 112 pixels in race_tick_defenses(); the margin allows
 * for a walk that bends away from the straight line it is sampled along. */
static int enemy_cannon_covering_walk(int x0, int y0, int x1, int y1)
{
    int i, step;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == ENEMY && b[i].type == B_PHOTON_CANNON)
            for (step = 0; step <= 8; ++step)
                if (dist2(x0 + (x1 - x0) * step / 8, y0 + (y1 - y0) * step / 8,
                          b[i].x, b[i].y) <= 128 * 128) return i;
    return -1;
}

/* Protoss base-building for the P1 and P2 routes. Live state only chooses when
 * and where to act; every order is ordinary input through game_update(). The
 * world clock stops while a command menu is open, so each order leaves the UI
 * normal again. */
static int controller_playing(void)
{ return screen == SCREEN_PLAY && !won && !lost; }

static void controller_normal_ui(void)
{
    int guard;
    for (guard = 0; guard < 3 && controller_playing() && (menu_open || mode != MODE_NORMAL); ++guard)
        controller_press(KEY_B);
}

/* Holding R doubles cursor speed; it only selects a group when pressed alone. */
static void controller_cursor_fast(int x, int y)
{
    int guard = 0;
    while ((abs(cx - x) > 3 || abs(cy - y) > 3) && controller_playing() && guard++ < 3000) {
        u16 held = KEY_R;
        if (abs(cx - x) > 3) held |= cx < x ? KEY_RIGHT : KEY_LEFT;
        if (abs(cy - y) > 3) held |= cy < y ? KEY_DOWN : KEY_UP;
        controller_frame(held);
    }
    controller_frame(0);
}

static int side_buildings_of(int type, int complete_only)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE && b[i].type == type &&
        (!complete_only || building_complete(i))) ++n;
    return n;
}

static int side_units_of(int type)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE && u[i].type == type) ++n;
    return n;
}

static int side_queued_of(int type)
{
    int i, j, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE)
        for (j = 0; j < b[i].queue_count; ++j)
            if (b[i].queue[(b[i].queue_head + j) % 5] == type) ++n;
    return n;
}

static int side_army_size(int excluded)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (i != excluded && u[i].alive && u[i].owner == SIDE &&
        !sc64_race_is_worker(u[i].type)) ++n;
    return n;
}

static int side_constructing(void)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].under_construction) ++n;
    return n;
}

static int side_home_nexus(void)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_NEXUS && building_complete(i)) return i;
    return -1;
}

/* An idle Probe still holding cargo cannot be sent back to minerals, so build
 * with empty ones. */
static int side_free_probe(int x, int y)
{
    int i, best = -1, best_d = 0x7fffffff;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
        !SC64_BUILD_WORKER_ORDER(u[i].order) && u[i].order != ORDER_GAS && !u[i].carry) {
        int d = dist2(u[i].x, u[i].y, x, y);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

/* A click near a mining Probe can land on the Nexus, so fall back to a drag.
 * Idle Probes stack on a spent patch, where a tap takes the nearest of them. A
 * drag there has no area and would clear the selection, so keep a stack mate. */
static int controller_select_probe(int probe)
{
    int i, picked;
    controller_normal_ui();
    if (!controller_select_worker(probe)) {
        picked = selected_building < 0 ? find_selected_worker() : -1;
        if ((picked < 0 || dist2(u[picked].x, u[picked].y, u[probe].x, u[probe].y) > 4 * 4 ||
             !u[picked].carry != !u[probe].carry) && !controller_select_worker_box(probe) &&
            (selected_building >= 0 || find_selected_worker() < 0)) return 0;
    }
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected && !sc64_race_is_worker(u[i].type)) return 0;
    return 1;
}

static int controller_open_menu_row(int row)
{
    int guard = 0;
    controller_press(KEY_B);
    while (menu_open && menu_cursor != row && guard++ < 2 * MENU_COUNT)
        controller_press(menu_cursor < row ? KEY_DOWN : KEY_UP);
    return menu_open && menu_cursor == row;
}

/* Buildings packed tightly wall in Gateway exits, so keep a wide lattice clear
 * of the mineral line and of each other. */
static int controller_find_site(int type, int *site_x, int *site_y)
{
    int home = side_home_nexus(), dx, dy, i, best = 0x7fffffff;
    int spacing = type == B_PYLON ? 48 : 72;
    if (home < 0) return 0;
    for (dy = -512; dy <= 512; dy += 32)
        for (dx = -512; dx <= 512; dx += 32) {
            int x = b[home].x + dx, y = b[home].y + dy, d = dx * dx + dy * dy, clear = 1;
            if (d >= best || d < 96 * 96 || x < 32 || x >= WORLD_W - 32 || y < 32 || y >= WORLD_H - 32 ||
                !race_can_site(SIDE, type, x, y) || !can_place(type, x, y)) continue;
            for (i = 0; i < MAX_MINERALS && clear; ++i)
                if (m[i].amount > 0 && dist2(x, y, m[i].x, m[i].y) < 80 * 80) clear = 0;
            for (i = 0; i < MAX_BUILDINGS && clear; ++i)
                if (b[i].alive && dist2(x, y, b[i].x, b[i].y) < spacing * spacing) clear = 0;
            if (clear) { best = d; *site_x = x; *site_y = y; }
        }
    return best != 0x7fffffff;
}

static int controller_place(int type, int row, int x, int y)
{
    int probe = side_free_probe(x, y), placed;
    if (probe < 0 || !controller_select_probe(probe) || !controller_open_menu_row(row)) {
        controller_normal_ui();
        return 0;
    }
    controller_press(KEY_A);
    if (mode != MODE_BUILD || menu != type) { controller_normal_ui(); return 0; }
    controller_cursor_fast(x, y);
    controller_press(KEY_A);
    placed = mode == MODE_NORMAL;
    controller_normal_ui();
    return placed;
}

static int controller_build(int type, int row)
{
    int x, y;
    return controller_find_site(type, &x, &y) && controller_place(type, row, x, y);
}

static int controller_build_assimilator(void)
{
    int home = side_home_nexus(), i, pick = -1, best = 0x7fffffff;
    if (home < 0) return 0;
    for (i = 0; i < geyser_count; ++i) {
        int d = dist2(b[home].x, b[home].y, geysers[i].x, geysers[i].y);
        if (d < best && d < 400 * 400 && race_can_site(SIDE, B_ASSIMILATOR, geysers[i].x, geysers[i].y) &&
            can_place(B_ASSIMILATOR, geysers[i].x, geysers[i].y)) { best = d; pick = i; }
    }
    return pick >= 0 && controller_place(B_ASSIMILATOR, MENU_REFINERY, geysers[pick].x, geysers[pick].y);
}

static int side_walking_builders(void)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        SC64_BUILD_WORKER_ORDER(u[i].order)) ++n;
    return n;
}

static int side_unpowered_gateway(void)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE && b[i].type == B_GATEWAY &&
        building_complete(i) && !race_site_powered(SIDE, b[i].x, b[i].y)) return i;
    return -1;
}

/* P1's rescued Gateway has no Pylon in range. */
static int controller_power_gateway(int gateway)
{
    static const int offsets[][2] = {{-64,-64},{-64,64},{0,96},{0,-96},{-96,0},{96,0},{64,64},{64,-64}};
    int i;
    for (i = 0; i < (int)(sizeof(offsets)/sizeof(offsets[0])); ++i) {
        int x = b[gateway].x + offsets[i][0], y = b[gateway].y + offsets[i][1];
        if (race_can_site(SIDE, B_PYLON, x, y) && can_place(B_PYLON, x, y))
            return controller_place(B_PYLON, MENU_DEPOT, x, y);
    }
    return 0;
}

/* A tap selects an own unit within 31 pixels or building within 26 before it can
 * order a patch within 16, so a Probe parked beside a patch swallows every tap.
 * Find a tap point clear of both, allowing for the cursor stopping 3 pixels off. */
static int controller_patch_tap(int patch, int *tap_x, int *tap_y)
{
    static const int offsets[][2] = {{0,0},{-8,0},{8,0},{0,-8},{0,8},{-8,-8},{8,-8},
                                     {-8,8},{8,8},{-12,0},{12,0},{0,-12},{0,12}};
    int k, i;
    for (k = 0; k < (int)(sizeof(offsets)/sizeof(offsets[0])); ++k) {
        int x = m[patch].x + offsets[k][0], y = m[patch].y + offsets[k][1];
        for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
            dist2(x, y, u[i].x, u[i].y) < 34 * 34) break;
        if (i < MAX_UNITS) continue;
        for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE &&
            dist2(x, y, b[i].x, b[i].y) < 29 * 29) break;
        if (i < MAX_BUILDINGS) continue;
        *tap_x = x; *tap_y = y;
        return 1;
    }
    return 0;
}

/* A patch that runs dry leaves its Probes idle, and Probes stacked on one patch
 * drain it quickly, so send up to three idle Probes to the least-worked patches
 * near the Nexus on each pass. */
static void controller_mine_idle_probes(void)
{
    int home = side_home_nexus(), pass;
    if (home < 0) return;
    for (pass = 0; pass < 3 && controller_playing(); ++pass) {
        int i, j, probe = -1, patch = -1, best = 0x7fffffff, tap_x = 0, tap_y = 0;
        for (i = 0; i < MAX_UNITS && probe < 0; ++i) if (u[i].alive && u[i].owner == SIDE &&
            u[i].type == U_PROBE && u[i].order == ORDER_IDLE && !u[i].carry) probe = i;
        if (probe < 0) return;
        for (i = 0; i < MAX_MINERALS; ++i) if (m[i].amount > 0 && currently_visible(m[i].x, m[i].y)) {
            int workers = 0, score = 0x7fffffff, x, y;
            if (!controller_patch_tap(i, &x, &y)) continue;
            for (j = 0; j < MAX_BUILDINGS; ++j) if (b[j].alive && b[j].owner == SIDE &&
                b[j].type == B_NEXUS && building_complete(j)) {
                int depot = dist2(m[i].x, m[i].y, b[j].x, b[j].y) / 256;
                if (depot < score) score = depot;
            }
            for (j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
                u[j].order == ORDER_MINE && u[j].target == i) ++workers;
            score += workers * 256;
            if (score < best) { best = score; patch = i; tap_x = x; tap_y = y; }
        }
        if (patch < 0 || !controller_select_probe(probe)) return;
        controller_cursor_fast(tap_x, tap_y);
        controller_press(KEY_A);
        controller_normal_ui();
    }
}

/* The mineral order refuses a loaded Probe and nothing returns its cargo, so a
 * builder that was carrying stays idle for good. Tapping the Assimilator discards
 * the cargo, and STOP frees the Probe for the next mining pass. */
static void controller_free_loaded_probe(void)
{
    int i, refinery = -1, probe = -1;
    for (i = 0; i < MAX_BUILDINGS && refinery < 0; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_ASSIMILATOR && building_complete(i)) refinery = i;
    for (i = 0; i < MAX_UNITS && probe < 0; ++i) if (u[i].alive && u[i].owner == SIDE &&
        u[i].type == U_PROBE && u[i].order == ORDER_IDLE && u[i].carry) probe = i;
    if (refinery < 0 || probe < 0 || !controller_select_probe(probe)) return;
    controller_cursor_fast(b[refinery].x, b[refinery].y);
    controller_press(KEY_A);
    if (controller_open_menu_row(MENU_STOP)) controller_press(KEY_A);
    controller_normal_ui();
}

/* Keep two Probes on the Assimilator while gas is short, and return them to
 * minerals once it has banked: Dragoons cost far more minerals than gas. */
static void controller_staff_gas(void)
{
    int i, gas_workers = 0, refinery = -1, probe = -1;
    for (i = 0; i < MAX_BUILDINGS && refinery < 0; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_ASSIMILATOR && building_complete(i)) refinery = i;
    if (refinery < 0) return;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE &&
        u[i].order == ORDER_GAS) {
        ++gas_workers;
        if (!u[i].carry) probe = i;
    }
    if (gas >= 600) {
        /* STOP frees the Probe; the idle-Probe pass then sends it to a patch. */
        if (probe < 0 || !controller_select_probe(probe) || !controller_open_menu_row(MENU_STOP)) {
            controller_normal_ui();
            return;
        }
        controller_press(KEY_A);
        controller_normal_ui();
        return;
    }
    if (gas_workers >= 2 || gas >= 300 || side_units_of(U_PROBE) < 6) return;
    probe = side_free_probe(b[refinery].x, b[refinery].y);
    if (probe < 0 || !controller_select_probe(probe)) return;
    controller_cursor_fast(b[refinery].x, b[refinery].y);
    controller_press(KEY_A);
    controller_normal_ui();
}

static int controller_queue_unit(int building, int type)
{
    static const int offsets[][2] = {{0,0},{-16,0},{16,0},{0,-16},{0,16}};
    int attempt, guard, before;
    controller_normal_ui();
    for (attempt = 0; attempt < 5 && selected_building != building && controller_playing(); ++attempt) {
        controller_cursor_fast(b[building].x + offsets[attempt][0], b[building].y + offsets[attempt][1]);
        controller_press(KEY_A);
    }
    if (selected_building != building) return 0;
    /* A taps on the selected building train; R cycles the Gateway's product. */
    if (b[building].type == B_GATEWAY)
        for (guard = 0; guard < 4 && train_type != type; ++guard) controller_press(KEY_R);
    before = b[building].queue_count;
    if (b[building].type != B_GATEWAY || train_type == type) controller_press(KEY_A);
    return b[building].queue_count > before;
}

/* One pass of the build order: Probes, Pylons when supply runs short, gas and
 * a Cybernetics Core for Dragoons, then up to max_gateways producing. */
static void controller_protoss_economy(int max_gateways, int probe_goal)
{
    int home = side_home_nexus(), i, free_supply, reserve = 0;
    int planned_minerals, planned_gas;
    if (home < 0 || !controller_playing()) return;
    controller_free_loaded_probe();
    controller_mine_idle_probes();
    controller_staff_gas();
    free_supply = race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE);
    if (side_units_of(U_PROBE) + side_queued_of(U_PROBE) < probe_goal && minerals >= 50 &&
        free_supply >= 2 && b[home].queue_count < 2)
        (void)controller_queue_unit(home, U_PROBE);
    /* A placed building appears only when its Probe arrives. Wait for that, or
     * the next pass orders the same building again. */
    if (side_constructing() < 2 && !side_walking_builders() &&
        side_free_probe(b[home].x, b[home].y) >= 0) {
        int unpowered = side_unpowered_gateway();
        if (unpowered >= 0 && minerals >= 100)
            (void)controller_power_gateway(unpowered);
        else if (free_supply <= 10 && minerals >= 100 && side_buildings_of(B_PYLON, 0) < 16)
            (void)controller_build(B_PYLON, MENU_DEPOT);
        else if (!side_buildings_of(B_GATEWAY, 0) && minerals >= 150)
            (void)controller_build(B_GATEWAY, MENU_BARRACKS);
        else if (!side_buildings_of(B_ASSIMILATOR, 0) && minerals >= 100 && side_units_of(U_PROBE) >= 5)
            (void)controller_build_assimilator();
        else if (!side_buildings_of(B_CYBERNETICS_CORE, 0) && minerals >= 200 &&
                 side_buildings_of(B_GATEWAY, 1))
            (void)controller_build(B_CYBERNETICS_CORE, MENU_ACADEMY);
        else if (side_buildings_of(B_GATEWAY, 0) < max_gateways && minerals >= 150)
            (void)controller_build(B_GATEWAY, MENU_BARRACKS);
    }
    /* Save for the Core rather than spending every 100 minerals on Zealots. */
    if (side_buildings_of(B_GATEWAY, 1) && !side_buildings_of(B_CYBERNETICS_CORE, 0)) reserve = 200;
    for (i = 0; i < MAX_BUILDINGS && controller_playing(); ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_GATEWAY && building_complete(i) &&
            race_site_powered(SIDE, b[i].x, b[i].y) && b[i].queue_count < 2) {
            /* Zealots cannot hit the Mutalisks, so save for Dragoons once they
             * are available. */
            int want_dragoon = side_buildings_of(B_CYBERNETICS_CORE, 1) && gas >= 50;
            free_supply = race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE);
            race_pending_build_cost(SIDE, &planned_minerals, &planned_gas);
            if (free_supply < 4 ||
                minerals - planned_minerals < (want_dragoon ? 125 : 100) + reserve) break;
            (void)controller_queue_unit(i, want_dragoon ? U_DRAGOON : U_ZEALOT);
        }
    controller_normal_ui();
}

/* R retrains a selected producer instead of selecting the army. A short drag
 * clears the producer through the ordinary box-select gesture. */
static void controller_select_army(void)
{
    int steps;
    controller_normal_ui();
    if (selected_building >= 0) {
        controller_frame(KEY_L);
        for (steps = 0; steps < 6 && controller_playing(); ++steps)
            controller_frame(KEY_L | KEY_RIGHT);
        controller_frame(0);
    }
    controller_press(KEY_R);
}

/* Meet raiders that reach the base; otherwise hold the mineral line, which the
 * AI's small attack waves reach first. */
static void controller_defend_base(void)
{
    int home = side_home_nexus(), i, raider = -1, best = 360 * 360;
    if (home < 0) return;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
        int d = dist2(u[i].x, u[i].y, b[home].x, b[home].y);
        if (d < best) { best = d; raider = i; }
    }
    controller_select_army();
    if (selected_count() > 0)
        (void)controller_attack_order(raider >= 0 ? u[raider].x : b[home].x,
                                      raider >= 0 ? u[raider].y : b[home].y - 64);
    controller_normal_ui();
}

/* The nearest enemy building to any unit of the force; force-wide nearest so
 * that a straggler cannot pull the army back across the map. */
static int controller_nearest_enemy_building(int excluded)
{
    int j, k, target = -1, closest = 0x7fffffff;
    for (j = 0; j < MAX_BUILDINGS; ++j) if (b[j].alive && b[j].owner == ENEMY) {
        for (k = 0; k < MAX_UNITS; ++k) if (k != excluded && u[k].alive && u[k].owner == SIDE &&
            !sc64_race_is_worker(u[k].type)) {
            int q = dist2(u[k].x, u[k].y, b[j].x, b[j].y);
            if (q < closest) { closest = q; target = j; }
        }
    }
    return target;
}

/* With no army left and nothing to rebuild it from, only Fenix can finish the
 * mission. Holding him back would stall to the input limit. */
static int controller_last_stand(void)
{
    int target = controller_nearest_enemy_building(-1);
    if (target < 0 || side_queued_of(U_ZEALOT) || side_queued_of(U_DRAGOON) || side_constructing() ||
        (minerals >= 100 && side_buildings_of(B_GATEWAY, 1) && side_unpowered_gateway() < 0))
        return 0;
    controller_select_army();
    (void)controller_attack_order(b[target].x, b[target].y);
    controller_normal_ui();
    return 1;
}

static int controller_mission(int mission)
{
    int i, target = -1;
    controller_previous = 0; controller_ticks = 0; controller_mission_started = 0;
    race_event_count = 0; race_checkpoint_count = 0; race_event_overflow = 0;
    game_init();
    /* HOME → CAMPAIGN; choose the mission's faction, then mission and briefing. */
    controller_press(KEY_A);
#ifdef SC64_UNIFIED
    {
        int faction = sc64_frontend_mission_faction(mission);
        int campaign_row = sc64_front_campaign_row(faction);
        int ordinal = sc64_catalog_playable_ordinal(sc64_front_catalog_faction(faction), mission);
        int target_page, page_count, guard = 0;
        if (ordinal < 0) return 0;
        if (race_front.page == SC_FRONT_CAMPAIGN) {
            while (race_front.row != campaign_row && guard++ < sc64_front_campaign_count())
                controller_press(KEY_DOWN);
            if (race_front.row != campaign_row) return 0;
            controller_press(KEY_A);
        } else if (race_front.page != SC_FRONT_MISSION ||
                   sc64_front_catalog_faction(race_front.faction) !=
                       sc64_front_catalog_faction(faction)) {
            return 0;
        }
        if (race_front.page != SC_FRONT_MISSION) return 0;
        target_page = ordinal / 2;
        page_count = (sc64_catalog_playable_count(sc64_front_catalog_faction(faction)) + 1) / 2;
        guard = 0;
        while (race_front.mission_page != target_page && guard++ < page_count)
            controller_press(KEY_RIGHT);
        if (race_front.mission_page != target_page) return 0;
        while (race_front.row != ordinal % 2 && guard++ < page_count + 2)
            controller_press(KEY_DOWN);
        if (race_front.row != ordinal % 2 ||
            sc64_catalog_playable_at(sc64_front_catalog_faction(faction),
                                     race_front.mission_page * 2 + race_front.row) != mission)
            return 0;
    }
#else
    if (mission >= SC64_RACE_MISSION_P1) controller_press(KEY_RIGHT);
    controller_press(KEY_A);
    if (mission & 1) controller_press(KEY_DOWN);
#endif
    controller_press(KEY_A);
    controller_press(KEY_START);
    if (screen != SCREEN_PLAY || race_campaign.mission != mission) return 0;
    controller_mission_started = 1;
    for (i = 0; i < controller_start_delay && controller_playing(); ++i) controller_frame(0);
    if (mission == SC64_RACE_MISSION_Z2) {
        int c = race_campaign.chrysalis_unit, drone = side_drone(), guard = 0;
        int other_drone = -1;
        static const int pickup_route[][2] = {{320,288},{320,160},{160,160},{168,64}};
        static const int escort_route[][4] = {
            {168,72,168,72},{264,136,264,136},{296,200,296,200},
            {216,280,216,280},{200,424,200,424},{200,584,200,584},
            {152,696,152,696},{216,792,216,792},{360,808,360,808},
            {456,744,456,744},{504,856,504,856},{584,936,584,936},
            {744,936,744,936},{856,888,856,888},{984,856,984,856},
            {1144,856,1144,856},{1256,904,1256,904},{1352,968,1352,968},
            {1432,1048,1432,1048},{1320,1096,1320,1096},{1208,1144,1208,1144},
            {1208,1240,1208,1240}
        };
        if (c >= MAX_UNITS || !u[c].alive || drone < 0) return 0;
        for (i = 0; i < MAX_UNITS; ++i)
            if (i != drone && u[i].alive && u[i].owner == SIDE && u[i].type == U_DRONE) { other_drone = i; break; }
        /* Keep the opening combat group near the carrier so it can serve as
         * an actual escort; moving it to the old staging waypoint left the
         * Drone exposed on the long source route. */
        controller_press(KEY_R);
        if (other_drone >= 0) {
            if (controller_select_worker(other_drone)) {
                if (!controller_move_selected(440,360)) return 0;
                for (i = 0; i < 2000 && screen == SCREEN_PLAY && !lost &&
                     dist2(u[other_drone].x,u[other_drone].y,440,360) > 20*20; ++i) controller_frame(0);
            }
        }
        if (!controller_select_worker(drone)) {
            fprintf(stderr, "Z2 carrier selection failed: drone=%d alive=%d owner=%d pos=%d,%d cursor=%d,%d order=%d selected=%d building=%d\n",
                    drone, u[drone].alive, u[drone].owner, u[drone].x, u[drone].y,
                    cx, cy, u[drone].order, u[drone].selected, selected_building);
            return 0;
        }
        for (guard = 0; guard < (int)(sizeof(pickup_route)/sizeof(pickup_route[0])) &&
             race_campaign.carrier_unit >= MAX_UNITS; ++guard) {
            if (!controller_move_selected(pickup_route[guard][0],pickup_route[guard][1])) return 0;
            for (i = 0; i < 2500 && screen == SCREEN_PLAY && !lost &&
                 race_campaign.carrier_unit >= MAX_UNITS &&
                 dist2(u[drone].x,u[drone].y,pickup_route[guard][0],pickup_route[guard][1]) > 10*10;
                 ++i) controller_frame(0);
        }
        if (race_campaign.carrier_unit >= MAX_UNITS) return 0;
        for (guard = 0; guard < (int)(sizeof(escort_route)/sizeof(escort_route[0])) &&
             race_campaign.carrier_unit < MAX_UNITS && !won && !lost; ++guard) {
            int carrier = race_campaign.carrier_unit, cannon, clears = 0;
            /* Two Photon Cannon hits kill the 40 HP Drone, and the chokepoint
             * around leg 12 has no route outside their range. Clear the cannons
             * covering the next walk while the carrier waits out of range. */
            while ((cannon = enemy_cannon_covering_walk(u[carrier].x,u[carrier].y,
                                                        escort_route[guard][2],escort_route[guard][3])) >= 0 &&
                   clears++ < 3 && screen == SCREEN_PLAY && !won && !lost) {
                controller_press(KEY_R);
                if (!controller_attack_order(b[cannon].x,b[cannon].y)) return 0;
                for (i = 0; i < 900 && b[cannon].alive && screen == SCREEN_PLAY && !won && !lost; ++i)
                    controller_frame(0);
            }
            controller_press(KEY_R);
            if (!controller_attack_order(escort_route[guard][0],escort_route[guard][1])) return 0;
            if (!controller_select_worker(carrier)) {
                fprintf(stderr, "Z2 carrier selection failed: drone=%d alive=%d owner=%d pos=%d,%d cursor=%d,%d order=%d selected=%d building=%d\n",
                        carrier, u[carrier].alive, u[carrier].owner, u[carrier].x, u[carrier].y,
                        cx, cy, u[carrier].order, u[carrier].selected, selected_building);
                return 0;
            }
            if (!controller_move_selected(escort_route[guard][2],escort_route[guard][3])) return 0;
            for (i = 0; i < 20000 && race_campaign.carrier_unit < MAX_UNITS &&
                 screen == SCREEN_PLAY && !won && !lost &&
                 (dist2(u[race_campaign.carrier_unit].x,u[race_campaign.carrier_unit].y,
                        escort_route[guard][2],escort_route[guard][3]) > 36*36); ++i) controller_frame(0);
            if (race_campaign.carrier_unit >= MAX_UNITS || !u[carrier].alive) {
                fprintf(stderr, "Z2 convoy stopped at leg %d tick=%lu carrier=%d alive=%d pos=%d,%d hp=%d order=%d target=%d,%d path=%d,%d retry=%d timer=%d chrysalis=%d,%d; escort hunters near=%d\n",
                        guard, controller_ticks, carrier, u[carrier].alive, u[carrier].x, u[carrier].y,
                        u[carrier].hp, u[carrier].order, u[carrier].tx, u[carrier].ty,
                        u[carrier].path_x, u[carrier].path_y, u[carrier].path_retry, u[carrier].path_timer,
                        u[c].x, u[c].y,
                        side_hunters_near(escort_route[guard][0],escort_route[guard][1],96));
                for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == ENEMY &&
                    dist2(u[j].x,u[j].y,u[c].x,u[c].y) < 180*180)
                    fprintf(stderr, "  nearby enemy slot=%d type=%d at=%d,%d hp=%d\n",
                            j,u[j].type,u[j].x,u[j].y,u[j].hp);
                for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
                    u[j].type == U_HUNTER_KILLER)
                    fprintf(stderr, "  escort slot=%d at=%d,%d order=%d target=%d,%d hp=%d\n",
                            j,u[j].x,u[j].y,u[j].order,u[j].tx,u[j].ty,u[j].hp);
                break;
            }
        }
        return won && !lost;
    }
    if (mission == SC64_RACE_MISSION_P2) {
        /* Waiting out the 900-second trigger and attack-moving the escorts once
         * only won while idle units could acquire targets through fog (#133).
         * The town holds about 100 units and 14 Sunken Colonies, so build a base
         * and army first, then press the assault with repeated attack orders.
         * Fenix keeps to his arrival point because his death loses the mission. */
        unsigned long next_economy = 0, next_order = 0;
        int assaulting = 0;
        while (controller_playing() && controller_ticks < 150000) {
            int fenix = race_campaign.rescue_unit;
            if (controller_ticks >= next_economy) {
                controller_protoss_economy(4, 12);
                next_economy = controller_ticks + 120;
            }
            if (!controller_playing()) break;
            /* An earlier assault trades worse (sampled over 72 start offsets):
             * the army is smaller and arrives well before the escorts do. */
            if (controller_ticks >= 24000) assaulting = 1;
            if (!assaulting && controller_ticks >= next_order) {
                controller_defend_base();
                next_order = controller_ticks + 600;
            }
            if (assaulting && controller_ticks >= next_order && fenix < MAX_UNITS &&
                !side_army_size(fenix) && controller_last_stand()) {
                next_order = controller_ticks + 900;
            } else if (assaulting && controller_ticks >= next_order && side_army_size(-1) > 0) {
                int fenix_side = fenix < MAX_UNITS && u[fenix].alive && u[fenix].owner == SIDE;
                target = controller_nearest_enemy_building(fenix_side ? fenix : -1);
                /* Fenix joins when an escort passes him. Until then R leaves
                 * him out; afterwards a box selects around him. */
                controller_normal_ui();
                if (!fenix_side) controller_select_army();
                else if (!controller_select_combat_except(fenix)) target = -1;
                if (target >= 0 && selected_count() > 0)
                    (void)controller_attack_order(b[target].x, b[target].y);
                controller_normal_ui();
                next_order = controller_ticks + 900;
            }
            controller_frame(0);
        }
        return won && !lost;
    }
    if (mission == SC64_RACE_MISSION_P1) {
        int fenix = race_campaign.rescue_unit;
        /* Original setup places the player force opposite Antioch. Route it
         * into the marked Rescue Passive base before crossing toward Zerg.
         * Park Fenix at the captured base before the assault to protect the
         * source survival objective. */
        controller_press(KEY_R);
        if (!controller_move_selected(504, 1024)) return 0;
        for (i = 0; i < 30000 && screen == SCREEN_PLAY && !won && !lost; ++i) {
            int k, remaining = 0;
            controller_frame(0);
            for (k = 0; k < MAX_BUILDINGS; ++k)
                if (b[k].alive && b[k].owner == RESCUABLE && b[k].x >= 224 && b[k].x <= 784 &&
                    b[k].y >= 848 && b[k].y <= 1200) ++remaining;
            if (remaining == 0 && u[race_campaign.rescue_unit].owner == SIDE) break;
        }
        if (fenix >= MAX_UNITS || !u[fenix].alive || u[fenix].owner != SIDE) return 0;
    }
    /* For the destruction missions, choose the closest extant enemy
     * structure to the surviving force and reissue a controller attack order
     * while it remains. State reads only select a route goal. */
    if (mission == SC64_RACE_MISSION_P1) {
        /* The rescue leaves the force over its supply cap, so nothing can be
         * trained until the assault has spent some of it. Send the force first:
         * idle units do not step aside for Fenix. Then walk him behind the Nexus,
         * away from the ramp the Zerg raids come through. */
        int fenix = race_campaign.rescue_unit, fenix_sent = 0;
        while (controller_playing() && controller_ticks < 150000) {
            int attacked;
            int wait_ticks;
            if (!side_army_size(fenix)) {
                /* Rebuild from the base rather than conceding the mission. */
                controller_protoss_economy(2, 6);
                (void)controller_last_stand();
                for (wait_ticks = 0; wait_ticks < 240 && controller_playing(); ++wait_ticks)
                    controller_frame(0);
                continue;
            }
            attacked = controller_nearest_enemy_building(fenix);
            if (attacked < 0) break;
            controller_normal_ui();
            if (!controller_select_combat_except(fenix) ||
                !controller_attack_order(b[attacked].x, b[attacked].y)) {
                controller_normal_ui();
                for (wait_ticks = 0; wait_ticks < 60 && controller_playing(); ++wait_ticks)
                    controller_frame(0);
                continue;
            }
            controller_normal_ui();
            for (wait_ticks = 0; wait_ticks < 120 && controller_playing(); ++wait_ticks)
                controller_frame(0);
            if (!fenix_sent && controller_select_actor(fenix) &&
                controller_move_selected(330, 1172)) fenix_sent = 1;
            controller_normal_ui();
            for (wait_ticks = 0; wait_ticks < 780 && controller_playing() && b[attacked].alive;
                 ++wait_ticks) {
                if (wait_ticks % 240 == 0) controller_protoss_economy(2, 6);
                controller_frame(0);
            }
        }
    } else
    for (i = 0; i < 100 && screen == SCREEN_PLAY && !won && !lost; ++i) {
        int j, k, closest = 0x7fffffff, alive_force = 0;
        for (j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
            !sc64_race_is_worker(u[j].type)) ++alive_force;
        if (!alive_force) break;
        target = -1;
        for (j = 0; j < MAX_BUILDINGS; ++j) if (b[j].alive && b[j].owner == ENEMY) {
            int d = 0x7fffffff;
            for (k = 0; k < MAX_UNITS; ++k) if (u[k].alive && u[k].owner == SIDE &&
                !sc64_race_is_worker(u[k].type)) {
                int q = dist2(u[k].x,u[k].y,b[j].x,b[j].y);
                if (q < d) d = q;
            }
            if (d < closest) { closest = d; target = j; }
        }
        if (target < 0) break;
        if (!controller_attack(b[target].x, b[target].y, 6000)) return 0;
    }
    /* The campaign tick sees the last building die on a later update. */
    for (i = 0; i < 90 && controller_playing(); ++i) controller_frame(0);
    return won && !lost;
}

static int
side_unit(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type)) return i;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) return i;
    return -1;
}

static int
side_drone(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_DRONE) return i;
    return -1;
}

static int
side_hunters_near(int x, int y, int radius)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        u[i].type == U_HUNTER_KILLER && dist2(u[i].x,u[i].y,x,y) <= radius * radius) ++n;
    return n;
}

static void
start_mission(int mission)
{
    race_mode = 0; /* Campaign profile; 1 selects skirmish. */
    race_selected_mission = mission;
    race_completed = 0;
    screen = SCREEN_TITLE;
    menu_open = 1; menu = 4; menu_cursor = 3; help = 1;
    selected_building = 0; train_type = U_MARINE;
    mode = MODE_ATTACK; enemy_timer = 999; wave = 9;
    box_state = 2; box_suppress_a = 1;
    init_state();
}

static void
clear_enemy_buildings(void)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == ENEMY) b[i].alive = 0;
}

static int
fail(const char *message)
{
    fprintf(stderr, "Race campaign: %s\n", message);
    return 1;
}

int
main(int argc, char **argv)
{
    int i, c, f, actor;
    if (argc >= 3 && strcmp(argv[1], "--controller") == 0) {
        int mission = atoi(argv[2]), controller_result, arg;
        if (mission < 0 || mission >= 4) return 64;
        for (arg = 3; arg + 1 < argc; arg += 2) {
            if (strcmp(argv[arg], "--trace") == 0) race_trace_path = argv[arg + 1];
            else if (strcmp(argv[arg], "--start-delay") == 0) controller_start_delay = atoi(argv[arg + 1]);
            else return 64;
        }
        if (arg != argc || controller_start_delay < 0) return 64;
        controller_result = controller_mission(mission);
        if (race_event_overflow) {
            race_trace_save();
            fprintf(stderr, "Controller trace input event overflow for mission %d (capacity=%lu).\n",
                    mission, (unsigned long)(sizeof(race_events)/sizeof(race_events[0])));
            return 65;
        }
        if (!controller_result) {
            race_trace_save();
            int i, side_units = 0, enemy_buildings = 0;
            for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) ++side_units;
            for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == ENEMY) ++enemy_buildings;
            fprintf(stderr, "Controller route did not complete mission %d (ticks=%lu, won=%d, lost=%d, phase=%d).\n",
                    mission, controller_ticks, won, lost, race_campaign.phase);
            fprintf(stderr, "Remaining side units=%d enemy buildings=%d minerals=%d camera=%d,%d selected=%d mode=%d cursor=%d,%d\n",
                    side_units, enemy_buildings, minerals, cx, cy, selected_count(), mode, cx, cy);
            if (mission == SC64_RACE_MISSION_Z2 && race_campaign.chrysalis_unit < MAX_UNITS) {
                int c = race_campaign.chrysalis_unit;
                fprintf(stderr, "Chrysalis alive=%d owner=%d at %d,%d carrier=%u; side units:",u[c].alive,u[c].owner,u[c].x,u[c].y,race_campaign.carrier_unit);
                for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE)
                    fprintf(stderr," %d:type%d@%d,%d/o%d t%d,%d p%d r%d",i,u[i].type,u[i].x,u[i].y,u[i].order,u[i].tx,u[i].ty,u[i].path_timer,u[i].path_retry);
                fputc('\n',stderr);
            }
            if (mission == SC64_RACE_MISSION_P2) {
                int j;
                fprintf(stderr,"P2 reinforcements=%x Fenix=%u/%d cerebrates:",race_campaign.objective_flags,
                        race_campaign.rescue_unit,race_campaign.rescue_unit<MAX_UNITS?u[race_campaign.rescue_unit].alive:0);
                for(j=0;j<MAX_BUILDINGS;++j)if(b[j].alive&&b[j].owner==ENEMY&&b[j].raw_type==151)
                    fprintf(stderr," %d@%d,%d hp%d",j,b[j].x,b[j].y,b[j].hp);
                fprintf(stderr,"; survivors:");
                for(j=0;j<MAX_UNITS;++j)if(u[j].alive&&u[j].owner==SIDE)
                    fprintf(stderr," %d/t%d@%d,%d o%d",j,u[j].type,u[j].x,u[j].y,u[j].order);
                fputc('\n',stderr);
            }
            if (mission == SC64_RACE_MISSION_P1) {
                for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE)
                    fprintf(stderr, "P1 surviving side building slot=%d type=%d at=%d,%d complete=%d under=%d progress=%d queue=%d powered=%d\n",
                            i,b[i].type,b[i].x,b[i].y,building_complete(i),b[i].under_construction,
                            b[i].build_progress,b[i].queue_count,
                            b[i].type == B_GATEWAY ? race_site_powered(SIDE,b[i].x,b[i].y) : 0);
                for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == ENEMY)
                    fprintf(stderr, "P1 remaining enemy building slot=%d type=%d raw=%d at=%d,%d hp=%d\n",
                            i,b[i].type,b[i].raw_type,b[i].x,b[i].y,b[i].hp);
                for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE)
                    fprintf(stderr, "P1 surviving side unit slot=%d type=%d at=%d,%d hp=%d/%d order=%d target=%d,%d selected=%d\n",
                            i,u[i].type,u[i].x,u[i].y,u[i].hp,u[i].maxhp,u[i].order,u[i].tx,u[i].ty,u[i].selected);
            }
            return 20 + mission;
        }
        race_trace_save();
        printf("Controller completed race mission %d in %lu input frames.\n", mission, controller_ticks);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--controller-all") == 0) {
        for (i = 0; i < 4; ++i) {
            int controller_result = controller_mission(i);
            if (race_event_overflow) {
                fprintf(stderr, "Controller input event overflow for mission %d (capacity=%lu).\n",
                        i, (unsigned long)(sizeof(race_events)/sizeof(race_events[0])));
                return 65;
            }
            if (!controller_result) {
                fprintf(stderr, "Controller route did not complete mission %d (ticks=%lu, won=%d, lost=%d, phase=%d).\n",
                        i, controller_ticks, won, lost, race_campaign.phase);
                return 20 + i;
            }
        }
        puts("Controller completed all four race missions.");
        return 0;
    }
    if (argc != 1) return 64;
    game_init();
    if (SC64_RACE_MAP_COUNT < 4) return fail("four authored maps available");

    start_mission(SC64_RACE_MISSION_Z1);
    if (screen != SCREEN_PLAY || menu_open || menu || menu_cursor || help ||
        selected_building != -1 || mode != MODE_NORMAL || enemy_timer || wave ||
        box_state || box_suppress_a || race_cols != 64 || race_rows != 64 || race_human != R_ZERG ||
        race_campaign.chrysalis_unit >= MAX_UNITS ||
        !u[race_campaign.chrysalis_unit].alive)
        return fail("Z1 imports Zerg ownership and protected Chrysalis");
    spawn_build(SIDE, B_SPAWNING_POOL, 350, 800, 0);
    race_campaign_tick();
    if (race_campaign_won()) return fail("Z1 waits for the Hydralisk Den");
    spawn_build(SIDE, B_HYDRALISK_DEN, 370, 800, 0);
    clear_enemy_buildings();
    race_campaign_tick();
    if (!race_campaign_won()) return fail("Z1 wins on tech objectives and buildings; enemy units may remain");

    start_mission(SC64_RACE_MISSION_Z2);
    if (race_cols != 96 || race_rows != 96 || race_human != R_ZERG)
        return fail("Z2 uses its 96 by 96 map and Zerg ownership");
    c = race_campaign.chrysalis_unit;
    actor = side_drone();
    if (c >= MAX_UNITS || actor < 0 || u[c].owner != NEUTRAL)
        return fail("Z2 begins with a recoverable Chrysalis and controlled Drone");
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == RESCUABLE && u[i].raw_type == 53) break;
    if (i >= MAX_UNITS) return fail("Z2 imports the six source Rescue Passive Hunter Killers");
    u[actor].x = u[i].x; u[actor].y = u[i].y;
    race_campaign_tick();
    if (side_hunters_near(u[i].x,u[i].y,256) < 4)
        return fail("Z2 transfers the nearby Rescue Passive Hunter Killer force");
    u[actor].x = u[c].x; u[actor].y = u[c].y;
    race_campaign_tick();
    if (u[c].owner != SIDE || race_campaign.carrier_unit != actor)
        return fail("Z2 Drone can pick up the Chrysalis");
    u[actor].x = 1208; u[actor].y = 1216;
    race_campaign_tick();
    if (!race_campaign_won()) return fail("Z2 wins at the Warp Point with enemy forces alive");

    start_mission(SC64_RACE_MISSION_P1);
    if (race_cols != 96 || race_rows != 96 || race_human != R_PROTOSS)
        return fail("P1 uses Protoss ownership and map dimensions");
    f = race_campaign.rescue_unit;
    actor = side_unit();
    i = 0;
    for (c = 0; c < MAX_BUILDINGS; ++c)
        if (b[c].alive && b[c].owner == RESCUABLE) ++i;
    if (f >= MAX_UNITS || actor < 0 || u[f].owner != RESCUABLE)
        return fail("P1 begins with a rescuable Fenix");
    u[actor].x = u[f].x; u[actor].y = u[f].y;
    race_campaign_tick();
    if (u[f].owner != SIDE) return fail("P1 can meet and recruit Fenix");
    if (i && race_campaign_count_buildings(SIDE) == 0)
        return fail("P1 recruits the Rescue Passive Protoss base when entering Antioch");
    clear_enemy_buildings();
    race_campaign_tick();
    if (!race_campaign_won()) return fail("P1 requires Fenix and the Zerg base objective");

    start_mission(SC64_RACE_MISSION_P2);
    if (race_cols != 96 || race_rows != 64 || race_human != R_PROTOSS)
        return fail("P2 uses its 96 by 64 Protoss map");
    race_campaign.elapsed_frames = 900u * 30u - 1;
    race_campaign_tick();
    if (!(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY) ||
        race_campaign.rescue_unit >= MAX_UNITS || !u[race_campaign.rescue_unit].alive)
        return fail("P2 adds Fenix only after the source 15 minute timer");
    f = race_campaign.rescue_unit;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == ENEMY && b[i].raw_type == 151) b[i].alive = 0;
    race_campaign_tick();
    if (!race_campaign_won()) return fail("P2 completes by destroying the Cerebrate while Fenix lives");

    start_mission(SC64_RACE_MISSION_P2);
    race_campaign.elapsed_frames = 900u * 30u - 1;
    race_campaign_tick();
    f = race_campaign.rescue_unit;
    u[f].alive = 0;
    race_campaign_tick();
    if (!race_campaign_lost()) return fail("P2 fails if Fenix dies");

    puts("Race campaign objectives preserve the required targets and mission-specific wins.");
    return 0;
}
