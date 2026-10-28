/* Controller-goal coverage for the private four-map race campaign profile.
 * Compile with SC64_RACES and its ignored .local generated headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

static u16 controller_previous;
static unsigned long controller_ticks;
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
    while ((abs(cx - x) > 3 || abs(cy - y) > 3) && guard++ < 3000) {
        u16 h = 0; /* R selects a group, so do not hold it during orders. */
        if (abs(cx - x) > 3) { if (cx < x) h |= KEY_RIGHT; else h |= KEY_LEFT; }
        if (abs(cy - y) > 3) { if (cy < y) h |= KEY_DOWN; else h |= KEY_UP; }
        controller_frame(h);
    }
    controller_frame(0);
    if (guard >= 3000) fprintf(stderr, "cursor guard target=%d,%d actual=%d,%d\n",x,y,cx,cy);
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
           screen == SCREEN_PLAY && !won && !lost && steps++ < 120) {
        u16 held = KEY_R;
        if (cx < u[worker].x) held |= KEY_RIGHT;
        if (cx > u[worker].x) held |= KEY_LEFT;
        if (cy < u[worker].y) held |= KEY_DOWN;
        if (cy > u[worker].y) held |= KEY_UP;
        controller_frame(held);
    }
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

static int controller_train_at_building(int building, int count)
{
    int attempt, n;
    static const int offsets[][2] = {{0,0},{-16,0},{16,0},{0,-16},{0,16},
                                    {-16,-16},{16,-16},{-16,16},{16,16}};
    if (building < 0 || building >= MAX_BUILDINGS || !building_complete(building) ||
        b[building].owner != SIDE || count < 1 || count > 5) return 0;
    for (attempt = 0; attempt < (int)(sizeof(offsets)/sizeof(offsets[0])); ++attempt) {
        controller_cursor(b[building].x + offsets[attempt][0],
                          b[building].y + offsets[attempt][1]);
        controller_press(KEY_A);
        if (selected_building == building) break;
    }
    if (selected_building != building) return 0;
    for (n = 0; n < count; ++n) controller_press(KEY_A);
    return b[building].queue_count == count;
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
           screen == SCREEN_PLAY && !won && !lost && steps++ < 200) {
        u16 held = KEY_R;
        if (cx < u[worker].x) held |= KEY_RIGHT;
        if (cx > u[worker].x) held |= KEY_LEFT;
        if (cy < u[worker].y) held |= KEY_DOWN;
        if (cy > u[worker].y) held |= KEY_UP;
        controller_frame(held);
    }
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

static int controller_build_pylon_near(int gateway)
{
    int probe = -1, i, slot;
    static const int offsets[][2] = {{-96,0},{-64,-64},{-64,64},{0,96},
                                    {0,-96},{96,0},{48,48},{-48,-48}};
    if (gateway < 0 || gateway >= MAX_BUILDINGS || !b[gateway].alive || minerals < 100)
        return -1;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        u[i].type == U_PROBE && u[i].order != ORDER_BUILD) { probe = i; break; }
    if (probe < 0 || !controller_select_worker_box(probe)) return -1;
    controller_press(KEY_B);
    for (i = 0; i < MENU_COUNT && menu_cursor != MENU_DEPOT; ++i)
        controller_press(KEY_DOWN);
    if (!menu_open || menu_cursor != MENU_DEPOT) return -1;
    controller_press(KEY_A);
    if (mode != MODE_BUILD || menu != B_PYLON) return -1;
    for (i = 0; i < (int)(sizeof(offsets)/sizeof(offsets[0])); ++i) {
        int x = b[gateway].x + offsets[i][0], y = b[gateway].y + offsets[i][1];
        controller_cursor(x, y);
        controller_press(KEY_A);
        if (mode == MODE_NORMAL) {
            for (slot = 0; slot < MAX_BUILDINGS; ++slot) if (b[slot].alive &&
                b[slot].owner == SIDE && b[slot].type == B_PYLON &&
                dist2(b[slot].x, b[slot].y, x, y) < 32 * 32) return slot;
            return -1;
        }
    }
    return -1;
}

static int controller_p1_reinforce(void)
{
    int nexus = -1, gateway = -1, i, probe = -1, pylon;
    if (race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE) < 2 || minerals < 50)
        return 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE &&
        building_complete(i)) {
        if (b[i].type == B_NEXUS && nexus < 0) nexus = i;
        if (b[i].type == B_GATEWAY && gateway < 0) gateway = i;
    }
    if (nexus < 0 || gateway < 0 || !controller_train_at_building(nexus, 1)) return 0;
    for (i = 0; i < 700 && screen == SCREEN_PLAY && !won && !lost && b[nexus].queue_count;
         ++i) controller_frame(0);
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        u[i].type == U_PROBE && u[i].order != ORDER_BUILD) { probe = i; break; }
    if (probe < 0) return 0;
    pylon = controller_build_pylon_near(gateway);
    if (pylon < 0) return 0;
    for (i = 0; i < 1800 && screen == SCREEN_PLAY && !won && !lost &&
         b[pylon].under_construction; ++i) controller_frame(0);
    if (!building_complete(pylon) || !race_site_powered(SIDE, b[gateway].x, b[gateway].y) ||
        race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE) < 4 || minerals < 100 ||
        !controller_train_at_building(gateway, 1)) return 0;
    for (i = 0; i < 900 && screen == SCREEN_PLAY && !won && !lost && b[gateway].queue_count;
         ++i) controller_frame(0);
    return b[gateway].queue_count == 0;
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
    controller_cursor(left, top);
    if (screen != SCREEN_PLAY || won || lost || abs(cx - left) > 3 || abs(cy - top) > 3)
        return 0;
    controller_frame(KEY_L);
    while (cx < right || cy < bottom) {
        u16 held = KEY_L;
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
    if (screen != SCREEN_PLAY || won || lost || abs(cx - right) > 3 || abs(cy - bottom) > 3)
        return 0;
    if (selected_count() <= 1 || (excluded >= 0 && excluded < MAX_UNITS && u[excluded].selected))
        fprintf(stderr, "P1 box bounds=%d,%d..%d,%d anchor=%d,%d end=%d,%d state=%d cursor=%d,%d selected=%d excluded=%d\n",
                left, top, right, bottom, box_anchor_x, box_anchor_y,
                box_end_x, box_end_y, box_state, cx, cy, selected_count(), excluded);
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

static int controller_mission(int mission)
{
    int i, target = -1, p1_fenix_parked = 0, p1_reinforcement_attempted = 0;
    controller_previous = 0; controller_ticks = 0;
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
            int carrier = race_campaign.carrier_unit;
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
        /* Let the 900-second trigger run through ordinary game frames. */
        for (i = 0; i < 900 * 30 + 10 && screen == SCREEN_PLAY && !lost; ++i) controller_frame(0);
        if (lost || !(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY)) return 0;
        for (i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == ENEMY && b[i].raw_type == 151) { target = i; break; }
        if (target < 0 || !controller_attack(b[target].x, b[target].y, 100000)) return 0;
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
        if (fenix >= MAX_UNITS || !u[fenix].alive || u[fenix].owner != SIDE ||
            !controller_select_actor(fenix) || !controller_move_selected(780, 1180)) return 0;
        for (i = 0; i < 3000 && screen == SCREEN_PLAY && !won && !lost &&
             dist2(u[fenix].x, u[fenix].y, 780, 1180) > 24 * 24; ++i) controller_frame(0);
        if (!u[fenix].alive || dist2(u[fenix].x, u[fenix].y, 780, 1180) > 24 * 24 ||
            !controller_select_combat_except(fenix)) return 0;
        p1_fenix_parked = 1;
    }
    /* For the destruction missions, choose the closest extant enemy
     * structure to the surviving force and reissue a controller attack order
     * while it remains. State reads only select a route goal. */
    for (i = 0; i < 100 && screen == SCREEN_PLAY && !won && !lost; ++i) {
        int j, k, closest = 0x7fffffff, alive_force = 0;
        int protected_actor = mission == SC64_RACE_MISSION_P1 && p1_fenix_parked ?
                              (int)race_campaign.rescue_unit : -1;
        if (mission == SC64_RACE_MISSION_P1 && !p1_reinforcement_attempted &&
            race_owner_supply_cap(SIDE) - race_owner_reserved_supply(SIDE) >= 2 && minerals >= 50) {
            int reinforced;
            p1_reinforcement_attempted = 1;
            reinforced = controller_p1_reinforce();
            if (reinforced) fprintf(stderr, "P1 ordinary Nexus/Probe/Pylon/Gateway reinforcement completed at tick %lu\n",
                                    controller_ticks);
            if (!controller_select_combat_except(protected_actor)) return 0;
        }
        for (j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
            j != protected_actor && !sc64_race_is_worker(u[j].type)) ++alive_force;
        if (!alive_force) break;
        target = -1;
        for (j = 0; j < MAX_BUILDINGS; ++j) if (b[j].alive && b[j].owner == ENEMY) {
            int d = 0x7fffffff;
            for (k = 0; k < MAX_UNITS; ++k) if (k != protected_actor &&
                u[k].alive && u[k].owner == SIDE &&
                !sc64_race_is_worker(u[k].type)) {
                int q = dist2(u[k].x,u[k].y,b[j].x,b[j].y);
                if (q < d) d = q;
            }
            if (d < closest) { closest = d; target = j; }
        }
        if (target < 0) break;
        if (mission == SC64_RACE_MISSION_P1) {
            int attacked = target, wait_ticks;
            if (!controller_attack_order(b[attacked].x, b[attacked].y)) {
                fprintf(stderr, "P1 attack order failed target=%d at=%d,%d mode=%d menu=%d cursor=%d,%d selected=%d ticks=%lu\n",
                        attacked, b[attacked].x, b[attacked].y, mode,
                        menu_open, cx, cy, selected_count(), controller_ticks);
                return 0;
            }
            for (wait_ticks = 0; wait_ticks < 6000 && screen == SCREEN_PLAY && !won && !lost &&
                 b[attacked].alive; ++wait_ticks) {
                controller_frame(0);
            }
        } else if (!controller_attack(b[target].x, b[target].y, 6000)) return 0;
    }
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
    if ((argc == 3 || argc == 5) && strcmp(argv[1], "--controller") == 0) {
        int mission = atoi(argv[2]), controller_result;
        if (mission < 0 || mission >= 4) return 64;
        if (argc == 5 && strcmp(argv[3], "--trace") != 0) return 64;
        race_trace_path = argc == 5 ? argv[4] : NULL;
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
