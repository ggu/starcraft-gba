/* Normal-controller Z5 playthrough attempt. All gameplay mutations are
 * delivered through game_update input; runtime state is observed only for
 * steering and diagnostics. A win is promoted only from SCREEN_RESULT. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define Z5_RUN_LIMIT 60000
#define CHECKPOINT_PERIOD 900
#define RETREAT_HP 340
#define RETREAT_ENERGY_Q8 (100 * 256)
#define Kerrigan_RAW 51
#define TARGET_STALL_LIMIT 900

static u16 prior_keys;
static u32 host_ticks;
static FILE *trace;
static FILE *replay_trace;
static int trace_first = 1;
static int replay_trace_first = 1;
static int replay_last_keys = -1;
static int run_limit = Z5_RUN_LIMIT;
static int deployed;
static int order_count;
static int controller_retreat_order;
static int controller_retreat_requested;
static int cloak_controller_busy;
static int cloak_controller_preloaded;
static int consume_controller_busy;
static int consume_controller_count;
static int controller_target_hero_only;
static unsigned target_last_progress_frame;
static int target_progress_x = -1;
static int target_progress_y = -1;
static u16 nav_component[MAP_CELLS];
static u16 nav_queue[MAP_CELLS];
static u32 blocked_triggers_lo;
static u32 blocked_triggers_hi;
static u32 observed_started_lo;
static u32 observed_started_hi;
static unsigned controller_target_trigger_debug = SC64_Z5_TRIGGER_COUNT;
static int controller_source_goal_x = -1;
static int controller_source_goal_y = -1;
static int controller_recovery_waypoint_x = 1696;
static int controller_recovery_waypoint_y = 115;
static int kerrigan_slot(void);

static void replay_trace_prefix(void)
{
    if (!replay_trace_first) fputs(",\n", replay_trace);
    replay_trace_first = 0;
}

static unsigned z5_fired_count(void)
{
    unsigned count = 0;
    for (unsigned i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i)
        count += (unsigned)sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                                             race_z5.runtime.model.started_hi,
                                             (u8)i);
    return count;
}

static void replay_state_event(const char *kind)
{
    int k = kerrigan_slot();
    replay_trace_prefix();
    fprintf(replay_trace,
        "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,"
        "\"screen\":%d,\"won\":%d,\"lost\":%d,\"outcome\":%u,"
        "\"fired\":%llu,\"started_lo\":%u,\"started_hi\":%u,"
        "\"action_cursors\":[",
        kind, (unsigned)host_ticks, (unsigned)frame, screen, won, lost,
        (unsigned)race_z5.runtime.model.outcome,
        (unsigned long long)race_z5.runtime.model.started_lo |
            ((unsigned long long)race_z5.runtime.model.started_hi << 32),
        (unsigned)race_z5.runtime.model.started_lo,
        (unsigned)race_z5.runtime.model.started_hi);
    for (unsigned i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i)
        fprintf(replay_trace, "%s%u", i ? "," : "",
                (unsigned)race_z5.runtime.model.action_cursor[i]);
    fputs("],\"action_ticks\":[", replay_trace);
    for (unsigned i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i)
        fprintf(replay_trace, "%s%u", i ? "," : "",
                (unsigned)race_z5.runtime.action_ticks[i]);
    fprintf(replay_trace,
        "],\"switch_bits\":[%u,%u,%u,%u,%u,%u,%u,%u],"
        "\"switch110\":%u,\"source_paused\":%u,\"pause_ack\":%u,"
        "\"fault\":%u,\"cursor_x\":%d,\"cursor_y\":%d,"
        "\"selected_units\":%d,\"selected_building\":%d,"
        "\"hero\":{\"slot\":%d,\"alive\":%d,\"raw\":%d,"
        "\"source_owner\":%d,\"owner\":%d,\"hp\":%d,\"shield\":%d,\"energy_q8\":%d,"
        "\"x\":%d,\"y\":%d,\"order\":%d,\"target_x\":%d,"
        "\"target_y\":%d,\"cloak\":%u},\"orders\":%d}",
        (unsigned)race_z5.runtime.switch_bits[0],
        (unsigned)race_z5.runtime.switch_bits[1],
        (unsigned)race_z5.runtime.switch_bits[2],
        (unsigned)race_z5.runtime.switch_bits[3],
        (unsigned)race_z5.runtime.switch_bits[4],
        (unsigned)race_z5.runtime.switch_bits[5],
        (unsigned)race_z5.runtime.switch_bits[6],
        (unsigned)race_z5.runtime.switch_bits[7],
        (unsigned)((race_z5.runtime.switch_bits[3] >> 14) & 1u),
        (unsigned)race_z5.runtime.source_paused,
        (unsigned)race_z5.pause_acknowledged,
        (unsigned)race_z5.runtime.fault, cx, cy, selected_count(),
        selected_building, k, k >= 0 && u[k].alive,
        k >= 0 ? u[k].raw_type : -1, k >= 0 ? u[k].raw_owner : -1,
        k >= 0 ? u[k].owner : -1,
        k >= 0 ? u[k].hp : 0, k >= 0 ? u[k].shield : 0,
        k >= 0 ? u[k].carry : 0, k >= 0 ? u[k].x : -1,
        k >= 0 ? u[k].y : -1, k >= 0 ? u[k].order : -1,
        k >= 0 ? u[k].tx : -1, k >= 0 ? u[k].ty : -1,
        (unsigned)race_z5.cloak.active, order_count);
    fflush(replay_trace);
}

static void manage_kerrigan_cloak(void);
static void move_cursor(int x, int y);
static int kerrigan_near_detector(void);

static void trace_prefix(void)
{
    if (!trace_first) fputs(",\n", trace);
    trace_first = 0;
}

static int kerrigan_slot(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == Kerrigan_RAW &&
            u[i].owner == SIDE) return i;
    return -1;
}

static int owner_units(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].owner == owner;
    return count;
}

static int owner_buildings(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        count += b[i].alive && b[i].owner == owner;
    return count;
}

static int route_crosses_detector(int x0, int y0, int x1, int y1,
                                 int *detector_x, int *detector_y)
{
    int dx = x1 - x0, dy = y1 - y0;
    int64_t length2 = (int64_t)dx * dx + (int64_t)dy * dy;
    int best = -1;
    if (!length2) return 0;
    for (unsigned row = 0; row < race_z5_world.doodad_count; ++row) {
        unsigned slot = race_z5_doodads[row].building_slot;
        const Sc64Z5BuildingFact *fact;
        int px, py, qx, qy, ddx, ddy, sight, d2;
        if (!race_z5_doodads[row].enabled || slot >= MAX_BUILDINGS ||
            !b[slot].alive || !sc64_z5_detector_raw(b[slot].raw_type)) continue;
        fact = sc64_z5_building_fact(b[slot].raw_type);
        if (!fact || fact->sight_px <= 0) continue;
        px = b[slot].x; py = b[slot].y;
        int64_t projection = (int64_t)(px - x0) * dx +
                             (int64_t)(py - y0) * dy;
        if (projection < 0) projection = 0;
        if (projection > length2) projection = length2;
        qx = x0 + (int)((int64_t)dx * projection / length2);
        qy = y0 + (int)((int64_t)dy * projection / length2);
        ddx = px - qx; ddy = py - qy;
        d2 = ddx * ddx + ddy * ddy;
        sight = fact->sight_px / 2 + 96; /* include path detours and actor boxes */
        if (d2 <= sight * sight && (best < 0 || d2 < best)) {
            best = d2;
            *detector_x = px; *detector_y = py;
        }
    }
    return best >= 0;
}

static int first_enemy_on_route(int x0, int y0, int goal_x, int goal_y,
                                int *enemy_x, int *enemy_y)
{
    int px = x0, py = y0;
    for (int step_index = 0; step_index < MAP_CELLS; ++step_index) {
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
            int dx = u[i].x - px, dy = u[i].y - py;
            int radius = sc64_game_unit_collision_radius(u[i].type) + 12;
            if (abs(dx) < radius && abs(dy) < radius &&
                dx * dx + dy * dy < radius * radius) {
                *enemy_x = u[i].x; *enemy_y = u[i].y;
                return i;
            }
        }
        int nx, ny;
        if (!path_next_tile(px, py, goal_x, goal_y, &nx, &ny) ||
            (nx == px && ny == py)) break;
        px = nx; py = ny;
    }
    return -1;
}

static void checkpoint(const char *kind)
{
    int k = kerrigan_slot();
    int next_x = -1, next_y = -1;
    int side_count = owner_units(SIDE), ling_count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        ling_count += u[i].alive && u[i].owner == SIDE && u[i].raw_type == 37;
    int next_ok = k >= 0 && path_next_tile(u[k].x, u[k].y, u[k].tx, u[k].ty,
                                            &next_x, &next_y);
    trace_prefix();
    fprintf(trace,
        "{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,"
        "\"screen\":%d,\"won\":%d,\"lost\":%d,\"kerrigan\":%d,"
        "\"kerrigan_hp\":%d,\"kerrigan_xy\":[%d,%d],\"kerrigan_order\":%d,"
        "\"kerrigan_target\":[%d,%d],\"path\":[%d,%d,%d,%d,%d],"
        "\"enemy_units\":%d,\"enemy_buildings\":%d,"
        "\"side_units\":%d,\"side_zerglings\":%d,"
        "\"doodad_binding_count\":%u,\"doodad_enabled_byte0\":%u,"
        "\"started_lo\":%u,\"started_hi\":%u,"
        "\"doodad_enabled\":[%u,%u,%u,%u,%u,%u],"
        "\"trigger23_started\":%d,\"fault\":%u,\"orders\":%d}\n",
        kind, (unsigned)host_ticks, (unsigned)frame, screen, won, lost, k,
        k >= 0 ? u[k].hp : 0, k >= 0 ? u[k].x : -1, k >= 0 ? u[k].y : -1,
        k >= 0 ? u[k].order : -1, k >= 0 ? u[k].tx : -1, k >= 0 ? u[k].ty : -1,
        next_ok, next_x, next_y, k >= 0 ? u[k].path_timer : -1,
        k >= 0 ? u[k].path_retry : -1,
        owner_units(ENEMY), owner_buildings(ENEMY), side_count, ling_count,
        (unsigned)race_z5_world.doodad_count,
        (unsigned)race_z5.doodad_enabled[0],
        (unsigned)race_z5.runtime.model.started_lo,
        (unsigned)race_z5.runtime.model.started_hi,
        (unsigned)race_z5.doodad_enabled[0],
        (unsigned)race_z5.doodad_enabled[1],
        (unsigned)race_z5.doodad_enabled[2],
        (unsigned)race_z5.doodad_enabled[3],
        (unsigned)race_z5.doodad_enabled[4],
        (unsigned)race_z5.doodad_enabled[5],
        sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                          race_z5.runtime.model.started_hi, 23),
        (unsigned)race_z5.runtime.fault,
        order_count);
    fflush(trace);
    if (!strcmp(kind, "periodic"))
        fprintf(stderr, "Z5 masks frame=%u started=%08x/%08x cursors=%u,%u,%u,%u\n",
                (unsigned)frame, (unsigned)race_z5.runtime.model.started_lo,
                (unsigned)race_z5.runtime.model.started_hi,
                (unsigned)race_z5.runtime.model.action_cursor[9],
                (unsigned)race_z5.runtime.model.action_cursor[11],
                (unsigned)race_z5.runtime.model.action_cursor[22],
                (unsigned)race_z5.runtime.model.action_cursor[23]);
    if (!strcmp(kind, "periodic")) {
        int shown = 0;
        for (int i = 0; i < MAX_UNITS && shown < 10; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 37) {
                int nx = -1, ny = -1;
                int route = path_next_tile(u[i].x, u[i].y, u[i].tx, u[i].ty, &nx, &ny);
                fprintf(stderr, "Z5 ling frame=%u slot=%d selected=%u xy=%d,%d order=%d target=%d,%d hp=%d\n",
                        (unsigned)frame, i, (unsigned)u[i].selected, u[i].x, u[i].y,
                        u[i].order, u[i].tx, u[i].ty, u[i].hp);
                fprintf(stderr, "Z5 ling path=%d next=%d,%d retry=%d timer=%d\n",
                        route, nx, ny, u[i].path_retry, u[i].path_timer);
                ++shown;
            }
    }
    if (!strcmp(kind, "periodic") && k >= 0 &&
        controller_target_trigger_debug < SC64_Z5_TRIGGER_COUNT) {
        fprintf(stderr, "Z5 route-sample trigger=%u hero_only=%d frame=%u hero=%d,%d order=%d target=%d,%d source_goal=%d,%d recovery_waypoint=%d,%d hp=%d energy_q8=%d cloak=%u\n",
                controller_target_trigger_debug, controller_target_hero_only,
                (unsigned)frame,u[k].x, u[k].y,u[k].order,
                u[k].tx,u[k].ty,controller_source_goal_x,
                controller_source_goal_y,controller_recovery_waypoint_x,
                controller_recovery_waypoint_y,u[k].hp,u[k].carry,
                (unsigned)race_z5.cloak.active);
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
            int dx = u[i].x - u[k].x, dy = u[i].y - u[k].y;
            int d2 = dx * dx + dy * dy;
            if (d2 <= 320 * 320)
                fprintf(stderr, "Z5 route-enemy slot=%d raw=%u type=%u xy=%d,%d hp=%d d2=%d range=%d order=%d\n",
                        i, u[i].raw_type, u[i].type, u[i].x, u[i].y,
                        u[i].hp, d2, unit_range(u[i].type), u[i].order);
        }
        for (unsigned i = 0; i < SC64_Z5_DOODAD_CAPACITY; ++i) {
            int slot = race_z5_doodads[i].building_slot;
            int dx, dy;
            if (slot >= MAX_BUILDINGS || !b[slot].alive) continue;
            dx = b[slot].x - u[k].x; dy = b[slot].y - u[k].y;
            if (dx * dx + dy * dy <= 320 * 320)
                fprintf(stderr, "Z5 route-prop row=%u slot=%d raw=%u xy=%d,%d hp=%d enabled=%u d2=%d\n",
                        i, slot, b[slot].raw_type, b[slot].x, b[slot].y,
                        b[slot].hp, race_z5_doodads[i].enabled, dx*dx+dy*dy);
        }
    }
    if (!strcmp(kind, "start") && k >= 0)
        fprintf(stderr, "Z5 initial navigation: cell=%d,%d walk=%d direct-trigger23=%d route-to-target=%d target=%d,%d walk=%d doodads=%u\n",
                u[k].x / 16, u[k].y / 16,
                is_traversable(u[k].x / 16, u[k].y / 16, -1),
                direct_path_clear(u[k].x, u[k].y, 1544, 328), next_ok,
                u[k].tx, u[k].ty,
                is_traversable(u[k].tx / 16, u[k].ty / 16, -1),
                (unsigned)race_z5.doodad_enabled[0]);
}

static void step(u16 held)
{
    int old_screen = screen;
    int old_hero = kerrigan_slot();
    int old_hp = old_hero >= 0 ? u[old_hero].hp : 0;
    /* A result screen is terminal for this one normal-controller attempt.
     * Do not let a UI key queued by a multi-step helper accidentally restart
     * after death or promote a replay as the same run. */
    if (won || lost || screen == SCREEN_RESULT) return;
    u16 pressed = held & (u16)~prior_keys;
    game_update(held, pressed);
    prior_keys = held;
    ++host_ticks;
    if (replay_trace && held != (u16)replay_last_keys) {
        replay_trace_prefix();
        fprintf(replay_trace,
                "{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}",
                (unsigned)host_ticks, (unsigned)held, (unsigned)pressed);
        replay_last_keys = held;
        fflush(replay_trace);
    }
    if (old_hero >= 0 && u[old_hero].alive && u[old_hero].hp < old_hp) {
        int now = kerrigan_slot();
        fprintf(stderr, "Z5 damage frame=%u hero_hp=%d->%d energy_q8=%d cloak=%u detector_near=%d attackable=%d xy=%d,%d\n",
                (unsigned)frame, old_hp, u[old_hero].hp, u[old_hero].carry,
                (unsigned)race_z5.cloak.active, kerrigan_near_detector(),
                now >= 0 ? race_z5_cloaked_target_attackable(now) : -1,
                u[old_hero].x, u[old_hero].y);
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
            int dx = u[i].x - u[old_hero].x, dy = u[i].y - u[old_hero].y;
            int d2 = dx*dx + dy*dy, range = unit_range(u[i].type);
            int can = race_can_attack(u[i].type, old_hero);
            if (d2 <= (range + 24) * (range + 24))
                fprintf(stderr, "Z5 damage-source candidate slot=%d raw=%u type=%u xy=%d,%d hp=%d range=%d d2=%d race_can_attack=%d\n",
                        i, u[i].raw_type, u[i].type, u[i].x, u[i].y,
                        u[i].hp, range, d2, can);
        }
        for (unsigned row = 0; row < race_z5_world.doodad_count; ++row) {
            unsigned slot = race_z5_doodads[row].building_slot;
            if (slot >= MAX_BUILDINGS || !race_z5_doodads[row].enabled ||
                !b[slot].alive || !sc64_z5_detector_raw(b[slot].raw_type)) continue;
            int dx = b[slot].x - u[old_hero].x, dy = b[slot].y - u[old_hero].y;
            if (dx * dx + dy * dy <= 128 * 128)
                fprintf(stderr, "Z5 damage-detector row=%u raw=%u xy=%d,%d center_d2=%d enabled=%u\n",
                        row, b[slot].raw_type, b[slot].x, b[slot].y,
                        dx*dx+dy*dy, race_z5_doodads[row].enabled);
        }
    }
    if (screen != old_screen)
        fprintf(stderr, "Z5 screen transition %d->%d held=%u pressed=%u frame=%u won=%u lost=%u source_paused=%u ack=%u\n",
                old_screen, screen, (unsigned)held, (unsigned)pressed,
                (unsigned)frame, (unsigned)won, (unsigned)lost,
                (unsigned)race_z5.runtime.source_paused,
                (unsigned)race_z5.pause_acknowledged);
    if (screen == SCREEN_PAUSE && old_screen == SCREEN_PLAY)
        fprintf(stderr, "Z5 pause diagnostics fault=%u unsupported=%d started=%08x/%08x action_cursor0=%u action_cursor23=%u action_ticks0=%u\n",
                (unsigned)race_z5.runtime.fault, mission_unsupported_count,
                (unsigned)race_z5.runtime.model.started_lo,
                (unsigned)race_z5.runtime.model.started_hi,
                (unsigned)race_z5.runtime.model.action_cursor[0],
                (unsigned)race_z5.runtime.model.action_cursor[23],
                (unsigned)race_z5.runtime.action_ticks[0]);
    if (screen == SCREEN_PAUSE && old_screen == SCREEN_PLAY) {
        for (unsigned row = 0; row < race_z5_world.doodad_count; ++row) {
            unsigned slot = race_z5_doodads[row].building_slot;
            if (slot < MAX_BUILDINGS && !b[slot].alive)
                fprintf(stderr, "Z5 pause dead prop row=%u raw=%u slot=%u owner=%u enabled=%u flags=%u\n",
                        row, race_z5_doodads[row].raw_type, slot,
                        race_z5_doodads[row].source_owner,
                        race_z5_doodads[row].enabled,
                        race_z5_doodads[row].raw_flags);
        }
    }
    if (trace && deployed && screen == SCREEN_PLAY && frame > 0 &&
        frame % CHECKPOINT_PERIOD == 0) checkpoint("periodic");
    if (replay_trace && deployed && screen == SCREEN_PLAY && frame > 0 &&
        frame % CHECKPOINT_PERIOD == 0) replay_state_event("periodic");
}

static void tap(u16 key)
{
    step(key);
    step(0);
}

static int kerrigan_near_enemy(void)
{
    int k = kerrigan_slot();
    if (k < 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
        int dx = u[i].x - u[k].x, dy = u[i].y - u[k].y;
        if (dx * dx + dy * dy <= 320 * 320) return 1;
    }
    return 0;
}

static int enemy_can_hit_kerrigan(void)
{
    int k = kerrigan_slot();
    if (k < 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
        int dx = u[i].x - u[k].x, dy = u[i].y - u[k].y;
        int range = unit_range(u[i].type) + 16;
        if (range > 16 && dx*dx + dy*dy <= range*range &&
            race_can_attack(u[i].type, k)) return 1;
    }
    return 0;
}

/* The controller uses the runtime's source-derived detector predicate while
 * cloaked. While uncloaked, the same read-only geometric helper estimates
 * whether activation would be immediately defeated by an enabled trap. */
static int kerrigan_near_detector(void)
{
    int k = kerrigan_slot();
    const Sc64ZpActorFact *actor;
    Sc64Z5DetectBox target;
    if (k < 0) return 0;
    if (race_z5.cloak.active) return race_z5_cloaked_target_attackable(k);
    actor = sc64_zp_actor_fact(u[k].type);
    if (!actor) return 0;
    target = (Sc64Z5DetectBox){u[k].x * 2, u[k].y * 2,
        actor->left, actor->up, actor->right, actor->down};
    for (unsigned i = 0; i < race_z5_world.doodad_count; ++i) {
        const Sc64Z5DoodadState *d = &race_z5_doodads[i];
        const Sc64Z5BuildingFact *source;
        const Building *trap;
        Sc64Z5TrapDetector detector;
        int owner;
        if (!d->enabled || d->building_slot >= MAX_BUILDINGS ||
            !sc64_z5_detector_raw(d->raw_type)) continue;
        source = sc64_z5_building_fact(d->raw_type);
        trap = &b[d->building_slot];
        owner = race_z5_source_owner(d->source_owner);
        if (!source || owner < 0 || owner == SIDE || !trap->alive || trap->hp <= 0 ||
            trap->raw_type != d->raw_type || trap->raw_owner != d->source_owner) continue;
        detector.raw_type = d->raw_type;
        detector.source_flags = source->flags;
        detector.box = (Sc64Z5DetectBox){trap->x * 2, trap->y * 2,
            source->left, source->up, source->right, source->down};
        detector.owner = d->source_owner;
        detector.alive = detector.enabled = detector.completed = 1;
        detector.blinded = 0;
        if (sc64_z5_trap_detects(&target, SC64_Z5_AUTH_KERRIGAN_SOURCE_OWNER,
                                 &detector)) return 1;
    }
    return 0;
}

static int kerrigan_detecting_trap_slot(void)
{
    int k = kerrigan_slot(), best_slot = -1;
    int best_distance = 0x7fffffff;
    const Sc64ZpActorFact *actor;
    Sc64Z5DetectBox target;
    if (k < 0) return -1;
    actor = sc64_zp_actor_fact(u[k].type);
    if (!actor) return -1;
    target = (Sc64Z5DetectBox){u[k].x * 2, u[k].y * 2,
        actor->left, actor->up, actor->right, actor->down};
    for (unsigned row = 0; row < race_z5_world.doodad_count; ++row) {
        const Sc64Z5DoodadState *d = &race_z5_doodads[row];
        const Sc64Z5BuildingFact *source;
        Sc64Z5TrapDetector detector;
        int owner, slot, dx, dy, distance;
        if (!d->enabled || !sc64_z5_detector_raw(d->raw_type) ||
            d->building_slot >= MAX_BUILDINGS) continue;
        slot = d->building_slot;
        if (!b[slot].alive || b[slot].hp <= 0 ||
            b[slot].raw_type != d->raw_type ||
            b[slot].raw_owner != d->source_owner) continue;
        owner = race_z5_source_owner(d->source_owner);
        if (owner < 0 || owner == SIDE) continue;
        source = sc64_z5_building_fact(d->raw_type);
        if (!source) continue;
        detector.raw_type = d->raw_type;
        detector.source_flags = source->flags;
        detector.box = (Sc64Z5DetectBox){b[slot].x * 2, b[slot].y * 2,
            source->left, source->up, source->right, source->down};
        detector.owner = d->source_owner;
        detector.alive = detector.enabled = detector.completed = 1;
        detector.blinded = 0;
        if (!sc64_z5_trap_detects(&target, SC64_Z5_AUTH_KERRIGAN_SOURCE_OWNER,
                                  &detector)) continue;
        dx = b[slot].x - u[k].x; dy = b[slot].y - u[k].y;
        distance = dx * dx + dy * dy;
        if (distance < best_distance) {
            best_distance = distance;
            best_slot = slot;
        }
    }
    return best_slot;
}

/* Use the normal selection and B-menu command row (index ten). These inputs
 * do not touch unit/model state, and the key loop is guarded against recursive
 * policy checks from step(). */
static int set_kerrigan_cloak(int enabled)
{
    int k = kerrigan_slot();
    int result;
    if (k < 0) return 0;
    if (!!race_z5.cloak.active == !!enabled) return 1;
    cloak_controller_busy = 1;
    if (menu_open) tap(KEY_B);
    if (mode != MODE_NORMAL) tap(KEY_B);
    if (menu_open) tap(KEY_B);
    move_cursor(u[k].x, u[k].y);
    tap(KEY_A);
    if (!u[k].selected || selected_count() != 1 || selected_building >= 0) {
        cloak_controller_busy = 0;
        return 0;
    }
    tap(KEY_B);
    for (int row = 0; row < 10; ++row) tap(KEY_DOWN);
    if (menu_cursor != MENU_STOP + 1 || !menu_open) {
        cloak_controller_busy = 0;
        return 0;
    }
    tap(KEY_A);
    result = !!race_z5.cloak.active == !!enabled;
    fprintf(stderr, "Z5 controller cloak request=%d success=%d carry=%d frame=%u mode=%d selected=%d menu=%d row=%d\n",
            enabled, result, u[k].carry, (unsigned)frame, mode,
            selected_count(), menu_open, menu_cursor);
    cloak_controller_busy = 0;
    return result;
}

static void manage_kerrigan_cloak(void)
{
    int k = kerrigan_slot();
    int enemy, detector;
    if (k < 0 || screen != SCREEN_PLAY || race_z5_world_paused() ||
        won || lost || menu_open || mode != MODE_NORMAL) return;
    enemy = kerrigan_near_enemy();
    detector = kerrigan_near_detector();
    if (race_z5.cloak.active) {
        if (enemy) cloak_controller_preloaded = 0;
        if (detector || (!enemy && u[k].hp >= 400 && !cloak_controller_preloaded)) {
            (void)set_kerrigan_cloak(0);
            cloak_controller_preloaded = 0;
        }
    } else if (enemy && !detector &&
               u[k].carry >= SC64_Z5_CLOAK_ACTIVATION_COST_Q8) {
        (void)set_kerrigan_cloak(1);
    }
}

static void manage_kerrigan_consume(void)
{
#ifdef SC64_Z5_CONSUME_ENABLED
    int k = kerrigan_slot(), ling = -1;
    int best = 48 * 48 + 1;
    if (consume_controller_busy || k < 0 || screen != SCREEN_PLAY ||
        won || lost || menu_open || mode != MODE_NORMAL ||
        race_z5_world_paused() || race_z5.cloak.active ||
        consume_controller_count >= 4 ||
        u[k].carry >= 100 * 256 || kerrigan_near_enemy() ||
        kerrigan_near_detector()) return;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && i != k &&
        u[i].owner == SIDE && u[i].raw_type == 37) {
        int d = dist2(u[i].x,u[i].y,u[k].x,u[k].y);
        if (d < best) { best=d; ling=i; }
    }
    if (ling < 0) return;
    consume_controller_busy = 1;
    if (!u[k].selected || selected_count() != 1 || selected_building >= 0) {
        move_cursor(u[k].x,u[k].y); tap(KEY_A);
    }
    if (!u[k].selected || selected_count() != 1 || selected_building >= 0) {
        consume_controller_busy=0; return;
    }
    tap(KEY_B);
    for (int row=0; row<11; ++row) tap(KEY_DOWN);
    if (!menu_open || menu_cursor != MENU_STOP + 2) {
        consume_controller_busy=0; return;
    }
    tap(KEY_A);
    if (mode != MODE_Z5_CONSUME) { consume_controller_busy=0; return; }
    move_cursor(u[ling].x,u[ling].y);
    tap(KEY_A);
    if (!u[ling].alive || u[k].carry > 50 * 256) {
        ++consume_controller_count;
        fprintf(stderr,"Z5 controller Consume used one nearby ling at frame=%u energy=%d\n",
                (unsigned)frame,u[k].carry/256);
    }
    consume_controller_busy=0;
#endif
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
    while ((cx != x || cy != y) && screen == SCREEN_PLAY && !won && !lost) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT;
        if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        if (cy > y) held |= KEY_UP;
        step(held);
        int k = kerrigan_slot();
        if (!controller_retreat_order && k >= 0 && u[k].hp < RETREAT_HP) {
            controller_retreat_requested = 1;
            break;
        }
    }
    step(0);
}

/* Label the current walk grid after a source doodad transition.  This is test
 * steering only; it does not write game state or substitute for game pathing. */
static void nav_relabel(void)
{
    unsigned component = 0;
    build_path_grid();
    memset(nav_component, 0, sizeof(nav_component));
    for (int seed = 0; seed < MAP_CELLS; ++seed) {
        if (nav_component[seed] || !SC64_GRID_GET(path_walkable, seed)) continue;
        ++component;
        unsigned head = 0, tail = 0;
        nav_component[seed] = (u16)component;
        nav_queue[tail++] = (u16)seed;
        while (head < tail) {
            int cell = nav_queue[head++];
            int x = cell % MAP_COLS, y = cell / MAP_COLS;
            const int adjacent[4] = {x ? cell - 1 : -1,
                x + 1 < MAP_COLS ? cell + 1 : -1,
                y ? cell - MAP_COLS : -1,
                y + 1 < MAP_ROWS ? cell + MAP_COLS : -1};
            for (unsigned i = 0; i < 4; ++i) {
                int next = adjacent[i];
                if (next < 0 || nav_component[next] ||
                    !SC64_GRID_GET(path_walkable, next)) continue;
                nav_component[next] = (u16)component;
                nav_queue[tail++] = (u16)next;
            }
        }
    }
}

static int reachable_point_in_location(unsigned id, int *x_out, int *y_out,
                                       int from_x, int from_y)
{
    int left, top, right, bottom, best_x = -1, best_y = -1;
    int center_x, center_y;
    unsigned best_distance = 0xffffffffu;
    unsigned best_clear_distance = 0xffffffffu;
    int clear_x = -1, clear_y = -1;
    if (!sc64_z5_world_location(id, &left, &top, &right, &bottom) ||
        right <= left || bottom <= top) return 0;
    /* The world-location helper already converts CHK coordinates to the
     * engine's half-scale world coordinates. */
    center_x = (left + right) / 2; center_y = (top + bottom) / 2;
    int c0 = limit(left / 16, 0, MAP_COLS - 1);
    int c1 = limit((right - 1) / 16, 0, MAP_COLS - 1);
    int r0 = limit(top / 16, 0, MAP_ROWS - 1);
    int r1 = limit((bottom - 1) / 16, 0, MAP_ROWS - 1);
    for (int row = r0; row <= r1; ++row) for (int col = c0; col <= c1; ++col) {
        int cell = col + row * MAP_COLS;
        int px = col * 16 + 8, py = row * 16 + 8;
        int dx, dy, next_x, next_y;
        int detector_x = -1, detector_y = -1;
        unsigned distance;
        if (px < left || px >= right || py < top || py >= bottom) continue;
        if (!path_next_tile(from_x, from_y, px, py, &next_x, &next_y)) continue;
        dx = px - center_x; dy = py - center_y;
        distance = (unsigned)(dx * dx + dy * dy);
        if (distance < best_distance) { best_distance = distance; best_x = px; best_y = py; }
        if (!route_crosses_detector(from_x,from_y,px,py,&detector_x,&detector_y) &&
            distance < best_clear_distance) {
            best_clear_distance=distance; clear_x=px; clear_y=py;
        }
    }
    if (clear_x >= 0) { best_x=clear_x; best_y=clear_y; }
    if (best_x < 0) return 0;
    *x_out = best_x; *y_out = best_y;
    return 1;
}

static int terminal_gate_ready(int k)
{
    return k >= 0 && u[k].hp >= 400 && u[k].carry >= 225 * 256 &&
        sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                          race_z5.runtime.model.started_hi, 25) &&
        !sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                           race_z5.runtime.model.started_hi, 0);
}

static int terminal_gate_goal(int k, int *x_out, int *y_out)
{
    const Sc64Z5SourceCondition *condition;
    return terminal_gate_ready(k) &&
        sc64_z5_condition_row(0, 0, &condition) && condition->opcode == 3 &&
        condition->player == 13 && condition->unit == Kerrigan_RAW &&
        reachable_point_in_location(condition->location, x_out, y_out,
                                    u[k].x, u[k].y);
}

static int next_reachable_bring(int k, unsigned *trigger_out, int *x_out, int *y_out)
{
    int best = -1, best_x = 0, best_y = 0;
    int hero_best = -1, hero_x = 0, hero_y = 0;
    int choose_hero = 0;
    unsigned best_distance = 0xffffffffu;
    unsigned hero_distance = 0xffffffffu;
    int origin_x, origin_y;
    u16 component;
    if (k < 0) return 0;
    origin_x = u[k].x; origin_y = u[k].y;
    /* Route all Bring objectives with Kerrigan. The source's Force 1 rows
     * accept her as well; preserve the six fragile lings at base for support. */
    nav_relabel();
    component = nav_component[limit(origin_x / 16, 0, MAP_COLS - 1) +
                              limit(origin_y / 16, 0, MAP_ROWS - 1) * MAP_COLS];
    if (!component) return 0;
    if (terminal_gate_goal(k, x_out, y_out)) {
        *trigger_out = 0;
        controller_target_hero_only = 1;
        controller_source_goal_x = *x_out;
        controller_source_goal_y = *y_out;
        return 1;
    }
    for (unsigned trigger = 0; trigger < SC64_Z5_TRIGGER_COUNT; ++trigger) {
        const Sc64Z5SourceTrigger *tr = &sc64_z5_source_triggers[trigger];
        int location = 0, eligible = 0, hero_only = 0, x, y;
        if (sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                              race_z5.runtime.model.started_hi, (u8)trigger)) continue;
        if (sc64_z5_mask_test(blocked_triggers_lo, blocked_triggers_hi,
                              (u8)trigger)) continue;
        if (tr->condition_count != 1) continue;
        {
            const Sc64Z5SourceCondition *condition;
            if (!sc64_z5_condition_row(trigger, 0, &condition) ||
                condition->opcode != 3 ||
                !((condition->player == 18 && condition->unit == 229) ||
                  (condition->player == 13 && condition->unit == Kerrigan_RAW))) continue;
            hero_only = 1;
            location = (int)condition->location;
            eligible = 1;
        }
        if (!eligible) continue;
        int reachable = reachable_point_in_location((unsigned)location,
                                                     &x, &y, origin_x, origin_y);
        if (!reachable) continue;
        int dx = x - origin_x, dy = y - origin_y;
        unsigned distance = (unsigned)(dx * dx + dy * dy);
        if (hero_only && distance < hero_distance) {
            hero_distance = distance; hero_best = (int)trigger; hero_x = x; hero_y = y;
        } else if (!hero_only && distance < best_distance) {
            best_distance = distance; best = (int)trigger; best_x = x; best_y = y;
        }
    }
    if (best < 0) {
        best = hero_best; best_x = hero_x; best_y = hero_y;
        choose_hero = best >= 0;
    }
    if (best < 0) return 0;
    *trigger_out = (unsigned)best; *x_out = best_x; *y_out = best_y;
    controller_target_hero_only = choose_hero;
    return 1;
}

/* Recompute the selected gate's walkable point after a retreat. The desired
 * source location is kept separate from the temporary base waypoint so a
 * recovery command cannot accidentally make the base the next objective. */
static int refresh_source_gate_goal(int k, unsigned *trigger_io,
                                    int *goal_x, int *goal_y)
{
    const Sc64Z5SourceTrigger *tr;
    const Sc64Z5SourceCondition *condition;
    int x, y;
    unsigned trigger = *trigger_io;
    if (k < 0 || trigger >= SC64_Z5_TRIGGER_COUNT ||
        sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                          race_z5.runtime.model.started_hi, (u8)trigger))
        goto choose_next;
    tr = &sc64_z5_source_triggers[trigger];
    for (unsigned i = 0; i < tr->condition_count; ++i) {
        if (!sc64_z5_condition_row(trigger, i, &condition) ||
            condition->opcode != 3 ||
            !((condition->player == 18 && condition->unit == 229) ||
              (condition->player == 13 && condition->unit == Kerrigan_RAW)))
            continue;
        if (reachable_point_in_location(condition->location, &x, &y,
                                        u[k].x, u[k].y)) {
            *goal_x = x;
            *goal_y = y;
            controller_source_goal_x = x;
            controller_source_goal_y = y;
            controller_target_hero_only = 1;
            return 1;
        }
    }
choose_next:
    if (!next_reachable_bring(k, trigger_io, goal_x, goal_y)) return 0;
    controller_source_goal_x = *goal_x;
    controller_source_goal_y = *goal_y;
    controller_target_trigger_debug = *trigger_io;
    return 1;
}

static void dump_navigation_frontier(int k)
{
    u16 origin_component = 0;
    if (k >= 0) {
        nav_relabel();
        origin_component = nav_component[
            limit(u[k].x / 16, 0, MAP_COLS - 1) +
            limit(u[k].y / 16, 0, MAP_ROWS - 1) * MAP_COLS];
    }
    fprintf(stderr, "Z5 frontier frame=%u hero=%d,%d hp=%d energy=%d component=%u fired=%08x/%08x terminal_started=%u terminal_cursor=%u switch110=%d\n",
            (unsigned)frame,k >= 0 ? u[k].x : -1,k >= 0 ? u[k].y : -1,
            k >= 0 ? u[k].hp : -1,k >= 0 ? u[k].carry : -1,origin_component,
            (unsigned)race_z5.runtime.model.started_lo,
            (unsigned)race_z5.runtime.model.started_hi,
            (unsigned)sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                                        race_z5.runtime.model.started_hi,0),
            (unsigned)race_z5.runtime.model.action_cursor[0],
            sc64_z5_switch_is_set(&race_z5.runtime,110));
    for (unsigned trigger = 0; trigger < SC64_Z5_TRIGGER_COUNT; ++trigger) {
        const Sc64Z5SourceTrigger *tr = &sc64_z5_source_triggers[trigger];
        for (unsigned i = 0; i < tr->condition_count; ++i) {
            const Sc64Z5SourceCondition *c;
            int left,top,right,bottom,px=-1,py=-1,reachable=0;
            u16 goal_component=0;
            if (!sc64_z5_condition_row(trigger,i,&c) || c->opcode != 3)
                continue;
            if (sc64_z5_world_location(c->location,&left,&top,&right,&bottom)) {
                reachable = k >= 0 && reachable_point_in_location(
                    c->location,&px,&py,u[k].x,u[k].y);
                if (reachable)
                    goal_component=nav_component[limit(px/16,0,MAP_COLS-1)+
                        limit(py/16,0,MAP_ROWS-1)*MAP_COLS];
                fprintf(stderr,"Z5 frontier-bring trigger=%u cond=%u started=%u player=%u unit=%u loc=%u rect=%d,%d,%d,%d reachable=%d point=%d,%d comp=%u\n",
                        trigger,i,
                        (unsigned)sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                                                    race_z5.runtime.model.started_hi,(u8)trigger),
                        c->player,c->unit,c->location,left,top,right,bottom,
                        reachable,px,py,goal_component);
            }
        }
        for (unsigned i = 0; i < tr->action_count; ++i) {
            const Sc64Z5SourceAction *a;
            if (sc64_z5_source_action(trigger,i,&a) && a->opcode == 42)
                fprintf(stderr,"Z5 frontier-door-action trigger=%u action=%u started=%u loc=%u raw=%u modifier=%u selector=%u\n",
                        trigger,i,
                        (unsigned)sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                                                    race_z5.runtime.model.started_hi,(u8)trigger),
                        a->location,a->unit,a->modifier,a->group1);
        }
    }
    for (unsigned row = 0; row < race_z5_world.doodad_count; ++row) {
        const Sc64Z5DoodadState *d = &race_z5_doodads[row];
        int slot = d->building_slot;
        if (slot >= MAX_BUILDINGS || !b[slot].alive) continue;
        int col=limit(b[slot].x/16,0,MAP_COLS-1);
        int r=limit(b[slot].y/16,0,MAP_ROWS-1);
        fprintf(stderr,"Z5 frontier-prop row=%u raw=%u owner=%u xy=%d,%d enabled=%u hp=%d poolslot=%d cell=%d,%d walk=%d component=%u\n",
                row,d->raw_type,d->source_owner,b[slot].x,b[slot].y,
                d->enabled,b[slot].hp,slot,col,r,
                SC64_GRID_GET(path_walkable,col+r*MAP_COLS),
                nav_component[col+r*MAP_COLS]);
    }
}

static void launch_from_normal_menu(void)
{
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) >= 5);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_ZERG, 16) == 4);
    game_init();
    prior_keys = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_ZERG);
    tap(KEY_RIGHT);
    tap(KEY_RIGHT);
    assert(race_front.mission_page == 2 && race_front.row == 0);
    assert(sc64_catalog_playable_at(SC64_FACTION_ZERG, 4) == 16);
    tap(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 16);
    step(KEY_A);
    deployed = 1;
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == 16 && race_campaign.mission == 16);
    assert(race_human == R_ZERG && race_z5_active());
    assert(mission_unsupported_count == 0 && kerrigan_slot() >= 0);
}

static int select_starting_force(void)
{
    tap(KEY_R); /* Ordinary combat group includes Kerrigan and all Zerglings. */
    return selected_count();
}

static int order_selected_move_to(int x, int y)
{
    if (!selected_count() || selected_building >= 0) return 0;
    if (menu_open) tap(KEY_B);
    if (mode != MODE_NORMAL) tap(KEY_B);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    tap(KEY_B);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    tap(KEY_A); /* first normal-command row: Move */
    if (screen != SCREEN_PLAY || won || lost || mode != MODE_MOVE) return 0;
    move_cursor(x, y);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    tap(KEY_A);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    ++order_count;
    return 1;
}

static int select_zerglings_only(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        u[i].raw_type == 37) {
        move_cursor(u[i].x, u[i].y);
        int picked = selection_target();
        if (picked < 0 || picked >= MAX_UNITS || !u[picked].alive ||
            u[picked].owner != SIDE || u[picked].raw_type != 37) continue;
        tap(KEY_A); tap(KEY_A);
        if (selected_count() > 0) return selected_count();
    }
    fprintf(stderr, "Z5 zergling selection failed frame=%u mode=%d menu=%d selected=%d building=%d cursor=%d,%d live=%d\n",
            (unsigned)frame, mode, menu_open, selected_count(), selected_building,
            cx, cy, owner_units(SIDE));
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE)
        fprintf(stderr, "Z5 remaining side slot=%d raw=%d type=%d xy=%d,%d hp=%d\n",
                i, u[i].raw_type, u[i].type, u[i].x, u[i].y, u[i].hp);
    return 0;
}

static int order_zerglings_move_to(int x, int y)
{
    if (select_zerglings_only() < 1 || screen != SCREEN_PLAY || won || lost)
        return 0;
    return order_selected_move_to(x, y);
}

static int retreat_side_point(const Unit *hero, int goal_x, int goal_y,
                              int *x_out, int *y_out)
{
    int nx, ny;
    int candidates[4][2];
    int ling = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 37) {
            ling = i; break;
        }
    if (ling < 0 || !path_next_tile(hero->x, hero->y, goal_x, goal_y, &nx, &ny))
        return 0;
    if (abs(nx - hero->x) >= abs(ny - hero->y)) {
        candidates[0][0]=hero->x; candidates[0][1]=hero->y+48;
        candidates[1][0]=hero->x; candidates[1][1]=hero->y-48;
        candidates[2][0]=hero->x+48; candidates[2][1]=hero->y;
        candidates[3][0]=hero->x-48; candidates[3][1]=hero->y;
    } else {
        candidates[0][0]=hero->x+48; candidates[0][1]=hero->y;
        candidates[1][0]=hero->x-48; candidates[1][1]=hero->y;
        candidates[2][0]=hero->x; candidates[2][1]=hero->y+48;
        candidates[3][0]=hero->x; candidates[3][1]=hero->y-48;
    }
    for (int i = 0; i < 4; ++i) {
        int px=limit(candidates[i][0],8,WORLD_W-8);
        int py=limit(candidates[i][1],8,WORLD_H-8), lx, ly;
        if (dist2(px,py,hero->x,hero->y) < 32*32 ||
            !path_next_tile(u[ling].x,u[ling].y,px,py,&lx,&ly)) continue;
        *x_out=px; *y_out=py; return 1;
    }
    return 0;
}

static int friendly_ling_blocks_next_step(int hero_slot)
{
    const Unit *hero;
    int speed, nx, ny;
    if (hero_slot < 0 || hero_slot >= MAX_UNITS || !u[hero_slot].alive) return 0;
    hero = &u[hero_slot];
    speed = hero->type == U_ZERGLING || hero->type == U_FIREBAT ||
            hero->type == U_RAYNOR || hero->type == U_VULTURE ? 2 : 1;
    nx = hero->x < hero->path_x ? hero->x + speed :
         hero->x > hero->path_x ? hero->x - speed : hero->x;
    ny = hero->y < hero->path_y ? hero->y + speed :
         hero->y > hero->path_y ? hero->y - speed : hero->y;
    for (int i = 0; i < MAX_UNITS; ++i) {
        int radius, dx, dy, other;
        if (i == hero_slot || !u[i].alive || u[i].owner != SIDE ||
            u[i].raw_type != 37 || SC64_TRANSPORT_LOADED(&u[i])) continue;
        radius = sc64_game_unit_collision_radius(hero->type);
        other = sc64_game_unit_collision_radius(u[i].type);
        if (other > radius) radius = other;
        dx = u[i].x - nx; dy = u[i].y - ny;
        if (abs(dx) < radius && abs(dy) < radius && dx * dx + dy * dy < radius * radius)
            return 1;
    }
    return 0;
}

static int order_combat_group_to(int x, int y, int zerglings_only)
{
    /* Keep Kerrigan under independent cloak/retreat control. Six Zerglings
     * are expendable source units; never send the hero into their attack-move. */
    (void)zerglings_only;
    if (select_zerglings_only() < 1) return 0;
    if (screen != SCREEN_PLAY || won || lost) return 0;
    tap(KEY_B);                 /* Normal command menu. */
    if (screen != SCREEN_PLAY || won || lost) return 0;
    tap(KEY_DOWN);              /* Attack Move is the second command. */
    tap(KEY_A);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    if (mode != MODE_ATTACK) return 0;
    move_cursor(x, y);
    if (screen != SCREEN_PLAY || won || lost || controller_retreat_requested) return 0;
    tap(KEY_A);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    ++order_count;
    return 1;
}

static void reset_target_progress(void)
{
    target_last_progress_frame = frame;
    target_progress_x = -1;
    target_progress_y = -1;
}

static int all_lings_dead(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 37) return 0;
    return 1;
}

static int active_route_position(int hero_only, int x, int y,
                                int *x_out, int *y_out)
{
    int best = 0x7fffffff;
    int has_ling = 0;
    int k = kerrigan_slot();
    for (int i = 0; i < MAX_UNITS; ++i)
        has_ling |= u[i].alive && u[i].owner == SIDE && u[i].raw_type == 37;
    if (hero_only || !has_ling || k < 0) {
        if (k < 0) return 0;
        *x_out = u[k].x; *y_out = u[k].y;
        return 1;
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 37) {
            int distance = dist2(u[i].x, u[i].y, x, y);
            if (distance < best) {
                best = distance;
                *x_out = u[i].x; *y_out = u[i].y;
            }
        }
    return best != 0x7fffffff;
}

static void diagnose_stalled_step(unsigned trigger, int slot)
{
    Unit *q;
    int nx, ny, from_cell, terrain_clear, actors_clear, speed;
    if (slot < 0 || slot >= MAX_UNITS) return;
    q = &u[slot];
    speed = q->type == U_ZERGLING || q->type == U_FIREBAT ||
            q->type == U_RAYNOR || q->type == U_VULTURE ? 2 : 1;
    nx = q->x < q->path_x ? q->x + speed :
         q->x > q->path_x ? q->x - speed : q->x;
    ny = q->y < q->path_y ? q->y + speed :
         q->y > q->path_y ? q->y - speed : q->y;
    from_cell = q->x / 16 + (q->y / 16) * MAP_COLS;
    terrain_clear = is_traversable(nx / 16, ny / 16, from_cell);
    actors_clear = unit_step_clear(q, nx, ny);
    fprintf(stderr, "Z5 stalled-step trigger=%u unit=%d raw=%u type=%u xy=%d,%d next=%d,%d waypoint=%d,%d cell=%d,%d walk=%d actor_clear=%d retry=%d timer=%d cloak=%u carry=%d\n",
            trigger, slot, q->raw_type, q->type, q->x, q->y, nx, ny,
            q->path_x, q->path_y, nx / 16, ny / 16, terrain_clear,
            actors_clear, q->path_retry, q->path_timer,
            (unsigned)race_z5.cloak.active, q->carry);
    fprintf(stderr, "Z5 stalled-order=%d target=%d target-xy=%d,%d cooldown=%d range=%d\n",
            q->order, q->target, q->target >= 0 && q->target < MAX_UNITS ?
                u[q->target].x : -1,
            q->target >= 0 && q->target < MAX_UNITS ? u[q->target].y : -1,
            q->cd, unit_range(q->type));
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
        int dx = u[i].x - q->x, dy = u[i].y - q->y;
        int range = unit_range(q->type) + 18;
        if (dx * dx + dy * dy <= range * range)
            fprintf(stderr, "Z5 enemy-in-attack-range slot=%d raw=%u type=%u xy=%d,%d hp=%d distance2=%d\n",
                    i, u[i].raw_type, u[i].type, u[i].x, u[i].y,
                    u[i].hp, dx*dx+dy*dy);
    }
    for (int i = 0; i < MAX_UNITS; ++i) {
        int radius, dx, dy;
        if (&u[i] == q || !u[i].alive || SC64_TRANSPORT_LOADED(&u[i]) ||
            sc64_race_unit_flying(u[i].type) != sc64_race_unit_flying(q->type)) continue;
        radius = sc64_game_unit_collision_radius(q->type);
        int candidate = sc64_game_unit_collision_radius(u[i].type);
        if (candidate > radius) radius = candidate;
        dx = u[i].x - nx; dy = u[i].y - ny;
        if (abs(dx) < radius && abs(dy) < radius && dx * dx + dy * dy < radius * radius)
            fprintf(stderr, "Z5 nearby-unit slot=%d raw=%u type=%u owner=%d flags=%u xy=%d,%d hp=%d radius=%d d2=%d collidable=%d\n",
                    i, u[i].raw_type, u[i].type, u[i].owner, u[i].flags,
                    u[i].x, u[i].y, u[i].hp, radius, dx*dx+dy*dy,
                    sc64_game_unit_collidable(u[i].type));
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int dx = b[i].x - nx, dy = b[i].y - ny;
        if (abs(dx) > 128 || abs(dy) > 128) continue;
        int left = 0, up = 0, right = 0, down = 0;
        (void)sc64_game_building_bounds_source(b[i].type, &left, &up, &right, &down);
        fprintf(stderr, "Z5 nearby-building slot=%d raw=%u type=%u owner=%d flags=%u xy=%d,%d hp=%d enabled-collision=%d source-bounds=%d,%d,%d,%d d2=%d\n",
                i, b[i].raw_type, b[i].type, b[i].owner, b[i].flags,
                b[i].x, b[i].y, b[i].hp, sc64_t4_building_collidable(i),
                left, up, right, down, dx*dx+dy*dy);
    }
}

static int order_kerrigan_move_to(int x, int y)
{
    int k = kerrigan_slot();
    if (k < 0) return 0;
    if (menu_open) tap(KEY_B);
    if (mode != MODE_NORMAL) {
        tap(KEY_B);
        if (screen != SCREEN_PLAY || won || lost) return 0;
        if (menu_open) tap(KEY_B);
    }
    if (!u[k].selected || selected_count() != 1 || selected_building >= 0) {
        for (int attempt = 0; attempt < 12 &&
             dist2(cx, cy, u[k].x, u[k].y) > 22 * 22; ++attempt)
            move_cursor(u[k].x, u[k].y);
        if (dist2(cx, cy, u[k].x, u[k].y) > 31 * 31) return 0;
        if (screen != SCREEN_PLAY || won || lost) return 0;
        tap(KEY_A);
        if (screen != SCREEN_PLAY || won || lost) return 0;
        if (!u[k].selected || selected_count() != 1 || selected_building >= 0) return 0;
    }
    tap(KEY_B);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    tap(KEY_A);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    if (mode != MODE_MOVE) return 0;
    move_cursor(x, y);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    tap(KEY_A);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    ++order_count;
    return 1;
}

static int order_kerrigan_attack_move_to(int x, int y)
{
    int k = kerrigan_slot();
    if (k < 0) return 0;
    if (menu_open) tap(KEY_B);
    if (mode != MODE_NORMAL) {
        tap(KEY_B);
        if (screen != SCREEN_PLAY || won || lost) return 0;
        if (menu_open) tap(KEY_B);
    }
    if (!u[k].selected || selected_count() != 1 || selected_building >= 0) {
        for (int attempt = 0; attempt < 12 &&
             dist2(cx, cy, u[k].x, u[k].y) > 22 * 22; ++attempt)
            move_cursor(u[k].x, u[k].y);
        if (dist2(cx, cy, u[k].x, u[k].y) > 31 * 31) return 0;
        if (screen != SCREEN_PLAY || won || lost) return 0;
        tap(KEY_A);
        if (!u[k].selected || selected_count() != 1 || selected_building >= 0)
            return 0;
    }
    tap(KEY_B);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    tap(KEY_DOWN);
    tap(KEY_A);
    if (screen != SCREEN_PLAY || won || lost || mode != MODE_ATTACK) return 0;
    move_cursor(x, y);
    if (screen != SCREEN_PLAY || won || lost || controller_retreat_requested) return 0;
    tap(KEY_A);
    if (screen != SCREEN_PLAY || won || lost) return 0;
    ++order_count;
    return 1;
}

int main(int argc, char **argv)
{
    const char *attempt_path = ".local/sc64/z5-controller-attempt.json";
    const char *verified_path = ".local/sc64/z5-controller-win.json";
    const char *trace_path = ".local/sc64/z5-controller-trace.json";
    const char *replay_temp_path = ".local/sc64/z5-controller-input-running.json";
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--limit") && i + 1 < argc) run_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc) {
            trace_path = argv[++i];
        }
        else if (!strcmp(argv[i], "--attempt") && i + 1 < argc) {
            attempt_path = argv[++i];
        }
        else if (!strcmp(argv[i], "--verified") && i + 1 < argc) verified_path = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            puts("usage: sc64_z5_campaign_run [--limit TICKS] [--trace PATH] [--attempt PATH] [--verified REPLAY_TRACE]");
            return 0;
        } else return 2;
    }
    if (run_limit < 1 || run_limit > 1000000) return 2;
    mkdir(".local", 0777);
    mkdir(".local/sc64", 0777);
    trace = fopen(trace_path, "w");
    assert(trace);
    fprintf(trace, "{\"schema\":1,\"mission\":16,\"profile\":\"zerg-z5\","
                   "\"policy\":\"normal-menu-combat-or-zergling-attackmove-with-source-healing-retreats\",\"events\":[\n");
    replay_trace = fopen(replay_temp_path, "w");
    assert(replay_trace);
    fprintf(replay_trace, "{\"schema\":1,\"mission\":16,\"profile\":\"zerg-z5\","
                         "\"policy\":\"normal-menu-key-inputs-with-source-gate-replanning-and-authored-cloak-retreats\",\"events\":[\n");
    gba_init();
    launch_from_normal_menu();
    checkpoint("start");
    replay_state_event("start");
    int initial_selection = select_starting_force();
    fprintf(stderr, "Z5 normal combat-group command selected %d starting units\n", initial_selection);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE)
        fprintf(stderr, "Z5 initial selected slot=%d raw=%u type=%u selected=%u xy=%d,%d\n",
                i, u[i].raw_type, u[i].type, (unsigned)u[i].selected, u[i].x, u[i].y);
    if (initial_selection > 1) {
        int moved = order_selected_move_to(1696, 115);
        fprintf(stderr, "Z5 consolidating starter group at base with Move: %d\n", moved);
        if (screen == SCREEN_PLAY && !won && !lost)
            (void)order_kerrigan_move_to(1696, 115);
    }

    {
        int side_count = 0, reachable = 0;
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
            int nx = -1, ny = -1;
            int can = path_next_tile(u[i].x, u[i].y, 1544, 328, &nx, &ny);
            ++side_count; reachable += can;
            if (i < 12)
                fprintf(stderr, "Z5 initial unit slot=%d raw=%d type=%d xy=%d,%d path-to-trigger23=%d next=%d,%d\n",
                        i, u[i].raw_type, u[i].type, u[i].x, u[i].y, can, nx, ny);
        }
        fprintf(stderr, "Z5 initial side units=%d with path to reachable trigger23 region=%d\n", side_count, reachable);
    }

    int goal_x = 0, goal_y = 0;
    int source_goal_x = 0, source_goal_y = 0;
    unsigned target_trigger = SC64_Z5_TRIGGER_COUNT;
    int target_changed = 0;
    unsigned idle_retry_frame = frame;
    int target_ling_fallback = 0;
    int retreating = 0;
    int retreat_progress_x = -1, retreat_progress_y = -1;
    unsigned retreat_progress_frame = frame;
    int retreat_lings_repositioned = 0;
    int recovery_replan_pending = 0;
    int target_force_attack = 0;
    int terminal_trap_slot = -1;
    u16 terminal_trap_raw = 0;
    int support_lings_forward = 0;
    int support_target_trigger = -1;
    int support_clear_slot = -1;
    u16 support_clear_raw = 0;
    unsigned support_clear_deadline = 0;
    reset_target_progress();
    observed_started_lo = race_z5.runtime.model.started_lo;
    observed_started_hi = race_z5.runtime.model.started_hi;
    while ((int)frame < run_limit && screen == SCREEN_PLAY && !won && !lost) {
        int k = kerrigan_slot();
        if (k < 0) {
            /* Allow the ordinary defeat/result transition to settle after
             * the hero dies; never feed a later menu input that restarts. */
            for (int settle = 0; settle < 30 && screen == SCREEN_PLAY &&
                 !won && !lost; ++settle) step(0);
            break;
        }
        if (observed_started_lo != race_z5.runtime.model.started_lo ||
            observed_started_hi != race_z5.runtime.model.started_hi) {
            blocked_triggers_lo = blocked_triggers_hi = 0;
            observed_started_lo = race_z5.runtime.model.started_lo;
            observed_started_hi = race_z5.runtime.model.started_hi;
            fprintf(stderr, "Z5 gate progress frame=%u fired=%08x/%08x; clearing blocked-gate set\n",
                    (unsigned)frame, (unsigned)observed_started_lo,
                    (unsigned)observed_started_hi);
            if (support_lings_forward && support_target_trigger >= 0 &&
                sc64_z5_mask_test(observed_started_lo, observed_started_hi,
                                  (u8)support_target_trigger)) {
                int side_x=0,side_y=0;
                if (retreat_side_point(&u[k],controller_recovery_waypoint_x,
                                      controller_recovery_waypoint_y,
                                      &side_x,&side_y) &&
                    order_zerglings_move_to(side_x,side_y)) {
                    support_lings_forward=0;
                    fprintf(stderr, "Z5 support lings cleared from the route after trigger %d fired; Move to %d,%d frame=%u\n",
                            support_target_trigger,side_x,side_y,(unsigned)frame);
                }
            }
        }
        if (host_ticks % 12u == 0u) {
            manage_kerrigan_cloak();
            manage_kerrigan_consume();
        }
        if (retreating && u[k].order == ORDER_IDLE && u[k].hp >= 400 &&
            u[k].carry >= 225 * 256 && !kerrigan_near_enemy() &&
            !kerrigan_near_detector()) {
            retreating=0;
            target_changed=1;
            recovery_replan_pending=1;
            fprintf(stderr,"Z5 safe-idle recovery at frame=%u xy=%d,%d; resuming reachable gate %u\n",
                    (unsigned)frame,u[k].x,u[k].y,target_trigger);
        }
        if (target_trigger < SC64_Z5_TRIGGER_COUNT && !retreating &&
            !sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                               race_z5.runtime.model.started_hi,
                               (u8)target_trigger) &&
            u[k].order == ORDER_IDLE && u[k].hp >= 340 &&
            u[k].carry >= 225 * 256 &&
            dist2(u[k].x,u[k].y,goal_x,goal_y) > 32*32 &&
            (u32)(frame-idle_retry_frame) >= 300u) {
            int blocker_x=0, blocker_y=0;
            int blocker=first_enemy_on_route(u[k].x,u[k].y,goal_x,goal_y,
                                             &blocker_x,&blocker_y);
            idle_retry_frame=frame;
            if (!kerrigan_near_detector() &&
                u[k].carry >= SC64_Z5_CLOAK_ACTIVATION_COST_Q8) {
                (void)set_kerrigan_cloak(1);
                cloak_controller_preloaded=1;
            }
            int accepted = blocker >= 0 ?
                order_kerrigan_attack_move_to(blocker_x,blocker_y) :
                order_kerrigan_move_to(goal_x,goal_y);
            if (!accepted || u[k].order == ORDER_IDLE ||
                dist2(u[k].tx,u[k].ty,blocker >= 0 ? blocker_x : goal_x,
                      blocker >= 0 ? blocker_y : goal_y) > 32*32) {
                fprintf(stderr,"Z5 idle gate command rejected trigger=%u frame=%u accepted=%d order=%d tx=%d ty=%d goal=%d,%d blocker=%d\n",
                        target_trigger,(unsigned)frame,accepted,u[k].order,
                        u[k].tx,u[k].ty,goal_x,goal_y,blocker);
            } else {
                fprintf(stderr,"Z5 idle Kerrigan %s for trigger %u frame=%u xy=%d,%d command=%d,%d\n",
                        blocker >= 0 ? "AttackMove" : "Move",target_trigger,
                        (unsigned)frame,u[k].x,u[k].y,
                        blocker >= 0 ? blocker_x : goal_x,
                        blocker >= 0 ? blocker_y : goal_y);
            }
        }
        if (support_clear_slot >= 0) {
            int same_enemy = support_clear_slot < MAX_UNITS &&
                u[support_clear_slot].alive && u[support_clear_slot].owner == ENEMY &&
                u[support_clear_slot].raw_type == support_clear_raw;
            if (!same_enemy || (s32)(frame - support_clear_deadline) >= 0) {
                fprintf(stderr, "Z5 support-clear %s slot=%d raw=%u frame=%u; resuming Kerrigan\n",
                        same_enemy ? "timed out" : "removed", support_clear_slot,
                        support_clear_raw, (unsigned)frame);
                support_clear_slot = -1;
                target_force_attack = 0;
                target_changed = 1;
                reset_target_progress();
            } else {
                step(0);
                if (screen == SCREEN_PLAY && race_z5_world_paused() &&
                    !race_z5.pause_acknowledged) tap(KEY_A);
                continue;
            }
        }
        if (!target_changed && !target_ling_fallback &&
            target_trigger < SC64_Z5_TRIGGER_COUNT &&
            !controller_target_hero_only &&
            !sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                               race_z5.runtime.model.started_hi,
                               (u8)target_trigger)) {
            int lings_remain = 0;
            for (int i = 0; i < MAX_UNITS; ++i)
                lings_remain += u[i].alive && u[i].owner == SIDE &&
                                u[i].raw_type == 37;
            if (!lings_remain) {
                target_changed = 1;
                target_ling_fallback = 1;
                fprintf(stderr, "Z5 no Zerglings remain for source trigger=%u; switching to Kerrigan frame=%u\n",
                        target_trigger, (unsigned)frame);
            }
        }
        if (retreating) {
            if (u[k].hp >= 400 &&
                dist2(u[k].x, u[k].y, controller_recovery_waypoint_x,
                      controller_recovery_waypoint_y) <= 48 * 48 &&
                u[k].carry >= 225 * 256) {
                if (race_z5.cloak.active) (void)set_kerrigan_cloak(0);
                retreating = 0;
                target_changed = 1;
                recovery_replan_pending = 1;
                fprintf(stderr, "Z5 Kerrigan recovered at base; resume frame=%u hp=%d carry=%d xy=%d,%d\n",
                        (unsigned)frame, u[k].hp, u[k].carry, u[k].x, u[k].y);
            } else {
                step(0);
                if (host_ticks % 12u == 0u) {
                    manage_kerrigan_cloak();
                    manage_kerrigan_consume();
                }
                if (screen == SCREEN_PLAY && race_z5_world_paused() &&
                    !race_z5.pause_acknowledged) tap(KEY_A);
                k = kerrigan_slot();
                if (k >= 0 && (retreat_progress_x < 0 ||
                    dist2(u[k].x,u[k].y,retreat_progress_x,retreat_progress_y) >= 16*16)) {
                    retreat_progress_x=u[k].x; retreat_progress_y=u[k].y;
                    retreat_progress_frame=frame;
                } else if (k >= 0 && (u32)(frame-retreat_progress_frame) >= 900u) {
                    int side_x=0,side_y=0;
                    diagnose_stalled_step(target_trigger,k);
                    if (friendly_ling_blocks_next_step(k) &&
                        !retreat_lings_repositioned && !all_lings_dead() &&
                        retreat_side_point(&u[k],goal_x,goal_y,&side_x,&side_y) &&
                        order_zerglings_move_to(side_x,side_y)) {
                        retreat_lings_repositioned=1;
                        support_lings_forward=0;
                        fprintf(stderr, "Z5 retreat path blocked by the support group; moving lings aside to %d,%d at frame=%u\n",
                                side_x,side_y,(unsigned)frame);
                        (void)order_kerrigan_move_to(controller_recovery_waypoint_x,
                                                     controller_recovery_waypoint_y);
                    } else {
                        int enemy_x=0,enemy_y=0;
                        int blocker=first_enemy_on_route(u[k].x,u[k].y,
                                                        controller_recovery_waypoint_x,
                                                        controller_recovery_waypoint_y,
                                                        &enemy_x,&enemy_y);
                        if (blocker>=0 && !all_lings_dead() &&
                            order_combat_group_to(enemy_x,enemy_y,1)) {
                            support_lings_forward=1;
                            support_target_trigger=(int)target_trigger;
                            fprintf(stderr, "Z5 retreat path hostile block raw=%u at %d,%d; lings attacking at frame=%u\n",
                                    u[blocker].raw_type,enemy_x,enemy_y,(unsigned)frame);
                            (void)order_kerrigan_move_to(controller_recovery_waypoint_x,
                                                         controller_recovery_waypoint_y);
                        }
                    }
                    retreat_progress_x=u[k].x; retreat_progress_y=u[k].y;
                    retreat_progress_frame=frame;
                }
                continue;
            }
        }
        if (recovery_replan_pending) {
            recovery_replan_pending = 0;
            if (!refresh_source_gate_goal(k, &target_trigger, &goal_x, &goal_y)) {
                fprintf(stderr, "Z5 recovery could not replan a required source gate at frame=%u\n",
                        (unsigned)frame);
                break;
            }
            controller_target_trigger_debug = target_trigger;
            target_changed = 1;
            target_ling_fallback = 1;
            target_force_attack = 0;
            idle_retry_frame = frame;
            reset_target_progress();
            fprintf(stderr, "Z5 recovery replanned source trigger=%u goal=%d,%d frame=%u\n",
                    target_trigger, goal_x, goal_y, (unsigned)frame);
        }
        if (!retreating && target_trigger != 0 && terminal_gate_ready(k)) {
            int terminal_x, terminal_y;
            if (terminal_gate_goal(k, &terminal_x, &terminal_y)) {
                target_trigger = 0;
                goal_x = source_goal_x = terminal_x;
                goal_y = source_goal_y = terminal_y;
                controller_source_goal_x = terminal_x;
                controller_source_goal_y = terminal_y;
                controller_target_trigger_debug = 0;
                controller_target_hero_only = 1;
                target_changed = 1;
                target_force_attack = 0;
                idle_retry_frame = frame;
                reset_target_progress();
                fprintf(stderr,"Z5 terminal gate0 became required after gate25 and full recovery; source goal=%d,%d frame=%u\n",
                        terminal_x,terminal_y,(unsigned)frame);
            }
        }
        if (target_trigger < SC64_Z5_TRIGGER_COUNT && !target_changed &&
            !retreating && !sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                                               race_z5.runtime.model.started_hi,
                                               (u8)target_trigger)) {
            int route_x = -1, route_y = -1;
            if (active_route_position(controller_target_hero_only, goal_x, goal_y,
                                      &route_x, &route_y) &&
                (target_progress_x < 0 ||
                 dist2(route_x, route_y, target_progress_x,
                       target_progress_y) >= 16 * 16)) {
                /* Count movement along a valid detour as progress even when
                 * it temporarily increases straight-line distance. */
                target_progress_x = route_x;
                target_progress_y = route_y;
                target_last_progress_frame = frame;
            } else if ((u32)(frame - target_last_progress_frame) >=
                       TARGET_STALL_LIMIT) {
                diagnose_stalled_step(target_trigger, k);
                if (friendly_ling_blocks_next_step(k) && !all_lings_dead()) {
                    if (order_zerglings_move_to(1696,115)) {
                        support_lings_forward=0;
                        target_force_attack=0;
                        reset_target_progress();
                        (void)order_kerrigan_move_to(goal_x,goal_y);
                        fprintf(stderr, "Z5 stalled route has a friendly ling in Kerrigan's next step; recalling lings to the safe base and retrying gate %u at frame=%u\n",
                                target_trigger,(unsigned)frame);
                    }
                } else if (!all_lings_dead()) {
                    int enemy_x=0,enemy_y=0;
                    int blocker=first_enemy_on_route(u[k].x,u[k].y,goal_x,goal_y,
                                                    &enemy_x,&enemy_y);
                    if (blocker >= 0 && target_trigger < 25 &&
                        order_kerrigan_attack_move_to(enemy_x,enemy_y)) {
                        target_force_attack=1;
                        reset_target_progress();
                        fprintf(stderr, "Z5 gate %u has a hostile route blocker raw=%u type=%u at %d,%d; Kerrigan attack-moving while preserving lings frame=%u\n",
                                target_trigger,u[blocker].raw_type,u[blocker].type,
                                enemy_x,enemy_y,(unsigned)frame);
                    } else if (blocker >= 0 && target_trigger == 25 &&
                        order_kerrigan_attack_move_to(enemy_x,enemy_y)) {
                        /* At the last gate a single surviving ling is too
                         * fragile to clear the blocker while Kerrigan waits
                         * beside it. Let Kerrigan attack the actual blocker
                         * with a normal attack-move; the next stall check
                         * replans after it dies. */
                        target_force_attack=1;
                        reset_target_progress();
                        fprintf(stderr, "Z5 gate 25 blocker raw=%u type=%u at %d,%d; Kerrigan attack-moving to clear it frame=%u\n",
                                u[blocker].raw_type,u[blocker].type,
                                enemy_x,enemy_y,(unsigned)frame);
                    } else if (blocker >= 0 &&
                        order_combat_group_to(enemy_x,enemy_y,1)) {
                        target_force_attack=1;
                        support_lings_forward=1;
                        support_target_trigger=(int)target_trigger;
                        support_clear_slot=blocker;
                        support_clear_raw=u[blocker].raw_type;
                        support_clear_deadline=frame+9000u;
                        reset_target_progress();
                        fprintf(stderr, "Z5 stalled route hostile blocker raw=%u type=%u at %d,%d; holding Kerrigan while lings clear it for gate %u frame=%u\n",
                                u[blocker].raw_type,u[blocker].type,enemy_x,enemy_y,
                                target_trigger,(unsigned)frame);
                    } else if (!target_force_attack &&
                               order_combat_group_to(goal_x,goal_y,1)) {
                        target_force_attack=1;
                        support_lings_forward=1;
                        support_target_trigger=(int)target_trigger;
                        reset_target_progress();
                    } else {
                        sc64_z5_mask_set(&blocked_triggers_lo, &blocked_triggers_hi,
                                         (u8)target_trigger);
                        fprintf(stderr, "Z5 controller stalled on source trigger=%u for %u frames at %u,%u; marking blocked and replanning\n",
                                target_trigger, TARGET_STALL_LIMIT,
                                k >= 0 ? u[k].x : -1, k >= 0 ? u[k].y : -1);
                        target_trigger = SC64_Z5_TRIGGER_COUNT;
                        target_changed = 1;
                    }
                } else if (all_lings_dead()) {
                    int enemy_x=0,enemy_y=0;
                    int blocker=first_enemy_on_route(u[k].x,u[k].y,goal_x,goal_y,
                                                    &enemy_x,&enemy_y);
                    target_force_attack = 1;
                    if (blocker >= 0 &&
                        order_kerrigan_attack_move_to(enemy_x,enemy_y)) {
                        target_changed=0;
                        reset_target_progress();
                        fprintf(stderr, "Z5 no lings remain; Kerrigan attack-moving on actual route blocker raw=%u type=%u at %d,%d for gate %u frame=%u\\n",
                                u[blocker].raw_type,u[blocker].type,enemy_x,enemy_y,
                                target_trigger,(unsigned)frame);
                    } else {
                        target_changed = 1;
                        fprintf(stderr, "Z5 controller stalled on source trigger=%u with no route target; retrying gate with attack-move\\n",
                                target_trigger);
                    }
                } else {
                    sc64_z5_mask_set(&blocked_triggers_lo, &blocked_triggers_hi,
                                     (u8)target_trigger);
                    fprintf(stderr, "Z5 controller stalled on source trigger=%u for %u frames at %u,%u; marking blocked and replanning\n",
                            target_trigger, TARGET_STALL_LIMIT,
                            k >= 0 ? u[k].x : -1, k >= 0 ? u[k].y : -1);
                    target_trigger = SC64_Z5_TRIGGER_COUNT;
                    target_changed = 1;
                }
            }
        }
        if (!retreating &&
            (u[k].hp < RETREAT_HP ||
             u[k].carry < RETREAT_ENERGY_Q8) &&
            dist2(u[k].x, u[k].y, controller_recovery_waypoint_x,
                  controller_recovery_waypoint_y) > 64 * 64) {
            controller_retreat_order = 1;
            controller_recovery_waypoint_x = 1696;
            controller_recovery_waypoint_y = 115;
            if (order_kerrigan_move_to(controller_recovery_waypoint_x,
                                       controller_recovery_waypoint_y)) {
                retreating = 1;
                retreat_progress_x=u[k].x; retreat_progress_y=u[k].y;
                retreat_progress_frame=frame; retreat_lings_repositioned=0;
                fprintf(stderr, "Z5 Kerrigan retreat for source-faithful recovery frame=%u hp=%d energy=%d cloak=%u\n",
                        (unsigned)frame, u[k].hp, u[k].carry / 256,
                        (unsigned)race_z5.cloak.active);
                controller_retreat_requested = 0;
                continue;
            }
            controller_retreat_order = 0;
            if (screen != SCREEN_PLAY || won || lost) break;
        }
        if (target_trigger >= SC64_Z5_TRIGGER_COUNT ||
            sc64_z5_mask_test(race_z5.runtime.model.started_lo,
                              race_z5.runtime.model.started_hi,
                              (u8)target_trigger)) {
            unsigned next_trigger;
            if (!next_reachable_bring(k, &next_trigger, &goal_x, &goal_y)) {
                fprintf(stderr, "Z5 controller found no reachable unstarted Bring region at frame=%u Kerrigan=%d,%d\n",
                        (unsigned)frame, u[k].x, u[k].y);
                break;
            }
            target_trigger = next_trigger;
            controller_target_trigger_debug = target_trigger;
            source_goal_x = goal_x;
            source_goal_y = goal_y;
            controller_source_goal_x = goal_x;
            controller_source_goal_y = goal_y;
            target_changed = 1;
            target_ling_fallback = 0;
            target_force_attack = 0;
            idle_retry_frame = frame;
            reset_target_progress();
            fprintf(stderr, "Z5 controller target source trigger=%u at %d,%d frame=%u\n",
                    target_trigger, goal_x, goal_y, (unsigned)frame);
        }
        if (!retreating && target_trigger == 0) {
            int trap_slot = kerrigan_detecting_trap_slot();
            if (trap_slot >= 0) {
                if (terminal_trap_slot != trap_slot ||
                    terminal_trap_raw != b[trap_slot].raw_type ||
                    dist2(u[k].tx,u[k].ty,b[trap_slot].x,b[trap_slot].y) > 24*24) {
                    int accepted = order_kerrigan_attack_move_to(
                        b[trap_slot].x,b[trap_slot].y);
                    accepted = accepted && u[k].order == ORDER_ATTACK_MOVE &&
                        u[k].tx == b[trap_slot].x && u[k].ty == b[trap_slot].y;
                    fprintf(stderr,"Z5 terminal gate0 detector response slot=%d raw=%u xy=%d,%d hp=%d accepted=%d source_goal=%d,%d frame=%u\n",
                            trap_slot,b[trap_slot].raw_type,b[trap_slot].x,
                            b[trap_slot].y,b[trap_slot].hp,accepted,
                            source_goal_x,source_goal_y,(unsigned)frame);
                    target_changed = 0;
                    reset_target_progress();
                }
                terminal_trap_slot=trap_slot;
                terminal_trap_raw=b[trap_slot].raw_type;
                target_force_attack=1;
            } else if (terminal_trap_slot >= 0) {
                int same_trap=terminal_trap_slot<MAX_BUILDINGS &&
                    b[terminal_trap_slot].alive &&
                    b[terminal_trap_slot].raw_type==terminal_trap_raw;
                fprintf(stderr,"Z5 terminal gate0 detector %s slot=%d; resume source goal=%d,%d frame=%u\n",
                        same_trap ? "no longer detects" : "destroyed",
                        terminal_trap_slot,source_goal_x,source_goal_y,
                        (unsigned)frame);
                terminal_trap_slot=-1;
                terminal_trap_raw=0;
                if (same_trap && u[k].order==ORDER_ATTACK_MOVE &&
                    dist2(u[k].tx,u[k].ty,source_goal_x,source_goal_y)>32*32) {
                    if (order_kerrigan_attack_move_to(source_goal_x,source_goal_y))
                        target_changed=0;
                }
            }
        }
        if (target_changed) {
            if (target_trigger != SC64_Z5_TRIGGER_COUNT &&
                ((controller_source_goal_x == 1696 &&
                  (controller_source_goal_y == 113 ||
                   controller_source_goal_y == 115)) ||
                 (goal_x == 1696 &&
                  (goal_y == 113 || goal_y == 115)))) {
                fprintf(stderr, "Z5 controller invariant failure: advancing trigger=%u has base as goal at frame=%u\n",
                        target_trigger, (unsigned)frame);
                assert(0 && "required source goal must differ from recovery base");
            }
            if (target_trigger < SC64_Z5_TRIGGER_COUNT) {
                source_goal_x = goal_x;
                source_goal_y = goal_y;
                controller_source_goal_x = source_goal_x;
                controller_source_goal_y = source_goal_y;
            }
            if (target_trigger == 25 && !target_force_attack &&
                !all_lings_dead()) {
                int support_x = 0, support_y = 0;
                int blocker = first_enemy_on_route(u[k].x, u[k].y,
                                                    goal_x, goal_y,
                                                    &support_x, &support_y);
                if (blocker >= 0 &&
                    order_kerrigan_attack_move_to(goal_x,goal_y)) {
                    target_force_attack = 1;
                    reset_target_progress();
                    fprintf(stderr,"Z5 trigger25 route has hostile raw=%u at %d,%d; Kerrigan attack-moving to the Bring point while keeping lings at base frame=%u\\n",
                            u[blocker].raw_type,support_x,support_y,(unsigned)frame);
                    target_changed=0;
                    reset_target_progress();
                    continue;
                }
            }
            controller_retreat_requested = 0;
            if (target_trigger == 0 || target_trigger == 25 ||
                controller_target_hero_only) {
                int accepted;
                if (target_trigger == 0) {
                    int trap_slot = kerrigan_detecting_trap_slot();
                    if (trap_slot >= 0) {
                        accepted = order_kerrigan_attack_move_to(
                            b[trap_slot].x,b[trap_slot].y);
                        accepted = accepted && u[k].order == ORDER_ATTACK_MOVE &&
                            u[k].tx == b[trap_slot].x && u[k].ty == b[trap_slot].y;
                        target_force_attack = accepted;
                        fprintf(stderr,"Z5 terminal gate0 clearing detecting trap slot=%d raw=%u xy=%d,%d hp=%d accepted=%d; retain source goal=%d,%d frame=%u\n",
                                trap_slot,b[trap_slot].raw_type,b[trap_slot].x,
                                b[trap_slot].y,b[trap_slot].hp,accepted,
                                source_goal_x,source_goal_y,(unsigned)frame);
                        target_changed = 0;
                        reset_target_progress();
                        continue;
                    }
                    if (!race_z5.cloak.active && !kerrigan_near_detector() &&
                        u[k].carry >= SC64_Z5_CLOAK_ACTIVATION_COST_Q8) {
                        if (set_kerrigan_cloak(1))
                            cloak_controller_preloaded = 1;
                    }
                    accepted = order_kerrigan_attack_move_to(source_goal_x,
                                                              source_goal_y);
                    target_force_attack = accepted;
                    fprintf(stderr, "Z5 terminal gate command trigger=0 AttackMove accepted=%d source_goal=%d,%d order=%d unit_target=%d,%d frame=%u\n",
                            accepted, source_goal_x, source_goal_y,
                            u[k].order, u[k].tx, u[k].ty, (unsigned)frame);
                } else {
                    accepted = order_kerrigan_move_to(source_goal_x,
                                                      source_goal_y);
                }
                if (!accepted)
                    fprintf(stderr, "Z5 controller could not send Kerrigan to final source region at frame=%u\n",
                            (unsigned)frame);
            } else {
                int lings_remain = 0;
                for (int i = 0; i < MAX_UNITS; ++i)
                    lings_remain += u[i].alive && u[i].owner == SIDE &&
                                    u[i].raw_type == 37;
                if (!controller_target_hero_only && lings_remain > 0 &&
                    order_combat_group_to(goal_x, goal_y,
                    target_trigger >= 24)) {
                    /* Attack-move the available combat group to clear the
                     * source-authenticated area while advancing. */
                } else if (!controller_target_hero_only && !lings_remain &&
                           (target_force_attack ?
                            order_kerrigan_attack_move_to(goal_x, goal_y) :
                            order_kerrigan_move_to(goal_x, goal_y))) {
                    /* Kerrigan first uses a direct MOVE while cloaked. If
                     * she truly stops, the stall guard retries that gate in
                     * attack-move so hostile blockers can be cleared. */
                } else if (!lings_remain &&
                           order_kerrigan_move_to(goal_x, goal_y)) {
                    /* The hero is the only remaining movable source actor. */
                } else {
                fprintf(stderr, "Z5 controller could not issue Zergling movement at frame=%u\n",
                        (unsigned)frame);
                if (controller_retreat_requested) continue;
                break;
                }
            }
            target_changed = 0;
            reset_target_progress();
        }
        step(0);
        if (host_ticks % 12u == 0u) {
            manage_kerrigan_cloak();
            manage_kerrigan_consume();
        }
        if (screen == SCREEN_PLAY && race_z5_world_paused() &&
            !race_z5.pause_acknowledged)
            tap(KEY_A); /* Explicitly acknowledge the authored PauseGame event. */
    }

    dump_navigation_frontier(kerrigan_slot());
    checkpoint(won && screen == SCREEN_RESULT ? "won-result" : lost ? "lost" : "limit");
    int verified = won && !lost && screen == SCREEN_RESULT;
    replay_state_event(verified ? "won" : lost ? "lost" : "limit");
    fputs("]}\n", replay_trace);
    fclose(replay_trace);
    const char *replay_output = verified ? verified_path :
        ".local/sc64/z5-controller-attempt-input.json";
    assert(rename(replay_temp_path, replay_output) == 0);
    fprintf(trace, "]}\n");
    fclose(trace);

    FILE *attempt = fopen(attempt_path, "w");
    assert(attempt);
    fprintf(attempt,
        "{\"schema\":1,\"mission\":16,\"result\":\"%s\",\"frame\":%u,"
        "\"won\":%s,\"lost\":%s,\"screen_result\":%s,\"outcome\":%u,"
        "\"fired\":%llu,\"started_lo\":%u,\"started_hi\":%u,"
        "\"action_cursors\":[",
        verified ? "verified-normal-result-win" : lost ? "failed-loss" : "failed-limit",
        (unsigned)frame, won ? "true" : "false", lost ? "true" : "false",
        screen == SCREEN_RESULT ? "true" : "false",
        (unsigned)race_z5.runtime.model.outcome,
        (unsigned long long)race_z5.runtime.model.started_lo |
            ((unsigned long long)race_z5.runtime.model.started_hi << 32),
        (unsigned)race_z5.runtime.model.started_lo,
        (unsigned)race_z5.runtime.model.started_hi);
    for (unsigned i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i)
        fprintf(attempt, "%s%u", i ? "," : "",
                (unsigned)race_z5.runtime.model.action_cursor[i]);
    fputs("],\"action_ticks\":[", attempt);
    for (unsigned i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i)
        fprintf(attempt, "%s%u", i ? "," : "",
                (unsigned)race_z5.runtime.action_ticks[i]);
    int final_k = kerrigan_slot();
    fprintf(attempt,
        "],\"switch_bits\":[%u,%u,%u,%u,%u,%u,%u,%u],\"switch110\":%u,"
        "\"source_paused\":%u,\"pause_ack\":%u,\"fault\":%u,"
        "\"hero\":{\"slot\":%d,\"alive\":%d,\"raw\":%d,\"owner\":%d,"
        "\"hp\":%d,\"shield\":%d,\"energy_q8\":%d,\"x\":%d,\"y\":%d,"
        "\"order\":%d,\"target_x\":%d,\"target_y\":%d,\"cloak\":%u},"
        "\"cursor_x\":%d,\"cursor_y\":%d,\"orders\":%d,"
        "\"trace\":\"%s\",\"replay_trace\":\"%s\"}\n",
        (unsigned)race_z5.runtime.switch_bits[0],
        (unsigned)race_z5.runtime.switch_bits[1],
        (unsigned)race_z5.runtime.switch_bits[2],
        (unsigned)race_z5.runtime.switch_bits[3],
        (unsigned)race_z5.runtime.switch_bits[4],
        (unsigned)race_z5.runtime.switch_bits[5],
        (unsigned)race_z5.runtime.switch_bits[6],
        (unsigned)race_z5.runtime.switch_bits[7],
        (unsigned)((race_z5.runtime.switch_bits[3] >> 14) & 1u),
        (unsigned)race_z5.runtime.source_paused,
        (unsigned)race_z5.pause_acknowledged,
        (unsigned)race_z5.runtime.fault, final_k,
        final_k >= 0 && u[final_k].alive,
        final_k >= 0 ? u[final_k].raw_type : -1,
        final_k >= 0 ? u[final_k].raw_owner : -1,
        final_k >= 0 ? u[final_k].hp : 0,
        final_k >= 0 ? u[final_k].shield : 0,
        final_k >= 0 ? u[final_k].carry : 0,
        final_k >= 0 ? u[final_k].x : -1,
        final_k >= 0 ? u[final_k].y : -1,
        final_k >= 0 ? u[final_k].order : -1,
        final_k >= 0 ? u[final_k].tx : -1,
        final_k >= 0 ? u[final_k].ty : -1,
        (unsigned)race_z5.cloak.active, cx, cy, order_count, trace_path,
        replay_output);
    fclose(attempt);
    printf("Z5 controller attempt=%s frame=%u result=%d/%d screen=%d Kerrigan=%d orders=%d trace=%s attempt=%s\n",
           verified ? "VERIFIED-WIN" : lost ? "LOSS" : "LIMIT", (unsigned)frame,
           won, lost, screen, kerrigan_slot(), order_count, replay_output, attempt_path);
    return verified ? 0 : 1;
}
